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
#include "monsters.h"
#include "player.h"

/* the world as we know it */
typedef struct game {
		player *p;					/* the player */
		level *levels[LEVEL_MAX];	/* the dungeon */
		time_t time_start;			/* start time */
		time_t gtime;				/* game time */
        gint difficulty;  /* game difficulty */

        gchar *basedir;
        gchar *libdir;
        gchar *mesgfile;
        gchar *helpfile;
        gchar *mazefile;
        gchar *fortunes;

        /* flags */
        unsigned
            wizard: 1; /* wizard mode */
} game;

/* direction of movement */
/* ordered by number keys */
typedef enum direction {
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

int game_save(game *g, char *filename);
game *game_load(char *filename);
game *game_new(int argc, char *argv[]);
int game_destroy(game *g);

void game_spin_the_wheel(game *g, int times);

/* macros */

#define game_difficulty(g) (((game *)(g))->difficulty)
#define game_wizardmode(g) (((game *)(g))->wizard)
#define game_turn(g) (((game *)(g))->gtime)

#define game_dir(g) (((game *)(g))->basedir)
#define game_lib(g) (((game *)(g))->libdir)

#define game_mesgfile(g) (((game *)(g))->mesgfile)
#define game_helpfile(g) (((game *)(g))->helpfile)
#define game_mazefile(g) (((game *)(g))->mazefile)
#define game_fortunes(g) (((game *)(g))->fortunes)

/* gtime <> mobuls conversion */
#define gtime2mobuls(gtime) ((int)(gtime) / 100)
#define mobuls2gtime(mobuls) ((int)(mobuls) * 100)

#endif
