/*
 * map.h
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

typedef enum map_stationary_type
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
    LS_ENTRANCE,		/* dungeon entrance */
    LS_HOME,
    LS_DNDSTORE,
    LS_TRADEPOST,
    LS_LRS,				/* Larn Revenue Service */
    LS_SCHOOL,
    LS_BANK,
    LS_BANK2,			/* branch office */
    LS_MAX
} map_stationary_t;

typedef enum map_element_type
{
    LE_NONE,
    LE_GROUND,
    LE_STATIONARY,
    LE_TRAP,
    LE_ITEM,
    LE_MONSTER,
    LE_MAX
} map_element_t;

typedef struct map_tile
{
    guint32
        type:       8,
        base_type:  8, /* if tile is covered with e.g. fire the original type is stored here */
        stationary: 8, /* something special located on this tile */
        trap:       8; /* trap located on this tile */
    guint8 timer;      /* countdown to when the type will become LT_FLOOR again */
    inventory *ilist;  /* items located on this tile */
} map_tile;

typedef struct map_tile_data
{
    map_tile_t tile;
    char image;
    short colour;
    char *description;
    unsigned
        passable:    1,     /* can be passed */
        transparent: 1;     /* see-through */
} map_tile_data;

typedef struct map_stationary_data
{
    map_stationary_t stationary;
    char image;
    short colour;
    char *description;
    unsigned
        passable:     1,   /* can be passed */
        transparent:  1;   /* see-through */
} map_stationary_data;

typedef struct map
{
    guint32 nlevel;                             /* map number */
    map_tile grid[MAP_MAX_Y][MAP_MAX_X];  /* "live" map */
    guint32 visited;                            /* last time player has been on this map */
    GPtrArray *mlist;                           /* monsters on this map */
    GPtrArray *slist;                           /* spheres of annihilation on this map */
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

map *map_new(int nlevel, char *mazefile);
void map_destroy(map *l);
char *map_dump(map *l);

position map_find_space(map *l, map_element_t element);
position map_find_space_in(map *l, rectangle where, map_element_t element);
position map_find_stationary(map *l, map_stationary_t stationary);
position map_find_stationary_in(map *l, map_stationary_t stationary, rectangle area);
gboolean map_validate_position(map *l, position pos, map_element_t element);

int *map_get_surrounding(map *l, position pos, map_stationary_t type);

int map_pos_is_visible(map *l, position source, position target);
map_path *map_find_path(map *l, position start, position goal);
void map_path_destroy(map_path *path);

area *map_get_obstacles(map *l, position center, int radius);
void map_set_tiletype(map *l, area *area, map_tile_t type, guint8 duration);

map_tile *map_tile_at(map *l, position pos);
inventory **map_ilist_at(map *l, position pos);
map_tile_t map_tiletype_at(map *l, position pos);
map_tile_t map_basetype_at(map *l, position pos);
guint8 map_timer_at(map *l, position pos);
trap_t map_trap_at(map *l, position pos);
void map_trap_set(map *l, position pos, trap_t type);
map_stationary_t map_stationary_at(map *l, position pos);
void map_stationary_set(map *l, position pos, map_stationary_t type);

damage *map_tile_damage(map *l, position pos);

monster *map_get_monster_at(map *l, position pos);
int map_is_monster_at(map *l, position pos);
GPtrArray *map_get_monsters_in(map *l, rectangle area);
int map_fill_with_live(map *l);

void map_timer(map *l, guint8 count);

/* external vars */

extern const map_tile_data map_tiles[LT_MAX];
extern const map_stationary_data map_stationaries[LS_MAX];
extern const char *map_names[MAP_MAX];

/* Macros */

#define lt_get_image(tile)      (map_tiles[(tile)].image)
#define lt_get_colour(tile)     (map_tiles[(tile)].colour)
#define lt_get_desc(tile)       (map_tiles[(tile)].description)
#define lt_is_passable(tile)    (map_tiles[(tile)].passable)
#define lt_is_transparent(tile) (map_tiles[(tile)].transparent)

#define ls_get_image(stationary)      (map_stationaries[(stationary)].image)
#define ls_get_colour(stationary)     (map_stationaries[(stationary)].colour)
#define ls_get_desc(stationary)       (map_stationaries[(stationary)].description)
#define ls_is_passable(stationary)    (map_stationaries[(stationary)].passable)
#define ls_is_transparent(stationary) (map_stationaries[(stationary)].transparent)

#define map_name(l) (map_names[(l)->nlevel])

#define map_pos_transparent(l,pos) (lt_is_transparent((l)->grid[(pos).y][(pos).x].type) \
                                        && ls_is_transparent((l)->grid[(pos).y][(pos).x].stationary))

#define map_pos_passable(l,pos) (lt_is_passable((l)->grid[(pos).y][(pos).x].type) \
                                        && ls_is_passable((l)->grid[(pos).y][(pos).x].stationary))

#endif
