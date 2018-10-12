/*
 * display.h
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <curses.h>
#include <panel.h>

#include "game.h"
#include "items.h"
#include "player.h"


/* missing key definitions */
#define KEY_BS   8 /* backspace */
#define KEY_TAB  9 /* tab */
#define KEY_LF  10 /* line feed, i.e. enter */
#define KEY_CR  13 /* carriage return */
#define KEY_ESC 27 /* escape */
#define KEY_SPC 32 /* space */

/* colour pairs */
enum display_colour_pairs
{
    DCP_WHITE_BLACK,
    DCP_RED_BLACK,
    DCP_GREEN_BLACK,
    DCP_BLUE_BLACK,
    DCP_YELLOW_BLACK,
    DCP_MAGENTA_BLACK,
    DCP_CYAN_BLACK,
    DCP_BLACK_BLACK,

    DCP_WHITE_RED,
    DCP_RED_RED,
    DCP_GREEN_RED,
    DCP_BLUE_RED,
    DCP_YELLOW_RED,
    DCP_MAGENTA_RED,
    DCP_CYAN_RED,
    DCP_BLACK_RED,

    DCP_RED_WHITE,
    DCP_BLACK_WHITE
};

enum display_default_colours
{
    COLOURLESS,
    BLACK        = COLOR_PAIR(DCP_BLACK_BLACK),
    RED          = COLOR_PAIR(DCP_RED_BLACK),
    GREEN        = COLOR_PAIR(DCP_GREEN_BLACK),
    BROWN        = COLOR_PAIR(DCP_YELLOW_BLACK),
    BLUE         = COLOR_PAIR(DCP_BLUE_BLACK),
    MAGENTA      = COLOR_PAIR(DCP_MAGENTA_BLACK),
    CYAN         = COLOR_PAIR(DCP_CYAN_BLACK),
    LIGHTGRAY    = COLOR_PAIR(DCP_WHITE_BLACK),
    DARKGRAY     = COLOR_PAIR(DCP_BLACK_BLACK)   | A_BOLD,
    LIGHTRED     = COLOR_PAIR(DCP_RED_BLACK)     | A_BOLD,
    LIGHTGREEN   = COLOR_PAIR(DCP_GREEN_BLACK)   | A_BOLD,
    YELLOW       = COLOR_PAIR(DCP_YELLOW_BLACK)  | A_BOLD,
    LIGHTBLUE    = COLOR_PAIR(DCP_BLUE_BLACK)    | A_BOLD,
    LIGHTMAGENTA = COLOR_PAIR(DCP_MAGENTA_BLACK) | A_BOLD,
    LIGHTCYAN    = COLOR_PAIR(DCP_CYAN_BLACK)    | A_BOLD,
    WHITE        = COLOR_PAIR(DCP_WHITE_BLACK)   | A_BOLD
};

enum display_dialog_colours
{
    DDC_BLACK        = COLOR_PAIR(DCP_BLACK_RED),
    DDC_RED          = COLOR_PAIR(DCP_RED_RED),
    DDC_GREEN        = COLOR_PAIR(DCP_GREEN_RED),
    DDC_BROWN        = COLOR_PAIR(DCP_YELLOW_RED),
    DDC_BLUE         = COLOR_PAIR(DCP_BLUE_RED),
    DDC_MAGENTA      = COLOR_PAIR(DCP_MAGENTA_RED),
    DDC_CYAN         = COLOR_PAIR(DCP_CYAN_RED),
    DDC_LIGHTGRAY    = COLOR_PAIR(DCP_WHITE_RED),
    DDC_DARKGRAY     = COLOR_PAIR(DCP_BLACK_RED)   | A_BOLD,
    DDC_LIGHTRED     = COLOR_PAIR(DCP_RED_RED)     | A_BOLD,
    DDC_LIGHTGREEN   = COLOR_PAIR(DCP_GREEN_RED)   | A_BOLD,
    DDC_YELLOW       = COLOR_PAIR(DCP_YELLOW_RED)  | A_BOLD,
    DDC_LIGHTBLUE    = COLOR_PAIR(DCP_BLUE_RED)    | A_BOLD,
    DDC_LIGHTMAGENTA = COLOR_PAIR(DCP_MAGENTA_RED) | A_BOLD,
    DDC_LIGHTCYAN    = COLOR_PAIR(DCP_CYAN_RED)    | A_BOLD,
    DDC_WHITE        = COLOR_PAIR(DCP_WHITE_RED)   | A_BOLD
};
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

/* function declarations */

void display_init();
void display_shutdown();

/**
 * @brief Check if the display system has been initialised.
 * @return TRUE or FALSE
 */
gboolean display_available();

/**
 * Repaint the screen.
  */
int display_draw();

int display_paint_screen(player *p);

/**
 * Generic inventory display function
 *
 * @param Window title
 * @param player
 * @param inventory to display
 * @param a GPtrArray of display_inv_callbacks (may be NULL)
 * @param display prices
 * @param a filter function: will be called for every item
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
 * @param window title
 * @param message to be displayed inside window
 * @param the number of chars wrapped lines will be indented
 * @return key pressed to close window
 */
int display_show_message(const char *title, const char *message, int indent);

/**
 * @brief Show a popup message.
 *
 * @param The top left x position.
 * @param The top left y position.
 * @param The width of the popup. Determined automatically if 0.
 * @param The popup window title. May be NULL.
 * @param The popup window message. The message will be wrapped to fit the
 *        window's width. If the message is too long to fit the screen, it
 *        will be truncated.
 *
 * @return A pointer to the newly allocated window structure.
 */
display_window *display_popup(int x1, int y1, int width, const char *title, const char *msg);

/**
 * @brief Destroy a window and the resources allocated for it.
 * @param A pointer to a window structure.
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

#define display_getch getch


#endif
