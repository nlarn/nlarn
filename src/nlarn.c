/*
 * nlarn.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

/* needed for the key definitions */
#include <curses.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <ctype.h>

#include "container.h"
#include "display.h"
#include "game.h"
#include "nlarn.h"

game *nlarn = NULL;

static gboolean adjacent_monster(position p, gboolean ignore_eye);
static gboolean adjacent_corridor(position pos, char move);

int main(int argc, char *argv[])
{
    /* count of moves used by last action */
    int moves_count = 0;

    /* used to read in e.g. the help file */
    gchar *strbuf;

    /* position to examine */
    position pos;

    /* save file name */
    char *save_file_name = NULL;

    /* assemble save file name */
    gchar *userdir = game_userdir();
    save_file_name = g_build_path(G_DIR_SEPARATOR_S, userdir, "nlarn.sav", NULL);
    g_free(userdir);

    /* find save file */
    gboolean loaded = FALSE;
    gboolean display_initialised = FALSE;
    if (g_file_test(save_file_name, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    {
        /* restore savegame */
        loaded = game_load(save_file_name, argc, argv);

        if (!loaded)
        {
            /* initialize display */
            display_init();

            /* call display_shutdown when terminating the game */
            atexit(display_shutdown);

            display_draw();
            display_initialised = TRUE;

            if (display_get_yesno("Saved game could not be loaded. " \
                                  "Delete and start new game?",
                                  NULL, NULL))
            {
                /* delete save file */
                g_unlink(save_file_name);
            }
            else
            {
                g_printerr("Save file not compatible to current version.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (loaded)
    {
        /* delete save file */
        g_unlink(save_file_name);

        /* refresh FOV */
        player_update_fov(nlarn->p);
    }
    else
    {
        /* start new game */
        game_new(argc, argv);

        /* put the player into the town */
        player_map_enter(nlarn->p, game_map(nlarn, 0), FALSE);

        /* give player knowledge of the town */
        scroll_mapping(nlarn->p, NULL);
    }

    if (!display_initialised)
    {
        /* initialize display */
        display_init();

        /* call display_shutdown when terminating the game */
        atexit(display_shutdown);

        display_draw();
    }

    /* check if mesgfile exists */
    if (!g_file_get_contents(game_mesgfile(nlarn), &strbuf, NULL, NULL))
    {
        game_destroy(nlarn);

        fprintf(stderr, "Error: Cannot find message file.\n");
        exit(EXIT_FAILURE);
    }

    display_show_message("Welcome to the game of NLarn!", strbuf, 0);
    g_free(strbuf);

    display_paint_screen(nlarn->p);

    while (nlarn->p->name == NULL)
    {
        nlarn->p->name = display_get_string("By what name shall you be called?",
                                            NULL, 45);

        display_paint_screen(nlarn->p);
    }

    if (nlarn->p->sex == PS_NONE)
    {
        int res = display_get_yesno("Are you male or female?", "Female", "Male");

        /* display_get_yesno() returns 0 or one */
        nlarn->p->sex = (res == TRUE) ?  PS_FEMALE : PS_MALE;

        display_paint_screen(nlarn->p);
    }

    char run_cmd = 0;
    int ch;
    gboolean adj_corr = FALSE;
    int old_hp;
    int end_resting = 0;

    /* main event loop */
    do
    {
        if (run_cmd != 0)
        {
            ch = run_cmd;
            // Check if we're in open surroundings.
            adj_corr = adjacent_corridor(nlarn->p->pos, ch);
            old_hp = nlarn->p->hp;
        }
        else
        {
            ch = display_getch();

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
                old_hp = nlarn->p->hp;
                adj_corr = adjacent_corridor(nlarn->p->pos, ch);
                break;
            case 'w': /* rest up to 1 mobul */
                ch = '.';
                run_cmd = ch;
                old_hp = nlarn->p->hp;
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
            log_add_entry(nlarn->p->log, strbuf);
            g_free(strbuf);
            break;

            /* look at different position */
        case ';':
            pos = display_get_position(nlarn->p,
                                       "Choose a position to examine:",
                                       FALSE, FALSE, 0, FALSE);

            if (pos_valid(pos))
            {
                strbuf = map_pos_examine(pos);
                log_add_entry(nlarn->p->log, strbuf);
                g_free(strbuf);
            }
            else
                log_add_entry(nlarn->p->log, "Aborted.");

            break;

            /* pick up */
        case ',':
            moves_count = player_pickup(nlarn->p);
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
                display_show_message("Help for The Caverns of NLarn", strbuf, 0);
                g_free(strbuf);
            }
            else
            {
                display_show_message("Help for The Caverns of NLarn",
                                     "\n The help file could not be found. \n", 0);
            }
            break;

            /* work magic */
        case 'm':
            moves_count = spell_cast(nlarn->p);
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

            /* display inventory weight */
        case 'g':
            log_add_entry(nlarn->p->log, "The weight of your inventory is %s.",
                          player_inv_weight(nlarn->p));
            break;

            /* display inventory */
        case 'i':
            player_inv_display(nlarn->p);
            break;

            /* desecrate altar */
        case 'A':
            moves_count = player_altar_desecrate(nlarn->p);
            break;

            /* bank account information */
        case '$':
            log_add_entry(nlarn->p->log, "There %s %s gp on your bank account.",
                          (nlarn->p->bank_account == 1) ? "is" : "are",
                          int2str(nlarn->p->bank_account));
            break;

            /* close door */
        case 'c':
            moves_count = player_door_close(nlarn->p);
            break;

            /* open door / container */
        case 'o':
            if (inv_length_filtered(*map_ilist_at(game_map(nlarn, nlarn->p->pos.z), nlarn->p->pos),
                                    &inv_filter_container) > 0)
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
                log_add_entry(nlarn->p->log, "You presently owe %d gp in taxes.",
                              nlarn->p->outstanding_taxes);
            }
            else
                log_add_entry(nlarn->p->log, "You do not owe any taxes.");
            break;

            /* drink from fountain */
        case 'q':
            moves_count = player_fountain_drink(nlarn->p);
            break;

            /* remove gems from throne */
        case 'R':
            moves_count = player_throne_pillage(nlarn->p);
            break;

            /* sit on throne */
        case 's':
            moves_count = player_throne_sit(nlarn->p);
            break;

        case 'v':
            log_add_entry(nlarn->p->log, "NLarn version %d.%d.%d, built on %s.",
                          VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, __DATE__);
            break;

            /* wash at fountain */
        case 'W':
            moves_count = player_fountain_wash(nlarn->p);
            break;

        case '\\':
            if ((strbuf = player_item_identified_list(nlarn->p)))
            {
                display_show_message("Identified items", strbuf, 0);
                g_free(strbuf);
            }
            else
            {
                log_add_entry(nlarn->p->log, "You have not discovered any item yet.");
            }
            break;

            /* save */
        case 'S':
            if (game_save(nlarn, save_file_name))
            {
                /* only terminate the game if saving was successful */
                game_destroy(nlarn);
                exit(EXIT_SUCCESS);
            }
            break;

            /* quit */
        case KEY_F(12) :
        case 'Q':
            if (display_get_yesno("Are you sure you want to quit?", NULL, NULL))
                player_die(nlarn->p, PD_QUIT, 0);
            break;

            /* redraw screen */
        case 12: /* ^L */
            clear();
            display_draw();
            break;

            /* message log browser */
        case 18: /* ^R */
            display_show_history(nlarn->p->log, "Message history");
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
                    log_add_entry(nlarn->p->log, "Wizard mode has been activated.");
                }
             }
            else
            {
                log_add_entry(nlarn->p->log, "Wizard mode is already enabled.");
            }
            break;

            /* *** DEBUGGING SUPPORT *** */
        case '*':
            if (game_wizardmode(nlarn)) nlarn->p->bank_account += 1000;
            break;

        case '+': /* dungeon map up */
            if (game_wizardmode(nlarn) && (nlarn->p->pos.z > 0))
            {
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, nlarn->p->pos.z - 1),
                                               nlarn->p->pos.z == MAP_DMAX);
            }
            break;

        case '-': /* dungeon map down */
            if (game_wizardmode(nlarn) && (nlarn->p->pos.z < (MAP_MAX - 1)))
            {
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, nlarn->p->pos.z + 1),
                                               nlarn->p->pos.z == MAP_DMAX - 1);
            }
            break;

        case 't': /* intra-level teleport */
            if (game_wizardmode(nlarn))
            {
                nlarn->p->pos = map_find_space(game_map(nlarn, nlarn->p->pos.z),
                                               LE_MONSTER, FALSE);
            }
            break;

        case 'x': /* gain experience level */
        case 562: /* ^up */
        case 480: /* same for PDCurses */
            if (game_wizardmode(nlarn))
                player_level_gain(nlarn->p, 1);

            break;

        case 521: /* ^down - lose experience level */
        case 481: /* same for PDCurses */
            if (game_wizardmode(nlarn))
                player_level_lose(nlarn->p, 1);

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
            }
            break;
        }

        gboolean no_move = (moves_count == 0);

        /* manipulate game time */
        if (moves_count)
        {
            player_make_move(nlarn->p, moves_count);
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
            if (no_move || nlarn->p->hp < old_hp
                || adjacent_monster(nlarn->p->pos, run_cmd == '.'))
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
                    || (nlarn->p->hp == nlarn->p->hp_max
                        && nlarn->p->mp == nlarn->p->mp_max
                        && !player_effect_get(nlarn->p, ET_CONFUSION)
                        && !player_effect_get(nlarn->p, ET_BLINDNESS)
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

        /* repaint screen */
        display_paint_screen(nlarn->p);
    }
    while (TRUE); /* main event loop */
}

static gboolean adjacent_monster(position p, gboolean ignore_eye)
{
    // Ignore adjacent umber hulk if already confused.
    const gboolean ignore_umber_hulk
        = ignore_eye && player_effect_get(nlarn->p, ET_CONFUSION);

    // Only ignore floating eye if already paralysed.
    if (ignore_eye && !player_effect_get(nlarn->p, ET_PARALYSIS))
        ignore_eye = FALSE;

    int i, j;
    for (i = -1; i <= 1; i++)
        for (j = -1; j <= 1; j++)
        {
            if (i == 0 && j == 0)
                continue;

            position pos = p;
            pos.x += i;
            pos.y += j;

            if (pos.x < 0 || pos.x >= MAP_MAX_X
                || pos.y < 0 || pos.y >= MAP_MAX_Y)
            {
                continue;
            }

            monster *m = map_get_monster_at(game_map(nlarn, nlarn->p->pos.z), pos);
            if (m == NULL)
                continue;

            if (ignore_eye && monster_type(m) == MT_FLOATING_EYE)
                continue;

            if (ignore_umber_hulk && monster_type(m) == MT_UMBER_HULK)
                continue;

            if (monster_in_sight(m))
                return TRUE;
        }

    return FALSE;
}

static gboolean adjacent_corridor(position pos, char move)
{
    position p1 = pos, p2 = pos;
    switch (move)
    {
    case 'h': // left
        p1.x -= 1; p1.y -= 1;
        p2.x -= 1; p2.y += 1;
        break;
    case 'j': // down
        p1.x -= 1; p1.y += 1;
        p2.x += 1; p2.y += 1;
        break;
    case 'k': // up
        p1.x -= 1; p1.y -= 1;
        p2.x += 1; p2.y -= 1;
        break;
    case 'l': // right
        p1.x += 1; p1.y -= 1;
        p2.x += 1; p2.y += 1;
        break;
    case 'y': // up left
        p1.y -= 1;
        p2.x -= 1;
        break;
    case 'u': // up right
        p1.y -= 1;
        p2.x += 1;
        break;
    case 'b': // down left
        p1.x -= 1;
        p2.y += 1;
        break;
    case 'n': // down right
        p1.x += 1;
        p2.y += 1;
        break;
    }

    if (p1.x >= 0 && p1.x < MAP_MAX_X && p1.y >= 0 && p1.y < MAP_MAX_Y
        && lt_is_passable(map_tiletype_at(game_map(nlarn, nlarn->p->pos.z), p1)))
    {
        return TRUE;
    }
    if (p2.x >= 0 && p2.x < MAP_MAX_X && p2.y >= 0 && p2.y < MAP_MAX_Y
        && lt_is_passable(map_tiletype_at(game_map(nlarn, nlarn->p->pos.z), p2)))
    {
        return TRUE;
    }

    return FALSE;
}
