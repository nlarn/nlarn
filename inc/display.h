/*
 * display.h
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

enum display_colours
{
    DC_NONE,
    DC_WHITE,
    DC_RED,
    DC_GREEN,
    DC_BLUE,
    DC_YELLOW,
    DC_MAGENTA,
    DC_CYAN,
    DC_BLACK,
    DC_MAX
};

typedef struct display_inv_callback
{
    char *description;
    char key;
    int (*function)(player *, item *);
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

int display_init();
void display_shutdown();
int display_draw();

int display_paint_screen(player *p);

void display_inventory(char *title, player *p, inventory *inv,
                       GPtrArray *callbacks, int show_price,
                       int (*filter)(item *));

void display_inv_callbacks_clean(GPtrArray *callbacks);

void display_config_autopickup(player *p);

spell *display_spell_select(char *title, player *p);

int display_get_count(char *caption, int value);
char *display_get_string(char *caption, char *value, size_t max_len);
int display_get_yesno(char *question, char *yes, char *no);
direction display_get_direction(char *title, int *available);
position display_get_position(player *p, char *message, int draw_line, int passable);

void display_show_history(message_log *log, char *title);
char display_show_message(char *title, char *message);

#define display_getch getch


#endif
