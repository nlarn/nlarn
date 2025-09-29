/*
 * effects.h
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

#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdbool.h>
#include <glib.h>

#include "cJSON.h"
#include "enumFactory.h"

#define EFFECT_TYPE_ENUM(EFFECT_TYPE) \
    EFFECT_TYPE(ET_NONE,)              /* no short-term effect */ \
    /* POSITIVE EFFECTS */ \
    EFFECT_TYPE(ET_INC_CON,)           /* enhanced constitution */ \
    EFFECT_TYPE(ET_INC_DEX,)           /* enhanced dexterity */ \
    EFFECT_TYPE(ET_INC_INT,)           /* enhanced intelligence */ \
    EFFECT_TYPE(ET_INC_STR,)           /* enhanced strength */ \
    EFFECT_TYPE(ET_INC_WIS,)           /* enhanced wisdom */ \
    EFFECT_TYPE(ET_INC_RND,)           /* increase random ability */ \
    EFFECT_TYPE(ET_INC_DAMAGE,)        /* increased damage */ \
    EFFECT_TYPE(ET_INC_HP_MAX,)        /* increased HP max */ \
    EFFECT_TYPE(ET_INC_MP_MAX,)        /* increased MP max */ \
    EFFECT_TYPE(ET_INC_HP_REGEN,)      /* increased hp regeneration */ \
    EFFECT_TYPE(ET_INC_MP_REGEN,)      /* increased mp regeneration */ \
    EFFECT_TYPE(ET_INC_LEVEL,)         /* gain level */ \
    EFFECT_TYPE(ET_INC_EXP,)           /* gain experience */ \
    EFFECT_TYPE(ET_RESIST_FIRE,)       /* resist fire */ \
    EFFECT_TYPE(ET_RESIST_COLD,)       /* resist cold */ \
    EFFECT_TYPE(ET_RESIST_MAGIC,)      /* resist magic */ \
    EFFECT_TYPE(ET_PROTECTION,)        /* adds to AC  */ \
    EFFECT_TYPE(ET_STEALTH,)           /* stealth */ \
    EFFECT_TYPE(ET_AWARENESS,)         /* expanded awareness */ \
    EFFECT_TYPE(ET_SPEED,)             /* fast */ \
    EFFECT_TYPE(ET_HEROISM,)           /* heroism: big bonus on all base attributes */ \
    EFFECT_TYPE(ET_INVISIBILITY,)      /* invisible */ \
    EFFECT_TYPE(ET_INVULNERABILITY,)   /* invulnerability */ \
    EFFECT_TYPE(ET_INFRAVISION,)       /* see invisible monsters */ \
    EFFECT_TYPE(ET_ENLIGHTENMENT,)     /* enlightenment */ \
    EFFECT_TYPE(ET_REFLECTION,)        /* reflection */ \
    EFFECT_TYPE(ET_DETECT_MONSTER,)    /* sense presence of monsters */ \
    EFFECT_TYPE(ET_HOLD_MONSTER,)      /* monsters can't flee */ \
    EFFECT_TYPE(ET_SCARED,)            /* monsters turn to flee */ \
    EFFECT_TYPE(ET_CHARM_MONSTER,)     /* make monsters peaceful */ \
    EFFECT_TYPE(ET_INC_HP,)            /* heal */ \
    EFFECT_TYPE(ET_MAX_HP,)            /* full healing */ \
    EFFECT_TYPE(ET_INC_MP,)            /* restore mana */ \
    EFFECT_TYPE(ET_MAX_MP,)            /* full mana restore */ \
    EFFECT_TYPE(ET_CANCELLATION,)      /* cancels spheres */ \
    EFFECT_TYPE(ET_UNDEAD_PROTECTION,) /* protection against undead */ \
    EFFECT_TYPE(ET_SPIRIT_PROTECTION,) /* protection against spirits */ \
    EFFECT_TYPE(ET_LIFE_PROTECTION,)   /* you only live twice */ \
    EFFECT_TYPE(ET_NOTHEFT,)           /* protection from thievish monsters */ \
    EFFECT_TYPE(ET_SUSTAINMENT,)       /* protection from stat drain attacks */ \
    EFFECT_TYPE(ET_TIMESTOP,)          /* game time modification */ \
    EFFECT_TYPE(ET_WALL_WALK,)         /* ability to walk through walls */ \
    EFFECT_TYPE(ET_LEVITATION,)        /* affected person floats in the air */ \
    /* NEGATIVE EFFECTS */ \
    EFFECT_TYPE(ET_DEC_CON,)           /* reduced constitution */ \
    EFFECT_TYPE(ET_DEC_DEX,)           /* reduced dexterity */ \
    EFFECT_TYPE(ET_DEC_INT,)           /* reduced intelligence */ \
    EFFECT_TYPE(ET_DEC_STR,)           /* reduced strength */ \
    EFFECT_TYPE(ET_DEC_WIS,)           /* reduced wisdom */ \
    EFFECT_TYPE(ET_DEC_RND,)           /* reduce random attribute */ \
    EFFECT_TYPE(ET_AGGRAVATE_MONSTER,) /* aggravate monsters */ \
    EFFECT_TYPE(ET_SLEEP,)             /* no move for a certain amount of time */ \
    EFFECT_TYPE(ET_DIZZINESS,)         /* decrease all primary attributes */ \
    EFFECT_TYPE(ET_SICKNESS,)          /* decreased damage */ \
    EFFECT_TYPE(ET_BLINDNESS,)         /* remove the ability to see */ \
    EFFECT_TYPE(ET_CLUMSINESS,)        /* unable to wield weapon */ \
    EFFECT_TYPE(ET_ITCHING,)           /* unable to wear armour */ \
    EFFECT_TYPE(ET_CONFUSION,)         /* random movement */ \
    EFFECT_TYPE(ET_PARALYSIS,)         /* loss of ability to move */ \
    EFFECT_TYPE(ET_POISON,)            /* cause by potion or trap */ \
    EFFECT_TYPE(ET_AMNESIA,)           /* potion of forgetfulness */ \
    EFFECT_TYPE(ET_SLOWNESS,)          /* reduced speed */ \
    EFFECT_TYPE(ET_BURDENED,)          /* overloaded */ \
    EFFECT_TYPE(ET_OVERSTRAINED,)      /* extremely overloaded */ \
    EFFECT_TYPE(ET_TRAPPED,)           /* trapped in a pit */ \
    EFFECT_TYPE(ET_MAX,)               /* last effect known */ \

DECLARE_ENUM(effect_t, EFFECT_TYPE_ENUM)

typedef struct effect_data
{
    effect_t id;
    const char *name;        /* name of the effect's constant */
    guint duration;          /* duration of effect. 0 = permanent */
    int amount;              /* if modifier: amount of attribute modification */
    const char *desc;        /* description for status display and obituary */
    const char *msg_start;   /* message displayed when effect starts */
    const char *msg_stop;    /* message displayed when effect ends */
    const char *msg_start_monster; /* messages shown when the effect happens on a monster */
    const char *msg_stop_monster;
    bool                     /* effect flags */
        var_duration: 1,     /* the effect's duration is variable */
        var_amount: 1,       /* the effect's amount is variable */
        inc_duration: 1,     /* reset the duration of unique effects */
        inc_amount: 1;       /* extend the amount of unique effects */
} effect_data;

typedef struct effect
{
    gpointer oid;       /* effect's game object id */
    effect_t type;      /* type of effect */
    guint32 start;      /* game time the effect began */
    guint32 turns;      /* number of turns this effect remains */
    gint32 amount;      /* power of effect, if applicable */
    gpointer item;      /* oid of item which causes the effect (if caused by item) */
} effect;

struct game;

/* function declarations */

effect *effect_new(effect_t type);
effect *effect_copy(effect *e);
void effect_destroy(effect *e);

void effect_serialize(gpointer oid, effect *e, cJSON *root);
effect *effect_deserialize(cJSON *eser, struct game *g);
cJSON *effects_serialize(GPtrArray *effs);
GPtrArray *effects_deserialize(cJSON *eser);

const char *effect_type_name(effect_t type);
guint effect_type_duration(effect_t type);
int effect_type_amount(effect_t type);
gboolean effect_type_inc_duration(effect_t type);
gboolean effect_type_inc_amount(effect_t type);
const char *effect_get_desc(effect *e);
const char *effect_get_msg_start(effect *e);
const char *effect_get_msg_stop(effect *e);
const char *effect_get_msg_m_start(effect *e);
const char *effect_get_msg_m_stop(effect *e);

int effect_get_amount(effect *e);

effect *effect_add(GPtrArray *ea, effect *e);
int effect_del(GPtrArray *ea, effect *e);
effect *effect_get(GPtrArray *ea, effect_t type);

/* check if an effect is set */
int effect_query(GPtrArray *ea, effect_t type);

/**
 * Count down the number of turns remaining for an effect.
 *
 * @param e an effect
 * @return turns remaining. Expired effects return -1, permanent effects 0
 */
int effect_expire(effect *e);

#endif
