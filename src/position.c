/*
 * position.c
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

#include <glib.h>
#include <stdlib.h>

#include "cJSON.h"
#include "display.h"
#include "map.h"
#include "extdefs.h"
#include "position.h"

#define POS_MAX_XY (1<<10)
#define POS_MAX_Z  (1<<6)

static void area_flood_worker(area *flood, area *obstacles, int x, int y);

const position pos_invalid = { { POS_MAX_XY, POS_MAX_XY, POS_MAX_Z } };

position pos_move(position pos, direction dir)
{
    /* return given position if direction is not implemented */
    position npos = pos;

    g_assert(dir > GD_NONE && dir < GD_MAX);

    switch (dir)
    {
    case GD_WEST:
        if (X(pos) > 0)
            X(npos) -= 1;
        else
            npos = pos_invalid;

        break;

    case GD_NW:
        if ((X(pos) > 0) && (Y(pos) > 0))
        {
            X(npos) -= 1;
            Y(npos) -= 1;
        }
        else
            npos = pos_invalid;

        break;

    case GD_NORTH:
        if (Y(pos) > 0)
            Y(npos) -= 1;
        else
            npos = pos_invalid;

        break;

    case GD_NE:
        if ((X(pos) < MAP_MAX_X - 1) && (Y(pos) > 0))
        {
            X(npos) += 1;
            Y(npos) -= 1;
        }
        else
            npos = pos_invalid;

        break;

    case GD_EAST:
        if (X(pos) < MAP_MAX_X - 1)
            X(npos) += 1;
        else
            npos = pos_invalid;

        break;

    case GD_SE:
        if ((X(pos) < MAP_MAX_X - 1) && (Y(pos) < MAP_MAX_Y - 1))
        {
            X(npos) += 1;
            Y(npos) += 1;
        }
        else
            npos = pos_invalid;

        break;

    case GD_SOUTH:
        if (Y(pos) < MAP_MAX_Y - 1)
            Y(npos) += 1;
        else
            npos = pos_invalid;

        break;

    case GD_SW:
        if ((X(pos) > 0) && (Y(pos) < MAP_MAX_Y - 1))
        {
            X(npos) -= 1;
            Y(npos) += 1;
        }
        else
            npos = pos_invalid;

        break;

    default:
        npos = pos;

    }

    return npos;
}

direction pos_direction(position here, position there)
{
    // determine the distance between the two positions
    int dx = X(there) - X(here);
    int dy = Y(there) - Y(here);
    direction dir;

    if (dx  < 0 && dy  < 0) dir = GD_NW;
    if (dx  < 0 && dy == 0) dir = GD_WEST;
    if (dx  < 0 && dy  > 0) dir = GD_SW;
    if (dx == 0 && dy  < 0) dir = GD_NORTH;
    if (dx == 0 && dy == 0) dir = GD_CURR;
    if (dx == 0 && dy  > 0) dir = GD_SOUTH;
    if (dx  > 0 && dy  < 0) dir = GD_NE;
    if (dx  > 0 && dy == 0) dir = GD_EAST;
    if (dx  > 0 && dy  > 0) dir = GD_SE;

    return dir;
}

gint pos_distance(position first, position second)
{
    if (Z(first) != Z(second))
        return INT_MAX;

    return abs(X(first) - X(second)) + abs(Y(first) - Y(second));
}

int pos_adjacent(position first, position second)
{
    if (Z(first) != Z(second))
        return false;

    guint dist_x = abs(X(first) - X(second));
    guint dist_y = abs(Y(first) - Y(second));

    return ((dist_x < 2) && (dist_y < 2));
}

int pos_valid(position pos)
{
    return (0 <= X(pos) && X(pos) < MAP_MAX_X)
        && (0 <= Y(pos) && Y(pos) < MAP_MAX_Y)
        && (0 <= Z(pos) && Z(pos) < MAP_MAX);
}

direction pos_dir(position origin, position target)
{
    g_assert (pos_valid(origin) && pos_valid(target));

    if ((X(origin) >  X(target)) && (Y(origin) <  Y(target))) return GD_SW;
    if ((X(origin) == X(target)) && (Y(origin) <  Y(target))) return GD_SOUTH;
    if ((X(origin) <  X(target)) && (Y(origin) <  Y(target))) return GD_SE;
    if ((X(origin) >  X(target)) && (Y(origin) == Y(target))) return GD_WEST;
    if ((X(origin) == X(target)) && (Y(origin) == Y(target))) return GD_CURR;
    if ((X(origin) <  X(target)) && (Y(origin) == Y(target))) return GD_EAST;
    if ((X(origin) >  X(target)) && (Y(origin) >  Y(target))) return GD_NW;
    if ((X(origin) == X(target)) && (Y(origin) >  Y(target))) return GD_NORTH;
    if ((X(origin) <  X(target)) && (Y(origin) >  Y(target))) return GD_NE;

    /* impossible! */
    return GD_NONE;
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

int pos_in_rect(position pos, rectangle rect)
{
    if ((X(pos) >= rect.x1)
            && (X(pos) <= rect.x2)
            && (Y(pos) >= rect.y1)
            && (Y(pos) <= rect.y2))
    {
        return true;
    }
    else
        return false;
}

area *area_new(int start_x, int start_y, int size_x, int size_y)
{
    area *a = g_malloc0(sizeof(area));

    a->start_x = start_x;
    a->start_y = start_y;
    a->size_x = size_x;
    a->size_y = size_y;

    a->area = g_malloc0(size_y * sizeof(int *));

    for (int y = 0; y < size_y; y++)
        a->area[y] = g_malloc0(size_x * sizeof(int));

    return a;
}

area *area_new_circle(position center, guint radius, gboolean hollow)
{
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    if (!pos_valid(center))
        return NULL;

    area *circle = area_new(X(center) - radius,
                            Y(center) - radius,
                            2 * radius + 1,
                            2 * radius + 1);

    /* reposition the center to relative values */
    X(center) = radius;
    Y(center) = radius;

    area_point_set(circle, X(center), Y(center) + radius);
    area_point_set(circle, X(center), Y(center) - radius);
    area_point_set(circle, X(center) + radius, Y(center));
    area_point_set(circle, X(center) - radius, Y(center));

    while (x < y)
    {
        g_assert(ddF_x == 2 * x + 1);
        g_assert(ddF_y == -2 * y);
        g_assert(f == x * x + y * y - (int)radius * (int)radius + 2 * x - y + 1);

        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        area_point_set(circle, X(center) + x, Y(center) + y);
        area_point_set(circle, X(center) - x, Y(center) + y);
        area_point_set(circle, X(center) + x, Y(center) - y);
        area_point_set(circle, X(center) - x, Y(center) - y);
        area_point_set(circle, X(center) + y, Y(center) + x);
        area_point_set(circle, X(center) - y, Y(center) + x);
        area_point_set(circle, X(center) + y, Y(center) - x);
        area_point_set(circle, X(center) - y, Y(center) - x);
    }

    if (hollow)
        return circle;

    /* fill the circle
     * - set fill to true when spotting the left border
     * - set position if (fill == true)
     * - set fill = false when spotting the right border
     *
     * do not need to fill the first and last row
     */

    for (y = 1; y < circle->size_y - 1; y++)
    {
        gboolean fill = false;

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

area *area_new_circle_flooded(position center, guint radius, area *obstacles)
{
    g_assert(radius > 0 && obstacles != NULL);

    if (!pos_valid(center))
        return NULL;

    /* add circle boundary to obstacle map */
    obstacles = area_add(obstacles, area_new_circle(center, radius, true));

    /* translate absolute center position to area */
    int start_x = X(center) - obstacles->start_x;
    int start_y = Y(center) - obstacles->start_y;

    /* fill narea */
    area *narea = area_flood(obstacles, start_x, start_y);

    return narea;
}

gboolean area_blast(position center, guint radius,
                    const damage_originator *damo,
                    area_hit_sth pos_hitfun,
                    gpointer data1, gpointer data2,
                    char glyph, colour fg)
{
    map *cmap = game_map(nlarn, Z(center));
    position cursor = center;
    gboolean retval = false;

    area *obsmap = map_get_obstacles(cmap, center, radius, true);
    area *ball = area_new_circle_flooded(center, radius, obsmap);

    attron(COLOR_PAIR(fg));

    for (Y(cursor) = ball->start_y; Y(cursor) < ball->start_y + ball->size_y; Y(cursor)++)
    {
        for (X(cursor) = ball->start_x; X(cursor) < ball->start_x + ball->size_x; X(cursor)++)
        {
            monster *m = NULL;

            /* skip this position if it is not affected by the blast */
            if (!area_pos_get(ball, cursor))
                continue;

            /* move the cursor to the position */
            move(Y(cursor), X(cursor));

            if (map_sobject_at(cmap, cursor))
            {
                /* The blast hit a stationary object. */
                addch(so_get_glyph(map_sobject_at(cmap, cursor)));
            }
            else if ((m = map_get_monster_at(cmap, cursor)))
            {
                /* The blast hit a monster */
                if (monster_in_sight(m))
                    addch(monster_glyph(m));
                else
                    addch(glyph);
            }
            else if (pos_identical(nlarn->p->pos, cursor))
            {
                /* The blast hit the player */
                addch('@');
            }
            else
            {
                /* The blast hit nothing */
                addch(glyph);
            }

            /* keep track if the blast hit something */
            if (pos_hitfun(cursor, damo, data1, data2))
                retval = true;
        }
    }

    area_destroy(ball);
    attroff(COLOR_PAIR(fg));

    /* make sure the blast shows up */
    display_draw();

    /* sleep a 3/4 second */
    napms(750);

    return retval;
}

void area_destroy(area *a)
{
    g_assert(a != NULL);

    for (int y = 0; y < a->size_y; y++)
        g_free(a->area[y]);

    g_free(a->area);

    g_free(a);
}

area *area_add(area *a, area *b)
{
    g_assert (a != NULL && b != NULL);
    g_assert (a->size_x == b->size_x && a->size_y == b->size_y);

    for (int y = 0; y < a->size_y; y++)
    {
        for (int x = 0; x < a->size_x; x++)
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
    g_assert (obstacles != NULL && area_point_valid(obstacles, start_x, start_y));

    area *flood = area_new(obstacles->start_x, obstacles->start_y,
                           obstacles->size_x, obstacles->size_y);

    area_flood_worker(flood, obstacles, start_x, start_y);

    area_destroy(obstacles);

    return flood;
}

void area_point_set(area *a, int x, int y)
{
    g_assert(a != NULL);
    g_assert(area_point_valid(a, x, y));
    a->area[y][x] = true;
}

int area_point_get(area *a, int x, int y)
{
    g_assert (a != NULL);

    if (!area_point_valid(a, x, y))
        return false;

    return a->area[y][x];
}

int area_pos_get(area *a, position pos)
{
    g_assert (a != NULL);

    int x = X(pos) - a->start_x;
    int y = Y(pos) - a->start_y;

    return area_point_get(a, x, y);
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
