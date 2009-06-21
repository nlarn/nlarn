/*
 * effects.h
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

#ifndef __EFFECTS_H_
#define __EFFECTS_H_

enum effect_types
{
    ET_NONE,                    /* no short-term effect */
    /* POSITIVE EFFECTS */
    /* base attribute modification */
    ET_INC_CHA,                 /* enhanced charisma */
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
    ET_FIRE_RESISTANCE,         /* resist fire */
    ET_COLD_RESISTANCE,         /* resist cold */
    ET_MAGIC_RESISTANCE,        /* resist magic */
    ET_PROTECTION,              /* adds to AC  */

    /* ability improvements */
    ET_STEALTH,                 /* aggravate monsters */
    ET_AWARENESS,               /* expanded awareness */
    ET_SPEED,                   /* fast */
    ET_HEROISM,                 /* heroism: big bonus on all base attributes */
    ET_INVISIBILITY,            /* invisible */
    ET_INVULNERABILITY,         /* invulnerability */
    ET_INFRAVISION,             /* see invisible monsters */
    ET_ENLIGHTENMENT,           /* enlightenment */

    ET_DETECT_MONSTER,          /* sense presence of monsters */
    ET_HOLD_MONSTER,            /* monsters can't flee */
    ET_SCARE_MONSTER,           /* monsters turn to flee */
    ET_CHARM_MONSTER,           /* make monsters peaceful */
    ET_AGGRAVATE_MONSTER,       /* aggravate monsters */
    ET_HASTE_MONSTER,           /* speed up monsters */

    /* healing */
    ET_INC_HP,                  /* heal */
    ET_MAX_HP,                  /* full healing */
    ET_INC_MP,                  /* restore mana */
    ET_MAX_MP,                  /* full mana restore */

    ET_CANCELLATION,            /* cancels spheres */
    ET_DIVINE_PROTECTION,       /* adds to AC, repels undead, caused by praying */
    ET_UNDEAD_PROTECTION,       /* protection against undead */
    ET_SPIRIT_PROTECTION,       /* protection against spirits */

    ET_TIMESTOP,				/* game time modification */
    ET_WALL_WALK,               /* ability to walk through walls */
    ET_LIFE_PROTECTION,         /* you only live twice */
    ET_NOTHEFT,                 /* protection from thievish monsters */

    /* NEGATIVE EFFECTS */
    /* base attribute modification */
    ET_DEC_CHA,                 /* reduced charisma */
    ET_DEC_CON,                 /* reduced constitution */
    ET_DEC_DEX,                 /* reduced dexterity */
    ET_DEC_INT,                 /* reduced intelligence */
    ET_DEC_STR,                 /* reduced strength */
    ET_DEC_WIS,                 /* reduced wisdom */
    ET_DEC_RND,                 /* reduce random attribute */

    /* secondary attributes modification */
    ET_DEC_DAMAGE,              /* decreased damage */
    ET_DEC_HP_MAX,              /* decreased HP max */
    ET_DEC_MP_MAX,              /* decreased MP max */
    ET_DEC_HP_REGEN,            /* decreased hp regeneration */
    ET_DEC_MP_REGEN,            /* decreased mp regeneration */
    ET_DEC_LEVEL,               /* lose level */
    ET_DEC_EXP,                 /* lose experience */

    ET_SLEEP,                   /* no move for a certain amount of time */
    ET_DIZZINESS,               /* decrease all primary attributes */
    ET_BLINDNESS,               /* remove the ability to see */
    ET_CLUMSINESS,              /* unable to wield weapon */
    ET_ITCHING,                 /* unable to wear armour */
    ET_CONFUSION,               /* random movement */
    ET_LAUGHTER,                /* FIXME: ??? */
    ET_POISON,                  /* by potion */
    ET_AMNESIA,                 /* potion of forgetfulness */
    ET_SLOWNESS,                /* reduce speed */

    ET_MAX                      /* last effect known */
};

typedef struct effect_data
{
    int	id;
    int	duration;           /* duration of effect. 0 = permanent */
    int value;              /* if modifier: amount of attribute modification */
    char *msg_start;        /* message displayed when effect starts */
    char *msg_stop;         /* message displayed when effect ends */
    /* TODO: perhaps add this field: char *msg_fail; */
    char *msg_start_monster; /*	If the effect happens on a monster */
    char *msg_stop_monster;
} effect_data;

/* forward declaration to avoid circular dependencies */
struct item;

typedef struct effect
{
    int	type;               /* type of effect */
    time_t start;           /* game time the effect began */
    int	turns;              /* number of turns this effect remains */
    int amount;             /* power of effect, if applicable */
    struct item *item;      /* item which causes the effect (if caused by item) */
} effect;


/* function declarations */

effect *effect_new(int type, time_t now);
void effect_destroy(effect *e);

char *effect_get_msg_start(effect *e);
char *effect_get_msg_stop(effect *e);
char *effect_get_msg_m_start(effect *e);
char *effect_get_msg_m_stop(effect *e);

int effect_get_amount(effect *e);

void effect_add(GPtrArray *a, effect *e);
int effect_del(GPtrArray *a, effect *e);
effect *effect_get(GPtrArray *a, int effect_type);

/* check if an effect is set */
int effect_query(GPtrArray *a, int effect_type);

int effect_expire(effect *ae, int turns);

#endif
