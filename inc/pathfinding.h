/*
 * pathfinding.h
 * Copyright (C) 2009-2020 Joachim de Groot <jdegroot@web.de>
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

#include <glib.h>

#include "map.h"

/* Structure for path elements */
typedef struct path_element
{
    position pos;
    guint32 g_score;
    guint32 h_score;
    struct path_element* parent;
} path_element;

typedef struct path
{
    GQueue *path;
    GPtrArray *closed;
    GPtrArray *open;
    position start;
    position goal;
} path;

/**
 * @brief Find a path between two positions
 *
 * @param the map to work on
 * @param the starting position
 * @param the destination
 * @param the map_element_t that can be travelled
 * @return a path or NULL if none could be found
 */
path *path_find(map *m, position start, position goal,
                map_element_t element);

/**
 * @brief Free memory allocated for a given path.
 *
 * @param a path returned by <find_path>"()"
 */
void path_destroy(path *path);
