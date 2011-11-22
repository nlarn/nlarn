/*
 * map.h
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MAP_H_
#define __MAP_H_

#include <assert.h>

#include "cJSON.h"
#include "items.h"
#include "monsters.h"
#include "position.h"
#include "traps.h"
#include "utils.h"

/* dungeon dimensions */
#define MAP_MAX_X 67
#define MAP_MAX_Y 17
#define MAP_SIZE MAP_MAX_X*MAP_MAX_Y

/* number of levels */
#define MAP_DMAX 11						/* max # levels in the dungeon */
#define MAP_VMAX  3						/* max # of levels in the temple of the luran */
#define MAP_MAX (MAP_DMAX + MAP_VMAX)	/* total number of levels */

/* number of the last custom maze map (including town) */
#define MAP_MAX_MAZE_NUM 24
#define MAP_MAZE_NUM     (MAP_MAX_MAZE_NUM + 1)

typedef enum map_tile_type
{
    LT_NONE,
    LT_MOUNTAIN,
    LT_GRASS,
    LT_DIRT,
    LT_TREE,
    LT_FLOOR,
    LT_WATER,
    LT_DEEPWATER,
    LT_LAVA,
    LT_FIRE,
    LT_CLOUD, /* gas cloud */
    LT_WALL,
    LT_MAX				/* ~ map tile type count */
} map_tile_t;

typedef enum map_sobject_type
{
    LS_NONE,
    LS_ALTAR,
    LS_THRONE,			/* throne with gems and king */
    LS_THRONE2,			/* throne with gems, without king */
    LS_DEADTHRONE,		/* throne without gems or king */
    LS_STAIRSDOWN,
    LS_STAIRSUP,
    LS_ELEVATORDOWN,	/* Enter the volcano */
    LS_ELEVATORUP,		/* leave the volcano */
    LS_FOUNTAIN,
    LS_DEADFOUNTAIN,
    LS_STATUE,
    LS_URN,				/* golden urn - not implemented */
    LS_MIRROR,
    LS_OPENDOOR,
    LS_CLOSEDDOOR,
    LS_DNGN_ENTRANCE,   /* dungeon entrance */
    LS_DNGN_EXIT,       /* leave the dungeon */
    LS_HOME,
    LS_DNDSTORE,
    LS_TRADEPOST,
    LS_LRS,				/* Larn Revenue Service */
    LS_SCHOOL,
    LS_BANK,
    LS_BANK2,			/* branch office */
    LS_MONASTERY,
    LS_MAX
} map_sobject_t;

typedef enum map_element_type
{
    LE_NONE,
    LE_GROUND,
    LE_SOBJECT,
    LE_TRAP,
    LE_ITEM,
    LE_MONSTER,
    LE_SWIMMING_MONSTER,
    LE_FLYING_MONSTER,
    LE_XORN,    /* can move through walls */
    LE_MAX
} map_element_t;

typedef struct map_tile
{
    guint32
        type:       8,
        base_type:  8, /* if tile is covered with e.g. fire the original type is stored here */
        sobject:    8, /* something special located on this tile */
        trap:       8; /* trap located on this tile */
    guint8 timer;      /* countdown to when the type will become base_type again */
    gpointer monster;  /* id of monster located on this tile */
    inventory *ilist;  /* items located on this tile */
} map_tile;

typedef struct map_tile_data
{
    map_tile_t tile;
    const char image;
    int colour;
    const char *description;
    unsigned
        passable:    1,     /* can be passed */
        transparent: 1;     /* see-through */
} map_tile_data;

typedef struct map_sobject_data
{
    map_sobject_t sobject;
    const char image;
    int colour;
    const char *description;
    unsigned
        passable:     1,   /* can be passed */
        transparent:  1;   /* see-through */
} map_sobject_data;

typedef struct map
{
    guint32 nlevel;                       /* map number */
    guint32 visited;                      /* last time player has been on this map */
    guint32 mcount;                       /* monster count */
    map_tile grid[MAP_MAX_Y][MAP_MAX_X];  /* the map */
} map;

/* Structure for path elements */
typedef struct map_path_element
{
    position pos;
    guint32 g_score;
    guint32 h_score;
    struct map_path_element* parent;
} map_path_element;

typedef struct map_path
{
    GQueue *path;
    GPtrArray *closed;
    GPtrArray *open;
    position start;
    position goal;
} map_path;

/* function declarations */

map *map_new(int num, char *mazefile);
void map_destroy(map *m);

cJSON *map_serialize(map *m);
map *map_deserialize(cJSON *mser);
char *map_dump(map *m, position ppos);

position map_find_space(map *m, map_element_t element,
                        gboolean dead_end);

position map_find_space_in(map *m,
                           rectangle where,
                           map_element_t element,
                           gboolean dead_end);

position map_find_sobject(map *m, map_sobject_t sobject);

position map_find_sobject_in(map *m,
                             map_sobject_t sobject,
                             rectangle area);

gboolean map_pos_validate(map *m,
                          position pos,
                          map_element_t element,
                          gboolean dead_end);

void map_item_add(map *m, item *it);

int *map_get_surrounding(map *m, position pos, map_sobject_t type);

/**
 * determine if a position can be seen from another position
 *
 * @param the map
 * @param first position
 * @param second position
 * @return TRUE or FALSE
 */
int map_pos_is_visible(map *m, position source, position target);

/**
 * @brief Find a path between two positions
 *
 * @param the map to work on
 * @param the starting position
 * @param the destination
 * @param the map_element_t that can be travelled
 * @return a path or NULL if none could be found
 */
map_path *map_find_path(map *m, position start, position goal,
                        map_element_t element);

/**
 * @brief Free memory allocated for a given map path.
 *
 * @param a map path returned by <map_find_path>"()"
 */
void map_path_destroy(map_path *path);

area *map_get_obstacles(map *m, position center, int radius);

void map_set_tiletype(map *m, area *area, map_tile_t type, guint8 duration);

damage *map_tile_damage(map *m, position pos, gboolean flying);

char *map_pos_examine(position pos);

monster *map_get_monster_at(map *m, position pos);

/**
 * @brief Creates new monsters for a map.
 *
 * @param a map
 */
void map_fill_with_life(map *m);

gboolean map_is_exit_at(map *m, position pos);

/**
 * Process temporary effects for a map.
 *
 * @param the map on which timed events have to be processed
 */
void map_timer(map *m);

/**
 * @brief Get the glyph for a door.
 *
 * @param The map.
 * @param The position of the door.
 *
 * @return The glyph for the door.
 */
char map_get_door_glyph(map *m, position pos);

/* external vars */

extern const map_tile_data map_tiles[LT_MAX];
extern const map_sobject_data map_sobjects[LS_MAX];
extern const char *map_names[MAP_MAX];

/* inline accessor functions */

static inline map_tile *map_tile_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return &m->grid[Y(pos)][X(pos)];
}

static inline inventory **map_ilist_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return &m->grid[Y(pos)][X(pos)].ilist;
}

static inline map_tile_t map_tiletype_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].type;
}

static inline void map_tiletype_set(map *m, position pos, map_tile_t type)
{
    assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].type = type;
}

static inline map_tile_t map_basetype_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].base_type;
}

static inline void map_basetype_set(map *m, position pos, map_tile_t type)
{
    assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].base_type = type;
}

static inline guint8 map_timer_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].timer;
}

static inline trap_t map_trap_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].trap;
}

static inline void map_trap_set(map *m, position pos, trap_t type)
{
    assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].trap = type;
}

static inline map_sobject_t map_sobject_at(map *m, position pos)
{
    assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].sobject;
}

static inline void map_sobject_set(map *m, position pos, map_sobject_t type)
{
    assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].sobject = type;
}

static inline void map_set_monster_at(map *m, position pos, monster *monst)
{
    assert(m != NULL && m->nlevel == Z(pos) && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].monster = (monst != NULL) ? monster_oid(monst) : NULL;
}

static inline gboolean map_is_monster_at(map *m, position pos)
{
    assert(m != NULL);
    return ((map_get_monster_at(m, pos) != NULL));
}

static inline char mt_get_image(map_tile_t t)
{
    return map_tiles[t].image;
}

static inline int  mt_get_colour(map_tile_t t)
{
    return map_tiles[t].colour;
}

static inline const char *mt_get_desc(map_tile_t t)
{
    return map_tiles[t].description;
}

static inline gboolean mt_is_passable(map_tile_t t)
{
    return map_tiles[t].passable;
}

static inline gboolean mt_is_transparent(map_tile_t t)
{
    return map_tiles[t].transparent;
}

static inline char mso_get_image(map_sobject_t s)
{
    return map_sobjects[s].image;
}

static inline int mso_get_colour(map_sobject_t s)
{
    return map_sobjects[s].colour;
}

static inline const char *mso_get_desc(map_sobject_t s)
{
    return map_sobjects[s].description;
}

static inline gboolean mso_is_passable(map_sobject_t s)
{
    return map_sobjects[s].passable;
}

static inline gboolean mso_is_transparent(map_sobject_t s)
{
    return map_sobjects[s].transparent;
}

static inline const char *map_name(map *m)
{
    return map_names[m->nlevel];
}

static inline gboolean map_pos_transparent(map *m, position pos)
{
    return mt_is_transparent(m->grid[Y(pos)][X(pos)].type)
        && mso_is_transparent(m->grid[Y(pos)][X(pos)].sobject);
}

static inline gboolean map_pos_passable(map *m, position pos)
{
    return mt_is_passable(m->grid[Y(pos)][X(pos)].type)
        && mso_is_passable(m->grid[Y(pos)][X(pos)].sobject);
}

#endif
