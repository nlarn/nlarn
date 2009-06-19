/*
 * armour.h
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

#ifndef __ARMOUR_H_
#define __ARMOUR_H_

#include "items.h"

typedef enum armour_categories {
	AC_NONE,
	AC_BOOTS,
	AC_CLOAK,
	AC_GLOVES,
	AC_HELMET,
	AC_SHIELD,
	AC_SUIT,
	AC_MAX
} armour_cat;

enum armour_types {
	AT_NONE,
	AT_SHIELD,
	AT_LEATHER,
	AT_SLEATHER,
	AT_RIGNMAIL,
	AT_CHAINMAIL,
	AT_SPLINTMAIL,
	AT_PLATEMAIL,
	AT_PLATE,
	AT_SPLATE,
	AT_ELVENCHAIN,
	AT_MAX
};

typedef struct armour_data {
	int id;
	char *name;
	int ac;
    armour_cat category;
	int	material;       /* material type from item_materials */
	int weight;         /* used to determine inventory weight and if item can be thrown */
	int price;          /* base price in the shops */
	unsigned
		unique: 		1;	/* unique */
} armour_data;

typedef struct armour {
	int type;
	int ac_bonus;
	unsigned
		blessedness: 	2,	/* 0: cursed; 1: uncursed; 2: blessed */
		corroded:		2,	/* 0: no; 1: yes; 2: very */
		burnt:			2,	/* 0: no; 1: yes; 2: very */
		rusty:			2;	/* 0: no; 1: yes; 2: very */
} armour;

/* function definitions */

armour *armour_new(int armour_id, int bonus);
void armour_destroy(armour *a);

inline char *armour_get_name(armour *a);
inline int armour_get_ac(armour *a);
inline armour_cat armour_get_category(armour *a);
inline int armour_get_material(armour *a);
inline int armour_get_weight(armour *a);
inline int armour_get_price(armour *a);
inline gboolean armour_is_uniqe(armour *a);

int armour_bless(armour *a);
int armour_curse(armour *a);

int armour_enchant(armour *a);
int armour_disenchant(armour *a);

int armour_rust(armour *a);
int armour_corrode(armour *a);
int armour_burn(armour *a);

#endif
