/*
 * config.h
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#include <glib.h>

#include "items.h"

struct game_config {
    gint difficulty;
    gboolean wizard;
    gboolean no_autosave;
    gboolean show_version;
    char *name;
    char *gender;
    char *auto_pickup;
    char *savefile;
    char *stats;
    char *inifile;
#ifdef SDLPDCURSES
    int font_size;
#endif
};

gboolean parse_ini_file(const char *filename, struct game_config *config);

/* parse the command line */
void parse_commandline(int argc, char *argv[], struct game_config *config);

void parse_autopickup_settings(const char *settings, gboolean config[IT_MAX]);
char *compose_autopickup_settings(const gboolean config[IT_MAX]);

int parse_gender(const char gender);
char compose_gender(const int gender);
