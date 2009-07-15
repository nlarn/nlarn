/*
 * level.h
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

#ifndef __LEVEL_H_
#define __LEVEL_H_

#include "display.h"
#include "monsters.h"
#include "position.h"
#include "traps.h"
#include "utils.h"

/* dungeon dimensions */
#define LEVEL_MAX_X 67
#define LEVEL_MAX_Y 17
#define LEVEL_SIZE LEVEL_MAX_X*LEVEL_MAX_Y

/* number of levels */
#define LEVEL_DMAX 11						/* max # levels in the dungeon */
#define LEVEL_VMAX  3						/* max # of levels in the temple of the luran */
#define LEVEL_MAX (LEVEL_DMAX + LEVEL_VMAX)	/* total number of levels */

typedef enum level_tile_type
{
    LT_NONE,
    LT_GRASS,
    LT_FLOOR,
    LT_WATER,
    LT_DEEPWATER,
    LT_LAVA,
    LT_FIRE,
    LT_CLOUD, /* gas cloud */
    LT_WALL,
    LT_MAX				/* ~ level tile type count */
} level_tile_t;

typedef enum level_stationary_type
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
} level_stationary_t;

typedef enum level_element_type
{
    LE_NONE,
    LE_GROUND,
    LE_STATIONARY,
    LE_TRAP,
    LE_ITEM,
    LE_MONSTER,
    LE_MAX
} level_element_t;

typedef struct level_tile
{
    level_tile_t type;
    level_stationary_t stationary; /* if something special is here */
    trap_t trap; /* trap located on this tile */
    guint32 timer; /* countdown to when the type will become LT_FLOOR again */
    inventory *ilist; /* items located on this tile */
} level_tile;

typedef struct level_tile_data
{
    level_tile_t tile;
    char image;
    short colour;
    char *description;
    unsigned
passable:
    1,        /* can be passed */
transparent:
    1;     /* see-through */
} level_tile_data;

typedef struct level_stationary_data
{
    level_stationary_t stationary;
    char image;
    short colour;
    char *description;
    unsigned
passable:
    1,        /* can be passed */
transparent:
    1;     /* see-through */
} level_stationary_data;

typedef struct level
{
    guint32 nlevel;                             /* level number */
    level_tile map[LEVEL_MAX_Y][LEVEL_MAX_X];   /* "live" map */
    guint32 visited;                            /* last time player has been on this level */
    GPtrArray *mlist;                           /* monsters on this level */
    GPtrArray *slist;                           /* spheres of annihilation on this level */
} level;

/* Structure for path elements */
typedef struct level_path_element
{
    position pos;
    guint32 g_score;
    guint32 h_score;
    struct level_path_element* parent;
} level_path_element;

typedef struct level_path
{
    GQueue *path;
    GPtrArray *closed;
    GPtrArray *open;
    position start;
    position goal;
} level_path;

/* function declarations */

void level_new(level *l, int difficulty, char *mazefile);
void level_destroy(level *l);
void level_dump(level *l);

position level_find_space(level *l, level_element_t element);
position level_find_space_in(level *l, rectangle where, level_element_t element);
position level_find_stationary(level *l, level_stationary_t stationary);
position level_find_stationary_in(level *l, level_stationary_t stationary, rectangle area);

int *level_get_surrounding(level *l, position pos, level_stationary_t type);

int level_pos_is_visible(level *l, position source, position target);
level_path *level_find_path(level *l, position start, position goal);
void level_path_destroy(level_path *path);

area *level_get_obstacles(level *l, position center, int radius);
void level_set_tiletype(level *l, area *area, level_tile_t type, guint8 duration);

int level_tile_damage(level *l, position pos);

monster *level_get_monster_at(level *l, position pos);
int level_is_monster_at(level *l, position pos);
GPtrArray *level_get_monsters_in(level *l, rectangle area);
int level_fill_with_live(level *l);

void level_expire_timer(level *l, guint8 count);

/* external vars */

extern const level_tile_data level_tiles[LT_MAX];
extern const level_stationary_data level_stationaries[LS_MAX];

/* Macros */

#define lt_get_image(tile)      (level_tiles[(tile)].image)
#define lt_get_colour(tile)     (level_tiles[(tile)].colour)
#define lt_get_desc(tile)       (level_tiles[(tile)].description)
#define lt_is_passable(tile)    (level_tiles[(tile)].passable)
#define lt_is_transparent(tile) (level_tiles[(tile)].transparent)

#define ls_get_image(stationary)      (level_stationaries[(stationary)].image)
#define ls_get_colour(stationary)     (level_stationaries[(stationary)].colour)
#define ls_get_desc(stationary)       (level_stationaries[(stationary)].description)
#define ls_is_passable(stationary)    (level_stationaries[(stationary)].passable)
#define ls_is_transparent(stationary) (level_stationaries[(stationary)].transparent)

#define level_tile_at(l,pos)       (pos_valid(pos) ? &((l)->map[(pos).y][(pos).x]) : NULL)
#define level_ilist_at(l,pos)      ((l)->map[(pos).y][(pos).x].ilist)
#define level_tiletype_at(l,pos)   ((l)->map[(pos).y][(pos).x].type)
#define level_timer_at(l,pos)      ((l)->map[(pos).y][(pos).x].timer)
#define level_trap_at(l,pos)       ((l)->map[(pos).y][(pos).x].trap)
#define level_stationary_at(l,pos) ((l)->map[(pos).y][(pos).x].stationary)

#define level_pos_transparent(l,pos) (lt_is_transparent((l)->map[(pos).y][(pos).x].type) \
                                        && ls_is_transparent((l)->map[(pos).y][(pos).x].stationary))

#define level_pos_passable(l,pos) (lt_is_passable((l)->map[(pos).y][(pos).x].type) \
                                        && ls_is_passable((l)->map[(pos).y][(pos).x].stationary))

#endif
