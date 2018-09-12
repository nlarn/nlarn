/*
 * container.c
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#include "container.h"
#include "display.h"
#include "nlarn.h"
#include "player.h"
#include "random.h"

const container_data containers[CT_MAX] =
{
    { CT_BAG,    "bag",     375, IM_CLOTH,  15, },
    { CT_CASKET, "casket", 3900, IM_WOOD,   30, },
    { CT_CHEST,  "chest", 13500, IM_WOOD,   70, },
    { CT_CRATE,  "crate", 65000, IM_WOOD,  100, },
};

static item *container_choose(player *p, inventory **floor, gboolean notice);
static gboolean container_trigger_trap(player *p, item *container, gboolean force);

void container_open(player *p, inventory **inv __attribute__((unused)), item *container)
{
    gchar *container_desc;
    GPtrArray *callbacks;
    display_inv_callback *callback;
    gboolean container_provided = (container == NULL);

    g_assert (p != NULL);

    if (container == NULL)
    {
        /* no container has been passed - look for container on the floor */
        inventory **finv = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);
        container = container_choose(p, finv, TRUE);

        /* abort if no container has been selected */
        if (container == NULL) return;
    }

    /* Check if the player is able to move. */
    if (!player_movement_possible(p))
        return;

    /* Describe container */
    container_desc = item_describe(container, player_item_known(p, container),
            TRUE, TRUE);

    if (!player_make_move(p, 2, TRUE, "opening %s", container_desc))
    {
        /* interrupted */
        g_free(container_desc);
        return;
    }

    /* log the event */
    if (!container_provided)
        log_add_entry(nlarn->log, "You carefully open %s.", container_desc);

    /* If there is a trap on the container the player might trigger it. */
    if (container->cursed)
        container_trigger_trap(p, container, FALSE);

    /* check for empty container */
    if (inv_length(container->content) == 0)
    {
        g_free(container_desc);

        /* same description, this time definite */
        container_desc = item_describe(container, player_item_known(p, container),
                                       TRUE, TRUE);

        /* and the first char has to be upper case */
        container_desc[0] = g_ascii_toupper(container_desc[0]);

        log_add_entry(nlarn->log, "%s is empty.", container_desc);
        g_free(container_desc);
        return;
    }

    /* prepare callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(g)et";
    callback->helpmsg = "Get the selected item out the container.";
    callback->key = 'g';
    callback->inv = &container->content;
    callback->function = &container_item_unpack;
    callback->active = TRUE;

    g_ptr_array_add(callbacks, callback);

    /* upper case the first char for the dialogue title */
    container_desc[0] = g_ascii_toupper(container_desc[0]);

    display_inventory(container_desc, p, &container->content, callbacks, FALSE,
                      TRUE, FALSE, NULL);

    g_free(container_desc);
    display_inv_callbacks_clean(callbacks);
}

void container_item_add(player *p, inventory **inv, item *element)
{
    inventory **target_inv = NULL;
    gchar *container_desc = NULL, *element_desc;
    gboolean carried_container = FALSE;

    g_assert(p != NULL && element != NULL);

    /* Check if the player is able to move. */
    if (!player_movement_possible(p))
        return;

    if (inv == &nlarn->player_home)
    {
        /* player wants to deposit something at home */
        container_desc = g_strdup("your storage room");
        target_inv = &nlarn->player_home;
    }
    else if (inv == NULL || (inv == &p->inventory))
    {
        item *container = NULL;
        inventory **floor = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);
        /* length of player's filtered inventory */
        guint pilen = inv_length_filtered(p->inventory, item_filter_container);
        /* length of filtered floor inventory */
        guint filen = inv_length_filtered(*floor, item_filter_container);

        /* choose the container to add the item element to. */
        if (pilen > 0)
        {
            /* choose a container from the player's inventory */
            container = container_choose(p, &p->inventory, FALSE);
            carried_container = TRUE;
        }
        else if (filen > 0)
        {
            container = container_choose(p, floor, TRUE);
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
        container_desc = item_describe(container, TRUE, TRUE, TRUE);

        if (!player_make_move(p, 2, TRUE, "opening %s", container_desc))
        {
            g_free(container_desc);
            return; /* interrupted */
        }
    }

    /* mute the log if the container is in the player's inventory.
       otherwise mindless burdened status messages would appear */
    if (carried_container) log_disable(nlarn->log);

    if (element->count > 1)
    {
        /* use the item type plural name except for ammunition */
        gchar *q = g_strdup_printf("How many %s%s do you want to put into %s?",
                (element->type == IT_AMMO ? ammo_name(element) : item_name_pl(element->type)),
                (element->type == IT_AMMO ? "s" : ""), container_desc);

        guint count = display_get_count(q, element->count);
        g_free(q);

        if (count == 0)
        {
            g_free(container_desc);
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
    element_desc = item_describe(element, player_item_known(p, element),
            FALSE, TRUE);

    log_add_entry(nlarn->log, "You put %s into %s.",
            element_desc, container_desc);

    g_free(element_desc);
    g_free(container_desc);

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
    gchar *desc;

    g_assert(p != NULL && inv != NULL && element != NULL);

    if (element->count > 1)
    {
        /* use the item type plural name except for ammunition */
        gchar *q = g_strdup_printf("How many %s%s do you want to take out?",
                (element->type == IT_AMMO ? ammo_name(element) : item_name_pl(element->type)),
                (element->type == IT_AMMO ? "s" : ""));

        guint count = display_get_count(q, element->count);
        g_free(q);

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

    desc = item_describe(element, player_item_known(p, element), FALSE, FALSE);

    if (!player_make_move(p, 2, TRUE, "putting %s in your pack", desc))
    {
        /* interrupted! */
        /* return the item into the originating inventory */
        inv_add(inv, element);
        g_free(desc);
        return;
    }

    /* keep track of the amount of gold found as element is freed by inv_add */
    int goldcount = (element->type == IT_GOLD) ? goldcount = element->count : 0;

    if (inv_add(&p->inventory, element))
    {
        log_add_entry(nlarn->log, "You put %s into your pack.", desc);
        p->stats.gold_found += goldcount;
    }
    else
    {
        /* if adding the element to the player's pack
           has failed put it back into the container */
        inv_add(inv, element);
    }

    g_free(desc);
}

int container_move_content(player *p __attribute__((unused)), inventory **inv, inventory **new_inv)
{
    g_assert(inv != NULL);

    guint count = 0;

    for (guint idx = 0; idx < inv_length(*inv);)
    {
        item *it = inv_get(*inv, idx);
        inv_del_element(inv, it);
        if (inv_add(new_inv, it))
            count++;
    }

    return count;
}

gboolean container_untrap(player *p)
{
    map *m = game_map(nlarn, Z(p->pos));
    inventory **floor = map_ilist_at(m, p->pos);
    item *container = container_choose(p, floor, FALSE);

    if (container == NULL)
        return FALSE;

    char *desc = item_describe(container, TRUE, TRUE, TRUE);

    if (!container->cursed && !container->blessed_known)
    {
        log_add_entry(nlarn->log, "As far as you know %s is not trapped.", desc);
        g_free(desc);

        return FALSE;
    }

    guint prop = (2 * player_get_dex(p)) + p->level;
    log_add_entry(nlarn->log, "You try to disarm the trap on %s.", desc);
    /*
       Time usage: container type * 2:
         -> a bag takes two turns,
         -> a crate takes eight turns.
     */
    if (!player_make_move(p, container->id * 2, TRUE,
                "Removing the trap on %s?", desc))
    {
        /* action aborted */
        g_free(desc);
        return TRUE;
    }

    if (chance(prop))
    {
        log_add_entry(nlarn->log, "You manage to remove the trap on %s!", desc);

        /* Remove the trap. */
        container->cursed = FALSE;
        container->blessed_known = FALSE;
    }
    else
    {
        /* Sorry, mate.. */
        log_add_entry(nlarn->log, "You trigger the trap on %s!", desc);

        /* The player removed the trap by triggering it. */
        container_trigger_trap(p, container, TRUE);
    }

    g_free(desc);

    return TRUE;
}

static item *container_choose(player *p, inventory **floor, gboolean notice)
{
    int count = inv_length_filtered(*floor, &item_filter_container);

    if (count == 0)
    {
        if (notice)
            log_add_entry(nlarn->log, "I see no container here.");

        return NULL;
    }
    else if (count == 1)
    {
        return inv_get_filtered(*floor, 0, &item_filter_container);
    }
    else
    {
        /* multiple containers on the floor, offer to choose one */
        return display_inventory("Choose a container", p, floor, NULL,
                FALSE, FALSE, FALSE, item_filter_container);
    }
}

static gboolean container_trigger_trap(player *p, item *container, gboolean force)
{
    effect_t et = ET_NONE;

    /* if player knows that the container is trapped,
     * the chance to trigger the trap is lowered */
    if (!force && container->blessed_known && chance(3 * player_get_dex(p)))
    {
        gchar *idesc = item_describe(container, FALSE, TRUE, TRUE);

        log_add_entry(nlarn->log, "You carefully avoid the trap on %s.", idesc);
        g_free(idesc);

        /* the trap remains on the container */
        return FALSE;
    }

    const char *msg = "A little needle shoots out and stings you!";

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

    if (et != ET_NONE)
    {
        /* tell what's happening */
        log_add_entry(nlarn->log, msg);

        /* add the effect */
        player_effect_add(p, effect_new(et));
    }

    /* the player has triggered the trap, thus remove it */
    container->cursed = FALSE;

    /* reset blessed_known, otherwise "uncursed xxx" would appear */
    container->blessed_known = FALSE;

    /* the trap is destroyed */
    return TRUE;
}
