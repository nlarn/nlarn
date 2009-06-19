/*
 * spells.h
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

#ifndef __SPELLS_H_
#define __SPELLS_H_

#include "effects.h"

typedef enum spell_type
{
    SC_NONE,
    SC_PLAYER, /* affects the player */
    SC_POINT,  /* affects a single monster */
    SC_RAY,    /* creates a ray */
    SC_SQUARE, /* effect occurs in a square */
    SC_CIRCLE, /* effect occurs in a circle */
    SC_OTHER,  /* unclassified */
    SC_MAX
} spell_t;

typedef struct spell_data {
	int id;
	char *code;
	char *name;
	spell_t type;
	int effect_type;
	char *description;
	char *msg_success;
	char *msg_fail;
	int level;
	int price;
} spell_data;

typedef struct spell {
	int id;         /* reference to spell_data */
	int learnt;     /* game time learnt */
	int knowledge;  /* quality of knowledge */
	int used;       /* usage counter */
} spell;

enum spell_ids {
	SP_NONE,
	SP_PRO,		/* protection */
	SP_MLE,		/* magic missile */
	SP_DEX,		/* dexterity */
	SP_SLE,		/* sleep */
	SP_CHM,		/* charm monster */
	SP_SSP,		/* sonic spear */
	SP_STR,		/* strength */
	SP_ENL,		/* enlightnement */
	SP_HEL,		/* healing */
	SP_CBL,		/* cure blindness */
	SP_CRE,		/* create monster */
	SP_PHA,		/* phantasmal forces */
	SP_INV,		/* invisibility */
	SP_BAL,		/* fireball */
	SP_CLD,		/* cold */
	SP_PLY,		/* polymorph */
	SP_CAN,		/* cancellation */
	SP_HAS,		/* haste self */
	SP_CKL,		/* cloud kill */
	SP_VPR,		/* vaporize rock */
	SP_DRY,		/* dehydration */
	SP_LIT,		/* lightning */
	SP_DRL,		/* drain life */
	SP_GLO,		/* globe of invulnerability */
	SP_FLO,		/* flood */
	SP_FGR,		/* finger of death */
	SP_SCA,		/* scare monster */
	SP_HLD,		/* hold monster */
	SP_STP,		/* time stop */
	SP_TEL,		/* teleport */
	SP_MFI,		/* magic fire */
	SP_MKW,		/* make wall */
	SP_SPH,		/* sphere of annihilation */
	SP_GEN,		/* genocide monster */
	SP_SUM,		/* summon daemon */
	SP_WTW,		/* walk through walls */
	SP_ALT,		/* alter reality */
	SP_PER,		/* permanence */
	SP_MAX		/* last known spell */
};

typedef struct book {
	int type;       /* id of spell described within */
	unsigned blessedness: 2;
} book;

/* external vars */

extern const spell_data spells[SP_MAX];

/* function definitions */

spell *spell_new(int id);
void spell_destroy(spell *s);
int spell_sort(gconstpointer a, gconstpointer b);

#define spell_code(spell)     (spells[(spell)->id].code)
#define spell_name(spell)     (spells[(spell)->id].name)
#define spell_type(spell)     (spells[(spell)->id].type)
#define spell_effect(spell)   (spells[(spell)->id].effect_type)
#define spell_desc(spell)     (spells[(spell)->id].description)
#define spell_msg_succ(spell) (spells[(spell)->id].msg_success)
#define spell_msg_fail(spell) (spells[(spell)->id].msg_fail)
#define spell_level(spell)    (spells[(spell)->id].level)

#define spell_code_by_id(id)     (spells[(id)].code)
#define spell_name_by_id(id)     (spells[(id)].name)
#define spell_type_by_id(id)     (spells[(id)].type)
#define spell_effect_by_id(id)   (spells[(id)].effect_type)
#define spell_desc_by_id(id)     (spells[(id)].description)
#define spell_msg_succ_by_id(id) (spells[(id)].msg_success)
#define spell_msg_fail_by_id(id) (spells[(id)].msg_fail)
#define spell_level_by_id(id)    (spells[(id)].level)

/* *** BOOKS *** */

void book_desc_shuffle();
book *book_new(int spell_id);
book *book_new_by_level(int level);
void book_destroy(book *b);

char *book_get_desc(book *b);

#define book_get_name(book)   (spells[(book)->type].name)
#define book_get_weight(book) (1) /* FIXME: return real weight */
#define book_get_price(book)  (spells[(book)->type].price)

#endif
