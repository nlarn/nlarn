/*
 * display.h
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

#ifndef DISPLAY_H
#define DISPLAY_H

#include <curses.h>
#include <panel.h>
#include <glib.h>

#include "items.h"
#include "player.h"

/* missing key definitions */
#define KEY_BS   8 /* backspace */
#define KEY_TAB  9 /* tab */
#define KEY_LF  10 /* line feed, i.e. enter */
#define KEY_CR  13 /* carriage return */
#define KEY_ESC 27 /* escape */
#define KEY_SPC 32 /* space */

typedef void (*display_inv_callback_func)(player *, inventory **, item *);

typedef struct display_inv_callback
{
    const char *description;
    const char *helpmsg;
    char key;
    inventory **inv;
    display_inv_callback_func function;
    int (*checkfun)(player *, item *);
    gboolean active;
} display_inv_callback;

typedef struct display_window
{
    guint x1;
    guint y1;
    guint width;
    guint height;
    char *title;
    char *caption;
    WINDOW *window;
    PANEL *panel;
} display_window;

/* information required to draw a map cell */
typedef struct display_cell
{
    wchar_t glyph;
    int colour;
    gboolean reversed;
} display_cell;

/* function declarations */

void display_init();
void display_shutdown();

/**
 * @brief Check if the display system has been initialised.
 * @return true or false
 */
gboolean display_available();

/**
 * Repaint the screen.
  */
void display_draw();

void display_paint_screen(player *p);

/**
 * Generic inventory display function
 *
 * @param title Window title
 * @param p player
 * @param inv inventory to display
 * @param callbacks a GPtrArray of display_inv_callbacks (may be NULL)
 * @param show_price display prices
 * @param show_weight display item weight
 * @param show_account display bank account balance in title
 * @param filter a filter function: will be called for every item
 * @return if no callbacks have been supplied, the selected item will be returned on pressing enter
 *
 */
item *display_inventory(const char *title, player *p, inventory **inv,
                        GPtrArray *callbacks, gboolean show_price,
                        gboolean show_weight, gboolean show_account,
                        int (*filter)(item *));

void display_inv_callbacks_clean(GPtrArray *callbacks);

void display_config_autopickup(gboolean settings[IT_MAX]);

spell *display_spell_select(const char *title, player *p);

int display_get_count(const char *caption, int value);
char *display_get_string(const char *title, const char *caption, const char *value, size_t max_len);
int display_get_yesno(const char *question, const char *title, const char *yes, const char *no);
direction display_get_direction(const char *title, int *available);

position display_get_new_position(player *p,
                                  position start,
                                  const char *message,
                                  gboolean ray,
                                  gboolean ball,
                                  gboolean travel,
                                  guint radius,
                                  gboolean passable,
                                  gboolean visible);

position display_get_position(player *p,
                              const char *message,
                              gboolean ray,
                              gboolean ball,
                              guint radius,
                              gboolean passable,
                              gboolean visible);

void display_show_history(message_log *log, const char *title);

/**
 * Simple "popup" message window
 *
 * @param title window title
 * @param message message to be displayed inside window
 * @param indent the number of chars wrapped lines will be indented
 * @return key pressed to close window
 */
int display_show_message(const char *title, const char *message, int indent);

/**
 * @brief Show a popup message.
 *
 * @param x1 The top left x position.
 * @param y1 The top left y position.
 * @param width The width of the popup. Determined automatically if 0.
 * @param title The popup window title. May be NULL.
 * @param msg The popup window message. The message will be wrapped to fit the
 *        window's width. If the message is too long to fit the screen, it
 *        will be truncated.
 * @param indent Indentation for wrapped lines
 *
 * @return A pointer to the newly allocated window structure.
 */
display_window *display_popup(int x1, int y1, int width, const char *title,
    const char *msg, int indent);

/**
 * @brief Destroy a window and the resources allocated for it.
 * @param dwin A pointer to a window structure.
 */
void display_window_destroy(display_window *dwin);

/**
 * @brief Hide all windows.
 */
void display_windows_hide();

/**
 * @brief Set all windows visible.
 */
void display_windows_show();


/**
 * @brief Wrap curses wgetch() with added functionality
 * @param win A pointer to a window structure. May be NULL.
              In this case stdscr is used.
 */
int display_getch(WINDOW *win);

#ifdef SDLPDCURSES
/* toggle SDL fullscreen mode */
void display_toggle_fullscreen(gboolean toggle);

/* replace font at runtime */
void display_change_font();
#endif

#endif
