/*
 * rings.h
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

#ifndef __RINGS_H_
#define __RINGS_H_

#include "effects.h"
#include "items.h"

enum ring_types
{
    RT_NONE,
    RT_REGENERATION,
    RT_PROTECTION,
    RT_ENERGY,
    RT_DEXTERITY,
    RT_STRENGTH,
    RT_CLEVERNESS,
    RT_INC_DAMAGE,
    RT_EXTRA_REGEN,
    RT_MAX
};

typedef struct ring_data
{
    int id;
    const char *name;
    effect_t effect; /* effect causes by this ring */
    int price;
    unsigned
        obtainable: 1,  /* is available for sale in the shop */
        bonus_obs: 1;   /* can determine bonus by using */
} ring_data;

/* function definitions */

item_material_t ring_material(int ring_id);

/* external vars */

extern const ring_data rings[RT_MAX];

/* macros */

#define ring_name(item)          (rings[(item)->id].name)
#define ring_effect_t(item)   (rings[(item)->id].effect)
#define ring_price(item)         (rings[(item)->id].price)
#define ring_bonus_is_obs(item)  (rings[(item)->id].bonus_obs)

#define ring_type_obtainable(type) (rings[type].obtainable)

#endif
