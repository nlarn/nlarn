/*
 * nlarn.c
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

/* setres[gu]id() from unistd.h are only defined when this is set
   *before* including it. Another header in the list seems to do so
   before we include it here.*/
#ifdef __linux__
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
#endif

#include <ctype.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#ifdef __unix
# include <signal.h>
# include <unistd.h>
# include <sys/file.h>
# include <sys/stat.h>
#endif

#include "config.h"
#include "container.h"
#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "pathfinding.h"
#include "player.h"
#include "scoreboard.h"
#include "sobjects.h"
#include "traps.h"
#include "extdefs.h"

/* see https://stackoverflow.com/q/36764885/1519878 */
#define _STR(x) #x
#define STR(x) _STR(x)

/* version string */
const char *nlarn_version = STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH) GITREV;

/* empty scoreboard description */
const char *room_for_improvement = "\n...room for improvement...\n";

/* path and file name constants*/
static const char *default_lib_dir = "/usr/share/nlarn";
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
static const char *default_var_dir = "/var/games/nlarn";
#endif
static const char *mesgfile = "nlarn.msg";
static const char *helpfile = "nlarn.hlp";
static const char *mazefile = "maze";
static const char *fortunes = "fortune";
static const char *highscores = "highscores";
static const char *config_file = "nlarn.ini";
static const char *save_file = "nlarn.sav";

/* global game object */
game *nlarn = NULL;

/* the game settings */
struct game_config config = {};

/* death jump buffer - used to return to the main loop when the player has died */
jmp_buf nlarn_death_jump;

static gboolean adjacent_corridor(position pos, char mv);

#ifdef __unix
static void nlarn_signal_handler(int signo);
#endif

static const gchar *nlarn_userdir()
{
    static gchar *userdir = NULL;

    if (userdir == NULL)
    {
        if (config.userdir)
        {
            if (!g_file_test(config.userdir, G_FILE_TEST_IS_DIR))
            {
                g_printerr("Supplied user directory \"%s\" does not exist.",
                        config.userdir);

                exit(EXIT_FAILURE);
            }
            else
            {
                userdir = g_strdup(config.userdir);
            }
        }
        else
        {
#ifdef G_OS_WIN32
            userdir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(),
                    "nlarn", NULL);
#else
            userdir = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(),
                    ".nlarn", NULL);
#endif
        }
    }

    return userdir;
}

/* initialize runtime environment */
static void nlarn_init(int argc, char *argv[])
{
    /* determine paths and file names */
    /* base directory for a local install */
    g_autofree char *basedir = g_path_get_dirname(argv[0]);

    /* try to use the directory below the binary's location first */
    nlarn_libdir = g_build_path(G_DIR_SEPARATOR_S, basedir, "lib", NULL);

    if (!g_file_test(nlarn_libdir, G_FILE_TEST_IS_DIR))
    {
        /* local lib directory could not be found, try the system wide directory. */
#ifdef __APPLE__
        char *rellibdir = g_build_path(G_DIR_SEPARATOR_S, basedir,
                                       "../Resources", NULL);
#endif
        if (g_file_test(default_lib_dir, G_FILE_TEST_IS_DIR))
        {
            /* system-wide data directory exists */
            /* string has to be dup'd as it is feed in the end */
            nlarn_libdir = g_strdup((char *)default_lib_dir);
        }
#ifdef __APPLE__
        else if (g_file_test(rellibdir, G_FILE_TEST_IS_DIR))
        {
            /* program seems to be installed relocatable */
            nlarn_libdir = g_strdup(rellibdir);
        }
#endif
        else
        {
            g_printerr("Could not find game library directory.\n\n"
                       "Paths I've tried:\n"
                       " * %s\n"
#ifdef __APPLE__
                       " * %s\n"
#endif
                       " * %s\n\n"
                       "Please reinstall the game.\n",
                       nlarn_libdir,
#ifdef __APPLE__
                       rellibdir,
#endif
                       default_lib_dir);

            exit(EXIT_FAILURE);
        }
    }

    nlarn_mesgfile = g_build_filename(nlarn_libdir, mesgfile, NULL);
    nlarn_helpfile = g_build_filename(nlarn_libdir, helpfile, NULL);
    nlarn_mazefile = g_build_filename(nlarn_libdir, mazefile, NULL);
    nlarn_fortunes = g_build_filename(nlarn_libdir, fortunes, NULL);

#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    /* highscore file handling for SETGID builds */
    gid_t realgid;
    uid_t realuid;

    /* assemble the scoreboard filename */
    nlarn_highscores = g_build_path(G_DIR_SEPARATOR_S, default_var_dir,
                                              highscores, NULL);

    /* Open the scoreboard file. */
    if ((scoreboard_fd = open(nlarn_highscores, O_RDWR)) == -1)
    {
        perror("Could not open scoreboard file");
        exit(EXIT_FAILURE);
    }

    /* Figure out who we really are. */
    realgid = getgid();
    realuid = getuid();

    /* This is where we drop our setuid/setgid privileges. */
#ifdef __linux__
    if (setresgid(-1, realgid, realgid) != 0) {
#else
    if (setregid(-1, realgid) != 0) {
#endif
        perror("Could not drop setgid privileges");
        exit(EXIT_FAILURE);
    }

#ifdef __linux__
    if (setresuid(-1, realuid, realuid) != 0) {
#else
    if (setreuid(-1, realuid) != 0) {
#endif
        perror("Could not drop setuid privileges");
        exit(EXIT_FAILURE);
    }

    /* ensure the scoreboard file descriptor is closed at exit */
    atexit(scoreboard_close_fd);
#else
    /* highscore file handling for non-SETGID builds -
       store high scores in the same directory as the configuation */
    nlarn_highscores = g_build_filename(nlarn_userdir(), highscores, NULL);
#endif

    /* parse the command line options */
    parse_commandline(argc, argv, &config);

    /* show version information */
    if (config.show_version) {
        g_printf("NLarn version %s, built on %s.\n\n", nlarn_version, __DATE__);
        g_printf("Game lib directory:\t%s\n", nlarn_libdir);
        g_printf("Game savefile version:\t%d\n", SAVEFILE_VERSION);

        exit(EXIT_SUCCESS);
    }

    /* show highscores */
    if (config.show_scores) {
        GList *scores = scores_load();
        g_autofree char *s = scores_to_string(scores, NULL);

        g_printf("NLarn Hall of Fame\n==================\n%s",
                scores ? s : room_for_improvement);
        scores_destroy(scores);

        exit(EXIT_SUCCESS);
    }

    /* verify that user directory exists */
    if (!g_file_test(nlarn_userdir(), G_FILE_TEST_IS_DIR))
    {
        /* directory is missing -> create it */
        int ret = g_mkdir(nlarn_userdir(), 0755);

        if (ret == -1)
        {
            /* creating the directory failed */
            g_printerr("Failed to create directory %s.", nlarn_userdir());
            exit(EXIT_FAILURE);
        }
    }

    /* try loading settings from the default configuration file */
    nlarn_inifile = g_build_path(G_DIR_SEPARATOR_S, nlarn_userdir(),
            config_file, NULL);

    /* write a default configuration file, if none exists */
    if (!g_file_test(nlarn_inifile, G_FILE_TEST_IS_REGULAR))
    {
        write_ini_file(nlarn_inifile, NULL);
    }

    /* try to load settings from the configuration file */
    parse_ini_file(nlarn_inifile, &config);

    /* initialise the display - must not happen before this point
       otherwise displaying the command line help fails */
    display_init();

    /* call display_shutdown when terminating the game */
    atexit(display_shutdown);

    /* assemble the save file name */
    nlarn_savefile = g_build_path(G_DIR_SEPARATOR_S, nlarn_userdir(),
            save_file, NULL);

    /* set the console shutdown handler */
#ifdef __unix
    signal(SIGTERM, nlarn_signal_handler);
    signal(SIGHUP, nlarn_signal_handler);
#endif
}

static void mainloop()
{
    /* count of moves used by last action */
    int moves_count = 0;

    /* used to read in e.g. the help file */
    gchar *strbuf;

    /* position to examine / to travel to */
    position pos = pos_invalid;

    /* position chosen for auto travel, allowing to continue travel */
    position cpos = pos_invalid;

    char run_cmd = 0;
    int ch = 0;
    gboolean adj_corr = false;
    guint end_resting = 0;

    /* main event loop
       keep running until the game object was destroyed */
    while (nlarn)
    {
        /* repaint screen */
        display_paint_screen(nlarn->p);

        if (pos_valid(pos))
        {
            /* travel mode */

            /* check if travel mode shall be aborted:
               attacked or fell through trap door */
            if (nlarn->p->attacked || player_adjacent_monster(nlarn->p, false)
                || Z(pos) != Z(nlarn->p->pos))
            {
                pos = pos_invalid;
            }
            else if (pos_adjacent(nlarn->p->pos, pos))
            {
                /* the target has almost been reached. This is the last move. */
                moves_count = player_move(nlarn->p, pos_dir(nlarn->p->pos, pos), true);
                /* reset the target position */
                pos = cpos = pos_invalid;
            }
            else
            {
                /* find a path to the destination */
                path *path = path_find(game_map(nlarn, Z(nlarn->p->pos)),
                                       nlarn->p->pos, pos, LE_GROUND);

                if (path && !g_queue_is_empty(path->path))
                {
                    /* Path found. Move the player. */
                    path_element *el = g_queue_pop_head(path->path);
                    moves_count = player_move(nlarn->p, pos_dir(nlarn->p->pos, el->pos), true);

                    if (moves_count == 0)
                    {
                        /* for some reason movement is impossible, therefore
                           stop auto travel. */
                        pos = pos_invalid;
                    }
                }
                else
                {
                    /* No path found. Stop traveling */
                    pos = pos_invalid;
                }

                /* clean up */
                if (path) path_destroy(path);
            }
        }
        else if (run_cmd != 0)
        {
            /* run mode */
            ch = run_cmd;
            // Check if we're in open surroundings.
            adj_corr = adjacent_corridor(nlarn->p->pos, ch);
        }
        else
        {
            /* not running or travelling, get a key and handle it */
            ch = display_getch(NULL);

            if (ch == '/' || ch == 'g')
            {
                /* fast movement: get direction of movement */
                ch = display_getch(NULL);
                switch (ch)
                {
                case 'b':
                case KEY_END:
                case KEY_C1:
                case '1':
                    ch = 'B';
                    break;

                case 'j':
                case KEY_DOWN:
#ifdef KEY_C2
                case KEY_C2:
#endif
                case '2':
                    ch = 'J';
                    break;

                case 'n':
                case KEY_NPAGE:
                case KEY_C3:
                case '3':
                    ch = 'N';
                    break;

                case 'h':
                case KEY_LEFT:
#ifdef KEY_B1
                case KEY_B1:
#endif
                case '4':
                    ch = 'H';
                    break;

                case '5':
                case KEY_B2:
                    ch = 'w';
                    break;

                case 'l':
                case KEY_RIGHT:
#ifdef KEY_B3
                case KEY_B3:
#endif
                case '6':
                    ch = 'L';
                    break;

                case 'y':
                case KEY_HOME:
                case KEY_A1:
                case '7':
                    ch = 'Y';
                    break;

                case 'k':
                case KEY_UP:
#ifdef KEY_A2
                case KEY_A2:
#endif
                case '8':
                    ch = 'K';
                    break;

                case 'u':
                case KEY_PPAGE:
                case KEY_A3:
                case '9':
                    ch = 'U';
                    break;
                }
            }

            switch (ch)
            {
            case 'H':
            case 'J':
            case 'K':
            case 'L':
            case 'Y':
            case 'U':
            case 'B':
            case 'N':
                ch = tolower(ch);
                run_cmd = ch;
                adj_corr = adjacent_corridor(nlarn->p->pos, ch);
                break;
            case 'w': /* rest up to 1 mobul */
                ch = '.';
                run_cmd = ch;
                end_resting = game_turn(nlarn) + 100;
                break;
            }
        }

        /* get key and analyse it */
        switch (ch)
        {
            /* *** MOVEMENT *** */
        case 'h':
        case '4':
        case KEY_LEFT:
#ifdef KEY_B1
        case KEY_B1:
#endif
            moves_count = player_move(nlarn->p, GD_WEST, run_cmd == 0);
            break;

        case 'y':
        case '7':
        case KEY_HOME:
        case KEY_A1:
            moves_count = player_move(nlarn->p, GD_NW, run_cmd == 0);
            break;

        case 'l':
        case '6':
        case KEY_RIGHT:
#ifdef KEY_B3
        case KEY_B3:
#endif
            moves_count = player_move(nlarn->p, GD_EAST, run_cmd == 0);
            break;

        case 'n':
        case '3':
        case KEY_NPAGE:
        case KEY_C3:
            moves_count = player_move(nlarn->p, GD_SE, run_cmd == 0);
            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif
            moves_count = player_move(nlarn->p, GD_NORTH, run_cmd == 0);
            break;

        case 'u':
        case '9':
        case KEY_PPAGE:
        case KEY_A3:
            moves_count = player_move(nlarn->p, GD_NE, run_cmd == 0);
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            moves_count = player_move(nlarn->p, GD_SOUTH, run_cmd == 0);
            break;

        case 'b':
        case '1':
        case KEY_END:
        case KEY_C1:
            moves_count = player_move(nlarn->p, GD_SW, run_cmd == 0);
            break;

            /* look at current position */
        case ':':
            strbuf = map_pos_examine(nlarn->p->pos);
            log_add_entry(nlarn->log, strbuf);
            g_free(strbuf);
            break;

            /* look at different position */
        case ';':
            if (!player_effect(nlarn->p, ET_BLINDNESS))
                (void)display_get_new_position(nlarn->p, nlarn->p->pos,
                        "Choose a position to examine", false, false,
                        false, 0, false, true);
            else
                log_add_entry(nlarn->log, "You can't look around "
                        "while blinded!");
            break;

            /* pick up */
        case ',':
            player_pickup(nlarn->p);
            break;

            /* sit and wait */
        case '5':
        case '.':
        case KEY_B2:
            moves_count = 1;
            break;

            /* help */
        case KEY_F(1):
        case '?':
            if (g_file_get_contents(nlarn_helpfile, &strbuf, NULL, NULL))
            {
                display_show_message("Help for the game of NLarn", strbuf, 1);
                g_free(strbuf);
            }
            else
            {
                display_show_message("Error",
                                     "\n The help file could not be found. \n", 0);
            }
            break;

            /* go down stairs / enter a building */
        case '>':
            if (!(moves_count = player_stairs_down(nlarn->p)))
                moves_count = player_building_enter(nlarn->p);
            break;

            /* go up stairs */
        case '<':
            moves_count = player_stairs_up(nlarn->p);
            break;

            /* bank account information */
        case '$':
            log_add_entry(nlarn->log, "There %s %s gold on your bank account.",
                          is_are(nlarn->p->bank_account),
                          int2str(nlarn->p->bank_account));
            break;

        case '\\':
            if ((strbuf = player_item_identified_list(nlarn->p)))
            {
                display_show_message("Identified items", strbuf, 0);
                g_free(strbuf);
            }
            else
            {
                log_add_entry(nlarn->log, "You have not discovered any item yet.");
            }
            break;

            /* recast previous spell */
        case 'a':
            moves_count = spell_cast_previous(nlarn->p);
            break;

            /* desecrate altar */
        case 'A':
            moves_count = player_altar_desecrate(nlarn->p);
            break;

            /* cast a spell */
        case 'c':
            moves_count = spell_cast_new(nlarn->p);
            break;

            /* close door */
        case 'C':
            moves_count = player_door_close(nlarn->p);
            break;

            /* disarm a trapped container or a trap */
        case 'D':
            if (!container_untrap(nlarn->p))
                moves_count = trap_disarm(nlarn->p);
            break;

            /* drop something */
        case 'd':
            player_drop(nlarn->p);
            break;

            /* wash at fountain */
        case 'F':
            moves_count = player_fountain_wash(nlarn->p);
            break;

            /* fire a ranged weapon */
        case 'f':
            moves_count = weapon_fire(nlarn->p);
            break;

            /* display inventory */
        case 'i':
            player_inv_display(nlarn->p);
            break;

            /* open door / container */
        case 'o':
            if (inv_length_filtered(*map_ilist_at(game_map(nlarn, Z(nlarn->p->pos)),
                                                  nlarn->p->pos),
                                    &item_filter_container) > 0)
            {
                container_open(nlarn->p, NULL, NULL);
            }
            else
            {
                moves_count = player_door_open(nlarn->p, GD_NONE);
            }
            break;

            /* pray at altar */
        case 'p':
            moves_count = player_altar_pray(nlarn->p);
            break;

        case 'P':
            if (nlarn->p->outstanding_taxes)
            {
                log_add_entry(nlarn->log, "You presently owe %d gold in taxes.",
                              nlarn->p->outstanding_taxes);
            }
            else
                log_add_entry(nlarn->log, "You do not owe any taxes.");
            break;

            /* drink a potion or from a fountain */
        case 'q':
        {
            sobject_t ms = map_sobject_at(game_map(nlarn, Z(nlarn->p->pos)),
                                              nlarn->p->pos);

            if ((ms == LS_FOUNTAIN || ms == LS_DEADFOUNTAIN)
                    && display_get_yesno("There is a fountain here, drink from it?",
                                         NULL, NULL, NULL))
            {
                moves_count = player_fountain_drink(nlarn->p);
            }
            else
            {
                player_quaff(nlarn->p);
            }
        }
        break;

            /* remove gems from throne */
        case 'R':
            moves_count = player_throne_pillage(nlarn->p);
            break;

            /* read something */
        case 'r':
            player_read(nlarn->p);
            break;

            /* sit on throne */
        case 'S':
            moves_count = player_throne_sit(nlarn->p);
            break;

            /* search */
        case 's':
            player_search(nlarn->p);
            break;

            /* take off something */
        case 'T':
            player_take_off(nlarn->p);
            break;

            /* throw a potion */
        case 't':
            moves_count = potion_throw(nlarn->p);
            break;

            /* voyage (travel) */
        case 'v':
            pos = display_get_new_position(nlarn->p, cpos,
                                           "Choose a destination to travel to.",
                                           false, false, true, 0, true, false);

            if (pos_valid(pos))
            {
                /* empty key input buffer to avoid recurring position queries */
                ch = 0;
                /* store position for resuming travel */
                cpos = pos;
            }
            else
            {
                log_add_entry(nlarn->log, "Aborted.");
            }
            break;

            /* continue auto travel */
        case 'V':
            /* delete last auto travel target if it was on another map */
            if (Z(cpos) != Z(nlarn->p->pos))
            {
                cpos = pos_invalid;
            }

            if (pos_valid(cpos))
            {
                /* restore last known auto travel position */
                pos = cpos;
                /* reset keyboard input */
                ch = 0;
            }
            else
                log_add_entry(nlarn->log, "No travel destination known.");
            break;

            /* wear/wield something */
        case 'W':
            player_equip(nlarn->p);
            break;

            /* swap weapons */
        case 'x':
            weapon_swap(nlarn->p);
            break;

            /* configure auto-pickup */
        case 1: /* ^A */
            {
                display_config_autopickup(nlarn->p->settings.auto_pickup);
                char *settings = verbose_autopickup_settings(nlarn->p->settings.auto_pickup);

                if (!settings)
                {
                    log_add_entry(nlarn->log, "Auto-pickup is not enabled.");
                }
                else
                {
                    log_add_entry(nlarn->log, "Auto-pickup is enabled for %s.", settings);
                    g_free(settings);
                }
            }
            break;

            /* show stationary object memory */
        case 4: /* ^D */
            player_list_sobjmem(nlarn->p);
            break;

            /* "paper doll" */
        case KEY_TAB:
            player_paperdoll(nlarn->p);
            break;

            /* redraw screen */
        case 12: /* ^L */
#ifdef SDLPDCURSES
        case KEY_RESIZE: /* SDL window size event */
#endif
            clear();
            display_draw();
            break;

#ifdef SDLPDCURSES
        case 13: /* ENTER */
            if (PDC_get_key_modifiers() & PDC_KEY_MODIFIER_ALT)
                display_toggle_fullscreen(true);

            break;
#endif

            /* configure defaults */
        case 16: /* ^P */
            configure_defaults(nlarn_inifile);
            break;

            /* quit */
        case 17: /* ^Q */
            if (display_get_yesno("Are you sure you want to quit?", NULL, NULL, NULL))
                player_die(nlarn->p, PD_QUIT, 0);
            break;

            /* message log browser */
        case 18: /* ^R */
            display_show_history(nlarn->log, "Message history");
            break;

            /* save */
        case 19: /* ^S */
            if (game_save(nlarn))
            {
                /* only terminate the game if saving was successful */
                nlarn = game_destroy(nlarn);

                /* return control to the main function and
                   indicate our desire to quit the game */
                longjmp(nlarn_death_jump, PD_QUIT);
            }
            break;

        case 22: /* ^V */
            log_add_entry(nlarn->log, "NLarn version %s, built on %s.", nlarn_version, __DATE__);
            break;

            /* enable wizard mode */
        case 23: /* ^W */
            if (!game_wizardmode(nlarn))
            {
                if (display_get_yesno("Are you sure you want to switch to Wizard mode?\n" \
                                      "You will not be able to switch back to normal " \
                                      "gameplay and your score will not be counted.", NULL, NULL, NULL))
                {
                    game_wizardmode(nlarn) = true;
                    log_add_entry(nlarn->log, "Wizard mode has been activated.");
                }
            }
            else
            {
                log_add_entry(nlarn->log, "Wizard mode is already enabled.");
            }
            break;

            /* *** DEBUGGING SUPPORT *** */

            /* toggle visibility of entire map in wizard mode */
        case 6: /* ^F */
            if (game_wizardmode(nlarn))
                game_fullvis(nlarn) = (!game_fullvis(nlarn));
            break;

        case '*':
            if (game_wizardmode(nlarn)) nlarn->p->bank_account += 1000;
            break;

        case '+': /* map up */
            if (game_wizardmode(nlarn) && (Z(nlarn->p->pos) > 0))
            {
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, Z(nlarn->p->pos) - 1),
                                               Z(nlarn->p->pos) == MAP_CMAX);
            }
            break;

        case '-': /* map down */
            if (game_wizardmode(nlarn) && (Z(nlarn->p->pos) < (MAP_MAX - 1)))
            {
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, Z(nlarn->p->pos) + 1),
                                               Z(nlarn->p->pos) == MAP_CMAX - 1);
            }
            break;

        case 20: /* (^T) intra-level teleport */
            if (game_wizardmode(nlarn))
            {
                pos = display_get_new_position(nlarn->p, nlarn->p->pos,
                                               "Choose a position to teleport to.",
                                               false, false, true, 0, true, false);

                if (pos_valid(pos))
                {
                    effect *e;
                    if ((e = player_effect_get(nlarn->p, ET_TRAPPED)))
                        player_effect_del(nlarn->p, e);

                    nlarn->p->pos = pos;

                    /* reset pos, otherwise auto travel would be enabled */
                    pos = pos_invalid;
                }
            }
            break;

            /* gain experience level */
        case 24:  /* ^X */
            if (game_wizardmode(nlarn))
                player_level_gain(nlarn->p, 1);

            break;

        case '&': /* instaheal */
            if (game_wizardmode(nlarn))
            {
                nlarn->p->hp = nlarn->p->hp_max;
                nlarn->p->mp = nlarn->p->mp_max;

                /* clear some nasty effects */
                effect *e;
                if ((e = player_effect_get(nlarn->p, ET_PARALYSIS)))
                    player_effect_del(nlarn->p, e);
                if ((e = player_effect_get(nlarn->p, ET_CONFUSION)))
                    player_effect_del(nlarn->p, e);
                if ((e = player_effect_get(nlarn->p, ET_BLINDNESS)))
                    player_effect_del(nlarn->p, e);
                if ((e = player_effect_get(nlarn->p, ET_POISON)))
                    player_effect_del(nlarn->p, e);
                if ((e = player_effect_get(nlarn->p, ET_TRAPPED)))
                    player_effect_del(nlarn->p, e);
            }
            break;

        case 3: /* ^C */
            if (game_wizardmode(nlarn))
                calc_fighting_stats(nlarn->p);
            break;
        }

        gboolean no_move = (moves_count == 0);
        gboolean was_attacked = false;

        /* manipulate game time */
        if (moves_count)
        {
            player_make_move(nlarn->p, moves_count, false, NULL);
            was_attacked = nlarn->p->attacked;
            nlarn->p->attacked = false;
            moves_count = 0;
        }

        /* recalculate FOV */
        player_update_fov(nlarn->p);

        if (run_cmd != 0)
        {
            // Interrupt running AND resting if:
            // * last action cost no turns (we ran into a wall)
            // * we took damage (trap, poison, or invisible monster)
            // * a monster has moved adjacent to us
            if (no_move || was_attacked
                    || player_adjacent_monster(nlarn->p, run_cmd == '.'))
            {
                run_cmd = 0;
            }
            // Interrupt resting if we've rested for 100 turns OR
            // * hp is full,
            // * mp is full,
            // * we are not confused,
            // * we are not blinded, AND
            // * we are not paralysed
            else if (run_cmd == '.')
            {
                if (game_turn(nlarn) >= end_resting
                        || (nlarn->p->hp == (gint)nlarn->p->hp_max
                            && nlarn->p->mp == (gint)nlarn->p->mp_max
                            && !player_effect_get(nlarn->p, ET_CONFUSION)
                            && !player_effect_get(nlarn->p, ET_PARALYSIS)
                            && !player_effect_get(nlarn->p, ET_DIZZINESS)
                            && !player_effect_get(nlarn->p, ET_BLINDNESS)))
                {
                    run_cmd = 0;
                }
            }
            // Interrupt running if:
            // * became confused (umber hulk)
            // * there's a fork in the path ahead
            else if (player_effect_get(nlarn->p, ET_CONFUSION)
                     || (!adj_corr && adjacent_corridor(nlarn->p->pos, run_cmd)))
            {
                run_cmd = 0;
            }
        }
    }
}

gboolean main_menu()
{
    const char *main_menu_tpl =
        "\n"
        "      `KEY`a`end`) %s Game\n"
        "      `KEY`b`end`) Configure Settings\n"
        "      `KEY`c`end`) Visit the Hall of Fame\n"
        "\n"
        "      `KEY`q`end`) Quit Game\n"
        "\n"
        "    You have reached difficulty level %d\n";


    g_autofree char *title = g_strdup_printf("NLarn %s", nlarn_version);
    g_autofree char *main_menu = g_strdup_printf(main_menu_tpl,
        (game_turn(nlarn) == 1) ? "New" : "Continue saved", game_difficulty(nlarn));

    char input = 0;

    while (input != 'q' && input != KEY_ESC)
    {
        input = display_show_message(title, main_menu, 0);

        switch (input)
        {
        case 'a':
            return true;
            break;

        case 'b':
            configure_defaults(nlarn_inifile);
            break;

        case 'c':
        {
            GList *hs = scores_load();
            char *rendered_highscores = scores_to_string(hs, NULL);

            display_show_message("NLarn Hall of Fame",
                    highscores ? rendered_highscores : room_for_improvement, 0);

            if (hs)
            {
                scores_destroy(hs);
                g_free(rendered_highscores);
            }
        }
            break;
        }
    }

    return false;
}

int main(int argc, char *argv[])
{
    /* initialisation */
    nlarn_init(argc, argv);

    /* check if the message file exists */
    gchar *message_file = NULL;
    if (!g_file_get_contents(nlarn_mesgfile, &message_file, NULL, NULL))
    {
        nlarn = game_destroy(nlarn);
        display_shutdown();
        g_printerr("Error: Cannot find the message file.\n");

        return EXIT_FAILURE;
    }

    /* show message file */
    display_show_message("Welcome to the game of NLarn!", message_file, 0);
    g_free(message_file);

    /* Create the jump target for player death. Death will destroy the game
       object, thus control will be returned to the line after this one, i.e
       the game will be created again and the main menu will be shown. To
       ensure that the game quits when quitting from inside the game, return
       the cause of death from player_die().
    */
    player_cod cod = setjmp(nlarn_death_jump);

    /* clear the screen to wipe remains from the previous game */
    clear();

    /* can be broken by quitting in the game, or with q or ESC in main menu */
    while (cod != PD_QUIT)
    {
        /* initialise the game */
        game_init(&config);

        /* present main menu - */
        if (false == main_menu()) {
            break;
        }

        /* ask for a character name if none has been supplied */
        while (nlarn->p->name == NULL)
        {
            nlarn->p->name = display_get_string("Choose your name",
                    "By what name shall you be called?", NULL, 45);
        }

        /* ask for character's gender if it is not known yet */
        if (nlarn->p->sex == PS_NONE)
        {
            int res = display_get_yesno("Are you male or female?", NULL, "Female", "Male");

            /* display_get_yesno() returns 0 or one */
            nlarn->p->sex = (res == true) ? PS_FEMALE : PS_MALE;
        }

        while (!nlarn->player_stats_set)
        {
            /* assign the player's stats */
            char selection = player_select_bonus_stats();
            nlarn->player_stats_set = player_assign_bonus_stats(nlarn->p, selection);
        }

        /* automatic save point (not when restoring a save) */
        if ((game_turn(nlarn) == 1) && !config.no_autosave)
        {
            game_save(nlarn);
        }

        /* main event loop */
        mainloop();
    }

    /* persist configuration before freeing it */
    write_ini_file(nlarn_inifile, &config);
    free_config(config);

    return EXIT_SUCCESS;
}

static gboolean adjacent_corridor(position pos, char mv)
{
    position p1 = pos, p2 = pos;
    switch (mv)
    {
    case 'h': // left
        X(p1) -= 1;
        Y(p1) -= 1;
        X(p2) -= 1;
        Y(p2) += 1;
        break;
    case 'j': // down
        X(p1) -= 1;
        Y(p1) += 1;
        X(p2) += 1;
        Y(p2) += 1;
        break;
    case 'k': // up
        X(p1) -= 1;
        Y(p1) -= 1;
        X(p2) += 1;
        Y(p2) -= 1;
        break;
    case 'l': // right
        X(p1) += 1;
        Y(p1) -= 1;
        X(p2) += 1;
        Y(p2) += 1;
        break;
    case 'y': // up left
        Y(p1) -= 1;
        X(p2) -= 1;
        break;
    case 'u': // up right
        Y(p1) -= 1;
        X(p2) += 1;
        break;
    case 'b': // down left
        X(p1) -= 1;
        Y(p2) += 1;
        break;
    case 'n': // down right
        X(p1) += 1;
        Y(p2) += 1;
        break;
    }

    if (X(p1) < MAP_MAX_X && Y(p1) < MAP_MAX_Y
            && mt_is_passable(map_tiletype_at(game_map(nlarn, Z(nlarn->p->pos)), p1)))
    {
        return true;
    }
    if (X(p2) < MAP_MAX_X && Y(p2) < MAP_MAX_Y
            && mt_is_passable(map_tiletype_at(game_map(nlarn, Z(nlarn->p->pos)), p2)))
    {
        return true;
    }

    return false;
}

#ifdef __unix
static void nlarn_signal_handler(int signo)
{
    /* restore the display down before emitting messages */
    display_shutdown();

    /* attempt to save and clear the game, when initialized */
    if (nlarn)
    {
        game_save(nlarn);

        if (signo == SIGTERM)
        {
            g_printf("Terminated. Your progress has been saved.\n");
        }

        nlarn = game_destroy(nlarn);
    }

    exit(EXIT_SUCCESS);
}
#endif
