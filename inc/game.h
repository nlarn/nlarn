/*
 * game.h
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

#ifndef __GAME_H_
#define __GAME_H_

#include "level.h"
#include "player.h"

/* the world as we know it */
typedef struct game
{
    player *p;                  /* the player */
    level *levels[LEVEL_MAX];   /* the dungeon */
    guint64 time_start;         /* start time */
    guint32 gtime;              /* turn count */
    guint8 difficulty;          /* game difficulty */

    gchar *basedir;
    gchar *libdir;
    gchar *mesgfile;
    gchar *helpfile;
    gchar *mazefile;
    gchar *fortunes;
    gchar *highscores;

    /* flags */
    guint32
        wizard: 1; /* wizard mode */
} game;

/* direction of movement */
/* ordered by number keys */
typedef enum direction
{
    GD_NONE,
    GD_SW,
    GD_SOUTH,
    GD_SE,
    GD_WEST,
    GD_CURR, /* special case: current position */
    GD_EAST,
    GD_NW,
    GD_NORTH,
    GD_NE,
    GD_MAX
} direction;

typedef struct _game_score
{
    char *player_name;
    gint8 sex;
    gint64 score;
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
} game_score_t;

/* function declarations */

game *game_new(int argc, char *argv[]);
int game_destroy(game *g);

int game_save(game *g, char *filename);
game *game_load(char *filename);

void game_scores_destroy(GList *gs);
game_score_t *game_score(game *g, player_cod cod, int cause);
GList *game_score_add(game *g, game_score_t *score);

void game_spin_the_wheel(game *g, guint times);

/* macros */

#define game_difficulty(g) ((g)->difficulty)
#define game_wizardmode(g) ((g)->wizard)

#define game_turn(g)            ((g)->gtime)
#define game_remaining_turns(g) (TIMELIMIT - (g)->gtime)

#define game_dir(g) ((g)->basedir)
#define game_lib(g) ((g)->libdir)

#define game_mesgfile(g) ((g)->mesgfile)
#define game_helpfile(g) ((g)->helpfile)
#define game_mazefile(g) ((g)->mazefile)
#define game_fortunes(g) ((g)->fortunes)
#define game_highscores(g) ((g)->highscores)

/* gtime <> mobuls conversion */
#define gtime2mobuls(gtime)  ((abs((int)(gtime)) + 99) / 100)
#define mobuls2gtime(mobuls) ((int)(mobuls) * 100)

#endif
