/*
 * amulets.h
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

#ifndef __AMULETS_H_
#define __AMULETS_H_

#include "items.h"

enum amulet_types {
	AM_NONE,
	AM_ENLIGHTENMENT,
	AM_DRAGON_SLAYING,
	AM_NEGATE_SPIRIT,
	AM_INVISIBILITY,
	AM_UNDEAD_CONTROL,
	AM_NOTHEFT,
	AM_POWER,
	AM_LARN,
	AM_MAX
};

typedef struct amulet_data {
	int id;
	char *name;
    int	effect_type; 	/* effect causes by this amulet */
	int price;          /* base price in the shops */
} amulet_data;

/* external vars */

extern const amulet_data amulets[AM_MAX];

/* function declarations */

void amulet_material_shuffle();
item_material_t amulet_material(int amulet_id);

/* macros */

#define amulet_name(item)        (amulets[(item)->id].name)
#define amulet_effect_type(item) (amulets[(item)->id].effect_type)
#define amulet_price(item)       (amulets[(item)->id].price)

#endif
