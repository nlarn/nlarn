/*
 * position.c
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

#include "nlarn.h"

position pos_new(int x, int y)
{
    position pos;

    assert((x >= 0 && x <= LEVEL_MAX_X) || x == G_MAXUINT16);
    assert((y >= 0 && y <= LEVEL_MAX_Y) || y == G_MAXUINT16);

    pos.x = x;
    pos.y = y;

    return pos;
}

position pos_move(position pos, int direction)
{
    /* return given position if direction is not implemented */
    position npos = pos;

    assert(direction > GD_NONE && direction < GD_MAX);

    switch (direction)
    {
    case GD_WEST:
        if (pos.x > 0)
            npos = pos_new(pos.x - 1, pos.y);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_NW:
        if ((pos.x > 0) && (pos.y > 0))
            npos = pos_new(pos.x - 1, pos.y - 1);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_NORTH:
        if (pos.y > 0)
            npos = pos_new(pos.x, pos.y - 1);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_NE:
        if ((pos.x < LEVEL_MAX_X - 1) && (pos.y > 0))
            npos = pos_new(pos.x + 1, pos.y - 1);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_EAST:
        if (pos.x < LEVEL_MAX_X - 1)
            npos = pos_new(pos.x + 1, pos.y);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_SE:
        if ((pos.x < LEVEL_MAX_X - 1) && (pos.y < LEVEL_MAX_Y - 1))
            npos = pos_new(pos.x + 1, pos.y + 1);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_SOUTH:
        if (pos.y < LEVEL_MAX_Y - 1)
            npos = pos_new(pos.x, pos.y + 1);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;

    case GD_SW:
        if ((pos.x > 0) && (pos.y < LEVEL_MAX_Y - 1))
            npos = pos_new(pos.x - 1, pos.y + 1);
        else
            npos = pos_new(G_MAXUINT16, G_MAXUINT16);

        break;
    }

    return npos;
}

/**
 * Create a new rectangle of given dimensions. Do a sanity check and
 * replace out-of-bounds values.
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 *
 * @return corrected area
 *
 */
rectangle rect_new(int x1, int y1, int x2, int y2)
{
    rectangle rect;

    rect.x1 = (x1 < 0) ? 0 : x1;
    rect.y1 = (y1 < 0) ? 0 : y1;
    rect.x2 = (x2 > LEVEL_MAX_X) ? LEVEL_MAX_X : x2;
    rect.y2 = (y2 > LEVEL_MAX_Y) ? LEVEL_MAX_Y : y2;

    return(rect);
}

rectangle rect_new_sized(position center, int size)
{
    return rect_new(center.x - size,
                    center.y - size,
                    center.x + size,
                    center.y + size);
}

int pos_in_rect(position pos, rectangle rect)
{
    if ((pos.x >= rect.x1)
            && (pos.x <= rect.x2)
            && (pos.y >= rect.y1)
            && (pos.y <= rect.y2))
        return TRUE;
    else
        return FALSE;
}

area *area_new(int start_x, int start_y, int size_x, int size_y)
{
    area *area;
    int y;

    area = g_malloc0(sizeof(area));

    area->start_x = start_x;
    area->start_y = start_y;

    area->size_x = size_x;
    area->size_y = size_y;

    area->area = g_malloc0(size_y * sizeof(int *));

    for (y = 0; y < size_y; y++)
        area->area[y] = g_malloc0(size_x * sizeof(int));

    return area;
}

/**
 * Draw a circle
 * Midpoint circle algorithm
 * from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 *
 * @param center point of the circle
 * @param radius of the circle
 * @param how shall the cirle be filled
 * @param an area which contains the obstacles.
 *        Must match the new area in size. NULL for FILL_SOLID.
 *        Will be freed in this function.
 * @return a new area.
 */
area *area_new_circle(position center, int radius, fill_t filling, area *obstacles)
{
    area *area, *narea;

    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    int fill;

    area = area_new(center.x - radius, center.y - radius, 2 * radius, 2 * radius);

    /* reposition center to relative values */
    center.x = radius;
    center.y = radius;

    area_point_set(area, center.x, center.y + radius);
    area_point_set(area, center.x, center.y - radius);
    area_point_set(area, center.x + radius, center.y);
    area_point_set(area, center.x - radius, center.y);

    while (x < y)
    {
        assert(ddF_x == 2 * x + 1);
        assert(ddF_y == -2 * y);
        assert(f == x * x + y * y - radius * radius + 2 * x - y + 1);

        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        area_point_set(area, center.x + x, center.y + y);
        area_point_set(area, center.x - x, center.y + y);
        area_point_set(area, center.x + x, center.y - y);
        area_point_set(area, center.x - x, center.y - y);
        area_point_set(area, center.x + y, center.y + x);
        area_point_set(area, center.x - y, center.y + x);
        area_point_set(area, center.x + y, center.y - x);
        area_point_set(area, center.x - y, center.y - x);
    }

    /* fill the circle
     * - set fill to 1 when spotting the left border
     * - set position if (fill == 1)
     * - set fill = 2 when spotting the right border
     *
     * do not need to fill the first and last row
     */

    switch (filling)
    {
    case FILL_SOLID:
        for (y = 1; y < area->size_y - 1; y++)
        {
            fill = 0;

            for (x = 0; x < area->size_x; x++)
            {
                /* there are double dots at the beginning and the end of the square */
                if (area_point_get(area, x, y) && (!area_point_get(area, x + 1, y)))
                {
                    fill = !fill;
                    continue;
                }

                if (fill)
                {
                    area_point_set(area, x, y);
                }
            }
        }
        break; /* FILL_SOLID */

    case FILL_FLOOD:
        for (y = 0; y < area->size_y; y++)
        {
            for (x = 0; x < area->size_x; x++)
            {

            }
        }
        break; /* FILL_FLOOD */

    case FILL_BLAST:
        break; /* FILL_BLAST */

    default:
        /* do nothing */
        break;
    }

    if (obstacles)
    {
        area_destroy(obstacles);
    }

    return area;
}

void area_destroy(area *area)
{
    int y;

    assert(area != NULL);

    for (y = 0; y < area->size_y; y++)
        g_free(area->area[y]);

    g_free(area->area);

    g_free(area);
}
