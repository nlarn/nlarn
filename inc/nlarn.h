/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * nlarn.h
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

#ifndef __NULARN_H_
#define __NULARN_H_

#include <assert.h>
#include <curses.h>
#include <ctype.h>
#include <glib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "armour.h"
#include "container.h"
#include "display.h"
#include "effects.h"
#include "food.h"
#include "game.h"
#include "gems.h"
#include "items.h"
#include "level.h"
#include "monsters.h"
#include "player.h"
#include "buildings.h"
#include "position.h"
#include "potions.h"
#include "rings.h"
#include "scrolls.h"
#include "spells.h"
#include "spheres.h"
#include "traps.h"
#include "weapons.h"


#define MAJOR_VERSION 0	/* this is the present version # of the program */
#define MINOR_VERSION 2
#define PATCH_LEVEL   0

#define TIMELIMIT 30000 /* maximum number of moves before the game is called */
#define TAXRATE 1/20    /* tax rate for the LRS */

#endif
