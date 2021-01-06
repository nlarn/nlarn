/*
 * config.c
 * Copyright (C) 2009-2020 Joachim de Groot <jdegroot@web.de>
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
#include <string.h>
#include <glib.h>

#include "config.h"
#include "display.h"
#include "extdefs.h"
#include "items.h"
#include "player.h"

static const char *default_config_file =
    "[nlarn]\n"
    "# Difficulty (i.e. number of games won)\n"
    "difficulty=0\n"
    "\n"
    "# Set character's name\n"
    "name=\n"
    "\n"
    "# Choose the gender of your character (m/f)\n"
    "gender=\n"
    "\n"
    "# Choose the stats of your character\n"
    "# a Strong character\n"
    "# b Agile character\n"
    "# c Tough character\n"
    "# d Smart character\n"
    "# e Randomly pick one of the above\n"
    "# f Stats assigned randomly\n"
    "stats=\n"
    "\n"
    "# Item types to pick up automatically:\n"
    "# \" amulets\n"
    "# ' ammunition (ammunition once fired will be picked up anyway)\n"
    "# [ armour\n"
    "# + books\n"
    "# ] containers\n"
    "# * gems\n"
    "# $ money\n"
    "# ! potions\n"
    "# = rings\n"
    "# ? scrolls\n"
    "# ( weapons\n"
    "auto-pickup=\"+*$\n"
    "\n"
#ifdef SDLPDCURSES
    "# Font size for the game. Defaults to 18 when not defined.\n"
    "font-size=18\n"
    "\n"
#endif
    "# Disable automatic saving when switching a level. Saving the game is\n"
    "# enabled by default, disable when it's too slow on your computer\n"
    "no-autosave=false\n";

/* shared config cleanup helper */
void free_config(const struct game_config config)
{
    if (config.name)        g_free(config.name);
    if (config.gender)      g_free(config.gender);
    if (config.stats)       g_free(config.stats);
    if (config.auto_pickup) g_free(config.auto_pickup);
}

/* parse the command line */
void parse_commandline(int argc, char *argv[], struct game_config *config)
{
    const GOptionEntry entries[] =
    {
        { "name",        'n', 0, G_OPTION_ARG_STRING, &config->name,         "Set character's name", NULL },
        { "gender",      'g', 0, G_OPTION_ARG_STRING, &config->gender,       "Set character's gender (m/f)", NULL },
        { "stats",       's', 0, G_OPTION_ARG_STRING, &config->stats,        "Set character's stats (a-f)", NULL },
        { "auto-pickup", 'a', 0, G_OPTION_ARG_STRING, &config->auto_pickup,  "Item types to pick up automatically, e.g. '$*+'", NULL },
        { "no-autosave", 'N', 0, G_OPTION_ARG_NONE,   &config->no_autosave,  "Disable autosave", NULL },
        { "wizard",      'w', 0, G_OPTION_ARG_NONE,   &config->wizard,       "Enable wizard mode", NULL },
#ifdef SDLPDCURSES
        { "font-size",   'S', 0, G_OPTION_ARG_INT,    &config->font_size,   "Set font size", NULL },
#endif
        { "userdir",     'D', 0, G_OPTION_ARG_FILENAME, &config->userdir,    "Alternate directory for config file and saved games", NULL },
        { "highscores",  'h', 0, G_OPTION_ARG_NONE,   &config->show_scores,  "Show highscores and exit", NULL },
        { "version",     'v', 0, G_OPTION_ARG_NONE,   &config->show_version, "Show version information and exit", NULL },
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
        if (nlarn && nlarn->log) {
            log_add_entry(nlarn->log,
                "Unable to parse configuration file: %s\n",
                error->message);
        }
        g_clear_error(&error);
    }

    /* clean-up */
    g_key_file_free(ini_file);

    return success;
}

void write_ini_file(const char *filename, struct game_config *config)
{
    /* create configuration file from defaults */
    GKeyFile *kf = g_key_file_new();
    g_key_file_load_from_data(kf, default_config_file,
            strlen(default_config_file), G_KEY_FILE_KEEP_COMMENTS, NULL);

    if (config)
    {
        /* apply configuration to configuration file */
        g_key_file_set_integer(kf, "nlarn", "difficulty",  config->difficulty);
        g_key_file_set_string(kf,  "nlarn", "name",        config->name ? config->name : "");
        g_key_file_set_value(kf,   "nlarn", "gender",      config->gender ? config->gender : "");
        g_key_file_set_value(kf,   "nlarn", "stats",       config->stats ? config->stats : "");
        g_key_file_set_value(kf,   "nlarn", "auto-pickup", config->auto_pickup ? config->auto_pickup : "");
        g_key_file_set_boolean(kf, "nlarn", "no-autosave", config->no_autosave);
#ifdef SDLPDCURSES
        g_key_file_set_integer(kf, "nlarn", "font-size", config->font_size);
#endif
    }

    /* write config file contents to the give file */
    g_key_file_save_to_file(kf, filename, NULL);

    /* clean up */
    g_key_file_free(kf);
}

void parse_autopickup_settings(const char *settings, gboolean config[IT_MAX])
{
    /* reset configuration */
    memset(config, 0, sizeof(gboolean) * IT_MAX);

    /* parsing config has failed, not settings string given */
    if (!settings) return;

    for (guint idx = 0; idx < strlen(settings); idx++)
    {
        for (item_t it = IT_NONE; it < IT_MAX; it++)
        {
            if (settings[idx] == item_glyph(it))
            {
                config[it] = TRUE;
            }
        }
    }
}

char *compose_autopickup_settings(const gboolean config[IT_MAX])
{
    char *settings = g_malloc0(IT_MAX);

    int idx = 0;
    for (item_t it = IT_NONE; it < IT_MAX; it++)
    {
        if (config[it])
        {
            settings[idx] = item_glyph(it);
            idx++;
        }
    }

    return settings;
}

char *verbose_autopickup_settings(const gboolean config[IT_MAX])
{
    GString *settings = g_string_new(NULL);
    int count = 0;

    for (item_t it = IT_NONE; it < IT_MAX; it++)
    {
        if (config[it])
        {
            if (count)
                g_string_append(settings, ", ");

            g_string_append(settings, item_name_pl(it));
            count++;
        }
    }

    return g_string_free(settings, settings->len == 0);
}

int parse_gender(const char gender)
{
    char _gender = g_ascii_tolower(gender);

    switch (_gender)
    {
    case 'm':
        return PS_MALE;
        break;

    case 'f':
        return PS_FEMALE;
        break;

    default:
        return PS_NONE;
        break;
    }
}

char compose_gender(const int gender)
{
    switch(gender)
    {
        case PS_MALE:
            return 'm';
            break;
        case PS_FEMALE:
            return 'f';
            break;
        default:
            return ' ';
    }
}

void configure_defaults(const char *inifile)
{
    const char *undef = "not defined";
    const char *menu =
        "Configure game defaults\n"
        "\n"
        "  `lightgreen`a`end`) Character name         - %s\n"
        "  `lightgreen`b`end`) Character gender       - %s\n"
        "  `lightgreen`c`end`) Character stats        - %s\n"
        "  `lightgreen`d`end`) Configure auto-pickup  - %s\n"
        "  `lightgreen`e`end`) Autosave on map change - `yellow`%s`end`\n"
#ifdef SDLPDCURSES
        "  `lightgreen`f`end`) Configure font size    - `yellow`%d`end`\n"
#endif
        "\n"
        "Clear values with `lightgreen`A`end`-`lightgreen`D`end`\n";

    struct game_config config = {};
    parse_ini_file(inifile, &config);

    gboolean leaving = FALSE;

    while (!leaving)
    {
        /* name */
        char *nbuf = (config.name && strlen(config.name) > 0)
            ? g_strdup_printf("`yellow`%s`end`", config.name)
            : NULL;
        /* gender */
        char *gbuf = config.gender
            ? g_strdup_printf("`yellow`%s`end`",
                    player_sex_str[parse_gender(config.gender[0])])
            : NULL;
        /* stats */
        char *sbuf = (config.stats && strlen(config.stats) > 0)
            ? g_strdup_printf("`yellow`%s`end`", player_bonus_stat_desc[config.stats[0] - 'a'])
            : NULL;
        /* auto-pickup */
        gboolean autopickup[IT_MAX];
        parse_autopickup_settings(config.auto_pickup, autopickup);
        char *verboseap = verbose_autopickup_settings(autopickup);
        char *abuf = config.auto_pickup
            ? g_strdup_printf("`yellow`%s`end`", verboseap)
            : NULL;
        if (verboseap) g_free(verboseap);

        char *msg = g_strdup_printf(menu,
                nbuf ? nbuf : undef,
                gbuf ? gbuf : undef,
                sbuf ? sbuf : undef,
                abuf ? abuf: undef,
                config.no_autosave ? "no" : "yes"
#ifdef SDLPDCURSES
                , config.font_size
#endif
                );

        if (nbuf) g_free(nbuf);
        if (gbuf) g_free(gbuf);
        if (sbuf) g_free(sbuf);
        if (abuf) g_free(abuf);

        display_window *cwin = display_popup(COLS / 2 - 34, LINES / 2 - 6, 68,
                "Configure defaults", msg, 30);
        g_free(msg);

        int res = display_getch(cwin->window);
        switch (res)
        {
            /* default name */
            case 'a':
            {
                char *name = display_get_string("Choose default name",
                        "By what name shall all your charactes be called?",
                        config.name, 45);
                if (name)
                {
                    if (config.name) g_free(config.name);
                    config.name = name;
                }
                break;
            }

            /* clear name */
            case 'A':
                if (config.name) g_free(config.name);
                config.name = NULL;
                break;

            /* default gender */
            case 'b':
            {
                int gender = (display_get_yesno("Shall your future characters be "
                            "male or female?", "Choose default gender",
                            "Female", "Male") == TRUE)
                    ? PS_FEMALE : PS_MALE;

                if (config.gender) g_free(config.gender);
                config.gender = g_strdup_printf("%c", compose_gender(gender));
                break;
            }

            /* clear gender */
            case 'B':
                if (config.gender) g_free(config.gender);
                config.gender = NULL;
                break;

            /* default stats */
            case 'c':
            {
                char stats = player_select_bonus_stats();

                if (config.stats) free(config.stats);
                config.stats = g_strdup_printf("%c", stats);
                break;
            }

            /* clear stats */
            case 'C':
                if (config.stats) g_free(config.stats);
                config.stats = NULL;
                break;

            /* auto-pickup defaults */
            case 'd':
            {
                gboolean conf[IT_MAX] = {};
                if (config.auto_pickup)
                {
                    parse_autopickup_settings(config.auto_pickup, conf);
                    g_free(config.auto_pickup);
                }

                display_config_autopickup(conf);
                config.auto_pickup = compose_autopickup_settings(conf);

                break;
            }

            /* clear auto-pickup */
            case 'D':
                if (config.auto_pickup) g_free(config.auto_pickup);
                config.auto_pickup = NULL;
                break;

            /* autosave */
            case 'e':
                config.no_autosave = !config.no_autosave;
                break;

            /* font size */
#ifdef SDLPDCURSES
            case 'f':
                {
                    char *cval = g_strdup_printf("%d", config.font_size);
                    char *nval = display_get_string("Default font size",
                        "Font size (6 - 48): ", cval, 2);
                    g_free(cval);

                    if (nval)
                    {
                        int val = atoi(nval);
                        g_free(nval);

                        if (val >= 6 && val <= 48)
                        {
                            config.font_size = val;
                        }
                        else
                        {
                            display_show_message("Error", "Invalid font size", 0);
                        }
                    }
                }
                break;
#endif

            case KEY_ESC:
                leaving = TRUE;
                break;

            default:
                /* ignore input */
                break;
        }
        display_window_destroy(cwin);
    }

    /* write back modified config */
    write_ini_file(inifile, &config);

    /* clean up */
    free_config(config);
}

void config_increase_difficulty(const char *inifile, const int new_difficulty)
{
    struct game_config config = {};
    parse_ini_file(inifile, &config);

    config.difficulty = new_difficulty;

    /* write back modified config */
    write_ini_file(inifile, &config);

    /* clean up */
    free_config(config);
}
