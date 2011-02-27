/*
 * container.c
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

#include <assert.h>

#include "container.h"
#include "display.h"
#include "nlarn.h"
#include "player.h"

const container_data containers[CT_MAX] =
{
    { CT_NONE,   "",          0, IM_NONE,    0, },
    { CT_BAG,    "bag",     375, IM_CLOTH,  15, },
    { CT_CASKET, "casket", 3900, IM_WOOD,   30, },
    { CT_CHEST,  "chest", 13500, IM_WOOD,   70, },
    { CT_CRATE,  "crate", 65000, IM_WOOD,  100, },
};

static void container_trigger_trap(player *p, item *container);

void container_open(player *p, inventory **inv, item *container)
{
    gchar container_desc[61] = { 0 };
    GPtrArray *callbacks;
    display_inv_callback *callback;
    gboolean container_provided = (container == NULL);

    assert (p != NULL);

    /* don't need that parameter */
    inv = NULL;

    if (container == NULL)
    {
        /* no container has been passed - look for container on the floor */
        inventory **inv = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);
        int count = inv_length_filtered(*inv, &item_filter_container);

        if (count == 0)
        {
            log_add_entry(nlarn->log, "I see no container here.");
            return;
        }
        else if (count == 1)
        {
            container = inv_get_filtered(*inv, 0, &item_filter_container);
        }
        else
        {
            /* multiple containers */
            log_add_entry(nlarn->log, "I don't know which container I should open!");
            return;
        }
    }

    /* Describe container */
    item_describe(container, player_item_known(p, container),
                  TRUE, FALSE, container_desc, 60);

    if (!player_make_move(p, 2, TRUE, "opening %s", container_desc))
        return; /* interrupted */

    /* log the event */
    if (!container_provided)
        log_add_entry(nlarn->log, "You carefully open the %s.", container_desc);

    /* check if the container is trapped */
    if (container->cursed)
    {
        /* the player has triggerd the trap on the container */
        container_trigger_trap(p, container);

        /* remove the trap */
        container->cursed = FALSE;
    }

    /* check for empty container */
    if (inv_length(container->content) == 0)
    {
        /* same description, this time definite */
        item_describe(container, player_item_known(p, container),
                      TRUE, TRUE, container_desc, 60);

        /* and the first char has to be upper case */
        container_desc[0] = g_ascii_toupper(container_desc[0]);

        log_add_entry(nlarn->log, "%s is empty.", container_desc);
        return;
    }

    /* prepare callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(g)et";
    callback->key = 'g';
    callback->inv = &container->content;
    callback->function = &container_item_unpack;
    callback->active = TRUE;

    g_ptr_array_add(callbacks, callback);

    /* upper case the first char for the dialogue title */
    container_desc[0] = g_ascii_toupper(container_desc[0]);

    display_inventory(container_desc, p, &container->content, callbacks, FALSE,
                      TRUE, FALSE, NULL);

    display_inv_callbacks_clean(callbacks);
}

void container_item_add(player *p, inventory **inv, item *element)
{
    inventory **target_inv = NULL;
    gchar container_desc[61]  = { 0 };
    gchar element_desc[61]  = { 0 };
    guint pilen = 0; /* length of player's filtered inventory */
    guint filen = 0; /* length of filtered floor inventory */
    gboolean carried_container = FALSE;
    int count = 0;

    assert(p != NULL && element != NULL);

    if (inv == &nlarn->player_home)
    {
        /* player wants to deposit something at home */
        g_snprintf(container_desc, 60, "your storage room");
        target_inv = &nlarn->player_home;
    }
    else if (inv == NULL || (inv == &p->inventory))
    {
        item *container = NULL;
        inventory **floor = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);
        pilen = inv_length_filtered(p->inventory, item_filter_container);
        filen = inv_length_filtered(*floor, item_filter_container);

        /* choose the container to add the item element to. */
        if (pilen == 1)
        {
            /* only one container in inventory - this is the one */
            container = inv_get_filtered(p->inventory, 0, item_filter_container);
            carried_container = TRUE;
        }
        else if (pilen > 1)
        {
            /* multiple containers in the player's inventory, offer to choose one */
            container = display_inventory("Choose a container", p,
                                          &p->inventory, NULL, FALSE, FALSE,
                                          FALSE, item_filter_container);
            carried_container = TRUE;
        }
        else if (filen == 1)
        {
            /* conly one container on the floor */
            container = inv_get_filtered(*floor, 0, item_filter_container);
        }
        else if (filen > 1)
        {
            /* multiple containers on the floor, offer to choose one */
            container = display_inventory("Choose a container", p, floor, NULL,
                                          FALSE, FALSE, FALSE, item_filter_container);
        }

        /* has a container been found? */
        if (container == NULL)
        {
            /* no container has been selected */
            log_add_entry(nlarn->log, "Huh?");
            return;
        }

        /* set target inventory for item movement */
        target_inv = &container->content;

        /* prepare container description */
        item_describe(container, TRUE, TRUE, TRUE, container_desc, 60);

        if (!player_make_move(p, 2, TRUE, "opening %s", container_desc))
            return; /* interrupted */
    }

    /* mute the log if the container is in the player's inventory.
       otherwise mindless burdened staus messages would appear */
    if (carried_container) log_disable(nlarn->log);

    if (element->count > 1)
    {
        /* use the item type plural name except for ammunition */
        g_snprintf(element_desc, 60, "How many %s%s do you want to put into %s?",
                   (element->type == IT_AMMO ? ammo_name(element) : item_name_pl(element->type)),
                   (element->type == IT_AMMO ? "s" : ""), container_desc);

        count = display_get_count(element_desc, element->count);

        if (count == 0)
        {
            return;
        }

        if (count < element->count)
        {
            /* replace element with a copy of element with the chosen amount */
            element = item_split(element, count);
        }
        else
        {
            /* remove the entire item from the player's inventory */
            inv_del_element(&p->inventory, element);
        }
    }
    else
    {
        /* remove the item from the player's inventory */
        inv_del_element(&p->inventory, element);
    }

    if (carried_container) log_enable(nlarn->log);

    /* log the event */
    item_describe(element, player_item_known(p, element),
                  (element->count == 1), TRUE, element_desc, 60 );

    log_add_entry(nlarn->log, "You put %s into %s.", element_desc,
                  container_desc);

    if (element->type == IT_GOLD)
        p->stats.gold_found -= element->count;

    inv_add(target_inv, element);

    if (carried_container)
    {
        /* the container is in the player's inventory, thus the weight has
           to recalculated. Silence the log in the meantime to avoid
           pointless messages. */

        log_disable(nlarn->log);
        player_inv_weight_recalc(p->inventory, NULL);
        log_enable(nlarn->log);
    }
}

void container_item_unpack(player *p, inventory **inv, item *element)
{
    gchar desc[61] = { 0 };
    int count = 0;

    assert(p != NULL && inv != NULL && element != NULL);

    if (element->count > 1)
    {
        /* use the item type plural name except for ammunition */
        g_snprintf(desc, 60, "How many %s%s do you want to take out?",
                   (element->type == IT_AMMO ? ammo_name(element) : item_name_pl(element->type)),
                   (element->type == IT_AMMO ? "s" : ""));

        count = display_get_count(desc, element->count);

        if (count == 0)
        {
            return;
        }

        if (count < element->count)
        {
            /* replace element with a copy of element with the chosen amount */
            element = item_split(element, count);
        }
        else
        {
            /* take the entire item out of the container */
            inv_del_element(inv, element);
        }
    }
    else
    {
        /* remove the element from the originating inventory before
           adding it to the player's. This allows to move items from
           carried containers to the main inventory without being
           rejected because of weight issues. */
        inv_del_element(inv, element);
    }

    item_describe(element, player_item_known(p, element), FALSE, FALSE, desc, 60);

    if (!player_make_move(p, 2, TRUE, "putting %s in your pack", desc))
    {
        /* interrupted! */
        /* return the item into the originating inventory */
        inv_add(inv, element);
        return;
    }

    if (inv_add(&p->inventory, element))
    {
        log_add_entry(nlarn->log, "You put %s into your pack.", desc);

        if (element->type == IT_GOLD)
            p->stats.gold_found += element->count;
    }
    else
    {
        /* if adding the element to the player's pack
           has failed put it back into the container */
        inv_add(inv, element);
        return;
    }
}

int container_move_content(player *p, inventory **inv, inventory **new_inv)
{
    assert(p != NULL && inv != NULL);

    item *it;

    guint idx;
    guint count = 0;
    for (idx = 0; idx < inv_length(*inv);)
    {
        it = inv_get(*inv, idx);
        inv_del_element(inv, it);
        if (inv_add(new_inv, it))
            count++;
    }

    return count;
}

static void container_trigger_trap(player *p, item *container)
{
    effect_t et;
    char *msg = "A little needle shoots out and stings you!";

    switch(rand_1n(5))
    {
    case 1: /* sickness */
        et = ET_SICKNESS;
        break;

    case 2: /* itching powder */
        et = ET_ITCHING;
        msg = "You are engulfed in a cloud of dust!";
        break;

    case 3: /* clumsiness */
        et = ET_CLUMSINESS;
        break;

    case 4: /* slowness */
        et = ET_SLOWNESS;
        break;
    }

    /* tell whats happening */
    log_add_entry(nlarn->log, msg);

    /* add the effect */
    player_effect_add(p, effect_new(et));
}
