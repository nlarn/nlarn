/*
 * map.h
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

#include "cJSON.h"
#include "colours.h"
#include "enumFactory.h"
#include "items.h"
#include "monsters.h"
#include "position.h"
#include "sobjects.h"
#include "traps.h"
#include "utils.h"

/* map dimensions */
#define MAP_MAX_X 67
#define MAP_MAX_Y 17
#define MAP_SIZE MAP_MAX_X*MAP_MAX_Y

/* number of levels */
#define MAP_CMAX 11                   /* max # levels in the caverns */
#define MAP_VMAX  3                   /* max # of levels in the temple of the luran */
#define MAP_MAX (MAP_CMAX + MAP_VMAX) /* total number of levels */

/* number of the last custom maze map (including town) */
#define MAP_MAX_MAZE_NUM 24
#define MAP_MAZE_NUM     (MAP_MAX_MAZE_NUM + 1)

#define MAP_TILE_TYPE_ENUM(MAP_TILE_TYPE) \
    MAP_TILE_TYPE(LT_NONE,) \
    MAP_TILE_TYPE(LT_MOUNTAIN,) \
    MAP_TILE_TYPE(LT_GRASS,) \
    MAP_TILE_TYPE(LT_DIRT,) \
    MAP_TILE_TYPE(LT_TREE,) \
    MAP_TILE_TYPE(LT_FLOOR,) \
    MAP_TILE_TYPE(LT_WATER,) \
    MAP_TILE_TYPE(LT_DEEPWATER,) \
    MAP_TILE_TYPE(LT_LAVA,) \
    MAP_TILE_TYPE(LT_FIRE,) \
    MAP_TILE_TYPE(LT_CLOUD,) /* gas cloud */ \
    MAP_TILE_TYPE(LT_WALL,) \
    MAP_TILE_TYPE(LT_MAX,)   /* ~ map tile type count */ \

DECLARE_ENUM(map_tile_t, MAP_TILE_TYPE_ENUM)

typedef enum map_element_type
{
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
    guint64
        type:       8,
        base_type:  8, /* if tile is covered with e.g. fire the original type is stored here */
        timer:      8, /* countdown to when the type will become base_type again */
        sobject:    8, /* something special located on this tile */
        trap:       8, /* trap located on this tile */
        spill:      8, /* base colour of the liquid spilled here */
        spilltime:  8; /* countdown for the time the spilled liquid is visible */
    gpointer m_oid;    /* id of monster located on this tile */
    inventory *ilist;  /* items located on this tile */
} map_tile;

typedef struct map_tile_data
{
    map_tile_t tile;
    const char glyph;
    int colour;
    const char *description;
    unsigned
        passable:    1,     /* can be passed */
        transparent: 1;     /* see-through */
} map_tile_data;

typedef struct map
{
    guint32 nlevel;                       /* map number */
    guint32 visited;                      /* last time player has been on this map */
    guint32 mcount;                       /* monster count */
    map_tile grid[MAP_MAX_Y][MAP_MAX_X];  /* the map */
} map;

/* callback function for trajectories */
typedef gboolean (*trajectory_hit_sth)(const GList *trajectory,
        const damage_originator *damo,
        gpointer data1, gpointer data2);

/* function declarations */

map *map_new(int num, const char *mazefile);
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

position map_find_sobject(map *m, sobject_t sobject);

gboolean map_pos_validate(map *m,
                          position pos,
                          map_element_t element,
                          gboolean dead_end);

void map_item_add(map *m, item *it);

int *map_get_surrounding(map *m, position pos, sobject_t type);

/**
 * determine if a position can be seen from another position
 *
 * @param the map
 * @param first position
 * @param second position
 * @return true or false
 */
int map_pos_is_visible(map *m, position source, position target);

/**
 * Return a linked list with every position between two points.
 *
 * @param The map that contains both positions.
 * @param The starting position.
 * @param The destination.
 */
GList *map_ray(map *m, position source, position target);

/**
 * Follow a ray from target to destination.
 *
 * @param The starting position.
 * @param The destination.
 * @param The originator of the trajectory.
 * @param The callback function for every affected position.
 * @param A pointer passed to the callback function.
 * @param A pointer passed to the callback function.
 * @param true if reflection should be honoured.
 * @param The glyph to display at an affected position
 * @param The colour of the glyph.
 * @param true to keep the glyph at affected positions.
 *
 * @return true if one of the callbacks returned true.
 */
gboolean map_trajectory(position source, position target,
        const damage_originator *damo,
        trajectory_hit_sth pos_hitfun,
        gpointer data1, gpointer data2, gboolean reflectable,
        char glyph, colour fg, gboolean keep_ray);

/**
 * @brief Get an area of defined dimensions with all blocked positions set.
 *
 * @param A map.
 * @param The center position.
 * @param The radius.
 * @param Shall closed doors be handled as passable?
 *
 * @return A freshly allocated area with all impassable positions set.
 */
area *map_get_obstacles(map *m, position center, int radius, gboolean doors);

void map_set_tiletype(map *m, area *area, map_tile_t type, guint8 duration);

damage *map_tile_damage(map *m, position pos, gboolean flying);

/**
 * @brief Creates description of items on the floor for a given position.
 *
 * @param a map
 * @param a position
 * @param "here" or "there"
 * @param a filter function to restrict the described items
 */
char *map_inv_description(map *m, position pos, const char* where, int (*ifilter)(item *));

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

/**
 * Get the display details for a given map tile.
 */
struct display_cell map_get_tile(map *m, position pos);

/* external vars */

extern const map_tile_data map_tiles[LT_MAX];
extern const char *map_names[MAP_MAX];

/* inline accessor functions */

static inline map_tile *map_tile_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return &m->grid[Y(pos)][X(pos)];
}

static inline inventory **map_ilist_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return &m->grid[Y(pos)][X(pos)].ilist;
}

static inline map_tile_t map_tiletype_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].type;
}

static inline void map_tiletype_set(map *m, position pos, map_tile_t type)
{
    g_assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].type = type;
}

static inline map_tile_t map_basetype_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].base_type;
}

static inline void map_basetype_set(map *m, position pos, map_tile_t type)
{
    g_assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].base_type = type;
}

static inline guint8 map_timer_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].timer;
}

static inline trap_t map_trap_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].trap;
}

static inline void map_trap_set(map *m, position pos, trap_t type)
{
    g_assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].trap = type;
}

static inline guint8 map_spill_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].spill;
}

static inline void map_spill_set(map *m, position pos, int colour)
{
    g_assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].spill = colour;
    m->grid[Y(pos)][X(pos)].spilltime = 20;
}

static inline sobject_t map_sobject_at(map *m, position pos)
{
    g_assert(m != NULL && pos_valid(pos));
    return m->grid[Y(pos)][X(pos)].sobject;
}

static inline void map_sobject_set(map *m, position pos, sobject_t type)
{
    g_assert(m != NULL && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].sobject = type;
}

static inline void map_set_monster_at(map *m, position pos, monster *monst)
{
    g_assert(m != NULL && m->nlevel == Z(pos) && pos_valid(pos));
    m->grid[Y(pos)][X(pos)].m_oid = (monst != NULL) ? monster_oid(monst) : NULL;
}

static inline gboolean map_is_monster_at(map *m, position pos)
{
    g_assert(m != NULL);
    return ((map_get_monster_at(m, pos) != NULL));
}

static inline char mt_get_glyph(map_tile_t t)
{
    return map_tiles[t].glyph;
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

static inline const char *map_name(map *m)
{
    return map_names[m->nlevel];
}

static inline gboolean map_pos_transparent(map *m, position pos)
{
    return mt_is_transparent(m->grid[Y(pos)][X(pos)].type)
        && so_is_transparent(m->grid[Y(pos)][X(pos)].sobject);
}

static inline gboolean map_pos_passable(map *m, position pos)
{
    return mt_is_passable(m->grid[Y(pos)][X(pos)].type)
        && so_is_passable(m->grid[Y(pos)][X(pos)].sobject);
}

#endif
