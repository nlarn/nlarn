/*
 * position.h
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

#ifndef __POSITION_H_
#define __POSITION_H_

#include <glib.h>

#include "colours.h"
#include "combat.h"

/* direction of movement */
/* ordered by number keys */
typedef enum _direction
{
    GD_NONE,
    GD_SW,
    GD_SOUTH,
    GD_SE,
    GD_WEST,
    GD_CURR, /* special case: current position */
    GD_EAST,
    GD_NW,
    GD_NORTH,
    GD_NE,
    GD_MAX
} direction;

typedef union _position
{
    struct _bf
    {
        gint32 x: 12;
        gint32 y: 12;
        guint32 z: 8;
    } bf;
    guint32 val;
} position;

typedef struct _rectangle
{
    guint64 x1: 16;
    guint64 y1: 16;
    guint64 x2: 16;
    guint64 y2: 16;
} rectangle;

typedef struct _area
{
    gint16 start_x;
    gint16 start_y;
    gint16 size_x;
    gint16 size_y;
    int **area;
} area;

#define X(pos) ((pos).bf.x)
#define Y(pos) ((pos).bf.y)
#define Z(pos) ((pos).bf.z)
#define pos_val(pos) ((pos).val)

position pos_move(position pos, direction dir);

/**
 * Return the direction of a position relative to a given position.
 *
 * @param here the current position
 * @param there the other position
 *
 * @return the direction
 */
direction pos_direction(position here, position there);

int pos_distance(position first, position second);

static inline int pos_identical(const position pos1, const position pos2)
{
    return (pos1.val == pos2.val);
}

int pos_adjacent(position first, position second);
int pos_valid(position pos);

/**
 * @brief Determine the direction of a position relative to another position.
 * @param origin The source position.
 * @param target The target position.
 */
direction pos_dir(position origin, position target);

/**
 * Create a new rectangle of given dimensions.
 * Does a sanity check and replaces out-of-bounds values.
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 *
 * @return corrected area
 */
rectangle rect_new(int x1, int y1, int x2, int y2);

static inline rectangle rect_new_sized(const position center, const int size)
{
    return rect_new(X(center) - size,
                    Y(center) - size,
                    X(center) + size,
                    Y(center) + size);
}

int pos_in_rect(position pos, rectangle rect);

area *area_new(int start_x, int start_y, int size_x, int size_y);

/**
 * Draw a circle: Midpoint circle algorithm
 * from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 *
 * @param center The center of the circle
 * @param radius The radius of the circle
 * @param hollow true if the circle shall not be filled
 * @return a new area.
 */
area *area_new_circle(position center, guint radius, gboolean hollow);

/**
 * Draw a circle with every unobstructed point inside it set.
 *
 * @param center The center of the circle
 * @param radius The radius of the circle
 * @param obstacles An area with every obstructed point set.
 * @return a new area.
 */
area *area_new_circle_flooded(position center, guint radius, area *obstacles);

/* callback function for blasts */
typedef gboolean (*area_hit_sth)(position pos, const damage_originator *damo,
                                 gpointer data1, gpointer data2);

/**
 * Affect an area by a blast.
 *
 * @param center The center of the blast position.
 * @param radius The affected radius.
 * @param damo The originator of the blast.
 * @param pos_hitfun The callback function for every affected position.
 * @param data1 A pointer passed to the callback function.
 * @param data2 A pointer passed to the callback function.
 * @param glyph The glyph to display at an affected position
 * @param fg The colour of the glyph.
 *
 * @return true if one of the callbacks returned true.
 */
gboolean area_blast(position center, guint radius,
                    const damage_originator *damo,
                    area_hit_sth pos_hitfun,
                    gpointer data1, gpointer data2,
                    char glyph, colour fg);

/**
 * @brief Destroy a given area
 *
 * @param a An area.
 */
void area_destroy(area *a);

/**
 * Add one area to another.
 *
 * @param a first area (will be returned)
 * @param b second area (will be freed)
 * @return first area with additional set point of second area
 */
area *area_add(area *a, area *b);

/**
 * Flood fill an area from a given starting point
 *
 * @param obstacles An area which marks the points which shall not be flooded
 *     (will be freed)
 * @param start_x starting x
 * @param start_y starting y
 * @return an area with all reached points set (newly allocated, must be freed)
 */
area *area_flood(area *obstacles, int start_x, int start_y);

void area_point_set(area *a, int x, int y);
int  area_point_get(area *a, int x, int y);

static inline int area_point_valid(const area *a, const int x, const int y)
{
    return ((x < a->size_x) && (x >= 0)) && ((y < a->size_y) && (y >= 0));
}

int  area_pos_get(area *a, position pos);

#endif
