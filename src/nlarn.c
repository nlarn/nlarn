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

int main(int argc, char *argv[])
{
    game *g;

    /* count of moves used by last action */
    int moves_count = 0;
    /* next level for level changes */
    level *nlevel = NULL;
    /* position used to interact with stationaries */
    position pos;
    /* used to read in e.g. the help file */
    gchar *file_content;

    /* possible directions of actions */
    int *dirs;

    /* direction of action */
    int dir = GD_NONE;

    /* a counter and another one */
    int count, num;

    /* a monster */
    monster *m;

    /* visual range */
    int visrange;


    printf("%s",
           "NLarn Copyright (C) 2009  Joachim de Groot\n\n"
           "This program comes with ABSOLUTELY NO WARRANTY.\n"
           "This is free software, and you are welcome to\n"
           "redistribute it under certain conditions.\n\n");

    g = game_new(argc, argv);

    /* put the player into the dungeon */
    player_level_enter(g->p, g->levels[0]);

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
        switch (getch())
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
            g->p->settings.auto_pickup = !(g->p->settings.auto_pickup);
            log_add_entry(g->p->log, "Auto-pickup is %s.",
                          g->p->settings.auto_pickup ? "on" : "off");
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
                                     "Help file not found.");
            }

            break;

            /* cast a spell */
        case 'c':
            moves_count = player_spell_cast(g->p);
            break;

            /* go down stairs */
        case '>':
            switch (level_stationary_at(g->p->level, g->p->pos))
            {
            case LS_STAIRSDOWN:
                nlevel = g->levels[g->p->level->nlevel + 1];
                break;

            case LS_ELEVATORDOWN:
                /* first vulcano level */
                nlevel = g->levels[LEVEL_MAX - 1];
                break;

            case LS_ENTRANCE:
                if (g->p->level->nlevel == 0)
                    nlevel = g->levels[1];
                else
                    log_add_entry(g->p->log, "Climb up to return to town.");
                break;

            default:
                nlevel = NULL;
                log_add_entry(g->p->log, "I see no stairway down here.");
            }

            break;

            /* go up stairs */
        case '<':
            switch (level_stationary_at(g->p->level, g->p->pos))
            {
            case LS_STAIRSUP:
                nlevel = g->levels[g->p->level->nlevel - 1];
                break;

            case LS_ELEVATORUP:
                /* return to town */
                nlevel = g->levels[0];
                break;

            case LS_ENTRANCE:
                if (g->p->level->nlevel == 1)
                    nlevel = g->levels[0];
                else
                    log_add_entry(g->p->log, "Climb down to enter the dungeon.");

                break;
            default:
                nlevel = NULL;
                log_add_entry(g->p->log, "I see no stairway up here.");
            }
            break;

        case 'e':
            switch (level_stationary_at(g->p->level, g->p->pos))
            {
            case LS_BANK:
            case LS_BANK2:
                moves_count = building_bank(g->p);
                break;

            case LS_DNDSTORE:
                moves_count = building_dndstore(g->p);
                break;

            case LS_HOME:
                moves_count = building_home(g->p);
                break;

            case LS_LRS:
                moves_count = building_lrs(g->p);
                break;

            case LS_SCHOOL:
                moves_count = building_school(g->p);
                break;

            case LS_TRADEPOST:
                moves_count = building_tradepost(g->p);
                break;

            default:
                log_add_entry(g->p->log, "I seen no building here.");

            }

            break;

        case 'i':
            player_inv_display(g->p);
            break;

            /* close door */
        case 'C':

            dirs = level_get_surrounding(g->p->level,
                                         g->p->pos,
                                         LS_OPENDOOR);

            for (count = 0, num = 1; num < GD_MAX; num++)
            {
                if (dirs[num])
                {
                    count++;
                    dir = num;
                }
            }

            if (count > 1)
            {
                dir = display_get_direction("Close which door?", dirs);
                g_free(dirs);
            }
            /* dir has been set in the for loop above if count == 1 */
            else if (count == 0)
            {
                dir = GD_NONE;
            }

            /* select random direction if player is confused */
            if (player_effect(g->p, ET_CONFUSION))
            {
                dir = rand_0n(GD_MAX - 1);
            }

            if (dir)
            {
                pos = pos_move(g->p->pos, dir);
                if (pos_valid(pos)
                    && (level_stationary_at(g->p->level, pos) == LS_OPENDOOR))
                {

                    /* check if player is standing in the door */
                    if ((pos.x == g->p->pos.x) && (pos.y == g->p->pos.y))
                    {
                        log_add_entry(g->p->log, "Please step out of the doorway.");
                        break;
                    }

                    /* check for monster in the doorway */
                    m = level_get_monster_at(g->p->level, pos);

                    if (m)
                    {
                        log_add_entry(g->p->log,
                                      "You cannot close the door. The %s is in the way.",
                                      monster_get_name(m));
                        break;
                    }

                    /* check for items in the doorway */
                    if (level_ilist_at(g->p->level, pos))
                    {
                        log_add_entry(g->p->log,
                                      "You cannot close the door. There is something in the way.");
                        break;
                    }

                    level_stationary_at(g->p->level, pos) = LS_CLOSEDDOOR;
                    log_add_entry(g->p->log, "You close the door.");
                    moves_count = 1;
                }
                else
                {
                    log_add_entry(g->p->log, "Huh?");
                }

            }
            else
            {
                log_add_entry(g->p->log,
                              "Which door are you talking about?");
            }

            break;

            /* open door */
        case 'O':
            dirs = level_get_surrounding(g->p->level,
                                         g->p->pos,
                                         LS_CLOSEDDOOR);

            for (count = 0, num = 1; num < GD_MAX; num++)
            {
                if (dirs[num])
                {
                    count++;
                    dir = num;
                }
            }

            if (count > 1)
            {
                dir = display_get_direction("Open which door?", dirs);
                g_free(dirs);
            }
            /* dir has been set in the for loop above if count == 1 */
            else if (count == 0)
            {
                dir = GD_NONE;
            }

            /* select random direction if player is confused */
            if (player_effect(g->p, ET_CONFUSION))
            {
                dir = rand_0n(GD_MAX - 1);
            }

            if (dir)
            {
                pos = pos_move(g->p->pos, dir);
                if (pos_valid(pos)
                    && (level_stationary_at(g->p->level, pos) == LS_CLOSEDDOOR))
                {
                    g->p->level->map[pos.y][pos.x].stationary = LS_OPENDOOR;
                    log_add_entry(g->p->log, "You open the door.");
                    moves_count = 1;
                }
                else
                {
                    log_add_entry(g->p->log, "Huh?");
                }
            }
            else
            {
                log_add_entry(g->p->log,
                              "Which door are you talking about?");
            }

            break;

        case 'P':
            if (g->p->outstanding_taxes)
                log_add_entry(g->p->log,
                              "You presently owe %d gp in taxes.",
                              g->p->outstanding_taxes);
            else
                log_add_entry(g->p->log, "You do not owe any taxes.");
            break;

        case 'v':
            log_add_entry(g->p->log, "NLarn version %d.%d.%d, built on %s.",
                          VERSION_MAJOR,
                          VERSION_MINOR,
                          VERSION_PATCH,
                          __DATE__);
            break;

        case KEY_F(12) :
        case 'Q':
            if (display_get_yesno("Are you sure you want to quit?", NULL, NULL))
                player_die(g->p, PD_QUIT, 0);
            break;

        case 12: /* ^L */
            clear();
            display_draw();
            break;

            /* message log browser */
        case 18: /* ^R */
            display_show_history(g->p->log, "Message history");
            break;

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
        case '+': /* UP */
            if (game_wizardmode(g) && (g->p->level->nlevel > 0))
                nlevel = g->levels[g->p->level->nlevel - 1];

            break;

        case '-': /* DOWN */
            if (game_wizardmode(g) && (g->p->level->nlevel < (LEVEL_MAX - 1)))
                nlevel = g->levels[g->p->level->nlevel + 1];

            break;

        case 562: /* ^up */
            if (game_wizardmode(g))
                player_lvl_gain(g->p, 1);

            break;

        case 521: /* ^down */
            if (game_wizardmode(g))
                player_lvl_lose(g->p, 1);

            break;
        }

        /* if told to switch level, do so */
        if (nlevel != NULL)
        {
            moves_count = player_level_enter(g->p, nlevel);
            nlevel = NULL;
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

    /* should not be able to reach this point */
    return EXIT_FAILURE;
}
