/*
 * display.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

#include <assert.h>
#include <curses.h>
#include <panel.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "nlarn.h"
#include "spheres.h"

static guint display_rows = 0;
static guint display_cols = 0;

/* linked list of opened windows */
static GList *windows = NULL;

static display_window *display_window_new(int x1, int y1, int width, int height, char *title, char *caption);
static void display_window_destroy(display_window *dwin, gboolean shall_clear);

static int display_window_move(display_window *dwin, int key);
static void display_window_update_caption(display_window *dwin);
static void display_window_update_arrow_up(display_window *dwin, gboolean on);
static void display_window_update_arrow_down(display_window *dwin, gboolean on);

static void display_windows_hide();
static void display_windows_show();

static void display_spheres_paint(sphere *s, player *p);

int display_init()
{
    /* taken from Angband 3.1.1 */
    /* overwrite bit is false, so users on real (serial) terminals can override
       this; terminal emulators drop an entire sequence into the input buffer at
       once, and I/O-buffering makes the packet very unlikely to be split even
       over networks */
#ifndef WIN32
    setenv("ESCDELAY", "0", 0);
#endif

    /* Start curses mode */
    initscr();

    /* get screen dimensions */
    getmaxyx(stdscr, display_rows, display_cols);
    start_color();

    /* black background */
    init_pair(DCP_WHITE_BLACK,   COLOR_WHITE,   COLOR_BLACK);
    init_pair(DCP_RED_BLACK,     COLOR_RED,     COLOR_BLACK);
    init_pair(DCP_GREEN_BLACK,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(DCP_BLUE_BLACK,    COLOR_BLUE,    COLOR_BLACK);
    init_pair(DCP_YELLOW_BLACK,  COLOR_YELLOW,  COLOR_BLACK);
    init_pair(DCP_MAGENTA_BLACK, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(DCP_CYAN_BLACK,    COLOR_CYAN,    COLOR_BLACK);
    init_pair(DCP_BLACK_BLACK,   COLOR_BLACK,   COLOR_BLACK);

    /* these colour pairs are used by dialogs */
    init_pair(DCP_WHITE_RED,    COLOR_WHITE,    COLOR_RED);
    init_pair(DCP_RED_WHITE,    COLOR_RED,      COLOR_WHITE);
    init_pair(DCP_BLUE_RED,     COLOR_BLUE,     COLOR_RED);
    init_pair(DCP_YELLOW_RED,   COLOR_YELLOW,   COLOR_RED);
    init_pair(DCP_BLACK_WHITE,  COLOR_BLACK,    COLOR_WHITE);
    init_pair(DCP_CYAN_RED,     COLOR_CYAN,     COLOR_RED);

    /* control special keys in application */
    raw();

    /* supress input echo */
    noecho();

    /* enable function keys */
    keypad(stdscr, TRUE);

    /* want all 8 bits */
    meta(stdscr, TRUE);

    /* make cursor invisible */
    curs_set(0);

    return TRUE;
}

int display_paint_screen(player *p)
{
    guint x, y, i;
    position pos;
    map *map;

    /* curses attributes */
    int attrs;

    /* needed to display messages */
    message_log_entry *le;

    /* storage for formatted messages */
    GPtrArray *text = NULL;

    /* storage for the game time of messages */
    guint *ttime = NULL;

    /* refresh screen dimensions */
    getmaxyx(stdscr, display_rows, display_cols);

    /* draw line around map */
    mvhline(MAP_MAX_Y, 0, ACS_HLINE, MAP_MAX_X);
    mvvline(0, MAP_MAX_X, ACS_VLINE, MAP_MAX_Y);
    mvaddch(MAP_MAX_Y, MAP_MAX_X, ACS_LRCORNER);

    /* make shortcut */
    map = game_map(nlarn, p->pos.z);

    /* draw map */
    pos.z = p->pos.z;
    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        /* position cursor */
        move(pos.y, 0);

        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            if (game_wizardmode(nlarn))
            {
                /* draw the truth */
                if (inv_length(*map_ilist_at(map, pos)) > 0)
                {
                    /* draw items */
                    item *it = inv_get(*map_ilist_at(map, pos),
                                       inv_length(*map_ilist_at(map, pos)) - 1);

                    if (player_pos_visible(p, pos))
                        attron(attrs = DC_BLUE);
                    else
                        attron(attrs = DC_LIGHTGRAY);

                    addch(item_image(it->type));
                    attroff(attrs);
                }

                else if (map_sobject_at(map, pos))
                {
                    /* draw sobject stuff */
                    if (player_pos_visible(p, pos))
                        attron(attrs = ls_get_colour(map_sobject_at(map, pos)));
                    else
                        attron(attrs = DC_DARKGRAY);

                    addch(ls_get_image(map_sobject_at(map, pos)));
                    attroff(attrs);
                }
                else if (map_trap_at(map, pos))
                {
                    if (player_pos_visible(p, pos))
                        attron(attrs = DC_MAGENTA);
                    else
                        attron(attrs = DC_LIGHTGRAY);

                    addch('^');
                    attroff(attrs);
                }

                else
                {
                    /* draw tile */
                    if (player_pos_visible(p, pos))
                        attron(attrs = lt_get_colour(map_tiletype_at(map, pos)));
                    else
                        attron(attrs = DC_DARKGRAY);

                    addch(lt_get_image(map_tiletype_at(map, pos)));
                    attroff(attrs);
                }
            }
            else /* i.e. !wizardmode */
                /* draw players fov & memory */
            {
                /* draw items */
                if (player_memory_of(p, pos).item)
                {
                    if (player_pos_visible(p, pos))
                        attron(attrs = DC_BLUE);
                    else
                        attron(attrs = DC_LIGHTGRAY);

                    addch(item_image(player_memory_of(p, pos).item));
                    attroff(attrs);
                }
                /* draw stationary objects */
                else if (player_memory_of(p, pos).sobject)
                {
                    if (player_pos_visible(p, pos))
                        attron(attrs = ls_get_colour(player_memory_of(p, pos).sobject));
                    else
                        attron(attrs = DC_DARKGRAY);

                    addch(ls_get_image(player_memory_of(p, pos).sobject));
                    attroff(attrs);
                }
                else if (player_memory_of(p, pos).trap)
                {
                    if (player_pos_visible(p, pos))
                        attron(attrs = DC_MAGENTA);
                    else
                        attron(attrs = DC_LIGHTGRAY);

                    addch('^');
                    attroff(attrs);
                }
                /* draw tile */
                else
                {
                    /* draw tile */
                    if (player_pos_visible(p, pos))
                        attron(attrs = lt_get_colour(map_tiletype_at(map, pos)));
                    else
                        attron(attrs = DC_DARKGRAY);

                    addch(lt_get_image(player_memory_of(p, pos).type));
                    attroff(attrs);
                }
            }

            /* draw monsters */
            monster *monst = map_get_monster_at(map, pos);

            if (monst == NULL)
            {
                /* no monster found */
                continue;
            }

            if (game_wizardmode(nlarn)
                    || player_effect(p, ET_DETECT_MONSTER)
                    || (player_pos_visible(p, monster_pos(monst))
                        && (!monster_is_invisible(monst) || player_effect(p, ET_INFRAVISION))
                        && !monster_unknown(monst))) /* hide the mimic */
            {
                attron(DC_RED);
                position mpos = monster_pos(monst);
                mvaddch(mpos.y, mpos.x, monster_image(monst));
                attroff(DC_RED);
            }
        }
    }

    /* draw spheres */
    g_ptr_array_foreach(nlarn->spheres, (GFunc)display_spheres_paint, p);

    /* draw player */
    char pc;
    if (player_effect(p, ET_INVISIBILITY))
    {
        pc = ' ';
        attrs = A_REVERSE | DC_WHITE;
    }
    else
    {
        pc = '@';
        attrs = DC_YELLOW;
    }

    attron(attrs);
    mvaddch(p->pos.y, p->pos.x, pc);
    attroff(attrs);


    /* *** status line *** */
    move(MAP_MAX_Y + 1, 0);
    clrtoeol();

    /* player name and level */
    printw("%s, %s", p->name, player_get_level_desc(p));

    /* current HPs */
    if (p->hp <= ((int)p->hp_max / 10)) /* 10% hp left */
        attrs = DC_LIGHTRED;
    else if (p->hp <= ((int)p->hp_max / 4))  /* 25% hp left */
        attrs = DC_RED;
    else if (p->hp <= ((int)p->hp_max / 2))  /* 50% hp left */
        attrs = DC_GREEN;
    else
        attrs = DC_LIGHTGREEN;

    attron(attrs);
    mvprintw(MAP_MAX_Y + 1, MAP_MAX_X - 21, "%3d", p->hp, player_get_hp_max(p));
    attroff(attrs);

    /* max HPs */
    attron(attrs = DC_LIGHTGREEN);
    mvprintw(MAP_MAX_Y + 1, MAP_MAX_X - 18, "/%-3d HP", player_get_hp_max(p));
    attroff(attrs);

    /* current MPs */
    if (p->mp <= ((int)p->mp_max / 10)) /* 10% mp left */
        attrs = DC_LIGHTMAGENTA;
    else if (p->mp <= ((int)p->mp_max / 4))  /* 25% mp left */
        attrs = DC_MAGENTA;
    else if (p->mp <= ((int)p->mp_max / 2))  /* 50% mp left */
        attrs = DC_CYAN;
    else
        attrs = DC_LIGHTCYAN;

    attron(attrs);
    mvprintw(MAP_MAX_Y + 1, MAP_MAX_X - 10, "%3d", p->mp);
    attroff(attrs);

    /* max MPs */
    attron(attrs = DC_LIGHTCYAN);
    mvprintw(MAP_MAX_Y + 1, MAP_MAX_X - 7, "/%-3d MP", player_get_mp_max(p));
    attroff(attrs);

    /* game time */
    mvprintw(MAP_MAX_Y + 1, MAP_MAX_X + 1, "T %-6d", game_turn(nlarn));

    /* experience points / level */
    move(MAP_MAX_Y + 2, 0);
    clrtoeol();

    attron(attrs = DC_LIGHTBLUE);

    mvprintw(MAP_MAX_Y + 2, MAP_MAX_X - 20, "XP %d/%-5d",
             p->experience, p->level);

    attroff(attrs);

    /* dungeon map */
    mvprintw(MAP_MAX_Y + 2, MAP_MAX_X + 1, "Lvl: %s", map_name(map));


    /* *** RIGHT STATUS *** */

    /* strenght */
    mvprintw(1, MAP_MAX_X + 3, "STR ");

    if (player_get_str(p) > (int)p->strength)
        attrs = DC_YELLOW;
    else if (player_get_str(p) < (int)p->strength)
        attrs = DC_LIGHTRED;
    else
        attrs = DC_WHITE;

    attron(attrs);
    printw("%2d", player_get_str(p));
    attroff(attrs);
    clrtoeol();

    /* dexterity */
    mvprintw(2, MAP_MAX_X + 3, "DEX ");

    if (player_get_dex(p) > (int)p->dexterity)
        attrs = DC_YELLOW;
    else if (player_get_dex(p) < (int)p->dexterity)
        attrs = DC_LIGHTRED;
    else
        attrs = DC_WHITE;

    attron(attrs);
    printw("%2d", player_get_dex(p));
    attroff(attrs);
    clrtoeol();

    /* constitution */
    mvprintw(3, MAP_MAX_X + 3, "CON ");

    if (player_get_con(p) > (int)p->constitution)
        attrs = DC_YELLOW;
    else if (player_get_con(p) < (int)p->constitution)
        attrs = DC_LIGHTRED;
    else
        attrs = DC_WHITE;

    attron(attrs);
    printw("%2d", player_get_con(p));
    attroff(attrs);
    clrtoeol();

    /* intelligence */
    mvprintw(4, MAP_MAX_X + 3, "INT ");

    if (player_get_int(p) > (int)p->intelligence)
        attrs = DC_YELLOW;
    else if (player_get_int(p) < (int)p->intelligence)
        attrs = DC_LIGHTRED;
    else
        attrs = DC_WHITE;

    attron(attrs);
    printw("%2d", player_get_int(p));
    attroff(attrs);
    clrtoeol();

    /* wisdom */
    mvprintw(5, MAP_MAX_X + 3, "WIS ");

    if (player_get_wis(p) > (int)p->wisdom)
        attrs = DC_YELLOW;
    else if (player_get_wis(p) < (int)p->wisdom)
        attrs = DC_LIGHTRED;
    else
        attrs = DC_WHITE;

    attron(attrs);
    printw("%2d", player_get_wis(p));
    attroff(attrs);
    clrtoeol();

    /* charisma */
    mvprintw(6, MAP_MAX_X + 3, "CHA ");

    if (player_get_cha(p) > (int)p->charisma)
        attrs = DC_YELLOW;
    else if (player_get_cha(p) < (int)p->charisma)
        attrs = DC_LIGHTRED;
    else
        attrs = DC_WHITE;

    attron(attrs);
    printw("%2d", player_get_cha(p));
    attroff(attrs) ;
    clrtoeol();

    /* clear line below charisma */
    move(7, MAP_MAX_X + 1);
    clrtoeol();

    /* armour class */
    mvprintw(8, MAP_MAX_X + 3, "AC: %2d", player_get_ac(p));
    clrtoeol();

    /* gold */
    mvprintw(9, MAP_MAX_X + 3, "$%-7d", player_get_gold(p));
    clrtoeol();

    /* clear line below gold */
    move(10, MAP_MAX_X + 1);
    clrtoeol();

    /* display negative effects */
    text = player_effect_text(p);

    /* clear lines */
    for (i = 0; i < 7; i++)
    {
        move(11 + i, MAP_MAX_X + 3);
        clrtoeol();
    }

    if (text->len > 0)
    {
        int available_space = display_cols - MAP_MAX_X - 4;

        attron(attrs = DC_LIGHTCYAN);

        for (i = 0; i < text->len; i++)
        {
            char *desc = g_strdup(g_ptr_array_index(text, i));

            if (strlen(desc) > available_space)
            {
                desc[available_space - 1] = '.';
                desc[available_space] = '\0';
            }

            mvprintw(11 + i, MAP_MAX_X + 3, desc);
            g_free(desc);
        }

        attroff(attrs);
    }

    g_ptr_array_free(text, TRUE);
    text = NULL; /* needed in this state below */

    /* *** MESSAGES *** */
    /* number of lines which can be displayed */
    y = display_rows - 20;

    /* storage for game time of message */
    ttime = g_new0(guint, y);

    /* hold original length of text */
    x = 1;

    /* line counter */
    i = 0;

    /* if log contains buffered messaged, display them */
    if (log_buffer(p->log))
    {
        text = text_wrap(log_buffer(p->log), display_cols, 2);
        for (x = 1; x <= min(text->len, y); x++)
            ttime[x - 1] = game_turn(nlarn);
    }

    /* retrieve game log and reformat messages to window width */
    while (((text == NULL) || (text->len < y)) && (log_length(p->log) > i))
    {
        le = log_get_entry(p->log, log_length(p->log) - 1 - i);

        if (text == NULL)
            text = text_wrap(le->message, display_cols, 2);
        else
            text = text_append(text, text_wrap(le->message, display_cols, 2));

        /* store game time for associated text line */
        while ((x <= text->len) && (x <= y))
        {
            ttime[x - 1] = le->gtime;
            x++;
        }

        i++;
    }

    for (y = 20, i = 0; (y < display_rows) && (i < text->len); i++, y++)
    {
        move(y, 0);
        clrtoeol();

        if ((p->log->gtime - 15) < ttime[i])
            attrs = DC_WHITE;
        else
            attrs = DC_LIGHTGRAY;

        attron(attrs);

        printw(g_ptr_array_index(text, i));

        attroff(attrs);
    }

    text_destroy(text);
    g_free(ttime);

    return display_draw();
}

void display_shutdown()
{
    /* End curses mode */
    endwin();
}

int display_draw()
{
    GList *win_ptr = g_list_first(windows);

    /* mark stdscr for redraw */
    refresh();

    /* prepare windows for redraw */
    if (win_ptr) do
        {
            display_window *win = (display_window *)win_ptr->data;
            redrawwin(win->window);
        }
        while ((win_ptr = g_list_next(win_ptr)));

    return doupdate();
}

/**
 * Generic inventory display function
 *
 * @param Window caption
 * @param player
 * @param inventory to display
 * @param a GPtrArray of display_inv_callbacks
 * @param display prices
 * @param a filter function: will be called for every item
 *
 */
item *display_inventory(char *title, player *p, inventory **inv,
                        GPtrArray *callbacks, int show_price,
                        int (*filter)(item *))
{
    display_window *iwin = NULL;
    guint width, height, maxvis;
    guint startx, starty;
    guint len_orig, len_curr;
    gboolean redraw = FALSE;

    gboolean keep_running = TRUE;

    /* temp var to assemble window caption from callback descriptions */
    char *tmp;

    /* used for looping over callbacks */
    guint cb_nr;
    display_inv_callback *cb;
    int key;

    /* time usage returned by callback function */
    int time;

    /* offset to element position (when displaying more than maxvis items) */
    guint offset = 0;

    /* currently selected item */
    guint curr = 1;

    /* position in inventory (loop var) */
    guint pos;
    item *it;
    char item_desc[81];

    /* curses attributes */
    int attrs;

    assert(p != NULL && inv != NULL);

    /* sort inventory by item type */
    inv_sort(*inv, (GCompareDataFunc)item_sort, (gpointer)p);

    /* store inventory length */
    len_orig = len_curr = inv_length_filtered(*inv, filter);

    if (show_price)
    {
        width = 59;
    }
    else
    {
        width = 54;
    }

    /* main loop */
    do
    {
        height = min((display_rows - 3), len_curr + 2);
        maxvis = min(len_curr, height - 2);

        starty = (display_rows - height) / 2; /* calculation for centered */
        startx = (min(MAP_MAX_X, display_cols) - width) / 2; /* placement of the window */

        /* fix marked item */
        if (curr > len_curr)
            curr = len_curr;

        /* rebuild image if needed */
        if (iwin != NULL && redraw)
        {
            /* save strarting point */
            startx = iwin->x1;
            starty = iwin->y1;

            display_window_destroy(iwin, FALSE);
            iwin = NULL;
            redraw = FALSE;

            display_paint_screen(p);

            /* check for inventory modifications */
            if (len_orig > len_curr)
            {
                /* inventory length is smaller than before */
                /* if on the last page, reduce offset */
                if ((offset > 0) && ((offset + maxvis) > len_curr))
                    offset--;

                /* remember current length */
                len_orig = len_curr;
            }
            else if (len_curr > len_orig)
            {
                /* inventory has grown */
                /* sort inventory by item type */
                inv_sort(*inv, (GCompareDataFunc)item_sort, (gpointer)p);
            }
        }

        if (!iwin)
        {
            iwin = display_window_new(startx, starty, width, height, title, NULL);
        }

        it = inv_get_filtered(*inv, curr + offset - 1, filter);

        /* assemble window caption (if callbacks have been defined) */
        for (cb_nr = 0; callbacks != NULL && cb_nr < callbacks->len; cb_nr++)
        {
            cb = g_ptr_array_index(callbacks, cb_nr);

            /* check if callback is approriate for this item */
            /* if no checkfunktion is set, always display item */
            if ((cb->checkfun == NULL) || cb->checkfun(p, it))
            {
                cb->active = TRUE;

                if (iwin->caption)
                {
                    tmp = g_strconcat(iwin->caption, " ", cb->description, NULL);
                    g_free(iwin->caption);
                    iwin->caption = tmp;
                }
                else
                {
                    iwin->caption = g_strdup(cb->description);
                }
            }
            else
            {
                /* it isn't */
                cb->active = FALSE;
            }
        }

        display_window_update_caption(iwin);

        if (iwin->caption)
        {
            g_free(iwin->caption);
            iwin->caption = NULL;
        }

        for (pos = 1; pos <= min(len_curr, maxvis); pos++)
        {
            it = inv_get_filtered(*inv, (pos - 1) + offset, filter);

            if ((curr == pos) && player_item_is_equipped(p, it))
                attrs = COLOR_PAIR(DCP_BLACK_WHITE);
            else if (curr == pos)
                attrs = COLOR_PAIR(DCP_RED_WHITE);
            else if (player_item_is_equipped(p, it))
                attrs = COLOR_PAIR(DCP_WHITE_RED) | A_BOLD;
            else
                attrs = COLOR_PAIR(DCP_WHITE_RED);

            wattron(iwin->window, attrs);

            if (show_price)
            {
                /* inside shop */
                mvwprintw(iwin->window, pos, 1, " %-48s %5d$ ",
                          item_describe(it, TRUE, FALSE, FALSE, item_desc, 80),
                          item_price(it));
            }
            else
            {
                mvwprintw(iwin->window, pos, 1, " %-48s %c ",
                          item_describe(it, player_item_known(p, it),
                                        FALSE, FALSE, item_desc, 80),
                          player_item_is_equipped(p, it) ? '*' : ' ');
            }

            wattroff(iwin->window, attrs);
        }

        display_window_update_arrow_up(iwin, offset > 0);
        display_window_update_arrow_down(iwin, (offset + maxvis) < len_curr);

        wrefresh(iwin->window);

        switch (key = getch())
        {

        case '7':
        case KEY_HOME:
        case KEY_A1:

            curr = 1;
            offset = 0;

            break;

        case '9':
        case KEY_PPAGE:
        case KEY_A3:

            if ((curr == maxvis) || offset == 0)
                curr = 1;
            else
                offset = (offset > maxvis) ? (offset - maxvis) : 0;

            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif

            if (curr > 1)
                curr--;

            else if ((curr == 1) && (offset > 0))
                offset--;

            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            if ((curr + offset) < len_curr)
            {
                if (curr == maxvis)
                    offset++;
                else
                    curr++;
            }

            break;

        case '3':
        case KEY_NPAGE:
        case KEY_C3:

            if (curr == 1)
            {
                curr = maxvis;
            }
            else
            {
                offset = offset + maxvis;

                if ((offset + maxvis) >= len_curr)
                {
                    curr = min(len_curr, maxvis);
                    offset = len_curr - curr;
                }
            }
            break;

        case '1':
        case KEY_END:
        case KEY_C1:

            if (len_curr > maxvis)
            {
                curr = maxvis;
                offset = len_curr - maxvis;
            }
            else
            {
                curr = len_curr;
            }

            break;

        case 27:
            keep_running = FALSE;
            break;

        case 10:
        case 13:
            if (callbacks == NULL)
            {
                /* if no callbacks have been defines enter selects item */
                keep_running = FALSE;
            }

        default:
            /* perhaps the window shall be moved */
            if (display_window_move(iwin, key))
            {
                break;
            }

            /* check callback function keys (if defined) */
            for (cb_nr = 1; callbacks != NULL && cb_nr <= callbacks->len; cb_nr++)
            {
                cb = g_ptr_array_index(callbacks, cb_nr - 1);

                if ((cb->key == key) && cb->active)
                {
                    time = 0;

                    /* trigger callback */
                    time = cb->function(p, cb->inv, inv_get_filtered(*inv, curr + offset - 1, filter));

                    if (time) game_spin_the_wheel(nlarn, time);

                    redraw = TRUE;
                }
            }

        };

        len_curr = inv_length_filtered(*inv, filter);

    }
    while (keep_running && (len_curr > 0)); /* ESC pressed or empty inventory*/

    display_window_destroy(iwin, TRUE);

    if ((callbacks == NULL) && (key != 27))
    {
        /* return selected item if no callbacks have been provided */
        return inv_get_filtered(*inv, offset + curr - 1, filter);
    }
    else
    {
        return NULL;
    }
}

void display_inv_callbacks_clean(GPtrArray *callbacks)
{
    if (!callbacks) return;

    while (callbacks->len > 0)
    {
        g_free(g_ptr_array_remove_index_fast(callbacks, callbacks->len - 1));
    }

    g_ptr_array_free(callbacks, TRUE);
}

void display_config_autopickup(player *p)
{
    display_window *cwin;
    int width, height;
    int startx, starty;
    int key; /* keyboard input */
    int RUN = TRUE;
    int attrs; /* curses attributes */

    item_t it;

    height = 6;
    width = max(36 /* length of first label */, IT_MAX * 2 + 1);

    starty = (display_rows - height) / 2;
    startx = (min(MAP_MAX_X, display_cols) - width) / 2;

    cwin = display_window_new(startx, starty, width, height, "Autopickup", NULL);

    wattron(cwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
    mvwprintw(cwin->window, 1, 2, "Enabled types are shown inverted");
    mvwprintw(cwin->window, 2, 7, "type symbol to toggle");
    wattroff(cwin->window, attrs);

    do
    {
        for (it = 1; it < IT_MAX; it++)
        {
            if (p->settings.auto_pickup[it])
                attrs = COLOR_PAIR(DCP_RED_WHITE);
            else
                attrs = COLOR_PAIR(DCP_WHITE_RED);

            wattron(cwin->window, attrs);
            mvwprintw(cwin->window, 4, 6 + it * 2, "%c", item_image(it));
            wattroff(cwin->window, attrs);
        }

        wrefresh(cwin->window);

        switch (key = getch())
        {
        case 10:
        case 13:
        case 27:
        case 32:
            RUN = FALSE;

        default:
            if (!display_window_move(cwin, key))
            {
                for (it = 1; it < IT_MAX; it++)
                {
                    if (item_image(it) == key)
                    {
                        p->settings.auto_pickup[it] = !p->settings.auto_pickup[it];
                        break;
                    }
                }
            }
        }
    }
    while (RUN);

    display_window_destroy(cwin, TRUE);
}

spell *display_spell_select(char *title, player *p)
{
    display_window *swin;
    guint width, height;
    guint startx, starty;
    guint maxvis;
    guint pos;
    int key; /* keyboard input */
    int RUN = TRUE;

    /* currently displayed spell; return value */
    spell *sp;

    /* offset to element position (when displaying more than maxvis items) */
    int offset = 0;

    /* currently selected item */
    guint curr = 1;

    /* curses attributes */
    int attrs;

    assert(p != NULL);

    if (!p->known_spells || !p->known_spells->len)
    {
        log_add_entry(p->log, "You don't know any spells.");
        return NULL;
    }

    /* buffer for spell code type ahead */
    char *code_buf = g_malloc0(sizeof(char) * 4);

    /* sort spell list  */
    g_ptr_array_sort(p->known_spells, &spell_sort);

    /* set height according to spell count */
    height = min((display_rows - 3), (p->known_spells->len + 2));
    maxvis = min(p->known_spells->len, height - 2);

    width = 44;
    starty = (display_rows - height) / 2;
    startx = (min(MAP_MAX_X, display_cols) - width) / 2;

    swin = display_window_new(startx, starty, width, height, title, NULL);

    do
    {
        /* display spells */
        for (pos = 1; pos <= maxvis; pos++)
        {
            sp = g_ptr_array_index(p->known_spells, pos + offset - 1);

            if (curr == pos) attrs = COLOR_PAIR(DCP_RED_WHITE);
            else attrs = COLOR_PAIR(DCP_WHITE_RED);

            wattron(swin->window, attrs);

            mvwprintw(swin->window, pos, 1, " %3s - %-23s (Lvl %d) %2d ",
                      spell_code(sp),
                      spell_name(sp),
                      spell_level(sp),
                      sp->knowledge);

            wattroff(swin->window, attrs);
        }

        /* display up / down markers */
        display_window_update_arrow_up(swin, (offset > 0));
        display_window_update_arrow_down(swin, ((offset + maxvis) < p->known_spells->len));

        /* display typeahead keys */
        gchar caption[39];
        g_snprintf(caption, 38, "%s%s%s%s(?) description",
                   (strlen(code_buf) ? "[" : ""),
                   code_buf,
                   (strlen(code_buf) ? "]" : ""),
                   (strlen(code_buf) ? " " : ""));

        swin->caption = caption;
        display_window_update_caption(swin);

        /* store currently highlighted spell */
        sp = g_ptr_array_index(p->known_spells, curr + offset - 1);

        switch ((key = getch()))
        {
        case '7':
        case KEY_HOME:
        case KEY_A1:

            curr = 1;
            offset = 0;
            code_buf[0] = '\0';

            break;

        case '9':
        case KEY_PPAGE:
        case KEY_A3:

            if ((curr == maxvis) || offset == 0)
                curr = 1;
            else
                offset = (offset > maxvis) ? (offset - maxvis) : 0;

            code_buf[0] = '\0';
            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif

            if (curr > 1)
                curr--;

            else if ((curr == 1) && (offset > 0))
                offset--;

            code_buf[0] = '\0';
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            if ((curr + offset) < p->known_spells->len)
            {
                if (curr == maxvis)
                    offset++;
                else
                    curr++;
            }

            code_buf[0] = '\0';
            break;

        case '3':
        case KEY_NPAGE:
        case KEY_C3:

            if (curr == 1)
            {
                curr = maxvis;
            }
            else
            {
                offset = offset + maxvis;

                if ((offset + maxvis) >= p->known_spells->len)
                {
                    curr = min(p->known_spells->len, maxvis);
                    offset = p->known_spells->len - curr;
                }
            }

            code_buf[0] = '\0';
            break;

        case '1':
        case KEY_END:
        case KEY_C1:

            if (p->known_spells->len > maxvis)
            {
                curr = maxvis;
                offset = p->known_spells->len - maxvis;
            }
            else
            {
                curr = p->known_spells->len;
            }

            code_buf[0] = '\0';
            break;

        case '?':
        case KEY_F(1):
            display_show_message(spell_name(sp), spell_desc(sp));

            /* repaint everything after displaying the message */
            display_paint_screen(p);
            display_draw();
            break;

        case 27: /* ESC */
            RUN = FALSE;
            sp = NULL;

            break;

        case 10:
        case 13:
        case KEY_ENTER:
        case 32: /* space bar */
            RUN = FALSE;
            break;

        case 127: /* backspace */
        case 263:
            if (strlen(code_buf))
            {
                code_buf[strlen(code_buf) - 1] = '\0';
            }
            else
            {
                if (!beep())
                    flash();
            }
            break;

        default:
            /* check if the key is used for window placement */
            if (display_window_move(swin, key))
            {
                break;
            }
            /* add key to spell code buffer */
            if ((key >= 'a') && (key <= 'z'))
            {
                if (strlen(code_buf) < 3)
                {
                    code_buf[strlen(code_buf)] = key;
                    /* search for match */

                    for (pos = 1; pos <= p->known_spells->len; pos++)
                    {
                        sp = g_ptr_array_index(p->known_spells, pos - 1);

                        if (g_str_has_prefix(spell_code(sp), code_buf))
                        {
                            /* match found - reposition selection */
                            if (pos > maxvis)
                            {
                                offset = pos - maxvis;
                                curr = pos - offset;
                            }
                            else
                            {
                                offset = 0;
                                curr = pos;
                            }

                            break;
                        }
                    }

                    /* if no match has been found remove key from buffer */
                    sp = g_ptr_array_index(p->known_spells, curr + offset - 1);
                    if (!g_str_has_prefix(spell_code(sp), code_buf))
                    {
                        code_buf[strlen(code_buf) - 1] = '\0';

                        if (!beep())
                            flash();
                    }

                }
                else
                {
                    if (!beep())
                        flash();
                }
            }

            break;
        }
    }
    while (RUN);

    g_free(code_buf);

    display_window_destroy(swin, TRUE);

    return sp;
}

int display_get_count(char *caption, int value)
{
    display_window *mwin;
    int height, width, basewidth;
    int startx, starty;

    GPtrArray *text;

    int tmp;

    /* curses attributes */
    int attrs;

    /* toggle insert / overwrite mode */
    int insert_mode = TRUE;

    /* user input */
    int key;

    /* input length */
    int ilen = 0;

    /* cursor position */
    int ipos = 0;

    /* input as char */
    char ivalue[8] = { 0 };

    /* continue editing the number */
    int cont = TRUE;

    /* 8: input field width; 5: 3 spaces between border, caption + input field, 2 border */
    basewidth = 8 + 5;

    /* choose a sane dialog width */
    width = min(basewidth + strlen(caption), display_cols - 4);

    text = text_wrap(caption, width - basewidth, 0);
    height = 2 + text->len;

    starty = (display_rows - height) / 2;
    startx = (display_cols - width) / 2;

    mwin = display_window_new(startx, starty, width, height, NULL, NULL);

    wattron(mwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));

    guint line;
    for (line = 0; line < text->len; line++)
    {
        /* fill the box background */
        mvwprintw(mwin->window, 1 + line, 1, "%-*s", width - 2, "");
        /* print text */
        mvwprintw(mwin->window, 1 + line, 2, g_ptr_array_index(text, line));
    }

    wattroff(mwin->window, attrs);

    /* prepare string to edit */
    g_snprintf(ivalue, 8, "%d", value);

    wattron(mwin->window, (attrs = COLOR_PAIR(DCP_BLACK_WHITE)));

    do
    {
        ilen = strlen(ivalue);

        /* make cursor visible and set according to insert mode */
        if (insert_mode)
            curs_set(1); /* underline */
        else
            curs_set(2); /* block */

        mvwprintw(mwin->window,  mwin->height - 2, mwin->width - 10,
                  "%-8s", ivalue);

        wmove(mwin->window, mwin->height - 2, mwin->width - 10 + ipos);
        wrefresh(mwin->window);

#ifdef PDCURSES
        key = wgetch(mwin->window);
#else
        key = getch();
#endif

        switch (key)
        {
        case KEY_LEFT:
            if (ipos > 0)
                ipos--;
            break;

        case KEY_RIGHT:
            if (ipos < ilen)
                ipos++;
            break;

        case KEY_BACKSPACE:
        case 8: /* backspace generates 8, not KEY_BACKSPACE on Windows */
            if ((ipos == ilen) && (ipos > 0))
            {
                ivalue[ipos - 1] = '\0';
                ipos--;
            }
            else if (ipos > 0)
            {
                for (tmp = ipos - 1; tmp < ilen; tmp++)
                    ivalue[tmp] = ivalue[tmp + 1];

                ipos--;
            }
            break;

        case KEY_IC:
            /* toggle insert mode */
            insert_mode = !insert_mode;
            break;

        case KEY_DC:
            if (ipos < ilen)
            {
                for (tmp = ipos; tmp < ilen; tmp++)
                    ivalue[tmp] = ivalue[tmp + 1];
            }
            break;

        case KEY_END:
            ipos = ilen;
            break;

        case KEY_HOME:
            ipos = 0;
            break;

        case 10: /* LF */
        case 13: /* CR */
        case 27: /* ESC */
        case KEY_ENTER: /* keypad enter */
            cont = FALSE;
            break;

        default:
            if ((key >= '0') && (key <= '9'))
            {
                if (insert_mode)
                {
                    /* insert */
                    if (strlen(ivalue) < 7)
                    {
                        int cppos = strlen(ivalue) + 1;

                        while (cppos >= ipos)
                        {
                            ivalue[cppos] = ivalue[cppos - 1];
                            cppos--;
                        }

                        ivalue[ipos] = key;

                        /* move position */
                        if (ipos < 7) ipos++;
                    }
                    else if (!beep()) flash();
                }
                else
                {
                    /* overwrite */
                    if (ipos < 8)
                    {
                        ivalue[ipos] = key;
                        /* move position */
                        if (ipos < 7) ipos++;
                    }

                    else if (!beep()) flash();
                }
            }
            else if (!display_window_move(mwin, key))
            {
                if (!beep())
                    flash();
            }

            break;
        }
    }
    while (cont);

    wattroff(mwin->window, attrs);

    /* hide cursor */
    curs_set(0);

    text_destroy(text);
    display_window_destroy(mwin, TRUE);

    if (key == 27)
        return 0;

    return atoi(ivalue);
}

char *display_get_string(char *caption, char *value, size_t max_len)
{
    /* user input */
    int key;

    /* curses attributes */
    int attrs;

    /* toggle insert / overwrite mode */
    int insert_mode = TRUE;

    /* cursor position */
    guint ipos = 0;

    /* text to be edited */
    GString *string = g_string_new(value);

    /* continue editing the number */
    int cont = TRUE;

    /* 3 spaces between border, caption + input field, 2 border */
    int basewidth = 5;

    /* choose a sane dialog width */
    guint width;
    guint maxwidth = display_cols - 4;

    /* prevent overly large input fields */
    if (max_len + basewidth > maxwidth)
    {
        max_len = maxwidth - basewidth;
    }

    if (basewidth + strlen(caption) + max_len > maxwidth)
    {
        if (strlen(caption) + basewidth > maxwidth)
        {
            width = maxwidth - basewidth;
        }
        else
        {
            width = basewidth + max(strlen(caption), max_len);
        }
    }
    else
    {
        /* input box fits on same line as caption */
        width = basewidth + strlen(caption) + max_len + 1;
    }

    GPtrArray *text = text_wrap(caption, width - basewidth, 0);

    /* determine if the input box fits on the last line */
    int box_start = 3 + strlen(g_ptr_array_index(text, text->len - 1));
    if (box_start + max_len + 2 > width)
        box_start = 2;

    int height = 2 + text->len; /* borders and text length */
    if (box_start == 2) height += 1; /* grow the dialog if input box doesn't fit */

    int starty = (display_rows - height) / 2;
    int startx = (display_cols - width) / 2;

    display_window *mwin = display_window_new(startx, starty, width, height, NULL, NULL);

    wattron(mwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
    guint line;

    for (line = 0; line < text->len; line++)
    {
        /* print text */
        mvwprintw(mwin->window, 1 + line, 1, " %-*s ", width - 4,
                  g_ptr_array_index(text, line));
    }

    wattroff(mwin->window, attrs);
    wattron(mwin->window, (attrs = COLOR_PAIR(DCP_BLACK_WHITE)));

    do
    {
        mvwprintw(mwin->window,  mwin->height - 2, box_start,
                  "%-*s", max_len + 1, string->str);
        wmove(mwin->window, mwin->height - 2, box_start + ipos);

        /* make cursor visible and set according to insert mode */
        if (insert_mode)
            curs_set(1); /* underline */
        else
            curs_set(2); /* block */

        wrefresh(mwin->window);

#ifdef PDCURSES
        key = wgetch(mwin->window);
#else
        key = getch();
#endif

        switch (key)
        {
        case KEY_LEFT:
            if (ipos > 0)
                ipos--;
            break;

        case KEY_RIGHT:
            if (ipos < string->len)
                ipos++;
            break;

        case KEY_BACKSPACE:
        case 8: /* backspace generates 8, not KEY_BACKSPACE on Windows */
            if (ipos > 0)
            {
                g_string_erase(string, ipos - 1, 1);
                ipos--;
            }
            break;

        case KEY_IC:
            /* toggle insert mode */
            insert_mode = !insert_mode;
            break;

        case KEY_DC:
            if (ipos < string->len)
            {
                g_string_erase(string, ipos, 1);
            }
            break;

        case KEY_END:
            ipos = string->len;
            break;

        case KEY_HOME:
            ipos = 0;
            break;

        case 10: /* LF */
        case 13: /* CR */
        case 27: /* ESC */
        case KEY_ENTER: /* keypad enter */
            cont = FALSE;
            break;

        default:
            /* handle window movement first */
            if (display_window_move(mwin, key))
                break;

            /* filter out unwanted keys */
            if (!g_ascii_isalnum(key) && !g_ascii_ispunct(key) && (key != ' '))
            {
                if (!beep()) flash();
                break;
            }

            if (insert_mode)
            {
                if (string->len < max_len)
                {
                    g_string_insert_c(string, ipos, key);
                    if (ipos < max_len) ipos++;
                }
                else
                {
                    if (!beep()) flash();
                }
            }
            else
            {
                if (ipos < string->len)
                {
                    string->str[ipos] = key;
                    if (ipos < max_len) ipos++;
                }
                else if (string->len < max_len)
                {
                    g_string_append_c(string, key);
                    ipos++;
                }
                else
                {
                    if (!beep()) flash();
                }
            }
            break;
        }
    }
    while (cont);

    wattroff(mwin->window, attrs);

    /* hide cursor */
    curs_set(0);

    text_destroy(text);
    display_window_destroy(mwin, TRUE);

    if (key == 27)
    {
        g_string_free(string, TRUE);
        return NULL;
    }

    return g_string_free(string, FALSE);
}

int display_get_yesno(char *question, char *yes, char *no)
{
    display_window *ywin;
    guint startx, starty;
    guint width, text_width;
    int RUN = TRUE;
    int selection = FALSE;
    guint line;
    int attrs; /* curses attributes */
    GPtrArray *text;
    int key; /* input key buffer */

    const guint padding = 1;
    const guint margin = 2;

    /* default values */
    if (!yes)
        yes = "Yes";

    if (!no)
        no = "No";

    /* determine text width, either defined by space available  for the window
     * or the length of question */
    text_width = min(display_cols - 2 /* borders */
                     - (2 * margin) /* space outside window */
                     - (2 * padding), /* space between border and text */
                     strlen(question));

    /* broad windows are hard to read */
    if (text_width > 60)
        text_width = 60;

    /* wrap question according to width */
    text = text_wrap(question, text_width + 1, 0);

    /* determine window width. either defined by the length of the button
     * labels or width of the text */
    width = max(strlen(yes) + strlen(no)
                + 2 /* borders */
                + (4 * padding)  /* space between "button" border and label */
                + margin, /* space between "buttons" */
                text_width + 2 /* borders */ + (2 * padding));

    /* set startx and starty to something that makes sense */
    startx = (min(MAP_MAX_X, display_cols) / 2) - (width / 2);
    starty = (display_rows / 2) - 4;

    ywin = display_window_new(startx, starty, width, text->len + 4, NULL, NULL);

    wattron(ywin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));

    for (line = 0; line < text->len; line++)
        mvwprintw(ywin->window,
                  line + 1,
                  1 + padding,
                  g_ptr_array_index(text, line));

    wattroff(ywin->window, attrs);

    text_destroy(text);

    do
    {
        /* paint */
        if (selection) wattron(ywin->window, (attrs = COLOR_PAIR(DCP_BLACK_WHITE) | A_BOLD));
        else           wattron(ywin->window, (attrs = COLOR_PAIR(DCP_RED_WHITE)));

        mvwprintw(ywin->window, line + 2, margin, "%*s%s%*s", padding, " ",
                  yes, padding, " ");

        wattroff(ywin->window, attrs);

        if (!selection) wattron(ywin->window, (attrs = COLOR_PAIR(DCP_BLACK_WHITE) | A_BOLD));
        else            wattron(ywin->window, (attrs = COLOR_PAIR(DCP_RED_WHITE)));

        mvwprintw(ywin->window, line + 2,
                  width - margin - strlen(no) - (2 * padding),
                  "%*s%s%*s", padding, " ", no, padding, " ");

        wattroff(ywin->window, attrs);
        wrefresh(ywin->window);

        /* wait for input */
        switch ((key = getch()))
        {
        case 27: /* ESC */
            selection = FALSE;
            /* fall through desired */

        case 10: /* LF */
        case 13: /* CR */
        case KEY_ENTER: /* keypad enter */
        case 32: /* space bar */
            RUN = FALSE;
            break;

        case 'h':
        case '4':
#ifdef KEY_B1
        case KEY_B1:
#endif
        case KEY_LEFT:
            if (!selection)
                selection = TRUE;
            break;

        case 'l':
        case '6':
#ifdef KEY_B3
        case KEY_B3:
#endif
        case KEY_RIGHT:
            if (selection)
                selection = FALSE;
            break;

            /* shortcuts */
        case 'y':
        case 'Y':
            selection = TRUE;
            RUN = FALSE;
            break;

        case 'n':
        case 'N':
            selection = FALSE;
            RUN = FALSE;
            break;

        default:
            /* perhaps the window shall be moved */
            display_window_move(ywin, key);
        }
    }
    while (RUN);

    display_window_destroy(ywin, TRUE);

    return selection;
}

direction display_get_direction(char *title, int *available)
{
    display_window *dwin;

    int *dirs = NULL;
    int startx, starty;
    int width;
    int x, y;
    int attrs; /* curses attributes */
    int key; /* input key buffer */
    int RUN = TRUE;

    /* direction to return */
    direction dir = GD_NONE;

    if (!available)
    {
        dirs = g_malloc0(sizeof(int) * GD_MAX);
        for (x = 0; x < GD_MAX; x++)
            dirs[x] = TRUE;

        dirs[GD_CURR] = FALSE;
    }
    else
    {
        dirs = available;
    }


    width = max(9, strlen(title) + 4);

    /* set startx and starty to something that makes sense */
    startx = (min(MAP_MAX_X, display_cols) / 2) - (width / 2);
    starty = (display_rows / 2) - 4;

    dwin = display_window_new(startx, starty, width, 9, title, NULL);

    wattron(dwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
    mvwprintw(dwin->window, 3, 3, "\\|/");
    mvwprintw(dwin->window, 4, 3, "- -");
    mvwprintw(dwin->window, 5, 3, "/|\\");
    wattroff(dwin->window, attrs);

    wattron(dwin->window, (attrs = COLOR_PAIR(DCP_YELLOW_RED)));

    for (x = 0; x < 3; x++)
        for (y = 0; y < 3; y++)
        {
            if (dirs[(x + 1) + (y * 3)])
                mvwprintw(dwin->window,
                          6 - (y * 2), /* start in the last row, move up, skip one */
                          (x * 2) + 2, /* start in the second col, skip one */
                          "%d",
                          (x + 1) + (y * 3));

        }

    wattroff(dwin->window, attrs);

    if (!available)
        g_free(dirs);

    wrefresh(dwin->window);

    do
    {
        switch ((key = getch()))
        {

        case 'h':
        case '4':
        case KEY_LEFT:
#ifdef KEY_B1
        case KEY_B1:
#endif
            if (dirs[GD_WEST]) dir = GD_WEST;
            break;

        case 'y':
        case '7':
        case KEY_HOME:
        case KEY_A1:
            if (dirs[GD_NW]) dir = GD_NW;
            break;

        case 'l':
        case '6':
        case KEY_RIGHT:
#ifdef KEY_B3
        case KEY_B3:
#endif
            if (dirs[GD_EAST]) dir = GD_EAST;
            break;

        case 'n':
        case '3':
        case KEY_NPAGE:
        case KEY_C3:
            if (dirs[GD_SE]) dir = GD_SE;
            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif
            if (dirs[GD_NORTH]) dir = GD_NORTH;
            break;

        case 'u':
        case '9':
        case KEY_PPAGE:
        case KEY_A3:
            if (dirs[GD_NE]) dir = GD_NE;
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            if (dirs[GD_SOUTH]) dir = GD_SOUTH;
            break;

        case 'b':
        case '1':
        case KEY_END:
        case KEY_C1:
            if (dirs[GD_SW]) dir = GD_SW;
            break;

        case '.':
        case '5':
            if (dirs[GD_CURR]) dir = GD_CURR;
            break;

        case 27:
            RUN = FALSE;
            break;

        default:
            /* perhaps the window shall be moved */
            display_window_move(dwin, key);
        }
    }
    while ((dir == GD_NONE) && RUN);

    if (!available)
    {
        g_free(dirs);
    }

    display_window_destroy(dwin, TRUE);

    return dir;
}

position display_get_position(player *p, char *message, int draw_line, int passable)
{
    int RUN  = TRUE;
    direction dir = GD_NONE;
    position pos, npos;
    map *map;

    /* curses attributes */
    int attrs;

    /* variables for ray painting */
    area *ray;
    int distance = 0;
    int x, y;
    monster *target, *m;

    /* start at player's position */
    pos = p->pos;

    /* hide windows */
    display_windows_hide();

    /* display message */
    log_add_entry(p->log, message);
    display_paint_screen(p);

    /* show cursor */
    curs_set(1);

    /* make shortcut to map */
    map = game_map(nlarn, p->pos.z);

    do
    {
        distance = max(abs(pos.x - p->pos.x),
                       abs(pos.y - p->pos.y));

        /* redraw screen to erase old rays */
        if (draw_line) display_paint_screen(p);

        /* draw a line between source and target if told to */
        if (draw_line && distance)
        {
            target = map_get_monster_at(map, pos);

            display_paint_screen(p);

            if (target)
                attrs = DC_LIGHTRED;
            else
                attrs = DC_LIGHTCYAN;

            attron(attrs);

            ray = area_new_ray(p->pos, pos,
                               map_get_obstacles(map, p->pos, distance));

            for (y = 0; y < ray->size_y; y++)
            {
                for (x = 0; x < ray->size_x; x++)
                {
                    if (area_point_get(ray, x, y))
                    {
                        position tpos = pos_new(ray->start_x + x, ray->start_y + y, p->pos.z);

                        if (target && pos_identical(monster_pos(target), tpos))
                        {
                            mvaddch(ray->start_y + y, ray->start_x + x, monster_image(target));
                        }
                        else if ((m = map_get_monster_at(map, tpos)))
                        {
                            mvaddch(ray->start_y + y, ray->start_x + x, monster_image(m));
                        }
                        else
                        {
                            mvaddch(ray->start_y + y, ray->start_x + x, '*');
                        }
                    }
                }
            }

            attroff(attrs);
            area_destroy(ray);
        }

        /* position cursor */
        move(pos.y, pos.x);

        /* wait for input */
        switch (getch())
        {
        case 27: /* ESC */
            pos.x = G_MAXINT16;
            pos.y = G_MAXINT16;
            /* fall through desired */

        case 10: /* LF */
        case 13: /* CR */
        case KEY_ENTER: /* keypad enter */
        case 32: /* space bar */
            RUN = FALSE;
            break;

            /* move cursor */
        case 'h':
        case '4':
        case KEY_LEFT:
#ifdef KEY_B1
        case KEY_B1:
#endif
            dir = GD_WEST;
            break;

        case 'y':
        case '7':
        case KEY_HOME:
        case KEY_A1:
            dir = GD_NW;
            break;

        case 'l':
        case '6':
        case KEY_RIGHT:
#ifdef KEY_B3
        case KEY_B3:
#endif
            dir = GD_EAST;
            break;

        case 'n':
        case '3':
        case KEY_NPAGE:
        case KEY_C3:
            dir = GD_SE;
            break;

        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif
            dir = GD_NORTH;
            break;

        case 'u':
        case '9':
        case KEY_PPAGE:
        case KEY_A3:
            dir = GD_NE;
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            dir = GD_SOUTH;
            break;

        case 'b':
        case '1':
        case KEY_END:
        case KEY_C1:
            dir = GD_SW;
            break;
        }

        if (dir)
        {
            npos = pos_move(pos, dir);
            dir = GD_NONE;
        }

        if (pos_valid(npos) && player_pos_visible(p, npos))
        {
            if (passable && !map_pos_passable(map, npos))
                /* a passable position has been requested and this one isn't */
                continue;

            /* new position is within bounds and visible */
            pos = npos;

            /* make npos invalid */
            npos.x = G_MAXINT16;
            npos.y = G_MAXINT16;
        }

    }
    while (RUN);

    /* hide cursor */
    curs_set(0);

    /* show windows */
    display_windows_show();

    return pos;
}

void display_show_history(message_log *log, char *title)
{
    guint idx;
    char *text = NULL;
    char *tmp = NULL;
    message_log_entry *le;

    for (idx = 0; idx < log_length(log); idx++)
    {
        le = log_get_entry(log, idx);

        if (!text)
        {
            text = g_strconcat(le->message, "\n", NULL);
        }
        else
        {
            tmp = g_strconcat(text, le->message, "\n", NULL);
            g_free(text);
            text = tmp;
        }
    }

    display_show_message(title, text);
    g_free(text);
}

/**
 * Simple "popup" message window
 * @param window title
 * @param message to be displayed inside window
 * @return key pressed to close window
 */
char display_show_message(char *title, char *message)
{
    guint height, width;
    guint startx, starty;
    display_window *mwin;
    int key;
    int attrs; /* curses attributes */

    GPtrArray *text;
    guint idx;
    guint maxvis = 0;
    guint offset = 0;

    gboolean RUN = TRUE;

    width = display_cols - 8;

    /* wrap message according to width */
    text = text_wrap(message, width - 4, 0);

    /* set height according to message line count */
    height = min((display_rows - 3), (text->len + 2));

    starty = (display_rows - height) / 2;
    startx = (display_cols - width) / 2;

    mwin = display_window_new(startx, starty, width, height, title, NULL);

    maxvis = min(text->len, height - 2);

    do
    {
        for (idx = 0; idx < maxvis; idx++)
        {
            wattron(mwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
            mvwprintw(mwin->window, idx + 1, 1, " %-*s ",
                      width - 4,
                      g_ptr_array_index(text, idx + offset));

            wattroff(mwin->window, attrs);
        }

        display_window_update_arrow_up(mwin, offset > 0);
        display_window_update_arrow_down(mwin, (offset + maxvis) < text->len);

        wrefresh(mwin->window);
        key = getch();

        switch (key)
        {
        case 'k':
        case '8':
        case KEY_UP:
#ifdef KEY_A2
        case KEY_A2:
#endif
            if (offset > 0)
                offset--;
            break;

        case '9':
        case KEY_PPAGE:
        case KEY_A3:
            if (offset > maxvis + 1)
                offset -= maxvis;
            else
                offset = 0;
            break;

        case '7':
        case KEY_HOME:
        case KEY_A1:
            offset = 0;
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
#ifdef KEY_C2
        case KEY_C2:
#endif
            if (text->len > (maxvis + offset))
                offset++;
            break;

        case '3':
        case KEY_NPAGE:
        case KEY_C3:
            offset = min((offset + maxvis - 1), text->len - maxvis);
            break;

        case '1':
        case KEY_END:
        case KEY_C1:
            offset = text->len - maxvis;
            break;

        default:
            /* perhaps the window shall be moved */
            if (!display_window_move(mwin, key))
            {
                /* some other key -> close window */
                RUN = FALSE;
            }
        }
    }
    while (RUN);

    display_window_destroy(mwin, TRUE);
    text_destroy(text);

    return key;
}

static display_window *display_window_new(int x1, int y1, int width, int height, char *title, char *caption)
{
    int i;
    display_window *dwin;
    int attrs; /* curses attributes */

    dwin = g_malloc0(sizeof(display_window));

    dwin->x1 = x1;
    dwin->y1 = y1;
    dwin->width = width;
    dwin->height = height;
    dwin->title = title;
    dwin->caption = caption;

    dwin->window = newwin(dwin->height, dwin->width, dwin->y1, dwin->x1);

#ifdef PDCURSES
    /* PDCurses does not inherit keypad setting form stdscr */
    dwin->window->_use_keypad = stdscr->_use_keypad;
#endif

    /* fill window background */
    wattron(dwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));

    for (i = 1; i < height; i++)
        mvwprintw(dwin->window, i, 1, "%*s", width - 2, "");

    wattroff(dwin->window, attrs);


    /* draw borders */
    wattron(dwin->window, (attrs = COLOR_PAIR(DCP_BLUE_RED)));
    box(dwin->window, 0, 0);
    wattroff(dwin->window, attrs);

    if (title && strlen(title))
    {
        wattron(dwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED) | A_BOLD));
        mvwprintw(dwin->window, 0, 1, " %s ", title);
        wattroff(dwin->window, attrs);
    }

    if (caption)
    {
        display_window_update_caption(dwin);
    }

    /* create a panel for the window */
    dwin->panel = new_panel(dwin->window);

    /* refresh panels */
    update_panels();

    /* add window to the list of opened windows */
    windows = g_list_append(windows, dwin);

    return dwin;
}

static void display_window_destroy(display_window *dwin, gboolean shall_clear)
{
    del_panel(dwin->panel);
    delwin(dwin->window);

    /* remove window from the list of windows */
    windows = g_list_remove(windows, dwin);

    g_free(dwin);

    /* repaint screen */
    if (shall_clear)
    {
        clear();
    }
}

static int display_window_move(display_window *dwin, int key)
{
    gboolean refresh = TRUE;

    assert (dwin != NULL);

    switch (key)
    {
    case 0:
        /* the windows keys generate two key presses, of which the first
           is a zero. flush the buffer or the second key code will confuse
           everything. This happens here as all dialogs call this function
           after everything else. */
        flushinp();
        break;
        /* ^left */
    case 541: /* NCurses - Linux */
    case 443: /* PDCurses - Windows */
        if (dwin->x1 > 0) dwin->x1--;
        break;

        /* ^right */
    case 556: /* NCurses - Linux */
    case 444: /* PDCurses - Windows */
        if (dwin->x1 < (display_cols - dwin->width)) dwin->x1++;
        break;

        /* ^up */
    case 562: /* NCurses - Linux */
    case 480: /* PDCurses - Windows */
        if (dwin->y1 > 0) dwin->y1--;
        break;

        /* ^down */
    case 521: /* NCurses - Linux */
    case 481: /* PDCurses - Windows */
        if (dwin->y1 < (display_rows - dwin->height)) dwin->y1++;
        break;

    default:
        refresh = FALSE;
    }

    if (refresh)
    {
        move_panel(dwin->panel, dwin->y1, dwin->x1);
        update_panels();
        doupdate();
    }

    return refresh;
}

static void display_window_update_caption(display_window *dwin)
{
    int attrs; /* curses attributes */

    assert (dwin != NULL && dwin->window != NULL);

    /* repaint line to overwrite previous captions */
    wattron(dwin->window, (attrs = COLOR_PAIR(DCP_BLUE_RED)));
    mvwhline(dwin->window, dwin->height - 1, 3, ACS_HLINE, dwin->width - 7);
    wattroff(dwin->window, attrs);

    /* print caption if caption is set */
    if (dwin->caption && strlen(dwin->caption))
    {
        wattron(dwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
        mvwprintw(dwin->window, dwin->height - 1, 3, " %s ", dwin->caption);
        wattroff(dwin->window, attrs);
    }

    wrefresh(dwin->window);
}

static void display_window_update_arrow_up(display_window *dwin, gboolean on)
{
    int attrs; /* curses attributes */

    assert (dwin != NULL && dwin->window != NULL);

    if (on)
    {
        wattron(dwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
        mvwprintw(dwin->window, 0, dwin->width - 5, " ^ ");
        wattroff(dwin->window, attrs);
    }
    else
    {
        wattron(dwin->window, (attrs = COLOR_PAIR(DCP_BLUE_RED)));
        mvwhline(dwin->window, 0, dwin->width - 5, ACS_HLINE, 3);
        wattroff(dwin->window, attrs);
    }
}

static void display_window_update_arrow_down(display_window *dwin, gboolean on)
{
    int attrs; /* curses attributes */

    assert (dwin != NULL && dwin->window != NULL);

    if (on)
    {
        wattron(dwin->window, (attrs = COLOR_PAIR(DCP_WHITE_RED)));
        mvwprintw(dwin->window, dwin->height - 1, dwin->width - 5, " v ");
        wattroff(dwin->window, attrs);
    }
    else
    {
        wattron(dwin->window, (attrs = COLOR_PAIR(DCP_BLUE_RED)));
        mvwhline(dwin->window, dwin->height - 1, dwin->width - 5, ACS_HLINE, 3);
        wattroff(dwin->window, attrs);
    }
}

static void display_windows_hide()
{
    GList *iterator = windows;

    while (iterator)
    {
        display_window *win = (display_window *)iterator->data;
        hide_panel(win->panel);
        iterator = iterator->next;
    }
}

static void display_windows_show()
{
    GList *iterator = windows;

    while (iterator)
    {
        display_window *win = (display_window *)iterator->data;
        show_panel(win->panel);
        iterator = iterator->next;
    }
}

static void display_spheres_paint(sphere *s, player *p)
{
    /* check if sphere is on current level */
    if (!(s->pos.z == p->pos.z))
        return;

    if (game_wizardmode(nlarn) || player_pos_visible(p, s->pos))
    {
        attron(DC_MAGENTA);
        mvaddch(s->pos.y, s->pos.x, '0');
        attroff(DC_MAGENTA);
    }
}
