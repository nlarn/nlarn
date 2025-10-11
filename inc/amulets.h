/*
 * amulets.h
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

#ifndef AMULETS_H
#define AMULETS_H

#include "items.h"

#define AMULET_TYPE_ENUM(AMULET_TYPE) \
    AMULET_TYPE(AM_AWARENESS,) \
    AMULET_TYPE(AM_SUSTAINMENT,) \
    AMULET_TYPE(AM_UNDEAD_CONTROL,) \
    AMULET_TYPE(AM_NEGATE_SPIRIT,) \
    AMULET_TYPE(AM_NOTHEFT,) \
    AMULET_TYPE(AM_DRAGON_SLAYING,) \
    AMULET_TYPE(AM_POWER,) \
    AMULET_TYPE(AM_REFLECTION,) \
    AMULET_TYPE(AM_LARN,) \
    AMULET_TYPE(AM_MAX,)

DECLARE_ENUM(amulet_t, AMULET_TYPE_ENUM)

typedef enum amulet_type
{
    AMULET,
    TALISMAN
} amulet_type;

typedef struct amulet_data
{
    amulet_t id;
    amulet_type typ;  /* talisman or amulet? */
    const char *name;
    effect_t effect; /* effect causes by this amulet */
    int price;       /* base price in the shops */
} amulet_data;

/* external vars */

extern const amulet_data amulets[AM_MAX];

/* function declarations */

item_material_t amulet_material(amulet_t amulet_id);

/* macros */

#define amulet_type(item)        (amulets[(item)->id].typ)
#define amulet_name(item)        (amulets[(item)->id].name)
#define amulet_effect_t(item)    (amulets[(item)->id].effect)
#define amulet_price(item)       (amulets[(item)->id].price)

#endif
