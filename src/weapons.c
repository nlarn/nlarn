/*
 * weapons.c
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

#include "items.h"
#include "weapons.h"

const weapon_data weapons[WT_MAX] =
{
    /* ID               name                         wc  material    we     pr tw un ob */
    { WT_NONE,          "",                           0, IM_NONE,     0,     0, 0, 0, 0 },
    { WT_ODAGGER,       "orcish dagger",              2, IM_IRON,   850,     6, 0, 0, 0 },
    { WT_DAGGER,        "dagger",                     3, IM_IRON,   600,    10, 0, 0, 1 },
    { WT_OSHORTSWORD,   "orcish short sword",         5, IM_IRON,  1350,    30, 0, 0, 0 },
    { WT_SHORTSWORD,    "short sword",                6, IM_IRON,  1000,    60, 0, 0, 1 },
    { WT_ESHORTSWORD,   "elven short sword",          7, IM_WOOD,  1180,    68, 0, 0, 0 },
    { WT_OSPEAR,        "orcish spear",               8, IM_IRON,  2100,    75, 0, 0, 0 },
    { WT_SPEAR,         "spear",                     10, IM_IRON,  1800,   120, 0, 0, 1 },
    { WT_ESPEAR,        "elven spear",               11, IM_WOOD,  1600,   140, 0, 0, 0 },
    { WT_MACE,          "mace",                      12, IM_IRON,  2600,   160, 0, 0, 1 },
    { WT_FLAIL,         "flail",                     14, IM_IRON,  2900,   195, 1, 0, 1 },
    { WT_BATTLEAXE,     "battle-axe",                18, IM_IRON,  2700,   350, 1, 0, 1 },
    { WT_LONGSWORD,     "longsword",                 22, IM_IRON,  1950,   550, 0, 0, 1 },
    { WT_2SWORD,        "two-handed sword",          26, IM_IRON,  3600,  1000, 1, 0, 1 },
    { WT_SWORDSLASHING, "sword of slashing",         30, IM_IRON,  2200,  6000, 0, 0, 1 },
    /* unique weapons */
    { WT_LANCEOFDEATH,  "lance of death",            20, IM_WOOD,  2900, 16500, 1, 1, 1 },
    { WT_VORPALBLADE,   "Vorpal blade",              22, IM_STEEL, 1600,  3800, 0, 1, 1 },
    { WT_SLAYER,        "Slayer",                    30, IM_STEEL, 1800,  6800, 0, 1, 1 },
    { WT_SUNSWORD,      "Sunsword",                  32, IM_STEEL, 1800,  7000, 0, 1, 1 },
    { WT_BESSMAN,       "Bessman's flailing hammer", 35, IM_STEEL, 5800, 10000, 1, 1, 1 },
};
