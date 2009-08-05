/*
 * main.c
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

#include "nlarn.h"
/* needed for the key definitions */
#include <curses.h>

int main(int argc, char *argv[])
{
    game *g;

    /* count of moves used by last action */
    int moves_count = 0;

    /* used to read in e.g. the help file */
    gchar *file_content;

    /* visual range */
    int visrange;

    printf("%s",
           "NLarn Copyright (C) 2009  Joachim de Groot\n\n"
           "This program comes with ABSOLUTELY NO WARRANTY.\n"
           "This is free software, and you are welcome to\n"
           "redistribute it under certain conditions.\n\n");

    g = game_new(argc, argv);

    /* put the player into the town */
    player_level_enter(g->p, g->levels[0], FALSE);

    display_init();
    display_draw();

    /* check if mesgfile exists */
    if (!g_file_get_contents(game_mesgfile(g), &file_content, NULL, NULL))
    {
        display_shutdown();
        game_destroy(g);

        fprintf(stderr, "Error: Cannot find message file.\n");
        exit(EXIT_FAILURE);
    }

    display_show_message("Welcome to the game of NLarn!", file_content);
    g_free(file_content);

    display_paint_screen(g->p);

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
            moves_count = player_move(g->p, GD_WEST);
            break;

        case 'y':
        case '7':
        case KEY_HOME:
        case KEY_A1:
            moves_count = player_move(g->p, GD_NW);
            break;

        case 'l':
        case '6':
        case KEY_RIGHT:
#ifdef KEY_B3
        case KEY_B3:
#endif
            moves_count = player_move(g->p, GD_EAST);
            break;

        case 'n':
        case '3':
        case KEY_NPAGE:
        case KEY_C3:
            moves_count = player_move(g->p, GD_SE);
            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif
            moves_count = player_move(g->p, GD_NORTH);
            break;

        case 'u':
        case '9':
        case KEY_PPAGE:
        case KEY_A3:
            moves_count = player_move(g->p, GD_NE);
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            moves_count = player_move(g->p, GD_SOUTH);
            break;

        case 'b':
        case '1':
        case KEY_END:
        case KEY_C1:
            moves_count = player_move(g->p, GD_SW);
            break;

            /* look */
        case ':':
            moves_count = player_examine(g->p, g->p->pos);
            break;

            /* pick up */
        case ',':
            moves_count = player_pickup(g->p);
            break;

        case '@':
            display_config_autopickup(g->p);
            player_autopickup_show(g->p);
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
            if (g_file_get_contents(game_helpfile(g), &file_content, NULL, NULL))
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
            moves_count = player_spell_cast(g->p);
            break;

            /* go down stairs */
        case '>':
            moves_count = player_stairs_down(g->p);
            break;

            /* go up stairs */
        case '<':
            moves_count = player_stairs_up(g->p);
            break;

            /* enter a building */
        case 'e':
            moves_count = player_building_enter(g->p);
            break;

            /* display inventory weight */
        case 'g':
            log_add_entry(g->p->log, "The weight of your inventory is %s.",
                          player_inv_weight(g->p));
            break;

            /* display inventory */
        case 'i':
            player_inv_display(g->p);
            break;

            /* desecrate altar */
        case 'A':
            moves_count = player_altar_desecrate(g->p);
            break;

            /* close door */
        case 'C':
            moves_count = player_door_close(g->p);
            break;

            /* drink from fountain */
        case 'D':
            moves_count = player_fountain_drink(g->p);
            break;

            /* open door */
        case 'O':
            moves_count = player_door_open(g->p);
            break;

            /* pray at altar */
        case 'p':
            moves_count = player_altar_pray(g->p);
            break;

        case 'P':
            if (g->p->outstanding_taxes)
                log_add_entry(g->p->log, "You presently owe %d gp in taxes.",
                              g->p->outstanding_taxes);
            else
                log_add_entry(g->p->log, "You do not owe any taxes.");
            break;

            /* remove gems from throne */
        case 'R':
            moves_count = player_throne_pillage(g->p);
            break;

            /* sit on throne */
        case 'S':
            moves_count = player_throne_sit(g->p);
            break;

            /* tidy up at fountain */
        case 't':
            moves_count = player_fountain_wash(g->p);
            break;

        case 'v':
            log_add_entry(g->p->log, "NLarn version %d.%d.%d, built on %s.",
                          VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, __DATE__);
            break;

        case '\\':
            file_content = player_item_identified_list(g->p);
            display_show_message("Identified items", file_content);
            g_free(file_content);
            break;

        case KEY_F(12) :
        case 'Q':
            if (display_get_yesno("Are you sure you want to quit?", NULL, NULL))
                player_die(g->p, PD_QUIT, 0);
            break;

            /* redraw screen */
        case 12: /* ^L */
            clear();
            display_draw();
            break;

            /* message log browser */
        case 18: /* ^R */
            display_show_history(g->p->log, "Message history");
            break;

            /* enable wizard mode */
        case 23: /* ^W */
            if (!game_wizardmode(g))
            {
                if (display_get_yesno("Are you sure you want to switch to Wizard mode?\n" \
                                      "You will not be able to switch back to normal " \
                                      "gameplay and your score will not be counted.", NULL, NULL))
                    game_wizardmode(g) = TRUE;
                log_add_entry(g->p->log, "Wizard mode has been activated.");
            }
            else
            {
                log_add_entry(g->p->log, "Wizard mode is already enabled.");
            }

            break;

            /* *** DEBUGGING SUPPORT *** */
        case '+': /* dungeon level up */
            if (game_wizardmode(g) && (g->p->level->nlevel > 0))
                moves_count = player_level_enter(g->p, g->levels[g->p->level->nlevel - 1], TRUE);

            break;

        case '-': /* dungeon level down */
            if (game_wizardmode(g) && (g->p->level->nlevel < (LEVEL_MAX - 1)))
                moves_count = player_level_enter(g->p, g->levels[g->p->level->nlevel + 1], TRUE);
            break;

        case 562: /* ^up - gain experience level */
            if (game_wizardmode(g))
                player_lvl_gain(g->p, 1);

            break;

        case 521: /* ^down - lose experience level */
            if (game_wizardmode(g))
                player_lvl_lose(g->p, 1);

            break;
        }

        /* manipulate game time */
        if (moves_count)
        {
            game_spin_the_wheel(g, moves_count);

            g->p->stats.moves_made += moves_count;
            moves_count = 0;
        }

        /* recalculate FOV */
        visrange = (player_effect(g->p, ET_BLINDNESS) ? 0 : 6 + player_effect(g->p, ET_AWARENESS));
        player_update_fov(g->p, visrange);

        /* repaint screen */
        display_paint_screen(g->p);
    }
    while (TRUE); /* main event loop */
}
