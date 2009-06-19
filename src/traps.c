/*
 * traps.c
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
