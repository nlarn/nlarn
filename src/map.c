/*
 * map.c
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#include <glib.h>
#include <stdlib.h>

#include "container.h"
#include "display.h"
#include "items.h"
#include "map.h"
#include "nlarn.h"
#include "random.h"
#include "sobjects.h"
#include "spheres.h"

static int map_fill_with_stationary_objects(map *maze);
static void map_fill_with_objects(map *m);
static void map_fill_with_traps(map *m);

static gboolean map_load_from_file(map *m, const char *mazefile, guint which);
static void map_make_maze(map *m, int treasure_room);
static void map_make_maze_eat(map *m, int x, int y);
static void map_make_river(map *m, map_tile_t rivertype);
static void map_make_lake(map *m, map_tile_t laketype);
static void map_make_treasure_room(map *m, rectangle **rooms);
static int map_validate(map *m);

static map_path *map_path_new(position start, position goal);
static map_path_element *map_path_element_new(position pos);
static int map_step_cost(map *m, map_path_element* element,
                         map_element_t map_elem, gboolean ppath);
static int map_path_cost(map_path_element* element, position target);
static map_path_element *map_path_element_in_list(map_path_element* el,
                                                  GPtrArray *list);
static map_path_element *map_path_find_best(map_path *path);
static GPtrArray *map_path_get_neighbours(map *m, position pos,
                                          map_element_t element,
                                          gboolean ppath);

static inline void map_sphere_destroy(sphere *s, map *m __attribute__((unused)))
{
    sphere_destroy(s, nlarn);
}

const map_tile_data map_tiles[LT_MAX] =
{
    /* type         gly  color          desc           pa tr */
    { LT_NONE,      ' ', DC_NONE,       NULL,          0, 0 },
    { LT_MOUNTAIN,  '^', DC_LIGHTGRAY,  "a mountain",  0, 0 },
    { LT_GRASS,     '"', DC_LIGHTGREEN, "grass",       1, 1 },
    { LT_DIRT,      '.', DC_BROWN,      "dirt",        1, 1 },
    { LT_TREE,      '&', DC_GREEN,      "a tree",      0, 0 },
    { LT_FLOOR,     '.', DC_LIGHTGRAY,  "floor",       1, 1 },
    { LT_WATER,     '~', DC_LIGHTBLUE,  "water",       1, 1 },
    { LT_DEEPWATER, '~', DC_BLUE,       "deep water",  0, 1 },
    { LT_LAVA,      '~', DC_RED,        "lava",        0, 1 },
    { LT_FIRE,      '*', DC_RED,        "fire",        1, 1 },
    { LT_CLOUD,     '*', DC_WHITE,      "a gas cloud", 1, 1 },
    { LT_WALL,      '#', DC_LIGHTGRAY,  "a wall",      0, 0 },
};

/* keep track which levels have been used before */
static int map_used[MAP_MAZE_NUM + 1] = { 1, 0 };

const char *map_names[MAP_MAX] =
{
    "Town",
    "D1",
    "D2",
    "D3",
    "D4",
    "D5",
    "D6",
    "D7",
    "D8",
    "D9",
    "D10",
    "V1",
    "V2",
    "V3"
};

static gboolean is_town(int nlevel)
{
    return (nlevel == 0);
}

static gboolean is_dungeon_bottom(int nlevel)
{
    return (nlevel == MAP_DMAX - 1);
}

static gboolean is_volcano_bottom(int nlevel)
{
    return (nlevel == MAP_MAX - 1);
}

static gboolean is_volcano_map(int nlevel)
{
    return (nlevel >= MAP_DMAX);
}

map *map_new(int num, char *mazefile)
{
    gboolean map_loaded = FALSE;
    gboolean keep_maze = TRUE;

    map *nmap = nlarn->maps[num] = g_malloc0(sizeof(map));
    nmap->nlevel = num;

    /* create map */
    if ((num == 0) /* town is stored in file */
            || is_dungeon_bottom(num) /* level 10 */
            || is_volcano_bottom(num) /* volcano level 3 */
            || (num > 1 && chance(25)))
    {
        /* read maze from data file */
        map_loaded = map_load_from_file(nmap, mazefile, (num == 0) ? 0 : -1);

        /* add stationary objects (not to the town) */
        if (num > 0)
        {
            if (!map_fill_with_stationary_objects(nmap))
            {
                /* adding stationary objects failed; generate a new map */
                map_destroy(nmap);
                return NULL;
            }
        }
    }

    if (!map_loaded)
    {
        /* determine if to add treasure room */
        gboolean treasure_room = num > 1 && chance(25);

        /* generate random map */
        do
        {
            /* dig cave */
            map_make_maze(nmap, treasure_room);

            /* check if entire map is reachable */
            keep_maze = map_validate(nmap);
        }
        while (!keep_maze);
    }

    if (num != 0)
    {
        /* home town is not filled with crap */
        map_fill_with_objects(nmap);

        /* and not trapped */
        map_fill_with_traps(nmap);
    }

    /* add inhabitants to the map */
    map_fill_with_life(nmap);

    return nmap;
}

cJSON *map_serialize(map *m)
{
    cJSON *mser, *grid, *tile;

    mser = cJSON_CreateObject();

    cJSON_AddNumberToObject(mser, "nlevel", m->nlevel);
    cJSON_AddNumberToObject(mser, "visited", m->visited);

    cJSON_AddItemToObject(mser, "grid", grid = cJSON_CreateArray());

    for (int y = 0; y < MAP_MAX_Y; y++)
    {
        for (int x = 0; x < MAP_MAX_X; x++)
        {
            cJSON_AddItemToArray(grid, tile = cJSON_CreateObject());

            cJSON_AddNumberToObject(tile, "type", m->grid[y][x].type);

            if (m->grid[y][x].base_type > 0
                    && m->grid[y][x].base_type != m->grid[y][x].type)
            {
                cJSON_AddNumberToObject(tile, "base_type",
                                        m->grid[y][x].base_type);
            }

            if (m->grid[y][x].sobject)
            {
                cJSON_AddNumberToObject(tile, "sobject",
                                        m->grid[y][x].sobject);
            }

            if (m->grid[y][x].trap)
            {
                cJSON_AddNumberToObject(tile, "trap",
                                        m->grid[y][x].trap);
            }

            if (m->grid[y][x].timer)
            {
                cJSON_AddNumberToObject(tile, "timer",
                                        m->grid[y][x].timer);
            }

            if (m->grid[y][x].m_oid)
            {
                cJSON_AddNumberToObject(tile, "monster",
                                        GPOINTER_TO_UINT(m->grid[y][x].m_oid));
            }

            if (m->grid[y][x].ilist )
            {
                cJSON_AddItemToObject(tile, "inventory",
                                      inv_serialize(m->grid[y][x].ilist));
            }
        }
    }

    return mser;
}

map *map_deserialize(cJSON *mser)
{
    cJSON *grid, *tile, *obj;
    map *m;

    m = g_malloc0(sizeof(map));

    m->nlevel = cJSON_GetObjectItem(mser, "nlevel")->valueint;
    m->visited = cJSON_GetObjectItem(mser, "visited")->valueint;

    grid = cJSON_GetObjectItem(mser, "grid");

    for (int y = 0; y < MAP_MAX_Y; y++)
    {
        for (int x = 0; x < MAP_MAX_X; x++)
        {
            tile = cJSON_GetArrayItem(grid, x + (y * MAP_MAX_X));

            m->grid[y][x].type = cJSON_GetObjectItem(tile, "type")->valueint;

            obj = cJSON_GetObjectItem(tile, "base_type");
            if (obj != NULL) m->grid[y][x].base_type = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "sobject");
            if (obj != NULL) m->grid[y][x].sobject = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "trap");
            if (obj != NULL) m->grid[y][x].trap = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "timer");
            if (obj != NULL) m->grid[y][x].timer = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "monster");
            if (obj != NULL) m->grid[y][x].m_oid = GUINT_TO_POINTER(obj->valueint);

            obj = cJSON_GetObjectItem(tile, "inventory");
            if (obj != NULL) m->grid[y][x].ilist = inv_deserialize(obj);
        }
    }

    return m;
}

char *map_dump(map *m, position ppos)
{
    position pos = pos_invalid;
    GString *dump;
    monster *mon;

    dump = g_string_new_len(NULL, MAP_SIZE);

    Z(pos) = m->nlevel;

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            if (pos_identical(pos, ppos))
            {
                g_string_append_c(dump, '@');
            }
            else if ((mon = map_get_monster_at(m, pos)))
            {
                g_string_append_c(dump, monster_glyph(mon));
            }
            else if (map_trap_at(m, pos))
            {
                g_string_append_c(dump, '^');
            }
            else if (map_sobject_at(m, pos))
            {
                g_string_append_c(dump, so_get_glyph(map_sobject_at(m, pos)));
            }
            else
            {
                g_string_append_c(dump, mt_get_glyph(map_tiletype_at(m, pos)));
            }
        }
        g_string_append_c(dump, '\n');
    }

    return g_string_free(dump, FALSE);
}

void map_destroy(map *m)
{
    g_assert(m != NULL);

    /* destroy spheres on this level */
    g_ptr_array_foreach(nlarn->spheres, (GFunc)map_sphere_destroy, m);

    /* destroy items and monsters */
    for (int y = 0; y < MAP_MAX_Y; y++)
        for (int x = 0; x < MAP_MAX_X; x++)
        {
            if (m->grid[y][x].m_oid != NULL) {
                monster *mon = game_monster_get(nlarn, m->grid[y][x].m_oid);

                /* I wonder why it is possible that a monster ID is stored at
                 * a position while there is no matching monster registered.
                 * This seems to occur when called by game_destroy(). */
                if (mon != NULL) monster_destroy(mon);
            }

            if (m->grid[y][x].ilist != NULL)
                inv_destroy(m->grid[y][x].ilist, TRUE);
        }

    g_free(m);
}

/* return coordinates of a free space */
position map_find_space(map *m, map_element_t element, gboolean dead_end)
{
    rectangle entire_map = rect_new(1, 1, MAP_MAX_X - 2, MAP_MAX_Y - 2);
    return map_find_space_in(m, entire_map, element, dead_end);
}

position map_find_space_in(map *m,
                           rectangle where,
                           map_element_t element,
                           gboolean dead_end)
{
    position pos = pos_invalid;
    int count, iteration = 0;

    g_assert (m != NULL && element < LE_MAX);

    X(pos) = rand_m_n(where.x1, where.x2);
    Y(pos) = rand_m_n(where.y1, where.y2);
    Z(pos) = m->nlevel;

    /* number of positions inside the rectangle */
    count = (where.x2 - where.x1 + 1) * (where.y2 - where.y1 + 1);

    do
    {
        X(pos)++;

        if (X(pos) > where.x2)
        {
            X(pos) = where.x1;
            Y(pos)++;
        }

        if (Y(pos) > where.y2)
        {
            Y(pos) = where.y1;
        }

        iteration++;
    }
    while (!map_pos_validate(m, pos, element, dead_end) && (iteration <= count));

    if (iteration > count )
        pos = pos_invalid;

    return pos;
}

int *map_get_surrounding(map *m, position pos, sobject_t type)
{
    position p = pos_invalid;
    int nmove = 1;
    int *dirs;

    dirs = g_malloc0(sizeof(int) * GD_MAX);

    while (nmove < GD_MAX)
    {
        p = pos_move(pos, nmove);

        if (pos_valid(p) && map_sobject_at(m, p) == type)
        {
            dirs[nmove] = TRUE;
        }

        nmove++;
    }

    return dirs;
}

position map_find_sobject(map *m, sobject_t sobject)
{
    position pos = pos_invalid;

    g_assert(m != NULL);

    Z(pos) = m->nlevel;

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
            if (map_sobject_at(m, pos) == sobject)
                return pos;

    /* if we reach this point, the sobject is not on the map */
    return pos_invalid;
}

gboolean map_pos_validate(map *m, position pos, map_element_t element,
                          int dead_end)
{
    map_tile *tile;

    g_assert(m != NULL && element < LE_MAX);

    /* if the position is invalid it is invalid for the map as well */
    if (!pos_valid(pos))
        return FALSE;

    /* if the position is on another map it is invalid for this level */
    if (Z(pos) != m->nlevel)
        return FALSE;

    /* make shortcut */
    tile = map_tile_at(m, pos);

    /* check for an dead end */
    if (dead_end)
    {
        int wall_count = 0;
        position p = pos;

        for (Y(p) = Y(pos) -1; Y(p) < Y(pos) + 2; Y(p)++)
            for (X(p) = X(pos) -1; X(p) < X(pos) + 2; X(p)++)
                if (map_tiletype_at(m, p) == LT_WALL)
                    wall_count++;

        if (wall_count < 7)
        {
            /* not enclosed by walls */
            return FALSE;
        }
    }

    switch (element)
    {
    case LE_GROUND:
        return mt_is_passable(tile->type);
        break;

    case LE_SOBJECT:
        if (mt_is_passable(tile->type) && (tile->sobject == LS_NONE))
        {
            /* find free space */
            position p = pos;

            for (Y(p) = Y(pos) -1; Y(p) < Y(pos) + 2; Y(p)++)
                for (X(p) = X(pos) -1; X(p) < X(pos) + 2; X(p)++)
                {
                    if (map_sobject_at(m, p) != LS_NONE)
                        return FALSE;
                }

            return TRUE;
        }
        break;

    case LE_TRAP:
        return (mt_is_passable(tile->type)
                && (tile->sobject == LS_NONE)
                && (tile->trap == TT_NONE));
        break;

    case LE_ITEM:
        /* we can stack like mad, so we only need to check if
         * there is an open space */
        return (map_pos_passable(m, pos) && (tile->sobject == LS_NONE));
        break;

    case LE_MONSTER:
    case LE_SWIMMING_MONSTER:
    case LE_FLYING_MONSTER:
    case LE_XORN:
        /* not OK if player is standing on that tile */
        if (pos_identical(pos, nlarn->p->pos))
            return FALSE;

        if (map_is_monster_at(m, pos))
            return FALSE;

        return monster_valid_dest(m, pos, element);
        break;

    case LE_MAX:
        return FALSE;
        break;

    } /* switch */

    return FALSE;
}

int map_pos_is_visible(map *m, position s, position t)
{
    int delta_x, delta_y;
    int x, y;
    signed int ix, iy;

    /* positions on different levels? */
    if (Z(s) != Z(t))
        return FALSE;

    x = X(s);
    y = Y(s);

    delta_x = abs(X(t) - X(s)) << 1;
    delta_y = abs(Y(t) - Y(s)) << 1;

    /* if x1 == x2 or y1 == y2, then it does not matter what we set here */
    ix = X(t) > X(s) ? 1 : -1;
    iy = Y(t) > Y(s) ? 1 : -1;

    if (delta_x >= delta_y)
    {
        /* error may go below zero */
        int error = delta_y - (delta_x >> 1);

        while (x != X(t))
        {
            if (error >= 0)
            {
                if (error || (ix > 0))
                {
                    y += iy;
                    error -= delta_x;
                }
            }

            x += ix;
            error += delta_y;

            if (!mt_is_transparent(m->grid[y][x].type)
                    || !so_is_transparent(m->grid[y][x].sobject))
            {
                return FALSE;
            }
        }
    }
    else
    {
        /* error may go below zero */
        int error = delta_x - (delta_y >> 1);

        while (y != Y(t))
        {
            if (error >= 0)
            {
                if (error || (iy > 0))
                {
                    x += ix;
                    error -= delta_y;
                }
            }

            y += iy;
            error += delta_x;

            if (!mt_is_transparent(m->grid[y][x].type)
                    || !so_is_transparent(m->grid[y][x].sobject))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

map_path *map_find_path(map *m, position start, position goal,
                        map_element_t element)
{
    g_assert(m != NULL);

    map_path *path;
    map_path_element *curr, *next;
    gboolean next_is_better;

    /* if the starting position is on another map, fail for now */
    /* TODO: could be changed to support 3D path finding */
    if (Z(start) != Z(goal))
        return NULL;

    path = map_path_new(start, goal);

    /* add start to open list */
    curr = map_path_element_new(start);
    curr->g_score = 0; /* no distance yet */
    g_ptr_array_add(path->open, curr);

    /* check if the path is being determined for the player */
    gboolean ppath = pos_identical(start, nlarn->p->pos);

    while (path->open->len)
    {
        curr = map_path_find_best(path);

        g_ptr_array_remove_fast(path->open, curr);
        g_ptr_array_add(path->closed, curr);

        if (pos_identical(curr->pos, path->goal))
        {
            /* arrived at goal */

            /* reconstruct path */
            do
            {
                /* don't need the starting point in the path */
                if (curr->parent != NULL)
                    g_queue_push_head(path->path, curr);

                curr = curr->parent;
            }
            while (curr != NULL);

            return path;
        }

        GPtrArray *neighbours = map_path_get_neighbours(m, curr->pos, element, ppath);

        while (neighbours->len)
        {
            next = g_ptr_array_remove_index_fast(neighbours,
                                                 neighbours->len - 1);

            next_is_better = FALSE;

            if (map_path_element_in_list(next, path->closed))
            {
                g_free(next);
                continue;
            }

            const guint32 next_g_score =
                curr->g_score
                + map_step_cost(m, next, element, ppath);

            if (!map_path_element_in_list(next, path->open))
            {
                g_ptr_array_add(path->open, next);
                next_is_better = TRUE;
            }
            else if (next->g_score > next_g_score)
            {
                next_is_better = TRUE;
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

        g_ptr_array_free(neighbours, TRUE);
    }

    /* could not find a path */
    map_path_destroy(path);

    return NULL;
}

void map_path_destroy(map_path *path)
{
    g_assert(path != NULL);

    /* clean up open list */
    for (guint idx = 0; idx < path->open->len; idx++)
    {
        g_free(g_ptr_array_index(path->open, idx));
    }

    g_ptr_array_free(path->open, TRUE);

    for (guint idx = 0; idx < path->closed->len; idx++)
    {
        g_free(g_ptr_array_index(path->closed, idx));
    }

    g_ptr_array_free(path->closed, TRUE);

    g_queue_free(path->path);

    g_free(path);
}

GList *map_ray(map *m, position source, position target)
{
    GList *ray = NULL;
    int delta_x, delta_y;
    int inc_x, inc_y;
    position pos = source;

    /* Insert the source position */
    ray = g_list_append(ray, GUINT_TO_POINTER(source.val));

    delta_x = abs(X(target) - X(source)) << 1;
    delta_y = abs(Y(target) - Y(source)) << 1;

    /* if x1 == x2 or y1 == y2, then it does not matter what we set here */
    inc_x = X(target) > X(source) ? 1 : -1;
    inc_y = Y(target) > Y(source) ? 1 : -1;

    if (delta_x >= delta_y)
    {
        /* error may go below zero */
        int error = delta_y - (delta_x >> 1);

        while (X(pos) != X(target))
        {
            if (error >= 0)
            {
                if (error || (inc_x > 0))
                {
                    Y(pos) += inc_y;
                    error -= delta_x;
                }
            }

            X(pos) += inc_x;
            error += delta_y;

            /* append even the last position to the list */
            ray = g_list_append(ray, GUINT_TO_POINTER(pos.val));

            if (!map_pos_transparent(m, pos))
                break; /* stop following ray */
        }
    }
    else
    {
        /* error may go below zero */
        int error = delta_x - (delta_y >> 1);

        while (Y(pos) != Y(target))
        {
            if (error >= 0)
            {
                if (error || (inc_y > 0))
                {
                    X(pos) += inc_x;
                    error -= delta_y;
                }
            }

            Y(pos) += inc_y;
            error += delta_x;

            /* append even the last position to the list */
            ray = g_list_append(ray, GUINT_TO_POINTER(pos.val));

            if (!map_pos_transparent(m, pos))
                break; /* stop following ray */
        }
    }

    if (ray && GPOINTER_TO_UINT(g_list_last(ray)->data) != target.val)
    {
        g_list_free(ray);
        ray = NULL;
    }

    return ray;
}

gboolean map_trajectory(position source, position target,
                        const damage_originator * const damo,
                        trajectory_hit_sth pos_hitfun,
                        gpointer data1, gpointer data2, gboolean reflectable,
                        char glyph, int colour, gboolean keep_ray)
{
    g_assert(pos_valid(source) && pos_valid(target));

    map *tmap = game_map(nlarn, Z(source));

    /* get the ray */
    GList *ray = map_ray(tmap, source, target);
    GList *iter = ray;

    /* it was impossible to get a ray for the given positions */
    if (!ray)
        return FALSE;

    /* follow the ray to determine if it hits something */
    do
    {
        gboolean result = FALSE;
        position cursor;
        pos_val(cursor) = GPOINTER_TO_UINT(iter->data);

        /* skip the source position */
        if (pos_identical(source, cursor))
            continue;

        /* the position is affected, call the callback function */
        if (pos_hitfun(iter, damo, data1, data2))
        {
            /* the callback returned that the ray if finished */
            result = TRUE;
        }

        /* check for reflection: mirrors and the amulet of reflection */
        if (reflectable && ((map_sobject_at(tmap, cursor) == LS_MIRROR)
                || (pos_identical(cursor, nlarn->p->pos)
                            && player_effect(nlarn->p, ET_REFLECTION))))
        {
            g_list_free(ray);
            /* repaint the screen before showing the reflection, otherwise
             * the reflection wouldn't be visible! */
            display_paint_screen(nlarn->p);

            return map_trajectory(cursor, source, damo, pos_hitfun, data1,
                                  data2, FALSE,  glyph, colour, keep_ray);
        }

        /* after checking for reflection, abort the function if the
           callback indicated success */
        if (result == TRUE)
        {
            g_list_free(ray);
            return result;
        }

        /* show the position of the ray*/
        /* FIXME: move curses functions to display.c */
        attron(colour);
        (void)mvaddch(Y(cursor), X(cursor), glyph);
        attroff(colour);
        display_draw();

        /* sleep a while to show the ray's position */
        napms(100);
        /* repaint the screen unless requested otherwise */
        if (!keep_ray) display_paint_screen(nlarn->p);
    }
    while ((iter = iter->next));

    /* none of the trigger functions succeeded */
    g_list_free(ray);
    return FALSE;
}

area *map_get_obstacles(map *m, position center, int radius, gboolean doors)
{
    area *narea;
    position pos = pos_invalid;
    int x, y;

    g_assert(m != NULL);

    if (!pos_valid(center))
    {
        return NULL;
    }

    narea = area_new(X(center) - radius, Y(center) - radius,
                     radius * 2 + 1, radius * 2 + 1);

    Z(pos) = m->nlevel;

    for (Y(pos) = Y(center) - radius, y = 0;
         Y(pos) <= Y(center) + radius;
         Y(pos)++, y++)
    {
        for (X(pos) = X(center) - radius, x = 0;
             X(pos) <= X(center) + radius;
             X(pos)++, x++)
        {
            if (!pos_valid(pos))
            {
                area_point_set(narea, x, y);
                continue;
            }
            if (doors && map_sobject_at(m, pos) == LS_CLOSEDDOOR)
                continue;
            else if (!map_pos_transparent(m, pos))
                area_point_set(narea, x, y);
        }
    }

    return narea;
}

void map_set_tiletype(map *m, area *ar, map_tile_t type, guint8 duration)
{
    position pos = pos_invalid;
    int x, y;

    g_assert (m != NULL && ar != NULL);

    position center = pos_invalid;
    X(center) = ar->start_x + ar->size_x / 2;
    Y(center) = ar->start_y + ar->size_y / 2;
    Z(center) = m->nlevel;

    Z(pos) = m->nlevel;
    for (Y(pos) = ar->start_y, y = 0;
            Y(pos) < ar->start_y + ar->size_y;
            Y(pos)++, y++)
    {
        for (X(pos) = ar->start_x, x = 0;
                X(pos) < ar->start_x + ar->size_x;
                X(pos)++, x++)
        {
            /* check if pos is inside the map */
            if (!pos_valid(pos))
                continue;

            /* if the position is marked in area set the tile to type */
            if (area_point_get(ar, x, y))
            {
                map_tile *tile = map_tile_at(m, pos);

                /* store original type if it has not been set already
                   (this can occur when casting multiple flood
                   spells on the same tile) */
                if (tile->base_type == LT_NONE)
                    tile->base_type = map_tiletype_at(m, pos);

                tile->type = type;
                /* if non-permanent, let the radius shrink with time */
                if (duration != 0)
                    tile->timer = max(1, duration - 5 * pos_distance(pos, center));
            }
        }
    }
}

damage *map_tile_damage(map *m, position pos, gboolean flying)
{
    g_assert (m != NULL && pos_valid(pos));

    switch (map_tiletype_at(m, pos))
    {
    case LT_CLOUD:
        return damage_new(DAM_ACID, ATT_NONE, 3 + rand_0n(2), DAMO_MAP, NULL);
        break;

    case LT_FIRE:
        return damage_new(DAM_FIRE, ATT_NONE, 5 + rand_0n(2), DAMO_MAP, NULL);
        break;

    case LT_WATER:
        if (flying)
            return NULL;

        return damage_new(DAM_WATER, ATT_NONE, 4 + rand_0n(2), DAMO_MAP, NULL);
        break;

    default:
        return NULL;
    }
}

char *map_pos_examine(position pos)
{
    map *cm = game_map(nlarn, Z(pos));
    monster *monst;
    char *tmp = NULL;
    const char *where;
    GString *desc = g_string_new(NULL);

    g_assert(pos_valid(pos));

    if (pos_identical(pos, nlarn->p->pos))
        where = "here";
    else
        where = "there";

    /* describe the level tile */
    tmp = g_strdup(mt_get_desc(map_tiletype_at(cm, pos)));
    tmp[0] = g_ascii_toupper(tmp[0]);
    g_string_append_printf(desc, "%s. ", tmp);
    g_free(tmp);

    gboolean has_mimic = FALSE;
    /* add description of monster, if there is one on the tile */
    if ((monst = map_get_monster_at(cm, pos)))
    {
        if (game_wizardmode(nlarn) || monster_in_sight(monst))
        {
            tmp = monster_desc(monst);

            tmp[0] = g_ascii_toupper(tmp[0]);
            g_string_append_printf(desc, "%s. ", tmp);
            g_free(tmp);

            if (monster_unknown(monst))
                has_mimic = TRUE;
        }
    }

    /* add message if target tile contains a stationary object */
    if (map_sobject_at(cm, pos) > LS_NONE)
    {
        g_string_append_printf(desc, "You see %s %s. ",
                               so_get_desc(map_sobject_at(cm, pos)), where);
    }

    /* add message if target tile contains a known trap */
    if (player_memory_of(nlarn->p, pos).trap)
    {
        g_string_append_printf(desc, "There is %s %s %s. ",
                               a_an(trap_description(map_trap_at(cm, pos))),
                               trap_description(map_trap_at(cm, pos)), where);
    }

    /* add message if target tile contains items, but only if there's
       a mimic there (items don't stack correctly otherwise) */
    if (!has_mimic && inv_length(*map_ilist_at(cm, pos)) > 0)
    {
        if (inv_length(*map_ilist_at(cm, pos)) > 3)
        {
            g_string_append_printf(desc, "There are multiple items %s.", where);
        }
        else
        {
            GString *items_desc = NULL;

            for (guint idx = 0; idx < inv_length(*map_ilist_at(cm, pos)); idx++)
            {
                item *it = inv_get(*map_ilist_at(cm, pos), idx);
                gchar *item_desc = item_describe(it, player_item_known(nlarn->p, it),
                                                 FALSE, FALSE);

                if (idx > 0)
                    g_string_append_printf(items_desc, " and %s", item_desc);
                else
                    items_desc = g_string_new(item_desc);

                g_free(item_desc);
            }

            if (items_desc != NULL)
            {
                g_string_append_printf(desc, "You see %s %s.", items_desc->str, where);
                g_string_free(items_desc, TRUE);
            }
        }
    }

    return g_string_free(desc, FALSE);
}

monster *map_get_monster_at(map *m, position pos)
{
    g_assert(m != NULL && m->nlevel == Z(pos) && pos_valid(pos));

    gpointer mid = m->grid[Y(pos)][X(pos)].m_oid;
    return (mid != NULL) ? game_monster_get(nlarn, mid) : NULL;
}

void map_fill_with_life(map *m)
{
    position pos = pos_invalid;
    guint new_monster_count;

    g_assert(m != NULL);

    new_monster_count = rand_1n(14 + m->nlevel);

    if (m->nlevel == 0)
    {
        /* do not add an excessive amount of town people */
        new_monster_count = min(5, new_monster_count);
    }

    if (m->mcount > new_monster_count)
        /* no monsters added */
        return;
    else
        new_monster_count -= m->mcount;

    for (guint i = 0; i <= new_monster_count; i++)
    {
        do
        {
            pos = map_find_space(m, LE_MONSTER, FALSE);

            if (!pos_valid(pos))
            {
                /* it seems that the map is fully crowded,
                   thus abort monster creation. */
                return;
            }
        }
        while (fov_get(nlarn->p->fv, pos));

        monster_new_by_level(pos);
    }

    return;
}

gboolean map_is_exit_at(map *m, position pos)
{
    g_assert (m != NULL && pos_valid(pos));

    switch (map_sobject_at(m, pos))
    {
    case LS_DNGN_ENTRANCE:
    case LS_DNGN_EXIT:
    case LS_ELEVATORDOWN:
    case LS_ELEVATORUP:
    case LS_STAIRSUP:
    case LS_STAIRSDOWN:
        return TRUE;
        break;

    default:
        return FALSE;
        break;
    }
}

void map_timer(map *m)
{
    position pos = pos_invalid;
    item_erosion_type erosion;

    g_assert (m != NULL);

    Z(pos) = m->nlevel;

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            if (map_timer_at(m, pos))
            {
                map_tile *tile = map_tile_at(m, pos);
                tile->timer--;

                /* affect items every three turns */
                if ((tile->ilist != NULL) && (tile->timer % 5 == 0))
                {
                    switch (tile->type)
                    {
                    case LT_CLOUD:
                        erosion = IET_CORRODE;
                        break;

                    case LT_FIRE:
                        erosion = IET_BURN;
                        break;

                    case LT_WATER:
                        erosion = IET_RUST;
                        break;
                    default:
                        erosion = IET_NONE;
                        break;
                    }

                    inv_erode(&tile->ilist, erosion,
                            fov_get(nlarn->p->fv, pos), NULL);
                }

                /* reset tile type if temporary effect has expired */
                if (tile->timer == 0)
                {
                    if ((tile->type == LT_FIRE)
                            && (tile->base_type == LT_GRASS))
                    {
                        tile->base_type = LT_NONE;
                        tile->type = LT_DIRT;
                    }
                    else
                    {
                        tile->type = tile->base_type;
                    }
                }
            } /* if map_timer_at */
        } /* for X(pos) */
    } /* for Y(pos) */
}

char map_get_door_glyph(map *m, position pos)
{
    position n, e, s, w;

    g_assert(m != NULL && pos_valid(pos));

    n = pos_move(pos, GD_NORTH);
    e = pos_move(pos, GD_EAST);
    s = pos_move(pos, GD_SOUTH);
    w = pos_move(pos, GD_WEST);

    /* some predefined maps have double doors, thus it is
       necessary to check for adjacent doors */

    if (map_sobject_at(m, pos) == LS_CLOSEDDOOR)
    {
        /* #-# */
        if (((map_tiletype_at(m, e) == LT_WALL)
             || (map_sobject_at(m, e) == LS_CLOSEDDOOR)
             || (map_sobject_at(m, e) == LS_OPENDOOR))
             && ((map_tiletype_at(m, w) == LT_WALL)
             || (map_sobject_at(m, w) == LS_CLOSEDDOOR)
             || (map_sobject_at(m, w) == LS_OPENDOOR)))
             return '-';

        /* #
         * |
         * # */
        if (((map_tiletype_at(m, n) == LT_WALL)
             || (map_sobject_at(m, n) == LS_CLOSEDDOOR)
             || (map_sobject_at(m, n) == LS_OPENDOOR))
             && ((map_tiletype_at(m, s) == LT_WALL)
             || (map_sobject_at(m, s) == LS_CLOSEDDOOR)
             || (map_sobject_at(m, s) == LS_OPENDOOR)))
            return '|';

    }
    else
    {
    /* #-# */
    if ((map_sobject_at(m, e) == LS_CLOSEDDOOR)
         || (map_sobject_at(m, e) == LS_OPENDOOR))
         return '/';

    if ((map_sobject_at(m, w) == LS_CLOSEDDOOR)
         || (map_sobject_at(m, w) == LS_OPENDOOR))
         return '\\';

    /* #
     * |
     * # */
    if ((map_sobject_at(m, s) == LS_CLOSEDDOOR)
         || (map_sobject_at(m, s) == LS_OPENDOOR))
        return '\\';

    if ((map_sobject_at(m, n) == LS_CLOSEDDOOR)
         || (map_sobject_at(m, n) == LS_OPENDOOR))
        return '/';
    }

    /* no idea. */
    return so_get_glyph(map_sobject_at(m, pos));
}

static int map_fill_with_stationary_objects(map *m)
{
    position pos = pos_invalid;

    /* volcano shaft up from the temple */
    if (m->nlevel == MAP_DMAX)
    {
        pos = map_find_space(m, LE_SOBJECT, TRUE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_ELEVATORUP);
    }

    /*  make the fixed objects in the maze: STAIRS */
    if (!is_town(m->nlevel) && !is_dungeon_bottom(m->nlevel)
            && !is_volcano_bottom(m->nlevel))
    {
        pos = map_find_space(m, LE_SOBJECT, TRUE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_STAIRSDOWN);
    }

    if ((m->nlevel > 1) && (m->nlevel != MAP_DMAX))
    {
        pos = map_find_space(m, LE_SOBJECT, TRUE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_STAIRSUP);
    }

    /* make the random objects in the maze */
    /* 33 percent chance for an altar */
    if (chance(33))
    {
        pos = map_find_space(m, LE_SOBJECT, FALSE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_ALTAR);
    }

    /* up to three statues */
    for (guint i = 0; i < rand_0n(3); i++)
    {
        pos = map_find_space(m, LE_SOBJECT, FALSE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_STATUE);
    }

    /* up to three fountains */
    for (guint i = 0; i < rand_0n(3); i++)
    {
        pos = map_find_space(m, LE_SOBJECT, FALSE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_FOUNTAIN);
    }

    /* up to two thrones */
    for (guint i = 0; i < rand_0n(2); i++)
    {
        pos = map_find_space(m, LE_SOBJECT, FALSE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_THRONE);
    }

    /* up to two  mirrors */
    for (guint i = 0; i < rand_0n(2); i++)
    {
        pos = map_find_space(m, LE_SOBJECT, FALSE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_MIRROR);
    }

    if (m->nlevel == 5)
    {
        /* branch office of the bank */
        pos = map_find_space(m, LE_SOBJECT, TRUE);
        if (!pos_valid(pos)) return FALSE;
        map_sobject_set(m, pos, LS_BANK2);
    }

    return TRUE;
}

static void map_fill_with_objects(map *m)
{
    /* up to two pieces of armour */
    for (guint i = 0; i <= rand_0n(2); i++)
    {
        map_item_add(m, item_new_by_level(IT_ARMOUR, m->nlevel));
    }

    /* up to two amulets on levels > 5 */
    if (m->nlevel > 5)
    {
        for (guint i = 0; i <= rand_0n(2); i++)
            map_item_add(m, item_new_by_level(IT_AMULET, m->nlevel));
    }

    /* up to two piles of ammunition */
    for (guint i = 0; i <= rand_0n(2); i++)
    {
        map_item_add(m, item_new_by_level(IT_AMMO, m->nlevel));
    }

    /* up to three books */
    for (guint i = 0; i <= rand_0n(3); i++)
    {
        map_item_add(m, item_new_by_level(IT_BOOK, m->nlevel));
    }

    /* up to two containers */
    for (guint i = 1; i <= rand_0n(2); i++)
    {
        /* random container type */
        item *container = item_new(IT_CONTAINER, rand_1n(CT_MAX));

        /* up to 5 items inside the container */
        for (guint j = 0; j < rand_0n(5); j++)
        {
            item_t it;

            /* prevent containers inside the container */
            do
            {
                it = rand_1n(IT_MAX - 1);
            }
            while (it == IT_CONTAINER);

            inv_add(&(container->content), item_new_by_level(it, m->nlevel));
        }

        /* there is a chance that the container is trapped */
        if (chance(33))
        {
            container->cursed = TRUE;
        }

        /* add the container to the map */
        map_item_add(m, container);
    }

    /* up to 10 piles of gold */
    for (guint i = 0; i <= rand_0n(10); i++)
    {
        /* There is nothing like a newly minted pound. */
        map_item_add(m, item_new(IT_GOLD, rand_m_n(10, (m->nlevel + 1) * 15)));
    }

    /* up to three gems */
    for (guint i = 0; i <= rand_0n(3); i++)
    {
        map_item_add(m, item_new_random(IT_GEM, FALSE));
    }

    /* up to four potions */
    for (guint i = 0; i <= rand_0n(4); i++)
    {
        map_item_add(m, item_new_by_level(IT_POTION, m->nlevel));
    }

    /* up to three scrolls */
    for (guint i = 0; i <= rand_0n(3); i++)
    {
        map_item_add(m, item_new_by_level(IT_SCROLL, m->nlevel));
    }

    /* up to two rings */
    for (guint i = 0; i <= rand_0n(2); i++)
    {
        map_item_add(m, item_new_by_level(IT_RING, m->nlevel));
    }

    /* up to two weapons */
    for (guint i = 0; i <= rand_0n(2); i++)
    {
        map_item_add(m, item_new_by_level(IT_WEAPON, m->nlevel));
    }

} /* map_fill_with_objects */

static void map_fill_with_traps(map *m)
{
    g_assert(m != NULL);

    /* Trapdoor cannot be placed in the last dungeon map and the last volcano map */
    gboolean trapdoor = (!is_dungeon_bottom(m->nlevel)
            && !is_volcano_bottom(m->nlevel));

    for (guint count = 0; count < rand_0n((trapdoor ? 8 : 6)); count++)
    {
        position pos = map_find_space(m, LE_TRAP, FALSE);
        map_trap_set(m, pos, rand_1n(trapdoor ? TT_MAX : TT_TRAPDOOR));
    }
} /* map_fill_with_traps */

/* Subroutine to make the caverns for a given map. Only walls are made. */
static void map_make_maze(map *m, int treasure_room)
{
    position pos = pos_invalid;
    int mx, my;
    int nrooms;
    rectangle **rooms = NULL;
    gboolean want_monster = FALSE;

    g_assert (m != NULL);

    Z(pos) = m->nlevel;

generate:
    /* reset map by filling it with walls */
    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            monster *mon;

            map_tiletype_set(m, pos, LT_WALL);
            map_sobject_set(m, pos, LS_NONE);

            if ((mon = map_get_monster_at(m, pos)))
            {
                monster_destroy(mon);
            }

            if (map_tile_at(m, pos)->ilist != NULL)
            {
                inv_destroy(map_tile_at(m, pos)->ilist, TRUE);
                map_tile_at(m, pos)->ilist = NULL;
            }
        }

    /* Maybe add a river or lake. */
    const map_tile_t rivertype = (is_volcano_map(m->nlevel) ? LT_LAVA : LT_DEEPWATER);

    if (m->nlevel > 1
            && (is_volcano_map(m->nlevel) ? chance(90) : chance(40)))
    {
        if (chance(70))
            map_make_river(m, rivertype);
        else
            map_make_lake(m, rivertype);

        if (m->grid[1][1].type == LT_WALL)
            map_make_maze_eat(m, 1, 1);
    }
    else
        map_make_maze_eat(m, 1, 1);

    /* add exit to town on map 1 */
    if (m->nlevel == 1)
    {
        m->grid[MAP_MAX_Y - 1][(MAP_MAX_X - 1) / 2].type = LT_FLOOR;
        m->grid[MAP_MAX_Y - 1][(MAP_MAX_X - 1) / 2].sobject = LS_DNGN_EXIT;
    }

    /* generate open spaces */
    nrooms = rand_1n(3) + 3;
    if (treasure_room)
        nrooms++;

    rooms = g_malloc0(sizeof(rectangle*) * (nrooms + 1));

    for (int room = 0; room < nrooms; room++)
    {
        rooms[room] = g_malloc0(sizeof(rectangle));

        my = rand_1n(11) + 2;
        rooms[room]->y1 = my - rand_1n(2);
        rooms[room]->y2 = my + rand_1n(2);

        if (is_volcano_map(m->nlevel))
        {
            mx = rand_1n(60) + 3;
            rooms[room]->x1 = mx - rand_1n(2);
            rooms[room]->x2 = mx + rand_1n(2);

            want_monster = TRUE;
        }
        else
        {
            mx = rand_1n(44) + 5;
            rooms[room]->x1 = mx - rand_1n(4);
            rooms[room]->x2 = mx + rand_1n(12) + 3;
        }

        for (Y(pos) = rooms[room]->y1 ; Y(pos) < rooms[room]->y2 ; Y(pos)++)
        {
            for (X(pos) = rooms[room]->x1 ; X(pos) < rooms[room]->x2 ; X(pos)++)
            {
                map_tile *tile = map_tile_at(m, pos);
                if (tile->type == rivertype)
                    continue;

                tile->type = LT_FLOOR;

                if (want_monster == TRUE)
                {
                    monster_new_by_level(pos);
                    want_monster = FALSE;
                }
            }
        }
    }

    /* mark the end of the rooms array */
    rooms[nrooms] = NULL;

    /* add stationary objects */
    if (!map_fill_with_stationary_objects(m))
    {
        /* adding stationary objects failed; generate a new map */
        goto generate;
    }

    /* add treasure room if requested */
    if (treasure_room)
        map_make_treasure_room(m, rooms);

    /* clean up */
    for (int room = 0; room < nrooms; room++)
        g_free(rooms[room]);

    g_free(rooms);
}

/* function to eat away a filled in maze */
static void map_make_maze_eat(map *m, int x, int y)
{
    int dir;
    int try = 2;

    dir = rand_1n(4);

    while (try)
    {
        switch (dir)
        {
        case 1: /* west */
            if ((x > 2) &&
                    (m->grid[y][x - 1].type == LT_WALL) &&
                    (m->grid[y][x - 2].type == LT_WALL))
            {
                m->grid[y][x - 1].type = m->grid[y][x - 2].type = LT_FLOOR;
                map_make_maze_eat(m, x - 2, y);
            }
            break;

        case 2: /* east */
            if (x < (MAP_MAX_X - 3) &&
                    (m->grid[y][x + 1].type == LT_WALL) &&
                    (m->grid[y][x + 2].type == LT_WALL))
            {
                m->grid[y][x + 1].type = m->grid[y][x + 2].type = LT_FLOOR;
                map_make_maze_eat(m, x + 2, y);
            }
            break;

        case 3: /* south */
            if ((y > 2) &&
                    (m->grid[y - 1][x].type == LT_WALL) &&
                    (m->grid[y - 2][x].type == LT_WALL))
            {
                m->grid[y - 1][x].type = m->grid[y - 2][x].type = LT_FLOOR;
                map_make_maze_eat(m, x, y - 2);
            }
            break;

        case 4: /* north */
            if ((y < MAP_MAX_Y - 3) &&
                    (m->grid[y + 1][x].type == LT_WALL) &&
                    (m->grid[y + 2][x].type == LT_WALL))
            {
                m->grid[y + 1][x].type = m->grid[y + 2][x].type = LT_FLOOR;
                map_make_maze_eat(m, x, y + 2);
            }

            break;
        };
        if (++dir > 4)
        {
            dir = 1;
            --try;
        }
    }
}

/* The river/lake creation algorithm has been copied in entirety
   from Dungeon Crawl Stone Soup, with only very slight changes. (jpeg) */
static void map_make_vertical_river(map *m, map_tile_t rivertype)
{
    guint width  = 3 + rand_0n(4);
    guint startx = 6 - width + rand_0n(MAP_MAX_X - 8);

    const guint starty = rand_1n(4);
    const guint endy   = MAP_MAX_Y - (4 - starty);
    const guint minx   = rand_1n(3);
    const guint maxx   = MAP_MAX_X - rand_1n(3);

    position pos = pos_invalid;
    Z(pos) = m->nlevel;
    for (Y(pos) = starty; Y(pos) < endy; Y(pos)++)
    {
        if (chance(33)) startx++;
        if (chance(33)) startx--;
        if (chance(50)) width++;
        if (chance(50)) width--;

        if (width < 2) width = 2;
        if (width > 6) width = 6;

        for (X(pos) = startx; X(pos) < startx + width; X(pos)++)
        {
            if (X(pos) > minx && X(pos) < maxx && chance(99))
                map_tiletype_set(m, pos, rivertype);
        }
    }
}

static void map_make_river(map *m, map_tile_t rivertype)
{
    if (chance(20))
    {
        map_make_vertical_river(m, rivertype);
        return;
    }

    guint width  = 3 + rand_0n(4);
    guint starty = 10 - width + rand_0n(MAP_MAX_Y - 12);

    const guint startx = rand_1n(7);
    const guint endx   = MAP_MAX_X - (7 - startx);
    const guint miny   = rand_1n(3);
    const guint maxy   = MAP_MAX_Y - rand_1n(3);

    position pos = pos_invalid;
    Z(pos) = m->nlevel;
    for (X(pos) = startx; X(pos) < endx; X(pos)++)
    {
        if (chance(33)) starty++;
        if (chance(33)) starty--;
        if (chance(50)) width++;
        if (chance(50)) width--;

        /* sanity checks */
        if (starty < 2) starty = 2;
        if (starty > maxy) starty = maxy;
        if (width < 2) width = 2;
        if (width > 6) width = 6;

        for (Y(pos) = starty; Y(pos) < starty + width; Y(pos)++)
        {
            if (Y(pos) > miny && Y(pos) < maxy && chance(99))
                map_tiletype_set(m, pos, rivertype);
        }
    }
}

static void map_make_lake(map *m, map_tile_t laketype)
{
    guint x1 = 5 + rand_0n(MAP_MAX_X - 30);
    guint y1 = 3 + rand_0n(MAP_MAX_Y - 15);
    guint x2 = x1 + 4 + rand_0n(16);
    guint y2 = y1 + 4 + rand_0n(5);

    position pos = pos_invalid;
    Z(pos) = m->nlevel;
    for (Y(pos) = y1; Y(pos) < y2; Y(pos)++)
    {
        if (Y(pos) <= 1 || Y(pos) >= MAP_MAX_Y - 1)
            continue;

        if (chance(50))  x1 += rand_0n(3);
        if (chance(50))  x1 -= rand_0n(3);
        if (chance(50))  x2 += rand_0n(3);
        if (chance(50))  x2 -= rand_0n(3);

        if (Y(pos) - y1 < (y2 - y1) / 2)
        {
            x2 += rand_0n(3);
            x1 -= rand_0n(3);
        }
        else
        {
            x2 -= rand_0n(3);
            x1 += rand_0n(3);
        }

        for (X(pos) = x1; X(pos) < x2 ; X(pos)++)
        {
            if (X(pos) <= 1 || X(pos) >= MAP_MAX_X - 1)
                continue;

            if (chance(99))
                map_tiletype_set(m, pos, laketype);
        }
    }
}

static void place_special_item(map *m, position npos)
{
    map_tile *tile = map_tile_at(m, npos);

    switch (m->nlevel)
    {
    case MAP_DMAX - 1: /* the amulet of larn */
        inv_add(&tile->ilist, item_new(IT_AMULET, AM_LARN));

        monster_new(MT_DEMONLORD_I + rand_0n(7), npos);
        break;

    case MAP_MAX - 1: /* potion of cure dianthroritis */
        inv_add(&tile->ilist, item_new(IT_POTION, PO_CURE_DIANTHR));
        monster_new(MT_DEMON_PRINCE, npos);

    default:
        /* plain level, add neither monster nor item */
        break;
    }
}

/*
 *  function to read in a maze from a data file
 *
 *  Format of maze data file:
 *  For each maze:  MAP_MAX_Y + 1 lines (MAP_MAX_Y used)
 *                  MAP_MAX_X characters per line
 *
 *  Special characters in maze data file:
 *
 *      #   wall
 *      +   door
 *      m   random monster
 *      !   potion of cure dianthroritis, or the amulet of larn, as appropriate
 *      o   random object
 */
static gboolean map_load_from_file(map *m, const char *mazefile, guint which)
{
    position pos;       /* current position on map */
    int map_num = 0;    /* number of selected map */
    item_t it;          /* item type for random objects */

    FILE *levelfile;

    if (!(levelfile = fopen(mazefile, "r")))
    {
        /* maze file cannot be opened */
        return FALSE;
    }

    if (feof(levelfile))
    {
        /* FIXME: debug output */
        fclose(levelfile);

        return FALSE;
    }

    /* FIXME: calculate how many levels are in the file  */

    /* roll the dice: which map? */
    if (which <= MAP_MAX_MAZE_NUM)
    {
        map_num = which;
    }
    else
    {
        int tries = 0;
        do
        {
            map_num = rand_1n(MAP_MAX_MAZE_NUM);
        }
        while (map_used[map_num] && ++tries < 100);

        map_used[map_num] = TRUE;
    }

    /* determine number of line separating character(s) */
    guint lslen;

    /* get first character at the end of the first line */
    fseek(levelfile, MAP_MAX_X, SEEK_SET);
    switch (fgetc(levelfile))
    {
        case 10: /* i.e. LF */
            lslen = 1;
            break;
        case 13: /* i.e. CR/LF*/
            lslen = 2;
            break;
        default:
            /* maze file is corrupted - show error message and quit */
            nlarn = game_destroy(nlarn);
            display_show_message("Error", "Maze file is corrupted. Please reinstall the game.", 0);
            exit(EXIT_FAILURE);
            break;
    }

    /* advance to desired maze */
    fseek(levelfile, (map_num * ((MAP_MAX_X + lslen) * MAP_MAX_Y + lslen)), SEEK_SET);

    if (feof(levelfile))
    {
        /* FIXME: debug output */
        fclose(levelfile);

        return FALSE;
    }

    // Sometimes flip the maps. (Never the town)
    gboolean flip_vertical   = (map_num > 0 && chance(50));
    gboolean flip_horizontal = (map_num > 0 && chance(50));

    /* replace which of 3 '!' with a special item? (if appropriate) */
    int spec_count = rand_0n(3);

    Z(pos) = m->nlevel;
    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            position map_pos = pos;
            if (flip_vertical)
                X(map_pos) = MAP_MAX_X - X(pos) - 1;
            if (flip_horizontal)
                Y(map_pos) = MAP_MAX_Y - Y(pos) - 1;

            map_tile *tile = map_tile_at(m, map_pos);

            tile->type = LT_FLOOR;    /* floor is default */

            switch (fgetc(levelfile))
            {

            case '^': /* mountain */
                tile->type = LT_MOUNTAIN;
                break;

            case '"': /* grass */
                tile->type = LT_GRASS;
                break;

            case '.': /* dirt */
                tile->type = LT_DIRT;
                break;

            case '&': /* tree */
                tile->type = LT_TREE;
                break;

            case '~': /* deep water */
                tile->type = LT_DEEPWATER;
                break;

            case '=': /* lava */
                tile->type = LT_LAVA;
                break;

            case '#': /* wall */
                tile->type =  LT_WALL;
                break;

            case '_': /* altar */
                tile->sobject = LS_ALTAR;
                break;

            case '+': /* door */
                tile->sobject = LS_CLOSEDDOOR;
                break;

            case 'O': /* dungeon entrance */
                tile->sobject = LS_DNGN_ENTRANCE;
                break;

            case 'I': /* elevator */
                tile->sobject = LS_ELEVATORDOWN;
                break;

            case 'H': /* home */
                tile->sobject = LS_HOME;
                break;

            case 'D': /* dnd store */
                tile->sobject = LS_DNDSTORE;
                break;

            case 'T': /* trade post */
                tile->sobject = LS_TRADEPOST;
                break;

            case 'L': /* LRS */
                tile->sobject = LS_LRS;
                break;

            case 'S': /* school */
                tile->sobject = LS_SCHOOL;
                break;

            case 'B': /* bank */
                tile->sobject = LS_BANK;
                break;

            case 'M': /* monastery */
                tile->sobject = LS_MONASTERY;
                break;

            case '!': /* potion of cure dianthroritis, eye of larn */
                if (spec_count-- == 0)
                    place_special_item(m, map_pos);
                break;

            case 'm': /* random monster */
                monster_new_by_level(map_pos);
                break;

            case 'o': /* random item */
                do
                {
                    it = rand_1n(IT_MAX - 1);
                }
                while (it == IT_CONTAINER);

                inv_add(&tile->ilist, item_new_by_level(it, m->nlevel));
                break;
            };
        }

        (void)fgetc(levelfile); /* eat EOL */
    }

    fclose(levelfile);

    /* if the amulet of larn/pcd has not been placed yet, place it randomly */
    if (spec_count >= 0)
        place_special_item(m, map_find_space(m, LE_ITEM, FALSE));

    return TRUE;
}

/*
 * function to make a treasure room on a map
 */
static void map_make_treasure_room(map *m, rectangle **rooms)
{
    position pos = pos_invalid, npos = pos_invalid;
    sobject_t mst;
    item *itm;
    int success;

    int nrooms = 0; /* count of available rooms */
    int room;   /* number of chose room */

    /* There's nothing to do if there are no rooms. */
    if (rooms == NULL) { return; }

    /* determine number of rooms */
    while(rooms[nrooms] != NULL) { nrooms++; }

    /* choose a room to turn into an treasure room */
    room = rand_0n(nrooms);

    /* sanity check */
    if (rooms[room] == NULL) { return; }

    Z(pos) = Z(npos) = m->nlevel;

    for (Y(pos) = rooms[room]->y1; Y(pos) <= rooms[room]->y2; Y(pos)++)
    {
        for (X(pos) = rooms[room]->x1; X(pos) <= rooms[room]->x2; X(pos)++)
        {
            if ( (Y(pos) == rooms[room]->y1)
                    || (Y(pos) == rooms[room]->y2)
                    || (X(pos) == rooms[room]->x1)
                    || (X(pos) == rooms[room]->x2) )
            {
                /* if we are on the border of a room, make wall */
                map_tiletype_set(m, pos, LT_WALL);
            }
            else
            {
                /* make sure there's floor here */
                map_tiletype_set(m, pos, LT_FLOOR);

                /* create loot */
                itm = item_new_random(IT_GOLD, FALSE);
                inv_add(map_ilist_at(m, pos), itm);

                /* create a monster */
                monster_new_by_level(pos);
            }

            /* now clear out interior */
            if ((mst = map_sobject_at(m, pos)))
            {
                success = FALSE;
                do
                {
                    npos = map_find_space(m, LE_SOBJECT, FALSE);
                    if (!pos_in_rect(npos, *rooms[room]))
                    {
                        /* pos is outside of room */
                        map_sobject_set(m, npos, mst);
                        map_sobject_set(m, pos, LS_NONE);

                        success = TRUE;
                    }
                }
                while (!success);
            } /* if map_sobject_at() */
        } /* for x */
    } /* for y */

    /* place the door on the treasure room */
    switch (rand_1n(2))
    {
    case 1: /* horizontal */
        X(pos) = rand_m_n(rooms[room]->x1, rooms[room]->x2);
        Y(pos) = rand_0n(1) ? rooms[room]->y1 : rooms[room]->y2;
        break;

    case 2: /* vertical */
        X(pos) = rand_0n(1) ? rooms[room]->x1 : rooms[room]->x2;
        Y(pos) = rand_m_n(rooms[room]->y1, rooms[room]->y2);
        break;
    };

    map_tiletype_set(m, pos, LT_FLOOR);
    map_sobject_set(m, pos, LS_CLOSEDDOOR);
}

/* verify that every space on the map can be reached */
static int map_validate(map *m)
{
    position pos = pos_invalid;
    int connected = TRUE;
    area *floodmap = NULL;
    area *obsmap = area_new(0, 0, MAP_MAX_X, MAP_MAX_Y);

    Z(pos) = m->nlevel;

    /* generate an obstacle map */
    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
            if (!map_pos_passable(m, pos)
                    && (map_sobject_at(m, pos) != LS_CLOSEDDOOR))
            {
                area_point_set(obsmap, X(pos), Y(pos));
            }

    /* get position of entrance */
    switch (m->nlevel)
    {
        /* caverns entrance */
    case 1:
        pos = map_find_sobject(m, LS_DNGN_EXIT);
        break;

        /* volcano entrance */
    case MAP_DMAX:
        pos = map_find_sobject(m, LS_ELEVATORUP);
        break;

    default:
        pos = map_find_sobject(m, LS_STAIRSDOWN);
        break;
    }

    /* flood fill the maze starting at the entrance */
    floodmap = area_flood(obsmap, X(pos), Y(pos));

    /* compare flooded area with obstacle map */
    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            int pp = map_pos_passable(m, pos);
            int cd = (map_sobject_at(m, pos) == LS_CLOSEDDOOR);

            /* point should be set on floodmap if it is passable */
            if (area_point_get(floodmap, X(pos), Y(pos)) != (pp || cd))
            {
                connected = FALSE;
                break;
            }
        }

        if (!connected)
            break;
    }

    area_destroy(floodmap);

    return connected;
}

/* subroutine to put an item onto an empty space */
void map_item_add(map *m, item *what)
{
    position pos = map_find_space(m, LE_ITEM, FALSE);
    inv_add(map_ilist_at(m, pos), what);
}

static map_path *map_path_new(position start, position goal)
{
    map_path *path;

    path = g_malloc0(sizeof(map_path));

    path->open   = g_ptr_array_new();
    path->closed = g_ptr_array_new();
    path->path   = g_queue_new();

    path->start = start;
    path->goal  = goal;

    return path;
}

static map_path_element *map_path_element_new(position pos)
{
    map_path_element *lpe;

    lpe = g_malloc0(sizeof(map_path_element));
    lpe->pos = pos;

    return lpe;
}

/* calculate the cost of stepping into this new field */
static int map_step_cost(map *m, map_path_element* element,
                         map_element_t map_elem, gboolean ppath)
{
    map_tile_t tt;
    guint32 step_cost = 1; /* at least 1 movement cost */

    /* get the monster located on the map tile */
    monster *mon = map_get_monster_at(m, element->pos);

    /* get the tile type of the map tile */
    if (ppath)
    {
        tt = player_memory_of(nlarn->p, element->pos).type ;
    }
    else
    {
        tt = map_tiletype_at(m, element->pos);
    }

    /* penalize for traps known to the player */
    if (ppath && player_memory_of(nlarn->p, element->pos).trap)
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
    if (mon != NULL && (!ppath || monster_in_sight(mon)))
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
static int map_path_cost(map_path_element* element, position target)
{
    /* estimate the distance from the current position to the target */
    element->h_score = pos_distance(element->pos, target);

    return element->g_score + element->h_score;
}

static map_path_element *map_path_element_in_list(map_path_element* el,
                                                  GPtrArray *list)
{
    g_assert(el != NULL && list != NULL);

    for (guint idx = 0; idx < list->len; idx++)
    {
        map_path_element *li = g_ptr_array_index(list, idx);

        if (pos_identical(li->pos, el->pos))
            return li;
    }

    return NULL;
}

static map_path_element *map_path_find_best(map_path *path)
{
    map_path_element *el, *best = NULL;

    for (guint idx = 0; idx < path->open->len; idx++)
    {
        el = g_ptr_array_index(path->open, idx);

        if (best == NULL || map_path_cost(el, path->goal)
                < map_path_cost(best, path->goal))
        {
            best = el;
        }
    }

    return best;
}

static GPtrArray *map_path_get_neighbours(map *m, position pos,
                                          map_element_t element,
                                          gboolean ppath)
{
    GPtrArray *neighbours = g_ptr_array_new();

    for (direction dir = GD_NONE + 1; dir < GD_MAX; dir++)
    {
        if (dir == GD_CURR)
            continue;

        position npos = pos_move(pos, dir);

        if (!pos_valid(npos))
            continue;

        if ((ppath && mt_is_passable(player_memory_of(nlarn->p, npos).type))
                || (!ppath && monster_valid_dest(m, npos, element)))
        {
            map_path_element *pe = map_path_element_new(npos);
            g_ptr_array_add(neighbours, pe);
        }
    }

    return neighbours;
}
