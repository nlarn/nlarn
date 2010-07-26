/*
 * position.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

#include <assert.h>
#include <stdlib.h>

#include "cJSON.h"
#include "map.h"
#include "position.h"

static void area_flood_worker(area *flood, area *obstacles, int x, int y);

position pos_new(int x, int y, int z)
{
    position pos;

    assert((x >= 0 && x <= MAP_MAX_X) || x == G_MAXINT16);
    assert((y >= 0 && y <= MAP_MAX_Y) || y == G_MAXINT16);
    assert((z >= 0 && z <= MAP_MAX) || z == G_MAXINT16);

    pos.x = x;
    pos.y = y;
    pos.z = z;

    return pos;
}

position pos_move(position pos, direction dir)
{
    /* return given position if direction is not implemented */
    position npos;

    assert(dir > GD_NONE && dir < GD_MAX);

    switch (dir)
    {
    case GD_WEST:
        if (pos.x > 0)
            npos = pos_new(pos.x - 1, pos.y, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_NW:
        if ((pos.x > 0) && (pos.y > 0))
            npos = pos_new(pos.x - 1, pos.y - 1, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_NORTH:
        if (pos.y > 0)
            npos = pos_new(pos.x, pos.y - 1, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_NE:
        if ((pos.x < MAP_MAX_X - 1) && (pos.y > 0))
            npos = pos_new(pos.x + 1, pos.y - 1, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_EAST:
        if (pos.x < MAP_MAX_X - 1)
            npos = pos_new(pos.x + 1, pos.y, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_SE:
        if ((pos.x < MAP_MAX_X - 1) && (pos.y < MAP_MAX_Y - 1))
            npos = pos_new(pos.x + 1, pos.y + 1, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_SOUTH:
        if (pos.y < MAP_MAX_Y - 1)
            npos = pos_new(pos.x, pos.y + 1, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    case GD_SW:
        if ((pos.x > 0) && (pos.y < MAP_MAX_Y - 1))
            npos = pos_new(pos.x - 1, pos.y + 1, pos.z);
        else
            npos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

        break;

    default:
        npos = pos;

    }

    return npos;
}

gint pos_distance(position first, position second)
{
    if (first.z != second.z)
        return INT_MAX;

    return (abs(first.x - second.x) + 1) + (abs(first.y - second.y) + 1);
}

gint pos_grid_distance(position first, position second)
{
    if (first.z != second.z)
        return INT_MAX;

    return (max(abs(first.x - second.x) + 1, abs(first.y - second.y) + 1));
}

int pos_identical(position pos1, position pos2)
{
    if (pos1.z != pos2.z)
        return FALSE;

    return ((pos1).x == (pos2).x) && ((pos1).y == (pos2).y);
}

int pos_adjacent(position first, position second)
{
    guint dist_x, dist_y;

    if (first.z != second.z)
        return FALSE;

    dist_x = abs(first.x - second.x);
    dist_y = abs(first.y - second.y);

    return ((dist_x < 2) && (dist_y < 2));
}

int pos_valid(position pos)
{
    return (pos.x >= 0) && (pos.x < MAP_MAX_X)
           && (pos.y >= 0) && (pos.y < MAP_MAX_Y)
           && (pos.z >= 0) && (pos.z < MAP_MAX);
}

cJSON *pos_serialize(position pos)
{
    cJSON *pval = cJSON_CreateObject();

    cJSON_AddNumberToObject(pval, "x", pos.x);
    cJSON_AddNumberToObject(pval, "y", pos.y);
    cJSON_AddNumberToObject(pval, "z", pos.z);

    return pval;
}

position pos_deserialize(cJSON *pser)
{
    position pos;

    pos.x = cJSON_GetObjectItem(pser, "x")->valueint;
    pos.y = cJSON_GetObjectItem(pser, "y")->valueint;
    pos.z = cJSON_GetObjectItem(pser, "z")->valueint;

    return pos;
}

rectangle rect_new(int x1, int y1, int x2, int y2)
{
    rectangle rect;

    rect.x1 = (x1 < 0) ? 0 : x1;
    rect.y1 = (y1 < 0) ? 0 : y1;
    rect.x2 = (x2 > MAP_MAX_X) ? MAP_MAX_X : x2;
    rect.y2 = (y2 > MAP_MAX_Y) ? MAP_MAX_Y : y2;

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
    {
        return TRUE;
    }
    else
        return FALSE;
}

area *area_new(int start_x, int start_y, int size_x, int size_y)
{
    area *a;
    int y;

    a = g_malloc0(sizeof(area));

    a->start_x = start_x;
    a->start_y = start_y;

    a->size_x = size_x;
    a->size_y = size_y;

    a->area = g_malloc0(size_y * sizeof(int *));

    for (y = 0; y < size_y; y++)
        a->area[y] = g_malloc0(size_x * sizeof(int));

    return a;
}

area *area_new_circle(position center, int radius, int hollow)
{
    area *circle;

    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    int fill;

    if (!pos_valid(center))
        return NULL;

    circle = area_new(center.x - radius,
                      center.y - radius,
                      2 * radius + 1,
                      2 * radius + 1);

    /* reposition center to relative values */
    center.x = radius;
    center.y = radius;

    area_point_set(circle, center.x, center.y + radius);
    area_point_set(circle, center.x, center.y - radius);
    area_point_set(circle, center.x + radius, center.y);
    area_point_set(circle, center.x - radius, center.y);

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

        area_point_set(circle, center.x + x, center.y + y);
        area_point_set(circle, center.x - x, center.y + y);
        area_point_set(circle, center.x + x, center.y - y);
        area_point_set(circle, center.x - x, center.y - y);
        area_point_set(circle, center.x + y, center.y + x);
        area_point_set(circle, center.x - y, center.y + x);
        area_point_set(circle, center.x + y, center.y - x);
        area_point_set(circle, center.x - y, center.y - x);
    }

    if (hollow)
        return circle;

    /* fill the circle
     * - set fill to 1 when spotting the left border
     * - set position if (fill == 1)
     * - set fill = 2 when spotting the right border
     *
     * do not need to fill the first and last row
     */

    for (y = 1; y < circle->size_y - 1; y++)
    {
        fill = 0;

        for (x = 0; x < circle->size_x; x++)
        {
            /* there are double dots at the beginning and the end of the square */
            if (area_point_get(circle, x, y) && (!area_point_get(circle, x + 1, y)))
            {
                fill = !fill;
                continue;
            }

            if (fill)
            {
                area_point_set(circle, x, y);
            }
        }
    }

    return circle;
}

area *area_new_circle_flooded(position center, int radius, area *obstacles)
{
    area *narea;
    int start_x, start_y;

    assert(radius > 0 && obstacles != NULL);

    if (!pos_valid(center))
        return NULL;

    /* add circle boundary to obstacle map */
    obstacles = area_add(obstacles, area_new_circle(center, radius, TRUE));

    /* translate absolute center position to area */
    start_x = center.x - obstacles->start_x;
    start_y = center.y - obstacles->start_y;

    /* fill narea */
    narea = area_flood(obstacles, start_x, start_y);

    return narea;
}

area *area_new_ray(position source, position target, area *obstacles)
{
    area *narea;
    int delta_x, delta_y;
    int offset_x, offset_y;
    int x, y;
    signed int ix, iy;
    int error;

    narea = area_new(min(source.x, target.x), min(source.y, target.y),
                     abs(target.x - source.x) + 1, abs(target.y - source.y) + 1);

    /* offset = offset to level map */
    offset_x = narea->start_x;
    offset_y = narea->start_y;

    /* reposition source and target to get a position inside narea */
    source.x -= offset_x;
    source.y -= offset_y;
    target.x -= offset_x;
    target.y -= offset_y;

    /* offset = relative offset for obstacle lookup */
    offset_x -= obstacles->start_x;
    offset_y -= obstacles->start_y;

    x = source.x;
    y = source.y;

    delta_x = abs(target.x - source.x) << 1;
    delta_y = abs(target.y - source.y) << 1;

    /* if x1 == x2 or y1 == y2, then it does not matter what we set here */
    ix = target.x > source.x ? 1 : -1;
    iy = target.y > source.y ? 1 : -1;

    if (delta_x >= delta_y)
    {
        /* error may go below zero */
        error = delta_y - (delta_x >> 1);

        while (x != target.x)
        {
            if (error >= 0)
            {
                if (error || (ix > 0))
                {
                    y += iy;
                    error -= delta_x;
                }
            }

            x += ix;
            error += delta_y;

            if (area_point_get(obstacles, x + offset_x, y + offset_y))
            {
                /* stop painting ray */
                break;
            }
            else
            {
                area_point_set(narea, x, y);
            }
        }
    }
    else
    {
        /* error may go below zero */
        int error = delta_x - (delta_y >> 1);

        while (y != target.y)
        {
            if (error >= 0)
            {
                if (error || (iy > 0))
                {
                    x += ix;
                    error -= delta_y;
                }
            }

            y += iy;
            error += delta_x;

            if (area_point_get(obstacles, x + offset_x, y + offset_y))
            {
                /* stop painting ray */
                area_destroy(obstacles);
                area_destroy(narea);
                return NULL;
            }
            else
            {
                area_point_set(narea, x, y);
            }
        }
    }

    area_destroy(obstacles);

    return narea;
}

area *area_copy(area *a)
{
    area *narea;
    int x, y;

    assert(a != NULL);
    narea = area_new(a->start_x, a->start_y, a->size_x, a->size_y);

    for (y = 0; y < a->size_y; y++)
    {
        for (x = 0; x < a->size_x; x++)
        {
            if (area_point_get(a, x, y))
            {
                area_point_set(narea, x, y);
            }
        }
    }

    return narea;
}

void area_destroy(area *a)
{
    int y;

    assert(a != NULL);

    for (y = 0; y < a->size_y; y++)
        g_free(a->area[y]);

    g_free(a->area);

    g_free(a);
}

area *area_add(area *a, area *b)
{
    int x, y;

    assert (a != NULL && b != NULL);
    assert (a->size_x == b->size_x && a->size_y == b->size_y);

    for (y = 0; y < a->size_y; y++)
    {
        for (x = 0; x < a->size_x; x++)
        {
            if (area_point_get(b, x, y))
            {
                area_point_set(a, x, y);
            }
        }
    }

    area_destroy(b);

    return a;
}

area *area_flood(area *obstacles, int start_x, int start_y)
{
    area *flood = NULL;

    assert (obstacles != NULL && area_point_valid(obstacles, start_x, start_y));

    flood = area_new(obstacles->start_x, obstacles->start_y,
                     obstacles->size_x, obstacles->size_y);

    area_flood_worker(flood, obstacles, start_x, start_y);

    area_destroy(obstacles);

    return flood;
}

void area_point_set(area *a, int x, int y)
{
    assert (a != NULL && area_point_valid(a, x, y));
    a->area[y][x] = TRUE;
}

int area_point_get(area *a, int x, int y)
{
    assert (a != NULL);

    if (!area_point_valid(a, x, y))
        return FALSE;

    return a->area[y][x];
}

void area_point_del(area *a, int x, int y)
{
    assert (a != NULL && area_point_valid(a, x, y));
    a->area[y][x] = FALSE;
}

int area_point_valid(area *a, int x, int y)
{
    assert (a != NULL);
    return ((x < a->size_x) && (x >= 0)) && ((y < a->size_y) && (y >= 0));
}

void area_pos_set(area *a, position pos)
{
    int x, y;

    assert (a != NULL);

    x = pos.x - a->start_x;
    y = pos.y - a->start_y;

    area_point_set(a, x, y);
}

int area_pos_get(area *a, position pos)
{
    int x, y;

    assert (a != NULL);

    x = pos.x - a->start_x;
    y = pos.y - a->start_y;

    return area_point_get(a, x, y);
}

void area_pos_del(area *a, position pos)
{
    int x, y;

    assert (a != NULL);

    x = pos.x - a->start_x;
    y = pos.y - a->start_y;

    area_point_del(a, x, y);
}

static void area_flood_worker(area *flood, area *obstacles, int x, int y)
{
    /* stepped out of area */
    if (!area_point_valid(flood, x, y))
        return;

    /* can't flood this */
    if (area_point_get(obstacles, x, y))
        return;

    /* been here before */
    if (area_point_get(flood, x, y))
        return;

    area_point_set(flood, x, y);

    area_flood_worker(flood, obstacles, x + 1, y);
    area_flood_worker(flood, obstacles, x - 1, y);
    area_flood_worker(flood, obstacles, x, y + 1);
    area_flood_worker(flood, obstacles, x, y - 1);
}
