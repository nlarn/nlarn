/*
 * fov.h
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

#ifndef __FOV_H_
#define __FOV_H_

#include <glib.h>
#include "map.h"
#include "monsters.h"
#include "position.h"


/* the members of this struct are only known to the implementation of the
   following functions */
struct _fov;
typedef struct _fov fov;


/** @brief Create a FOV data structure
  *
  * @param X size
  * @param Y size
  */
fov *fov_new(guint size_x, guint size_y);

/** @brief calculate the FOV for a map
  *
  * @param pointer to a fov structure.
  * @param the map
  * @param the starting position
  * @param the radius of vision
  * @param True if the mobile has infravision.
  */
void fov_calculate(fov *fov, map *m, position pos, int radius, gboolean infravision);

/** @brief check if a certain position is visible.
  *
  * @param pointer to a fov structure.
  * @param a position.
  *
  * @return TRUE/FALSE
  */
gboolean fov_get(fov *fov, position pos);

/** @brief set visibility for a certain position.
  *
  * @param pointer to a fov structure.
  * @param a position.
  */
void fov_set(fov *fov, position pos, gboolean visible);

/** @brief reset visibility for an entire fov structure.
  *
  * @param pointer to a fov structure.
  */
void fov_reset(fov *fov);

/** @brief Get the closest monster for a field of vision.
  *
  * @param pointer to a fov structure
  */
monster *fov_get_closest_monster(fov *fov);

/** @brief destroy fov data
  *
  * @param A pointer to a fov structure.
  * @param The starting position.
  */
void fov_free(fov *fov);

#endif
