/*
 * combat.c
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

#include <string.h>

#include "combat.h"
#include "enumFactory.h"

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

