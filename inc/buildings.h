/*
 * buildings.h
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

#ifndef __BUILDINGS_H_
#define __BUILDINGS_H_

#include "player.h"

typedef struct school_course {
    int course_time;
    char *description;
    char *message;
} school_course;

int building_bank(player *p);
int building_dndstore(player *p);
int building_home(player *p);
int building_lrs(player *p);
int building_school(player *p);
int building_tradepost(player *p);

#endif
