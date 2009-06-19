/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * rings.c
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

static const ring_data rings[RT_MAX] =
{
    /* type            name                  effect           price ob */
    { RT_NONE,         "",                   ET_NONE,            0, 0, },
    { RT_EXTRA_REGEN,  "extra regeneration", ET_INC_HP_REGEN, 1000, 1, },
    { RT_REGENERATION, "regeneration",       ET_INC_HP_REGEN,  220, 1, },
    { RT_PROTECTION,   "protection",         ET_PROTECTION,    150, 0, },
    { RT_ENERGY,       "energy",             ET_INC_MP_REGEN,  180, 1, },
    { RT_DEXTERITY,    "dexterity",          ET_INC_DEX,       120, 1, },
    { RT_STRENGTH,     "strength",           ET_INC_STR,        85, 1, },
    { RT_CLEVERNESS,   "cleverness",         ET_INC_INT,       120, 1, },
    { RT_INC_DAMAGE,   "increase damage",    ET_INC_DAMAGE,    125, 0, },
};

static int ring_material_mapping[RT_MAX - 1] = { 0 };

/* mean: some materials appear more than once */
static const int ring_material[RT_MAX - 1] =
{
    IM_GOLD,
    IM_SILVER,
    IM_PLATINUM,
    IM_SILVER,
    IM_COPPER,
    IM_STEEL,
    IM_GLASS,
    IM_BONE
};

void ring_material_shuffle()
{
    shuffle(ring_material_mapping, RT_MAX - 1, 0);
}

ring *ring_new(int ring_type, int bonus)
{
    ring *nring;

    assert(ring_type > RT_NONE && ring_type < RT_MAX);

	/* has to be zeroed or memcmp will fail */
    nring = g_malloc0(sizeof(ring));
    assert(nring != NULL);

    nring->type = ring_type;
    nring->bonus = bonus;

    if (ring_get_effect(nring))
    {
    	nring->effect = effect_new(ring_get_effect(nring), 0);
    	/* this effect is permanent */
    	nring->effect->turns = 0;

    	if (bonus)
            nring->effect->amount += bonus;

    	/* ring of extra regeneration is better than the average */
    	if (ring_get_effect(nring) == RT_EXTRA_REGEN)
            nring->effect->amount *= 5;
    }

    return nring;
}

void ring_destroy(ring *r)
{
    assert (r != NULL);
	if (r->effect != NULL)
		effect_destroy(r->effect);

    g_free(r);
}

inline char *ring_get_name(ring *r)
{
    assert(r != NULL && r->type > RT_NONE && r->type < RT_MAX);
    return rings[r->type].name;
}

inline int ring_get_effect(ring *r)
{
    assert(r != NULL && r->type > RT_NONE && r->type < RT_MAX);
    return rings[r->type].effect_type;
}

inline item_material_t ring_get_material(ring *r)
{
    assert(r != NULL && r->type > RT_NONE && r->type < RT_MAX);
    return ring_material[ring_material_mapping[r->type - 1]];
}

inline int ring_get_price(ring *r)
{
    assert(r != NULL && r->type > RT_NONE && r->type < RT_MAX);
    return rings[r->type].price;
}

inline int ring_is_observable(ring *r)
{
    assert(r != NULL && r->type > RT_NONE && r->type < RT_MAX);
    return rings[r->type].observable;
}

int ring_bless(ring *r)
{
    if (r->blessed)
        return FALSE;

    r->blessed = 1;
    r->cursed = 0;

    return TRUE;
}

int ring_curse(ring *r)
{
    if (r->cursed)
        return FALSE;

    r->blessed = 0;
    r->cursed = 1;

    return TRUE;
}
