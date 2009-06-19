/*
 * potions.h
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

#ifndef __POTIONS_H_
#define __POTIONS_H_

typedef struct potion_data {
    int	id;
    char *name;
    int	effect_type; 		/* if this potion causes any effect */
    int price;
} potion_data;

typedef struct potion {
    int	type;
    /* currently nothing else */
} potion;

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
	PO_INC_CHA,
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
	PO_POISON,
	PO_SEE_INVISIBLE,
	PO_CURE_DIANTHR,
	PO_MAX
};

/* function declarations */

void potion_desc_shuffle();
potion *potion_new(int potion_id);
void potion_destroy(potion *p);

inline char *potion_get_name(potion *p);
inline char *potion_get_desc(potion *p);
inline int potion_get_effect(potion *p);
inline int potion_get_price(potion *p);

#endif
