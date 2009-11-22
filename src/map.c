/*
 * map.c
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

#include <assert.h>
#include <stdlib.h>

#include "container.h"
#include "display.h"
#include "items.h"
#include "map.h"
#include "nlarn.h"
#include "spheres.h"

static void map_fill_with_stationary_objects(map *maze);
static void map_fill_with_objects(map *l);
static void map_fill_with_traps(map *l);

static int map_load_from_file(map *l, char *mazefile, int which);
static void map_make_maze(map *maze, int treasure_room);
static void map_make_maze_eat(map *l, int x, int y);
static void map_make_treasure_room(map *maze, rectangle **rooms);
static int map_validate(map *maze);
static void map_item_add(map *maze, item *what);

static map_path *map_path_new(position start, position goal);
static map_path_element *map_path_element_new(position pos);
static int map_path_cost(map *l, map_path_element* element, position target);
static map_path_element *map_path_element_in_list(map_path_element* el, GPtrArray *list);
static map_path_element *map_path_find_best(map *l, map_path *path);
static GPtrArray *map_path_get_neighbours(map *l, position pos);

static gboolean map_monster_destroy(gpointer key, monster *monst, map *m);
static gboolean map_sphere_destroy(sphere *s, map *m);

const map_tile_data map_tiles[LT_MAX] =
{
    /* type         img  color      desc           pa tr */
    { LT_NONE,      ' ', DC_NONE,   NULL,          0, 0 },
    { LT_MOUNTAIN,  '^', DC_WHITE,  "a mountain",  0, 0 },
    { LT_GRASS,     '"', DC_GREEN,  "grass",       1, 1 },
    { LT_DIRT,      '.', DC_YELLOW, "dirt",        1, 1 },
    { LT_TREE,      '&', DC_GREEN,  "a tree",      0, 0 },
    { LT_FLOOR,     ' ', DC_NONE,   "floor",       1, 1 },
    { LT_WATER,     '~', DC_BLUE,   "water",       1, 1 },
    { LT_DEEPWATER, '~', DC_BLUE,   "deep water",  0, 1 },
    { LT_LAVA,      '=', DC_RED,    "lava",        0, 1 },
    { LT_FIRE,      '*', DC_RED,    "fire",        1, 1 },
    { LT_CLOUD,     '*', DC_WHITE,  "a gas cloud", 1, 1 },
    { LT_WALL,      '#', DC_WHITE,  "a wall",      0, 0 },
};

const map_stationary_data map_stationaries[LS_MAX] =
{
    { LS_NONE,          ' ',  DC_NONE,    NULL,                                  1, 1, },
    { LS_ALTAR,         '_',  DC_WHITE,   "a holy altar",                        1, 1, },
    { LS_THRONE,        '\\', DC_MAGENTA, "a handsome, jewel-encrusted throne",  1, 1, },
    { LS_THRONE2,       '\\', DC_MAGENTA, "a handsome, jewel-encrusted throne",  1, 1, },
    { LS_DEADTHRONE,    '\\', DC_WHITE,   "a massive throne",                    1, 1, },
    { LS_STAIRSDOWN,    '>',  DC_WHITE,   "a circular staircase",                1, 1, },
    { LS_STAIRSUP,      '<',  DC_WHITE,   "a circular staircase",                1, 1, },
    { LS_ELEVATORDOWN,  'I',  DC_RED,     "a volcanic shaft leaning downward",   1, 1, },
    { LS_ELEVATORUP,    'I',  DC_RED,     "the base of a volcanic shaft",        1, 1, },
    { LS_FOUNTAIN,      '{',  DC_BLUE,    "a bubbling fountain",                 1, 1, },
    { LS_DEADFOUNTAIN,  '{',  DC_WHITE,   "There is a dead fountain",            1, 1, },
    { LS_STATUE,        '|',  DC_WHITE,   "a great marble statue",               1, 1, },
    { LS_URN,           'u',  DC_YELLOW,  "a golden urn",                        1, 1, },
    { LS_MIRROR,        '\'', DC_WHITE,   "a mirror",                            1, 1, },
    { LS_OPENDOOR,      '/',  DC_YELLOW,  "an open door.",                       1, 1, },
    { LS_CLOSEDDOOR,    '+',  DC_YELLOW,  "a closed door.",                      0, 0, },
    { LS_ENTRANCE,      'O',  DC_RED,     "the dungeon entrance",                1, 1, },
    { LS_HOME,          'H',  DC_WHITE,   "your home",                           1, 0, },
    { LS_DNDSTORE,      'D',  DC_WHITE,   "a DND store",                         1, 0, },
    { LS_TRADEPOST,     'T',  DC_WHITE,   "the Larn trading Post",               1, 0, },
    { LS_LRS,           'L',  DC_WHITE,   "an LRS office",                       1, 0, },
    { LS_SCHOOL,        'S',  DC_WHITE,   "the College of Larn",                 1, 0, },
    { LS_BANK,          'B',  DC_WHITE,   "the bank of Larn",                    1, 0, },
    { LS_BANK2,         'B',  DC_WHITE,   "a branch office of the bank of Larn", 1, 0, },
};

/* keep track which levels have been used before */
static int map_used[23] = { 1, 0 };

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

map *map_new(int num, char *mazefile)
{
    gboolean map_loaded = FALSE;
    gboolean treasure_room = FALSE;
    gboolean keep_maze = TRUE;

    map *nmap = nlarn->maps[num] = g_malloc0(sizeof(map));
    nmap->nlevel = num;

    /* create map */
    if ((num == 0) /* town is stored in file */
            || (num == MAP_DMAX - 1) /* level 10 */
            || (num == MAP_MAX - 1)  /* volcano level 3 */
            || (num > 1 && chance(25)))
    {
        /* read maze from data file */
        map_loaded = map_load_from_file(nmap, mazefile, (num == 0) ? 0 : -1);

        /* add stationary objects (not to the town) */
        if (num > 0) map_fill_with_stationary_objects(nmap);
    }

    if (!map_loaded)
    {
        /* determine if to add treasure room */
        treasure_room = num > 1 && chance(25);

        /* generate random map */
        do
        {
            /* dig cave */
            map_make_maze(nmap, treasure_room);

            /* add stationary objects */
            map_fill_with_stationary_objects(nmap);

            /* check if entire map is reachable */
            keep_maze = map_validate(nmap);
        }
        while (!keep_maze);

        /* add monsters */
        map_fill_with_life(nmap);
    }

    if (num > 0)
    {
        /* home town is not filled with crap */
        map_fill_with_objects(nmap);

        /* and not trapped */
        map_fill_with_traps(nmap);
    }

    return nmap;
}

cJSON *map_serialize(map *m)
{
    int x, y;
    cJSON *mser, *grid, *tile;

    mser = cJSON_CreateObject();

    cJSON_AddNumberToObject(mser, "nlevel", m->nlevel);
    cJSON_AddNumberToObject(mser, "visited", m->visited);

    cJSON_AddItemToObject(mser, "grid", grid = cJSON_CreateArray());

    for (y = 0; y < MAP_MAX_Y; y++)
    {
        for (x = 0; x < MAP_MAX_X; x++)
        {
            cJSON_AddItemToArray(grid, tile = cJSON_CreateObject());

            cJSON_AddNumberToObject(tile, "type", m->grid[y][x].type);

            if (m->grid[y][x].base_type > 0
                    && m->grid[y][x].base_type != m->grid[y][x].type)
            {
                cJSON_AddNumberToObject(tile, "base_type",
                                        m->grid[y][x].base_type);
            }

            if (m->grid[y][x].stationary)
            {
                cJSON_AddNumberToObject(tile, "stationary",
                                        m->grid[y][x].stationary);
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

            if (m->grid[y][x].monster)
            {
                cJSON_AddNumberToObject(tile, "monster",
                                        GPOINTER_TO_UINT(m->grid[y][x].monster));
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

map *map_deserialize(cJSON *mser, game *g)
{
    int x, y;
    cJSON *grid, *tile, *obj;
    map *m;

    m = g_malloc0(sizeof(map));

    m->nlevel = cJSON_GetObjectItem(mser, "nlevel")->valueint;
    m->visited = cJSON_GetObjectItem(mser, "visited")->valueint;

    grid = cJSON_GetObjectItem(mser, "grid");

    for (y = 0; y < MAP_MAX_Y; y++)
    {
        for (x = 0; x < MAP_MAX_X; x++)
        {
            tile = cJSON_GetArrayItem(grid, x + (y * MAP_MAX_X));

            m->grid[y][x].type = cJSON_GetObjectItem(tile, "type")->valueint;

            obj = cJSON_GetObjectItem(tile, "base_type");
            if (obj != NULL) m->grid[y][x].base_type = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "stationary");
            if (obj != NULL) m->grid[y][x].stationary = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "trap");
            if (obj != NULL) m->grid[y][x].trap = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "timer");
            if (obj != NULL) m->grid[y][x].timer = obj->valueint;

            obj = cJSON_GetObjectItem(tile, "monster");
            if (obj != NULL) m->grid[y][x].monster = GUINT_TO_POINTER(obj->valueint);

            obj = cJSON_GetObjectItem(tile, "inventory");
            if (obj != NULL) m->grid[y][x].ilist = inv_deserialize(obj);
        }
    }

    return m;
}

char *map_dump(map *l, position ppos)
{
    position pos;
    GString *map;
    monster *m;

    map = g_string_new_len(NULL, MAP_SIZE);

    pos.z = l->nlevel;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            if (pos_identical(pos, ppos))
            {
                g_string_append_c(map, '@');
            }
            else if ((m = map_get_monster_at(l, pos)))
            {
                g_string_append_c(map, monster_image(m));
            }
            else if (map_trap_at(l, pos))
            {
                g_string_append_c(map, '^');
            }
            else if (map_stationary_at(l, pos))
            {
                g_string_append_c(map, ls_get_image(map_stationary_at(l, pos)));
            }
            else
            {
                g_string_append_c(map, lt_get_image(map_tiletype_at(l, pos)));
            }
        }
        g_string_append_c(map, '\n');
    }

    return g_string_free(map, FALSE);
}

void map_destroy(map *m)
{
    int x, y;

    assert(m != NULL);

    /* destroy monster and spheres on this level */
    g_hash_table_foreach_remove(nlarn->monsters, (GHRFunc) map_monster_destroy, m);
    g_ptr_array_foreach(nlarn->spheres, (GFunc)map_sphere_destroy, m);

    /* free items */
    for (y = 0; y < MAP_MAX_Y; y++)
    {
        for (x = 0; x < MAP_MAX_X; x++)
        {
            if (m->grid[y][x].ilist != NULL)
                inv_destroy(m->grid[y][x].ilist);
        }
    }

    g_free(m);
}

/* return coordinates of a free space */
position map_find_space(map *maze, map_element_t element)
{
    rectangle entire_map = rect_new(1, 1, MAP_MAX_X - 2, MAP_MAX_Y - 2);

    return map_find_space_in(maze, entire_map, element);
}

position map_find_space_in(map *maze, rectangle where, map_element_t element)
{
    position pos, pos_orig;

    assert (maze != NULL && element > LE_NONE && element < LE_MAX);

    pos_orig.x = pos.x = rand_m_n(where.x1, where.x2);
    pos_orig.y = pos.y = rand_m_n(where.y1, where.y2);
    pos_orig.z = pos.z = maze->nlevel;

    do
    {
        pos.x += 1;
        pos.y += 1;

        if (pos.x > where.x2)
            pos.x = where.x1;

        if (pos.x < where.x1)
            pos.x = where.x2;

        if (pos.y > where.y2)
            pos.y = where.y1;

        if (pos.y < where.y1)
            pos.y = where.y2;
    }
    while (!map_pos_validate(maze, pos, element)
            && !pos_identical(pos, pos_orig));

    if (pos_identical(pos, pos_orig))
    {
        pos.x = G_MAXINT16;
        pos.y = G_MAXINT16;
    }

    return pos;
}

int *map_get_surrounding(map *l, position pos, map_stationary_t type)
{
    position p;
    int move = 1;
    int *dirs;

    dirs = g_malloc0(sizeof(int) * GD_MAX);

    while (move < GD_MAX)
    {
        p = pos_move(pos, move);

        if (pos_valid(p) && map_stationary_at(l, p) == type)
        {
            dirs[move] = TRUE;
        }

        move++;
    }

    return dirs;
}

position map_find_stationary_in(map *l, map_stationary_t stationary, rectangle area)
{
    position pos;

    assert(l != NULL);

    pos.z = l->nlevel;

    for (pos.y = area.y1; pos.y <= area.y2; pos.y++)
        for (pos.x = area.x1; pos.x <= area.x2; pos.x++)
            if (map_stationary_at(l,pos) == stationary)
                return pos;

    /* if we reach this point, the stationary is not on the map */
    return pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);
}

position map_find_stationary(map *l, map_stationary_t stationary)
{
    position pos;

    assert(l != NULL);

    pos.z = l->nlevel;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
            if (map_stationary_at(l,pos) == stationary)
                return pos;

    /* if we reach this point, the stationary is not on the map */
    return pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);
}

gboolean map_pos_validate(map *l, position pos, map_element_t element)
{
    map_tile *tile;

    assert(l != NULL && element > LT_NONE && element < LE_MAX);

    /* if the position is invalid it is invalid for the map as well */
    if (!pos_valid(pos))
        return FALSE;

    /* if the position is on another map it is invalid for this lavel */
    if (pos.z != l->nlevel)
        return FALSE;

    /* make shortcut */
    tile = map_tile_at(l, pos);

    switch (element)
    {

    case LE_GROUND:
        /* why should we need this case? */
        return TRUE;
        break;

    case LE_STATIONARY:
        if (lt_is_passable(tile->type) && (tile->stationary == LS_NONE) )
        {
            /* FIXME: don't want two stationaries next to each other */
            /* FIXME: don't want stationaries next to the wall */
            return TRUE;
        }
        break;

    case LE_TRAP:
        if (lt_is_passable(tile->type)
                && (tile->stationary == LS_NONE)
                && (tile->trap == TT_NONE) )
            return TRUE;
        break;

    case LE_ITEM:
        /* we can stack like mad, so we only need to check if there is an open space */
        if (map_pos_passable(l, pos) && (tile->stationary == LS_NONE))
            return TRUE;
        break;

    case LE_MONSTER:
        /* not ok if player is standing on that tile */
        if (pos_identical(pos, nlarn->p->pos))
            return FALSE;

        if (map_pos_passable(l, pos) && !map_is_monster_at(l, pos))
            return TRUE;
        break;

    case LE_NONE:
    case LE_MAX:
        return FALSE;
        break;

    } /* switch */

    return FALSE;
}

/**
 * determine if a position can be seen from another position
 * @param the map
 * @param first position
 * @param second position
 * @return TRUE or FALSE
 */
int map_pos_is_visible(map *l, position s, position t)
{
    int delta_x, delta_y;
    int x, y;
    signed int ix, iy;
    int error;

    /* positions on different levels? */
    if (s.z != t.z)
        return FALSE;

    x = s.x;
    y = s.y;

    delta_x = abs(t.x - s.x) << 1;
    delta_y = abs(t.y - s.y) << 1;

    /* if x1 == x2 or y1 == y2, then it does not matter what we set here */
    ix = t.x > s.x ? 1 : -1;
    iy = t.y > s.y ? 1 : -1;

    if (delta_x >= delta_y)
    {
        /* error may go below zero */
        error = delta_y - (delta_x >> 1);

        while (x != t.x)
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

            if (!lt_is_transparent(l->grid[y][x].type) || !ls_is_transparent(l->grid[y][x].stationary))
                return FALSE;
        }
    }
    else
    {
        /* error may go below zero */
        int error = delta_x - (delta_y >> 1);

        while (y != t.y)
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

            if (!lt_is_transparent(l->grid[y][x].type) || !ls_is_transparent(l->grid[y][x].stationary))
                return FALSE;
        }
    }

    return TRUE;
}

map_path *map_find_path(map *l, position start, position goal)
{
    assert(l != NULL && (start.z == goal.z));

    map_path *path;
    map_path_element *curr, *next;
    gboolean next_is_better;
    GPtrArray *neighbours;

    path = map_path_new(start, goal);

    /* add start to open list */
    curr = map_path_element_new(start);
    g_ptr_array_add(path->open, curr);

    while (path->open->len)
    {
        curr = map_path_find_best(l, path);

        g_ptr_array_remove_fast(path->open, curr);
        g_ptr_array_add(path->closed, curr);

        if (curr->pos.x == path->goal.x && curr->pos.y == path->goal.y)
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

        neighbours = map_path_get_neighbours(l, curr->pos);

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

            if (!map_path_element_in_list(next, path->open))
            {
                g_ptr_array_add(path->open, next);
                next_is_better = TRUE;
            }
            else if (map_path_cost(l, curr, path->goal) > map_path_cost(l, next, path->goal))
            {
                next_is_better = TRUE;
            }
            else
            {
                g_free(next);
            }

            if (next_is_better)
            {
                next->parent = curr;
            }
        }

        g_ptr_array_free(neighbours, TRUE);
    }

    return NULL;
}

void map_path_destroy(map_path *path)
{
    guint idx;

    assert(path != NULL);

    /* cleanup open list */
    for (idx = 0; idx < path->open->len; idx++)
    {
        g_free(g_ptr_array_index(path->open, idx));
    }

    g_ptr_array_free(path->open, TRUE);

    for (idx = 0; idx < path->closed->len; idx++)
    {
        g_free(g_ptr_array_index(path->closed, idx));
    }

    g_ptr_array_free(path->closed, TRUE);

    g_queue_free(path->path);

    g_free(path);
}

area *map_get_obstacles(map *l, position center, int radius)
{
    area *narea;
    position pos;
    int x, y;

    assert(l != NULL);

    if (!pos_valid(center))
    {
        return NULL;
    }

    narea = area_new(center.x - radius, center.y - radius,
                     radius * 2 + 1, radius * 2 + 1);

    pos.z = l->nlevel;

    for (pos.y = center.y - radius, y = 0; pos.y <= center.y + radius; pos.y++, y++)
    {
        for (pos.x = center.x - radius, x = 0; pos.x <= center.x + radius; pos.x++, x++)
        {
            if (!pos_valid(pos) || !map_pos_passable(l, pos))
            {
                area_point_set(narea, x, y);
            }
        }
    }

    return narea;
}

void map_set_tiletype(map *l, area *area, map_tile_t type, guint8 duration)
{
    position pos;
    int x, y;

    assert (l != NULL && area != NULL);

    for (pos.y = area->start_y, y = 0;
            pos.y < area->start_y + area->size_y;
            pos.y++, y++)
    {
        for (pos.x = area->start_x, x = 0;
                pos.x < area->start_x + area->size_x;
                pos.x++, x++)
        {
            /* check if pos is inside the map */
            if (!pos_valid(pos))
            {
                continue;
            }

            /* if the position is marked in area set the tile to type */
            if (area_point_get(area, x, y))
            {
                map_tile *tile = map_tile_at(l, pos);

                /* store original type if it has not been set already
                   (this can occur when casting multiple flood
                   spells on the same tile) */
                if (tile->base_type == LT_NONE)
                {
                    tile->base_type = map_tiletype_at(l, pos);
                }

                tile->type = type;
                tile->timer = duration;
            }
        }
    }
}

map_tile *map_tile_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return &l->grid[pos.y][pos.x];
}

inventory **map_ilist_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return &l->grid[pos.y][pos.x].ilist;
}

map_tile_t map_tiletype_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return l->grid[pos.y][pos.x].type;
}

void map_tiletype_set(map *l, position pos, map_tile_t type)
{
    assert(l != NULL && pos_valid(pos));

    l->grid[pos.y][pos.x].type = type;
}

map_tile_t map_basetype_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return l->grid[pos.y][pos.x].base_type;
}

void map_basetype_set(map *l, position pos, map_tile_t type)
{
    assert(l != NULL && pos_valid(pos));

    l->grid[pos.y][pos.x].base_type = type;
}

guint8 map_timer_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return l->grid[pos.y][pos.x].timer;
}

trap_t map_trap_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return l->grid[pos.y][pos.x].trap;
}

void map_trap_set(map *l, position pos, trap_t type)
{
    assert(l != NULL && pos_valid(pos));

    l->grid[pos.y][pos.x].trap = type;
}

map_stationary_t map_stationary_at(map *l, position pos)
{
    assert(l != NULL && pos_valid(pos));

    return l->grid[pos.y][pos.x].stationary;
}

void map_stationary_set(map *l, position pos, map_stationary_t type)
{
    assert(l != NULL && pos_valid(pos));

    l->grid[pos.y][pos.x].stationary = type;
}

damage *map_tile_damage(map *l, position pos)
{
    assert (l != NULL && pos_valid(pos));

    switch (map_tiletype_at(l, pos))
    {
    case LT_CLOUD:
        return damage_new(DAM_ACID, 3 + rand_0n(2), NULL);
        break;

    case LT_FIRE:
        return damage_new(DAM_FIRE, 5 + rand_0n(2), NULL);
        break;

    case LT_WATER:
        return damage_new(DAM_WATER, 4 + rand_0n(2), NULL);
        break;

    default:
        return NULL;
    }
}

monster *map_get_monster_at(map *m, position pos)
{
    assert(m != NULL && m->nlevel == pos.z && pos_valid(pos));

    gpointer mid = m->grid[pos.y][pos.x].monster;
    return (mid != NULL) ? game_monster_get(nlarn, mid) : NULL;
}

int map_set_monster_at(map *map, position pos, monster *monst)
{
    assert(map != NULL && map->nlevel == pos.z && pos_valid(pos));

    map->grid[pos.y][pos.x].monster = (monst != NULL) ? monster_oid(monst) : NULL;

    return TRUE;
}

int map_is_monster_at(map *m, position pos)
{
    assert(m != NULL);

    return ((map_get_monster_at(m, pos) != NULL));
}

GPtrArray *map_get_monsters_in(map *m, rectangle area)
{
    GPtrArray *monsters;
    monster *monst;
    position pos;

    assert(m != NULL);

    monsters = g_ptr_array_new();

    pos.z = m->nlevel;

    for (pos.y = area.y1; pos.y <= area.y2; pos.y++)
    {
        for (pos.x = area.x1; pos.x <= area.x2; pos.x++)
        {
            if ((monst = map_get_monster_at(m, pos)))
            {
                g_ptr_array_add(monsters, monst);
            }
        }
    }

    return monsters;
}

/**
 * creates an entire set of monsters for a map
 * @param a map
 */
int map_fill_with_life(map *l)
{
    int new_monster_count;
    int i;

    assert(l != NULL);

    new_monster_count = rand_1n(14) + l->nlevel;

    if (l->mcount > new_monster_count)
        /* no monsters added */
        return FALSE;
    else
        new_monster_count -= l->mcount;

    for (i = 0; i <= new_monster_count; i++)
    {
        position pos = map_find_space(l, LE_MONSTER);
        monster_new_by_level(pos);
    }

    return(new_monster_count);
}

void map_timer(map *l, guint8 count)
{
    position pos;
    item *it;
    guint idx;
    int impact;
    char *impact_desc;
    char item_desc[61];

    assert (l != NULL);

    pos.z = l->nlevel;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            if (map_timer_at(l, pos))
            {
                map_tile *tile = map_tile_at(l, pos);
                tile->timer -= min(map_timer_at(l, pos), count);

                /* affect items */
                if (tile->ilist != NULL)
                {
                    for (idx = 0; idx < inv_length(tile->ilist); idx++)
                    {
                        it = inv_get(tile->ilist, idx);

                        switch (tile->type)
                        {
                        case LT_FIRE:
                            impact = item_burn(it);
                            impact_desc = "burn";
                            break;

                        case LT_CLOUD:
                            impact = item_corrode(it);
                            impact_desc = "corrode";
                            break;

                        case LT_WATER:
                            impact = item_rust(it);
                            impact_desc = "rust";
                            break;
                        default:
                            impact = PI_NONE;
                            break;
                        }

                        /* notifiy player of impact if the item is visible */
                        if (impact && map_pos_is_visible(l, nlarn->p->pos, pos))
                        {
                            item_describe(it, player_item_known(nlarn->p, it),
                                          (it->count == 1), TRUE, item_desc, 60);

                            if (impact < PI_DESTROYED)
                            {
                                log_add_entry(nlarn->p->log, "The %s %s%s.", item_desc,
                                              impact_desc, (it->count == 1) ? "s" : "");
                            }
                            else
                            {
                                log_add_entry(nlarn->p->log, "The %s %s destroyed.", item_desc,
                                              (it->count == 1) ? "is" : "are");
                            }
                        }

                        if (impact == PI_DESTROYED)
                        {
                            inv_del_element(&tile->ilist, it);
                            item_destroy(it);
                        }
                    } /* foreach item */
                } /* if leve_ilist_at */

                /* reset tile type if temporary effect has expired */
                if (tile->timer == 0)
                {
                    if ((tile->type == LT_FIRE)
                            && (tile->type == LT_GRASS))
                    {
                        tile->type = LT_DIRT;
                    }
                    else
                    {
                        tile->type = tile->base_type;
                    }
                }
            } /* if map_timer_at */
        } /* for pos.x */
    } /* for pos.y */
}

static void map_fill_with_stationary_objects(map *maze)
{
    position pos;
    int i;						/* loop var */

    /* volcano shaft up from the temple */
    if (maze->nlevel == MAP_MAX -1)
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_ELEVATORUP);
    }

    /*  make the fixed objects in the maze: STAIRS */
    if ((maze->nlevel > 0) && (maze->nlevel != MAP_DMAX - 1) && (maze->nlevel != MAP_MAX - 1))
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_STAIRSDOWN);
    }

    if ((maze->nlevel > 1) && (maze->nlevel != MAP_DMAX))
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_STAIRSUP);
    }

    /* make the random objects in the maze */
    /* 33 percent chance for an altar */
    if (chance(33))
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_ALTAR);
    }

    /* up to three statues */
    for (i = 0; i < rand_0n(3); i++)
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_STATUE);
    }

    /* up to three fountains */
    for (i = 0; i < rand_0n(3); i++)
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_FOUNTAIN);
    }

    /* up to two thrones */
    for (i = 0; i < rand_0n(2); i++)
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_THRONE);
    }

    /* up to two  mirrors */
    for (i = 0; i < rand_0n(2); i++)
    {
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_MIRROR);
    }

    if (maze->nlevel == 5)
    {
        /* branch office of the bank */
        pos = map_find_space(maze, LE_STATIONARY);
        map_stationary_set(maze, pos, LS_BANK2);
    }
} /* map_fill_with_stationary */

static void map_fill_with_objects(map *l)
{
    int i,j;                    /* loop vars */
    item_t it;
    item *container = NULL;

    /* up to two pieces of armour */
    for (i = 0; i < rand_0n(2); i++)
    {
        map_item_add(l, item_new_by_level(IT_ARMOUR, l->nlevel));
    }

    /* up to three books */
    for (i = 0; i <= rand_0n(3); i++)
    {
        map_item_add(l, item_new_by_level(IT_BOOK, l->nlevel));
    }

    /* up to two containers */
    for (i = 1; i <= rand_0n(2); i++)
    {
        /* random container type */
        container = item_new(IT_CONTAINER, rand_1n(CT_MAX), 0);

        /* up to 5 items inside the container */
        for (j = 0; j < rand_0n(5); j++)
        {
            /* prevent containers inside the container */
            do
            {
                it = rand_1n(IT_MAX - 1);
            }
            while (it == IT_CONTAINER);

            inv_add(&(container->content), item_new_by_level(it, l->nlevel));
        }

        /* add the container to the map */
        map_item_add(l, container);
    }

    /* up to 10 piles of gold */
    for (i = 0; i < rand_0n(10); i++)
    {
        /* There is nothing like a newly minted pound. */
        map_item_add(l, item_new(IT_GOLD, rand_m_n(10, (l->nlevel + 1) * 15), 0));
    }

    /* up to three gems */
    for (i = 0; i < rand_0n(3); i++)
    {
        map_item_add(l, item_new(IT_GEM, rand_1n(item_max_id(IT_GEM)),
                                 rand_0n(6 * (l->nlevel + 1))));
    }

    /* up to four potions */
    for (i = 0; i < rand_0n(4); i++)
    {
        map_item_add(l, item_new_by_level(IT_POTION, l->nlevel));
    }

    /* up to three scrolls */
    for (i = 0; i < rand_0n(3); i++)
    {
        map_item_add(l, item_new_by_level(IT_SCROLL, l->nlevel));
    }

    /* up to two rings */
    for (i = 0; i < rand_0n(3); i++)
    {
        map_item_add(l, item_new_by_level(IT_RING, l->nlevel));
    }

    /* up to two weapons */
    for (i = 0; i < rand_0n(2); i++)
    {
        map_item_add(l, item_new_by_level(IT_WEAPON, l->nlevel));
    }

} /* map_fill_with_objects */

static void map_fill_with_traps(map *l)
{
    int count;
    position pos;
    int trapdoor = FALSE;

    assert(l != NULL);

    /* Trapdoor cannot be placed in the last dungeon map and the last vulcano map */
    trapdoor = ((l->nlevel != MAP_DMAX - 1) && (l->nlevel != MAP_MAX - 1));

    for (count = 0; count < rand_0n((trapdoor ? 8 : 6)); count++)
    {
        pos = map_find_space(l, LE_TRAP);
        map_trap_set(l, pos, rand_1n(trapdoor ? TT_MAX : TT_TRAPDOOR));
    }
} /* map_fill_with_traps */

/* subroutine to make the caverns for a given map. only walls are made. */
static void map_make_maze(map *maze, int treasure_room)
{
    position pos;
    int mx, my;
    int nrooms, room;
    rectangle **rooms = NULL;
    gboolean want_monster = FALSE;

    assert (maze != NULL);

    pos.z = maze->nlevel;

    /* reset map by filling it with walls */
    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            monster *m;

            map_tiletype_set(maze, pos, LT_WALL);
            map_stationary_set(maze, pos, LS_NONE);

            if ((m = map_get_monster_at(maze, pos)))
            {
                monster_destroy(m);
            }

            if (map_tile_at(maze, pos)->ilist != NULL)
            {
                inv_destroy(map_tile_at(maze, pos)->ilist);
                map_tile_at(maze, pos)->ilist = NULL;
            }
        }

    map_make_maze_eat(maze, 1, 1);

    /* add exit to town on map 1 */
    if (maze->nlevel == 1)
    {
        maze->grid[MAP_MAX_Y - 1][(MAP_MAX_X - 1) / 2].type = LT_FLOOR;
        maze->grid[MAP_MAX_Y - 1][(MAP_MAX_X - 1) / 2].stationary = LS_ENTRANCE;
    }

    /* generate open spaces */
    nrooms = rand_1n(3) + 3;
    if (treasure_room)
        nrooms++;

    rooms = g_malloc0(sizeof(rectangle *) * (nrooms + 1));

    for (room = 0; room < nrooms; room++)
    {
        rooms[room] = g_malloc0(sizeof(rectangle));

        my = rand_1n(11) + 2;
        rooms[room]->y1 = my - rand_1n(2);
        rooms[room]->y2 = my + rand_1n(2);

        if (maze->nlevel < MAP_DMAX)
        {
            mx = rand_1n(44)+5;
            rooms[room]->x1 = mx - rand_1n(4);
            rooms[room]->x2 = mx + rand_1n(12)+3;
        }
        else
        {
            mx = rand_1n(60)+3;
            rooms[room]->x1 = mx - rand_1n(2);
            rooms[room]->x2 = mx + rand_1n(2);

            want_monster = TRUE;
        }

        for (pos.y = rooms[room]->y1 ; pos.y < rooms[room]->y2 ; pos.y++)
        {
            for (pos.x = rooms[room]->x1 ; pos.x < rooms[room]->x2 ; pos.x++)
            {
                map_tile *tile = map_tile_at(maze, pos);
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

    /* add treasure room if requested */
    if (treasure_room)
    {
        map_make_treasure_room(maze, rooms);
    }

    /* cleanup */
    for (room = 0; room < nrooms; room++)
        g_free(rooms[room]);

    g_free(rooms);
}

/* function to eat away a filled in maze */
static void map_make_maze_eat(map *l, int x, int y)
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
                    (l->grid[y][x - 1].type == LT_WALL) &&
                    (l->grid[y][x - 2].type == LT_WALL))
            {
                l->grid[y][x - 1].type = l->grid[y][x - 2].type = LT_FLOOR;
                map_make_maze_eat(l, x - 2, y);
            }
            break;

        case 2: /* east */
            if (x < (MAP_MAX_X - 3) &&
                    (l->grid[y][x + 1].type == LT_WALL) &&
                    (l->grid[y][x + 2].type == LT_WALL))
            {
                l->grid[y][x + 1].type = l->grid[y][x + 2].type = LT_FLOOR;
                map_make_maze_eat(l, x + 2, y);
            }
            break;

        case 3: /* south */
            if ((y > 2) &&
                    (l->grid[y - 1][x].type == LT_WALL) &&
                    (l->grid[y - 2][x].type == LT_WALL))
            {
                l->grid[y - 1][x].type = l->grid[y - 2][x].type = LT_FLOOR;
                map_make_maze_eat(l, x, y - 2);
            }

            break;

        case 4: /* north */
            if ((y < MAP_MAX_Y - 3) &&
                    (l->grid[y + 1][x].type == LT_WALL) &&
                    (l->grid[y + 2][x].type == LT_WALL))
            {
                l->grid[y + 1][x].type = l->grid[y + 2][x].type = LT_FLOOR;
                map_make_maze_eat(l, x, y + 2);
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
 *      D   door
 *      .   random monster
 *      ~   eye of larn
 *      !   potion of cure dianthroritis
 *      -   random object
 */
static int map_load_from_file(map *nmap, char *mazefile, int which)
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

    /* roll the dice: which map? we can currently choose from a variety of 24 */
    if (which >= 0 && which < 25)
    {
        map_num = which;
    }
    else
    {
        while (map_used[map_num])
        {
            map_num = rand_1n(25);
        }
        map_used[map_num] = TRUE;
    }

    /* advance to desired maze */
    fseek(levelfile, (map_num * ((MAP_MAX_X + 1) * MAP_MAX_Y + 1)), SEEK_SET);

    pos.z = nmap->nlevel;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X ; pos.x++)
        {
            map_tile *tile = map_tile_at(nmap, pos);

            tile->type = LT_FLOOR;	/* floor is default */

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
                tile->stationary = LS_ALTAR;
                break;

            case '+': /* door */
                tile->stationary = LS_CLOSEDDOOR;
                break;

            case 'O': /* dungeon entrance */
                tile->stationary = LS_ENTRANCE;
                break;

            case 'I': /* elevator */
                tile->stationary = LS_ELEVATORDOWN;
                break;

            case 'H': /* home */
                tile->stationary = LS_HOME;
                break;

            case 'D': /* dnd store */
                tile->stationary = LS_DNDSTORE;
                break;

            case 'T': /* trede post */
                tile->stationary = LS_TRADEPOST;
                break;

            case 'L': /* LRS */
                tile->stationary = LS_LRS;
                break;

            case 'S': /* school */
                tile->stationary = LS_SCHOOL;
                break;

            case 'B': /*  */
                tile->stationary = LS_BANK;
                break;

            case '*': /* eye of larn */
                if (nmap->nlevel != MAP_DMAX - 1)
                {
                    break;
                }
                inv_add(&tile->ilist, item_new(IT_AMULET, AM_LARN, 0));

                monster_new(MT_DEMONLORD_I + rand_0n(7), pos);
                break;

            case '!':	/* potion of cure dianthroritis */
                if (nmap->nlevel != MAP_MAX - 1)
                    break;

                inv_add(&tile->ilist, item_new(IT_POTION, PO_CURE_DIANTHR, 0));
                monster_new(MT_DAEMON_PRINCE, pos);
                break;

            case 'M':	/* random monster */
                monster_new_by_level(pos);
                break;

            case '-':
                do
                {
                    it = rand_1n(IT_MAX - 1);
                }
                while (it == IT_CONTAINER);

                inv_add(&tile->ilist, item_new_by_level(it, nmap->nlevel));
                break;
            };
        }
        (void)fgetc(levelfile); /* eat EOL */
    }

    fclose(levelfile);

    return TRUE;
}

/*
 * function to make a treasure room on a map
 */
static void map_make_treasure_room(map *maze, rectangle **rooms)
{
    position pos, npos;
    map_stationary_t mst;
    item *itm;
    int success;

    int nrooms; /* count of available rooms */
    int room;   /* number of chose room */

    /* determine number of rooms */
    for (nrooms = 0; rooms[nrooms] != NULL; nrooms++);

    /* choose a room to turn into an treasure room */
    room = rand_0n(nrooms);

    pos.z = npos.z = maze->nlevel;

    for (pos.y = rooms[room]->y1; pos.y <= rooms[room]->y2; pos.y++)
    {
        for (pos.x = rooms[room]->x1; pos.x <= rooms[room]->x2; pos.x++)
        {
            if ( (pos.y == rooms[room]->y1)
                    || (pos.y == rooms[room]->y2)
                    || (pos.x == rooms[room]->x1)
                    || (pos.x == rooms[room]->x2) )
            {
                /* if we are on the border of a room, make wall */
                map_tiletype_set(maze, pos, LT_WALL);
            }
            else
            {
                /* create loot */
                itm = item_new_random(IT_GOLD);
                inv_add(map_ilist_at(maze, pos), itm);

                /* create a monster */
                monster_new_by_level(pos);
            }

            /* now clear out interior */
            if ((mst = map_stationary_at(maze, pos)))
            {
                success = FALSE;
                do
                {

                    npos = map_find_space(maze, LE_STATIONARY);
                    if (!pos_in_rect(npos, *rooms[room]))
                    {
                        /* pos is outside of room */
                        map_stationary_set(maze, npos, mst);
                        map_stationary_set(maze, pos, LS_NONE);

                        success = TRUE;
                    }
                }
                while (!success);
            } /* if map_stationary_at() */
        } /* for x */
    } /* for y */

    /* place the door on the treasure room */
    switch (rand_1n(2))
    {
    case 1: /* horizontal */
        pos.x = rand_m_n(rooms[room]->x1 + 1, rooms[room]->x2 - 1);
        pos.y = rand_0n(1) ? rooms[room]->y1 : rooms[room]->y2;
        break;

    case 2: /* vertical */
        pos.x = rand_0n(1) ? rooms[room]->x1 : rooms[room]->x2;
        pos.y = rand_m_n(rooms[room]->y1 + 1, rooms[room]->y2 - 1);
        break;
    };

    map_tiletype_set(maze, pos, LT_FLOOR);
    map_stationary_set(maze, pos, LS_CLOSEDDOOR);
}

/* verify that every space on the map can be reached */
static int map_validate(map *maze)
{
    position pos;
    int connected = TRUE;
    area *floodmap = NULL;
    area *obsmap = area_new(0, 0, MAP_MAX_X, MAP_MAX_Y);

    pos.z = maze->nlevel;

    /* generate an obstacle map */
    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
            if (!map_pos_passable(maze, pos)
                    && (map_stationary_at(maze, pos) != LS_CLOSEDDOOR))
                area_point_set(obsmap, pos.x, pos.y);

    /* get position of entrance */
    switch (maze->nlevel)
    {
        /* caverns entrance */
    case 1:
        pos = map_find_stationary(maze, LS_ENTRANCE);
        break;

        /* volcano entrance */
    case MAP_MAX - 1:
        pos = map_find_stationary(maze, LS_ELEVATORUP);
        break;

    default:
        pos = map_find_stationary(maze, LS_STAIRSDOWN);
        break;
    }

    /* flood fill the maze starting at the entrance */
    floodmap = area_flood(obsmap, pos.x, pos.y);

    /* compare flooded area with obstacle map */
    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            int pp = map_pos_passable(maze, pos);
            int cd = (map_stationary_at(maze, pos) == LS_CLOSEDDOOR);

            /* point should be set on floodmap if it is passable */
            if (area_point_get(floodmap, pos.x, pos.y) != (pp || cd))
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
static void map_item_add(map *maze, item *what)
{
    position pos;

    pos = map_find_space(maze, LE_ITEM);

    inv_add(map_ilist_at(maze, pos), what);
}

static map_path *map_path_new(position start, position goal)
{
    map_path *path;

    path = g_malloc0(sizeof(map_path));

    path->open = g_ptr_array_new();
    path->closed = g_ptr_array_new();
    path->path = g_queue_new();

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

/* Returns cost from position defined by element to goal.*/
static int map_path_cost(map *l, map_path_element* element, position target)
{
    element->h_score = pos_distance(element->pos, target);

    /* penalize fields occupied by monsters */
    if (map_is_monster_at(l, element->pos))
        element->h_score += 10;

    /* penalize fields covered with water, fire or cloud */
    if ((map_tiletype_at(l, element->pos) == LT_FIRE)
            || (map_tiletype_at(l, element->pos) == LT_WATER)
            || (map_tiletype_at(l, element->pos) == LT_CLOUD))
    {
        element->h_score += 50;
    }


    return element->g_score + element->h_score;
}

static map_path_element *map_path_element_in_list(map_path_element* el, GPtrArray *list)
{
    guint idx;
    map_path_element *li;

    assert(el != NULL && list != NULL);

    for (idx = 0; idx < list->len; idx++)
    {
        li = g_ptr_array_index(list, idx);

        if (pos_identical(li->pos, el->pos))
        {
            return li;
        }
    }

    return NULL;
}

static map_path_element *map_path_find_best(map *l, map_path *path)
{
    map_path_element *el, *best = NULL;
    guint idx;

    for (idx = 0; idx < path->open->len; idx++)
    {
        el = g_ptr_array_index(path->open, idx);

        if (best == NULL || map_path_cost(l, el, path->goal) < map_path_cost(l, best, path->goal))
        {
            best = el;
        }
    }

    return best;
}

static GPtrArray *map_path_get_neighbours(map *l, position pos)
{
    GPtrArray *neighbours;
    map_path_element *pe;
    position npos;
    direction dir;

    neighbours = g_ptr_array_new();

    for (dir = GD_NONE + 1; dir < GD_MAX; dir++)
    {
        if (dir == GD_CURR)
            continue;

        npos = pos_move(pos, dir);

        if (pos_valid(npos)
                && lt_is_passable(map_tiletype_at(l, npos)))
        {
            pe = map_path_element_new(npos);
            g_ptr_array_add(neighbours, pe);
        }
    }

    return neighbours;
}

static gboolean map_monster_destroy(gpointer key, monster *monst, map *m)
{
    if (monster_pos(monst).z != m->nlevel)
    {
        return FALSE;
    }

    monster_oid_set(monst, 0);
    monster_destroy(monst);

    return TRUE;

}

static gboolean map_sphere_destroy(sphere *s, map *m)
{
    if (s->pos.z != m->nlevel)
    {
        return FALSE;
    }

    sphere_destroy(s, nlarn);

    return TRUE;
}
