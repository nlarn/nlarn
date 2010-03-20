/*
 * map.h
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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
    LE_MAX
} map_element_t;

typedef struct map_tile
{
    guint32
        type:       8,
        base_type:  8, /* if tile is covered with e.g. fire the original type is stored here */
        sobject:    8, /* something special located on this tile */
        trap:       8; /* trap located on this tile */
    guint8 timer;      /* countdown to when the type will become LT_FLOOR again */
    gpointer monster;  /* id of monster located on this tile */
    inventory *ilist;  /* items located on this tile */
} map_tile;

typedef struct map_tile_data
{
    map_tile_t tile;
    char image;
    int colour;
    char *description;
    unsigned
        passable:    1,     /* can be passed */
        transparent: 1;     /* see-through */
} map_tile_data;

typedef struct map_sobject_data
{
    map_sobject_t sobject;
    char image;
    int colour;
    char *description;
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
map *map_deserialize(cJSON *mser, struct game *g);
char *map_dump(map *l, position ppos);

position map_find_space(map *maze, map_element_t element, int dead_end);
position map_find_space_in(map *maze, rectangle where, map_element_t element, int dead_end);
position map_find_sobject(map *l, map_sobject_t sobject);
position map_find_sobject_in(map *l, map_sobject_t sobject, rectangle area);
gboolean map_pos_validate(map *l, position pos, map_element_t element, int dead_end);

int *map_get_surrounding(map *l, position pos, map_sobject_t type);

int map_pos_is_visible(map *l, position source, position target);
map_path *map_find_path(map *l, position start, position goal);
void map_path_destroy(map_path *path);

area *map_get_obstacles(map *l, position center, int radius);
void map_set_tiletype(map *l, area *area, map_tile_t type, guint8 duration);

map_tile *map_tile_at(map *l, position pos);
inventory **map_ilist_at(map *l, position pos);
map_tile_t map_tiletype_at(map *l, position pos);
void map_tiletype_set(map *l, position pos, map_tile_t type);
map_tile_t map_basetype_at(map *l, position pos);
void map_basetype_set(map *l, position pos, map_tile_t type);
guint8 map_timer_at(map *l, position pos);
trap_t map_trap_at(map *l, position pos);
void map_trap_set(map *l, position pos, trap_t type);
map_sobject_t map_sobject_at(map *l, position pos);
void map_sobject_set(map *l, position pos, map_sobject_t type);

damage *map_tile_damage(map *l, position pos);

char *map_pos_examine(position pos);

monster *map_get_monster_at(map *m, position pos);
int map_set_monster_at(map *map, position pos, monster *monst);
int map_is_monster_at(map *m, position pos);
int map_fill_with_life(map *l);

void map_timer(map *l, guint8 count);

/* external vars */

extern const map_tile_data map_tiles[LT_MAX];
extern const map_sobject_data map_sobjects[LS_MAX];
extern const char *map_names[MAP_MAX];

/* Macros */

#define lt_get_image(tile)      (map_tiles[(tile)].image)
#define lt_get_colour(tile)     (map_tiles[(tile)].colour)
#define lt_get_desc(tile)       (map_tiles[(tile)].description)
#define lt_is_passable(tile)    (map_tiles[(tile)].passable)
#define lt_is_transparent(tile) (map_tiles[(tile)].transparent)

#define ls_get_image(sobject)      (map_sobjects[(sobject)].image)
#define ls_get_colour(sobject)     (map_sobjects[(sobject)].colour)
#define ls_get_desc(sobject)       (map_sobjects[(sobject)].description)
#define ls_is_passable(sobject)    (map_sobjects[(sobject)].passable)
#define ls_is_transparent(sobject) (map_sobjects[(sobject)].transparent)

#define map_name(l) (map_names[(l)->nlevel])

#define map_pos_transparent(l,pos) (lt_is_transparent((l)->grid[(pos).y][(pos).x].type) \
                                        && ls_is_transparent((l)->grid[(pos).y][(pos).x].sobject))

#define map_pos_passable(l,pos) (lt_is_passable((l)->grid[(pos).y][(pos).x].type) \
                                        && ls_is_passable((l)->grid[(pos).y][(pos).x].sobject))

#endif
