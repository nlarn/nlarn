/*
 * config.c
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

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
    "# a semicolon-separated list of the following item type names\n"
    "# IT_AMULET, IT_AMMO, IT_ARMOUR, IT_BOOK, IT_CONTAINER, IT_GEM,\n"
    "# IT_GOLD, IT_POTION, IT_RING, IT_SCROLL, IT_WEAPON\n"
    "# (ammunition once fired will be picked up anyway)\n"
    "auto-pickup=IT_AMULET;IT_BOOK;IT_GEM;IT_GOLD;\n"
    "\n"
    "# UI Colour scheme\n"
    "# one of\n"
    "#  - TRADITIONAL\n"
    "#  - RETROBOX\n"
    "#  - STASIS\n"
    "#  - SOURLICK\n"
    "#  - FODDER\n"
    "#  - MELLOW\n"
    "colours=TRADITIONAL\n"
    "\n"
#ifdef SDLPDCURSES
    "# Font size for the game. Defaults to 18 when not defined.\n"
    "font-size=18\n"
    "\n"
    "# Enable full screen mode for the game. Defaults to false.\n"
    "fullscreen=false\n"
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
    if (config.auto_pickup) g_strfreev(config.auto_pickup);
}

/* parse the command line */
void parse_commandline(int argc, char *argv[], struct game_config *config)
{
    const GOptionEntry entries[] =
    {
        { "no-autosave", 'N', 0, G_OPTION_ARG_NONE,   &config->no_autosave,  "Disable autosave", NULL },
        { "wizard",      'w', 0, G_OPTION_ARG_NONE,   &config->wizard,       "Enable wizard mode", NULL },
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

/* detect an auto-pickup configuration written by an older version of
   the game, which stored the item glyphs in a single string */
static bool autopickup_uses_glyphs(char *const *settings)
{
    return (settings != NULL && settings[0] != NULL
            && !g_str_has_prefix(settings[0], "IT_"));
}

/* convert an old style auto-pickup configuration into
   a list of item type names */
static char **convert_autopickup_glyphs(char **settings)
{
    bool config[IT_MAX] = { 0 };

    for (const char *glyph = settings[0]; *glyph != '\0'; glyph++)
    {
        for (item_t it = IT_NONE; it < IT_MAX; it++)
        {
            if (*glyph == item_glyph(it))
                config[it] = true;
        }
    }

    g_strfreev(settings);

    return compose_autopickup_settings(config);
}

bool parse_ini_file(const char *filename, struct game_config *config)
{
    /* ini file handling */
    GKeyFile *ini_file = g_key_file_new();
    GError *error = NULL;
    bool success;

    /* the file needs to be rewritten, e.g. after converting values
       written by an older version of the game */
    bool update_config_file = false;

    /* config file defined on the command line precedes over the default */
    g_key_file_load_from_file(ini_file, filename,
            G_KEY_FILE_NONE, &error);

    if ((success = (!error)))
    {
        /* ini file has been found, get values */
        /* clear error after each attempt as values may not be defined */
        int difficulty = g_key_file_get_integer(ini_file, "nlarn", "difficulty", &error);
        if (!error) config->difficulty = difficulty;
        g_clear_error(&error);

        bool no_autosave = g_key_file_get_boolean(ini_file, "nlarn", "no-autosave", &error);
        if (!config->no_autosave && !error) config->no_autosave = no_autosave;
        g_clear_error(&error);

        char *name = g_key_file_get_string(ini_file, "nlarn", "name", &error);
        if (!error) config->name = name;
        g_clear_error(&error);

        char *gender = g_key_file_get_string(ini_file, "nlarn", "gender", &error);
        if (!error) config->gender = gender;
        g_clear_error(&error);

        char **auto_pickup = g_key_file_get_string_list(ini_file, "nlarn",
                "auto-pickup", NULL, &error);
        if (!error)
        {
            /* Configurations written by older versions stored the item
               glyphs in a single string; convert those to a list of item
               type names and update the configuration file. */
            if (autopickup_uses_glyphs(auto_pickup))
            {
                config->auto_pickup = convert_autopickup_glyphs(auto_pickup);
                update_config_file = true;
            }
            else
            {
                config->auto_pickup = auto_pickup;
            }
        }
        g_clear_error(&error);

        char *stats = g_key_file_get_string(ini_file, "nlarn", "stats", &error);
        if (!error) config->stats = stats;
        g_clear_error(&error);

        char *colour_scheme = g_key_file_get_string(ini_file, "nlarn", "colours", &error);
        if (!error) config->colour_scheme = ui_colour_scheme_value(colour_scheme);
        g_clear_error(&error);

#ifdef SDLPDCURSES
        /* Default to a sane font size after switching from the ncurses build */
        int font_size = g_key_file_get_integer(ini_file, "nlarn", "font-size", &error);
        if (!config->font_size && !error)
            config->font_size = font_size;
        else
            config->font_size = 18;;
        g_clear_error(&error);

        bool fullscreen = g_key_file_get_boolean(ini_file, "nlarn", "fullscreen", &error);
        if (!error) config->fullscreen = fullscreen;
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

    /* persist converted legacy values */
    if (update_config_file)
        write_ini_file(filename, config);

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
        if (config->auto_pickup)
            g_key_file_set_string_list(kf, "nlarn", "auto-pickup",
                    (const gchar * const *)config->auto_pickup,
                    g_strv_length(config->auto_pickup));
        else
            g_key_file_set_value(kf, "nlarn", "auto-pickup", "");
        g_key_file_set_boolean(kf, "nlarn", "no-autosave", config->no_autosave);
        g_key_file_set_value(kf,   "nlarn", "colours",     ui_colour_scheme_string(config->colour_scheme));
#ifdef SDLPDCURSES
        g_key_file_set_integer(kf, "nlarn", "font-size",   config->font_size);
        g_key_file_set_boolean(kf, "nlarn", "fullscreen",  config->fullscreen);
#endif
    }

    /* write config file contents to the give file */
    g_key_file_save_to_file(kf, filename, NULL);

    /* clean up */
    g_key_file_free(kf);
}

void parse_autopickup_settings(char *const *settings, bool config[IT_MAX])
{
    /* reset configuration */
    memset(config, 0, sizeof(bool) * IT_MAX);

    /* parsing config has failed, no settings given */
    if (!settings) return;

    for (guint idx = 0; settings[idx] != NULL; idx++)
    {
        /* item_t_value() returns IT_NONE for unknown item type names */
        item_t it = item_t_value(settings[idx]);

        if (it > IT_NONE && it < IT_MAX)
            config[it] = true;
    }
}

char **compose_autopickup_settings(const bool config[IT_MAX])
{
    /* a NULL-terminated array of item type names */
    char **settings = g_new0(char *, IT_MAX + 1);

    int idx = 0;
    for (item_t it = IT_NONE; it < IT_MAX; it++)
    {
        if (config[it])
        {
            settings[idx] = g_strdup(item_t_string(it));
            idx++;
        }
    }

    return settings;
}

char *verbose_autopickup_settings(const bool config[IT_MAX])
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

    case 'f':
        return PS_FEMALE;

    default:
        return PS_NONE;
    }
}

char compose_gender(const int gender)
{
    switch(gender)
    {
        case PS_MALE:
            return 'm';
        case PS_FEMALE:
            return 'f';
        default:
            return ' ';
    }
}

void configure_defaults(const char *inifile)
{
    const char *undef = _("not defined");
    const char *menu =
        _("\n"
          "  `KEY`a`end`) Character name         - %s\n"
          "  `KEY`b`end`) Character gender       - %s\n"
          "  `KEY`c`end`) Character stats        - %s\n"
          "  `KEY`d`end`) Configure auto-pickup  - %s\n"
          "  `KEY`e`end`) Autosave on map change - `EMPH`%s`end`\n"
          "  `KEY`f`end`) Colour scheme          - `EMPH`%s`end`\n"
#ifdef SDLPDCURSES
          "  `KEY`g`end`) Configure font size    - `EMPH`%d`end`\n"
          "  `KEY`h`end`) Full screen mode       - `EMPH`%s`end`\n"
#endif
          "\n"
          "Clear values with `KEY`A`end`-`KEY`F`end`. "
          "Return to the main menu with `KEY`ESC`end`.\n");

    bool leaving = false;

    while (!leaving)
    {
        /* name */
        char *nbuf = (config.name && strlen(config.name) > 0)
            ? g_strdup_printf("`EMPH`%s`end`", config.name)
            : NULL;
        /* gender */
        char *gbuf = config.gender
            ? g_strdup_printf("`EMPH`%s`end`",
                    _(player_sex_str[parse_gender(config.gender[0])]))
            : NULL;
        /* stats */
        char *sbuf = (config.stats && strlen(config.stats) > 0)
            ? g_strdup_printf("`EMPH`%s`end`", _(player_bonus_stat_desc[config.stats[0] - 'a']))
            : NULL;
        /* auto-pickup */
        bool autopickup[IT_MAX];
        parse_autopickup_settings(config.auto_pickup, autopickup);
        char *verboseap = verbose_autopickup_settings(autopickup);
        char *abuf = config.auto_pickup
            ? g_strdup_printf("`EMPH`%s`end`", verboseap)
            : NULL;
        if (verboseap) g_free(verboseap);
        /* UI colour scheme */
        char *lucss = g_ascii_strdown(ui_colour_scheme_string(config.colour_scheme), -1);

        char *msg = g_strdup_printf(menu,
                nbuf ? nbuf : undef,
                gbuf ? gbuf : undef,
                sbuf ? sbuf : undef,
                abuf ? abuf : undef,
                config.no_autosave ? _("no") : _("yes"),
                lucss
#ifdef SDLPDCURSES
                , config.font_size
                , config.fullscreen ? _("yes") : _("no")
#endif
                );

        if (nbuf) g_free(nbuf);
        if (gbuf) g_free(gbuf);
        if (sbuf) g_free(sbuf);
        if (abuf) g_free(abuf);
        g_free(lucss);

        display_window *cwin = display_popup(COLS / 2 - 34, LINES / 2 - 6, 68,
                _("Configure game defaults"), msg, 30);
        g_free(msg);

        int res = display_getch(cwin->window);
        switch (res)
        {
            /* default name */
            case 'a':
            {
                char *name = display_get_string(_("Choose default name"),
                        _("By what name shall all your characters be called?"),
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
                int gender = (display_get_yesno(_("Shall your future characters be "
                            "male or female?"), _("Choose default gender"),
                            _("Female"), _("Male")) == true)
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
                bool conf[IT_MAX] = {};
                if (config.auto_pickup)
                {
                    parse_autopickup_settings(config.auto_pickup, conf);
                    g_strfreev(config.auto_pickup);
                }

                display_config_autopickup(conf);
                config.auto_pickup = compose_autopickup_settings(conf);

                break;
            }

            /* clear auto-pickup */
            case 'D':
                if (config.auto_pickup) g_strfreev(config.auto_pickup);
                config.auto_pickup = NULL;
                break;

            /* autosave */
            case 'e':
                config.no_autosave = !config.no_autosave;
                break;

            /* colour scheme */
            case 'f':
                {
                    GString *csmsg = g_string_new(_("Select UI colour scheme:\n\n"));
                    for (int i = 0; i < UI_COLOUR_SCHEME_MAX; i++)
                    {
                        char *lucss = g_ascii_strdown(ui_colour_scheme_string(i), -1);
                        g_string_append_printf(csmsg, " `KEY`%c`end`) %s\n", i + 'a', lucss);
                        g_free(lucss);
                    }

                    char ret = display_show_message(_("UI colours"), csmsg->str, 0);

                    if (ret >= 'a' && ret < 'a' + UI_COLOUR_SCHEME_MAX)
                    {
                        config.colour_scheme = ret - 'a';
                        colours_init(config.colour_scheme);
                    }

                    g_string_free(csmsg, true);
                }
                break;

            /* default colour scheme */
            case 'F':
                config.colour_scheme = TRADITIONAL;
                break;

#ifdef SDLPDCURSES
            /* font size */
            case 'g':
                {
                    char *cval = g_strdup_printf("%d", config.font_size);
                    char *nval = display_get_string(_("Default font size"),
                        _("Font size (6 - 48): "), cval, 2);
                    g_free(cval);

                    if (nval)
                    {
                        int val = atoi(nval);
                        g_free(nval);

                        if (val >= 6 && val <= 48)
                        {
                            config.font_size = val;
                            display_change_font();
                        }
                        else
                        {
                            display_show_message(_("Error"), _("Invalid font size"), 0);
                        }
                    }
                }
                break;

            /* fullscreen */
            case 'h':
                display_toggle_fullscreen(true);
                break;
#endif

            case KEY_ESC:
                leaving = true;
                break;

            default:
                /* ignore input */
                break;
        }
        display_window_destroy(cwin);
    }

    /* write modified config */
    write_ini_file(inifile, &config);
}
