/*
 * potions.h
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
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

#ifndef __POTIONS_H_
#define __POTIONS_H_

#include "items.h"

/* forward declarations */

struct player;

/* type definitions */

typedef struct potion_data {
    int id;
    const char *name;
    int effect_t;       /* if this potion causes any effect */
    int price;
    unsigned
        obtainable: 1;  /* available in the shop */
} potion_data;

enum potion_objects {
    PO_NONE,
    PO_WATER,
    PO_SLEEP,
    PO_HEAL,
    PO_INC_LEVEL,
    PO_INC_RND,
    PO_INC_STR,
    PO_LEARNING,
    PO_INC_WIS,
    PO_INC_CON,
    PO_RECOVERY,
    PO_DIZZINESS,
    PO_OBJ_DETECT,
    PO_MON_DETECT,
    PO_AMNESIA,
    PO_BLINDNESS,
    PO_CONFUSION,
    PO_HEROISM,
    PO_GIANT_STR,
    PO_FIRE_RES,
    PO_TRE_DETECT,
    PO_MAX_HP,
    PO_INC_MP,
    PO_POISON,
    PO_SEE_INVISIBLE,
    PO_LEVITATION,
    PO_CURE_DIANTHR,
    PO_MAX
};

/* function declarations */

char *potion_desc(int potion_id);
int potion_colour(int potion_id);
int potion_throw(struct player *p);
item_usage_result potion_quaff(struct player *p, item *potion);

/* external vars */

extern const potion_data potions[PO_MAX];

/* macros */

#define potion_type_obtainable(id) (potions[id].obtainable)

#define potion_name(potion)   (potions[(potion)->id].name)
#define potion_effect(potion) (potions[(potion)->id].effect_t)
#define potion_price(potion)  (potions[(potion)->id].price)

#endif
