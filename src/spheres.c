/*
 * spheres.c
 * Copyright (C) 2009-2011, 2012 Joachim de Groot <jdegroot@web.de>
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

#include "game.h"
#include "nlarn.h"
#include "random.h"
#include "spheres.h"

static sphere *sphere_at(game *g, position pos, sphere *s);
static void sphere_hit_owner(game *g, sphere *s);
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

void sphere_destroy(sphere *s, game *g)
{
    g_assert(s != NULL);

    g_ptr_array_remove_fast(g->spheres, s);
    g_free(s);
}

void sphere_serialize(sphere *s, cJSON *root)
{
    cJSON *sval;

    cJSON_AddItemToArray(root, sval = cJSON_CreateObject());

    cJSON_AddNumberToObject(sval, "dir", s->dir);
    cJSON_AddNumberToObject(sval, "lifetime", s->lifetime);
    cJSON_AddNumberToObject(sval, "pos", pos_val(s->pos));

    if (!s->owner)
        cJSON_AddFalseToObject(sval, "owner");
}

void sphere_deserialize(cJSON *sser, game *g)
{
    sphere *s = g_malloc0(sizeof(sphere));

    s->dir = cJSON_GetObjectItem(sser, "dir")->valueint;
    s->lifetime = cJSON_GetObjectItem(sser, "lifetime")->valueint;
    pos_val(s->pos) = cJSON_GetObjectItem(sser, "pos")->valueint;

    if (!cJSON_GetObjectItem(sser, "owner"))
        s->owner = g->p;

    g_ptr_array_add(g->spheres, s);
}

void sphere_move(sphere *s, game *g)
{
    position npos;
    int tries = 0;
    direction dir;
    monster *m;
    map *smap;

    g_assert(s != NULL && g != NULL);

    /* reduce lifetime */
    s->lifetime--;

    /* sphere has reached end of life */
    if (s->lifetime == 0)
    {
        sphere_destroy(s, g);
        return;
    }

    smap = game_map(g, Z(s->pos));

    /* try to move sphere into its direction */
    dir = s->dir;
    npos = pos_move(s->pos, dir);

    /* if the new position does not work, try to find another one */
    while ((!pos_valid(npos) || !map_pos_passable(smap, npos))
            && (tries < GD_MAX))
    {
        dir++;

        if (dir == GD_CURR)
            dir++;

        if (dir == GD_MAX)
            dir = 1;

        npos = pos_move(s->pos, dir);
        tries++;
    }

    /* new position has been found, save it and the direction */
    if (tries < GD_MAX)
    {
        s->dir = dir;
        s->pos = npos;
    }
    /* otherwise stand still */

    /* sphere rolled over it's creator */
    if (pos_identical(s->pos, s->owner->pos))
    {
        sphere_hit_owner(g, s);
        return;
    }

    /* check if a monster is located at the sphere's position */
    if ((m = map_get_monster_at(smap, s->pos)))
    {
        /* demons dispel spheres */
        if (monster_flags(m, MF_DEMON))
        {
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s dispels the sphere!",
                        monster_name(m));
            }

            sphere_destroy(s, g);

            return;
        }

        /* disenchantress cancels sphere */
        if (monster_type(m) == MT_DISENCHANTRESS)
        {
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log,
                        "The %s causes cancellation of the sphere!",
                        monster_name(m));
            }

            sphere_destroy(s, g);

            return;
        }

        /* kill monster */
        sphere_kill_monster(s, m);

        return;
    }

    /* check if another sphere is located at the same position */
    sphere *other;
    if ((other = sphere_at(g, s->pos, s)))
    {
        if (fov_get(g->p->fv, s->pos))
        {
            log_add_entry(nlarn->log,
                    "Two spheres of annihilation collide! "
                    "You hear a great earth shaking blast!");
        }
        else if(Z(g->p->pos) == Z(s->pos))
        {
            /* The player is on the same level as the spheres */
            log_add_entry(nlarn->log,
                    "You hear a great earth shaking blast!");
        }

        sphere_destroy(s, g);
        sphere_destroy(other, g);
    }
}


static sphere *sphere_at(game *g, position pos, sphere *s)
{
    for (guint idx = 0; idx < g->spheres->len; idx++)
    {
        sphere *cs = (sphere *)g_ptr_array_index(g->spheres, idx);

        if (s != cs && pos_identical(cs->pos, pos))
            return cs;
    }

    return NULL;
}

static void sphere_hit_owner(game *g, sphere *s)
{
    log_add_entry(nlarn->log, "You are hit by a sphere of annihilation!");

    if (player_effect(s->owner, ET_CANCELLATION))
    {
        /* cancellation protects from spheres */
        log_add_entry(nlarn->log,
                "As the cancellation takes effect, you "
                "hear a great earth shaking blast!");

        sphere_destroy(s, g);

        effect *e = player_effect_get(g->p, ET_CANCELLATION);
        player_effect_del(g->p, e);
    }
    else
    {
        player_die(s->owner, PD_SPHERE, 0);
    }
}

static void sphere_kill_monster(sphere *s, monster *m)
{
    monster *mret; /* monster returned by monster_damage_take */
    guint mexp;    /* xp for killing the monster */

    g_assert(s != NULL && m != NULL);

    mexp = monster_exp(m);

    if (monster_in_sight(m))
    {
        log_add_entry(nlarn->log,
                "The sphere of annihilation hits the %s.",
                monster_name(m));
    }

    mret = monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_MAGIC, 2000, DAMO_SPHERE, s));

    if (!mret && s->owner)
    {
        /* the monster has been killed, grant experience */
        player_exp_gain(s->owner, mexp);
    }
}
