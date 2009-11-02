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

#ifndef __NLARN_H_
#define __NLARN_H_

#include "game.h"

#define VERSION_MAJOR 0 /* this is the present version # of the program */
#define VERSION_MINOR 5
#define VERSION_PATCH 0

#define TAXRATE 1/20    /* tax rate for the LRS */

/* the entire game */
game *nlarn;

#endif
