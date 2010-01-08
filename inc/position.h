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
position pos_deserialize(cJSON *pser);

rectangle rect_new(int x1, int y1, int x2, int y2);
rectangle rect_new_sized(position center, int size);
int pos_in_rect(position pos, rectangle rect);

area *area_new(int start_x, int start_y, int size_x, int size_y);
area *area_new_circle(position center, int radius, int hollow);
area *area_new_circle_flooded(position center, int radius, area *obstacles);
area *area_new_ray(position source, position target, area *obstacles);
area *area_copy(area *a);
void area_destroy(area *a);

area *area_add(area *a, area *b);
area *area_flood(area *obstacles, int start_x, int start_y);

void area_point_set(area *a, int x, int y);
int  area_point_get(area *a, int x, int y);
void area_point_del(area *a, int x, int y);
int area_point_valid(area *a, int x, int y);

void area_pos_set(area *a, position pos);
int  area_pos_get(area *a, position pos);
void area_pos_del(area *a, position pos);

#endif
