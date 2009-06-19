/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

typedef struct weapon {
	int	type;
	int wc_bonus;		/* improvement caused by enchant_weapon */
	unsigned
		blessedness: 	2,	/* 0: cursed; 1: uncursed; 2: blessed */
		corroded:		2,	/* 0: no; 1: yes; 2: very */
		burnt:			2,	/* 0: no; 1: yes; 2: very */
		rusty:			2;	/* 0: no; 1: yes; 2: very */
} weapon;

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

weapon *weapon_new(int weapon_type, int bonus);
void weapon_destroy(weapon *w);

inline char *weapon_get_name(weapon *w);
inline int weapon_get_wc(weapon *w);
inline int weapon_get_material(weapon *w);
inline int weapon_get_weight(weapon *w);
inline int weapon_get_price(weapon *w);
inline gboolean weapon_is_twohanded(weapon *w);
inline gboolean weapon_is_uniqe(weapon *w);

int weapon_bless(weapon *w);
int weapon_curse(weapon *w);

int weapon_enchant(weapon *w);
int weapon_disenchant(weapon *w);

int weapon_rust(weapon *w);
int weapon_corrode(weapon *w);
int weapon_burn(weapon *w);

int weapon_throw(weapon *w);

#endif
