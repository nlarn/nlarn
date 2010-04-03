/*
 * sobjects.h
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "player.h"

/* deal with stationary objects */
int player_altar_desecrate(player *p);
int player_altar_pray(player *p);
int player_building_enter(player *p);
int player_door_close(player *p);
int player_door_open(player *p, int dir);
int player_fountain_drink(player *p);
int player_fountain_wash(player *p);
int player_stairs_down(player *p);
int player_stairs_up(player *p);
int player_throne_pillage(player *p);
int player_throne_sit(player *p);
