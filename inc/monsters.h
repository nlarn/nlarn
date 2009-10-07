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

#include <glib.h>
#include <time.h>

#include "defines.h"
#include "effects.h"
#include "items.h"
#include "position.h"
#include "traps.h"
#include "utils.h"

/* forward declarations */

struct _monster;
typedef struct _monster monster;

struct player;
struct map;

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
    MF_METALLIVORE  = 1 << 9,   /* eats metal */
};

/* external vars */

extern const monster_data monsters[MT_MAX];

/* function definitions */

monster *monster_new(int type);
monster *monster_new_by_level(int nlevel);
void monster_destroy(monster *m);

/* getter / setter */

int monster_hp(monster *m);
void monster_inc_hp(monster *m, int amount);

item_t monster_item_type(monster *m);

position monster_pos(monster *m);
int monster_set_pos(monster *m, struct map *map, position target);

monster_t monster_type(monster *m);

gboolean monster_unknown(monster *m);
void monster_set_unknown(monster *m, gboolean what);

gboolean monster_in_sight(monster *m);

/* other functions */

void monster_level_enter(monster *m, struct map *l);
void monster_move(monster *m, struct player *p);

monster *monster_trap_trigger(monster *m, struct player *p);
void monster_polymorph(monster *m);

void monster_items_drop(monster *m, inventory **floor);
void monster_items_pickup(monster *m, struct player *p);

int monster_attack_count(monster *m);
void monster_player_attack(monster *m, struct player *p);
monster *monster_damage_take(monster *m, damage *dam);

gboolean monster_update_action(monster *m);
void monster_update_player_pos(monster *m, position ppos);
gboolean monster_regenerate(monster *m, time_t gtime, int difficulty, message_log *log);

void monsters_genocide(struct map *l);

#define monster_name(monster)        (monsters[monster_type(monster)].name)
#define monster_level(monster)       (monsters[monster_type(monster)].level)
#define monster_ac(monster)          (monsters[monster_type(monster)].ac)
#define monster_int(monster)         (monsters[monster_type(monster)].intelligence)
#define monster_gold(monster)        (monsters[monster_type(monster)].gold)
#define monster_hp_max(monster)      (monsters[monster_type(monster)].hp_max)
#define monster_exp(monster)         (monsters[monster_type(monster)].exp)
#define monster_image(monster)       (monsters[monster_type(monster)].image)
#define monster_speed(monster)       (monsters[monster_type(monster)].mspeed)
#define monster_attack(monster, idx) (monsters[monster_type(monster)].attacks[(idx)])

/* flags */
#define monster_has_head(monster)        (monsters[monster_type(monster)].flags & MF_HEAD)
#define monster_is_beheadable(monster)   (!(monsters[monster_type(monster)].flags & MF_NOBEHEAD))
#define monster_has_hands(monster)       (monsters[monster_type(monster)].flags & MF_HANDS)
#define monster_can_fly(monster)         (monsters[monster_type(monster)].flags & MF_FLY)
#define monster_is_spirit(monster)       (monsters[monster_type(monster)].flags & MF_SPIRIT)
#define monster_is_undead(monster)       (monsters[monster_type(monster)].flags & MF_UNDEAD)
#define monster_is_invisible(monster)    (monsters[monster_type(monster)].flags & MF_INVISIBLE)
#define monster_has_infravision(monster) (monsters[monster_type(monster)].flags & MF_INFRAVISION)
#define monster_can_regenerate(monster)  (monsters[monster_type(monster)].flags & MF_REGENERATE)
#define monster_is_metallivore(monster)  (monsters[monster_type(monster)].flags & MF_METALLIVORE)

#define monster_name_by_type(type)  (monsters[(type)].name)
#define monster_image_by_type(type) (monsters[(type)].image)

int monster_genocide(int monster_id);
int monster_is_genocided(int monster_id);

/* dealing with temporary effects */
void monster_effect_add(monster *m, effect *e);
int monster_effect_del(monster *m, effect *e);
effect *monster_effect_get(monster *m , effect_type type);
int monster_effect(monster *m, effect_type type);
void monster_effect_expire(monster *m, message_log *log);

#endif
