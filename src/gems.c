/*
 * gems.c
 * Copyright (C) 2009-2011, 2014 Joachim de Groot <jdegroot@web.de>
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

#include "display.h"
#include "gems.h"
#include "items.h"

typedef struct gem_data
{
    int id;
    const char *name;
    int colour;
    int price;          /* price per carat in the shops */
} gem_data;

static const gem_data gems[GT_MAX] =
{
    /* id          name        colour    pr */
    { GT_NONE,     "",         DC_NONE,  0, },
    { GT_DIAMOND,  "diamond",  DC_WHITE, 50, },
    { GT_RUBY,     "ruby",     DC_RED,   40, },
    { GT_EMERALD,  "emerald",  DC_GREEN, 30, },
    { GT_SAPPHIRE, "sapphire", DC_BLUE,  20, },
};

const char *gem_name(item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id > GT_NONE && gem->id < IT_MAX);
    return gems[gem->id].name;
}

int gem_colour(item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id > GT_NONE && gem->id < IT_MAX);
    return gems[gem->id].colour;
}

int gem_weight(item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id > GT_NONE && gem->id < IT_MAX);
    return (gem->bonus / 1000);
}

int gem_price(item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id > GT_NONE && gem->id < IT_MAX);
    return (gem->bonus * gems[gem->id].price);
}

int gem_size(item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id > GT_NONE && gem->id < IT_MAX);
    return gem->bonus;
}
