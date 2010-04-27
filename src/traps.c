/*
 * traps.c
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

#include "effects.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "traps.h"

const trap_data traps[TT_MAX] = {
    {
        TT_NONE,
        ET_NONE,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        TT_ARROW,
        ET_POISON,
        75,
        50,
        10,
        "arrow trap",
        "You are hit by an arrow.",
        "The arrow was poisoned.",
        "The %s is hit by an arrow.",
    },
    {
        TT_DART,
        ET_POISON,
        75,
        50,
        5,
        "dart trap",
        "You are hit by a dart.",
        "The dart was poisoned.",
        "The %s is hit by a dart.",
    },
    {
        TT_TELEPORT,
        ET_NONE,
        55,
        0,
        0,
        "teleport trap",
        "Zaaaappp! You've been teleported!",
        NULL,
        "The %s has been teleported away.",
    },
    {
        TT_PIT,
        ET_NONE,
        80,
        0,
        6,
        "pit",
        "You fall into a pit!",
        NULL,
        "The %s falls into a pit.",
    },
    {
        TT_SPIKEDPIT,
        ET_POISON,
        80,
        60,
        12,
        "pit full of spikes",
        "You fall into a pit full of spikes!",
        "",
        "The %s falls into a pit full of spikes.",
    },
    {
        TT_SLEEPGAS,
        ET_SLEEP,
        75,
        100,
        0,
        "sleeping gas trap",
        "A cloud of gas engulfs you.",
        NULL,
        "A cloud of gas engulfs the %s.",
    },
    {
        TT_TRAPDOOR,
        ET_NONE,
        75,
        0,
        5,
        "trapdoor",
        "You fall through a trap door!",
        NULL,
        "The %s falls through a trap door!",
    },
};

int player_trap_trigger(player *p, trap_t trap, int force)
{
    /* additional time of turn, if any */
    int time = 0;

    /* chance to trigger the trap on target tile */
    int possibility = trap_chance(trap);

    if (player_memory_of(p, p->pos).trap == trap)
        /* if player knows the trap a 5% chance remains */
        possibility = 5;

    if (force || chance(possibility))
    {
        log_add_entry(nlarn->log, trap_p_message(trap));

        /* refresh player's knowlege of trap */
        player_memory_of(p, p->pos).trap = trap;

        switch (trap)
        {
        case TT_TRAPDOOR:
            time += player_map_enter(p, game_map(nlarn, p->pos.z + 1), TRUE);
            break;

        case TT_TELEPORT:
            p->pos = map_find_space(game_map(nlarn, p->pos.z), LE_MONSTER, FALSE);
            break;

        default:
            if (trap_damage(trap))
            {
                damage *dam = damage_new(DAM_PHYSICAL, ATT_NONE,
                                         rand_1n(trap_damage(trap)) + p->pos.z,
                                         NULL);

                player_damage_take(p, dam, PD_TRAP, trap);
            }

            /* if there is an effect on the trap add it to player's effects. */
            if (trap_effect(trap) && chance(trap_effect_chance(trap)))
            {
                /* display message if there is one */
                if (trap_e_message(trap))
                {
                    log_add_entry(nlarn->log, trap_e_message(trap));
                }

                player_effect_add(p, effect_new(trap_effect(trap)));
            }
        }

    }
    else if (player_memory_of(p, p->pos).trap == trap)
    {
        log_add_entry(nlarn->log, "You evade the %s.", trap_description(trap));
    }

    return time;
}
