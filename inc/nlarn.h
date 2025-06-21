/*
 * nlarn.h
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

#include <setjmp.h>

#include "game.h"

#define VERSION_MAJOR 0 /* this is the present version # of the program */
#define VERSION_MINOR 7
#define VERSION_PATCH 8

/* this allows to add the id of the current commit to the version information */
#ifndef GITREV
#define GITREV ""
#endif

/* game version string */
const char *nlarn_version;

/* the entire game */
game *nlarn;

/* death jump buffer - used to return to the main loop when the player has died */
extern jmp_buf nlarn_death_jump;

/* file paths */
const char *nlarn_libdir;
const char *nlarn_mesgfile;
const char *nlarn_helpfile;
const char *nlarn_mazefile;
const char *nlarn_fortunes;
const char *nlarn_highscores;
const char *nlarn_inifile;
const char *nlarn_savefile;

#endif
