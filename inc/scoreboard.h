/*
 * scoreboard.h
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#include <glib.h>

#include "player.h"

#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
/* file descriptor for the scoreboard file when running setgid */
extern int scoreboard_fd;
#endif

typedef struct _score_t
{
    char *player_name;
    gint8 sex;
    guint64 score;
    guint32 moves;
    player_cod cod;
    gint32 cause;
    gint32 hp;
    guint32 hp_max;
    gint32 level;
    gint32 level_max;
    gint32 dlevel;
    gint32 dlevel_max;
    gint32 difficulty;
    gint64 time_start;
    gint64 time_end;
} score_t;

score_t *score_new(game *g, player_cod cod, int cause);
GList *score_add(game *g, score_t *score);
void scores_destroy(GList *gs);
