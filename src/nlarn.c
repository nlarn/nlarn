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

#include "container.h"
#include "display.h"
#include "game.h"
#include "nlarn.h"

game *nlarn = NULL;

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
    if (g_file_test(save_file_name, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    {
        /* restore savegame */
        game_load(save_file_name, argc, argv);

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

    display_init();
    display_draw();

    /* check if mesgfile exists */
    if (!g_file_get_contents(game_mesgfile(nlarn), &strbuf, NULL, NULL))
    {
        display_shutdown();
        game_destroy(nlarn);

        fprintf(stderr, "Error: Cannot find message file.\n");
        exit(EXIT_FAILURE);
    }

    display_show_message("Welcome to the game of NLarn!", strbuf);
    g_free(strbuf);

    display_paint_screen(nlarn->p);

    /* main event loop */
    do
    {
        /* get key and analyze it */
        switch (display_getch())
        {
            /* *** MOVEMENT *** */
        case 'h':
        case '4':
        case KEY_LEFT:
#ifdef KEY_B1
        case KEY_B1:
#endif
            moves_count = player_move(nlarn->p, GD_WEST);
            break;

        case 'y':
        case '7':
        case KEY_HOME:
        case KEY_A1:
            moves_count = player_move(nlarn->p, GD_NW);
            break;

        case 'l':
        case '6':
        case KEY_RIGHT:
#ifdef KEY_B3
        case KEY_B3:
#endif
            moves_count = player_move(nlarn->p, GD_EAST);
            break;

        case 'n':
        case '3':
        case KEY_NPAGE:
        case KEY_C3:
            moves_count = player_move(nlarn->p, GD_SE);
            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif
            moves_count = player_move(nlarn->p, GD_NORTH);
            break;

        case 'u':
        case '9':
        case KEY_PPAGE:
        case KEY_A3:
            moves_count = player_move(nlarn->p, GD_NE);
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            moves_count = player_move(nlarn->p, GD_SOUTH);
            break;

        case 'b':
        case '1':
        case KEY_END:
        case KEY_C1:
            moves_count = player_move(nlarn->p, GD_SW);
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
                                       FALSE, FALSE);

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
                display_show_message("Help for The Caverns of NLarn", strbuf);
                g_free(strbuf);
            }
            else
            {
                display_show_message("Help for The Caverns of NLarn",
                                     "\n The help file could not be found. \n");
            }

            break;

            /* cast a spell */
        case 'c':
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
        case 'B':
            log_add_entry(nlarn->p->log, "There are %s gp on your bank account.",
                          int2str(nlarn->p->bank_account));
            break;

            /* close door */
        case 'C':
            moves_count = player_door_close(nlarn->p);
            break;

            /* open door / container */
        case 'O':
            if (inv_length_filtered(*map_ilist_at(game_map(nlarn, nlarn->p->pos.z), nlarn->p->pos),
                                    &inv_filter_container) > 0)
            {
                container_open(nlarn->p, NULL, NULL);
            }
            else
            {
                moves_count = player_door_open(nlarn->p);
            }
            break;

            /* pray at altar */
        case 'p':
            moves_count = player_altar_pray(nlarn->p);
            break;

        case 'P':
            if (nlarn->p->outstanding_taxes)
                log_add_entry(nlarn->p->log, "You presently owe %d gp in taxes.",
                              nlarn->p->outstanding_taxes);
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
                display_show_message("Identified items", strbuf);
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
                display_shutdown();
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
        case '$':
            if (game_wizardmode(nlarn)) nlarn->p->bank_account += 1000;
            break;

        case '+': /* dungeon map up */
            if (game_wizardmode(nlarn) && (nlarn->p->pos.z > 0))
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, nlarn->p->pos.z - 1), TRUE);

            break;

        case '-': /* dungeon map down */
            if (game_wizardmode(nlarn) && (nlarn->p->pos.z < (MAP_MAX - 1)))
                moves_count = player_map_enter(nlarn->p, game_map(nlarn, nlarn->p->pos.z + 1), TRUE);
            break;

        case 562: /* ^up - gain experience level */
        case 480: /* same for PDCurses */
            if (game_wizardmode(nlarn))
                player_level_gain(nlarn->p, 1);

            break;

        case 521: /* ^down - lose experience level */
        case 481: /* same for PDCurses */
            if (game_wizardmode(nlarn))
                player_level_lose(nlarn->p, 1);

            break;
        }

        /* manipulate game time */
        if (moves_count)
        {
            game_spin_the_wheel(nlarn, moves_count);
            moves_count = 0;
        }

        /* recalculate FOV */
        player_update_fov(nlarn->p);

        /* repaint screen */
        display_paint_screen(nlarn->p);
    }
    while (TRUE); /* main event loop */
}
