/*
 * monsters.h
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

#ifndef MONSTERS_H
#define MONSTERS_H

#include <glib.h>
#include <time.h>

#include "cJSON.h"
#include "colours.h"
#include "effects.h"
#include "inventory.h"
#include "items.h"
#include "position.h"

/* forward declarations */

struct monster;
typedef struct monster monster;

struct game;
struct player;
struct map;

#define MONSTER_TYPE_ENUM(MT) \
    /* D1 - D4 */ \
    MT(MT_GIANT_BAT,) \
    MT(MT_GNOME,) \
    MT(MT_HOBGOBLIN,) \
    MT(MT_JACKAL,) \
    /* D2 - D4 */ \
    MT(MT_KOBOLD,)          /*  5 */ \
    MT(MT_ORC,) \
    MT(MT_SNAKE,) \
    MT(MT_CENTIPEDE,) \
    MT(MT_JACULUS,) \
    MT(MT_TROGLODYTE,)      /* 10 */ \
    /* D3 - D5 */ \
    MT(MT_GIANT_ANT,) \
    MT(MT_FLOATING_EYE,) \
    MT(MT_LEPRECHAUN,) \
    MT(MT_NYMPH,) \
    MT(MT_QUASIT,)          /* 15 */ \
    MT(MT_RUST_MONSTER,) \
    /* D4 - D6 */ \
    MT(MT_ZOMBIE,) \
    MT(MT_ASSASSIN_BUG,) \
    MT(MT_BUGBEAR,) \
    MT(MT_HELLHOUND,)       /* 20 */ \
    MT(MT_ICE_LIZARD,) \
    /* D5 - D7 */ \
    MT(MT_CENTAUR,) \
    MT(MT_TROLL,) \
    MT(MT_YETI,) \
    MT(MT_ELF,)             /* 25 */ \
    MT(MT_GELATINOUSCUBE,) \
    /* D6 - D8 */ \
    MT(MT_WHITE_DRAGON,) \
    MT(MT_METAMORPH,) \
    MT(MT_VORTEX,) \
    MT(MT_ZILLER,)          /* 30 */ \
    MT(MT_VIOLET_FUNGUS,) \
    MT(MT_WRAITH,) \
    /* D7 - D9 */ \
    MT(MT_FORVALAKA,) \
    MT(MT_LAMA_NOBE,) \
    MT(MT_OSQUIP,)          /* 35 */ \
    MT(MT_ROTHE,) \
    MT(MT_XORN,) \
    MT(MT_VAMPIRE,) \
    /* D8 - D10 */ \
    MT(MT_STALKER,) \
    MT(MT_POLTERGEIST,)     /* 40 */ \
    MT(MT_DISENCHANTRESS,) \
    /* D9 - V1 */ \
    MT(MT_SHAMBLINGMOUND,) \
    MT(MT_YELLOW_MOLD,) \
    MT(MT_UMBER_HULK,) \
    MT(MT_GNOME_KING,)      /* 45 */ \
    /* D10 - V2 */ \
    MT(MT_MIMIC,) \
    MT(MT_WATER_LORD,) \
    MT(MT_PURPLE_WORM,) \
    MT(MT_XVART,) \
    /* V1 - V3 */ \
    MT(MT_BRONZE_DRAGON,)   /* 50 */ \
    MT(MT_GREEN_DRAGON,) \
    MT(MT_SILVER_DRAGON,) \
    /* V2 - V3 */ \
    MT(MT_PLATINUM_DRAGON,) \
    MT(MT_RED_DRAGON,) \
    MT(MT_SPIRIT_NAGA,)     /* 55 */ \
    /* V3 */ \
    MT(MT_GREEN_URCHIN,) \
    MT(MT_DEMONLORD_I,) \
    MT(MT_DEMONLORD_II,) \
    MT(MT_DEMONLORD_III,) \
    MT(MT_DEMONLORD_IV,)    /* 60 */ \
    MT(MT_DEMONLORD_V,) \
    MT(MT_DEMONLORD_VI,) \
    MT(MT_DEMONLORD_VII,) \
    /* not actually generated randomly */ \
    MT(MT_DEMON_PRINCE,) \
    MT(MT_TOWN_PERSON,) \
    MT(MT_MAX,)                /* maximum # monsters in the dungeon */

DECLARE_ENUM(monster_t, MONSTER_TYPE_ENUM)

#define MONSTER_ACTION_TYPE_ENUM(MONSTER_ACTION_TYPE) \
    MONSTER_ACTION_TYPE(MA_NONE,) \
    MONSTER_ACTION_TYPE(MA_FLEE,) \
    MONSTER_ACTION_TYPE(MA_REMAIN,) \
    MONSTER_ACTION_TYPE(MA_WANDER,) \
    MONSTER_ACTION_TYPE(MA_ATTACK,) \
    MONSTER_ACTION_TYPE(MA_CONFUSION,) \
    MONSTER_ACTION_TYPE(MA_SERVE,) \
    MONSTER_ACTION_TYPE(MA_CIVILIAN,) \

DECLARE_ENUM(monster_action_t, MONSTER_ACTION_TYPE_ENUM)

#define MONSTER_FLAG_ENUM(MF) \
    MF(HEAD         , = 1)       /* has a head */ \
    MF(NOBEHEAD     , = 1 << 1)  /* cannot be beheaded */ \
    MF(HANDS        , = 1 << 2)  /* has hands => can open doors */ \
    MF(FLY          , = 1 << 3)  /* can fly (not affected by pits and trapdoors) */ \
    MF(SPIRIT       , = 1 << 4)  /* is a spirit */ \
    MF(UNDEAD       , = 1 << 5)  /* is undead */ \
    MF(INVISIBLE    , = 1 << 6)  /* is invisible */ \
    MF(INFRAVISION  , = 1 << 7)  /* can see invisible */ \
    MF(REGENERATE   , = 1 << 8)  /* does regenerate */ \
    MF(METALLIVORE  , = 1 << 9)  /* eats metal */ \
    MF(DEMON        , = 1 << 10) /* is a demon */ \
    MF(DRAGON       , = 1 << 11) /* is a dragon */ \
    MF(MIMIC        , = 1 << 12) /* is a mimic */ \
    MF(RES_FIRE     , = 1 << 13) /* resistant to fire (half damage)*/ \
    MF(RES_COLD     , = 1 << 14) /* resistant to cold */ \
    MF(RES_ELEC     , = 1 << 15) /* resistant to electricity */ \
    MF(RES_SLEEP    , = 1 << 16) /* resistant to sleep */ \
    MF(RES_POISON   , = 1 << 17) /* resistant to poison */ \
    MF(RES_CONF     , = 1 << 18) /* resistant to confusion */ \
    MF(RES_MAGIC    , = 1 << 19) /* resistant to magic */ \
    MF(SWIM         , = 1 << 20) /* can swim through water */ \
    MF(PACK         , = 1 << 21) /* creature appears in packs */ \

#define MONSTER_FLAG_COUNT 20

DECLARE_ENUM(monster_flag, MONSTER_FLAG_ENUM)

/* function definitions */

monster *monster_new(monster_t type, position pos, gpointer leader);
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
 * @param m A monster.
 * @return The given monster's inventory.
 */
inventory **monster_inv(monster *m);

gboolean monster_in_sight(monster *m);

/** @brief Get the currently set AI action for a given monster.
  *
  * @param m A monster.
  * @return The currently set AI action for the given monster.
  */
monster_action_t monster_action(monster *m);

/* other functions */
const char *monster_get_name(monster *m);
const char* monster_type_plural_name(monster_t mt, int count);
void monster_die(monster *m, struct player *p);

void monster_level_enter(monster *m, struct map *l);
void monster_move(gpointer *oid, monster *m, struct game *g);

void monster_polymorph(monster *m);

/**
 * check stash at monster's position for something desired
 *
 * @param m a monster
 * @return true if something has been picked up, false if not
 */
int monster_items_pickup(monster *m);

/**
 * Returns the number of attack type a monster can choose from
 *
 * @param m a monster
 * @return the number of attacks
 */
guint monster_attack_count(monster *m);

/**
 * Returns the chosen attack type for the monster
 *
 * @param m a monster
 * @param num the number of an attack
 * @return an attack
 */
attack monster_attack(monster *m, guint num);

void monster_player_attack(monster *m, struct player *p);
int monster_player_ranged_attack(monster *m, struct player *p);

/**
 * Deal damage to a monster
 *
 * @param m monster
 * @param dam pointer to the damage to be dealt (will be free'd)
 * @return the monster if it has survived, otherwise NULL
 */
monster *monster_damage_take(monster *m, damage *dam);

/**
 * Determine a monster's action.
 *
 * @param m the monster
 * @param override manually set action for a monster
 * @return true if the action has changed
 */
gboolean monster_update_action(monster *m, monster_action_t override);

void monster_update_player_pos(monster *m, position ppos);
gboolean monster_regenerate(monster *m, time_t gtime, int difficulty);

item *get_mimic_item(monster *m);
char *monster_desc(monster *m);
char monster_glyph(monster *m);
colour monster_color(monster *m);

/* dealing with temporary effects */
effect *monster_effect_add(monster *m, effect *e);
int monster_effect_del(monster *m, effect *e);
effect *monster_effect_get(monster *m , effect_t type);
int monster_effect(monster *m, effect_t type);
void monster_effects_expire(monster *m);
int monster_is_carrying_item(monster *m, item_t type);

/* query monster data */
const char *monster_name(monster *m);
int monster_level(monster *m);
int monster_ac(monster *m);
guint monster_int(monster *m);
int monster_gold_chance(monster *m);
int monster_gold_amount(monster *m);
int monster_exp(monster *m);
int monster_size(monster *m);
int monster_speed(monster *m);
int monster_flags(monster *m, monster_flag f);
const char *monster_sound(monster *m);

#define monster_map(M)      game_map(nlarn, Z(monster_pos(M)))

/* query monster type data */
const char *monster_type_name(monster_t type);
int monster_type_ac(monster_t type);
int monster_type_size(monster_t type);
int monster_type_speed(monster_t type);
int monster_type_flags(monster_t type, monster_flag f);
int monster_type_hp_max(monster_t type);
char monster_type_glyph(monster_t type);
int monster_type_reroll_chance(monster_t type);

void monster_genocide(monster_t monster_id);
int monster_is_genocided(monster_t monster_id);

#endif
