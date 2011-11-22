/*
 * effects.h
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

#ifndef __EFFECTS_H_
#define __EFFECTS_H_

#include <glib.h>
#include <time.h>

#include "cJSON.h"

typedef enum _effect_t
{
    ET_NONE,                    /* no short-term effect */
    /* POSITIVE EFFECTS */
    /* base attribute modification */
    ET_INC_CON,                 /* enhanced constitution */
    ET_INC_DEX,                 /* enhanced dexterity */
    ET_INC_INT,                 /* enhanced intelligence */
    ET_INC_STR,                 /* enhanced strength */
    ET_INC_WIS,                 /* enhanced wisdom */
    ET_INC_RND,                 /* increase random ability */

    /* secondary attributes modification */
    ET_INC_DAMAGE,              /* increased damage */
    ET_INC_HP_MAX,              /* increased HP max */
    ET_INC_MP_MAX,              /* increased MP max */
    ET_INC_HP_REGEN,            /* increased hp regeneration */
    ET_INC_MP_REGEN,            /* increased mp regeneration */
    ET_INC_LEVEL,               /* gain level */
    ET_INC_EXP,                 /* gain experience */
    ET_RESIST_FIRE,             /* resist fire */
    ET_RESIST_COLD,             /* resist cold */
    ET_RESIST_MAGIC,            /* resist magic */
    ET_PROTECTION,              /* adds to AC  */

    /* ability improvements */
    ET_STEALTH,                 /* stealth */
    ET_AWARENESS,               /* expanded awareness */
    ET_SPEED,                   /* fast */
    ET_HEROISM,                 /* heroism: big bonus on all base attributes */
    ET_INVISIBILITY,            /* invisible */
    ET_INVULNERABILITY,         /* invulnerability */
    ET_INFRAVISION,             /* see invisible monsters */
    ET_ENLIGHTENMENT,           /* enlightenment */
    ET_REFLECTION,              /* reflection */

    ET_DETECT_MONSTER,          /* sense presence of monsters */
    ET_HOLD_MONSTER,            /* monsters can't flee */
    ET_SCARED,                  /* monsters turn to flee */
    ET_CHARM_MONSTER,           /* make monsters peaceful */

    /* healing */
    ET_INC_HP,                  /* heal */
    ET_MAX_HP,                  /* full healing */
    ET_INC_MP,                  /* restore mana */
    ET_MAX_MP,                  /* full mana restore */

    ET_CANCELLATION,            /* cancels spheres */
    ET_UNDEAD_PROTECTION,       /* protection against undead */
    ET_SPIRIT_PROTECTION,       /* protection against spirits */
    ET_LIFE_PROTECTION,         /* you only live twice */
    ET_NOTHEFT,                 /* protection from thievish monsters */
    ET_TIMESTOP,                /* game time modification */
    ET_WALL_WALK,               /* ability to walk through walls */
    ET_LEVITATION,              /* affected person floats in the air */

    /* NEGATIVE EFFECTS */
    /* base attribute modification */
    ET_DEC_CON,                 /* reduced constitution */
    ET_DEC_DEX,                 /* reduced dexterity */
    ET_DEC_INT,                 /* reduced intelligence */
    ET_DEC_STR,                 /* reduced strength */
    ET_DEC_WIS,                 /* reduced wisdom */
    ET_DEC_RND,                 /* reduce random attribute */

    /* secondary attributes modification */
    ET_DEC_HP_MAX,              /* decreased HP max */
    ET_DEC_MP_MAX,              /* decreased MP max */
    ET_DEC_HP_REGEN,            /* decreased hp regeneration */
    ET_DEC_MP_REGEN,            /* decreased mp regeneration */
    ET_DEC_LEVEL,               /* lose level */
    ET_DEC_EXP,                 /* lose experience */

    ET_AGGRAVATE_MONSTER,       /* aggravate monsters */
    ET_SLEEP,                   /* no move for a certain amount of time */
    ET_DIZZINESS,               /* decrease all primary attributes */
    ET_SICKNESS,                /* decreased damage */
    ET_BLINDNESS,               /* remove the ability to see */
    ET_CLUMSINESS,              /* unable to wield weapon */
    ET_ITCHING,                 /* unable to wear armour */
    ET_CONFUSION,               /* random movement */
    ET_PARALYSIS,               /* loss of ability to move */
    ET_POISON,                  /* cause by potion or trap */
    ET_AMNESIA,                 /* potion of forgetfulness */
    ET_SLOWNESS,                /* reduced speed */
    ET_BURDENED,                /* overloaded */
    ET_OVERSTRAINED,            /* extremely overloaded */
    ET_TRAPPED,                 /* trapped in a pit */

    ET_MAX                      /* last effect known */
} effect_t;

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
    guint                    /* effect flags */
        var_duration: 1,     /* the effect's duration is variable */
        var_amount: 1,       /* the effect's amount is variable */
        inc_duration: 1,     /* reset the duration of unique effects */
        inc_amount: 1;       /* extend the amount of unique effects */
} effect_data;

typedef struct effect
{
    gpointer oid;       /* effect's game object id */
    effect_t type;   /* type of effect */
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
 * @param an effect
 * @return turns remaining. Expired effects return -1, permantent effects 0
 */
int effect_expire(effect *e);

#endif
