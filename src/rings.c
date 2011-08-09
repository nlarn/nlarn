/*
 * rings.c
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

/* $Id$ */

#include <glib.h>

#include "nlarn.h"
#include "rings.h"

const ring_data rings[RT_MAX] =
{
    /* type            name                  effect           price ob bo */
    { RT_NONE,         "",                   ET_NONE,            0, 0, 0 },
    { RT_REGENERATION, "regeneration",       ET_INC_HP_REGEN,  250, 0, 0 },
    { RT_PROTECTION,   "protection",         ET_PROTECTION,    150, 0, 1 },
    { RT_ENERGY,       "energy",             ET_INC_MP_REGEN,  250, 0, 0 },
    { RT_DEXTERITY,    "dexterity",          ET_INC_DEX,       110, 1, 1 },
    { RT_STRENGTH,     "strength",           ET_INC_STR,       110, 1, 1 },
    { RT_CLEVERNESS,   "cleverness",         ET_INC_INT,       110, 1, 1 },
    { RT_INC_DAMAGE,   "increase damage",    ET_INC_DAMAGE,    150, 0, 0 },
    { RT_EXTRA_REGEN,  "extra regeneration", ET_INC_HP_REGEN, 1000, 0, 0 },
};

static const int ring_materials[RT_MAX - 1] =
{
    IM_GOLD,
    IM_SILVER,
    IM_PLATINUM,
    IM_GEMSTONE,
    IM_COPPER,
    IM_STEEL,
    IM_GLASS,
    IM_BONE
};

item_material_t ring_material(int ring_id)
{
    g_assert(ring_id > RT_NONE && ring_id < RT_MAX);
    return ring_materials[nlarn->ring_material_mapping[ring_id - 1]];
}
