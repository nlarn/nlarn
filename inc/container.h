/*
 * container.h
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

#ifndef __CONTAINER_H_
#define __CONTAINER_H_

#include "items.h"

enum container_types {
    CT_NONE,
    CT_BAG,
    CT_CASKET,
    CT_CHEST,
    CT_CRATE,
    CT_MAX
};

typedef struct container_data {
    int id;
    char *name;
    int weight;
    item_material_t material;
    int price;
} container_data;

/* external vars */

extern const container_data containers[CT_MAX];

/* forward declarations */

struct player;

/* functions */

int container_open(struct player *p, inventory **inv, item *container);
int container_item_add(struct player *p, inventory **inv, item *element);
int container_item_unpack(struct player *p, inventory **inv, item *element);

/* macros */

#define container_name(container)     (containers[(container)->id].name)
#define container_weight(container)   (containers[(container)->id].weight)
#define container_material(container) (containers[(container)->id].material)
#define container_price(container)    (containers[(container)->id].price)

#endif
