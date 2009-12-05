/*
 * food.h
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

#ifndef __FOOD_H_
#define __FOOD_H_

#include "items.h"

enum food_types
{
    FT_NONE,
    FT_FORTUNE_COOKIE,
    FT_MAX
};

typedef struct food_data {
    int id;
    char *name;
    int weight;
    int price;
} food_data;

/* forward declarations */

struct player;

/* function declarations */

item_usage_result food_eat(struct player *p, item *food);

/* external vars */

extern const food_data foods[FT_MAX];

/* macros */

#define food_name(food)   (foods[(food)->id].name)
#define food_weight(food) (foods[(food)->id].weight)
#define food_price(food)  (foods[(food)->id].price)

#endif
