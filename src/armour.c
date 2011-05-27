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

/* $Id$ */

#include "armour.h"
#include "items.h"

const armour_data armours[AT_MAX] =
{
    /* id            name                      ac  category         material       we    pr */
    { AT_NONE,       "",                        0, ARMOUR_NONE,     IM_NONE,        0,     0 },
    { AT_LGLOVES,    "pair of leather gloves",  1, ARMOUR_GLOVES,   IM_LEATHER,   800,    15 },
    { AT_LBOOTS,     "pair of leather boots",   1, ARMOUR_BOOTS,    IM_LEATHER,  1800,    15 },
    { AT_LHELMET,    "leather helmet",          1, ARMOUR_HELMET,   IM_LEATHER,  2200,    15 },
    { AT_LEATHER,    "leather armour",          2, ARMOUR_SUIT,     IM_LEATHER,  4000,    40 },
    { AT_WSHIELD,    "wooden shield",           1, ARMOUR_SHIELD,   IM_WOOD,     3500,    20 },
    { AT_SLEATHER,   "studded leather armour",  3, ARMOUR_SUIT,     IM_LEATHER,  7500,    80 },
    { AT_RINGMAIL,   "ring mail",               5, ARMOUR_SUIT,     IM_IRON,    10000,   320 },
    { AT_LSHIELD,    "large shield",            2, ARMOUR_SHIELD,   IM_IRON,     7800,   125 },
    { AT_CHAINHOOD,  "chainmail hood",          2, ARMOUR_HELMET,   IM_IRON,     3400,   180 },
    { AT_CHAINMAIL,  "chain mail",              6, ARMOUR_SUIT,     IM_IRON,    11500,   600 },
    { AT_SPLINTMAIL, "splint mail",             7, ARMOUR_SUIT,     IM_IRON,    13000,  1000 },
    { AT_PHELMET,    "plate helmet",            3, ARMOUR_HELMET,   IM_IRON,     2800,   450 },
    { AT_PBOOTS,     "pair of plate boots",     3, ARMOUR_BOOTS,    IM_IRON,     3400,   450 },
    { AT_PLATEMAIL,  "plate mail",              8, ARMOUR_SUIT,     IM_IRON,    17500,  2200 },
    { AT_SSHIELD,    "stainless shield",        4, ARMOUR_SHIELD,   IM_STEEL,    8000,   880 },
    { AT_SPLATEMAIL, "stainless plate mail",    9, ARMOUR_SUIT,     IM_STEEL,   19000,  3800 },
    { AT_ELVENCHAIN, "elven chain",            16, ARMOUR_SUIT,     IM_MITHRIL,  7500, 16400 },
};
