/*
 * traps.h
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

#ifndef TRAPS_H
#define TRAPS_H

#include "colours.h"
#include "enumFactory.h"
#include "monsters.h"

#define TRAP_TYPE_ENUM(TRAP_TYPE) \
    TRAP_TYPE(TT_NONE,) \
    TRAP_TYPE(TT_ARROW,) \
    TRAP_TYPE(TT_DART,) \
    TRAP_TYPE(TT_TELEPORT,) \
    TRAP_TYPE(TT_PIT,) \
    TRAP_TYPE(TT_SPIKEDPIT,) \
    TRAP_TYPE(TT_SLEEPGAS,) \
    TRAP_TYPE(TT_MANADRAIN,) \
    TRAP_TYPE(TT_TRAPDOOR,) \
    TRAP_TYPE(TT_MAX,) \

DECLARE_ENUM(trap_t, TRAP_TYPE_ENUM)

typedef struct trap_data
{
    trap_t type;             /* trap type */
    int effect_t;            /* effect caused by this trap */
    colour fg;               /* glyph color */
    int chance;              /* chance this trap triggers */
    int effect_chance;       /* chance the effect is activated */
    int damage;              /* base damage */
    const char *description; /* description of trap */
    const char *p_message;   /* message given to player when triggered */
    const char *e_message;   /* message shown when trap effect is activated */
    const char *m_message;   /* message displayed when a monster triggered the trap */
} trap_data;

extern const trap_data traps[TT_MAX];

/* forward declarations */
struct player;


/* functions */
/**
 * Player stepped on a trap
 *
 * @param p the player
 * @param trap the trap
 * @param force enforce activation of the trap
 * @return number of turns this move took
 */
int player_trap_trigger(struct player *p, trap_t trap, int force);

/**
 * A monster stepped on a trap.
 *
 * @param m The monster.
 * @return the monster, or NULL if the monster died.
 */
monster *monster_trap_trigger(monster *m);

/**
  * @brief Disarm a trap.
  *
  * @params p The player.
  * @return The number of turns elapsed.
  */
guint trap_disarm(struct player *p);

/* macros */
#define trap_effect(trap) (traps[(trap)].effect_t)
#define trap_colour(trap) (traps[(trap)].fg)
#define trap_chance(trap) (traps[(trap)].chance)
#define trap_effect_chance(trap) (traps[(trap)].effect_chance)
#define trap_damage(trap) (traps[(trap)].damage)
#define trap_description(trap) (traps[(trap)].description)
#define trap_p_message(trap) (traps[(trap)].p_message)
#define trap_e_message(trap) (traps[(trap)].e_message)
#define trap_m_message(trap) (traps[(trap)].m_message)

#endif
