/*
 * monsters.h
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

#ifndef __MONSTERS_H_
#define __MONSTERS_H_

#include "defines.h"
#include "effects.h"
#include "game.h"
#include "items.h"
#include "position.h"
#include "traps.h"

/* forward declarations */

struct player;
struct level;

/* local monster definition for data storage */

#define MONSTER_ATTACK_COUNT 2

/* TODO: spell casting ability, resistances */
typedef struct monster_data
{
    int type;
    char *name;			/* monster's name */
    int level;
    int ac;
    int intelligence;	/* used to choose movement */
    int gold;
    int hp_max;
    int exp;			/* xp granted to player */
    char image;
    speed mspeed;
    esize msize;
    guint32 flags;
    attack attacks[MONSTER_ATTACK_COUNT];
} monster_data;

typedef enum monster_action_type
{
    MA_NONE,
    MA_FLEE,
    MA_REMAIN,
    MA_WANDER,
    MA_ATTACK,
    MA_MAX
} monster_action_t;

typedef enum monster_t
{
    MT_NONE,
    MT_GIANT_BAT,
    MT_GNOME,
    MT_HOBGOBLIN,
    MT_JACKAL,
    MT_KOBOLD,
    MT_ORC,
    MT_SNAKE,
    MT_CENTIPEDE,
    MT_JACULUS,
    MT_TROGLODYTE,
    MT_GIANT_ANT,
    MT_FLOATING_EYE,
    MT_LEPRECHAUN,
    MT_NYMPH,
    MT_QUASIT,
    MT_RUST_MONSTER,
    MT_ZOMBIE,
    MT_ASSASSIN_BUG,
    MT_BUGBEAR,
    MT_HELLHOUND,
    MT_ICE_LIZARD,
    MT_CENTAUR,
    MT_TROLL,
    MT_YETI,
    MT_ELF,
    MT_GELATINOUSCUBE,
    MT_METAMORPH,
    MT_VORTEX,
    MT_ZILLER,
    MT_VIOLET_FUNGUS,
    MT_WRAITH,
    MT_FORVALAKA,
    MT_LAMA_NOBE,
    MT_OSQUIP,
    MT_ROTHE,
    MT_XORN,
    MT_VAMPIRE,
    MT_STALKER,
    MT_POLTERGEIST,
    MT_DISENCHANTRESS,
    MT_SHAMBLINGMOUND,
    MT_YELLOW_MOLD,
    MT_UMBER_HULK,
    MT_GNOME_KING,
    MT_MIMIC,
    MT_WATER_LORD,
    MT_PURPLE_WORM,
    MT_XVART,
    MT_WHITE_DRAGON,
    MT_BRONCE_DRAGON,
    MT_GREEN_DRAGON,
    MT_SILVER_DRAGON,
    MT_PLATINUM_DRAGON,
    MT_RED_DRAGON,
    MT_SPIRIT_NAGA,
    MT_GREEN_URCHIN,
    MT_DEMONLORD_I,
    MT_DEMONLORD_II,
    MT_DEMONLORD_III,
    MT_DEMONLORD_IV,
    MT_DEMONLORD_V,
    MT_DEMONLORD_VI,
    MT_DEMONLORD_VII,
    MT_DAEMON_PRINCE,
    MT_MAX				/* maximum # monsters in the dungeon */
} monster_t;

enum monster_flags
{
    MF_NONE         = 0,
    MF_HEAD         = 1,        /* has a head */
    MF_NOBEHEAD     = 1 << 1,   /* cannot be beheaded */
    MF_HANDS        = 1 << 2,	/* has hands => can open doors */
    MF_FLY          = 1 << 3,   /* can fly (not affected by pits and trapdoors) */
    MF_SPIRIT       = 1 << 4,   /* is a spirit */
    MF_UNDEAD       = 1 << 5,   /* is undead */
    MF_INVISIBLE    = 1 << 6,   /* is invisible */
    MF_INFRAVISION  = 1 << 7,   /* can see invisible */
    MF_REGENERATE   = 1 << 8,   /* does regenerate */
};

/* monster  */
typedef struct monster
{
    monster_t type;
    gint32 hp;
    position pos;
    monster_action_t action;    /* current action */

    /* number of turns since when player was last seen; 0 = never */
    guint32 lastseen;

    /* last known position of player */
    position player_pos;

    /* level monster is on */
    struct level *level;

    /* attacks already unsuccessfully tried */
    gboolean attacks_failed[MONSTER_ATTACK_COUNT];

    inventory *inventory;
    GPtrArray *effects;

    guint32
        m_visible: 1,    /* LOS between player -> monster */
        p_visible: 1,    /* LOS between monster -> player */
        item_type: 8,    /* item type monster is displayed as */
        unknown: 1;      /* monster is unknown */
} monster;

/* external vars */

extern const monster_data monsters[MT_MAX];

/* function definitions */

monster *monster_new(int monster_type, struct level *l);
monster *monster_new_by_level(struct level *l);
void monster_destroy(monster *m);

void monster_move(monster *m, struct player *p);
int monster_position(monster *m, position target);
monster *monster_trap_trigger(monster *m, struct player *p);

void monster_items_drop(monster *m, inventory *floor);
void monster_items_pickup(monster *m, struct player *p);

int monster_attacks_count(monster *m);
void monster_player_attack(monster *m, struct player *p);
monster *monster_damage_take(monster *m, damage *dam);

gboolean monster_update_action(monster *m);
void monster_update_player_pos(monster *m, position ppos);
gboolean monster_regenerate(monster *m, time_t gtime, int difficulty, message_log *log);

void monsters_genocide(struct level *l);

#define monster_name(monster)        (monsters[(monster)->type].name)
#define monster_level(monster)       (monsters[(monster)->type].level)
#define monster_ac(monster)          (monsters[(monster)->type].ac)
#define monster_int(monster)         (monsters[(monster)->type].intelligence)
#define monster_gold(monster)        (monsters[(monster)->type].gold)
#define monster_hp_max(monster)      (monsters[(monster)->type].hp_max)
#define monster_exp(monster)         (monsters[(monster)->type].exp)
#define monster_image(monster)       (monsters[(monster)->type].image)
#define monster_speed(monster)       (monsters[(monster)->type].mspeed)
#define monster_damage(monster)      (monsters[(monster)->type].attacks[0].base)

/* flags */
#define monster_has_head(monster)        (monsters[(monster)->type].flags & MF_HEAD)
#define monster_is_beheadable(monster)   (!(monsters[(monster)->type].flags & MF_NOBEHEAD))
#define monster_has_hands(monster)       (monsters[(monster)->type].flags & MF_HANDS)
#define monster_can_fly(monster)         (monsters[(monster)->type].flags & MF_FLY)
#define monster_is_spirit(monster)       (monsters[(monster)->type].flags & MF_SPIRIT)
#define monster_is_undead(monster)       (monsters[(monster)->type].flags & MF_UNDEAD)
#define monster_is_invisible(monster)    (monsters[(monster)->type].flags & MF_INVISIBLE)
#define monster_has_infravision(monster) (monsters[(monster)->type].flags & MF_INFRAVISION)
#define monster_can_regenerate(monster)  (monsters[(monster)->type].flags & MF_REGENERATE)

#define monster_name_by_type(type)  (monsters[(type)].name)
#define monster_image_by_type(type) (monsters[(type)].image)

int monster_genocide(int monster_id);
int monster_is_genocided(int monster_id);

/* dealing with temporary effects */
#define monster_effect_add(monster, effect) effect_add((monster)->effects, (effect))
#define monster_effect_del(monster, effect) effect_del((monster)->effects, (effect))
#define monster_effect_get(monster, effect_type) effect_get((monster)->effects, (effect_type))
#define monster_effect(monster, effect_type) effect_query((monster)->effects, (effect_type))
void monster_effect_expire(monster *m, message_log *log);

#endif
