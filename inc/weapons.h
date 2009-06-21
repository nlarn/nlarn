/*
 * weapons.h
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

#ifndef __WEAPONS_H_
#define __WEAPONS_H_

typedef struct weapon_data {
	int	id;
	char *name;
	int wc;				/* weapon class */
	int	material;		/* material type from item_materials */
	int weight;			/* used to determine inventory weight and if item can be thrown */
	int price;			/* base price in the shops */
	unsigned
		twohanded: 1,	/* needs two hands */
		unique: 1;		/* unique */
} weapon_data;

enum weapon_type {
	WT_NONE,
	WT_DAGGER,
	WT_SPEAR,
	WT_FLAIL,
	WT_BATTLEAXE,
	WT_LONGSWORD,
	WT_2SWORD,
	WT_SWORDSLASHING,
	WT_LANCEOFDEATH,
	WT_VORPALBLADE,
	WT_SLAYER,
	WT_SUNSWORD,
	WT_BESSMAN,
	WT_MAX
};

/* function definitions */

/* external vars */

extern const weapon_data weapons[WT_MAX];

/* macros */

#define weapon_name(weapon)         (weapons[(weapon)->id].name)
#define weapon_wc(weapon)           (weapons[(weapon)->id].wc + (weapon)->bonus)
#define weapon_material(weapon)     (weapons[(weapon)->id].material)
#define weapon_weight(weapon)       (weapons[(weapon)->id].weight)
#define weapon_price(weapon)        (weapons[(weapon)->id].price)
#define weapon_is_twohanded(weapon) (weapons[(weapon)->id].twohanded)
#define weapon_is_unique(weapon)    (weapons[(weapon)->id].unique)

#endif
