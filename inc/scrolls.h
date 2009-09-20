/*
 * scrolls.h
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

#ifndef __SCROLLS_H_
#define __SCROLLS_H_

#include "items.h"

/* forward declaration */

struct player;

/* type definitions */

typedef struct magic_scroll_data {
	int	id;
	char *name;
	effect_type effect; /* if this scroll causes an effect */
	int price;
} magic_scroll_data;

enum scroll_types {
	ST_NONE,
	ST_ENCH_ARMOUR,
	ST_ENCH_WEAPON,
	ST_ENLIGHTENMENT,
	ST_BLANK,
	ST_CREATE_MONSTER,
	ST_CREATE_ARTIFACT,
	ST_AGGRAVATE_MONSTER,
	ST_TIMEWARP,
	ST_TELEPORT,
	ST_AWARENESS,
	ST_SPEED,
	ST_HEAL_MONSTER,
	ST_SPIRIT_PROTECTION,
	ST_UNDEAD_PROTECTION,
	ST_STEALTH,
	ST_MAPPING,
	ST_HOLD_MONSTER,
	ST_GEM_PERFECTION,
	ST_SPELL_EXTENSION,
	ST_IDENTIFY,
	ST_REMOVE_CURSE,
	ST_ANNIHILATION,
	ST_PULVERIZATION,
	ST_LIFE_PROTECTION,
	ST_MAX
};

/* function declarations */

void scroll_desc_shuffle();
char *scroll_desc(int scroll_id);

int scroll_with_effect(struct player *p, item *scroll);
int scroll_annihilate(struct player *p, item *scroll);
int scroll_create_artefact(struct player *p, item *scroll);
int scroll_enchant_armour(struct player *p, item *scroll);
int scroll_enchant_weapon(struct player *p, item *scroll);
int scroll_gem_perfection(struct player *p, item *scroll);
int scroll_heal_monster(struct player *p, item *scroll);
int scroll_identify(struct player *p, item *scroll);
int scroll_mapping(struct player *p, item *scroll);
int scroll_remove_curse(struct player *p, item *scroll);
int scroll_spell_extension(struct player *p, item *scroll);
int scroll_teleport(struct player *p, item *scroll);
int scroll_timewarp(struct player *p, item *scroll);
/* external vars */

extern const magic_scroll_data scrolls[ST_MAX];

/* macros */

#define scroll_name(scroll)   (scrolls[(scroll)->id].name)
#define scroll_effect(scroll) (scrolls[(scroll)->id].effect)
#define scroll_price(scroll)  (scrolls[(scroll)->id].price)

#endif
