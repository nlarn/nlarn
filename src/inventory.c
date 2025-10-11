/*
 * inventory.c
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

#include <glib.h>

#include "amulets.h"
#include "game.h"
#include "inventory.h"
#include "items.h"
#include "extdefs.h"
#include "potions.h"

/* functions */

inventory *inv_new(gconstpointer owner)
{
    inventory *ninv = g_malloc0(sizeof(inventory));
    ninv->content = g_ptr_array_new();

    ninv->owner = owner;

    return ninv;
}

void inv_destroy(inventory *inv, gboolean special)
{
    g_assert(inv != NULL);

    while (inv_length(inv) > 0)
    {
        item *it = inv_get(inv, inv_length(inv) - 1);

        if (special)
        {
            /* Ensure the potion of cure dianthroritis can be recreated */
            if (it->type == IT_POTION && it->id == PO_CURE_DIANTHR)
                nlarn->cure_dianthr_created = false;

            /* allow recreating unique items */
            if (it->type == IT_AMULET)
                nlarn->amulet_created[it->id] = false;

            if (it->type == IT_ARMOUR && armour_unique(it))
                nlarn->armour_created[it->id] = false;

            if (it->type == IT_WEAPON && weapon_is_unique(it))
                nlarn->weapon_created[it->id] = false;
        }

        g_ptr_array_remove(inv->content, it->oid);
        item_destroy(it);
    }

    g_ptr_array_free(inv->content, true);

    g_free(inv);
}

cJSON *inv_serialize(inventory *inv)
{
    cJSON *sinv = cJSON_CreateArray();

    for (guint idx = 0; idx < inv_length(inv); idx++)
    {
        item *it = inv_get(inv, idx);
        cJSON_AddItemToArray(sinv, cJSON_CreateNumber(GPOINTER_TO_UINT(it->oid)));
    }

    return sinv;
}

inventory *inv_deserialize(cJSON *iser)
{
    inventory *inv = g_malloc0(sizeof(inventory));
    inv->content = g_ptr_array_new();

    for (int idx = 0; idx < cJSON_GetArraySize(iser); idx++)
    {
        guint oid = cJSON_GetArrayItem(iser, idx)->valueint;
        g_ptr_array_add(inv->content, GUINT_TO_POINTER(oid));
    }

    return inv;
}

void inv_callbacks_set(inventory *inv, inv_callback_bool pre_add,
                       inv_callback_void post_add, inv_callback_bool pre_del,
                       inv_callback_void post_del)
{
    g_assert (inv != NULL);

    inv->pre_add  = pre_add;
    inv->post_add = post_add;
    inv->pre_del  = pre_del;
    inv->post_del = post_del;
}

int inv_add(inventory **inv, item *it)
{
    g_assert(inv != NULL && it != NULL && it->oid != NULL);

    /* create inventory if necessary */
    if (!(*inv))
    {
        *inv = inv_new(NULL);
    }

    /* check if pre_add callback is set */
    if ((*inv)->pre_add)
    {
        /* call pre_add callback */
        if (!(*inv)->pre_add(*inv, it))
        {
            return false;
        }
    }

    /* stack stackable items */
    if (item_is_stackable(it->type))
    {
        /* loop through items in the target inventory to find a similar item */
        for (guint idx = 0; idx < inv_length(*inv); idx++)
        {
            item *i = inv_get(*inv, idx);
            /* compare the current item with the one which is to be added */
            if (item_compare(i, it))
            {
                /* just increase item count and release the original */
                i->count += it->count;
                item_destroy(it);

                it = NULL;

                break;
            }
        }
    }

    if (it != NULL)
    {
        /* add the item to the inventory if it has not already been added */
        g_ptr_array_add((*inv)->content, it->oid);
    }

    /* call post_add callback */
    if ((*inv)->post_add)
    {
        (*inv)->post_add(*inv, it);
    }

    return inv_length(*inv);
}

item *inv_get(inventory *inv, guint idx)
{
    g_assert (inv != NULL && idx < inv->content->len);
    gpointer oid = g_ptr_array_index(inv->content, idx);

    return game_item_get(nlarn, oid);
}

item *inv_del(inventory **inv, guint idx)
{
    g_assert(*inv != NULL && (*inv)->content != NULL && idx < inv_length(*inv));

    item *itm = inv_get(*inv, idx);

    if ((*inv)->pre_del)
    {
        if (!(*inv)->pre_del(*inv, itm))
        {
            return NULL;
        }
    }

    g_ptr_array_remove_index((*inv)->content, idx);

    if ((*inv)->post_del)
    {
        (*inv)->post_del(*inv, itm);
    }

    /* destroy inventory if empty and not owned by anybody */
    if (!inv_length(*inv) && !(*inv)->owner)
    {
        inv_destroy(*inv, false);
        *inv = NULL;
    }

    return itm;
}

int inv_del_element(inventory **inv, item *it)
{
    g_assert(*inv != NULL && (*inv)->content != NULL && it != NULL);

    if ((*inv)->pre_del)
    {
        if (!(*inv)->pre_del(*inv, it))
        {
            return false;
        }
    }

    g_ptr_array_remove((*inv)->content, it->oid);

    if ((*inv)->post_del)
    {
        (*inv)->post_del(*inv, it);
    }

    /* destroy inventory if empty and not owned by anybody */
    if (!inv_length(*inv) && !(*inv)->owner)
    {
        inv_destroy(*inv, false);
        *inv = NULL;
    }

    return true;
}

int inv_del_oid(inventory **inv, gpointer oid)
{
    g_assert(*inv != NULL && (*inv)->content != NULL && oid != NULL);

    if (!g_ptr_array_remove((*inv)->content, oid))
    {
        return false;
    }

    /* destroy inventory if empty and not owned by anybody */
    if (!inv_length(*inv) && !(*inv)->owner)
    {
        inv_destroy(*inv, false);
        *inv = NULL;
    }

    return true;
}

void inv_erode(inventory **inv, item_erosion_type iet,
               gboolean visible, int (*ifilter)(item *))
{
    g_assert(inv != NULL);

    for (guint idx = 0; idx < inv_length(*inv); idx++)
    {
        item *it = inv_get(*inv, idx);

        /*
         * If no filter was given, erode all items, otherwise
         * those which are agreed on by the filter function.
         */
        if (ifilter == NULL || ifilter(it))
        {
            item_erode(inv, it, iet, visible);
        }
    }
}

guint inv_length(inventory *inv)
{
    return (inv == NULL) ? 0 : inv->content->len;
}

void inv_sort(inventory *inv, GCompareDataFunc compare_func, gpointer user_data)
{
    g_assert(inv != NULL && inv->content != NULL);

    g_ptr_array_sort_with_data(inv->content, compare_func, user_data);
}

int inv_weight(inventory *inv)
{
    int sum = 0;

    if (inv == NULL)
    {
        return 0;
    }

    /* add contents weight */
    for (guint idx = 0; idx < inv_length(inv); idx++)
    {
        sum += item_weight(inv_get(inv, idx));
    }

    return sum;
}

guint inv_length_filtered(inventory *inv, int (*ifilter)(item *))
{
    int count = 0;

    if (inv == NULL)
    {
        /* check for non-existent inventories */
        return 0;
    }

    /* return the inventory length if no filter has been set */
    if (!ifilter)
    {
        return inv_length(inv);
    }

    for (guint pos = 0; pos < inv_length(inv); pos++)
    {
        item *i = inv_get(inv, pos);

        if (ifilter(i))
        {
            count++;
        }
    }

    return count;
}

item *inv_get_filtered(inventory *inv, guint idx, int (*ifilter)(item *))
{
    guint curr = 0;

    /* return the inventory length if no filter has been set */
    if (!ifilter)
    {
        return inv_get(inv, idx);
    }

    for (guint num = 0; num < inv_length(inv); num++)
    {
        item *i = inv_get(inv, num);

        if (ifilter(i))
        {
            /* filter matches */
            if (curr == idx)
            {
                /* this is the requested item */
                return i;
            }
            else
            {
                curr++;
            }
        }
    }

    /* not found */
    return NULL;
}
