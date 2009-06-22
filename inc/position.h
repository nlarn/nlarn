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

typedef enum fill_type
{
    FILL_NONE,
    FILL_SOLID, /* every */
    FILL_FLOOD, /* pour like water */
    FILL_BLAST, /* like fov */
    FILL_MAX
} fill_t;

typedef struct position
{
    guint32
        x: 16,
        y: 16;
} position;

typedef struct rectangle
{
    guint32
        x1: 8,
        y1: 8,
        x2: 8,
        y2: 8;
} rectangle;

typedef struct area
{
    int start_x;
    int start_y;
    int size_x;
    int size_y;
    int **area;
} area;

position pos_new(int x, int y);
position pos_move(position pos, int direction);

rectangle rect_new(int x1, int y1, int x2, int y2);
rectangle rect_new_sized(position center, int size);
int pos_in_rect(position pos, rectangle rect);

area *area_new(int start_x, int start_y, int size_x, int size_y);
area *area_new_circle(position center, int radius, fill_t filling, area *obstacles);
void area_destroy(area *area);

#define pos_distance(first, second) (abs((first).x - (second).x) \
                                    + abs((first).y - (second).y))

#define pos_identical(pos1,pos2) (((pos1).x == (pos2).x) && ((pos1).y == (pos2).y))

#define pos_valid(pos) (((pos).x >= 0) \
                        && ((pos).x < LEVEL_MAX_X) \
                        && ((pos).x != G_MAXUINT16) \
                        && ((pos).y >= 0) \
                        && ((pos).y < LEVEL_MAX_Y) \
                        && ((pos).y != G_MAXUINT16))

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
