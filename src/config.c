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

/* Wrap the first character of s in `KEY` markup so display_menu() uses it
   as the option's hotkey. The caller owns the returned string. */
static char *config_hotkey(const char *s)
{
    if (s == NULL || *s == '\0')
        return g_strdup(s ? s : "");

    const char *rest = g_utf8_next_char(s);
    return g_strdup_printf("`KEY`%.*s`end`%s", (int)(rest - s), s, rest);
}

void configure_defaults(const char *inifile)
{
    const char *undef = _("not defined");
    const char *title = _("Configure game defaults");
    const char *message =
        _("Options left \"not defined\" are chosen when a new game begins.");

    enum config_option
    {
        CO_NAME, CO_GENDER, CO_STATS, CO_PICKUP, CO_AUTOSAVE, CO_COLOUR,
#ifdef SDLPDCURSES
        CO_FONT, CO_FULLSCREEN,
#endif
        CO_COUNT
    };

    static const char *const names[] =
    {
        N_("Character name"),
        N_("Character gender"),
        N_("Character stats"),
        N_("Configure auto-pickup"),
        N_("Autosave on map change"),
        N_("Colour scheme"),
#ifdef SDLPDCURSES
        N_("Configure font size"),
        N_("Full screen mode"),
#endif
    };

    bool leaving = false;
    /* keep the selection on the option that was just changed */
    guint current = 0;
    while (!leaving)
    {
        const char *labels[CO_COUNT];
        char *label_buf[CO_COUNT];
        const char *details[CO_COUNT];
        char *detail_buf[CO_COUNT];

        for (int i = 0; i < CO_COUNT; i++)
        {
            label_buf[i] = g_strdup_printf("`KEY`%c`end`) %s", 'a' + i, _(names[i]));
            labels[i] = label_buf[i];
        }

        /* the details panel shows each setting's current value */
        detail_buf[CO_NAME] = g_strdup((config.name && *config.name)
                ? config.name : undef);
        detail_buf[CO_GENDER] = g_strdup(config.gender
                ? _(player_sex_str[parse_gender(config.gender[0])]) : undef);
        detail_buf[CO_STATS] = g_strdup((config.stats && *config.stats)
                ? _(player_bonus_stat_desc[config.stats[0] - 'a']) : undef);

        bool ap[IT_MAX];
        parse_autopickup_settings(config.auto_pickup, ap);
        char *vap = verbose_autopickup_settings(ap);
        detail_buf[CO_PICKUP] = g_strdup((config.auto_pickup && vap) ? vap : undef);
        if (vap) g_free(vap);

        detail_buf[CO_AUTOSAVE] = g_strdup(config.no_autosave ? _("no") : _("yes"));
        detail_buf[CO_COLOUR] = g_ascii_strdown(
                ui_colour_scheme_string(config.colour_scheme), -1);
#ifdef SDLPDCURSES
        detail_buf[CO_FONT] = g_strdup_printf("%d", config.font_size);
        detail_buf[CO_FULLSCREEN] = g_strdup(config.fullscreen ? _("yes") : _("no"));
#endif

        for (int i = 0; i < CO_COUNT; i++)
            details[i] = detail_buf[i];

        int sel = display_menu(title, message, labels, NULL, details, CO_COUNT, current);

        for (int i = 0; i < CO_COUNT; i++)
        {
            g_free(label_buf[i]);
            g_free(detail_buf[i]);
        }

        /* remember the option so the menu reopens on it */
        if (sel >= 0)
            current = (guint)sel;

        switch (sel)
        {
        case -1:
            leaving = true;
            break;

        /* character name: enter one or reset to "not defined" */
        case CO_NAME:
        {
            const char *opts[] = {
                config_hotkey(_("Enter a name")),
                config_hotkey(undef),
            };
            int c = display_menu(_(names[CO_NAME]),
                    _("By what name shall all your characters be called?"),
                    opts, NULL, NULL, 2, 0);
            if (c == 0)
            {
                char *name = display_get_string(_("Choose default name"),
                        _("By what name shall all your characters be called?"),
                        config.name, 45);
                if (name)
                {
                    if (config.name) g_free(config.name);
                    config.name = name;
                }
            }
            else if (c == 1)
            {
                if (config.name) g_free(config.name);
                config.name = NULL;
            }
            g_free((char *)opts[0]);
            g_free((char *)opts[1]);
            break;
        }

        /* character gender */
        case CO_GENDER:
        {
            const char *opts[] = {
                config_hotkey(_("Female")),
                config_hotkey(_("Male")),
                config_hotkey(undef),
            };
            const guint cur = config.gender
                ? (parse_gender(config.gender[0]) == PS_FEMALE ? 0 : 1) : 2;
            int c = display_menu(_(names[CO_GENDER]),
                    _("Shall your future characters be male or female?"),
                    opts, NULL, NULL, 3, cur);
            if (c == 0 || c == 1)
            {
                if (config.gender) g_free(config.gender);
                config.gender = g_strdup_printf("%c",
                        compose_gender(c == 0 ? PS_FEMALE : PS_MALE));
            }
            else if (c == 2)
            {
                if (config.gender) g_free(config.gender);
                config.gender = NULL;
            }
            g_free((char *)opts[0]);
            g_free((char *)opts[1]);
            g_free((char *)opts[2]);
            break;
        }

        /* character stats: the build selection includes "not defined" */
        case CO_STATS:
        {
            int stats = player_select_bonus_stats(true);
            if (stats > 0)
            {
                /* a build preset was chosen */
                if (config.stats) g_free(config.stats);
                config.stats = g_strdup_printf("%c", stats);
            }
            else if (stats == 0)
            {
                /* "not defined" was chosen */
                if (config.stats) g_free(config.stats);
                config.stats = NULL;
            }
            /* stats < 0: aborted, keep the current value */
            break;
        }

        /* auto-pickup: selecting nothing leaves it undefined */
        case CO_PICKUP:
        {
            bool conf[IT_MAX] = {};
            if (config.auto_pickup)
            {
                parse_autopickup_settings(config.auto_pickup, conf);
                g_strfreev(config.auto_pickup);
                config.auto_pickup = NULL;
            }

            display_config_autopickup(conf);

            bool any = false;
            for (item_t it = IT_NONE; it < IT_MAX; it++)
                any = any || conf[it];

            config.auto_pickup = any ? compose_autopickup_settings(conf) : NULL;
            break;
        }

        case CO_AUTOSAVE:
            config.no_autosave = !config.no_autosave;
            break;

        /* colour scheme: the schemes are the choices, TRADITIONAL is the
           default */
        case CO_COLOUR:
        {
            const char *opts[UI_COLOUR_SCHEME_MAX];
            char *buf[UI_COLOUR_SCHEME_MAX];
            for (int i = 0; i < UI_COLOUR_SCHEME_MAX; i++)
            {
                char *low = g_ascii_strdown(ui_colour_scheme_string(i), -1);
                buf[i] = g_strdup_printf("`KEY`%c`end`) %s", 'a' + i, low);
                g_free(low);
                opts[i] = buf[i];
            }
            int c = display_menu(_(names[CO_COLOUR]),
                    _("Select a UI colour scheme:"), opts, NULL, NULL,
                    UI_COLOUR_SCHEME_MAX, config.colour_scheme);
            if (c >= 0)
            {
                config.colour_scheme = c;
                colours_init(config.colour_scheme);
            }
            for (int i = 0; i < UI_COLOUR_SCHEME_MAX; i++)
                g_free(buf[i]);
            break;
        }

#ifdef SDLPDCURSES
        case CO_FONT:
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
            break;
        }

        case CO_FULLSCREEN:
            display_toggle_fullscreen(true);
            break;
#endif
        }
    }

    /* write modified config */
    write_ini_file(inifile, &config);
}
