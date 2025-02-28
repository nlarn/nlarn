/*
 * combat.c
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

#include <string.h>

#include "combat.h"
#include "extdefs.h"
#include "game.h"
#include "player.h"

#include "enumFactory.h"

DEFINE_ENUM(speed, SPEED_ENUM)
DEFINE_ENUM(size, SIZE_ENUM)
DEFINE_ENUM(attack_t, ATTACK_T_ENUM)
DEFINE_ENUM(damage_t, DAMAGE_T_ENUM)
DEFINE_ENUM(damage_originator_t, DAMAGE_ORIGINATOR_T_ENUM)

damage *damage_new(damage_t type, attack_t att_type, int amount,
                   damage_originator_t damo, gpointer originator)
{
    damage *dam = g_malloc0(sizeof(damage));

    dam->type = type;
    dam->attack = att_type;
    dam->amount = amount;
    dam->dam_origin.ot = damo;
    dam->dam_origin.originator = originator;

    return dam;
}

damage *damage_copy(damage *dam)
{
    g_assert (dam != NULL);

    damage *dcopy = g_malloc(sizeof(damage));
    memcpy(dcopy, dam, sizeof(damage));

    return dcopy;
}

char *damage_to_str(damage *dam)
{
    static char buf[121];
    g_snprintf(buf, 120, "[%s - %s - %s: %d]",
            attack_t_string(dam->attack),
            damage_t_string(dam->type),
            dam->type > DAM_ELECTRICITY ? "chance" : "hp",
            dam->amount);

    return buf;
}

// Turn to base chance to hit into a percental chance to hit
static inline int combat_calc_percentage(int base_to_hit)
{
    if (base_to_hit < 1)
        return 0;

    if (base_to_hit >= 20)
        return 100;

    return (5 * base_to_hit);
}

// Calculate the base chance of the play to hit a monster
static int combat_player_to_mt_base_chance_to_hit(struct player *p, enum monster_t mt, gboolean use_weapon)
{
    int base_to_hit = p->level
                       + max(0, player_get_dex(p) - 12)
                       + (use_weapon && p->eq_weapon ? weapon_acc(p->eq_weapon) : 0)
                       + (use_weapon && p->eq_weapon && weapon_is_ranged(p->eq_weapon)
                                     && p->eq_quiver
                               ? ammo_accuracy(p->eq_quiver) : 0)
                       + (player_get_speed(p) - monster_type_speed(mt)) / 25
                       /* the rule below gives a -3 for tiny monsters and a +4
                          for gargantuan monsters */
                       + ((monster_type_size(mt) - MEDIUM) / 25)
                       - monster_type_ac(mt);

    return base_to_hit;
}

int combat_chance_player_to_mt_hit(struct player *p, enum monster_t mt, gboolean use_weapon)
{
    g_assert (p != NULL);

    const int base_to_hit = combat_player_to_mt_base_chance_to_hit(p, mt, use_weapon);

    return combat_calc_percentage(base_to_hit);
}

int combat_chance_player_to_monster_hit(struct player *p, struct _monster *m, gboolean use_weapon)
{
    g_assert (p != NULL && m != NULL);

    int base_to_hit = combat_player_to_mt_base_chance_to_hit(p, monster_type(m), use_weapon)
                       - (!monster_in_sight(m) ? 5 : 0);

    return combat_calc_percentage(base_to_hit);
}

damage_min_max damage_calc_min_max(struct player *p, enum monster_t mt)
{
    /* without weapon cause a basic unarmed combat damage */
    int min_damage = 1;

    if (p->eq_weapon != NULL)
    {
        /* wielded weapon base damage */
        min_damage = weapon_damage(p->eq_weapon);

        // Blessed weapons do 50% bonus damage against demons and undead.
        if (p->eq_weapon->blessed
                && (monster_type_flags(mt, DEMON) || monster_type_flags(mt, UNDEAD)))
        {
            min_damage *= 3;
            min_damage /= 2;
        }
    }

    min_damage += player_effect(p, ET_INC_DAMAGE);
    min_damage -= player_effect(p, ET_SICKNESS);

    /* calculate maximum damage: strength bonus and difficulty malus */
    int max_damage = min_damage
                     + player_get_str(p) - 12
                     - game_difficulty(nlarn);

    /* ensure minimal damage */
    damage_min_max ret =
    {
        max(1, min_damage),
        max(max(1, min_damage), max_damage)
    };

    return ret;
}
