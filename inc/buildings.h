/*
 * buildings.h
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#ifndef BUILDINGS_H
#define BUILDINGS_H

/* number of courses available in school */
#define SCHOOL_COURSE_COUNT 7

/* forward declarations */

struct player;
struct game;

/* functions */
void building_bank_calc_interest(struct game *g);
int building_bank(struct player *p);
int building_dndstore(struct player *p);
void building_dndstore_init();
int building_home(struct player *p);
int building_lrs(struct player *p);
int building_school(struct player *p);
int building_tradepost(struct player *p);
int building_monastery(struct player *p);
void building_monastery_init();

#endif
