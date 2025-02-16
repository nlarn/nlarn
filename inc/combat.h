/*
 * combat.h
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

#ifndef __COMBAT_H_
#define __COMBAT_H_

#include <glib.h>

#include "items.h"
#include "enumFactory.h"

#define SPEED_ENUM(SPEED) \
    SPEED(XSLOW,  =  25) \
    SPEED(VSLOW,  =  50) \
    SPEED(SLOW,   =  75) \
    SPEED(NORMAL, = 100) \
    SPEED(FAST,   = 125) \
    SPEED(VFAST,  = 150) \
    SPEED(XFAST,  = 175) \
    SPEED(DOUBLE, = 200) \

DECLARE_ENUM(speed, SPEED_ENUM)

#define SIZE_ENUM(SIZE) \
    SIZE(TINY,       =  25) \
    SIZE(SMALL,      =  75) \
    SIZE(MEDIUM,     = 100) \
    SIZE(LARGE,      = 125) \
    SIZE(GIANT,      = 150) \
    SIZE(GARGANTUAN, = 200) \

DECLARE_ENUM(size, SIZE_ENUM)

#define ATTACK_T_ENUM(ATT) \
    ATT(ATT_NONE,) \
    ATT(ATT_WEAPON,) \
    ATT(ATT_MAGIC,)  /* e.g. psionics */ \
    ATT(ATT_CLAW,)   /* some dragons */ \
    ATT(ATT_BITE,)   /* bugbear, osquip, snake */ \
    ATT(ATT_STING,)  /* ant, centipede */ \
    ATT(ATT_SLAM,)   /* shambling mound */ \
    ATT(ATT_KICK,)   /* centaur */ \
    ATT(ATT_TOUCH,)  /* vampire, wraith */ \
    ATT(ATT_BREATH,) /* dragons, hellhound */ \
    ATT(ATT_GAZE,)   /* floating eye */ \

DECLARE_ENUM(attack_t, ATTACK_T_ENUM)

#define DAMAGE_T_ENUM(DAM) \
    DAM(DAM_NONE,)           /* pass-through: just reduce HP */ \
    DAM(DAM_PHYSICAL,)       /* e.g. magic missile */ \
    DAM(DAM_MAGICAL,) \
    /* elements */ \
    DAM(DAM_FIRE,) \
    DAM(DAM_COLD,) \
    DAM(DAM_ACID,) \
    DAM(DAM_WATER,) \
    DAM(DAM_ELECTRICITY,) \
    /* effects */ \
    DAM(DAM_POISON,)         /* traps, snake */ \
    DAM(DAM_BLINDNESS,)      /* lama nobe, green urchin */ \
    DAM(DAM_CONFUSION,)      /* umber hulk */ \
    DAM(DAM_PARALYSIS,)      /* floating eye */ \
    DAM(DAM_DEC_CON,) \
    DAM(DAM_DEC_DEX,)        /* quasit */ \
    DAM(DAM_DEC_INT,) \
    DAM(DAM_DEC_STR,)        /* ant, centipede */ \
    DAM(DAM_DEC_WIS,) \
    DAM(DAM_DEC_RND,)        /* ziller */ \
    DAM(DAM_DRAIN_LIFE,)     /* vampire, wraith */ \
    /* inventory manipulation */ \
    /* these damage types are handled by the monster, not the player */ \
    DAM(DAM_STEAL_GOLD,)     /* leprechaun */ \
    DAM(DAM_STEAL_ITEM,)     /* nymph */ \
    DAM(DAM_RUST,)           /* rust monster, gelatious cube */ \
    DAM(DAM_REM_ENCH,)       /* remove enchantment from player's items */ \
    DAM(DAM_MAX,) \
    DAM(DAM_RANDOM,)         /* random damage: any of the types above */ \

DECLARE_ENUM(damage_t, DAMAGE_T_ENUM)

typedef struct _attack
{
    attack_t type;
    damage_t damage;
    int base;
    int rand;
} attack;

#define DAMAGE_ORIGINATOR_T_ENUM(DAMO) \
    DAMO(DAMO_NONE,) \
    DAMO(DAMO_ITEM,) \
    DAMO(DAMO_MAP,) \
    DAMO(DAMO_MONSTER,) \
    DAMO(DAMO_PLAYER,) \
    DAMO(DAMO_SOBJECT,) \
    DAMO(DAMO_SPHERE,) \
    DAMO(DAMO_TRAP,) \
    DAMO(DAMO_GOD,) \

DECLARE_ENUM(damage_originator_t, DAMAGE_ORIGINATOR_T_ENUM)

typedef struct _damage_originator
{
    damage_originator_t ot;
    gpointer originator;
} damage_originator;

typedef struct _damage
{
    damage_t type;
    attack_t attack;
    gint amount;
    damage_originator dam_origin; /* the source of the damage */
} damage;

typedef struct _damage_msg
{
    char *msg_affected;
    char *msg_unaffected;
} damage_msg;

damage *damage_new(damage_t type, attack_t att_type, int amount,
                   damage_originator_t damo, gpointer originator);

damage *damage_copy(damage *dam);

static inline void damage_free(damage *dam) { g_free(dam); }

char *damage_to_str(damage *dam);

/* forward declarations */
struct player;
struct _monster;

int combat_calc_to_hit(struct player *p, struct _monster *m, item *weapon, item *ammo);

#endif
