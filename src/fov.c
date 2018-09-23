/*
 * fov.c
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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
#include <string.h>
#include "fov.h"
#include "game.h"
#include "map.h"
#include "nlarn.h"
#include "position.h"

static void fov_calculate_octant(fov *fv, map *m, position center,
                                 gboolean infravision, int row,
                                 float start, float end, int radius,
                                 int xx, int xy, int yx, int yy);

static gint fov_visible_monster_sort(gconstpointer a, gconstpointer b, gpointer center);

struct _fov
{
    /* the actual field of vision */
    guchar data[MAP_MAX_Y][MAP_MAX_X];

    /* the center of the fov */
    position center;

    /* The list of visible monsters - it's a hash as fields may get visited
       twice, which means that monsters may get added to the list multiple
       times. The hash overwrites duplicate values. */
    GHashTable *mlist;
};

fov *fov_new()
{
    fov *nfov = g_malloc0(sizeof(fov));
    nfov->center = pos_invalid;
    nfov->mlist = g_hash_table_new(g_direct_hash, g_direct_equal);

    return nfov;
}
/* this and the function fov_calculate_octant() have been
 * ported from python to c using the example at
 * http://roguebasin.roguelikedevelopment.org/index.php?title=Python_shadowcasting_implementation
 */
void fov_calculate(fov *fv, map *m, position pos, int radius, gboolean infravision)
{
    const int mult[4][8] =
    {
        { 1,  0,  0, -1, -1,  0,  0,  1 },
        { 0,  1, -1,  0,  0, -1,  1,  0 },
        { 0,  1,  1,  0,  0, -1, -1,  0 },
        { 1,  0,  0,  1, -1,  0,  0, -1 }
    };

    /* reset the entire fov to unseen */
    fov_reset(fv);

    /* set the center of the fov */
    fv->center = pos;

    /* determine which fields are visible */
    for (int octant = 0; octant < 8; octant++)
    {
        fov_calculate_octant(fv, m, pos, infravision,
                             1, 1.0, 0.0, radius,
                             mult[0][octant], mult[1][octant],
                             mult[2][octant], mult[3][octant]);
    }

    fov_set(fv, pos, TRUE, infravision, TRUE);
}

gboolean fov_get(fov *fv, position pos)
{
    g_assert (fv != NULL);
    g_assert (pos_valid(pos));

    return fv->data[Y(pos)][X(pos)];
}

void fov_set(fov *fv, position pos, guchar visible,
             gboolean infravision, gboolean mchk)
{
    g_assert (fv != NULL);
    g_assert (pos_valid(pos));

    fv->data[Y(pos)][X(pos)] = visible;
    monster *mon;

    /* If advised to do so, check if there is a monster at that
       position. Must not be an unknown mimic or invisible. */
    if (mchk && (mon = map_get_monster_at(game_map(nlarn, Z(pos)), pos))
        && !monster_unknown(mon)
        && (!monster_flags(mon, INVISIBLE) || infravision))
    {
        /* found a visible monster -> add it to the list */
        g_hash_table_insert(fv->mlist, mon, 0);
    }
}

void fov_reset(fov *fv)
{
    g_assert (fv != NULL);

    /* set fov_data to FALSE */
    memset(fv->data, 0, MAP_MAX_Y * MAP_MAX_X * sizeof(guchar));

    /* set the center to an invalid position */
    fv->center = pos_invalid;

    /* clean list of visible monsters */
    g_hash_table_remove_all(fv->mlist);
}

monster *fov_get_closest_monster(fov *fv)
{
    monster *closest_monster = NULL;

    if (g_hash_table_size(fv->mlist) > 0)
    {
        GList *mlist;

        /* get the list of all visible monsters */
        mlist = g_hash_table_get_keys(fv->mlist);

        /* sort the monsters list by distance */
        mlist = g_list_sort_with_data(mlist, fov_visible_monster_sort,
                                      &fv->center);

        /* get the first element in the list */
        closest_monster = mlist->data;

        g_list_free(mlist);
    }

    return closest_monster;
}

GList *fov_get_visible_monsters(fov *fv)
{
    GList *mlist = NULL;

    if (g_hash_table_size(fv->mlist) != 0)
    {
        /* get a GList of all visible monster all */
        mlist = g_hash_table_get_keys(fv->mlist);

        /* sort the list of monster by distance */
        mlist = g_list_sort_with_data(mlist, fov_visible_monster_sort,
                                      &fv->center);
    }

    return mlist;
}

void fov_free(fov *fv)
{
    g_assert (fv != NULL);

    /* free the allocated memory */
    g_hash_table_destroy(fv->mlist);
    g_free(fv);
}

static void fov_calculate_octant(fov *fv, map *m, position center,
                                 gboolean infravision, int row,
                                 float start, float end, int radius,
                                 int xx, int xy, int yx, int yy)
{
    int radius_squared;
    int X, Y;
    float l_slope, r_slope;
    float new_start = 0;

    if (start < end)
        return;

    radius_squared = radius * radius;

    for (int j = row; j <= radius + 1; j++)
    {
        int dx = -j - 1;
        int dy = -j;

        int blocked = FALSE;

        while (dx <= 0)
        {
            dx += 1;

            /* Translate the dx, dy coordinates into map coordinates: */
            X = X(center) + dx * xx + dy * xy;
            Y = Y(center) + dx * yx + dy * yy;

            /* check if coordinated are within bounds */
            if ((X < 0) || (X >= MAP_MAX_X))
                continue;

            if ((Y < 0) || (Y >= MAP_MAX_Y))
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
                position pos = { { X, Y, m->nlevel } };

                /* Our light beam is touching this square; light it */
                if ((dx * dx + dy * dy) < radius_squared)
                {
                    fov_set(fv, pos, TRUE, infravision, TRUE);
                }

                if (blocked)
                {
                    /* we're scanning a row of blocked squares */
                    if (!map_pos_transparent(m, pos))
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
                    if (!map_pos_transparent(m, pos) && (j < radius))
                    {
                        /* This is a blocking square, start a child scan */
                        blocked = TRUE;
                    }

                    fov_calculate_octant(fv, m, center, infravision,
                                         j + 1, start, l_slope,
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

static gint fov_visible_monster_sort(gconstpointer a, gconstpointer b, gpointer center)
{
    monster *ma, *mb;

    ma = (monster*)a;
    mb = (monster*)b;

    int da = pos_distance(*(position *)center, monster_pos(ma));
    int db = pos_distance(*(position *)center, monster_pos(mb));

    if (da < db)
        return -1;

    if (da > db)
        return 1;

    return 0;
}
