/*
 * amulets.c
 * Copyright (C) 2009-2026 Joachim de Groot <jdegroot@web.de>
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
#include <glib/gi18n.h>

#include "amulets.h"
#include "items.h"
#include "extdefs.h"

DEFINE_ENUM(amulet_t, AMULET_TYPE_ENUM)

const amulet_data amulets[AM_MAX] =
{
    { AM_AWARENESS,           AMULET,   NC_("amulet", "awareness"),        ET_AWARENESS,         400,  },
    { AM_SUSTAINMENT,         AMULET,   NC_("amulet", "sustainment"),      ET_SUSTAINMENT,       400,  },
    { AM_UNDEAD_CONTROL,      TALISMAN, NC_("amulet", "undead control"),   ET_UNDEAD_PROTECTION, 5000, },
    { AM_NEGATE_SPIRIT,       TALISMAN, NC_("amulet", "negate spirit"),    ET_SPIRIT_PROTECTION, 5000, },
    { AM_NOTHEFT,             TALISMAN, NC_("amulet", "theft prevention"), ET_NOTHEFT,           6000, },
    { AM_DRAGON_SLAYING,      TALISMAN, NC_("amulet", "dragon slaying"),   ET_NONE,              6000, },
    { AM_POWER,               AMULET,   NC_("amulet", "power"),            ET_NONE,              8000, },
    { AM_REFLECTION,          AMULET,   NC_("amulet", "reflection"),       ET_REFLECTION,        8000, },
    { AM_LARN,                AMULET,   NC_("amulet", "Eye of Larn"),      ET_INFRAVISION,       9000, },
};

static const int amulet_materials[AM_MAX] =
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

item_material_t amulet_material(amulet_t amulet_id)
{
    g_assert(amulet_id < AM_MAX);
    return amulet_materials[nlarn->amulet_material_mapping[amulet_id]];
}
