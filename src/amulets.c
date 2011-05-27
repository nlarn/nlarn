/*
 * amulets.c
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

#include <assert.h>
#include <glib.h>
#include "amulets.h"
#include "items.h"
#include "nlarn.h"

const amulet_data amulets[AM_MAX] =
{
    { AM_NONE,                NULL,               ET_NONE,              0,    },
    { AM_AWARENESS,           "awareness",        ET_AWARENESS,         400,  },
    { AM_INVISIBILITY,        "invisibility",     ET_INVISIBILITY,      400,  },
    { AM_UNDEAD_CONTROL,      "undead control",   ET_UNDEAD_PROTECTION, 5000, },
    { AM_NEGATE_SPIRIT,       "negate spirit",    ET_SPIRIT_PROTECTION, 5000, },
    { AM_NOTHEFT,             "theft prevention", ET_NOTHEFT,           6000, },
    { AM_DRAGON_SLAYING,      "dragon slaying",   ET_NONE,              6000, },
    { AM_POWER,               "power",            ET_NONE,              8000, },
    { AM_REFLECTION,          "reflection",       ET_REFLECTION,        8000, },
    { AM_LARN,                "larn",             ET_INFRAVISION,       9000, },
};

static const int amulet_materials[AM_MAX - 1] =
{
    IM_GOLD,
    IM_SILVER,
    IM_PLATINUM,
    IM_SILVER,
    IM_COPPER,
    IM_STEEL,
    IM_GLASS,
    IM_BONE,
    IM_GEMSTONE,
};

item_material_t amulet_material(int amulet_id)
{
    assert(amulet_id > AM_NONE && amulet_id < AM_MAX);
    return amulet_materials[nlarn->amulet_material_mapping[amulet_id - 1]];
}

