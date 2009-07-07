/*
 * armour.c
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

const armour_data armours[AT_MAX] =
{
    /* id            name                      ac  category     material       we   pr  un */
    { AT_NONE,       "",                        0, AC_NONE,     IM_NONE,        0,    0, 0 },
    { AT_SHIELD,     "shield",                  2, AC_SHIELD,   IM_WOOD,     3500,   15, 0 },
    { AT_LEATHER,    "leather armour",          2, AC_SUIT,     IM_LEATHER,  4000,    2, 0 },
    { AT_SLEATHER,   "studded leather armour",  3, AC_SUIT,     IM_LEATHER,  7500,   10, 0 },
    { AT_RINGMAIL,   "ring mail",               5, AC_SUIT,     IM_IRON,    10000,   40, 0 },
    { AT_CHAINMAIL,  "chain mail",              6, AC_SUIT,     IM_IRON,    11500,   85, 0 },
    { AT_SPLINTMAIL, "splint mail",             7, AC_SUIT,     IM_IRON,    13000,  220, 0 },
    { AT_PLATEMAIL,  "plate mail",              9, AC_SUIT,     IM_IRON,    17500,  400, 0 },
    { AT_PLATE,      "plate armour",           10, AC_SUIT,     IM_IRON,    20000,  900, 0 },
    { AT_SPLATE,     "stainless plate armour", 12, AC_SUIT,     IM_STEEL,   20000, 2600, 0 },
    { AT_ELVENCHAIN, "elven chain",            15, AC_SUIT,     IM_MITHRIL,  7500,    0, 0 },
};
