/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * container.c
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

static const container_data containers[CT_MAX] =
{
    { CT_NONE,   "",          0, },
    { CT_BAG,    "bag",     375, },
    { CT_CASKET, "casket", 3900, },
    { CT_CHEST,  "chest", 13500, },
    { CT_CRATE,  "crate", 65000, },
};

container *container_new(int container_type) {
    container *ncontainer;

    assert(container_type > CT_NONE && container_type < CT_MAX);

    ncontainer = g_malloc0(sizeof(container));

    ncontainer->type = container_type;
    ncontainer->content = inv_new();

    return ncontainer;
}

void container_destroy(container *c)
{
    assert(c != NULL);

    inv_destroy(c->content);
    g_free(c);
}

inline char *container_get_name(container *c)
{
    assert(c != NULL && c->type > CT_NONE && c->type < CT_MAX);
    return containers[c->type].name;
}

inline int container_get_weight(container *c)
{
    int sum = 0;
    int pos;

    assert(c != NULL && c->type > CT_NONE && c->type < CT_MAX);

    sum += containers[c->type].weight;

    /* add contents weight */
    for (pos = 1; pos <= inv_length(c->content); pos++)
        sum += item_get_weight(inv_get(c->content, pos - 1));

    return sum;
}
