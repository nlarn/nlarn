/*
 * monsters.h
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

/* $Id$ */

#ifndef __MONSTERS_H_
#define __MONSTERS_H_

#include <glib.h>
#include <lua.h>
#include <time.h>

#include "cJSON.h"
#include "defines.h"
#include "effects.h"
#include "items.h"
#include "lua_wrappers.h"
#include "position.h"
#include "utils.h"

/* forward declarations */

struct _monster;
typedef struct _monster monster;

struct game;
struct player;
struct map;

/* local monster definition for data storage */

typedef enum monster_t
{
    MT_NONE,            //  0
    // D1 - D4
    MT_GIANT_BAT,
    MT_GNOME,
    MT_HOBGOBLIN,
    MT_JACKAL,
    // D2 - D4
    MT_KOBOLD,          //  5
    MT_ORC,
    MT_SNAKE,
    MT_CENTIPEDE,
    MT_JACULUS,
    MT_TROGLODYTE,      // 10
    // D3 - D5
    MT_GIANT_ANT,
    MT_FLOATING_EYE,
    MT_LEPRECHAUN,
    MT_NYMPH,
    MT_QUASIT,          // 15
    MT_RUST_MONSTER,
    // D4 - D6
    MT_ZOMBIE,
    MT_ASSASSIN_BUG,
    MT_BUGBEAR,
    MT_HELLHOUND,       // 20
    MT_ICE_LIZARD,
    // D5 - D7
    MT_CENTAUR,
    MT_TROLL,
    MT_YETI,
    MT_ELF,             // 25
    MT_GELATINOUSCUBE,
    // D6 - D8
    MT_WHITE_DRAGON,
    MT_METAMORPH,
    MT_VORTEX,
    MT_ZILLER,          // 30
    MT_VIOLET_FUNGUS,
    MT_WRAITH,
    // D7 - D9
    MT_FORVALAKA,
    MT_LAMA_NOBE,
    MT_OSQUIP,          // 35
    MT_ROTHE,
    MT_XORN,
    MT_VAMPIRE,
    // D8 - D10
    MT_STALKER,
    MT_POLTERGEIST,     // 40
    MT_DISENCHANTRESS,
    // D9 - V1
    MT_SHAMBLINGMOUND,
    MT_YELLOW_MOLD,
    MT_UMBER_HULK,
    MT_GNOME_KING,      // 45
    // D10 - V2
    MT_MIMIC,
    MT_WATER_LORD,
    MT_PURPLE_WORM,
    MT_XVART,
    // V1 - V3
    MT_BRONZE_DRAGON,   // 50
    MT_GREEN_DRAGON,
    MT_SILVER_DRAGON,
    // V2 - V3
    MT_PLATINUM_DRAGON,
    MT_RED_DRAGON,
    MT_SPIRIT_NAGA,     // 55
    // V3
    MT_GREEN_URCHIN,
    MT_DEMONLORD_I,
    MT_DEMONLORD_II,
    MT_DEMONLORD_III,
    MT_DEMONLORD_IV,    // 60
    MT_DEMONLORD_V,
    MT_DEMONLORD_VI,
    MT_DEMONLORD_VII,
    // not actually generated randomly
    MT_DEMON_PRINCE,
    MT_MAX_GENERATED,
    MT_TOWN_PERSON = MT_MAX_GENERATED,
    MT_MAX				/* maximum # monsters in the dungeon */
} monster_t;

typedef enum monster_action_type
{
    MA_NONE,
    MA_FLEE,
    MA_REMAIN,
    MA_WANDER,
    MA_ATTACK,
    MA_CONFUSION,
    MA_SERVE,
    MA_CIVILIAN,
    MA_MAX
} monster_action_t;

typedef enum monster_flags
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
    MF_DEMON        = 1 << 10,  /* is a demon */
    MF_DRAGON       = 1 << 11,  /* is a dragon */
    MF_MIMIC        = 1 << 12,  /* is a mimic */
    MF_RES_FIRE     = 1 << 13,  /* resistant to fire (half damage)*/
    MF_RES_SLEEP    = 1 << 14,  /* resistant to sleep */
    MF_RES_POISON   = 1 << 15,  /* resistant to poison */
    MF_RES_ELEC     = 1 << 16,  /* resistant to electricity */
    MF_RES_CONF     = 1 << 17,  /* resistant to confusion */
    MF_SWIM         = 1 << 18,  /* can swim through water */
} monster_flag;

/* function definitions */

monster *monster_new(monster_t type, position pos);
monster *monster_new_by_level(position pos);
void monster_destroy(monster *m);

void monster_serialize(gpointer oid, monster *m, cJSON *root);
void monster_deserialize(cJSON *mser, struct game *g);

/* getters / setters */

int monster_hp_max(monster *m);
int monster_hp(monster *m);
void monster_hp_inc(monster *m, int amount);
gpointer monster_oid(monster *m);
position monster_pos(monster *m);
int monster_pos_set(monster *m, struct map *mp, position target);
int monster_valid_dest(struct map *m, position pos, int map_elem);
monster_t monster_type(monster *m);
gboolean monster_unknown(monster *m);
void monster_unknown_set(monster *m, gboolean what);

/**
 * @brief Return the monster's inventory.
 *
 * @param A monster.
 * @return The given monster's inventory.
 */
inventory **monster_inv(monster *m);

gboolean monster_in_sight(monster *m);

/** @brief Get the currently set AI action for a given monster.
  *
  * @param A monster.
  * @return The currently set AI action for the given monster.
  */
monster_action_t monster_action(monster *m);

/* other functions */
const char *monster_get_name(monster *m);
const char* monster_type_plural_name(const int montype, const int count);
void monster_die(monster *m, struct player *p);

void monster_level_enter(monster *m, struct map *l);
void monster_move(gpointer *oid, monster *m, struct game *g);

void monster_polymorph(monster *m);

/**
 * check stash at monster's position for something desired
 *
 * @param a monster
 * @return TRUE if something has been picked up, FALSE if not
 */
int monster_items_pickup(monster *m);

/**
 * Returns the number of attack type a monster can choose from
 *
 * @param a monster
 * @return the number of attacks
 */
int monster_attack_count(monster *m);

/**
 * Returns the chosen attack type for the monster
 *
 * @param a monster
 * @param the number of an attack
 * @return an attack
 */
attack monster_attack(monster *m, int num);

void monster_player_attack(monster *m, struct player *p);
int monster_player_ranged_attack(monster *m, struct player *p);

/**
 * Deal damage to a monster
 *
 * @param monster
 * @param pointer to the damage to be dealt (will be free'd)
 * @return the monster if it has survived, othewise NULL
 */
monster *monster_damage_take(monster *m, damage *dam);

/**
 * Determine a monster's action.
 *
 * @param the monster
 * @param manually set action for a monster
 * @return TRUE if the action has changed
 */
gboolean monster_update_action(monster *m, monster_action_t override);

void monster_update_player_pos(monster *m, position ppos);
gboolean monster_regenerate(monster *m, time_t gtime, int difficulty);

item *get_mimic_item(monster *m);
char *monster_desc(monster *m);
char monster_glyph(monster *m);
int monster_color(monster *m);

/* dealing with temporary effects */
effect *monster_effect_add(monster *m, effect *e);
int monster_effect_del(monster *m, effect *e);
effect *monster_effect_get(monster *m , effect_t type);
int monster_effect(monster *m, effect_t type);
void monster_effects_expire(monster *m);
int monster_is_carrying_item(monster *m, item_t type);

/* query monster data */

static inline const char *monster_name(monster *m) {
    return luaN_query_string("monsters", monster_type(m), "name");
}

static inline int monster_level(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "level");
}

static inline int monster_ac(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "ac");
}

static inline guint monster_int(monster *m)
{
    return (guint)luaN_query_int("monsters", monster_type(m), "intelligence")
            + monster_effect(m, ET_HEROISM)
            - monster_effect(m, ET_DIZZINESS);
}

static inline int monster_gold_chance(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "gold_chance");
}

static inline int monster_gold_amount(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "gold");
}

static inline int monster_exp(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "exp");
}

static inline int monster_size(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "size");
}

static inline int monster_speed(monster *m)
{
    return luaN_query_int("monsters", monster_type(m), "speed")
            + monster_effect(m, ET_SPEED)
            + (monster_effect(m, ET_HEROISM) * 5)
            - monster_effect(m, ET_SLOWNESS)
            - (monster_effect(m, ET_DIZZINESS) * 5);
}

static inline int monster_flags(monster *m, monster_flag f)
{
    return (luaN_query_int("monsters", monster_type(m), "flags") & f);
}

#define monster_map(M)      game_map(nlarn, Z(monster_pos(M)))
/* query monster type data */

static inline int monster_type_hp_max(monster_t type)
{
    return luaN_query_int("monsters", type, "hp_max");
}

static inline char monster_type_image(monster_t type)
{
    return luaN_query_char("monsters", type, "glyph");
}

static inline const char *monster_type_name(monster_t type)
{
    return luaN_query_string("monsters", type, "name");
}

static inline int monster_type_reroll_chance(monster_t type)
{
    return luaN_query_int("monsters", type, "reroll_chance");
}

void monster_genocide(monster_t monster_id);
int monster_is_genocided(monster_t monster_id);

#endif
