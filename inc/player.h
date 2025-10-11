/*
 * player.h
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

#ifndef PLAYER_H
#define PLAYER_H

#include "amulets.h"
#include "armour.h"
#include "buildings.h"
#include "colours.h"
#include "fov.h"
#include "map.h"
#include "monsters.h"
#include "position.h"
#include "potions.h"
#include "rings.h"
#include "scrolls.h"
#include "spells.h"

/* forward declaration */
struct game;

typedef struct player_stats
{
    guint deepest_level;
    gint  monsters_killed[MT_MAX];
    guint spells_cast;
    guint potions_quaffed;
    guint scrolls_read;
    guint books_read;
    guint weapons_wasted;
    guint life_protected;
    guint vandalism;

    // Keep track of the dungeon economy.
    guint items_bought;
    guint items_sold;
    guint gems_sold;
    guint gold_found;
    guint gold_sold_items;
    guint gold_sold_gems;
    guint gold_bank_interest;
    guint gold_spent_shop;
    guint gold_spent_id_repair;
    guint gold_spent_donation;
    guint gold_spent_college;
    guint gold_spent_taxes;

    guint max_level;
    guint max_xp;
    guint str_orig;
    guint int_orig;
    guint wis_orig;
    guint con_orig;
    guint dex_orig;
} player_stats;

typedef enum player_sex
{
    PS_NONE,
    PS_MALE,
    PS_FEMALE,
    PS_MAX
} player_sex;

typedef struct player_settings
{
    gboolean auto_pickup[IT_MAX]; /* automatically pick up item of enabled types */
} player_settings;

typedef struct player_tile_memory
{
    map_tile_t type;
    sobject_t sobject;
    item_t item;            /* type of item located here */
    colour item_colour;     /* colour of item located here */
    trap_t trap;
} player_tile_memory;

typedef struct player_sobject_memory
{
    position pos;
    sobject_t sobject;
} player_sobject_memory;

typedef struct player
{
    char *name;
    player_sex sex;

    guint strength;
    guint intelligence;
    guint wisdom;
    guint constitution;
    guint dexterity;

    gint  hp; /* current hp */
    guint hp_max; /* max hp */
    gint  mp; /* magic points */
    guint mp_max; /* max mp */
    guint regen_counter; /* regeneration counter */

    guint bank_account; /* There is nothing quite as wonderful as money */
    guint bank_ieslvtb; /* The interest earned since last visiting the bank */
    guint outstanding_taxes;

    guint experience; /* experience points */
    guint level; /* current experience level */

    speed speed; /* player's speed */
    guint movement; /* player's movement points */

    /* other stuff */
    GPtrArray *known_spells;
    inventory *inventory;
    GPtrArray *effects; /* temporary effects from potions, spells, ... */

    /* pointers to elements of items which are currently equipped */
    item *eq_amulet;
    item *eq_weapon;
    item *eq_sweapon;
    item *eq_quiver;

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
    amulet_t identified_amulets[AM_MAX];
    armour_t identified_armour[AT_MAX];
    spell_t  identified_books[SP_MAX];
    potion_t identified_potions[PO_MAX];
    ring_t   identified_rings[RT_MAX];
    scroll_t identified_scrolls[ST_MAX];

    position pos; /* player's position */
    player_stats stats; /* statistics */

    /* oid of the last targeted monster */
    gpointer *ptarget;

    /* player's field of vision */
    fov *fv;

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
typedef enum player_cod
{
    PD_NONE,      /* 0, required by setjmp for initialisation */
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

/* an array with textual descriptions of the player stac configurations */
extern const char *player_bonus_stat_desc[];
char player_select_bonus_stats();
/**
 * @brief Assigns player's stats to the given preset
 *
 * @param p the player
 * @param preset the preset (between 'a' and 'f')
 * @return true for valid presets, false for invalid presets
 */
gboolean player_assign_bonus_stats(player *p, char preset);
void player_destroy(player *p);

cJSON *player_serialize(player *p);
player *player_deserialize(cJSON *pser);

/**
 * @brief consume time for an action by the player
 *
 * @param p the player
 * @param turns the number of turns the move takes
 * @param interruptible true if the action can be interrupted
 * @param desc (optional) the description of the action in the form "reading
 *    the book of foo"
 * @return true if the action has completed, false if it has been interrupted
 *
 */
gboolean player_make_move(player *p, int turns, gboolean interruptible, const char *desc, ...);

/**
 * Kill the player
 *
 * @param p the player
 * @param cause_type the cause, e.g. PD_TRAP
 * @param cause the id of the specific cause, e.g. TT_DART
 *
 */
void player_die(player *p, player_cod cause_type, int cause);

guint64 player_calc_score(player *p, int won);
gboolean player_movement_possible(player *p);
int player_move(player *p, direction dir, gboolean open_door);
int player_attack(player *p, monster *m);
void player_update_fov(player *p);

/**
 * Function to enter a map.
 *
 * @param p the player
 * @param l entered map
 * @param teleported has to be true if the player didn't enter the map
 *      regularly. Sets a random starting point.
 * @return true
 */
int player_map_enter(player *p, map *l, gboolean teleported);

/**
 * Choose a random armour the player is wearing.
 *
 * @param p the player
 * @param enchantable return only pieces of armour that are not fully enchanted
 * @return a pointer to the slot of the armour
 *
 */
item **player_get_random_armour(player *p, bool enchantable);

void player_pickup(player *p);

void player_level_gain(player *p, int count);
void player_level_lose(player *p, int count);

void player_exp_gain(player *p, int count);
void player_exp_lose(player *p, guint count);

int player_hp_gain(player *p, int count);
int player_hp_lose(player *p, int count, player_cod cause_type, int cause);

/**
 * Determine if the player can evade a quickly moving attack
 *
 * @param p the player
 * @return result of evasion attempt
 */
gboolean player_evade(player *p);

/**
 * Inflict damage upon the player
 *
 * @param p the player
 * @param dam pointer to the damage (will be freed)
 * @param cause_type of the damage originator
 * @param cause the id of the damage originator, specific to the damage originator
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
effect *player_effect_get(player *p, effect_t et);
int player_effect(player *p, effect_t et); /* check if a effect is set */
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
 * @param inv the inventory to check
 * @param it the item which is about to be added
 *
 */
int player_inv_pre_add(inventory *inv, item *it);

/**
 * Callback function used from inv_add() for the player's inventory. Here the
 * weight of the inventory gets calculated and the burdened or overstrained
 * mode is set or removed.
 *
 * @param inv the inventory to check
 * @param it the item which is about to be added
 *
 */
void player_inv_weight_recalc(inventory *inv, item *it);

/**
 * @brief Display a message window with a list of equipped items
 *
 * @param p The player.
 */
void player_paperdoll(player *p);

/* dealing with items */

/**
  * @brief Function used to equip an item.
  *
  * @param p the player
  * @param inv unused, needed to make function signature match display_inventory requirements
  * @param it the item
  */
void player_item_equip(player *p, inventory **inv, item *it);

/**
  * @brief Function used to unequip an item.
  *
  * @param p the player
  * @param inv unused, needed to make function signature match display_inventory requirements
  * @param it the item
  */
void player_item_unequip_wrapper(player *p, inventory **inv, item *it);

/**
  * @brief Function used to unequip an item.
  *
  * @param p the player
  * @param inv unused, needed to make function signature match display_inventory requirements
  * @param it the item
  * @param forced true if the removal does not occur interactively.
  *     No time is consumed in this case.
  */
void player_item_unequip(player *p,
                         inventory **inv __attribute__((unused)),
                         item *it,
                         gboolean forced);

int player_item_is_container(player *p, item *it);
int player_item_can_be_added_to_container(player *p, item *it);
int player_item_filter_unequippable(item* it);

/**
 * Callback function used to check if an item is equipped.
 *
 * @param p the player
 * @param it the item
 * @return place where item is equipped
 */
int player_item_is_equipped(player *p, item *it);

int player_item_is_equippable(player *p, item *it);
int player_item_is_unequippable(player *p, item *it);
int player_item_is_usable(player *p, item *it);
int player_item_is_dropable(player *p, item *it);
int player_item_is_damaged(player *p, item *it);
int player_item_is_affordable(player *p, item *it);
int player_item_is_sellable(player *p, item *it);
int player_item_is_identifiable(player *p, item *it);
int player_item_known(player *p, item *it);
int player_item_identified(player *p, item *it);
int player_item_not_equipped(item *it);
char *player_item_identified_list(player *p);
void player_item_identify(player *p, inventory **inv, item *it);
void player_item_use(player *p, inventory **inv, item *it);
void player_item_destroy(player *p, item *it);
void player_item_drop(player *p, inventory **inv, item *it);

/**
 * @brief Take notes about an item.
 */
void player_item_notes(player *p, inventory **inv, item *it);

/* item usage shortcuts */
void player_read(player *p);
void player_quaff(player *p);
void player_equip(player *p);
void player_take_off(player *p);
void player_drop(player *p);

/* query values */
guint player_get_ac(player *p);
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
/**
  * @brief Remove a given number of gold coins from the Player's inventory
  *
  * @param p The player
  * @param amount The amount of gold to remove
  */
void player_remove_gold(player *p, guint amount);

const char *player_get_level_desc(player *p);

/**
  * @brief Look for traps on adjacent map tiles.
  *
  * @param p the player
  */
void player_search(player *p);

void player_list_sobjmem(player *p);
void player_sobject_forget(player *p, position pos);

/**
  * @brief Check for adjacent monsters.
  *
  * @param p The player.
  * @param ignore_harmless true if harmless monsters shall be ignored.
  *
  * @return true if there are adjacent monsters.
  */
gboolean player_adjacent_monster(player *p, gboolean ignore_harmless);

/* fighting simulation */
void calc_fighting_stats(player *p);

/* macros */

#define player_memory_of(p,pos) ((p)->memory[Z(pos)][Y(pos)][X(pos)])

#endif
