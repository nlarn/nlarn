/*
 * armour.c
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

#include "armour.h"
#include "items.h"

const armour_data armours[AT_MAX] =
{
    /* id            name                      ac  category   material    effect      we     pr disguise  un*/
    { AT_NONE,       NULL,                      0, AC_NONE,   IM_NONE,    ET_NONE,     0,     0, AT_NONE,   1 },
    { AT_CLOAK,      "cloak",                   1, AC_CLOAK,  IM_CLOTH,   ET_NONE,   400,    15, AT_NONE,   0 },
    { AT_LGLOVES,    "pair of leather gloves",  1, AC_GLOVES, IM_LEATHER, ET_NONE,   800,    25, AT_NONE,   0 },
    { AT_LBOOTS,     "pair of leather boots",   1, AC_BOOTS,  IM_LEATHER, ET_NONE,  1800,    25, AT_NONE,   0 },
    { AT_LHELMET,    "leather helmet",          1, AC_HELMET, IM_LEATHER, ET_NONE,  2200,    25, AT_NONE,   0 },
    { AT_LEATHER,    "leather armour",          2, AC_SUIT,   IM_LEATHER, ET_NONE,  4000,    40, AT_NONE,   0 },
    { AT_WSHIELD,    "wooden shield",           1, AC_SHIELD, IM_WOOD,    ET_NONE,  3500,    25, AT_NONE,   0 },
    { AT_SLEATHER,   "studded leather armour",  3, AC_SUIT,   IM_LEATHER, ET_NONE,  7500,    80, AT_NONE,   0 },
    { AT_RINGMAIL,   "ring mail",               5, AC_SUIT,   IM_IRON,    ET_NONE, 10000,   320, AT_NONE,   0 },
    { AT_LSHIELD,    "large shield",            2, AC_SHIELD, IM_IRON,    ET_NONE,  7800,   125, AT_NONE,   0 },
    { AT_CHAINHOOD,  "chainmail hood",          2, AC_HELMET, IM_IRON,    ET_NONE,  3400,   180, AT_NONE,   0 },
    { AT_CHAINMAIL,  "chain mail",              6, AC_SUIT,   IM_IRON,    ET_NONE, 11500,   600, AT_NONE,   0 },
    { AT_SPLINTMAIL, "splint mail",             7, AC_SUIT,   IM_IRON,    ET_NONE, 13000,  1000, AT_NONE,   0 },
    { AT_PHELMET,    "plate helmet",            3, AC_HELMET, IM_IRON,    ET_NONE,  2800,   450, AT_NONE,   0 },
    { AT_PBOOTS,     "pair of plate boots",     3, AC_BOOTS,  IM_IRON,    ET_NONE,  3400,   450, AT_NONE,   0 },
    { AT_SPEEDBOOTS, "pair of boots of speed",  1, AC_BOOTS,  IM_LEATHER, ET_SPEED,  800,  2800, AT_LBOOTS, 1 },
    { AT_PLATEMAIL,  "plate mail",              8, AC_SUIT,   IM_IRON,    ET_NONE, 17500,  2200, AT_NONE,   0 },
    { AT_SSHIELD,    "stainless shield",        4, AC_SHIELD, IM_STEEL,   ET_NONE,  8000,   880, AT_NONE,   0 },
    { AT_SPLATEMAIL, "stainless plate mail",    9, AC_SUIT,   IM_STEEL,   ET_NONE, 19000,  3800, AT_NONE,   0 },
    { AT_INVISCLOAK, "cloak of invisibility",   1, AC_CLOAK,  IM_CLOTH,   ET_INVISIBILITY, 400,2800, AT_CLOAK, 1 },
    { AT_ELVENCHAIN, "elven chain",            12, AC_SUIT,   IM_MITHRIL, ET_NONE,  8500, 16400, AT_NONE,   1 },
};
