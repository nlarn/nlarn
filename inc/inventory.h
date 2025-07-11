/*
 * inventory.h
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

#ifndef __INVENTORY_H_
#define __INVENTORY_H_

#include <glib.h>

#include "items.h"

/* forward declarations */
struct _inventory;

typedef gint (*inv_callback_bool) (struct _inventory *inv, item *item);
typedef void (*inv_callback_void) (struct _inventory *inv, item *item);

typedef struct _inventory
{
    inv_callback_bool pre_add;
    inv_callback_void post_add;
    inv_callback_bool pre_del;
    inv_callback_void post_del;
    gconstpointer owner;
    GPtrArray *content;
} inventory;

/* function definitions */

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
 * to the inventory is possible at all. If this callback returns false,
 * the item in not added to the inventory and control is returned to the
 * calling function.
 *
 * After putting the item into the inventory, the post_add callback is
 * called if it is defined.
 *
 * @param the inventory the item has to be added to
 * @param the item which has to be added
 * @return false on failure, the new length of the inventory upon success
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
 * @return true upon success, false on failure.
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
 * @return true if the oid was removed, false if the oid has not been found.
 *
 */
int inv_del_oid(inventory **inv, gpointer oid);

/**
 * Erode all items in an inventory.
 *
 * @param pointer to the address of the inventory to erode
 * @param the erosion type affecting the inventory
 * @param true if the player can see the inventory, false otherwise
 * @param a filter function to restrict the eroded items
 *
 */
void inv_erode(inventory **inv, item_erosion_type iet,
		gboolean visible, int (*ifilter)(item *));

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
 * @return the number of items for which the filter function returned true
 *
 */
guint inv_length_filtered(inventory *inv, int (*filter)(item *));

item *inv_get_filtered(inventory *inv, guint idx, int (*filter)(item *));

#endif
