/*
 * traps.h
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

#ifndef __TRAPS_H_
#define __TRAPS_H_

typedef enum trap_types
{
    TT_NONE,
    TT_ARROW,
    TT_DART,
    TT_TELEPORT,
    TT_PIT,
    TT_SPIKEDPIT,
    TT_SLEEPGAS,
    TT_TRAPDOOR,
    TT_MAX
} trap_t;

typedef struct trap_data
{
    trap_t type;        /* trap type */
    int effect_t;    /* effect caused by this trap */
    int color;          /* glyph color */
    int chance;         /* chance this trap triggers */
    int effect_chance;  /* chance the effect is activated */
    int damage;         /* base damage */
    char *description;  /* description of trap */
    char *p_message;    /* message given to player when triggered */
    char *e_message;    /* message shown when trap effect is activated */
    char *m_message;    /* message displayed when a monster triggered the trap */
} trap_data;

extern const trap_data traps[TT_MAX];

/* forward declarations */
struct player;


/* functions */
/**
 * Player stepped on a trap
 *
 * @param the player
 * @param the trap
 * @return number of turns this move took
 */
int player_trap_trigger(struct player *p, trap_t trap, int force);

/* macros */
#define trap_effect(trap) (traps[(trap)].effect_t)
#define trap_colour(trap) (traps[(trap)].color)
#define trap_chance(trap) (traps[(trap)].chance)
#define trap_effect_chance(trap) (traps[(trap)].effect_chance)
#define trap_damage(trap) (traps[(trap)].damage)
#define trap_description(trap) (traps[(trap)].description)
#define trap_p_message(trap) (traps[(trap)].p_message)
#define trap_e_message(trap) (traps[(trap)].e_message)
#define trap_m_message(trap) (traps[(trap)].m_message)

#endif
