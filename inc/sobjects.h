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
#include "enumFactory.h"

/* forward declarations */
struct player;
struct map;

#define SOBJECT_TYPE_ENUM(SOBJECT_TYPE) \
    SOBJECT_TYPE(LS_NONE,) \
    SOBJECT_TYPE(LS_ALTAR,) \
    SOBJECT_TYPE(LS_THRONE,)       /* throne with gems and king */ \
    SOBJECT_TYPE(LS_THRONE2,)      /* throne with gems, without king */ \
    SOBJECT_TYPE(LS_DEADTHRONE,)   /* throne without gems or king */ \
    SOBJECT_TYPE(LS_STAIRSDOWN,) \
    SOBJECT_TYPE(LS_STAIRSUP,) \
    SOBJECT_TYPE(LS_ELEVATORDOWN,) /* Enter the volcano */ \
    SOBJECT_TYPE(LS_ELEVATORUP,)   /* leave the volcano */ \
    SOBJECT_TYPE(LS_FOUNTAIN,) \
    SOBJECT_TYPE(LS_DEADFOUNTAIN,) \
    SOBJECT_TYPE(LS_STATUE,) \
    SOBJECT_TYPE(LS_URN,)          /* golden urn - not implemented */ \
    SOBJECT_TYPE(LS_MIRROR,) \
    SOBJECT_TYPE(LS_OPENDOOR,) \
    SOBJECT_TYPE(LS_CLOSEDDOOR,) \
    SOBJECT_TYPE(LS_CAVERNS_ENTRY,) \
    SOBJECT_TYPE(LS_CAVERNS_EXIT,) \
    SOBJECT_TYPE(LS_HOME,) \
    SOBJECT_TYPE(LS_DNDSTORE,) \
    SOBJECT_TYPE(LS_TRADEPOST,) \
    SOBJECT_TYPE(LS_LRS,)          /* Larn Revenue Service */ \
    SOBJECT_TYPE(LS_SCHOOL,) \
    SOBJECT_TYPE(LS_BANK,) \
    SOBJECT_TYPE(LS_BANK2,)        /* branch office */ \
    SOBJECT_TYPE(LS_MONASTERY,) \
    SOBJECT_TYPE(LS_MAX,) \

DECLARE_ENUM(sobject_t, SOBJECT_TYPE_ENUM)

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
