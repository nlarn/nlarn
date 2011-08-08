/*
 * items.h
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

#ifndef __ITEM_H_
#define __ITEM_H_

#include <glib.h>

#include "effects.h"

typedef enum item_types {
    IT_NONE,
    IT_AMULET,          /* amulet, defined in amulets.h */
    IT_AMMO,            /* ammunition, defined in weapons.h */
    IT_ARMOUR,          /* armour, defined in armour.h */
    IT_BOOK,            /* book, defined in spells.h */
    IT_CONTAINER,       /* container, defined in container.h */
    IT_GEM,             /* gem, defined in gems.h */
    IT_GOLD,            /* just gold. defined nowhere as type and count are sufficient. */
    IT_POTION,          /* potion, defined in potions.h */
    IT_RING,            /* ring, defined in rings.h */
    IT_SCROLL,          /* magic_scroll, defined in scrolls.h */
    IT_WEAPON,          /* weapon, defined in weapons.h */
    IT_MAX,             /* ~ item type count */
    IT_ALL             /* for special uses: all item types */
} item_t;

/* inspired by Nethack's objclass.h */
typedef enum item_material_t {
    IM_NONE,
    IM_PAPER,
    IM_CLOTH,
    IM_LEATHER,
    IM_WOOD,
    IM_BONE,
    /* materials up to here can burn */
    IM_DRAGON_HIDE,     /* not leather! */
    IM_LEAD,
    IM_IRON,            /* Fe, can rust/corrode */
    IM_STEEL,           /* stainless steel */
    IM_COPPER,          /* Cu - includes brass */
    IM_SILVER,
    IM_GOLD,            /* Au */
    IM_PLATINUM,        /* Pt */
    IM_MITHRIL,
    IM_GLASS,
    IM_STONE,
    IM_GEMSTONE,
    IM_MAX              /* ~ item material count */
} item_material_t;

typedef struct item_material_data {
    item_material_t type;
    char *name;
    char *adjective;
    int colour;
} item_material_data;

typedef struct item_usage_result
{
    int identified;
    int used_up;
} item_usage_result;

typedef enum item_erosion_t {
    IET_NONE,
    IET_BURN,
    IET_CORRODE,
    IET_RUST,
    IET_MAX
} item_erosion_type;

struct game;
struct _inventory;
struct _item;

typedef gint (*inv_callback_bool) (struct _inventory *inv, struct _item  *item);
typedef void (*inv_callback_void) (struct _inventory *inv, struct _item  *item);

typedef struct _inventory
{
    inv_callback_bool pre_add;
    inv_callback_void post_add;
    inv_callback_bool pre_del;
    inv_callback_void post_del;
    gconstpointer owner;
    GPtrArray *content;
} inventory;

typedef struct _item {
    gpointer oid;           /* item's game object id */
    item_t type;            /* element type */
    guint32 id;             /* item id, type specific */
    gint32 bonus;
    guint32 count;          /* for stackable items */
    GPtrArray *effects;     /* storage for effects */
    inventory *content;     /* for containers */
    char *notes;            /* storage for player's notes about the item */
    guint32
        blessed: 1,
        cursed: 1,
        corroded: 2,        /* 0: no; 1: yes; 2: very */
        burnt: 2,           /* 0: no; 1: yes; 2: very */
        rusty: 2,           /* 0: no; 1: yes; 2: very */
        blessed_known: 1,   /* player known if item is cursed / blessed */
        bonus_known: 1;     /* player knows the bonus */
} item;

typedef struct item_type_data {
    item_t id;
    char *name_sg;
    char *name_pl;
    char glyph;
    guint max_id;
    unsigned
        optimizable: 1,     /* item type can have a bonus */
        blessable: 1,       /* item type can be blessed / cursed */
        corrodible: 1,      /* item type can corrode */
        equippable: 1,
        usable: 1,
        stackable: 1,
        identifyable: 1;
} item_type_data;

/* function definitions */

item *item_new(item_t item_type, int item_id);
item *item_new_random(item_t item_type, gboolean finetouch);
item *item_new_by_level(item_t item_type, int num_level);
item *item_new_finetouch(item *it);
item *item_copy(item *original);
item *item_split(item *original, guint32 count);
void item_destroy(item *it);

void item_serialize(gpointer oid, gpointer it, gpointer root);
item *item_deserialize(cJSON *iser, struct game *g);

/**
 * Compare two items.
 *
 * @param a item1
 * @param b item2
 * @return TRUE if items are identical
 */
int item_compare(item *a, item *b);

int item_sort(gconstpointer a, gconstpointer b, gpointer data, gboolean force_id);

/**
 * Describe an item.
 *
 * @param the item
 * @param TRUE if the item is known to the player.
 * @param TRUE if the item count shall be ignored and the description for
 *       a single item of a stack shall be returned.
 * @param TRUE if the description shall be prepend by the definite article.
 * @return a newly allocated string that should be disposed with g_free().
 */
gchar *item_describe(item *it, int known, int singular, int definite);

item_material_t item_material(item *it);
guint item_base_price(item *it);
guint item_price(item *it);

/**
 * Calculate the weight of the given item.
 *
 * @param an item
 * @return weight in grams
 */
int item_weight(item *it);

/**
 * Determine the colour of the given object.
 *
 * @param an item
 * @return the coulour
 */
int item_colour(item *it);

void item_effect_add(item *it, effect *e);
void item_effect_del(item *it, effect *e);

int item_bless(item *it);
int item_curse(item *it);
int item_remove_blessing(item *it);
int item_remove_curse(item *it);

item *item_enchant(item *it);
item *item_disenchant(item *it);

/**
 * Erode an item.
 *
 * @param  the inventory the item is in (may be null for new items)
 * @param  the item to erode
 * @param  the type of erosion which affects the item
 * @param  TRUE if the player can see the item
 * @return the item, NULL it the item has been destroyed
 *
 */
item *item_erode(inventory **inv, item *it, item_erosion_type iet, gboolean visible);

int item_obtainable(item_t type, int id);

/**
 * @brief Describe an item thoroughly.
 *
 * @param An item.
 * @param (Y/N) if the item is known
 * @param (Y/N) show the item price
 * @return A newly allocated string that must be freed in the calling function.
 */
char *item_detailed_description(item *it, gboolean known, gboolean shop);

/* external vars */
extern const item_type_data item_data[IT_MAX];
extern const item_material_data item_materials[IM_MAX];

/* item macros */
#define item_glyph(type)          item_data[(type)].glyph
#define item_name_sg(type)        item_data[(type)].name_sg
#define item_name_pl(type)        item_data[(type)].name_pl
#define item_max_id(type)         item_data[(type)].max_id
#define item_is_optimizable(type)     item_data[(type)].optimizable
#define item_is_blessable(type)       item_data[(type)].blessable
#define item_is_corrodible(type)      item_data[(type)].corrodible
#define item_is_equippable(type)      item_data[(type)].equippable
#define item_is_usable(type)          item_data[(type)].usable
#define item_is_stackable(type)       item_data[(type)].stackable
#define item_is_identifyable(type)    item_data[(type)].identifyable
#define item_material_name(type)      item_materials[(type)].name
#define item_material_adjective(type) item_materials[(type)].adjective

/* inventory functions */

inventory *inv_new(gconstpointer owner);
void inv_destroy(inventory *inv, gboolean special);

cJSON *inv_serialize(inventory *inv);
inventory *inv_deserialize(cJSON *iser);

void inv_callbacks_set(inventory *inv, inv_callback_bool pre_add,
                       inv_callback_void post_add, inv_callback_bool pre_del,
                       inv_callback_void post_del);

/**
 * Function to add an item to an inventory.
 *
 * This function calls the pre_add callback to determine if adding the item
 * to the inventory is possible at all. If this callback returns FALSE,
 * the item in not added to the inventory and control is returned to the
 * calling function.
 *
 * After putting the item into the inventory, the post_add callback is
 * called if it is defined.
 *
 * @param the inventory the item has to be added to
 * @param the item which has to be added
 * @return FALSE on failure, the new length of the inventory upon success
 */
int inv_add(inventory **inv, item *it);

item *inv_get(inventory *inv, guint idx);

/**
 * Function to remove an item from an inventory by its index.
 *
 * If the pre_del callback is set, it is used to determine if the action is
 * valid. If the post_del callback is set, it is called after removing the
 * item.
 *
 * If the inventories owner attribute is not set, empty inventories get
 * destroyed.
 *
 * @param the inventory from which the item shall be removed
 * @param the index of the item in the inventory
 * @return an pointer to the removed item
 *
 */
item *inv_del(inventory **inv, guint idx);

/**
 * Function to remove an item from an inventory.
 *
 * If the pre_del callback is set, it is used to determine if the action is
 * valid. If the post_del callback is set, it is called after removing the
 * item.
 *
 * If the inventories owner attribute is not set, empty inventories get
 * destroyed.
 *
 * @param the inventory from which the item shall be removed
 * @param the item to be removed
 * @return TRUE upon success, FALSE on failure.
 *
 */
int inv_del_element(inventory **inv, item *it);

/**
 * Function to remove an item from an inventory by its oid.
 *
 * The inventory's callback functions are ignored.
 * If the inventories owner attribute is not set, empty inventories get
 * destroyed.
 *
 * @param the inventory from which the item shall be removed
 * @param the oid of the item to be removed
 * @return TRUE if the oid was removed, FALSE if the oid has not been found.
 *
 */
int inv_del_oid(inventory **inv, gpointer oid);

/**
 * Erode all items in an inventory.
 *
 * @param pointer to the address of the inventory to erode
 * @param the erosion type affecting the inventory
 * @param TRUE if the player can see the inventory, FALSE otherwise
 *
 */
void inv_erode(inventory **inv, item_erosion_type iet, gboolean visible);

/**
 * Function to determine the count of items in an inventory.
 *
 * @param the inventory to be counted
 * @return the count of items in the inventory
 *
 */
guint inv_length(inventory *inv);

/**
 * Function to sort the items in an inventory.
 *
 * @param the inventory to be sorted
 * @param the function used to compare the items
 * @param additional data for the compare function
 *
 */
void inv_sort(inventory *inv, GCompareDataFunc compare_func, gpointer user_data);

/**
 * Function to determine the weight of all items in an inventory.
 *
 * @param the inventory
 * @return the weight of the inventory in grams
 *
 */
int inv_weight(inventory *inv);

/**
 * Count an filtered inventory.
 *
 * @param the inventory to look in
 * @param the filter function
 * @return the number of items for which the filter function returned TRUE
 *
 */
guint inv_length_filtered(inventory *inv, int (*filter)(item *));

item *inv_get_filtered(inventory *inv, guint idx, int (*filter)(item *));

/* item filters */

int item_filter_container(item *it);
int item_filter_not_container(item *it);
int item_filter_gems(item *it);
int item_filter_gold(item *it);
int item_filter_not_gold(item *it);
int item_filter_potions(item *it);
int item_filter_legible(item *it);
int item_filter_unid(item *it);
int item_filter_cursed(item *it);
int item_filter_cursed_or_unknown(item *it);
int item_filter_nonblessed(item *it);

/**
 * @brief Item filter function for the potion of cure dianthroritis.
 * @param a pointer to an item
 * @return TRUE if the supplied item is the potion of cure dianthroritis
 */
int item_filter_pcd(item *it);
/**
 * @brief Item filter function for blank scrolls.
 * @param a pointer to an item
 * @return TRUE if the supplied item is a blank scroll
 */
int item_filter_blank_scroll(item *it);
#endif
