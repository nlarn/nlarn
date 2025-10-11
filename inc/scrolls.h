/*
 * scrolls.h
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

#ifndef SCROLLS_H
#define SCROLLS_H

#include "enumFactory.h"
#include "items.h"

/* forward declaration */

struct player;

/* type definitions */

#define SCROLL_TYPE_ENUM(SCROLL_TYPE) \
    SCROLL_TYPE(ST_BLANK,) \
    SCROLL_TYPE(ST_ENCH_ARMOUR,) \
    SCROLL_TYPE(ST_ENCH_WEAPON,) \
    SCROLL_TYPE(ST_ENLIGHTENMENT,) \
    SCROLL_TYPE(ST_CREATE_MONSTER,) \
    SCROLL_TYPE(ST_CREATE_ARTIFACT,) \
    SCROLL_TYPE(ST_AGGRAVATE_MONSTER,) \
    SCROLL_TYPE(ST_TIMEWARP,) \
    SCROLL_TYPE(ST_TELEPORT,) \
    SCROLL_TYPE(ST_AWARENESS,) \
    SCROLL_TYPE(ST_SPEED,) \
    SCROLL_TYPE(ST_HEAL_MONSTER,) \
    SCROLL_TYPE(ST_SPIRIT_PROTECTION,) \
    SCROLL_TYPE(ST_UNDEAD_PROTECTION,) \
    SCROLL_TYPE(ST_STEALTH,) \
    SCROLL_TYPE(ST_MAPPING,) \
    SCROLL_TYPE(ST_HOLD_MONSTER,) \
    SCROLL_TYPE(ST_GEM_PERFECTION,) \
    SCROLL_TYPE(ST_SPELL_EXTENSION,) \
    SCROLL_TYPE(ST_IDENTIFY,) \
    SCROLL_TYPE(ST_REMOVE_CURSE,) \
    SCROLL_TYPE(ST_ANNIHILATION,) \
    SCROLL_TYPE(ST_PULVERIZATION,) \
    SCROLL_TYPE(ST_LIFE_PROTECTION,) \
    SCROLL_TYPE(ST_GENOCIDE_MONSTER,) \
    SCROLL_TYPE(ST_MAX,)

DECLARE_ENUM(scroll_t, SCROLL_TYPE_ENUM)

typedef struct magic_scroll_data {
    scroll_t   id;
    const char *name;
    effect_t   effect;      /* effect causes by this scroll */
    int        price;
    int        store_stock; /* count in the store's starting stock */
} magic_scroll_data;

/* function declarations */

char *scroll_desc(scroll_t id);
item_usage_result scroll_read(struct player *p, item *scroll);
int scroll_mapping(struct player *p, item *scroll);

/* external vars */

extern const magic_scroll_data scrolls[ST_MAX];

/* macros */

#define scroll_type_store_stock(id) (scrolls[(id)].store_stock)

#define scroll_name(scroll)   (scrolls[(scroll)->id].name)
#define scroll_effect(scroll) (scrolls[(scroll)->id].effect)
#define scroll_price(scroll)  (scrolls[(scroll)->id].price)

#endif
