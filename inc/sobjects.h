/*
 * sobjects.h
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

#ifndef __SOBJECTS_H_
#define __SOBJECTS_H_

#include "colours.h"

/* forward declarations */
struct player;
struct map;

typedef enum sobject_type
{
    LS_NONE,
    LS_ALTAR,
    LS_THRONE,        /* throne with gems and king */
    LS_THRONE2,       /* throne with gems, without king */
    LS_DEADTHRONE,    /* throne without gems or king */
    LS_STAIRSDOWN,
    LS_STAIRSUP,
    LS_ELEVATORDOWN,  /* Enter the volcano */
    LS_ELEVATORUP,    /* leave the volcano */
    LS_FOUNTAIN,
    LS_DEADFOUNTAIN,
    LS_STATUE,
    LS_URN,           /* golden urn - not implemented */
    LS_MIRROR,
    LS_OPENDOOR,
    LS_CLOSEDDOOR,
    LS_CAVERNS_ENTRY,
    LS_CAVERNS_EXIT,
    LS_HOME,
    LS_DNDSTORE,
    LS_TRADEPOST,
    LS_LRS,           /* Larn Revenue Service */
    LS_SCHOOL,
    LS_BANK,
    LS_BANK2,         /* branch office */
    LS_MONASTERY,
    LS_MAX
} sobject_t;

typedef struct _sobject_data
{
    sobject_t sobject;
    const char glyph;
    colour fg;
    const char *description;
    unsigned
        passable:     1,   /* can be passed */
        transparent:  1;   /* see-through */
} sobject_data;

extern const sobject_data sobjects[LS_MAX];

static inline char so_get_glyph(sobject_t s)
{
    return sobjects[s].glyph;
}

static inline colour so_get_colour(sobject_t s)
{
    return sobjects[s].fg;
}

static inline const char *so_get_desc(sobject_t s)
{
    return sobjects[s].description;
}

static inline gboolean so_is_passable(sobject_t s)
{
    return sobjects[s].passable;
}

static inline gboolean so_is_transparent(sobject_t s)
{
    return sobjects[s].transparent;
}

/* deal with stationary objects */
int player_altar_desecrate(struct player *p);
int player_altar_pray(struct player *p);
int player_building_enter(struct player *p);
int player_door_close(struct player *p);
int player_door_open(struct player *p, int dir);
int player_fountain_drink(struct player *p);
int player_fountain_wash(struct player *p);
int player_stairs_down(struct player *p);
int player_stairs_up(struct player *p);
int player_throne_pillage(struct player *p);
int player_throne_sit(struct player *p);
void sobject_destroy_at(struct player *p, struct map *dmap, position pos);

#endif
