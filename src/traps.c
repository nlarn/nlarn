/*
 * traps.c
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
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

#include "display.h"
#include "effects.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "traps.h"

const trap_data traps[TT_MAX] =
{
    /*
        trap type
        effect type
        glyph colour
        trigger chance
        effect chance
        base damage
        description
        player trigger message
        effect message
        monster trigger message
    */
    {
        TT_NONE,
        ET_NONE,
        DC_NONE,
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
        DC_CYAN,
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
        DC_CYAN,
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
        DC_MAGENTA,
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
        ET_TRAPPED,
        DC_BROWN,
        80,
        100,
        6,
        "pit",
        "You fall into a pit!",
        NULL,
        "The %s falls into a pit.",
    },
    {
        TT_SPIKEDPIT,
        ET_POISON,
        DC_BROWN,
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
        DC_MAGENTA,
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
        DC_BROWN,
        75,
        0,
        5,
        "trapdoor",
        "You fall through a trap door!",
        NULL,
        "The %s falls through a trap door!",
    },
};

static int modified_effect_chance(trap_t trap, effect_t et, int level)
{
    const int base_chance = trap_effect_chance(trap);
    if (et == ET_POISON && level < 5)
    {
        /* lower poison chance in early levels */
        return (base_chance * level / 5);
    }

    return base_chance;
}

int player_trap_trigger(player *p, trap_t trap, int force)
{
    /* additional time of turn, if any */
    int time = 0;

    const int dex = player_get_dex(p);

    /* chance to trigger the trap on target tile */
    int possibility = trap_chance(trap);

    /* the value of the player's burden effect */
    int bval = player_effect(p, ET_BURDENED);

    if (player_memory_of(p, p->pos).trap == trap)
    {
        // Dex decreases the chance of triggering a known trap.
        if (dex >= 22)
            possibility = 0;
        else
            possibility = (22 - dex)/2;
    }

    /* Check if the player triggers the trap.
       Being burdened increases the chance due to clumsy movement. */
    if (force || chance(possibility + bval))
    {
        log_add_entry(nlarn->log, trap_p_message(trap));

        /* refresh player's knowlege of trap */
        player_memory_of(p, p->pos).trap = trap;

        if (trap_damage(trap))
        {
            /* deal more damage the deeper the dungeon
               level and if the player is burdened */
            damage *dam = damage_new(DAM_PHYSICAL, ATT_NONE,
                                     rand_1n(trap_damage(trap) + bval) + Z(p->pos),
                                     NULL);

            player_damage_take(p, dam, PD_TRAP, trap);
        }

        switch (trap)
        {
        case TT_TRAPDOOR:
            time += player_map_enter(p, game_map(nlarn, Z(p->pos) + 1), TRUE);
            break;

        case TT_TELEPORT:
            p->pos = map_find_space(game_map(nlarn, Z(p->pos)), LE_MONSTER, FALSE);
            break;

        default:
            if (trap == TT_SPIKEDPIT)
            {
                const trap_t trap2 = TT_PIT;

                if (trap_effect(trap2)
                        && chance(modified_effect_chance(trap2, trap_effect(trap2),
                                  Z(p->pos))))
                {
                    /* display message if there is one */
                    if (trap_e_message(trap2))
                        log_add_entry(nlarn->log, trap_e_message(trap2));

                    player_effect_add(p, effect_new(trap_effect(trap2)));
                }
            }

            /* if there is an effect on the trap add it to player's effects. */
            if (trap_effect(trap)
                    && chance(modified_effect_chance(trap, trap_effect(trap),
                              Z(p->pos))))
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
    /* not triggering */
    else if (player_memory_of(p, p->pos).trap == trap)
    {
        log_add_entry(nlarn->log, "You evade the %s.", trap_description(trap));
    }
    else if (chance((dex-12)/2))
    {
        /* detect the trap despite not setting it off */
        log_add_entry(nlarn->log, "You notice there's a %s here!",
                      trap_description(trap));
        player_memory_of(p, p->pos).trap = trap;
    }

    return time;
}
