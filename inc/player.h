/*
 * player.h
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

#ifndef __PLAYER_H_
#define __PLAYER_H_

#include "armour.h"
#include "level.h"
#include "potions.h"
#include "rings.h"
#include "scrolls.h"
#include "spells.h"
#include "utils.h"
#include "weapons.h"

/* number of courses available in school */
#define SCHOOL_COURSE_COUNT 8

typedef struct player_stats {
    int moves_made;
    int deepest_level;
    int monsters_killed;
    int spells_cast;
    int potions_quaffed;
    int scrolls_read;
    int books_read;
    int gold_collected;
    int gold_spent;
    int times_prayed;
    int max_level;
    int max_xp;
} player_stats;

struct _player_tile_memory {
    level_tile_t type;
    level_stationary_t stationary;
    /* type of item located here */
    item_t item;
    trap_t trap;
};

typedef struct _player_tile_memory player_tile_memory;

typedef struct player {
    char *name;
    int sex; /* 0 female, 1 male */

    int strength;
    int intelligence;
    int wisdom;
    int constitution;
    int dexterity;
    int charisma;

    int hp; /* current hp */
    int hp_max; /* max hp */
    int mp;
    int mp_max;
    int regen_counter; /* regeneration counter */

    int fire_resistance;
    int cold_resistance;
    int magic_resistance;

    int bank_account; /* There is nothing quite as wonderful as money */
    int outstanding_taxes;
    int interest_lasttime; /* last time interest has been calculated */

    int experience; /* experience points */
    int lvl; /* current experience level */

    /* other stuff */
    GPtrArray *known_spells;
    inventory *inventory;
    GPtrArray *effects; /* temporary effects from potions, spells, ... */

    /* pointers to elements of items which are currently equipped */
    weapon *eq_weapon;

    /* armour types */
    armour *eq_boots;
    armour *eq_cloak;
    armour *eq_gloves;
    armour *eq_helmet;
    armour *eq_shield;
    armour *eq_suit;

    /* rings */
    ring *eq_ring_l;
    ring *eq_ring_r;
    /* enough items for now */

    /* items identified */
    int identified_books[SP_MAX];
    int identified_potions[PO_MAX];
    int identified_rings[RT_MAX];
    int identified_scrolls[ST_MAX];

    position pos; /* player's position */
    level *level; /* current dungeon level */
    void *game; /* pointer to game the player is in */
    player_stats stats; /* statistics */
    message_log *log; /* game message log */

    /* player's field of vision */
    int fov[LEVEL_MAX_Y][LEVEL_MAX_X];

    /* player's memory of the map */
    player_tile_memory memory[LEVEL_MAX][LEVEL_MAX_Y][LEVEL_MAX_X];

    /* courses available in school */
    int school_courses_taken[SCHOOL_COURSE_COUNT];

    /* automatically pick up items */
    int auto_pickup;
} player;

typedef enum player_equipment_t {
    PE_NONE,
    PE_BOOTS,
    PE_CLOAK,
    PE_GLOVES,
    PE_HELMET,
    PE_RING_L,
    PE_RING_R,
    PE_SHIELD,
    PE_SUIT,
    PE_WEAPON,
    PE_MAX
} player_equipment_t;

/* various causes of death */
typedef enum player_cod {
    PD_NONE,
    PD_EFFECT,
    PD_LASTLEVEL, /* lost a level at level 1 */
    PD_MONSTER,
    PD_SPHERE,
    PD_TRAP,
    PD_SPELL, /* damaged by own spell */
    PD_TOO_LATE, /* returned with potion too late */
    PD_WON, /* won the game */
    PD_LOST, /* daughter has died, potion not found */
    PD_QUIT, /* quit the game */
    PD_MAX
} player_cod;

/* function declarations */

player *player_new(void *game);
void player_destroy(player *p);

int player_regenerate(player *p);
void player_die(player *p, player_cod cause_type, int cause);
int player_move(player *p, int direction);
int player_attack(player *p, monster *m);
int player_position(player *p, position target);
void player_update_fov(player *p, int radius);
int player_level_enter(player *p, level *l);
int player_level_leave(player *p);
int player_teleport(player *p);

int player_examine(player *p, position pos);
int player_pickup(player *p);

void player_lvl_gain(player *p, int count);
void player_lvl_lose(player *p, int count);

void player_exp_gain(player *p, int count);
void player_exp_lose(player *p, int count);

int player_hp_gain(player *p, int count);
void player_hp_lose(player *p, int count, player_cod cause_type, int cause);
int player_hp_max_gain(player *p, int count);
int player_hp_max_lose(player *p, int count);

int player_mp_gain(player *p, int count);
int player_mp_lose(player *p, int count);
int player_mp_max_gain(player *p, int count);
int player_mp_max_lose(player *p, int count);

int player_spell_cast(player *p);
int player_spell_learn(player *p, int spell_type);
int player_spell_forget(player *p, int spell_type);
int player_spell_known(player *p, int spell_type);

/* dealing with temporary effects */
void player_effect_add(player *p, effect *e);
int player_effect_del(player *p, effect *e);
effect *player_effect_get(player *p, int effect_id);
int player_effect(player *p, int effect_type); /* check if a effect is set */
void player_effects_expire(player *p, int turns);

/* dealing with the inventory */
int player_inv_display(player *p);

/* dealing with items */
int player_item_equip(player *p, item *it);
int player_item_unequip(player *p, item *it);
int player_item_is_equipped(player *p, item *it);
int player_item_is_equippable(player *p, item *it);
inline int player_item_is_usable(player *p, item *it);
inline int player_item_is_dropable(player *p, item *it);
int player_item_identified(player *p, item *it);
void player_item_identify(player *p, item *it);
int player_item_use(player *p, item *it);
int player_item_drop(player *p, item *it);
int player_item_pickup(player *p, item *it);

/* query values */
int player_get_ac(player *p);
int player_get_wc(player *p);
int player_get_hp_max(player *p);
int player_get_mp_max(player *p);
int player_get_str(player *p);
int player_get_int(player *p);
int player_get_wis(player *p);
int player_get_con(player *p);
int player_get_dex(player *p);
int player_get_cha(player *p);

/* deal with money */
int player_get_gold(player *p);
int player_set_gold(player *p, int amount);

inline const char *player_get_lvl_desc(player *p);

/* macros */

#define player_memory_of(p,pos) ((p)->memory[(p)->level->nlevel][(pos).y][(pos).x])
#define player_pos_visible(p,pos) ((p)->fov[(pos).y][(pos).x])

#endif
