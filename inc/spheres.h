/*
 * spheres.h
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

#ifndef __SPHERES_H_
#define __SPHERES_H_

#include "level.h"
#include "player.h"

typedef struct sphere {
    position pos;       /* location of the sphere */
    direction dir;      /* direction sphere is going in */
    guint32 lifetime;   /* duration of the sphere */
    player *owner;      /* pointer to player who created the sphere */
} sphere;

/* function declarations */
sphere *sphere_new(position pos, player *owner, int lifetime);
void sphere_destroy(sphere *s);

void sphere_move(sphere *s, level *l);
sphere *sphere_at(level *l, position pos);

#endif
