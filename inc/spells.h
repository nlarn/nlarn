/*
 * spells.h
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

#ifndef SPELLS_H
#define SPELLS_H

#include "colours.h"
#include "effects.h"
#include "enumFactory.h"
#include "items.h"

typedef enum spell_type
{
    SC_PLAYER, /* affects the player */
    SC_POINT,  /* affects a single monster */
    SC_RAY,    /* creates a ray */
    SC_FLOOD,  /* effect pours like water */
    SC_BLAST,  /* effect occurs like an explosion */
    SC_OTHER,  /* unclassified */
    SC_MAX
} spell_t;

#define SPELL_TYPE_ENUM(SPELL_TYPE) \
    SPELL_TYPE(SP_PRO,) /* protection */ \
    SPELL_TYPE(SP_MLE,) /* magic missile */ \
    SPELL_TYPE(SP_DEX,) /* dexterity */ \
    SPELL_TYPE(SP_SLE,) /* sleep */ \
    SPELL_TYPE(SP_CHM,) /* charm monster */ \
    SPELL_TYPE(SP_SSP,) /* sonic spear */ \
    SPELL_TYPE(SP_STR,) /* strength */ \
    SPELL_TYPE(SP_CPO,) /* cure poison */ \
    SPELL_TYPE(SP_HEL,) /* healing */ \
    SPELL_TYPE(SP_CBL,) /* cure blindness */ \
    SPELL_TYPE(SP_CRE,) /* create monster */ \
    SPELL_TYPE(SP_PHA,) /* phantasmal forces */ \
    SPELL_TYPE(SP_INV,) /* invisibility */ \
    SPELL_TYPE(SP_BAL,) /* fireball */ \
    SPELL_TYPE(SP_CLD,) /* cold */ \
    SPELL_TYPE(SP_PLY,) /* polymorph */ \
    SPELL_TYPE(SP_CAN,) /* cancellation */ \
    SPELL_TYPE(SP_HAS,) /* haste self */ \
    SPELL_TYPE(SP_CKL,) /* cloud kill */ \
    SPELL_TYPE(SP_VPR,) /* vaporize rock */ \
    SPELL_TYPE(SP_DRY,) /* dehydration */ \
    SPELL_TYPE(SP_LIT,) /* lightning */ \
    SPELL_TYPE(SP_DRL,) /* drain life */ \
    SPELL_TYPE(SP_GLO,) /* globe of invulnerability */ \
    SPELL_TYPE(SP_FLO,) /* flood */ \
    SPELL_TYPE(SP_FGR,) /* finger of death */ \
    SPELL_TYPE(SP_SCA,) /* scare monster */ \
    SPELL_TYPE(SP_HLD,) /* hold monster */ \
    SPELL_TYPE(SP_STP,) /* time stop */ \
    SPELL_TYPE(SP_TEL,) /* teleport */ \
    SPELL_TYPE(SP_MFI,) /* magic fire */ \
    SPELL_TYPE(SP_MKW,) /* make wall */ \
    SPELL_TYPE(SP_SPH,) /* sphere of annihilation */ \
    SPELL_TYPE(SP_SUM,) /* summon demon */ \
    SPELL_TYPE(SP_WTW,) /* walk through walls */ \
    SPELL_TYPE(SP_ALT,) /* alter reality */ \
    SPELL_TYPE(SP_MAX,) /* last known spell */

DECLARE_ENUM(spell_id, SPELL_TYPE_ENUM)

/* forward declarations */
struct spell;
struct player;

/* a function that implements a spell */
typedef int (*spell_func)(struct spell *, struct player *);

typedef struct spell_data {
    spell_id id;
    const char *code;
    const char *name;
    spell_t type;
    damage_t damage_type;
    effect_t effect;          /* the effect cause by this spell */
    spell_func function;      /* the function implementing this spell */
    const char *description;  /* the spell's description */
    const char *msg_success;  /* the message given upon success */
    const char *msg_fail;     /* the message give upon failure */
    colour_t fg;              /* the colour of visible spells */
    int level;                /* level of the spell */
    int price;                /* price of the book*/
    bool
        obtainable: 1;  /* available in the shop */
} spell_data;

typedef struct spell {
    spell_id id;      /* reference to spell_data */
    guint knowledge;  /* quality of knowledge */
    guint used;       /* usage counter */
} spell;

/* external vars */

extern const spell_data spells[SP_MAX];

/* function definitions */

spell *spell_new(spell_id id);
void spell_destroy(spell *s);

cJSON *spell_serialize(spell *s);
spell *spell_deserialize(cJSON *sser);
cJSON *spells_serialize(GPtrArray *sparr);
GPtrArray *spells_deserialize(cJSON *sser);

int spell_sort(gconstpointer a, gconstpointer b);

/**
 * Select a spell to cast and cast it
 *
 * @param p the player
 * @return number of turns elapsed
 */
int spell_cast_new(struct player *p);

/**
 * Cast the previous spell again
 *
 * @param p the player
 * @return number of turns elapsed
 */
int spell_cast_previous(struct player *p);

/**
 * Try to add a spell to the list of known spells
 *
 * @param p the player
 * @param spell_type ID of spell to learn
 * @return false if learning the spell failed, otherwise level of knowledge
 */
int spell_learn(struct player *p, spell_id spell_type);

/**
 * Check if a spell is known to the player
 *
 * @param p the player
 * @param spell_type ID of the spell in question
 * @return false if unknown, otherwise level of knowledge of that spell
 */
int spell_known(struct player *p, spell_id spell_type);

/**
 * Returns a textual description of a spell
 *
 * @param sid the spell ID
 * @return a string describing the spell type (must be released with g_free()))
 */
gchar* spell_desc_by_id(spell_id sid);

gboolean spell_create_monster(spell *s, struct player *p);
gboolean spell_vaporize_rock(spell *s, struct player *p);

#define spell_code(spell)     (spells[(spell)->id].code)
#define spell_name(spell)     (spells[(spell)->id].name)
#define spell_type(spell)     (spells[(spell)->id].type)
#define spell_damage(spell)   (spells[(spell)->id].damage_type)
#define spell_effect(spell)   (spells[(spell)->id].effect)
#define spell_msg_succ(spell) (spells[(spell)->id].msg_success)
#define spell_msg_fail(spell) (spells[(spell)->id].msg_fail)
#define spell_colour(spell)   (spells[(spell)->id].fg)
#define spell_level(spell)    (spells[(spell)->id].level)

#define spell_code_by_id(id)     (spells[(id)].code)
#define spell_name_by_id(id)     (spells[(id)].name)
#define spell_type_by_id(id)     (spells[(id)].type)
#define spell_damage_by_id(id)   (spells[(id)].damage_type)
#define spell_effect_by_id(id)   (spells[(id)].effect)
#define spell_msg_succ_by_id(id) (spells[(id)].msg_success)
#define spell_msg_fail_by_id(id) (spells[(id)].msg_fail)
#define spell_colour_by_id(id)   (spells[(id)].fg)
#define spell_level_by_id(id)    (spells[(id)].level)

/* *** BOOKS *** */

char *book_desc(spell_id book_id);
int book_weight(item *book);
colour_t book_colour(item *book);
item_usage_result book_read(struct player *p, item *book);

#define book_type_obtainable(id) (spells[id].obtainable)

#define book_name(book)   (spells[(book)->id].name)
#define book_price(book)  (spells[(book)->id].price)

#endif
