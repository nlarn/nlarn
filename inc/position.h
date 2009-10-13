/*
 * position.h
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

#ifndef __POSITION_H_
#define __POSITION_H_

#include <glib.h>

#include "cJSON.h"
#include "defines.h"

typedef struct _position
{
    gint16 x;
    gint16 y;
    gint16 z;
} position;

typedef struct _rectangle
{
    gint16 x1;
    gint16 y1;
    gint16 x2;
    gint16 y2;
} rectangle;

typedef struct area
{
    gint16 start_x;
    gint16 start_y;
    gint16 size_x;
    gint16 size_y;
    int **area;
} area;

position pos_new(int x, int y, int z);
position pos_move(position pos, direction dir);
int pos_distance(position first, position second);
int pos_identical(position pos1, position pos2);
int pos_adjacent(position first, position second);
int pos_valid(position pos);
cJSON *pos_serialize(position pos);

rectangle rect_new(int x1, int y1, int x2, int y2);
rectangle rect_new_sized(position center, int size);
int pos_in_rect(position pos, rectangle rect);

area *area_new(int start_x, int start_y, int size_x, int size_y);
area *area_new_circle(position center, int radius);
area *area_new_circle_flooded(position center, int radius, area *obstacles);
area *area_new_ray(position source, position target, area *obstacles);
void area_destroy(area *area);

#define area_size(area)         ((area)->size_x * (area)->size_y)

#define area_point_set(a, x, y) ((a)->area[((y) > (a)->size_y - 1) ? \
                                (a)->size_y - 1 : (y)][((x) > (a)->size_x - 1) ? \
                                (a)->size_x - 1 : (x)] = TRUE)

#define area_point_get(a, x, y) ((a)->area[(y)][(x)])
#define area_point_del(a, x, y) ((a)->area[(y)][(x)] = FALSE)

#define area_pos_set(area, pos) ((area)->area[(pos).y][(pos).x] = TRUE)
#define area_pos_get(area, pos) ((area)->area[(pos).y][(pos).x])
#define area_pos_del(area, pos) ((area)->area[(pos).y][(pos).x] = FALSE)

#endif
