/*
 * container.h
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

#ifndef __CONTAINER_H_
#define __CONTAINER_H_

#include "items.h"

enum container_types {
    CT_BAG,
    CT_CASKET,
    CT_CHEST,
    CT_CRATE,
    CT_MAX
};

typedef struct container_data {
    int id;
    const char *name;
    int weight;
    item_material_t material;
    int price;
} container_data;

/* external vars */

extern const container_data containers[CT_MAX];

/* forward declarations */

struct player;

/**
  * @brief Function used to open a container.
  *
  * @param the player
  * @param -unused-
  * @param the container to be opened
  */
void container_open(struct player *p, inventory **inv, item *container);

/**
  * @brief Function used to add an item to a container.
  *
  * @param the player
  * @param the container
  * @param the item to be added
  */
void container_item_add(struct player *p, inventory **inv, item *element);

/**
  * @brief Function used to remove an item from a container.
  *
  * @param the player
  * @param the originating inventory
  * @param the item to be taken out
  */
void container_item_unpack(struct player *p, inventory **inv, item *element);

/**
  * @brief Function used to add an entire inventory to another one.
  *
  * @param the player
  * @param the originating inventory
  * @param the new inventory
  */
int container_move_content(struct player *p, inventory **inv,
                           inventory **new_inv);

/**
  * @brief Remove a trap on a container
  *
  * @param the player
  * @return TRUE if there is a trapped container at the player's position
  */
gboolean container_untrap(struct player *p);

/* macros */

#define container_name(container)     (containers[(container)->id].name)
#define container_weight(container)   (containers[(container)->id].weight)
#define container_material(container) (containers[(container)->id].material)
#define container_price(container)    (containers[(container)->id].price)

#endif
