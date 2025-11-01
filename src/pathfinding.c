/*
 * pathfinding.c
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
 *
 * NLarn is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NLarn is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "extdefs.h"
#include "pathfinding.h"
#include "player.h"

static path *path_new(position start, position goal);
static path_element *path_element_new(position pos);
static guint path_step_cost(map *m, const path_element* element,
    map_element_t map_elem, bool for_player);
static guint path_cost(path_element* element, position target);
static path_element *path_element_in_list(const path_element* el, const GPtrArray *list);
static path_element *path_find_best(const path *pt);
static GPtrArray *path_get_neighbours(map *m, position pos,
    map_element_t element, bool for_player);

path *path_find(map *m, position start, position goal, map_element_t element)
{
    g_assert(m != NULL);
    g_assert(pos_valid(start));
    g_assert(pos_valid(goal));
    g_assert(element < LE_MAX);

    /* if the starting position is on another map, fail for now */
    /* TODO: could be changed to support 3D path finding */
    if (Z(start) != Z(goal))
        return NULL;

    path *pt = path_new(start, goal);

    /* add start to open list */
    path_element *curr = path_element_new(start);
    g_ptr_array_add(pt->open, curr);

    /* check if the path is being determined for the player */
    bool for_player = pos_identical(start, nlarn->p->pos);

    while (pt->open->len)
    {
        curr = path_find_best(pt);

        g_ptr_array_remove_fast(pt->open, curr);
        g_ptr_array_add(pt->closed, curr);

        if (pos_identical(curr->pos, pt->goal))
        {
            /* arrived at goal - reconstruct path */
            do {
                /* don't need the starting point in the path */
                if (curr->parent != NULL)
                    g_queue_push_head(pt->path, curr);

                curr = curr->parent;
            } while (curr != NULL);

            return pt;
        }

        GPtrArray *neighbours = path_get_neighbours(m, curr->pos, element, for_player);

        while (neighbours->len)
        {
            path_element *next = g_ptr_array_remove_index_fast(neighbours,
                                                neighbours->len - 1);

            bool next_is_better = false;

            if (path_element_in_list(next, pt->closed))
            {
                g_free(next);
                continue;
            }

            const guint32 next_g_score = curr->g_score
                + path_step_cost(m, next, element, for_player);

            if (!path_element_in_list(next, pt->open))
            {
                g_ptr_array_add(pt->open, next);
                next_is_better = true;
            }
            else if (next->g_score > next_g_score)
            {
                next_is_better = true;
            }
            else
            {
                g_free(next);
            }

            if (next_is_better)
            {
                next->parent  = curr;
                next->g_score = next_g_score;
            }
        }

        g_ptr_array_free(neighbours, true);
    }

    /* could not find a path */
    path_destroy(pt);

    return NULL;
}

void path_destroy(path *path)
{
    g_assert(path != NULL);

    /* clean up open list */
    for (guint idx = 0; idx < path->open->len; idx++)
    {
        g_free(g_ptr_array_index(path->open, idx));
    }
    g_ptr_array_free(path->open, true);

    for (guint idx = 0; idx < path->closed->len; idx++)
    {
        g_free(g_ptr_array_index(path->closed, idx));
    }
    g_ptr_array_free(path->closed, true);

    g_queue_free(path->path);
    g_free(path);
}

static path *path_new(position start, position goal)
{
    g_assert(pos_valid(start));
    g_assert(pos_valid(goal));

    path *pt = g_malloc0(sizeof(path));

    pt->open   = g_ptr_array_new();
    pt->closed = g_ptr_array_new();
    pt->path   = g_queue_new();

    pt->start = start;
    pt->goal  = goal;

    return pt;
}

static path_element *path_element_new(position pos)
{
    g_assert(pos_valid(pos));

    path_element *lpe = g_malloc0(sizeof(path_element));
    lpe->pos = pos;

    return lpe;
}

/* calculate the cost of stepping into this new field */
static guint path_step_cost(map *m, const path_element* element,
    map_element_t map_elem, bool for_player)
{
    map_tile_t tt;
    guint32 step_cost = 1; /* at least 1 movement cost */

    /* get the tile type of the map tile */
    if (for_player)
    {
        tt = player_memory_of(nlarn->p, element->pos).type ;
    }
    else
    {
        tt = map_tiletype_at(m, element->pos);
    }

    /* penalize for traps known to the player */
    if (for_player && player_memory_of(nlarn->p, element->pos).trap)
    {
        const trap_t trap = map_trap_at(m, element->pos);
        /* especially ones that may cause detours */
        if (trap == TT_TELEPORT || trap == TT_TRAPDOOR)
            step_cost += 50;
        else
            step_cost += 10;
    }

    /* penalize fields occupied by monsters: always for monsters,
       for the player only if (s)he can see the monster */
    monster *mon = map_get_monster_at(m, element->pos);
    if (mon != NULL && (!for_player || monster_in_sight(mon)))
    {
        step_cost += 10;
    }

    /* penalize fields covered with water, fire or cloud */
    switch (tt)
    {
    case LT_WATER:
        if (map_elem == LE_SWIMMING_MONSTER || map_elem == LE_FLYING_MONSTER)
            break;
        /* else fall through */
    case LT_FIRE:
    case LT_CLOUD:
        step_cost += 50;
        break;
    default:
        break;
    }

    return step_cost;
}

/* Returns the total estimated cost of the best path going
   through this new field */
static guint path_cost(path_element* element, position target)
{
    /* estimate the distance from the current position to the target */
    element->h_score = pos_distance(element->pos, target);

    return element->g_score + element->h_score;
}

static path_element *path_element_in_list(const path_element* el, const GPtrArray *list)
{
    g_assert(el != NULL && list != NULL);

    for (guint idx = 0; idx < list->len; idx++)
    {
        path_element *li = g_ptr_array_index(list, idx);

        if (pos_identical(li->pos, el->pos))
            return li;
    }

    return NULL;
}

static path_element *path_find_best(const path *pt)
{
    path_element *best = NULL;

    for (guint idx = 0; idx < pt->open->len; idx++)
    {
        path_element *el = g_ptr_array_index(pt->open, idx);

        if (best == NULL || path_cost(el, pt->goal)
                < path_cost(best, pt->goal))
        {
            best = el;
        }
    }

    return best;
}

static GPtrArray *path_get_neighbours(map *m, position pos,
                                      map_element_t element,
                                      bool for_player)
{
    GPtrArray *neighbours = g_ptr_array_new();

    for (direction dir = GD_NONE + 1; dir < GD_MAX; dir++)
    {
        if (dir == GD_CURR)
            continue;

        position new_pos = pos_move(pos, dir);

        if (!pos_valid(new_pos))
            continue;

        if ((for_player && mt_is_passable(player_memory_of(nlarn->p, new_pos).type))
                || (!for_player && monster_valid_dest(m, new_pos, element)))
        {
            path_element *pe = path_element_new(new_pos);
            g_ptr_array_add(neighbours, pe);
        }
    }

    return neighbours;
}
