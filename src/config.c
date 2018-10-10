/*
 * config.c
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

#include <stdlib.h>
#include <glib.h>

#include "config.h"

/* parse the command line */
void parse_commandline(int argc, char *argv[], struct game_config *config)
{
    const GOptionEntry entries[] =
    {
        { "name",        'n', 0, G_OPTION_ARG_STRING, &config->name,         "Set character's name", NULL },
        { "gender",      'g', 0, G_OPTION_ARG_STRING, &config->gender,       "Set character's gender (m/f)", NULL },
        { "stats",       's', 0, G_OPTION_ARG_STRING, &config->stats,        "Set character's stats (a-f)", NULL },
        { "auto-pickup", 'a', 0, G_OPTION_ARG_STRING, &config->auto_pickup,  "Item types to pick up automatically, e.g. '$*+'", NULL },
        { "difficulty",  'd', 0, G_OPTION_ARG_INT,    &config->difficulty,   "Set difficulty", NULL },
        { "no-autosave", 'N', 0, G_OPTION_ARG_NONE,   &config->no_autosave,  "Disable autosave", NULL },
        { "version",     'v', 0, G_OPTION_ARG_NONE,   &config->show_version, "Show version information and exit", NULL },
        { "wizard",      'w', 0, G_OPTION_ARG_NONE,   &config->wizard,       "Enable wizard mode", NULL },
        { "config",      'c', 0, G_OPTION_ARG_FILENAME, &config->inifile,    "Alternate configuration file name", NULL },
#ifdef DEBUG
        { "savefile",    'f', 0, G_OPTION_ARG_FILENAME, &config->savefile,   "Save file to restore", NULL },
#endif
#ifdef SDLPDCURSES
        { "font-size",   'S', 0, G_OPTION_ARG_INT,    &config->font_size,   "Set font size", NULL },
#endif
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    GError *error = NULL;
    GOptionContext *context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_printerr("option parsing failed: %s\n", error->message);

        exit (EXIT_FAILURE);
    }
    g_option_context_free(context);
}

gboolean parse_ini_file(const char *filename, struct game_config *config)
{
    /* ini file handling */
    GKeyFile *ini_file = g_key_file_new();
    GError *error = NULL;
    gboolean success;

    /* config file defined on the command line precedes over the default */
    g_key_file_load_from_file(ini_file, filename,
            G_KEY_FILE_NONE, &error);

    if ((success = (!error)))
    {
        /* ini file has been found, get values */
        /* clear error after each attempt as values need not to be defined */
        int difficulty = g_key_file_get_integer(ini_file, "nlarn", "difficulty", &error);
        if (!config->difficulty && !error) config->difficulty = difficulty;
        g_clear_error(&error);

        gboolean no_autosave = g_key_file_get_boolean(ini_file, "nlarn", "no-autosave", &error);
        if (!config->no_autosave && !error) config->no_autosave = no_autosave;
        g_clear_error(&error);

        char *name = g_key_file_get_string(ini_file, "nlarn", "name", &error);
        if (!config->name && !error) config->name = name;
        g_clear_error(&error);

        char *gender = g_key_file_get_string(ini_file, "nlarn", "gender", &error);
        if (!config->gender && !error) config->gender = gender;
        g_clear_error(&error);

        char *auto_pickup = g_key_file_get_string(ini_file, "nlarn", "auto-pickup", &error);
        if (!config->auto_pickup && !error) config->auto_pickup = auto_pickup;
        g_clear_error(&error);

        char *stats = g_key_file_get_string(ini_file, "nlarn", "stats", &error);
        if (!config->stats && !error) config->stats = stats;
        g_clear_error(&error);

#ifdef SDLPDCURSES
        int font_size = g_key_file_get_integer(ini_file, "nlarn", "font-size", &error);
        if (!config->font_size && !error) config->font_size = font_size;
        g_clear_error(&error);
#endif
    }
    else
    {
        /* File not found. Never mind but clean up the mess */
        g_clear_error(&error);
    }

    /* clean-up */
    g_key_file_free(ini_file);

    return success;
}


