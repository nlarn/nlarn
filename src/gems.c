/*
 * gems.c
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

#include "colours.h"
#include "gems.h"
#include "items.h"

DEFINE_ENUM(gem_t, GEM_TYPE_ENUM)

typedef struct gem_data
{
    gem_t id;
    const char *name;
    colour_t fg;
    int price;          /* price per carat in the shops */
} gem_data;

static const gem_data gems[GT_MAX] =
{
    /* id          name        fg                  pr */
    { GT_DIAMOND,  "diamond",  CHALKY_BLUE_WHITE,  50, },
    { GT_RUBY,     "ruby",     LUMINOUS_RED,       40, },
    { GT_EMERALD,  "emerald",  LIGHT_BRIGHT_GREEN, 30, },
    { GT_SAPPHIRE, "sapphire", BRIGHT_BLUE,        20, },
};

const char *gem_name(const item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id < GT_MAX);
    return gems[gem->id].name;
}

colour_t gem_colour(const item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id < GT_MAX);
    return gems[gem->id].fg;
}

int gem_weight(const item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id < GT_MAX);
    return (gem->bonus / 1000);
}

int gem_price(const item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id < GT_MAX);
    return (gem->bonus * gems[gem->id].price);
}

int gem_size(const item *gem)
{
    g_assert(gem->type == IT_GEM && gem->id < IT_MAX);
    return gem->bonus;
}
