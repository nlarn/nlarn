/*
 * position.h
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

#ifndef __POSITION_H_
#define __POSITION_H_

#include <glib.h>

#include "cJSON.h"
#include "defines.h"

typedef union _position
{
    struct _bf
    {
        unsigned int x: 12;
        unsigned int y: 12;
        unsigned int z:  8;
    } bf;
    unsigned int val;
} position;

const position pos_invalid;

typedef struct _rectangle
{
    guint64 x1: 16;
    guint64 y1: 16;
    guint64 x2: 16;
    guint64 y2: 16;
} rectangle;

typedef struct area
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
int pos_distance(position first, position second);
int pos_identical(position pos1, position pos2);
int pos_adjacent(position first, position second);
int pos_valid(position pos);

/**
 * @brief Determine the direction of a position relative to another position.
 * @param The source position.
 * @param The target position.
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

rectangle rect_new_sized(position center, int size);
int pos_in_rect(position pos, rectangle rect);

area *area_new(int start_x, int start_y, int size_x, int size_y);

/**
 * Draw a circle: Midpoint circle algorithm
 * from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 *
 * @param center point of the circle
 * @param radius of the circle
 * @return a new area.
 */
area *area_new_circle(position center, int radius, int hollow);

area *area_new_circle_flooded(position center, int radius, area *obstacles);
area *area_new_ray(position source, position target, area *obstacles);

/**
 *
 * Create a new area with the dimensions of the given one
 *
 * @param an area
 * @return a new, identical area
 */
area *area_copy(area *a);

void area_destroy(area *a);

/**
 * Add one area to another.
 *
 * @param first area (will be returned)
 * @param second area (will be freed)
 * @return first area with additional set point of second area
 */
area *area_add(area *a, area *b);

/**
 * Flood fill an area from a given starting point
 *
 * @param an area which marks the points which shall not be flooded (will be freed)
 * @param starting x
 * @param starting y
 * @return an area with all reached points set (newly allocated, must be freed)
 */
area *area_flood(area *obstacles, int start_x, int start_y);

void area_point_set(area *a, int x, int y);
int  area_point_get(area *a, int x, int y);
void area_point_del(area *a, int x, int y);
int area_point_valid(area *a, int x, int y);

void area_pos_set(area *a, position pos);
int  area_pos_get(area *a, position pos);
void area_pos_del(area *a, position pos);

#endif
