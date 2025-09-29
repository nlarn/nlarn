/*
 * fov.h
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

#ifndef FOV_H
#define FOV_H

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
  * @return A new FOV data structure.
  */
fov *fov_new();

/** @brief calculate the FOV for a map
  *
  * @param fv pointer to a fov structure.
  * @param m the map
  * @param pos the starting position
  * @param radius the radius of vision
  * @param infravision true if the mobile has infravision.
  */
void fov_calculate(fov *fv, map *m, position pos, int radius, gboolean infravision);

/** @brief check if a certain position is visible.
  *
  * @param fv pointer to a fov structure.
  * @param pos a position.
  *
  * @return true/false
  */
gboolean fov_get(fov *fv, position pos);

/** @brief set visibility for a certain position.
  *
  * @param fv pointer to a fov structure.
  * @param pos a position.
  * @param visible the visibility of the position.
  * @param infravision Beholder has infravision?
  * @param mchk Check for monsters at the position?
  */
void fov_set(fov *fv, position pos, guchar visible,
             gboolean infravision, gboolean mchk);

/** @brief reset visibility for an entire fov structure.
  *
  * @param fv pointer to a fov structure.
  */
void fov_reset(fov *fv);

/** @brief Get the closest monster for a field of vision.
  *
  * @param fv pointer to a fov structure
  * @return a pointer to the closest monster, or NULL
  */
monster *fov_get_closest_monster(fov *fv);

/** @brief Get a list of all visible monsters
  *
  * @param fv A pointer to a fov structure
  * @return A GList with all visible monsters, sorted by proximity, or NULL
  */
GList *fov_get_visible_monsters(fov *fv);

/** @brief destroy fov data
  *
  * @param fv A pointer to a fov structure.
  */
void fov_free(fov *fv);

#endif
