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
    char *stats;
    char *userdir;
#ifdef SDLPDCURSES
    int font_size;
#endif
};

/* configuration file reading and writing */
gboolean parse_ini_file(const char *filename, struct game_config *config);
void write_ini_file(const char *filename, struct game_config *config);

/* parse the command line */
void parse_commandline(int argc, char *argv[], struct game_config *config);

void parse_autopickup_settings(const char *settings, gboolean config[IT_MAX]);
char *compose_autopickup_settings(const gboolean config[IT_MAX]);

/**
 * @brief Return a comma separated list of all selected item types
 *
 * @param a boolean array (size IT_MAX)
 *
 * @return NULL if no item type is selected, otherwise a comma-separated
 *         list of item type names
 */
char *verbose_autopickup_settings(const gboolean config[IT_MAX]);

int parse_gender(const char gender);
char compose_gender(const int gender);

/* configure game defaults */
void configure_defaults(const char *inifile);
