/*
 * spheres.c
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

static void sphere_remove(sphere *s, level *l);
static void sphere_hit_owner(sphere *s, level *l);
static void sphere_kill_monster(sphere *s, monster *m);

sphere *sphere_new(position pos, player *owner, int lifetime)
{
    sphere *s;

    s = g_malloc0(sizeof(sphere));

    s->pos = pos;
    s->owner = owner;

    s->dir = rand_1n(GD_MAX);

    /* prevent spheres hanging around */
    if (s->dir == GD_CURR)
        s->dir++;

    s->lifetime = lifetime;

    return s;
}

void sphere_destroy(sphere *s)
{
    assert(s != NULL);
    g_free(s);
}

void sphere_move(sphere *s, level *l)
{
    position npos;
    int tries = 0;
    int direction;
    monster *m;

    assert(s != NULL && l != NULL);

    /* reduce lifetime */
    s->lifetime--;

    /* sphere has reached end of life */
    if (s->lifetime == 0)
    {
        sphere_remove(s, l);
        return;
    }


    /* try to move sphere into its direction */
    direction = s->dir;
    npos = pos_move(s->pos, direction);

    /* if the new position does not work, try to find another one */
    while ((!pos_valid(npos)
            || !lt_is_passable(level_tiletype_at(l,npos)))
            && (tries < GD_MAX))
    {
        direction++;

        if (direction == GD_CURR)
            direction++;

        if (direction == GD_MAX)
            direction = 1;

        npos = pos_move(s->pos, direction);
        tries++;
    }

    /* new position has been found, save it and the direction */
    if (tries < GD_MAX)
    {
        s->dir = direction;
        s->pos = npos;
    }
    /* otherwise stand still */


    /* sphere rolled over it's creator */
    if (pos_identical(s->pos, s->owner->pos))
    {
        sphere_hit_owner(s, l);
    }

    /* check if a monster is located at the sphere's position */
    if ((m = level_get_monster_at(l, s->pos)))
    {

        /* demons dispel spheres */
        if (m->type >= MT_DEMONLORD_I)
        {
            if (m->m_visible)
            {
                log_add_entry(s->owner->log,
                              "The %s dispels the sphere!",
                              monster_name(m));
            }

            sphere_remove(s, l);

            return;
        }

        /* disenchantress cancels sphere */
        if (m->type == MT_DISENCHANTRESS)
        {
            if (m->m_visible)
            {
                log_add_entry(s->owner->log,
                              "The %s causes cancellation of the sphere!",
                              monster_name(m));
            }

            sphere_remove(s, l);

            return;
        }

        /* kill monster */
        sphere_kill_monster(s, m);
    }

    /* check if another sphere is located at the same position */
    if (sphere_at(l, s->pos) != s)
    {
        log_add_entry(s->owner->log,
                      "Two spheres of annihilation collide! " \
                      "You hear a great earth shaking blast!");

        sphere_remove(s, l);

        return;
    }

}

sphere *sphere_at(level *l, position pos)
{
    guint idx;
    sphere *s;

    for (idx = 1; idx < l->slist->len; idx++)
    {
        s = g_ptr_array_index(l->slist, idx);

        if (pos_identical(pos, s->pos))
            return s;
    }

    return NULL;
}

static void sphere_remove(sphere *s, level *l)
{
    g_ptr_array_remove_fast(l->slist, s);
    sphere_destroy(s);
}

static void sphere_hit_owner(sphere *s, level *l)
{
    /* cancellation protects from spheres */
    if (player_effect(s->owner, ET_CANCELLATION))
    {
        log_add_entry(s->owner->log,
                      "As the cancellation takes effect, you hear a great earth shaking blast!");

        sphere_remove(s, l);

        return;
    }
    else
    {
        player_die(s->owner, PD_SPHERE, 0);
    }
}

static void sphere_kill_monster(sphere *s, monster *m)
{
    /* if the owner is set, grant experience */
    if (s->owner)
    {
        player_exp_gain(s->owner, monster_exp(m));

        if (m->m_visible)
        {
            log_add_entry(s->owner->log,
                          "The sphere of annihilation killed the %s.",
                          monster_name(m));
        }
    }

    monster_damage_take(m, damage_new(DAM_MAGICAL, 2000, NULL));
}
