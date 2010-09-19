/*
 * weapons.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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
    /* ID               name                         short name,        wc  acc  material    we     pr tw un ar ob */
    { WT_NONE,          "",                          "",		         0,   0, IM_NONE,     0,     0, 0, 0, 0, 0 },
    { WT_ODAGGER,       "orcish dagger",             "orc dagger",       2,   2, IM_IRON,   850,     6, 0, 0, 1, 0 },
    { WT_DAGGER,        "dagger",                    "dagger",           3,   3, IM_IRON,   600,    10, 0, 0, 1, 1 },
    { WT_OSHORTSWORD,   "orcish short sword",        "orc short sword",  5,   1, IM_IRON,  1350,    30, 0, 0, 1, 0 },
    { WT_SHORTSWORD,    "short sword",               "short sword",      6,   2, IM_IRON,  1000,    60, 0, 0, 1, 1 },
    { WT_ESHORTSWORD,   "elven short sword",         "elf short sword",  7,   2, IM_WOOD,  1180,    68, 0, 0, 1, 0 },
    { WT_OSPEAR,        "orcish spear",              "orc spear",        8,   2, IM_IRON,  2100,    75, 0, 0, 1, 0 },
    { WT_SPEAR,         "spear",                     "spear",           10,   2, IM_IRON,  1800,   120, 0, 0, 1, 1 },
    { WT_ESPEAR,        "elven spear",               "elf spear",       11,   3, IM_WOOD,  1600,   140, 0, 0, 1, 0 },
    { WT_MACE,          "mace",                      "mace",            12,   1, IM_IRON,  2600,   160, 0, 0, 1, 1 },
    { WT_FLAIL,         "flail",                     "flail",           14,   1, IM_IRON,  2900,   195, 1, 0, 1, 1 },
    { WT_BATTLEAXE,     "battle-axe",                "battle-axe",      18,   2, IM_IRON,  2700,   350, 1, 0, 1, 1 },
    { WT_LONGSWORD,     "longsword",                 "longsword",       22,   3, IM_IRON,  1950,   550, 0, 0, 1, 1 },
    { WT_2SWORD,        "two-handed sword",          "2-handed sword",  26,   4, IM_IRON,  3600,  1000, 1, 0, 1, 1 },
    { WT_SWORDSLASHING, "sword of slashing",         "slashing",        30,   5, IM_IRON,  2200,  6000, 0, 0, 1, 1 },
    /* unique weapons */
    { WT_LANCEOFDEATH,  "lance of death",            "lance of death",  20,   3, IM_WOOD,  2900, 65000, 1, 1, 1, 1 },
    { WT_VORPALBLADE,   "Vorpal blade",              "Vorpal blade",    22,   3, IM_STEEL, 1600,  3800, 0, 1, 1, 1 },
    { WT_SLAYER,        "Slayer",                    "Slayer",          30,   5, IM_STEEL, 1800,  6800, 0, 1, 0, 1 },
    { WT_SUNSWORD,      "Sunsword",                  "Sunsword",        32,   6, IM_STEEL, 1800,  7000, 0, 1, 1, 1 },
    { WT_BESSMAN,       "Bessman's flailing hammer", "Bessman's",       35,   6, IM_STEEL, 5800, 10000, 1, 1, 0, 1 },
};
