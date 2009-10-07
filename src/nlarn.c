/*
 * nlarn.c
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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
    gchar *file_content;

    /* visual range */
    int visrange;

    /* position to examine */
    position pos;

    printf("%s",
           "NLarn Copyright (C) 2009  Joachim de Groot\n\n"
           "This program comes with ABSOLUTELY NO WARRANTY.\n"
           "This is free software, and you are welcome to\n"
           "redistribute it under certain conditions.\n\n");

    game_new(argc, argv);

    /* put the player into the town */
    player_level_enter(nlarn->p, nlarn->levels[0], FALSE);

    /* give player knowledge of the town */
    scroll_mapping(nlarn->p, NULL);

    display_init();
    display_draw();

    /* check if mesgfile exists */
    if (!g_file_get_contents(game_mesgfile(nlarn), &file_content, NULL, NULL))
    {
        display_shutdown();
        game_destroy(nlarn);

        fprintf(stderr, "Error: Cannot find message file.\n");
        exit(EXIT_FAILURE);
    }

    display_show_message("Welcome to the game of NLarn!", file_content);
    g_free(file_content);

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
            moves_count = player_examine(nlarn->p, nlarn->p->pos);
            break;

            /* look at different position */
        case ';':
            pos = display_get_position(nlarn->p,
                                       "Choose a position to examine:",
                                       FALSE, FALSE);

            if (pos_valid(pos))
                player_examine(nlarn->p, pos);
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
            if (g_file_get_contents(game_helpfile(nlarn), &file_content, NULL, NULL))
            {
                display_show_message("Help for The Caverns of NLarn", file_content);
                g_free(file_content);
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

            /* close door */
        case 'C':
            moves_count = player_door_close(nlarn->p);
            break;

            /* drink from fountain */
        case 'D':
            moves_count = player_fountain_drink(nlarn->p);
            break;

            /* open door / container */
        case 'O':
            if (inv_length_filtered(*level_ilist_at(nlarn->p->level, nlarn->p->pos),
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

            /* remove gems from throne */
        case 'R':
            moves_count = player_throne_pillage(nlarn->p);
            break;

            /* sit on throne */
        case 'S':
            moves_count = player_throne_sit(nlarn->p);
            break;

            /* tidy up at fountain */
        case 't':
            moves_count = player_fountain_wash(nlarn->p);
            break;

        case 'v':
            log_add_entry(nlarn->p->log, "NLarn version %d.%d.%d, built on %s.",
                          VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, __DATE__);
            break;

        case '\\':
            if ((file_content = player_item_identified_list(nlarn->p)))
            {
                display_show_message("Identified items", file_content);
                g_free(file_content);
            }
            else
            {
                log_add_entry(nlarn->p->log, "You have not discovered any item yet.");
            }
            break;

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
                    game_wizardmode(nlarn) = TRUE;
                log_add_entry(nlarn->p->log, "Wizard mode has been activated.");
            }
            else
            {
                log_add_entry(nlarn->p->log, "Wizard mode is already enabled.");
            }

            break;

            /* *** DEBUGGING SUPPORT *** */
        case '+': /* dungeon level up */
            if (game_wizardmode(nlarn) && (nlarn->p->level->nlevel > 0))
                moves_count = player_level_enter(nlarn->p, nlarn->levels[nlarn->p->level->nlevel - 1], TRUE);

            break;

        case '-': /* dungeon level down */
            if (game_wizardmode(nlarn) && (nlarn->p->level->nlevel < (LEVEL_MAX - 1)))
                moves_count = player_level_enter(nlarn->p, nlarn->levels[nlarn->p->level->nlevel + 1], TRUE);
            break;

        case 562: /* ^up - gain experience level */
            if (game_wizardmode(nlarn))
                player_lvl_gain(nlarn->p, 1);

            break;

        case 521: /* ^down - lose experience level */
            if (game_wizardmode(nlarn))
                player_lvl_lose(nlarn->p, 1);

            break;
        }

        /* manipulate game time */
        if (moves_count)
        {
            game_spin_the_wheel(nlarn, moves_count);

            nlarn->p->stats.moves_made += moves_count;
            moves_count = 0;
        }

        /* recalculate FOV */
        visrange = (player_effect(nlarn->p, ET_BLINDNESS) ? 0 : 6 + player_effect(nlarn->p, ET_AWARENESS));
        player_update_fov(nlarn->p, visrange);

        /* repaint screen */
        display_paint_screen(nlarn->p);
    }
    while (TRUE); /* main event loop */
}
