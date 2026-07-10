/*
 * display.c
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

#include <ctype.h>
#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef SDLPDCURSES
# define PDC_WIDE
/* request the ncurses-compatible mouse interface */
# define PDC_NCMOUSE
# include "sdl2/pdcsdl.h"
# undef min
# undef max

static gchar *font_name;

const int DEFAULT_ROWS = 25;
const int DEFAULT_COLS = 90;
#endif

#include <glib/gi18n.h>

#include "colours.h"
#include "config.h"
#include "display.h"
#include "fov.h"
#include "map.h"
#include "extdefs.h"
#include "spheres.h"

static bool display_initialised = false;

/* linked list of opened windows */
static GList *windows = NULL;

/* The mouse event belonging to the last KEY_MOUSE key code returned
   by display_getch(). Retrieving the event right away keeps the mouse
   event queue in sync with the key codes even when a KEY_MOUSE key is
   discarded by an input loop that does not care for the mouse. */
static MEVENT display_mouse_event;

static attr_t mvwcprintw(WINDOW *win, attr_t defattr, attr_t currattr,
                         colour_t bg, int y, int x, const char *fmt, ...);

static void display_inventory_help(GPtrArray *callbacks);

static display_window *display_window_new(int x1, int y1, int width,
        int height, const char *title);

static void display_window_update_title(display_window *dwin, const char *title);
static void display_window_update_caption(display_window *dwin, char *caption);
static void display_window_update_arrow_up(display_window *dwin, bool on);
static void display_window_update_arrow_down(display_window *dwin, bool on);
static int display_window_arrow_at(display_window *dwin, int x, int y);
static int display_scroll_getch(display_window *dwin, int *autoscroll);

static display_window *display_item_details(guint x1, guint y1, guint width,
                                            item *it, player *p, bool shop);

static void display_spheres_paint(sphere *s, player *p);

const int DISPLAY_WINDOW_MAX_WIDTH = 78;

void display_init()
{
#ifdef NCURSES_VERSION
    /*
     * Don't wait for trailing key codes after an ESC key is pressed.
     * This breaks compatibility with very old terminals connected over
     * very slow serial lines - I think I can live with that.
     */
    set_escdelay(0);
#endif

#ifdef SDLPDCURSES
    /* Set the window icon */
    char *icon_name = g_strdup_printf("%s/nlarn-128.bmp", nlarn_libdir);
    g_setenv("PDC_ICON", icon_name, 1);
    g_free(icon_name);

    /* If a font size was defined, export it to the environment
     * before initialising PDCurses. */
    if (config.font_size)
    {
        gchar size[4];
        g_snprintf(size, 3, "%d", config.font_size);
        g_setenv("PDC_FONT_SIZE", size, true);
    }

    /* Set the font - allow overriding this default */
    font_name = g_strdup_printf("%s/FiraMono-Medium.otf", nlarn_libdir);
    g_setenv("PDC_FONT", font_name, 0);
#endif

    /* Start curses mode */
    initscr();

#ifdef SDLPDCURSES
    /* These initialisations have to be done after initscr(), otherwise
       the window is not yet available. */
    /* Set the window title */
    char *window_title = g_strdup_printf("NLarn %s", nlarn_version);

    PDC_set_title(window_title);
    g_free(window_title);

    /* default to 90 columns when using SDL PDCurses */
    resize_term(DEFAULT_ROWS, DEFAULT_COLS);

    display_toggle_fullscreen(false);

    /* return modifier keys pressed with key */
    PDC_return_key_modifiers(true);
#endif

    /* initialize colours */
    start_color();  /* Curses */
    colours_init(config.colour_scheme); /* We */

    /* control special keys in application */
    raw();

    /* suppress input echo */
    noecho();

    /* enable function keys */
    keypad(stdscr, true);

    /* want all 8 bits */
    meta(stdscr, true);

    /* Enable mouse support: window dragging needs the left button's
       press and release events plus the pointer position while the
       button is held. PDCurses reports the latter as BUTTONn_MOVED
       events, ncurses as REPORT_MOUSE_POSITION. The mouse wheel is
       reported as BUTTON4 (up) and BUTTON5 (down) by both. */
#ifdef NCURSES_VERSION
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED
            | BUTTON4_PRESSED | BUTTON5_PRESSED
            | REPORT_MOUSE_POSITION, NULL);
#else
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED
            | BUTTON4_PRESSED | BUTTON5_PRESSED
            | BUTTON1_MOVED, NULL);
#endif

    /* Report press and release events separately and immediately
       instead of merging them into click events after a delay. */
    mouseinterval(0);

    /* make cursor invisible */
    curs_set(0);

    /* update display initialisation status */
    display_initialised = true;
}

/* convenience helper against endless repetition */
static int waddwach(WINDOW *win, const wchar_t ch, short color_pair, attr_t attrs)
{
    cchar_t cchar = {0};
    /* transform the given character into a zero-terminated string */
    const wchar_t chs[] = { ch, 0 };

    setcchar(&cchar, chs, attrs, color_pair, NULL);
    return wadd_wch(win, &cchar);
}

#define addwach(ch, color_pair, attrs) waddwach(stdscr, ch, color_pair, attrs)

/* mvaddch for wide characters with an additional attribute parameter */
static int mvaddwach(int y, int x, attr_t attrs, wchar_t ch)
{
    move(y, x);
    return waddwach(stdscr, ch, PAIR_NUMBER(attrs), attrs & ~A_COLOR);
}

/* printw with an additional attribute parameter */
#define aprintw(attrs, fmt, ...) \
    attron(attrs); \
    printw(fmt, ##__VA_ARGS__); \
    attroff(attrs)

/* mvwprintw with an additional attribute parameter */
#define mvwaprintw(win, y, x, attrs, fmt, ...) \
    wattron(win, attrs); \
    mvwprintw(win, y, x, fmt, ##__VA_ARGS__); \
    wattroff(win, attrs)

/* mvprintw with an additional attribute parameter */
#define mvaprintw(y, x, attrs, fmt, ...) \
    mvwaprintw(stdscr, y, x, attrs, fmt, ##__VA_ARGS__)

attr_t display_player_hp_colour(const player *p, bool status)
{
    attr_t attrs = 0;
    if (p->hp <= ((int)p->hp_max / 10))      /* 10% hp left */
        attrs = COLOR_PAIR(LUMINOUS_RED) | A_BLINK;
    else if (p->hp <= ((int)p->hp_max / 4))  /* 25% hp left */
        attrs = COLOR_PAIR(STRAWBERRY_RED);
    else if (p->hp <= ((int)p->hp_max / 2))  /* 50% hp left */
        attrs = COLOR_PAIR(BLOOD_RED);
    else
        attrs = COLOR_PAIR(status ? LIGHT_BRIGHT_GREEN : WHEAT);

#ifdef SDLPDCURSES
    /* enable blinking on SDL PDCurses display for very low hp */
    if (attrs & A_BLINK) {
        PDC_set_blink(true);
    } else {
        PDC_set_blink(false);
    }
#endif

    return attrs;
}

void display_paint_screen(player *p)
{
    position pos = pos_invalid;
    attr_t attrs;              /* curses attributes */

    /* draw line around map */
    (void)mvhline(MAP_MAX_Y, 0, ACS_HLINE, MAP_MAX_X);
    (void)mvvline(0, MAP_MAX_X, ACS_VLINE, MAP_MAX_Y);
    (void)mvaddch(MAP_MAX_Y, MAP_MAX_X, ACS_LRCORNER);

    /* make shortcut to the visible map */
    map *vmap = game_map(nlarn, Z(p->pos));

    /* draw map */
    Z(pos) = Z(p->pos);
    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        /* position cursor */
        move(Y(pos), 0);

        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            if (game_fullvis(nlarn) || fov_get(p->fv, pos))
            {
                /* draw the truth */
                display_cell dc = map_get_tile(vmap, pos);
                addwach(dc.glyph, dc.colour, dc.reversed ? WA_REVERSE : 0);
            }
            else /* i.e. !fullvis && !visible: draw players memory */
            {
                const bool has_items = player_memory_of(p, pos).item;
                if (player_memory_of(p, pos).sobject)
                {
                    /* draw stationary object */
                    sobject_t ms = map_sobject_at(vmap, pos);

                    wchar_t glyph;
                    if (ms == LS_CLOSEDDOOR || ms == LS_OPENDOOR)
                        glyph = map_get_door_glyph(vmap, pos);
                    else
                        glyph = so_get_glyph(ms);

                    addwach(glyph, so_get_colour(ms), has_items ? WA_REVERSE : 0);
                }
                else if (has_items)
                {
                    /* draw items */
                    const bool has_trap = (player_memory_of(p, pos).trap);

                    addwach(item_glyph(
                        player_memory_of(p, pos).item),
                        player_memory_of(p, pos).item_colour,
                        has_trap ? WA_REVERSE : 0
                    );
                }
                else if (player_memory_of(p, pos).trap)
                {
                    /* draw trap */
                    addwach('^', trap_colour(map_trap_at(vmap, pos)), 0);
                }
                else
                {
                    /* draw tile */
                    addwach(mt_get_glyph(player_memory_of(p, pos).type), FUSCOUS_GREY, 0);
                }
            }

            /* draw monsters */
            monster *monst = map_get_monster_at(vmap, pos);

            if (monst == NULL)
            {
                /* no monster found */
                continue;
            }

            if (game_fullvis(nlarn)
                    || player_effect(p, ET_DETECT_MONSTER)
                    || monster_in_sight(monst))
            {
                position mpos = monster_pos(monst);
                mvaddwach(Y(mpos), X(mpos), COLOR_PAIR(monster_color(monst)), monster_glyph(monst));
            }
        }
    }

    /* draw spheres */
    g_ptr_array_foreach(nlarn->spheres, (GFunc)display_spheres_paint, p);

    /* draw player */
    wchar_t pc;
    attr_t player_attributes = display_player_hp_colour(p, false);

    if (player_effect(p, ET_INVISIBILITY))
    {
        pc = ' ';
        attrs = A_REVERSE | player_attributes;
    }
    else
    {
        pc = '@';
        attrs = player_attributes;
    }

    mvaddwach(Y(p->pos), X(p->pos), attrs, pc);


    /* *** first status line below map *** */
    move(MAP_MAX_Y + 1, 0);
    clrtoeol();

    /* player name */
    if (p->name)
    {
        /* the player's name can be NULL directly after starting the game */
        printw("%s", p->name);
    }

    /* current HPs */
    mvaprintw(MAP_MAX_Y + 1, MAP_MAX_X - 21, display_player_hp_colour(p, true),
            "HP %3d", p->hp);

    /* max HPs */
    mvaprintw(MAP_MAX_Y + 1, MAP_MAX_X - 15,
        COLOR_PAIR(LIGHT_BRIGHT_GREEN), "/%-3d", player_get_hp_max(p));

    /* current MPs */
    if (p->mp <= ((int)p->mp_max / 10)) /* 10% mp left */
        attrs = COLOR_PAIR(AQUA_BLUE);
    else if (p->mp <= ((int)p->mp_max / 4))  /* 25% mp left */
        attrs = COLOR_PAIR(CERULEAN);
    else if (p->mp <= ((int)p->mp_max / 2))  /* 50% mp left */
        attrs = COLOR_PAIR(WATER_BLUE);
    else
        attrs = COLOR_PAIR(AZUL);

    mvaprintw(MAP_MAX_Y + 1, MAP_MAX_X - 10, attrs, "MP %3d", p->mp);

    /* max MPs */
    mvaprintw(MAP_MAX_Y + 1, MAP_MAX_X - 4,
        COLOR_PAIR(AZUL), "/%-3d", player_get_mp_max(p));

    /* game time */
    mvprintw(MAP_MAX_Y + 1, MAP_MAX_X + 1, "T %-6d", game_turn(nlarn));

    /* *** second status line below map *** */
    move(MAP_MAX_Y + 2, 0);
    clrtoeol();

    /* player level description */
    char *pld = g_strdup(player_get_level_desc(p));
    pld[0] = g_ascii_toupper(pld[0]);
    printw("%s", pld);
    g_free(pld);

    /* experience points / level */
    mvaprintw(MAP_MAX_Y + 2, MAP_MAX_X - 21, COLOR_PAIR(LED_BLUE),
        "XP %3d/%-5d", p->level, p->experience);

    /* dungeon map */
    mvprintw(MAP_MAX_Y + 2, MAP_MAX_X + 1, "Lvl: %s", map_name(vmap));


    /* *** RIGHT STATUS *** */

    /* strength */
    mvprintw(1, MAP_MAX_X + 3, "STR ");

    if (player_get_str(p) > (int)p->strength)
        attrs = COLOR_PAIR(GREEN_HAZE);
    else if (player_get_str(p) < (int)p->strength)
        attrs = COLOR_PAIR(STRAWBERRY_RED);
    else
        attrs = COLOR_PAIR(WHITE);

    aprintw(attrs, "%2d", player_get_str(p));
    clrtoeol();

    /* dexterity */
    mvprintw(2, MAP_MAX_X + 3, "DEX ");

    if (player_get_dex(p) > (int)p->dexterity)
        attrs = COLOR_PAIR(GREEN_HAZE);
    else if (player_get_dex(p) < (int)p->dexterity)
        attrs = COLOR_PAIR(STRAWBERRY_RED);
    else
        attrs = COLOR_PAIR(WHITE);

    aprintw(attrs, "%2d", player_get_dex(p));
    clrtoeol();

    /* constitution */
    mvprintw(3, MAP_MAX_X + 3, "CON ");

    if (player_get_con(p) > (int)p->constitution)
        attrs = COLOR_PAIR(GREEN_HAZE);
    else if (player_get_con(p) < (int)p->constitution)
        attrs = COLOR_PAIR(STRAWBERRY_RED);
    else
        attrs = COLOR_PAIR(WHITE);

    aprintw(attrs, "%2d", player_get_con(p));
    clrtoeol();

    /* intelligence */
    mvprintw(4, MAP_MAX_X + 3, "INT ");

    if (player_get_int(p) > (int)p->intelligence)
        attrs = COLOR_PAIR(GREEN_HAZE);
    else if (player_get_int(p) < (int)p->intelligence)
        attrs = COLOR_PAIR(STRAWBERRY_RED);
    else
        attrs = COLOR_PAIR(WHITE);

    aprintw(attrs, "%2d", player_get_int(p));
    clrtoeol();

    /* wisdom */
    mvprintw(5, MAP_MAX_X + 3, "WIS ");

    if (player_get_wis(p) > (int)p->wisdom)
        attrs = COLOR_PAIR(GREEN_HAZE);
    else if (player_get_wis(p) < (int)p->wisdom)
        attrs = COLOR_PAIR(STRAWBERRY_RED);
    else
        attrs = COLOR_PAIR(WHITE);

    aprintw(attrs, "%2d", player_get_wis(p));
    clrtoeol();

    /* clear line below Wisdom */
    move(6, MAP_MAX_X + 1);
    clrtoeol();

    /* wielded weapon */
    if (p->eq_weapon)
    {
        char *wdesc = weapon_shortdesc(p->eq_weapon, COLS - MAP_MAX_X - 4);
        mvprintw(7, MAP_MAX_X + 3, "%s", wdesc);
        g_free(wdesc);
    }
    else
    {
        mvaprintw(7, MAP_MAX_X + 3, COLOR_PAIR(LUMINOUS_RED), "%s", _("unarmed"));
    }
    clrtoeol();

    /* armour class */
    mvprintw(8, MAP_MAX_X + 3, "AC: %2d", player_get_ac(p));
    clrtoeol();

    /* gold */
    mvaprintw(9, MAP_MAX_X + 3, COLOR_PAIR(PIRATE_GOLD), "$%-7d", player_get_gold(p));
    clrtoeol();

    /* clear line below gold */
    move(10, MAP_MAX_X + 1);
    clrtoeol();

    /* clear lines */
    for (guint i = 0; i < 7; i++)
    {
        move(11 + i, MAP_MAX_X + 3);
        clrtoeol();
    }

    /* display effect descriptions */
    if (p->effects->len > 0)
    {
        const guint available_space = COLS - MAP_MAX_X - 4;
        char **efdescs = strv_new();

        /* collect effect descriptions */
        for (guint i = 0; i < p->effects->len; i++)
        {
            effect *e = game_effect_get(nlarn, g_ptr_array_index(p->effects, i));

            if (effect_get_desc(e) == NULL)
                continue;

            char *desc = g_strdup(effect_get_desc(e));

            if (g_utf8_strlen(desc, -1) > (glong)available_space)
            {
                /* truncate at a character boundary, not mid-sequence */
                char *cut = g_utf8_offset_to_pointer(desc, available_space - 1);
                cut[0] = '.';
                cut[1] = '\0';
            }

            if ((e->type == ET_WALL_WALK || e->type == ET_LEVITATION)
                    && e->turns < 6)
            {
                /* fading effects */
                gchar *cdesc = g_strdup_printf("`LUMINOUS_RED`%s`end`", desc);
                strv_append_unique(&efdescs, cdesc);
                g_free(cdesc);
            }
            else if (e->type > ET_LEVITATION)
            {
                /* negative effects */
                gchar *cdesc = g_strdup_printf("`LIGHT_MAGENTA`%s`end`", desc);
                strv_append_unique(&efdescs, cdesc);
                g_free(cdesc);
            }
            else
            {
                /* other effects */
                strv_append_unique(&efdescs, desc);
            }

            g_free(desc);
        }

        /* display effect descriptions */
        attr_t currattr = COLOURLESS;
        for (guint i = 0; i < g_strv_length(efdescs); i++)
        {
            currattr = mvwcprintw(stdscr, COLOR_PAIR(CORNFLOWER_BLUE),
                currattr, PF_BG, 11 + i, MAP_MAX_X + 3, efdescs[i]);
        }

        g_strfreev(efdescs);
    }

    /* *** MESSAGES *** */
    /* number of lines which can be displayed */
    guint y = LINES > 20 ? LINES - 20 : 0;

    /* storage for the game time of messages */
    guint ttime[y];

    /* hold original length of text */
    guint x = 1;

    /* line counter */
    guint i = 0;

    /* storage for formatted messages */
    GPtrArray *text = NULL;

    /* if log contains buffered messaged, display them */
    if (log_buffer(nlarn->log))
    {
        text = text_wrap(log_buffer(nlarn->log), COLS, 2);
        for (x = 1; x <= (unsigned)min(text->len, y); x++)
            ttime[x - 1] = game_turn(nlarn);
    }

    /* retrieve game log and reformat messages to window width */
    while (((text == NULL) || (text->len < y)) && (log_length(nlarn->log) > i))
    {
        message_log_entry *le = log_get_entry(nlarn->log,
                                              log_length(nlarn->log) - 1 - i);

        if (text == NULL)
            text = text_wrap(le->message, COLS, 2);
        else
            text = text_append(text, text_wrap(le->message, COLS, 2));

        /* store game time for associated text line */
        while ((x <= text->len) && (x <= y))
        {
            ttime[x - 1] = le->gtime;
            x++;
        }

        i++;
    }

    /* ensure consistent colours for messages spanning multiple lines */
    attr_t currattr = COLOURLESS;
    for (y = 20, i = 0; (y < (unsigned)LINES) && (i < text->len); i++, y++)
    {
        /* default colour for the line */
        attr_t def_attrs = (i == 0 && ttime[i] > game_turn(nlarn) - 5)
            ? COLOR_PAIR(WHITE)
            : COLOR_PAIR(OSLO_GREY);

        /* reset current color when switching log entries */
        if (i > 0 && ttime[i - 1] != ttime[i])
            currattr = COLOURLESS;

        currattr = mvwcprintw(stdscr, def_attrs, currattr,
            BLACK, y, 0, g_ptr_array_index(text, i));

        clrtoeol();
    }

    text_destroy(text);

    display_draw();
}

void display_shutdown()
{
#ifdef SDLPDCURSES
    g_free(font_name);
    font_name = NULL;
#endif

    /* only terminate curses mode when the display has been initialised */
    if (display_initialised)
    {
        /* end curses mode */
        endwin();

        /* update display initialisation status */
        display_initialised = false;
    }
}

bool display_available()
{
    return display_initialised;
}

void display_draw()
{
#ifdef PDCURSES
    /* I have no idea why, but panels are not redrawn when
     * using PDCurses without calling touchwin for it. */
    GList *iterator = windows;
    while (iterator) {
        display_window *win = (display_window *)iterator->data;
        touchwin(win->window);
        iterator = iterator->next;
    }
#endif

    /* mark stdscr and all panels for redraw */
    update_panels();

    /* finally commit all the prepared updates */
    doupdate();
}

void display_paint_glyph(position pos, wchar_t glyph, colour_t fg)
{
    move(Y(pos), X(pos));
    (void)waddwach(stdscr, glyph, fg, 0);
}

void display_nap(guint ms)
{
    napms(ms);
}

void display_animate_glyph(position pos, wchar_t glyph, colour_t fg, bool keep)
{
    display_paint_glyph(pos, glyph, fg);
    display_draw();

    /* sleep a while to show the glyph's position */
    napms(100);

    /* repaint the screen unless requested otherwise */
    if (!keep)
        display_paint_screen(nlarn->p);
}

void display_flash_monsters(player *p, GList *monsters)
{
    if (monsters == NULL)
        return;

    /* blink the given monsters a few times to draw the player's
       attention to what interrupted the automatic movement */
    for (int blink = 0; blink < 3; blink++)
    {
        /* highlight the monsters' cells */
        for (GList *iter = monsters; iter != NULL; iter = iter->next)
        {
            position mpos = monster_pos((monster *)iter->data);
            mvwchgat(stdscr, Y(mpos), X(mpos), 1,
                     A_REVERSE | A_BOLD, LUMINOUS_RED, NULL);
        }
        display_draw();
        napms(90);

        /* restore the normal screen */
        display_paint_screen(p);
        napms(90);
    }
}

static int item_sort_normal(gconstpointer a, gconstpointer b, gpointer data)
{
    return item_sort(a, b, data, false);
}

static int item_sort_shop(gconstpointer a, gconstpointer b, gpointer data)
{
    return item_sort(a, b, data, true);
}

/* ---------------------------------------------------------------------------
 * Scroll state
 *
 * Tracks all position bookkeeping for a scrollable list widget. Both
 * display_inventory (with category headers) and display_spell_select
 * (without headers) share these helpers.
 *
 * Fields
 * ------
 *   curr          1-based index of the selected item inside the visible
 *                 window (1 … max_item_vis).
 *   offset        number of list items above the visible window.
 *   maxvis        total visible lines: item rows + header rows.
 *   max_item_vis  visible item rows only (= maxvis − shown header rows).
 *
 * For lists without headers maxvis == max_item_vis at all times.
 * --------------------------------------------------------------------------- */

typedef struct {
    guint curr;
    guint offset;
    guint maxvis;        // lines in window: items + headers
    guint max_item_vis;  // lines in window: items only
    guint len;           // number of total elements in list
} list_scroll_state;

/* Count category-header rows that appear when the visible window starts at
 * item index 'from' and has room for 'capacity' combined lines.
 * Headers are only emitted at category boundaries inside the visible range;
 * we prime last_type with the item just above the window so that a category
 * that started above the scroll position does not produce a spurious header.
 *
 * When suppress_first is true the header that would appear on the very first
 * visible line is not counted. This models the special case where the whole
 * list's items fit but its headers do not. */
static guint count_headers_in_range(inventory **inv, int (*ifilter)(item *),
                                    guint from, guint capacity, bool suppress_first)
{
    if (inv == NULL)
        return 0;

    item_t last_type = IT_NONE;
    if (from > 0) {
        item *prev = inv_get_filtered(*inv, from - 1, ifilter);
        last_type = prev->type;
    }

    guint headers = 0;
    guint len = inv_length_filtered(*inv, ifilter);
    for (guint idx = from; idx < len && (idx - from) + headers < capacity; idx++)
    {
        item *it = inv_get_filtered(*inv, idx, ifilter);
        if (it->type != last_type) {
            last_type = it->type;
            /* Skip counting the header on the first visible line when
             * suppression is requested. */
            if (idx == from && suppress_first)
                continue;
            headers++;
        }
    }
    return headers;
}

/* Decide whether the first visible category header should be suppressed at a
 * given offset to fit all the items in the window.
 *
 * Suppression applies when the remaining items plus their headers overflow the
 * window, but dropping the single first header brings them within the window.
 */
static bool lss_suppress_at(inventory **inv, int (*ifilter)(item *),
                            guint offset, guint len, guint target)
{
    if (inv == NULL || len == 0 || offset >= len)
        return false;

    guint items_below = len - offset;

    /* True total header count for the tail. The capacity passed to
     * count_headers_in_range bounds *combined* lines (items + headers), so it
     * must be large enough to never truncate a header: there can be at most
     * one header per item, so items_below*2 is a safe bound. */
    guint full_hdrs = count_headers_in_range(inv, ifilter, offset,
                                             items_below * 2, false);

    /* Tail does not fit with all headers, but fits with one fewer. */
    return (items_below + full_hdrs > target)
        && (items_below + (full_hdrs > 0 ? full_hdrs - 1 : 0) <= target);
}

/* Convenience wrapper for the current scroll state. */
static bool lss_suppress_first_header(list_scroll_state *s, inventory **inv,
                                      int (*ifilter)(item *), guint target)
{
    return lss_suppress_at(inv, ifilter, s->offset, s->len, target);
}

/* Find the offset that shows the last item while filling the window as much
 * as possible.
 *
 * Returns 0 when the whole list fits from the top. */
static guint find_last_page_offset(list_scroll_state *s, inventory **inv,
                                   int (*ifilter)(item *))
{
    const guint max_height = LINES - 10;
    const guint target = max_height - 2;

    /* If the entire list fits within the window from the top, offset=0 */
    guint hdrs_at_zero = count_headers_in_range(inv, ifilter, 0, target, false);
    if (s->len + hdrs_at_zero <= target)
    {
        return 0;
    }

    /* Scan all offsets, tracking the one whose (possibly suppressed) tail
     * fills the window most without overflowing. */
    guint best_offset = s->len > 0 ? s->len - 1 : 0;
    guint best_lines  = 0;

    for (guint off = 0; off < s->len; off++)
    {
        guint items_below = s->len - off;
        bool  sup = lss_suppress_at(inv, ifilter, off, s->len, target);
        guint hdrs = count_headers_in_range(inv, ifilter, off, target, sup);
        guint lines_used = items_below + hdrs;

        if (lines_used <= target && lines_used > best_lines)
        {
            best_lines  = lines_used;
            best_offset = off;

            if (lines_used == target)
                break;
        }
    }

    return best_offset;
}

/* --- State initialisation --- */

/* Initialise scroll state for a list that fits entirely in the window. */
static void lss_init(list_scroll_state *s, guint len, guint window_lines)
{
    guint vis = min(len, window_lines);
    s->curr         = (len > 0) ? 1 : 0;
    s->offset       = 0;
    s->maxvis       = vis;
    s->max_item_vis = vis;
    s->len          = len;
}

/* Recompute the derived fields after the window height or scroll position
 * has changed. Call once per render loop iteration for inventory lists. */
static void lss_recalc(list_scroll_state *s, inventory **inv,
                       int (*ifilter)(item *), guint len, guint max_height)
{
    /* check for data size change */
    if (s->len > len && (s->offset + s->maxvis > len))
    {
        /* number of entries is smaller than before: update len (and maxvis,
         * which depends on len) *before* calling find_last_page_offset, so
         * that it computes the offset for the new, smaller list. */
        s->len = len;

        guint vis_hdrs = count_headers_in_range(inv, ifilter,
            s->offset, max_height - 2, false);
        guint height = min((guint)max_height, len + vis_hdrs + 2);
        s->maxvis = min(len + vis_hdrs, height - 2);

        /* if on the last page, recalculate offset */
        guint old_offset = s->offset;
        s->offset = find_last_page_offset(s, inv, ifilter);

        /* Keep the selection on the same item: if offset moved up by N,
         * curr must move down by N (and vice versa) to compensate. Using
         * signed arithmetic and clamping to [1, len] handles both
         * directions and protects against curr underflowing to 0 or
         * exceeding the list when the previously selected item itself was
         * the one removed. */
        gint new_curr = (gint)s->curr + ((gint)old_offset - (gint)s->offset);
        if (new_curr < 1)
            new_curr = 1;
        if ((guint)new_curr > len)
            new_curr = (gint)len;

        s->curr = (guint)new_curr;
    }

    s->len = len;

    /* Determine whether the first category header must be dropped so that all
     * items fit (only relevant at offset 0). */
    const guint target = max_height - 2;
    bool suppress_first = lss_suppress_first_header(s, inv, ifilter, target);

    guint vis_hdrs = count_headers_in_range(inv, ifilter,
        s->offset, max_height - 2, suppress_first);

    guint height = min((guint)max_height,len + vis_hdrs + 2);

    s->maxvis = min(len + vis_hdrs, height - 2);

    /* Invariant: maxvis must not exceed the number of lines actually needed
     * to display the items below the current offset plus their headers.
     * maxvis is derived from the *total* list size, but when offset > 0 fewer
     * items remain below it. Clamping maxvis (rather than pulling offset back)
     * keeps the final items reachable even in heavily fragmented lists where
     * the whole list never fits the window at once. */
    {
        guint items_below = len - s->offset;
        guint lines_needed = items_below + vis_hdrs;
        if (s->maxvis > lines_needed)
            s->maxvis = lines_needed;
    }

    /* max_item_vis is updated after the render loop once shown_headers is
     * known; this pre-render value is used for navigation decisions. */
    s->max_item_vis = s->maxvis - vis_hdrs;

    if (s->curr > len)
        s->curr = len;

    if (s->curr > s->max_item_vis)
        s->curr = s->max_item_vis;
}

/* --- Navigation operations --- */

/* Move selection to the first item. */
static void lss_home(list_scroll_state *s)
{
    s->curr   = 1;
    s->offset = 0;
}

/* Move selection to the last item. */
static void lss_end(list_scroll_state *s,
                    inventory **inv, int (*ifilter)(item *))
{
    if (inv != NULL) {
        s->offset = find_last_page_offset(s, inv, ifilter);
    } else {
        s->offset = (s->len > s->max_item_vis) ? s->len - s->max_item_vis : 0;
    }

    s->curr = s->len - s->offset;
}

/* Move selection one item up. */
static void lss_up(list_scroll_state *s)
{
    if (s->curr > 1)
        s->curr--;
    else if (s->offset > 0)
        s->offset--;
}

/* Move selection one item down.
 * For inventory lists pass inv and ifilter so the function can detect
 * a newly visible category header and scroll one extra line to keep the
 * selected item inside the window.  Pass inv == NULL for header-free lists. */
static void lss_down(list_scroll_state *s, inventory **inv,
                     int (*ifilter)(item *), guint visible_category_headers)
{
    if (s->curr + s->offset >= s->len)
        /* already at last item */
        return;

    if (s->curr == s->max_item_vis) {
        s->offset++;

        /* If the newly scrolled-in item is the first of a new
         * category a header row will appear above it. */
        if (inv != NULL) {
            guint hdrs = count_headers_in_range(inv, ifilter, s->offset, s->maxvis, false);

            if (hdrs > visible_category_headers) {
                s->offset++;
                s->curr--;
            }
        }

        /* If we have just reached the bottom of the list, the incremental
         * offset++ may have advanced one step further than necessary,
         * leaving the window underfilled (the last item is visible, but a
         * line at the bottom stays blank because fewer headers are now in
         * view). Snap offset to the optimal last-page position so the window
         * is filled as much as possible while keeping the last item visible.
         * Compensate curr so the selection stays on the same item. */
        if (inv != NULL && s->curr + s->offset >= s->len) {
            guint old_offset = s->offset;
            s->offset = find_last_page_offset(s, inv, ifilter);
            s->curr  += old_offset - s->offset;
        }
    } else {
        s->curr++;

        /* curr may now exceed max_item_vis if a new category header became
         * visible since the last render (reducing max_item_vis by 1).
         * Transfer the excess to offset. */
        if (s->curr > s->max_item_vis) {
            s->offset += s->curr - s->max_item_vis;
            s->curr    = s->max_item_vis;
        }
    }
}

/* Scroll one page up. */
static void lss_page_up(list_scroll_state *s)
{
    if ((s->curr == s->max_item_vis) || s->offset == 0)
        s->curr = 1;
    else
        s->offset = (s->offset > s->max_item_vis)
                    ? (s->offset - s->max_item_vis) : 0;
}

/* Scroll one page down.
 * Pass inv == NULL for header-free lists. */
static void lss_page_down(list_scroll_state *s,
                          inventory **inv, int (*ifilter)(item *))
{
    if (s->curr == 1) {
        s->curr = s->max_item_vis;
        return;
    }

    int old_dist = s->max_item_vis - s->curr;
    s->offset += s->max_item_vis;

    if (inv != NULL) {
        guint hdrs = count_headers_in_range(inv, ifilter, s->offset, s->maxvis, false);
        s->curr = max(1, (int)s->maxvis - (int)hdrs - old_dist);
    } else {
        s->curr = s->max_item_vis;
    }

    if (s->offset + s->curr > s->len) {
        if (inv != NULL) {
            s->offset = find_last_page_offset(s, inv, ifilter);
        } else {
            s->offset = (s->len > s->max_item_vis) ? s->len - s->max_item_vis : 0;
        }

        s->curr = s->len - s->offset;
    }
}

/* Jump to a specific absolute list index (0-based).
 * Adjusts offset so the target is visible, preferring it at the top of the
 * window; falls back to lss_end positioning when near the end of the list.
 * Pass inv == NULL for header-free lists. */
static void lss_scroll_to(list_scroll_state *s, inventory **inv,
                          int (*ifilter)(item *), guint target)
{
    if (target >= s->len)
        return;

    if (target >= s->offset && target < s->offset + s->max_item_vis) {
        /* already visible */
        s->curr = target - s->offset + 1;
    } else if (target < s->offset) {
        /* above window: scroll up */
        s->offset = target;
        s->curr   = 1;
    } else {
        /* below window: place target at the top */
        s->offset = target;
        s->curr   = 1;

        if (inv != NULL) {
            guint hdrs = count_headers_in_range(inv, ifilter,
                s->offset, s->maxvis, false);

            if (s->offset + (s->maxvis - hdrs) >= s->len) {
                guint new_offset = find_last_page_offset(s, inv, ifilter);

                if (new_offset <= target) {
                    s->offset = new_offset;
                    s->curr   = target - s->offset + 1;
                }
            }
        } else {
            guint last = (s->len > s->max_item_vis) ? s->len - s->max_item_vis : 0;

            if (s->offset >= last) {
                s->offset = last;
                s->curr   = target - s->offset + 1;
            }
        }
    }
}

/* Handle a keystroke for a scrollable list.
 *
 * Processes all generic navigation keys (HOME, END, UP, DOWN, PAGE_UP,
 * PAGE_DOWN) and updates the given list_scroll_state accordingly.
 *
 * The inv/ifilter parameters are only relevant for inventory lists that have
 * category headers; pass NULL for both when used with header-free lists such
 * as the spell selector.
 *
 * The visible_category_headers parameter must reflect the number of header
 * lines currently rendered in the window (used by lss_down to detect a newly
 * appearing header at the bottom edge of the window). Pass 0 for header-free
 * lists.
 *
 * Returns true if the key was a navigation key and was handled, false if the
 * caller should continue processing the key itself. */
static bool list_handle_scroll_key(list_scroll_state *s, int key,
                                   inventory **inv, int (*ifilter)(item *),
                                   guint visible_category_headers)
{
    switch (key)
    {
    case '7':
    case KEY_HOME:
    case KEY_A1:
        lss_home(s);
        return true;

    case '9':
    case KEY_PPAGE:
    case KEY_A3:
    case 21: /* ^U */
        lss_page_up(s);
        return true;

    case 'k':
    case '8':
    case KEY_UP:
#ifdef KEY_A2
    case KEY_A2:
#endif
        lss_up(s);
        return true;

    case 'j':
    case '2':
    case KEY_DOWN:
#ifdef KEY_C2
    case KEY_C2:
#endif
        lss_down(s, inv, ifilter, visible_category_headers);
        return true;

    case '3':
    case KEY_NPAGE:
    case KEY_C3:
    case 4: /* ^D */
        lss_page_down(s, inv, ifilter);
        return true;

    case '1':
    case KEY_END:
    case KEY_C1:
        lss_end(s, inv, ifilter);
        return true;

    default:
        return false;
    }
}

item *display_inventory(const char *title, player *p, inventory **inv,
                        GPtrArray *callbacks, bool show_price,
                        bool show_weight, bool show_account,
                        int (*ifilter)(item *))
{
    /* the inventory window */
    display_window *iwin = NULL;
    /* the item description pop-up */
    display_window *ipop = NULL;

    /* the dialogue width and starting position */
    const guint width = min(COLS - 4, DISPLAY_WINDOW_MAX_WIDTH);
    const int startx = (COLS - width) / 2;

    guint len_curr;
    bool redraw = false;

    /* the window title used for shops */
    char *stitle = NULL;

    int key;

    /* string array used to assemble the window caption
       from the callback descriptions */
    char **captions;

    /* scroll position state */
    list_scroll_state s = { .curr = 1, .offset = 0, .maxvis = 0, .max_item_vis = 0 };

    item *it;

    /* curses attributes */
    attr_t attrs;

    g_assert(p != NULL && inv != NULL);

    /* sort inventory by item type */
    if (show_price)
        inv_sort(*inv, (GCompareDataFunc)item_sort_shop, (gpointer)p);
    else
        inv_sort(*inv, (GCompareDataFunc)item_sort_normal, (gpointer)p);

    /* store inventory length */
    guint len_orig = len_curr = inv_length_filtered(*inv, ifilter);

    /* main loop */
    bool keep_running = true;
    int autoscroll = 0; /* scroll-arrow auto-repeat state */

    do
    {
        /* Recompute window geometry and scroll state derived fields. */
        const guint max_height = LINES - 10;
        lss_recalc(&s, inv, ifilter, len_curr, max_height);

        guint height = min((guint)max_height,len_curr + (s.maxvis - s.max_item_vis) + 2);

        /* rebuild screen if needed */
        if (iwin != NULL && redraw)
        {
            display_window_destroy(iwin);
            iwin = NULL;

            display_paint_screen(p);
            redraw = false;

            if (len_curr > len_orig)
            {
                /* inventory has grown - sort inventory again */
                if (show_price)
                    inv_sort(*inv, (GCompareDataFunc)item_sort_shop, (gpointer)p);
                else
                    inv_sort(*inv, (GCompareDataFunc)item_sort_normal, (gpointer)p);
            }

            len_orig = len_curr;
        }

        if (!iwin)
        {
            iwin = display_window_new(startx, 2, width, height, title);
            if (callbacks != NULL)
            {
                iwin->destructor = (void (*)(void *))display_inv_callbacks_clean;
                iwin->destructor_data = callbacks;
            }
        }

        /*** draw all items ***/
        /* Seed last_type with the item just above the visible window so that
         * a category header is only rendered when the category boundary truly
         * falls inside the visible range. */
        item_t last_type = IT_NONE;
        if (s.offset > 0)
        {
            item *prev_it = inv_get_filtered(*inv, s.offset - 1, ifilter);
            last_type = prev_it->type;
        }

        /* count how many headlines we did show so far */
        guint shown_headers = 0;

        /* Mirror lss_recalc: when all items fit but their headers do not, the
         * first category header is suppressed at offset 0 so every item stays
         * visible. This must match count_headers_in_range exactly, otherwise
         * shown_headers would diverge from the pre-computed vis_hdrs and the
         * item_no arithmetic would run off the end of the list. */
        bool suppress_first = lss_suppress_first_header(&s, inv, ifilter,
                                                        max_height - 2);

        for (guint line = 1; line <= s.maxvis; line++)
        {
            guint item_no = (line - 1) + s.offset - shown_headers;

            /* Guard against out-of-bounds access: log the full state so the
             * faulty transition can be reconstructed, then stop drawing
             * rather than crashing. */
            if (item_no >= len_curr)
            {
                break;
            }

            it = inv_get_filtered(*inv, item_no, ifilter);

            /* Check if we need to display a category header */
            if (it->type != last_type)
            {
                last_type = it->type;

                /* Suppress the very first header when required, so all items
                 * fit. The item itself is still drawn on this line below. */
                if (line == 1 && suppress_first)
                {
                    /* fall through to draw the item, no header, no
                     * shown_headers increment */
                }
                else
                {
                    /* Display category header */
                    const char *category_name = item_name_pl(it->type);
                    /* Capitalize the first letter for display */
                    gchar *capitalized = str_capitalize(g_strdup(category_name));

                    mvwhline(iwin->window, line, 1, ACS_HLINE | CP_UI_BORDER, width - 2);
                    mvwaprintw(iwin->window, line, 4, CP_UI_BRIGHT_FG | A_BOLD,
                        " %s ", capitalized);
                    g_free(capitalized);

                    shown_headers++;

                    continue;
                }
            }

            bool item_equipped = false;

            if (!show_price)
            {
                /* shop items are definitely not equipped */
                item_equipped = player_item_is_equipped(p, it);
            }

            /* currently selected */
            if (s.curr == line - shown_headers)
            {
                if (item_equipped)
                    attrs = CP_UI_HL_REVERSE;
                else
                    attrs = CP_UI_FG_REVERSE;
            }
            else if (item_equipped)
                attrs = CP_UI_BRIGHT_FG;
            else
                attrs = CP_UI_FG;

            if (show_price)
            {
                /* inside shop */
                gchar *item_desc = item_describe_gc(it, true, false, false, GC_NOM);
                mvwaprintw(iwin->window, line, 1, attrs, _(" %-*s %5d gold "),
                          utf8_pad(item_desc, width - 15), item_desc,
                          item_price(it));

                g_free(item_desc);
            }
            else
            {
                gchar *item_desc = item_describe_gc(it, player_item_known(p, it),
                        false, false, GC_NOM);
                mvwaprintw(iwin->window, line, 1, attrs, " %-*s %c ",
                          utf8_pad(item_desc, width - 6), item_desc,
                          player_item_is_equipped(p, it) ? '*' : ' ');

                g_free(item_desc);
            }
        }

        /* update max_item_vis now that shown_headers is known exactly */
        s.max_item_vis = s.maxvis - shown_headers;

        /* keep the visible_category_headers value for lss_down */
        const guint visible_category_headers = shown_headers;

        /* prepare the window title */
        if (show_account)
        {
            /* show the balance of the bank account */
            stitle = g_strdup_printf(_("%s - %d gold on bank account"),
                                     title, p->bank_account);

            display_window_update_title(iwin, stitle);
            g_free(stitle);
        }
        else if (show_weight)
        {
            /* show the weight of the inventory */
            stitle = g_strdup_printf(_("%s - %s of %s carried"),
                                     title, player_inv_weight(p),
                                     player_can_carry(p));

            display_window_update_title(iwin, stitle);
            g_free(stitle);
        }

        /* get the currently selected item */
        it = inv_get_filtered(*inv, s.curr + s.offset - 1, ifilter);

        /* prepare the string array which will hold all the captions */
        captions = strv_new();

        /* assemble window caption (if callbacks have been defined) */
        for (guint cb_nr = 0; callbacks != NULL && cb_nr < callbacks->len; cb_nr++)
        {
            display_inv_callback *cb = g_ptr_array_index(callbacks, cb_nr);

            /* check if callback is appropriate for this item */
            /* if no check function is set, always display item */
            if ((cb->checkfun == NULL) || cb->checkfun(p, cb->inv, it))
            {
                cb->active = true;
                strv_append(&captions, cb->description);
            }
            else
            {
                /* it isn't */
                cb->active = false;
            }
        }

        /* refresh the item description pop-up */
        if (ipop != NULL)
            display_window_destroy(ipop);

        ipop = display_item_details(iwin->x1, iwin->y1 + iwin->height,
                                    iwin->width, it, p, show_price);

        if (g_strv_length(captions) > 0)
        {
            /* append "(?) help" to trigger the help pop-up */
            strv_append(&captions, _("(`KEY`?`end`) help"));

            /* update the window's caption with the assembled array of captions */
            display_window_update_caption(iwin, g_strjoinv(" ", captions));
        }
        else
        {
            /* reset the window caption */
            display_window_update_caption(iwin, NULL);
        }

        /* free the array of caption strings */
        g_strfreev(captions);

        display_window_update_arrow_up(iwin, s.offset > 0);
        display_window_update_arrow_down(iwin, (s.offset + s.maxvis) < len_curr);

        wrefresh(iwin->window);

        switch (key = display_scroll_getch(iwin, &autoscroll))
        {
        case KEY_ESC:
            keep_running = false;
            break;

        case '?':
            display_inventory_help(callbacks);
            break;

        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
            if (callbacks == NULL)
            {
                /* if no callbacks have been defined, enter selects item */
                keep_running = false;
            }
            break;

        default:
            /* handle window movement (including dragging by mouse) */
            if (display_window_move(iwin, key))
                break;

            if (list_handle_scroll_key(&s, key, inv, ifilter, visible_category_headers))
                break;

            /* Check if the key matches an item type glyph
             * and jump to the first item of that type. */
            for (item_t type = IT_NONE + 1; type < IT_MAX; type++)
            {
                if (item_data[type].glyph != key)
                    continue;

                /* Find the first item of this type. */
                guint target = len_curr; /* sentinel: not found */
                for (guint idx = 0; idx < len_curr; idx++)
                {
                    item *candidate = inv_get_filtered(*inv, idx, ifilter);
                    if (candidate->type == type) {
                        target = idx;
                        break;
                    }
                }

                if (target == len_curr)
                    /* type not present */
                    break;

                lss_scroll_to(&s, inv, ifilter, target);
                break;
            }

            /* check callback function keys (if defined) */
            for (guint cb_nr = 0; callbacks != NULL && cb_nr < callbacks->len; cb_nr++)
            {
                display_inv_callback *cb = g_ptr_array_index(callbacks, cb_nr);

                if ((cb->key == key) && cb->active)
                {
                    guint sel_idx = s.curr + s.offset - 1;
                    item *sel_it = inv_get_filtered(*inv, sel_idx, ifilter);

                    /* trigger callback */
                    cb->function(p, cb->inv, sel_it);

                    redraw = true;

                    /* don't check other callback functions */
                    break;
                }
            }
        };

        len_curr = inv_length_filtered(*inv, ifilter);
    }
    while (keep_running && (len_curr > 0)); /* ESC pressed or empty inventory*/

    display_window_destroy(ipop);
    display_window_destroy(iwin);

    if ((callbacks == NULL) && (key != KEY_ESC))
    {
        /* return selected item if no callbacks have been provided */
        return inv_get_filtered(*inv, s.offset + s.curr - 1, ifilter);
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

    g_ptr_array_free(callbacks, true);
}

void display_config_autopickup(bool settings[IT_MAX])
{
    int RUN = true;
    attr_t attrs; /* curses attributes */

    /* left column: types 1-6 (IT_AMULET..IT_GEM); right column:
       types 7-11 (IT_GOLD..IT_WEAPON) */
    const item_t left_types[]  = { IT_AMULET, IT_AMMO, IT_ARMOUR, IT_BOOK, IT_CONTAINER, IT_GEM };
    const item_t right_types[] = { IT_GOLD, IT_POTION, IT_RING, IT_SCROLL, IT_WEAPON };
    const guint left_count = G_N_ELEMENTS(left_types);
    const guint right_count = G_N_ELEMENTS(right_types);

    /* determine the label column width from the longest translated
       category label, so the layout fits any language */
    glong label_width = 0;
    for (guint i = 0; i < left_count; i++)
        label_width = MAX(label_width,
                g_utf8_strlen(item_name_pl(left_types[i]), -1));
    for (guint i = 0; i < right_count; i++)
        label_width = MAX(label_width,
                g_utf8_strlen(item_name_pl(right_types[i]), -1));

    /* column geometry, in window-relative coordinates */
    const int left_glyph_col = 4;
    const int left_label_col = left_glyph_col + 2;
    const int column_gap = 3;
    const int right_glyph_col = left_label_col + (int)label_width + column_gap;
    const int right_label_col = right_glyph_col + 2;
    const int columns_width = right_label_col + (int)label_width + 2;

    /* the title and the introductory message may also require more
       width than the columns do */
    const char *title = _("Configure auto pick-up");
    int width = MAX(columns_width, (int)g_utf8_strlen(title, -1) + 10);
    width = MIN(width, min(COLS - 4, DISPLAY_WINDOW_MAX_WIDTH));

    GPtrArray *intro_lines = text_wrap(
            _("Item types which will be picked up automatically are "
              "shown inverted."), width - 4, 0);

    const int msg_row = (int)intro_lines->len + 1;
    const int item_row = msg_row + 1;
    const int toggle_row = item_row + 6 + 1;
    const int height = toggle_row + 2;

    const int starty = (LINES - height) / 2;
    const int startx = (min(MAP_MAX_X, COLS) - width) / 2;

    display_window *cwin = display_window_new(startx, starty, width, height, title);

    for (guint i = 0; i < intro_lines->len; i++)
    {
        mvwaprintw(cwin->window, 1 + (int)i, 2, CP_UI_FG, "%s",
                (char *)g_ptr_array_index(intro_lines, i));
    }
    text_destroy(intro_lines);

    for (guint i = 0; i < left_count; i++)
    {
        mvwaprintw(cwin->window, item_row + (int)i, left_label_col, CP_UI_FG, "%s",
                item_name_pl(left_types[i]));
    }
    for (guint i = 0; i < right_count; i++)
    {
        mvwaprintw(cwin->window, item_row + (int)i, right_label_col, CP_UI_FG, "%s",
                item_name_pl(right_types[i]));
    }

    const char *toggle_msg = _("Type a symbol to toggle.");
    int toggle_col = MAX(2, (width - (int)g_utf8_strlen(toggle_msg, -1)) / 2);
    mvwaprintw(cwin->window, toggle_row, toggle_col, CP_UI_FG, "%s", toggle_msg);

    do
    {
        int key; /* keyboard input */

        for (item_t it = 1; it < IT_MAX; it++)
        {
            if (settings[it])
                attrs = CP_UI_FG_REVERSE;
            else
                attrs = CP_UI_FG;

            /* x / y position of the glyph depends on the item type number */
            int xpos = it < 7 ? left_glyph_col : right_glyph_col;
            int ypos = item_row + (it < 7 ? (int)it - 1 : (int)it - 7);
            mvwaprintw(cwin->window, ypos, xpos, attrs, "%c", item_glyph(it));
        }

        wrefresh(cwin->window);

        switch (key = display_getch(cwin->window))
        {
        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
        case KEY_ESC:
        case KEY_SPC:
            RUN = false;
            break;

        default:
            if (!display_window_move(cwin, key))
            {
                for (item_t it = 1; it < IT_MAX; it++)
                {
                    if (item_glyph(it) == key)
                    {
                        settings[it] = !settings[it];
                        break;
                    }
                }
            }
        }
    }
    while (RUN);

    display_window_destroy(cwin);
}

spell *display_spell_select(const char *title, player *p)
{
    display_window *ipop = NULL;
    int key; /* keyboard input */
    int RUN = true;
    int autoscroll = 0; /* scroll-arrow auto-repeat state */

    /* currently displayed spell; return value */
    spell *sp;

    /* curses attributes */
    attr_t attrs;

    g_assert(p != NULL);

    /* buffer for spell code type ahead */
    char *code_buf = g_malloc0(sizeof(char) * 4);

    /* sort spell list  */
    g_ptr_array_sort(p->known_spells, &spell_sort);

    /* set height according to spell count */
    guint height = min((LINES - 7), (p->known_spells->len + 2));

    guint width = 46;
    guint starty = (LINES - 3 - height) / 2;
    guint startx = (min(MAP_MAX_X, COLS) - width) / 2;

    display_window *swin = display_window_new(startx, starty, width, height, title);

    /* scroll state: no headers in the spell list */
    list_scroll_state s;
    lss_init(&s, p->known_spells->len, height - 2);

    int prev_key = 0;
    do
    {
        /* display spells */
        for (guint pos = 1; pos <= s.maxvis; pos++)
        {
            sp = g_ptr_array_index(p->known_spells, pos + s.offset - 1);

            if (s.curr == pos) attrs = CP_UI_FG_REVERSE;
            else attrs = CP_UI_FG;

            const char *sp_name = spell_name(sp);
            mvwaprintw(swin->window, pos, 1, attrs,
                      " %3s - %-*s (Level %d) %2d ",
                      spell_code(sp),
                      utf8_pad(sp_name, 23), sp_name,
                      spell_level(sp),
                      sp->knowledge);
        }

        /* display up / down markers */
        display_window_update_arrow_up(swin, (s.offset > 0));
        display_window_update_arrow_down(swin, ((s.offset + s.maxvis) < p->known_spells->len));

        /* construct the window caption: display type ahead keys */
        gchar *caption = g_strdup_printf("%s%s%s",
                (strlen(code_buf) ? "[" : ""),
                code_buf,
                (strlen(code_buf) ? "]" : ""));

        display_window_update_caption(swin, caption);

        /* store currently highlighted spell */
        sp = g_ptr_array_index(p->known_spells, s.curr + s.offset - 1);

        /* refresh the spell description pop-up */
        if (ipop != NULL)
            display_window_destroy(ipop);

        gchar *spdesc = spell_desc_by_id(sp->id);;
        ipop = display_popup(swin->x1, swin->y1 + swin->height, width,
                spell_name(sp), spdesc, 0);
        g_free(spdesc);

        switch (key = display_scroll_getch(swin, &autoscroll))
        {
        case KEY_ESC:
            RUN = false;
            sp = NULL;
            break;

        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
        case KEY_SPC:
            // It is much too easy to accidentally cast alter reality,
            // simply by pressing m + Enter. If the first key press in
            // the menu confirms this auto selected first spell, prompt.
            if (s.curr == 1 && prev_key == 0 && sp->id == SP_ALT)
            {
                char prompt[60];
                g_snprintf(prompt, 60, _("Really cast %s?"), spell_name(sp));
                if (!display_get_yesno(prompt, NULL, NULL, NULL))
                    sp = NULL;
            }
            RUN = false;
            break;

        case KEY_BS:
        case KEY_BACKSPACE:
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
                break;

            /* 'k' and 'j' redirect to mnemonics when a code is being typed */
            if ((key == 'k' || key == 'j') && strlen(code_buf) > 0)
                goto mnemonics;

            if (list_handle_scroll_key(&s, key, NULL, NULL, 0))
            {
                code_buf[0] = '\0';
                break;
            }

mnemonics:
            /* add key to spell code buffer */
            if ((key >= 'a') && (key <= 'z'))
            {
                if (strlen(code_buf) < 3)
                {
                    code_buf[strlen(code_buf)] = key;
                    /* search for match */

                    for (guint pos = 1; pos <= p->known_spells->len; pos++)
                    {
                        sp = g_ptr_array_index(p->known_spells, pos - 1);

                        if (g_str_has_prefix(spell_code(sp), code_buf)) {
                            /* match found: jump to the spell */
                            lss_scroll_to(&s, NULL, NULL, pos - 1);
                            break;
                        }
                    }

                    /* if no match has been found remove key from buffer */
                    sp = g_ptr_array_index(p->known_spells, s.curr + s.offset - 1);
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
        prev_key = key;
    }
    while (RUN);

    g_free(code_buf);

    display_window_destroy(swin);
    display_window_destroy(ipop);

    return sp;
}

int display_get_count(const char *caption, int value)
{
    /* toggle insert / overwrite mode; start with overwrite */
    int insert_mode = false;

    /* user input */
    int key;

    /* cursor position */
    int ipos = 0;

    /* input as char */
    char ivalue[8] = { 0 };

    /* continue editing the number */
    int cont = true;

    /* 8: input field width; 5: 3 spaces between border, caption + input field, 2 border */
    int basewidth = 8 + 5;

    /* choose a sane dialogue width */
    int width = min(basewidth + g_utf8_strlen(caption, -1), COLS - 4);

    GPtrArray *text = text_wrap(caption, width - basewidth, 0);
    int height = 2 + text->len;

    int starty = (LINES - height) / 2;
    int startx = (COLS - width) / 2;

    display_window *mwin = display_window_new(startx, starty, width, height, NULL);

    for (guint line = 0; line < text->len; line++)
    {
        /* print text */
        mvwaprintw(mwin->window, 1 + line, 2, CP_UI_FG,
            "%s", (char *)g_ptr_array_index(text, line));
    }

    /* prepare string to edit */
    g_snprintf(ivalue, 8, "%d", value);

    do
    {
        int ilen = strlen(ivalue); /* input length */

        /* make cursor visible and set according to insert mode */
        if (insert_mode)
            curs_set(1); /* underline */
        else
            curs_set(2); /* block */

        mvwaprintw(mwin->window,  mwin->height - 2, mwin->width - 10,
                  CP_UI_HL_REVERSE, "%-8s", ivalue);

        wmove(mwin->window, mwin->height - 2, mwin->width - 10 + ipos);
        wrefresh(mwin->window);

        switch (key = display_getch(mwin->window))
        {
        case KEY_LEFT:
            if (ipos > 0)
                ipos--;
            break;

        case KEY_RIGHT:
            if (ipos < ilen)
                ipos++;
            break;

        case KEY_BS:
        case KEY_BACKSPACE:
            if ((ipos == ilen) && (ipos > 0))
            {
                ivalue[ipos - 1] = '\0';
                ipos--;
            }
            else if (ipos > 0)
            {
                for (int tmp = ipos - 1; tmp < ilen; tmp++)
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
                for (int tmp = ipos; tmp < ilen; tmp++)
                    ivalue[tmp] = ivalue[tmp + 1];
            }
            break;

        case KEY_END:
            ipos = ilen;
            break;

        case KEY_HOME:
            ipos = 0;
            break;

            /* special cases to speed up getting/dropping multiple items */
        case 'y': /* yes */
        case 'd': /* drop */
        case 'g': /* get */
        case 'p': /* put */
            /* reset value to original value */
            g_snprintf(ivalue, 8, "%d", value);
            cont = false;
            break;

            /* special case to speed up aborting */
        case 'n': /* no */
            /* set value to 0 */
            g_snprintf(ivalue, 8, "%d", 0);
            cont = false;
            break;


        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
        case KEY_ESC:
            cont = false;
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

    /* hide cursor */
    curs_set(0);

    text_destroy(text);
    display_window_destroy(mwin);

    if (key == KEY_ESC)
        return 0;

    return atoi(ivalue);
}

char *display_get_string(const char *title, const char *caption, const char *value, size_t max_len)
{
    /* user input */
    int key;

    /* toggle insert / overwrite mode */
    int insert_mode = true;

    /* cursor position */
    guint ipos = 0;

    /* text to be edited */
    GString *string = g_string_new(value);

    /* continue editing the number */
    int cont = true;

    /* 3 spaces between border, caption + input field, 2 border */
    int basewidth = 5;

    /* choose a sane dialogue width */
    guint width;
    guint maxwidth = COLS - 4;

    /* prevent overly large input fields */
    if (max_len + basewidth > maxwidth)
    {
        max_len = maxwidth - basewidth;
    }

    const guint caption_len = g_utf8_strlen(caption, -1);

    if (basewidth + caption_len + max_len > maxwidth)
    {
        if (caption_len + basewidth > maxwidth)
        {
            width = maxwidth - basewidth;
        }
        else
        {
            width = basewidth + max(caption_len, max_len);
        }
    }
    else
    {
        /* input box fits on same line as caption */
        width = basewidth + caption_len + max_len + 1;
    }

    GPtrArray *text = text_wrap(caption, width - basewidth, 0);

    /* determine if the input box fits on the last line */
    int box_start = 3
        + g_utf8_strlen(g_ptr_array_index(text, text->len - 1), -1);
    if (box_start + max_len + 2 > width)
        box_start = 2;

    int height = 2 + text->len; /* borders and text length */
    if (box_start == 2) height += 1; /* grow the dialogue if input box doesn't fit */

    int starty = (LINES - height) / 2;
    int startx = (COLS - width) / 2;

    display_window *mwin = display_window_new(startx, starty, width, height, title);

    for (guint line = 0; line < text->len; line++)
    {
        /* print text */
        const char *tline = g_ptr_array_index(text, line);
        mvwaprintw(mwin->window, 1 + line, 1, CP_UI_FG,
            " %-*s ", utf8_pad(tline, width - 4), tline);
    }

    do
    {
        mvwaprintw(mwin->window,  mwin->height - 2, box_start,
            CP_UI_HL_REVERSE, "%-*s", (int)max_len + 1, string->str);

        wmove(mwin->window, mwin->height - 2, box_start + ipos);

        /* make cursor visible and set according to insert mode */
        if (insert_mode)
            curs_set(1); /* underline */
        else
            curs_set(2); /* block */

        wrefresh(mwin->window);

        switch (key = display_getch(mwin->window))
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

        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
        case KEY_ESC:
            cont = false;
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

    /* hide cursor */
    curs_set(0);

    text_destroy(text);
    display_window_destroy(mwin);

    if (key == KEY_ESC || string->len == 0)
    {
        g_string_free(string, true);
        return NULL;
    }

    return g_string_free(string, false);
}

int display_get_yesno(const char *question, const char *title, const char *yes, const char *no)
{
    int RUN = true;
    int selection = false;
    guint line;

    const guint padding = 1;
    const guint margin = 2;

    /* default values */
    if (!yes)
        yes = _("Yes");

    if (!no)
        no = _("No");

    /* determine text width, either defined by space available  for the window
     * or the length of question */
    guint text_width = min(COLS - 2 /* borders */
                     - (2 * margin) /* space outside window */
                     - (2 * padding), /* space between border and text */
                     g_utf8_strlen(question, -1));

    /* broad windows are hard to read */
    if (text_width > 60)
        text_width = 60;

    /* wrap question according to width */
    GPtrArray *text = text_wrap(question, text_width + 1, 0);

    /* Determine window width. Either defined by the length of the button
     * labels or width of the text */
    guint width = max(g_utf8_strlen(yes, -1) + g_utf8_strlen(no, -1)
                + 2 /* borders */
                + (4 * padding)  /* space between "button" border and label */
                + margin, /* space between "buttons" */
                text_width + 2 /* borders */ + (2 * padding));

    /* set startx and starty to something that makes sense */
    guint startx = (COLS / 2) - (width / 2);
    guint starty = (LINES / 2) - 4;

    display_window *ywin = display_window_new(startx, starty, width, text->len + 4, title);

    for (line = 0; line < text->len; line++)
    {
        mvwaprintw(ywin->window, line + 1, 1 + padding, CP_UI_FG,
            "%s", (char *)g_ptr_array_index(text, line));
    }

    text_destroy(text);

    do
    {
        /* paint */
        attr_t attrs;

        if (selection) attrs = CP_UI_HL_REVERSE;
        else           attrs = CP_UI_FG_REVERSE;

        mvwaprintw(ywin->window, line + 2, margin, attrs,
                   "%*s%s%*s", padding, " ", yes, padding, " ");

        if (selection) attrs = CP_UI_FG_REVERSE;
        else           attrs = CP_UI_HL_REVERSE;

        mvwaprintw(ywin->window, line + 2,
                   width - margin - g_utf8_strlen(no, -1) - (2 * padding),
                   attrs, "%*s%s%*s", padding, " ", no, padding, " ");

        wrefresh(ywin->window);

        int key = tolower(display_getch(ywin->window)); /* input key buffer */
        // Special case for the movement keys and y/n.
        if (key != 'h' && key != 'l' && key != 'y' && key != 'n')
        {
            char input_yes = g_ascii_tolower(yes[0]);
            char input_no  = g_ascii_tolower(no[0]);
            // If both answers share the same initial letter, we're out of luck.
            if (input_yes != input_no || input_yes == 'n' || input_no == 'y')
            {
                if (key == input_yes)
                    key = 'y';
                else if (key == input_no)
                    key = 'n';
            }
        }


        /* wait for input */
        switch (key)
        {
        case KEY_ESC:
            selection = false;
            /* fall-through */

        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
        case KEY_SPC:
            RUN = false;
            break;

        case 'h':
        case '4':
#ifdef KEY_B1
        case KEY_B1:
#endif
        case KEY_LEFT:
            if (!selection)
                selection = true;
            break;

        case 'l':
        case '6':
#ifdef KEY_B3
        case KEY_B3:
#endif
        case KEY_RIGHT:
            if (selection)
                selection = false;
            break;

            /* shortcuts */
        case 'y':
            selection = true;
            RUN = false;
            break;

        case 'n':
            selection = false;
            RUN = false;
            break;

        default:
            /* perhaps the window shall be moved */
            display_window_move(ywin, key);
        }
    }
    while (RUN);

    display_window_destroy(ywin);

    return selection;
}

direction display_get_direction(const char *title, int *available)
{
    int *dirs = NULL;
    int RUN = true;

    /* direction to return */
    direction dir = GD_NONE;

    if (!available)
    {
        dirs = g_malloc0(sizeof(int) * GD_MAX);
        for (int x = 0; x < GD_MAX; x++)
            dirs[x] = true;

        dirs[GD_CURR] = false;
    }
    else
    {
        dirs = available;
    }

    int width = max(9, g_utf8_strlen(title, -1) + 4);

    /* set startx and starty to something that makes sense */
    int startx = (min(MAP_MAX_X, COLS) / 2) - (width / 2);
    int starty = (LINES / 2) - 4;

    display_window *dwin = display_window_new(startx, starty, width, 9, title);

    mvwaprintw(dwin->window, 3, 3, CP_UI_FG, "\\|/");
    mvwaprintw(dwin->window, 4, 3, CP_UI_FG, "- -");
    mvwaprintw(dwin->window, 5, 3, CP_UI_FG, "/|\\");

    for (int x = 0; x < 3; x++)
        for (int y = 0; y < 3; y++)
        {
            if (dirs[(x + 1) + (y * 3)])
            {
                mvwaprintw(dwin->window,
                          6 - (y * 2), /* start in the last row, move up, skip one */
                          (x * 2) + 2, /* start in the second col, skip one */
                          CP_UI_TITLE,
                          "%d",
                          (x + 1) + (y * 3));
            }
        }

    if (!available)
        g_free(dirs);

    wrefresh(dwin->window);

    do
    {
        int key; /* input key buffer */

        switch (key = display_getch(dwin->window))
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

        case KEY_ESC:
            RUN = false;
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

    display_window_destroy(dwin);

    return dir;
}

position display_get_position(player *p,
                              const char *message,
                              bool ray,
                              bool ball,
                              guint radius,
                              bool passable,
                              bool visible)
{
    /* start at player's position */
    position start = p->pos;

    /* if the player has recently targeted a monster... */
    if (visible && p->ptarget != NULL)
    {
        monster *m = game_monster_get(nlarn, p->ptarget);

        /* ...check if it is still alive */
        if (m == NULL)
        {
            /* the monster has been eliminated */
            p->ptarget = NULL;
        }
        else
        {
            /* start at the monster's current position */
            start = monster_pos(m);

            /* don't use invisible position if unwanted */
            if (!fov_get(p->fv, start)) start = p->pos;
        }
    }

    /* check for visible opponents if no previous opponent has been found */
    if (visible && pos_identical(p->pos, start))
    {
        monster *m = fov_get_closest_monster(p->fv);

        /* found a visible monster -> use it as target */
        if (m != NULL)
            start = monster_pos(m);
    }

    /* the position chosen by the player */
    position cpos = display_get_new_position(p, start, message, ray, ball,
        false, radius, passable, visible);

    if (pos_valid(cpos))
    {
        /* the player has chosen a valid position, check if there
         * is a monster at that position. */
        map *gmap = game_map(nlarn, Z(cpos));
        monster *m = map_get_monster_at(gmap, cpos);

        if (m != NULL)
        {
            /* there is a monster, store its oid for later use */
            p->ptarget = monster_oid(m);
        }
    }

    return cpos;
}

position display_get_new_position(player *p,
                                  position start,
                                  const char *message,
                                  bool ray,
                                  bool ball,
                                  bool travel,
                                  guint radius,
                                  bool passable,
                                  bool visible)
{
    bool RUN = true;
    direction dir = GD_NONE;
    position pos;
    attr_t attrs; /* curses attributes */
    display_window *msgpop = NULL;

    /* list of visible monsters and the iterator for these */
    GList *mlist = NULL, *miter = NULL;

    /* variables for ray or ball painting */
    GList *r = NULL;      /* a ray position list */
    monster *m;

    /* check the starting position makes sense */
    if (pos_valid(start) && Z(start) == Z(p->pos))
        pos = start;
    else
        pos = p->pos;

    /* make shortcut to visible map */
    map *vmap = game_map(nlarn, Z(p->pos));

    /* get the list of visible monsters if looking for a visible position */
    if (visible)
        miter = mlist = fov_get_visible_monsters(p->fv);

    if (!visible)
        msgpop = display_popup(3, min(MAP_MAX_Y + 4, LINES - 4), 0, NULL, message, 0);

    /* if a starting position for a ray has been provided, check if it works */
    if (ray && !pos_identical(p->pos, start))
    {
        /* paint a ray to validate the starting position */
        r = map_ray(vmap, p->pos, pos);

        if (r == NULL)
        {
            /* it's not possible to draw a ray between the points
               -> leave everything as it has been */
            pos = p->pos;
        }
        else
        {
            /* position is valid */
            g_list_free(r);
            r = NULL;
        }
    } /* ray starting position validity check */

    do
    {
        /* refresh the pop-up content for every position change
           while looking for visible positions */
        if (visible)
        {
            /* clean old pop-up windows */
            if (msgpop != NULL)
                display_window_destroy(msgpop);

            /* display message or description of selected position */
            if (pos_identical(pos, p->pos))
            {
                msgpop = display_popup(3, min(MAP_MAX_Y + 4, LINES - 4), 0, NULL, message, 0);
            }
            else
            {
                char *desc = map_pos_examine(pos);
                msgpop = display_popup(3, min(MAP_MAX_Y + 4, LINES - 4), 0, message, desc, 0);
                g_free(desc);
            }
        } /* visible */

        /* redraw screen to erase previous modifications */
        display_paint_screen(p);

        /* reset npos to an invalid position */
        position npos = pos_invalid;

        /* draw a ray if the starting position is not the player's position */
        if (ray && !pos_identical(pos, p->pos))
        {
            r = map_ray(vmap, p->pos, pos);

            if (r == NULL)
            {
                /* It wasn't possible to paint a ray to the target position.
                   Revert to the player's position.*/
                pos = p->pos;
            }
        }

        if (ray && (r != NULL))
        {
            /* draw a line between source and target if told to */
            monster *target = map_get_monster_at(vmap, pos);

            if (target && monster_in_sight(target))
                attrs = COLOR_PAIR(LUMINOUS_RED);
            else
                attrs = COLOR_PAIR(ELECTRIC_BLUE);

            GList *iter = r;

            do
            {
                position tpos;
                pos_val(tpos) = GPOINTER_TO_UINT(iter->data);

                /* skip the player's position */
                if (pos_identical(p->pos, tpos))
                    continue;

                if (target && pos_identical(monster_pos(target), tpos)
                    && monster_in_sight(target))
                {
                    /* ray is targeted at a visible monster */
                    mvaddwach(Y(tpos), X(tpos), attrs, monster_glyph(target));
                }
                else if ((m = map_get_monster_at(vmap, tpos))
                         && monster_in_sight(m))
                {
                    /* ray sweeps over a visible monster */
                    mvaddwach(Y(tpos), X(tpos), attrs, monster_glyph(m));
                }
                else
                {
                    /* a position with no or an invisible monster on it */
                    mvaddwach(Y(tpos), X(tpos), attrs, '*');
                }
            } while ((iter = iter->next));

            g_list_free(r);
            r = NULL;
        }
        else if (ball && radius)
        {
            /* paint a ball if told to */
            area *obstacles = map_get_obstacles(vmap, pos, radius, false);
            area *b = area_new_circle_flooded(pos, radius, obstacles);
            position cursor = pos;

            for (Y(cursor) = b->start_y; Y(cursor) < b->start_y + b->size_y; Y(cursor)++)
            {
                for (X(cursor) = b->start_x; X(cursor) < b->start_x + b->size_x; X(cursor)++)
                {
                    if (area_pos_get(b, cursor))
                    {
                        colour_t fg;
                        wchar_t glyph;

                        if ((m = map_get_monster_at(vmap, cursor)) && monster_in_sight(m))
                        {
                            fg = monster_color(m) == STRAWBERRY_RED ? LUMINOUS_RED : STRAWBERRY_RED;
                            glyph = monster_glyph(m);
                        }
                        else if (pos_identical(p->pos, cursor))
                        {
                            fg = DEEP_ORANGE;
                            glyph = '@';
                        }
                        else
                        {
                            fg = ELECTRIC_BLUE;
                            glyph = '*';
                        }

                        move(Y(cursor), X(cursor));
                        addwach(glyph, fg, 0);
                    }
                }
            }
            area_destroy(b);
        }
        else
        {
            /* show the position of the cursor by inverting the attributes */
            (void)mvwchgat(stdscr, Y(pos), X(pos), 1, A_BOLD | A_STANDOUT,
                WHITE, NULL);
        }

        /* wait for input */
        const int ch = display_getch(NULL);
        switch (ch)
        {
            /* abort */
        case KEY_ESC:
            pos = pos_invalid;
            RUN = false;
            break;

            /* finish */
        case KEY_LF:
        case KEY_CR:
#ifdef PADENTER
        case PADENTER:
#endif
        case KEY_ENTER:
            RUN = false;
            /* if a passable position has been requested check if it
               actually is passable. Only known positions are allowed. */
            if (passable
                && (!(player_memory_of(nlarn->p, pos).type > LT_NONE
                      || game_wizardmode(nlarn))
                    || !map_pos_passable(vmap, pos)))
            {
                if (!beep()) flash();
                RUN = true;
            }
            break;

            /* jump to next visible monster */
        case KEY_SPC:
            if ((mlist == NULL) && visible)
            {
                flash();
            }
            else if (mlist != NULL)
            {
                /* jump to the next list entry or to the start of
                   the list if the end has been reached */
                if ((miter = miter->next) == NULL)
                    miter = mlist;

                /* get the currently selected monster */
                m = (monster *)miter->data;

                /* jump to the selected monster */
                npos = monster_pos(m);
            }
            break;

            /* mouse targeting */
        case KEY_MOUSE:
            /* a left click on the map moves the cursor to the clicked
               cell; clicking the cell the cursor already occupies
               confirms the choice, just like pressing ENTER */
            if (display_mouse_event.bstate
                    & (BUTTON1_PRESSED | BUTTON1_CLICKED))
            {
                const int mx = display_mouse_event.x;
                const int my = display_mouse_event.y;

                /* the map occupies screen rows 0 .. MAP_MAX_Y - 1 and
                   columns 0 .. MAP_MAX_X - 1, one character per cell */
                if (mx >= 0 && mx < MAP_MAX_X && my >= 0 && my < MAP_MAX_Y)
                {
                    position mpos = pos;
                    X(mpos) = mx;
                    Y(mpos) = my;

                    if (pos_identical(mpos, pos))
                        /* confirm via the ENTER handling, so the
                           passability checks are applied there */
                        ungetch(KEY_ENTER);
                    else
                        npos = mpos;
                }
            }
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

        case '@':
            /* bring the cursor back to the player */
            npos = p->pos;
            break;

        default:
            /* if travelling, use sobject glyphs as shortcuts */
            if (travel)
            {
                sobject_t sobj = LS_NONE;
                for (int i = LS_NONE + 1; i < LS_MAX; i++)
                    if (so_get_glyph(i) == (char) ch)
                    {
                        sobj = i;
                        log_add_entry(nlarn->log, _("Looking for '%c' (%s)."),
                                      (char) ch, so_get_desc(sobj));
                        break;
                    }

                /* found a matching glyph, now search the remembered level */
                if (sobj != LS_NONE)
                {
                    position origin = pos;
                    while (true)
                    {
                        if (++X(pos) >= MAP_MAX_X)
                        {
                            X(pos) = 0;
                            if (++Y(pos) >= MAP_MAX_Y)
                                Y(pos) = 0;
                        }
                        if (pos_identical(origin, pos))
                            break;

                        /* When in wizard mode, compare the selected glyph
                           with the real map, otherwise with the player's
                           memory of the map.
                           As multiple objects share the same glyph, it is
                           required to compare the glyph of the present
                           object with the glyph of the requested object. */
                        if ((game_wizardmode(nlarn)
                             && so_get_glyph(map_sobject_at(vmap, pos)) == (char) ch)
                            || (player_memory_of(nlarn->p, pos).sobject != LS_NONE
                                && so_get_glyph(player_memory_of(nlarn->p, pos).sobject) == (char) ch))
                        {
                            break;
                        }
                    }
                }
            } /* if (travel) */
            break;
        }

        /* get new position if cursor has been moved */
        if (dir)
        {
            npos = pos_move(pos, dir);
            dir = GD_NONE;
        }

        /* don't want to deal with invalid positions */
        if (pos_valid(npos))
        {
            /* don't use invisible positions */
            if (visible && !fov_get(p->fv, npos))
                npos = pos;

            if (ray)
            {
                /* paint a ray to validate the new position */
                r = map_ray(vmap, p->pos, npos);

                if (r == NULL)
                {
                    /* it's not possible to draw a ray between the points
                       -> return to previous position */
                    npos = pos;
                }
                else
                {
                    g_list_free(r);
                    r = NULL;
                }
            }

            if (ball)
            {
                /* check bounds of the ball */
                if (!map_pos_passable(vmap, npos)) npos = pos;
            }

            /* new position is within bounds and visible */
            pos = npos;
        } /* if (pos_valid(npos) */
    }
    while (RUN);

    /* destroy list of visible monsters */
    if (mlist != NULL)
        g_list_free(mlist);

    /* destroy the message pop-up */
    display_window_destroy(msgpop);

    /* hide cursor */
    curs_set(0);

    return pos;
}

void display_show_history(message_log *log, const char *title)
{
    GString *text = g_string_new(NULL);
    char intrep[11] = { 0 }; /* string representation of the game time */

    /* determine the number of characters of the current game turn */
    g_snprintf(intrep, 10, "%d", nlarn->gtime);
    int twidth = strlen(intrep);

    /* assemble reversed game log */
    for (guint idx = log_length(log); idx > 0; idx--)
    {
        message_log_entry *le = log_get_entry(log, idx - 1);
        g_string_append_printf(text, "%*d: %s\n", twidth, le->gtime, le->message);
    }

    /* display the log */
    display_show_message(title, text->str, twidth + 2);

    /* free the assembled log */
    g_string_free(text, true);
}

int display_show_message(const char *title, const char *message, int indent)
{
    int key;

    /* Number of columns required for
         a) the window border and the text padding
         b) the margin around the window */
    const guint wred = 4;

    bool RUN = true;

    /* default window width */
    guint width = min(COLS - 4, DISPLAY_WINDOW_MAX_WIDTH);

    /* wrap message according to width (minus border and padding) */
    GPtrArray *text = text_wrap(message, width - wred, indent);

    /* determine the length of longest text line */
    guint max_len = text_get_longest_line(text);

    /* shrink the window width if the default width is not required */
    if (max_len + wred < width)
        width = max_len + wred;

    /* set height according to message line count */
    guint height = min((LINES - 3), (text->len + 2));

    guint starty = (LINES - height) / 2;
    guint startx = (COLS - width) / 2;
    display_window *mwin = display_window_new(startx, starty, width, height, title);
    guint maxvis = min(text->len, height - 2);
    guint offset = 0;
    int autoscroll = 0; /* scroll-arrow auto-repeat state */

    do
    {
        /* display the window content */
        attr_t currattr = COLOURLESS;

        for (guint idx = 0; idx < maxvis; idx++)
        {
            currattr = mvwcprintw(mwin->window,
                CP_UI_FG, currattr, UI_BG, idx + 1, 2, "%s",
                g_ptr_array_index(text, idx + offset));

            /* erase to the end of the line (spare the borders of the window) */
            for (int pos = getcurx(mwin->window); pos < getmaxx(mwin->window) - 1; pos++)
            {
                waddch(mwin->window, ' ' | currattr);
            }
        }

        display_window_update_arrow_up(mwin, offset > 0);
        display_window_update_arrow_down(mwin, (offset + maxvis) < text->len);

        wrefresh(mwin->window);

        key = display_scroll_getch(mwin, &autoscroll);
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
        case 21: /* ^U */
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
        case 4: /* ^D */
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
                RUN = false;
            }
        }
    }
    while (RUN);

    display_window_destroy(mwin);
    text_destroy(text);

    return key;
}

display_window *display_popup(int x1, int y1, int width, const char *title, const char *msg, int indent)
{
    const guint max_width = COLS - x1 - 1;
    const guint max_height = LINES - y1;

    if (width == 0)
    {
        guint maxlen;

        /* The title is padded by 6 additional characters */
        if ((title != NULL)
                && (g_utf8_strlen(title, -1) + 6 > g_utf8_strlen(msg, -1)))
            maxlen = g_utf8_strlen(title, -1) + 6;
        else
            maxlen = g_utf8_strlen(msg, -1);

        /* determine window width */
        if (maxlen > (max_width - 4))
            width = max_width - 4;
        else
            width = maxlen + 4;
    }
    else
    {
        /* width supplied. Sanity check */
        if ((unsigned)width > max_width)
            width = max_width;
    }

    GPtrArray *text = text_wrap(msg, width - 4, indent);
    int height = min(text->len + 2, max_height);

    display_window *win = display_window_new(x1, y1, width, height, title);

    /* display message */
    attr_t currattr = COLOURLESS;
    for (guint idx = 0; idx < text->len; idx++)
    {
        const char *tline = g_ptr_array_index(text, idx);
        currattr = mvwcprintw(win->window,
            CP_UI_FG, currattr, UI_BG, idx + 1, 1, " %-*s ",
            utf8_pad(tline, width - 4), tline);
    }

    /* clean up */
    text_destroy(text);

    /* show the window */
    wrefresh(win->window);

    return win;
}

void display_window_destroy(display_window *dwin)
{
    del_panel(dwin->panel);
    delwin(dwin->window);

    /* remove window from the list of windows */
    windows = g_list_remove(windows, dwin);

    /* free title as it is a clone */
    if (dwin->title != NULL) g_free(dwin->title);

    g_free(dwin);

    if (nlarn != NULL && nlarn->p != NULL)
    {
        /* repaint the screen if the game has been initialized */
        display_paint_screen(nlarn->p);
    }
    else
    {
        /* refresh the screen */
        display_draw();
    }
}

void display_windows_hide()
{
    GList *iterator = windows;

    while (iterator)
    {
        display_window *win = (display_window *)iterator->data;
        hide_panel(win->panel);
        iterator = iterator->next;
    }
}

void display_windows_show()
{
    GList *iterator = windows;

    while (iterator)
    {
        display_window *win = (display_window *)iterator->data;
        show_panel(win->panel);
        iterator = iterator->next;
    }
}

void display_windows_destroy_all()
{
    while (windows)
    {
        display_window *win = (display_window *)windows->data;
        if (win->destructor != NULL)
            win->destructor(win->destructor_data);
        del_panel(win->panel);
        delwin(win->window);
        if (win->title != NULL) g_free(win->title);
        g_free(win);
        windows = g_list_delete_link(windows, windows);
    }
}

int display_getch(WINDOW *win) {
    int ch = wgetch(win ? win : stdscr);
#ifdef SDLPDCURSES
        /* on SDL2 PDCurses, keys entered on the numeric keypad while num
           lock is enabled are returned twice, hence we need to swallow
           the first one here. */
        if ((ch >= '1' && ch <= '9')
                && (PDC_get_key_modifiers() & PDC_KEY_MODIFIER_NUMLOCK)
                && PDC_check_key())
        {
            ch = wgetch(win ? win : stdscr);
        }
#endif

    if (ch == KEY_MOUSE)
    {
        memset(&display_mouse_event, 0, sizeof(display_mouse_event));
        /* a failed retrieval leaves an all-zero event, which no mouse
           handling code reacts upon */
        getmouse(&display_mouse_event);
    }

    return ch;
}

position display_get_mouse_position(void)
{
    position pos = pos_invalid;

    /* only a left click reports a target position */
    if (display_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED))
    {
        const int mx = display_mouse_event.x;
        const int my = display_mouse_event.y;

        /* the map occupies screen rows 0 .. MAP_MAX_Y - 1 and columns
           0 .. MAP_MAX_X - 1, one character per cell */
        if (mx >= 0 && mx < MAP_MAX_X && my >= 0 && my < MAP_MAX_Y)
        {
            X(pos) = mx;
            Y(pos) = my;
            Z(pos) = Z(nlarn->p->pos);
        }
    }

    return pos;
}

/* Determine which scroll arrow of a window is at the given screen
   position: -1 for the up arrow, +1 for the down arrow, 0 for
   neither. The arrows occupy the three columns rendered by
   display_window_update_arrow_up() / _down(). */
static int display_window_arrow_at(display_window *dwin, int x, int y)
{
    if (x < (int)(dwin->x1 + dwin->width - 5)
            || x > (int)(dwin->x1 + dwin->width - 3))
        return 0;

    if (y == (int)dwin->y1)
        return -1;
    if (y == (int)(dwin->y1 + dwin->height - 1))
        return 1;

    return 0;
}

/* Input reader for scrollable windows. Behaves like display_getch(),
   but additionally turns a left click on a scroll arrow into the
   matching KEY_UP / KEY_DOWN key code, and keeps emitting that code at
   a slow rate while the button is held on the arrow (auto-repeat).

   *autoscroll carries the repeat state between calls: it holds the
   active scroll direction (-1 / +1) while an arrow is held, and 0
   otherwise. Callers start it at 0 and pass the same variable each
   time. */
static int display_scroll_getch(display_window *dwin, int *autoscroll)
{
    /* auto-repeat delay in milliseconds - deliberately slow */
    const int repeat_ms = 120;

    if (*autoscroll != 0)
    {
        /* wait a short while for an event that would end the repeat */
        wtimeout(dwin->window, repeat_ms);
        int key = wgetch(dwin->window);
        wtimeout(dwin->window, -1);

        /* no event: the button is still held, keep scrolling */
        if (key == ERR)
            return (*autoscroll < 0) ? KEY_UP : KEY_DOWN;

        /* any event ends the current repeat */
        int dir = *autoscroll;
        *autoscroll = 0;

        if (key == KEY_MOUSE)
        {
            memset(&display_mouse_event, 0, sizeof(display_mouse_event));
            if (getmouse(&display_mouse_event) == OK
                    && !(display_mouse_event.bstate & BUTTON1_RELEASED)
                    && display_window_arrow_at(dwin, display_mouse_event.x,
                            display_mouse_event.y) == dir)
            {
                /* still pressing the same arrow: resume scrolling */
                *autoscroll = dir;
                return (dir < 0) ? KEY_UP : KEY_DOWN;
            }

            /* released or moved off the arrow: swallow the event */
            return KEY_MOUSE;
        }

        /* a genuine key press ends the repeat and is handled normally */
        ungetch(key);
    }

    int key = display_getch(dwin->window);

    if (key == KEY_MOUSE)
    {
        /* the mouse wheel scrolls one line per notch */
        if (display_mouse_event.bstate & BUTTON4_PRESSED)
            return KEY_UP;
        if (display_mouse_event.bstate & BUTTON5_PRESSED)
            return KEY_DOWN;

        /* a left click on a scroll arrow scrolls, and keeps scrolling
           while the button is held */
        if (display_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED))
        {
            int dir = display_window_arrow_at(dwin, display_mouse_event.x,
                    display_mouse_event.y);
            if (dir != 0)
            {
                /* a held button auto-repeats; a completed click (press
                   and release already merged by the backend) scrolls
                   once */
                if (display_mouse_event.bstate & BUTTON1_PRESSED)
                    *autoscroll = dir;

                return (dir < 0) ? KEY_UP : KEY_DOWN;
            }
        }
    }

    return key;
}

#ifdef SDLPDCURSES
void display_toggle_fullscreen(bool toggle)
{
    if (toggle)
        config.fullscreen = !config.fullscreen;

    int fullscreen = config.fullscreen
        ? SDL_WINDOW_FULLSCREEN_DESKTOP
        : false;

    SDL_SetWindowFullscreen(pdc_window, fullscreen);

    /* adapt and resize PDCurses internal structures */
    pdc_screen = SDL_GetWindowSurface(pdc_window);
    pdc_sheight = pdc_screen->h - pdc_xoffset;
    pdc_swidth = pdc_screen->w - pdc_yoffset;
    resize_term(PDC_get_rows(), PDC_get_columns());
}

void display_change_font()
{
    /* clearing ensures there are no fragments left after redraw */
    clear();
    pdc_font_size = config.font_size;

    TTF_CloseFont(pdc_ttffont);
    pdc_ttffont = TTF_OpenFont(font_name, pdc_font_size);

    /* adapt PDCurses internal variables */
    TTF_SizeText(pdc_ttffont, "W", &pdc_fwidth, &pdc_fheight);
    pdc_fthick = pdc_font_size / 20 + 1;

    /* ensure the window has the required minimum size  */
    resize_term(
        max(DEFAULT_ROWS, PDC_get_rows()),
        max(DEFAULT_COLS, PDC_get_columns())
    );

    /* redraw the screen as all dimensions have changed */
    display_paint_screen(nlarn->p);
}
#endif

static attr_t mvwcprintw(WINDOW *win, attr_t defattr, attr_t currattr,
                         const colour_t bg, int y, int x, const char *fmt, ...)
{
    /* assemble the message */
    va_list argp;

    va_start(argp, fmt);
    gchar *msg = g_strdup_vprintf(fmt, argp);
    va_end(argp);

    /* move to the starting position */
    wmove(win, y, x);

    /* restore the previously used attribute or use the default */
    attr_t attr = (currattr == COLOURLESS) ? defattr : currattr;

    const char *pos = msg;
    while (*pos != '\0')
    {
        /* parse tags */
        if (*pos == '`')
        {
            /* find the tag terminator; it must exist before
               the end of the string */
            const char *tend = strchr(pos + 1, '`');
            g_assert(tend != NULL);

            /* extract the tag value */
            char *tval = g_strndup(pos + 1, tend - pos - 1);

            /* find colour value for the tag content */
            if (strcmp(tval, "end") == 0)
                attr = defattr;
            else
                attr = COLOR_PAIR(colour_lookup(tval, bg));

            /* free temporary memory */
            g_free(tval);

            /* advance position over the end of the tag */
            pos = tend + 1;
            continue;
        }

        /* print the message character wise, as a character
           may be encoded as a multi-byte UTF-8 sequence */
        waddwach(win, g_utf8_get_char(pos), PAIR_NUMBER(attr), attr & ~A_COLOR);

        /* advance to the beginning of the next character */
        pos = g_utf8_next_char(pos);
    }

    /* clean assembled string */
    g_free(msg);

    /* return active attribute */
    return attr;
}

static void display_inventory_help(GPtrArray *callbacks)
{
    size_t maxlen = 0;
    GString *help = g_string_new(NULL);

    if (callbacks == NULL || callbacks->len == 0)
    {
        /* no callbacks available => select item with ENTER */
        g_string_append(help, _("Select the desired item with ENTER.\n"
                        "You may abort by pressing the escape key."));
    }
    else
    {
        /* determine the maximum length of the description */
        for (guint i = 0; i < callbacks->len; i++)
        {
            display_inv_callback *cb = g_ptr_array_index(callbacks, i);

            /* skip this callback function if it is not active */
            if (!cb->active) continue;

            char *sdesc = str_strip(cb->description);
            size_t desclen = g_utf8_strlen(sdesc, -1);
            if (desclen > maxlen)
                maxlen = desclen;
            g_free(sdesc);
        }

        if (maxlen == 0)
        {
            /* no active callbacks */
            g_string_append(help, _("There are no options available for the selected item."));
        }
        else
        {
            for (guint i = 0; i < callbacks->len; i++)
            {
                display_inv_callback *cb = g_ptr_array_index(callbacks, i);

                if (cb->active && cb->helpmsg != NULL)
                {
                    char *sdesc = str_strip(cb->description);
                    g_string_append_printf(help, "`TITLE`%*s`end`: %s\n",
                            (int)maxlen, sdesc, cb->helpmsg);
                    g_free(sdesc);
                }
            }
        }
    }

    display_show_message(_("Help"), help->str, maxlen + 2);
    g_string_free(help, true);
}

static display_window *display_window_new(int x1, int y1, int width,
                                          int height, const char *title)
{
    display_window *dwin = g_malloc0(sizeof(display_window));

    dwin->x1 = x1;
    dwin->y1 = y1;
    dwin->width = width;
    dwin->height = height;

    dwin->window = newwin(dwin->height, dwin->width, dwin->y1, dwin->x1);
    keypad(dwin->window, true);

    /* fill window background */
    wbkgd(dwin->window, CP_UI_FG);

    /* draw borders */
    wborder(
        dwin->window,
        ACS_VLINE | CP_UI_BORDER,
        ACS_VLINE | CP_UI_BORDER,
        ACS_HLINE | CP_UI_BORDER,
        ACS_HLINE | CP_UI_BORDER,
        ACS_ULCORNER | CP_UI_BORDER,
        ACS_URCORNER | CP_UI_BORDER,
        ACS_LLCORNER | CP_UI_BORDER,
        ACS_LRCORNER | CP_UI_BORDER
    );

    /* set the window title */
    display_window_update_title(dwin, title);

    /* create a panel for the window */
    dwin->panel = new_panel(dwin->window);

    /* add window to the list of opened windows */
    windows = g_list_append(windows, dwin);

    if (nlarn != NULL && nlarn->p != NULL)
    {
        /* repaint the screen if the game has been initialized */
        display_paint_screen(nlarn->p);
    }
    else
    {
        /* refresh panels */
        update_panels();
    }

    return dwin;
}

/* Move the window while the left mouse button, pressed on the
   window's title bar, is held down. Reacts on the mouse event stored
   by display_getch(); returns true when that event grabbed the title
   bar. */
static bool display_window_drag(display_window *dwin)
{
    const MEVENT *me = &display_mouse_event;

    /* only a left button press on the top border starts a drag */
    if (!(me->bstate & BUTTON1_PRESSED)
            || me->y != (int)dwin->y1
            || me->x < (int)dwin->x1
            || me->x >= (int)(dwin->x1 + dwin->width))
    {
        return false;
    }

    /* the grabbed spot shall stay under the pointer while dragging */
    const int grab_x = me->x - dwin->x1;

    int key;
    while ((key = display_getch(dwin->window)) != ERR)
    {
        if (key != KEY_MOUSE)
        {
            /* keyboard input ends the drag; leave the key for the
               window's own input handling */
            ungetch(key);
            break;
        }

        if (me->bstate & BUTTON1_RELEASED)
            break;

        /* clamp the new position to the screen */
        int nx = me->x - grab_x;
        int ny = me->y;
        nx = CLAMP(nx, 0, MAX(0, COLS - (int)dwin->width));
        ny = CLAMP(ny, 0, MAX(0, LINES - (int)dwin->height));

        if ((guint)nx != dwin->x1 || (guint)ny != dwin->y1)
        {
            dwin->x1 = nx;
            dwin->y1 = ny;
            move_panel(dwin->panel, dwin->y1, dwin->x1);
            display_draw();
        }
    }

    return true;
}

int display_window_move(display_window *dwin, int key)
{
    bool need_refresh = true;

    g_assert (dwin != NULL);

    switch (key)
    {
    case KEY_MOUSE:
        /* a mouse event is always consumed by the window system: it
           either drags the window or is ignored, but must never be
           treated as an unhandled key that closes the window */
        display_window_drag(dwin);
        break;

    case 0:
        /* The Windows keys generate two key presses, of which the first
           is a zero. Flush the buffer or the second key code will confuse
           everything. This happens here as all dialogue call this function
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
        if (dwin->x1 < (COLS - dwin->width)) dwin->x1++;
        break;

        /* ^up */
    case 562: /* NCurses - Linux */
    case 480: /* PDCurses - Windows */
        if (dwin->y1 > 0) dwin->y1--;
        break;

        /* ^down */
    case 521: /* NCurses - Linux */
    case 481: /* PDCurses - Windows */
        if (dwin->y1 < (LINES - dwin->height)) dwin->y1++;
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

    default:
        need_refresh = false;
    }

    if (need_refresh)
    {
        move_panel(dwin->panel, dwin->y1, dwin->x1);
        display_draw();
    }

    return need_refresh;
}

static void display_window_update_title(display_window *dwin, const char *title)
{
    g_assert (dwin != NULL && dwin->window != NULL);

    if (dwin->title)
    {
        /* free the previous title */
        g_free(dwin->title);

        /* repaint line to overwrite previous title */
        mvwhline(dwin->window, 0, 2, ACS_HLINE | CP_UI_BORDER,
            dwin->width - 7);
    }

    /* print the provided title */
    if (title && strlen(title))
    {
        /* copy the new title
         * the maximum length is determined by the window width
         * minus the space required for the left corner (3)
         * minus the space required for the right corner
         *       and the scroll marker (7)
         * truncate at a character boundary, not mid-sequence
         */
        const glong tmax = MIN(g_utf8_strlen(title, -1),
                               (glong)(dwin->width - 10));
        dwin->title = g_strndup(title,
                g_utf8_offset_to_pointer(title, tmax) - title);

        /* make sure the first letter of the window title is upper case */
        dwin->title[0] = g_ascii_toupper(dwin->title[0]);

        mvwaprintw(dwin->window, 0, 2, CP_UI_TITLE, " %s ", dwin->title);
    }

    wrefresh(dwin->window);
}

static void display_window_update_caption(display_window *dwin, char *caption)
{
    g_assert (dwin != NULL && dwin->window != NULL);

    /* repaint line to overwrite previous captions */
    mvwhline(dwin->window, dwin->height - 1, 3,
        ACS_HLINE | CP_UI_BORDER, dwin->width - 7);

    /* print caption if caption is set */
    if (caption && strlen(caption))
    {
        mvwcprintw(dwin->window, CP_UI_BRIGHT_FG, COLOURLESS, UI_BG,
                dwin->height - 1, 3, " %s ", caption);
    }

    if (caption)
    {
        /* free the provided caption */
        g_free(caption);
    }

    wrefresh(dwin->window);
}

static void display_window_update_arrow_up(display_window *dwin, bool on)
{
    g_assert (dwin != NULL && dwin->window != NULL);

    if (on)
    {
        mvwaprintw(dwin->window, 0, dwin->width - 5, CP_UI_BRIGHT_FG, " ^ ");
    }
    else
    {
        mvwhline(dwin->window, 0, dwin->width - 5,
            ACS_HLINE | CP_UI_BORDER, 3);
    }
}

static void display_window_update_arrow_down(display_window *dwin, bool on)
{
    g_assert (dwin != NULL && dwin->window != NULL);

    if (on)
    {
        mvwaprintw(dwin->window, dwin->height - 1, dwin->width - 5,
                  CP_UI_BRIGHT_FG, " v ");
    }
    else
    {
        mvwhline(dwin->window, dwin->height - 1, dwin->width - 5,
                  ACS_HLINE | CP_UI_BORDER, 3);
    }
}

static display_window *display_item_details(guint x1, guint y1, guint width,
                                            item *it, player *p, bool shop)
{
    /* determine if the item is known or displayed in the shop */
    const bool known = shop | player_item_known(p, it);

    /* the detailed item description */
    char *msg = item_detailed_description(it, known, shop);

    /* the pop-up window created by display_popup */
    display_window *idpop = display_popup(x1, y1, width, _("Item details"), msg, 0);

    /* tidy up */
    g_free(msg);

    return idpop;
}

static void display_spheres_paint(sphere *s, player *p)
{
    /* check if sphere is on current level */
    if (Z(s->pos) != Z(p->pos))
        return;

    if (game_fullvis(nlarn) || fov_get(p->fv, s->pos))
    {
        mvaddwach(Y(s->pos), X(s->pos), COLOR_PAIR(BRIGHT_MAGENTA), '0');
    }
}
