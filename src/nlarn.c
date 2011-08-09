/*
 * nlarn.c
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
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

/* $Id$ */

/* needed for the key definitions */
#include <ctype.h>
#include <curses.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#ifdef __unix
#  include <signal.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include "container.h"
#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "sobjects.h"

/* global game object */
game *nlarn = NULL;


static gboolean adjacent_monster(player *p, gboolean ignore_harmless);
static gboolean adjacent_corridor(position pos, char mv);

#ifdef WIN32
BOOL nlarn_control_handler(DWORD fdwCtrlType);
#endif

#ifdef __unix
static void nlarn_signal_handler(int signo);
#endif

int main(int argc, char *argv[])
{
    /* count of moves used by last action */
    int moves_count = 0;

    /* used to read in e.g. the help file */
    gchar *strbuf;

    /* position to examine / to travel to */
    position pos = pos_invalid;

    /* position chosen for autotravel, allowing to continue travel */
    position cpos = pos_invalid;

    /* call display_shutdown when terminating the game */
    atexit(display_shutdown);

    /* initialise the game */
    game_init(argc, argv);

    /* set the console shutdown handler */
#ifdef __unix
    signal(SIGTERM, nlarn_signal_handler);
#endif

#ifdef WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE) nlarn_control_handler, TRUE);
#endif

    /* check if mesgfile exists */
    if (!g_file_get_contents(game_mesgfile(nlarn), &strbuf, NULL, NULL))
    {
        game_destroy(nlarn);
        display_shutdown();
        g_printerr("Error: Cannot find the message file.\n");

        exit(EXIT_FAILURE);
    }

    display_show_message("Welcome to the game of NLarn!", strbuf, 0);
    g_free(strbuf);

    /* ask for a charakter name if none has been supplied */
    while (nlarn->p->name == NULL)
    {
        nlarn->p->name = display_get_string("By what name shall you be called?",
                                            NULL, 45);
    }

    /* ask for charakter's gender if it is not known yet */
    if (nlarn->p->sex == PS_NONE)
    {
        int res = display_get_yesno("Are you male or female?", "Female", "Male");

        /* display_get_yesno() returns 0 or one */
        nlarn->p->sex = (res == TRUE) ?  PS_FEMALE : PS_MALE;
    }

    while (!nlarn->player_stats_set)
    {
        /* assign the player's stats */
        nlarn->player_stats_set = player_assign_bonus_stats(nlarn->p, NULL);
    }

    /* automatic save point (not when restoring a save) */
    if ((game_turn(nlarn) == 1) && game_autosave(nlarn))
    {
        game_save(nlarn, NULL);
    }

    char run_cmd = 0;
    int ch = 0;
    gboolean adj_corr = FALSE;
    guint end_resting = 0;

    /* main event loop */
    do
    {
        /* repaint screen */
        display_paint_screen(nlarn->p);

        if (pos_valid(pos))
        {
            /* travel mode */

            /* check if travel mode shall be aborted:
               attacked or fell through trap door */
            if (nlarn->p->attacked || adjacent_monster(nlarn->p, FALSE)
                || Z(pos) != Z(nlarn->p->pos))
            {
                pos = pos_invalid;
            }
            else if (pos_adjacent(nlarn->p->pos, pos))
            {
                /* the target has almost been reached. This is the last move. */
                moves_count = player_move(nlarn->p, pos_dir(nlarn->p->pos, pos), TRUE);
                /* reset the target position */
                pos = cpos = pos_invalid;
            }
            else
            {
                /* find a path to the destination */
                map_path *path = map_find_path(game_map(nlarn, Z(nlarn->p->pos)),
                                               nlarn->p->pos, pos, LE_GROUND);

                if (path && !g_queue_is_empty(path->path))
                {
                    /* path found. move the player. */
                    map_path_element *el = g_queue_pop_head(path->path);
                    moves_count = player_move(nlarn->p, pos_dir(nlarn->p->pos, el->pos), TRUE);

                    if (moves_count == 0)
                    {
                        /* for some reason movement is impossible, therefore
                           stop autotravel. */
                        pos = pos_invalid;
                    }
                }
                else
                {
                    /* no path found. stop travelling */
                    pos = pos_invalid;
                }

                /* cleanup */
                if (path) map_path_destroy(path);
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
            /* not running or traveling, get a key and handle it */
            ch = display_getch();

            if (ch == '/' || ch == 'g')
            {
                /* fast movement: get direction of movement */
                ch = display_getch();
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

        /* get key and analyze it */
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
            (void)display_get_new_position(nlarn->p, nlarn->p->pos,
                                           "Choose a position to examine",
                                           FALSE, FALSE, FALSE, 0, FALSE, TRUE);
            break;

            /* pick up */
        case ',':
            player_pickup(nlarn->p);
            break;

        case '@':
            display_config_autopickup(nlarn->p);
            player_autopickup_show(nlarn->p);
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
            if (g_file_get_contents(game_helpfile(nlarn), &strbuf, NULL, NULL))
            {
                display_show_message("Help for The Caverns of NLarn", strbuf, 1);
                g_free(strbuf);
            }
            else
            {
                display_show_message("Help for The Caverns of NLarn",
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
            log_add_entry(nlarn->log, "There %s %s gp on your bank account.",
                          (nlarn->p->bank_account == 1) ? "is" : "are",
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

            /* desecrate altar */
        case 'A':
            moves_count = player_altar_desecrate(nlarn->p);
            break;

            /* continue autotravel */
        case 'C':
            /* delete last autotravel target if it was on another map */
            if (Z(cpos) != Z(nlarn->p->pos))
            {
                cpos = pos_invalid;
            }

            if (pos_valid(cpos))
            {
                /* restore last known autotravel position */
                pos = cpos;
                /* reset keyboard input */
                ch = 0;
            }
            else
                log_add_entry(nlarn->log, "No travel destination known.");
            break;

            /* close door */
        case 'c':
            moves_count = player_door_close(nlarn->p);
            break;

            /* show stationary object memory */
        case 'D':
            player_list_sobjmem(nlarn->p);
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

            /* work magic */
        case 'm':
            moves_count = spell_cast_new(nlarn->p);
            break;

            /* recast previous spell */
        case 'M':
            moves_count = spell_cast_previous(nlarn->p);
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
                log_add_entry(nlarn->log, "You presently owe %d gp in taxes.",
                              nlarn->p->outstanding_taxes);
            }
            else
                log_add_entry(nlarn->log, "You do not owe any taxes.");
            break;

            /* drink a potion or from a fountain */
        case 'q':
        {
            map_sobject_t ms = map_sobject_at(game_map(nlarn, Z(nlarn->p->pos)),
                                              nlarn->p->pos);

            if ((ms == LS_FOUNTAIN || ms == LS_DEADFOUNTAIN)
                    && display_get_yesno("There is a fountain here, drink from it?",
                                         NULL, NULL))
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
        case 'V':
            pos = display_get_new_position(nlarn->p, cpos,
                                           "Choose a destination to travel to.",
                                           FALSE, FALSE, TRUE, 0, TRUE, FALSE);

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

        case 'v':
            log_add_entry(nlarn->log, "NLarn version %d.%d.%d%s, built on %s.",
                          VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, SVNID,
                          __DATE__);
            break;

            /* wear/wield something */
        case 'W':
            player_equip(nlarn->p);
            break;

            /* swap weapons */
        case 'x':
            weapon_swap(nlarn->p);
            break;

            /* "paperdoll" */
        case KEY_TAB:
            player_paperdoll(nlarn->p);
            break;

            /* redraw screen */
        case 12: /* ^L */
            clear();
            display_draw();
            break;

            /* quit */
        case 17: /* ^Q */
            if (display_get_yesno("Are you sure you want to quit?", NULL, NULL))
                player_die(nlarn->p, PD_QUIT, 0);
            break;

            /* message log browser */
        case 18: /* ^R */
            display_show_history(nlarn->log, "Message history");
            break;

            /* save */
        case 19: /* ^S */
            if (game_save(nlarn, NULL))
            {
                /* only terminate the game if saving was successful */
                game_destroy(nlarn);
                exit(EXIT_SUCCESS);
            }
            break;

            /* enable wizard mode */
        case 23: /* ^W */
            if (!game_wizardmode(nlarn))
            {
                if (display_get_yesno("Are you sure you want to switch to Wizard mode?\n" \
                                      "You will not be able to switch back to normal " \
                                      "gameplay and your score will not be counted.", NULL, NULL))
                {
                    game_wizardmode(nlarn) = TRUE;
                    log_add_entry(nlarn->log, "Wizard mode has been activated.");
                }
            }
            else
            {
                log_add_entry(nlarn->log, "Wizard mode is already enabled.");
            }
            break;

            /* *** DEBUGGING SUPPORT *** */

            /* interact with the Lua interpreter */
        case 5: /* ^E */
            if (!game_wizardmode(nlarn)) break;

            strbuf = display_get_string("Interact with the Lua interpreter", NULL, 60);
            if (!strbuf) break;

            if (luaL_dostring(nlarn->L, strbuf))
            {
                log_add_entry(nlarn->log, "E: %s", lua_tostring(nlarn->L, -1));
                lua_pop(nlarn->L, 1);
            }

            g_free(strbuf);
            break;

            /* toggle visibility of entire map in wizard mode */
        case 22: /* ^V */
            if (game_wizardmode(nlarn))
                game_fullvis(nlarn) = (!game_fullvis(nlarn));
            break;

        case '*':
            if (game_wizardmode(nlarn)) nlarn->p->bank_account += 1000;
            break;

        case '+': /* dungeon map up */
            if (game_wizardmode(nlarn) && (Z(nlarn->p->pos) > 0))
            {
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, Z(nlarn->p->pos) - 1),
                                               Z(nlarn->p->pos) == MAP_DMAX);
            }
            break;

        case '-': /* dungeon map down */
            if (game_wizardmode(nlarn) && (Z(nlarn->p->pos) < (MAP_MAX - 1)))
            {
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, Z(nlarn->p->pos) + 1),
                                               Z(nlarn->p->pos) == MAP_DMAX - 1);
            }
            break;

        case 20: /* (^T) intra-level teleport */
            if (game_wizardmode(nlarn))
            {
                pos = display_get_new_position(nlarn->p, nlarn->p->pos,
                                               "Choose a position to teleport to.",
                                               FALSE, FALSE, FALSE, 0, TRUE, FALSE);

                if (pos_valid(pos))
                {
                    effect *e;
                    if ((e = player_effect_get(nlarn->p, ET_TRAPPED)))
                        player_effect_del(nlarn->p, e);

                    nlarn->p->pos = pos;

                    /* reset pos, otherwise autotravel would be enabled */
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

        case 6: /* ^F */
            if (game_wizardmode(nlarn))
                calc_fighting_stats(nlarn->p);
            break;
        }

        gboolean no_move = (moves_count == 0);
        gboolean was_attacked = FALSE;

        /* manipulate game time */
        if (moves_count)
        {
            player_make_move(nlarn->p, moves_count, FALSE, NULL);
            was_attacked = nlarn->p->attacked;
            nlarn->p->attacked = FALSE;
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
            if (no_move || was_attacked || adjacent_monster(nlarn->p, run_cmd == '.'))
            {
                run_cmd = 0;
            }
            // Interrupt resting if we've rested for 100 turns OR
            // * hp is full,
            // * mp is full,
            // * we are not confused,
            // * we are not blinded, AND
            // * we are not paralyzed
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
    while (TRUE); /* main event loop */
}

static gboolean adjacent_monster(player *p, gboolean ignore_harmless)
{
    gboolean monster_visible = FALSE;

    /* get the list of all visible monsters */
    GList *mlist, *miter;
    miter = mlist = fov_get_visible_monsters(p->fov);

    /* no visible monsters? */
    if (mlist == NULL)
        return monster_visible;

    /* got a list of monsters, check if any are dangerous */
    do
    {
        monster *m = (monster *)miter->data;

        // Ignore the town inhabitants.
        if (monster_type(m) == MT_TOWN_PERSON)
            continue;

        /* ignore servants */
        if (monster_action(m) == MA_SERVE)
            continue;

        /* Only ignore floating eye if already paralysed. */
        if (ignore_harmless
            && (monster_type(m) == MT_FLOATING_EYE)
            && player_effect_get(p, ET_PARALYSIS))
            continue;

        /* Ignore adjacent umber hulk if already confused. */
        if (ignore_harmless
            && (monster_type(m) == MT_UMBER_HULK)
            && player_effect_get(p, ET_CONFUSION))
            continue;

        /* when reaching this point, the monster is a threat */
        monster_visible = TRUE;
        break;
    }
    while ((miter = miter->next) != NULL);

    /* free the list returned by fov_get_visible_monsters() */
    g_list_free(mlist);

    return monster_visible;
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

    if (X(p1) >= 0 && X(p1) < MAP_MAX_X && Y(p1) >= 0 && Y(p1) < MAP_MAX_Y
            && mt_is_passable(map_tiletype_at(game_map(nlarn, Z(nlarn->p->pos)), p1)))
    {
        return TRUE;
    }
    if (X(p2) >= 0 && X(p2) < MAP_MAX_X && Y(p2) >= 0 && Y(p2) < MAP_MAX_Y
            && mt_is_passable(map_tiletype_at(game_map(nlarn, Z(nlarn->p->pos)), p2)))
    {
        return TRUE;
    }

    return FALSE;
}

#ifdef __unix
static void nlarn_signal_handler(int signo __attribute__((unused)))
{
    /* restore the display down before emitting messages */
    display_shutdown();

    /* attempt to save the game */
    game_save(nlarn, NULL);
    g_printf("Terminated. Your progress has been saved.\n");

    game_destroy(nlarn);
    exit(EXIT_SUCCESS);
}
#endif

#ifdef WIN32
BOOL nlarn_control_handler(DWORD fdwCtrlType)
{
    switch(fdwCtrlType)
    {
        /* Close Window button pressed: store the game progress */
    case CTRL_CLOSE_EVENT:
        /* save the game */
        game_save(nlarn, NULL);
        game_destroy(nlarn);

        return TRUE;

    default:
        return FALSE;
    }
}
#endif
