/*
 * potions.h
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

#ifndef POTIONS_H
#define POTIONS_H

#include "enumFactory.h"
#include "items.h"

/* forward declarations */

struct player;

/* type definitions */
#define POTION_TYPE_ENUM(POTION_TYPE) \
    POTION_TYPE(PO_WATER,) \
    POTION_TYPE(PO_SLEEP,) \
    POTION_TYPE(PO_HEAL,) \
    POTION_TYPE(PO_INC_LEVEL,) \
    POTION_TYPE(PO_INC_RND,) \
    POTION_TYPE(PO_INC_STR,) \
    POTION_TYPE(PO_LEARNING,) \
    POTION_TYPE(PO_INC_WIS,) \
    POTION_TYPE(PO_INC_CON,) \
    POTION_TYPE(PO_RECOVERY,) \
    POTION_TYPE(PO_DIZZINESS,) \
    POTION_TYPE(PO_OBJ_DETECT,) \
    POTION_TYPE(PO_MON_DETECT,) \
    POTION_TYPE(PO_AMNESIA,) \
    POTION_TYPE(PO_BLINDNESS,) \
    POTION_TYPE(PO_CONFUSION,) \
    POTION_TYPE(PO_HEROISM,) \
    POTION_TYPE(PO_GIANT_STR,) \
    POTION_TYPE(PO_FIRE_RES,) \
    POTION_TYPE(PO_TRE_DETECT,) \
    POTION_TYPE(PO_MAX_HP,) \
    POTION_TYPE(PO_INC_MP,) \
    POTION_TYPE(PO_POISON,) \
    POTION_TYPE(PO_SEE_INVISIBLE,) \
    POTION_TYPE(PO_LEVITATION,) \
    POTION_TYPE(PO_CURE_DIANTHR,) \
    POTION_TYPE(PO_MAX,)

DECLARE_ENUM(potion_t, POTION_TYPE_ENUM)

typedef struct potion_data {
    potion_t   id;
    const char *name;
    int        effect_t;    /* effect causes by this potion */
    int        price;
    int        store_stock; /* count in the store's starting stock */
} potion_data;

/* function declarations */

char *potion_desc(potion_t potion_id);
colour_t potion_colour(potion_t potion_id);
int potion_throw(struct player *p);
item_usage_result potion_quaff(struct player *p, item *potion);

/* external vars */

extern const potion_data potions[PO_MAX];

/* macros */

#define potion_type_store_stock(id) (potions[(id)].store_stock)

#define potion_name(potion)   (potions[(potion)->id].name)
#define potion_effect(potion) (potions[(potion)->id].effect_t)
#define potion_price(potion)  (potions[(potion)->id].price)

#endif
