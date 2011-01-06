/*
 * fov.c
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

#include <string.h>
#include "fov.h"
#include "map.h"
#include "position.h"

static void fov_calculate_octant(fov *fov, map *m, position center,
                                 int row, float start,
                                 float end, int radius, int xx,
                                 int xy, int yx, int yy);

struct _fov
{
    int size_x;
    int size_y;

    /* "flattened" array of size size_y * size_x */
    gboolean *fov_data;
};

fov *fov_new(guint size_x, guint size_y)
{
    fov *fov = g_malloc0(sizeof(fov));

    fov->size_x = size_x;
    fov->size_y = size_y;
    fov->fov_data = g_malloc0(size_x * size_y * sizeof(gboolean));

    return fov;
}
/* this and the function fov_calculate_octant() have been
 * ported from python to c using the example at
 * http://roguebasin.roguelikedevelopment.org/index.php?title=Python_shadowcasting_implementation
 */
void fov_calculate(fov *fov, map *m, position pos, int radius)
{
    const int mult[4][8] =
    {
        { 1,  0,  0, -1, -1,  0,  0,  1 },
        { 0,  1, -1,  0,  0, -1,  1,  0 },
        { 0,  1,  1,  0,  0, -1, -1,  0 },
        { 1,  0,  0,  1, -1,  0,  0, -1 }
    };

    int octant;

    /* reset the entire fov to unseen */
    fov_reset(fov);

    /* determine which fields are visible */
    for (octant = 0; octant < 8; octant++)
    {
        fov_calculate_octant(fov, m, pos, 1, 1.0, 0.0, radius,
                             mult[0][octant], mult[1][octant],
                             mult[2][octant], mult[3][octant]);
    }

    fov_set(fov, pos, TRUE);
}

gboolean fov_get(fov *fov, position pos)
{
    assert (fov != NULL);
    assert (pos.x <= fov->size_x);
    assert (pos.y <= fov->size_y);

    return fov->fov_data[pos.y * fov->size_x + pos.x];
}

void fov_set(fov *fov, position pos, gboolean visible)
{
    assert (fov != NULL);
    assert (pos.x <= fov->size_x);
    assert (pos.y <= fov->size_y);

    fov->fov_data[pos.y * fov->size_x + pos.x] = visible;
}

void fov_reset(fov *fov)
{
    assert (fov != NULL);

    /* set fov_data to FALSE */
    memset(fov->fov_data, 0, fov->size_x * fov->size_y * sizeof(int));
}

void fov_free(fov *fov)
{
    assert (fov != NULL);

    g_free(fov->fov_data);
    g_free(fov);
}

static void fov_calculate_octant(fov *fov, map *m, position center,
                                 int row, float start,
                                 float end, int radius, int xx,
                                 int xy, int yx, int yy)
{
    int radius_squared;
    int j;
    int dx, dy;
    int X, Y;
    int blocked;
    float l_slope, r_slope;
    float new_start = 0;

    if (start < end)
        return;

    radius_squared = radius * radius;

    for (j = row; j <= radius + 1; j++)
    {
        dx = -j - 1;
        dy = -j;

        blocked = FALSE;

        while (dx <= 0)
        {
            dx += 1;

            /* Translate the dx, dy coordinates into map coordinates: */
            X = center.x + dx * xx + dy * xy;
            Y = center.y + dx * yx + dy * yy;

            /* check if coordinated are within bounds */
            if ((X < 0) || (X >= fov->size_x))
                continue;

            if ((Y < 0) || (Y >= fov->size_y))
                continue;

            /* l_slope and r_slope store the slopes of the left and right
             * extremities of the square we're considering: */
            l_slope = (dx - 0.5) / (dy + 0.5);
            r_slope = (dx + 0.5) / (dy - 0.5);

            if (start < r_slope)
            {
                continue;
            }
            else if (end > l_slope)
            {
                break;
            }
            else
            {
                /* Our light beam is touching this square; light it */
                if ((dx * dx + dy * dy) < radius_squared)
                {
                    fov_set(fov, pos_new(X, Y, m->nlevel), TRUE);
                }

                if (blocked)
                {
                    /* we're scanning a row of blocked squares */
                    if (!map_pos_transparent(m, pos_new(X,Y, m->nlevel)))
                    {
                        new_start = r_slope;
                        continue;
                    }
                    else
                    {
                        blocked = FALSE;
                        start = new_start;
                    }
                }
                else
                {
                    if (!map_pos_transparent(m, pos_new(X, Y, m->nlevel)) && (j < radius))
                    {
                        /* This is a blocking square, start a child scan */
                        blocked = TRUE;
                    }

                    fov_calculate_octant(fov, m, center, j + 1, start, l_slope,
                                         radius, xx, xy, yx, yy);

                    new_start = r_slope;
                }
            }
        }

        /* Row is scanned; do next row unless last square was blocked */
        if (blocked)
        {
            break;
        }
    }
}
