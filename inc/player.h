/*
 * player.h
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

#include "amulets.h"
#include "armour.h"
#include "buildings.h"
#include "map.h"
#include "monsters.h"
#include "position.h"
#include "potions.h"
#include "rings.h"
#include "scrolls.h"
#include "spells.h"
#include "utils.h"
#include "weapons.h"

/* forward declaration */
struct game;

typedef struct _player_stats
{
    gint32 deepest_level;
    gint32 monsters_killed[MT_MAX];
    gint32 spells_cast;
    gint32 potions_quaffed;
    gint32 scrolls_read;
    gint32 books_read;
    gint32 life_protected;
    gint32 vandalism;

    // Keep track of the dungeon economy.
    gint32 items_bought;
    gint32 items_sold;
    gint32 gems_sold;
    gint32 gold_found;
    gint32 gold_sold_items;
    gint32 gold_sold_gems;
    gint32 gold_bank_interest;
    gint32 gold_spent_shop;
    gint32 gold_spent_id_repair;
    gint32 gold_spent_donation;
    gint32 gold_spent_college;

    gint32 max_level;
    gint32 max_xp;
    gint32 str_orig;
    gint32 int_orig;
    gint32 wis_orig;
    gint32 con_orig;
    gint32 dex_orig;
} player_stats;

typedef enum _player_sex
{
    PS_NONE,
    PS_MALE,
    PS_FEMALE
} player_sex;

typedef struct _player_settings
{
    gboolean auto_pickup[IT_MAX]; /* automatically pick up item of enabled types */
} player_settings;

typedef struct _player_tile_memory
{
    map_tile_t type;
    map_sobject_t sobject;
    item_t item;            /* type of item located here */
    int item_colour;        /* colour of item located here */
    trap_t trap;
} player_tile_memory;

typedef struct _player_sobject_memory
{
    position pos;
    map_sobject_t sobject;
} player_sobject_memory;

typedef struct player
{
    char *name;
    player_sex sex;

    guint32 strength;
    guint32 intelligence;
    guint32 wisdom;
    guint32 constitution;
    guint32 dexterity;

    gint32 hp; /* current hp */
    guint32 hp_max; /* max hp */
    gint32 mp;
    guint32 mp_max;
    guint32 regen_counter; /* regeneration counter */

    guint32 bank_account; /* There is nothing quite as wonderful as money */
    guint32 outstanding_taxes;
    guint32 interest_lasttime; /* last time interest has been calculated */

    guint32 experience; /* experience points */
    guint32 level; /* current experience level */

    speed speed; /* player's speed */
    guint32 movement; /* player's movement points */

    /* other stuff */
    GPtrArray *known_spells;
    inventory *inventory;
    GPtrArray *effects; /* temporary effects from potions, spells, ... */

    /* pointers to elements of items which are currently equipped */
    item *eq_amulet;
    item *eq_weapon;

    /* armour */
    item *eq_boots;
    item *eq_cloak;
    item *eq_gloves;
    item *eq_helmet;
    item *eq_shield;
    item *eq_suit;

    /* rings */
    item *eq_ring_l;
    item *eq_ring_r;
    /* enough items for now */

    /* items identified */
    gint identified_amulets[AM_MAX];
    gint identified_books[SP_MAX];
    gint identified_potions[PO_MAX];
    gint identified_rings[RT_MAX];
    gint identified_scrolls[ST_MAX];

    position pos; /* player's position */
    player_stats stats; /* statistics */

    /* player's field of vision */
    int fov[MAP_MAX_Y][MAP_MAX_X];

    /* player's memory of the map */
    player_tile_memory memory[MAP_MAX][MAP_MAX_Y][MAP_MAX_X];

    /* remembered positions of stationary objects */
    GArray *sobjmem;

    /* flag to store if the player has been attacked during the current turn */
    gboolean attacked;

    /* courses available in school */
    gint school_courses_taken[SCHOOL_COURSE_COUNT];

    player_settings settings; /* game configuration */
} player;

/* various causes of death */
typedef enum _player_cod
{
    PD_NONE,
    PD_EFFECT,
    PD_LASTLEVEL, /* lost a level at level 1 */
    PD_MONSTER,
    PD_SPHERE,
    PD_TRAP,
    PD_MAP,       /* damaged by map effects */
    PD_SPELL,     /* damaged by own spell */
    PD_CURSE,     /* damaged by a cursed item */
    PD_SOBJECT,   /* killed by stationary object */
    /* *** causes above this line can be stopped by live protection *** */
    PD_STUCK,     /* stuck in a wall */
    PD_DROWNED,   /* drowned in deep water */
    PD_MELTED,    /* stuck in lava */
    PD_GENOCIDE,  /* genocided themselves */
    /* *** caused below this line are described as "returning home" *** */
    PD_TOO_LATE,  /* returned with potion too late */
    PD_WON,       /* won the game */
    PD_LOST,      /* daughter has died, potion not found */
    PD_QUIT,      /* quit the game */
    PD_MAX
} player_cod;

/* function declarations */

player *player_new();
int player_assign_bonus_stats(player *p);
void player_destroy(player *p);

cJSON *player_serialize(player *p);
player *player_deserialize(cJSON *pser);

/**
 * @brief consume time for an action by the player
 *
 * @param the player
 * @param the number of turns the move takes
 * @param TRUE if the action can be interrupted
 * @param (optional) the description of the action in the form "reading the book of foo"
 * @return TRUE if the action has completed, FALSE if it has been interrupted
 *
 */
gboolean player_make_move(player *p, int turns, gboolean interruptible, const char *desc, ...);

/**
 * Kill the player
 *
 * @param the player
 * @param the cause, e.g. PD_TRAP
 * @param the id of the specific cause, e.g. TT_DART
 *
 */
void player_die(player *p, player_cod cause_type, int cause);

gint64 player_calc_score(player *p, int won);
gboolean player_movement_possible(player *p);
int player_move(player *p, direction dir, gboolean open_door);
int player_attack(player *p, monster *m);
void player_update_fov(player *p);
int player_pos_visible(player *p, position pos);

/**
 * Function to enter a map.
 *
 * @param the player
 * @param entered map
 * @param has to be TRUE if the player didn't enter the map regularly
 * @return TRUE
 */
int player_map_enter(player *p, map *l, gboolean teleported);

/**
 * Choose a random armour the player is wearing.
 *
 * @param   the player
 * @return  a pointer to the slot of the armour
 *
 */
item **player_get_random_armour(player *p);

void player_pickup(player *p);
void player_autopickup(player *p);
void player_autopickup_show(player *p);

void player_level_gain(player *p, int count);
void player_level_lose(player *p, int count);

void player_exp_gain(player *p, int count);
void player_exp_lose(player *p, int count);

int player_hp_gain(player *p, int count);

/**
 * Inflict damage upon the player
 *
 * @param the player
 * @param pointer to the damage (will be freed)
 * @param of the damage originator
 * @param the id of the damage originator, specific to the damage originator
 */
void player_damage_take(player *p, damage *dam, player_cod cause_type, int cause);

int player_hp_max_gain(player *p, int count);
int player_hp_max_lose(player *p, int count);

int player_mp_gain(player *p, int count);
int player_mp_lose(player *p, int count);
int player_mp_max_gain(player *p, int count);
int player_mp_max_lose(player *p, int count);

/* dealing with temporary effects */
effect *player_effect_add(player *p, effect *e);
void player_effects_add(player *p, GPtrArray *effects);
int player_effect_del(player *p, effect *e);
void player_effects_del(player *p, GPtrArray *effects);
effect *player_effect_get(player *p, effect_type et);
int player_effect(player *p, effect_type et); /* check if a effect is set */
char **player_effect_text(player *p);

/* dealing with the inventory */
int player_inv_display(player *p);
char *player_can_carry(player *p);
char *player_inv_weight(player *p);

/**
 * Callback function used from inv_add() for the player's inventory. Here it
 * is determined it an item can be picked up or if the weight of the player's
 * pack would be larger than the player could carry afterwards.
 *
 * @param the inventory to check
 * @param the item which is about to be added
 *
 */
int player_inv_pre_add(inventory *inv, item *item);

/**
 * Callback function used from inv_add() for the player's inventory. Here the
 * weight of the inventory gets calculated and the burdened or overstrained
 * mode is set or removed.
 *
 * @param the inventory to check
 * @param the item which is about to be added
 *
 */
void player_inv_weight_recalc(inventory *inv, item *item);


/* dealing with items */

/**
  * @brief Function used to equip an item.
  *
  * @param the player
  * @param unused, needed to make function signature match display_inventory requirements
  * @param the item
  */
void player_item_equip(player *p, inventory **inv, item *it);

/**
  * @brief Function used to unequip an item.
  *
  * @param the player
  * @param unused, needed to make function signature match display_inventory requirements
  * @param the item
  */
void player_item_unequip_wrapper(player *p, inventory **inv, item *it);
void player_item_unequip(player *p, inventory **inv, item *it, int forced);

int player_item_is_container(player *p, item *it);
int player_item_can_be_added_to_container(player *p, item *it);

/**
 * Callback funtion used to check if an item is equipped.
 *
 * @param the player
 * @param the item
 * @return place where item is equipped
 */
int player_item_is_equipped(player *p, item *it);

int player_item_is_equippable(player *p, item *it);
int player_item_is_usable(player *p, item *it);
int player_item_is_dropable(player *p, item *it);
int player_item_is_damaged(player *p, item *it);
int player_item_is_affordable(player *p, item *it);
int player_item_is_sellable(player *p, item *it);
int player_item_is_identifiable(player *p, item *it);
int player_item_known(player *p, item *it);
int player_item_identified(player *p, item *it);
char *player_item_identified_list(player *p);
void player_item_identify(player *p, inventory **inv, item *it);
void player_item_use(player *p, inventory **inv, item *it);
void player_item_destroy(player *p, item *it);
void player_item_drop(player *p, inventory **inv, item *it);

/**
 * @brief Take notes about an item.
 */
void player_item_notes(player *p, inventory **inv, item *it);

/**
 * @brief Try to pick up an item.
 *
 * @param the player
 * @param the source inventory
 * @param the item to pick up
 * @return the number of turns used for the action:
          0 if the action has been canceled,
 *        1 when the pre_add callback function failed (e.g. the item is too heavy),
 *        2 if adding the item to the player's inventory succeeded.
 */
void player_item_pickup(player *p, inventory **inv, item *it);

/* item usage shortcuts */
void player_read(player *p);
void player_quaff(player *p);
void player_equip(player *p);
void player_take_off(player *p);
void player_drop(player *p);

/* query values */
int player_get_ac(player *p);
int player_get_wc(player *p, monster *m);
int player_get_hp_max(player *p);
int player_get_mp_max(player *p);
int player_get_str(player *p);
int player_get_int(player *p);
int player_get_wis(player *p);
int player_get_con(player *p);
int player_get_dex(player *p);
int player_get_cha(player *p);
int player_get_speed(player *p);

/* deal with money */
guint player_get_gold(player *p);
guint player_set_gold(player *p, guint amount);

char *player_get_level_desc(player *p);
void player_list_sobjmem(player *p);
void player_sobject_forget(player *p, position pos);
/* fighting simulation */
void calc_fighting_stats(player *p);

/* macros */

#define player_memory_of(p,pos) ((p)->memory[(pos).z][(pos).y][(pos).x])

#endif
