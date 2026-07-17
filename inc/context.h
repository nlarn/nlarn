/*
 * context.h
 * Copyright (C) 2009-2026 Joachim de Groot <jdegroot@web.de>
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

#ifndef CONTEXT_H
#define CONTEXT_H

#include "player.h"
#include "position.h"

/**
 * Show a context menu listing the actions available at the given map
 * position and perform the one the player chooses. The menu is anchored
 * at the clicked tile.
 *
 * @param p the player
 * @param pos the map position that was clicked
 * @param travel_to output: set to a valid position when the player chose
 *                  to travel there, so the caller can start auto-travel;
 *                  left invalid otherwise.
 * @return the number of turns the chosen action consumed (0 when the menu
 *         was dismissed or the action takes no game turn).
 */
int context_menu(player *p, position pos, position *travel_to);

#endif
