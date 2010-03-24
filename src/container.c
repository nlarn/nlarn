/*
 * container.c
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

#include <assert.h>

#include "container.h"
#include "display.h"
#include "nlarn.h"
#include "player.h"

const container_data containers[CT_MAX] =
{
    { CT_NONE,   "",          0, IM_NONE,   0, },
    { CT_BAG,    "bag",     375, IM_CLOTH,  1, },
    { CT_CASKET, "casket", 3900, IM_WOOD,   5, },
    { CT_CHEST,  "chest", 13500, IM_WOOD,  10, },
    { CT_CRATE,  "crate", 65000, IM_WOOD,  20, },
};

int container_open(player *p, inventory **inv, item *container)
{
    gchar container_desc[61] = { 0 };
    GPtrArray *callbacks;
    display_inv_callback *callback;

    assert (p != NULL);

    /* don't need that parameter */
    inv = NULL;

    if (container == NULL)
    {
        /* no container has been passed - look for container on the floor */
        inventory **inv = map_ilist_at(game_map(nlarn, p->pos.z), p->pos);
        int count = inv_length_filtered(*inv, &inv_filter_container);

        if (count == 0)
        {
            log_add_entry(p->log, "I see no container here.");
            return FALSE;
        }
        else if (count == 1)
        {
            container = inv_get_filtered(*inv, 0, &inv_filter_container);
        }
        else
        {
            /* multiple containers */
            log_add_entry(p->log, "I don't know which container I should open!");
            return 2;
        }
    }

    /* check for empty container */
    if (inv_length(container->content) == 0)
    {
        item_describe(container, player_item_identified(p, container),
                      TRUE, TRUE, container_desc, 60);

        container_desc[0] = g_ascii_toupper(container_desc[0]);
        log_add_entry(p->log, "%s is empty.", container_desc);

        return 2;
    }

    /* Describe container */
    item_describe(container, player_item_identified(p, container),
                  TRUE, FALSE, container_desc, 60);

    container_desc[0] = g_ascii_toupper(container_desc[0]);

    /* prepare callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(g)et";
    callback->key = 'g';
    callback->inv = &container->content;
    callback->function = &container_item_unpack;
    callback->active = TRUE;

    g_ptr_array_add(callbacks, callback);

    display_inventory(container_desc, p, &container->content, callbacks, FALSE, NULL);

    display_inv_callbacks_clean(callbacks);

    return 2;
}

int container_item_add(player *p, inventory **inv, item *element)
{
    item *container = NULL;
    gchar container_desc[61]  = { 0 };
    gchar element_desc[61]  = { 0 };
    guint pilen = 0; /* length of player's filtered inventory */
    guint filen = 0; /* length of filtered floor inventory */
    int count = 0;
    item *original = element;

    assert(p != NULL && element != NULL);

    if (inv == NULL || (inv == &p->inventory))
    {
        inventory **floor = map_ilist_at(game_map(nlarn, p->pos.z), p->pos);
        pilen = inv_length_filtered(p->inventory, inv_filter_container);
        filen = inv_length_filtered(*floor, inv_filter_container);

        /* choose the container to add the item element to. */
        if (pilen == 1)
        {
            /* only one container in inventory - this is the one */
            container = inv_get_filtered(p->inventory, 0, inv_filter_container);
        }
        else if (pilen > 1)
        {
            /* multiple containers in the player's inventory, offer to choose one */
            container = display_inventory("Choose a container", p,
                                          &p->inventory, NULL, FALSE,
                                          inv_filter_container);
        }
        else if (filen == 1)
        {
            /* conly one container on the floor */
            container = inv_get_filtered(*floor, 0, inv_filter_container);
        }
        else if (filen > 1)
        {
            /* multiple containers on the floor, offer to choose one */
            container = display_inventory("Choose a container", p, floor,
                                          NULL, FALSE, inv_filter_container);
        }
    }

    if (container == NULL)
    {
        /* no container has been selected */
        log_add_entry(p->log, "Huh?");
        return FALSE;
    }

    /* prepare container description */
    item_describe(container, TRUE, TRUE, TRUE, container_desc, 60);

    if (element->count > 1)
    {
        g_snprintf(element_desc, 60, "How many %s do you want to put into the %s?",
                   item_name_pl(element->type), container_desc);

        count = display_get_count(element_desc, element->count);

        if (count == 0)
        {
            return 0;
        }

        if (count < element->count)
        {
            /* replace element with a copy of element with the chosen amount */
            element = item_split(element, count);

            /* set original to NULL to prevent that it gets removed
               from the originating inventory */
            original = NULL;
        }
    }

    /* log the event */
    item_describe(element, player_item_identified(p, element),
                  (element->count == 1), TRUE, element_desc, 60 );

    log_add_entry(p->log, "You put %s into %s.", element_desc,
                  container_desc);

    /* remove the item from the player's inventory if it has not been split */
    if (original != NULL)
    {
        inv_del_element(&p->inventory, element);
    }

    inv_add(&container->content, element);

    return 2;
}

int container_item_unpack(player *p, inventory **inv, item *element)
{
    gchar desc[61] = { 0 };
    int count = 0;
    gpointer oid = element->oid;

    assert(p != NULL && inv != NULL && element != NULL);

    if (element->count > 1)
    {
        g_snprintf(desc, 60, "Pick up how many %s?", item_name_pl(element->type));

        count = display_get_count(desc, element->count);

        if (count == 0)
        {
            return FALSE;
        }

        if (count < element->count)
        {
            /* replace element with a copy of element with the chosen amount */
            element = item_split(element, count);

            /* set original to NULL to prevent that it gets removed
               from the originating inventory */
            oid = NULL;
        }
    }

    if (inv_add(&p->inventory, element))
    {
        item_describe(element, player_item_known(p, element),
                      (element->count == 1), FALSE, desc, 60);
        log_add_entry(p->log, "You put %s into your pack.", desc);

        if (oid != NULL)
        {
            /* remove the element from the originating inventory
               if it has not been split */
             inv_del_oid(inv, oid);
        }
    }
    else if (oid == NULL)
    {
        /* if adding the newly created element to the player's pack
           has failed put it back into the container */
        inv_add(inv, element);

        return FALSE;
    }

    return 2;
}
