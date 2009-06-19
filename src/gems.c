/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gems.c
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

#include "nlarn.h"

/* TODO: differenciated prices */
static const gem_data gems[GT_MAX] =
{
    /* id          name        material         colour    pr */
    { GT_NONE,      "",         IM_NONE,        DC_NONE,  0, },
    { GT_DIAMOND,   "diamond",  IM_GEMSTONE,    DC_WHITE, 100, },
    { GT_RUBY,      "ruby",     IM_GEMSTONE,    DC_RED,   100, },
    { GT_EMERALD,   "emerald",  IM_GEMSTONE,    DC_GREEN, 100, },
    { GT_SAPPHIRE,  "sapphire", IM_GEMSTONE,    DC_BLUE,  100, },
};

gem *gem_new(int gem_type, int carat)
{
    gem *ngem;

    assert(gem_type > GT_NONE && gem_type < GT_MAX);

    ngem = g_malloc0(sizeof(gem));

	ngem->type = gem_type;
    ngem->carat = carat;

    /* ensure minimal size */
    if (ngem->carat == 0)
        ngem->carat = rand_1n(20);

    return ngem;
}

void gem_destroy(gem *g) {
	assert(g != NULL);

	g_free(g);
}

inline char *gem_get_name(gem *g)
{
    assert(g != NULL && g->type > GT_NONE && g->type < GT_MAX);
    return gems[g->type].name;
}

inline int gem_get_material(gem *g)
{
    assert(g != NULL && g->type > GT_NONE && g->type < GT_MAX);
    return gems[g->type].material;
}

inline int gem_get_colour(gem *g)
{
    assert(g != NULL && g->type > GT_NONE && g->type < GT_MAX);
    return gems[g->type].colour;
}

inline int gem_get_weight(gem *g)
{
    assert(g != NULL && g->type > GT_NONE && g->type < GT_MAX);
    /* one carat = 200 mg */
    return (g->carat / 1000);
}

inline int gem_get_price(gem *g)
{
    assert(g != NULL && g->type > GT_NONE && g->type < GT_MAX);
    return (g->carat * gems[g->type].price);
}

inline int gem_get_size(gem *g)
{
    assert(g != NULL && g->type > GT_NONE && g->type < GT_MAX);
    return g->carat;
}
