/*
 * memorial.h
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

#ifndef MEMORIAL_H
#define MEMORIAL_H

#include <glib.h>

#include "player.h"
#include "scoreboard.h"

/* assemble a textual description of the player's achievements */
char *memorial_create(player *p, score_t *score, GList *scores);

/* let the player save the given text to a memorial file */
void memorial_save(player *p, const char *text);

#endif
