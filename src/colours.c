/*
 * colours.c
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

#include <curses.h>
#include <string.h>

#include "colours.h"
#include "enumFactory.h"

DEFINE_ENUM(colour, COLOUR_ENUM)
DEFINE_ENUM(ui_colour_scheme, UI_COLOUR_SCHEME_ENUM)

// Colour pair initialisation
void colours_init(int ui_colour_scheme) {
    // redefine UI colours

    switch(ui_colour_scheme) {
        // vim retrobox
        case RETROBOX:
            init_color(UI_BG, 110, 110, 110);
            init_color(UI_BRIGHT_FG, 920, 860, 700);
            init_color(UI_FG, 570, 510, 460);
            init_color(UI_BORDER, 900, 680, 180);
            init_color(UI_TITLE, 550, 560, 140);
            init_color(UI_KEY, 490, 660, 430);
        break;

        // vim stasis
        case STASIS:
            init_color(UI_BG, 184, 173, 200);
            init_color(UI_BRIGHT_FG, 900, 1000, 1000);
            init_color(UI_FG, 600, 700, 700);
            init_color(UI_BORDER, 360, 320, 400);
            init_color(UI_TITLE, 820, 720, 120);
            init_color(UI_KEY, 490, 660, 430);
        break;

        // vim sourlick
        case SOURLICK:
            init_color(UI_BG, 180, 173, 169);
            init_color(UI_BRIGHT_FG, 898, 761, 702);
            init_color(UI_FG, 920, 920, 920);
            init_color(UI_BORDER, 231, 216, 212);
            init_color(UI_TITLE, 863, 878, 320);
            init_color(UI_KEY, 663, 851, 137);
        break;

        // vim fodder
        case FODDER:
            init_color(UI_BG, 176, 255, 216);
            init_color(UI_BRIGHT_FG, 588, 808, 271);
            init_color(UI_FG, 369, 541, 541);
            init_color(UI_BORDER, 137, 196, 165);
            init_color(UI_TITLE, 859, 580, 118);
            init_color(UI_KEY, 678, 596, 584);
        break;

        // vim mellow
        case MELLOW:
            init_color(UI_BG, 212, 192, 173);
            init_color(UI_BRIGHT_FG, 949, 737, 475);
            init_color(UI_FG, 973, 973, 949);
            init_color(UI_BORDER, 467, 431, 392);
            init_color(UI_TITLE, 894, 675, 216);
            init_color(UI_KEY, 557, 353, 298);
        break;

        // traditional NLarn colours
        case TRADITIONAL:
        default:
            init_color(UI_BG, 800, 0, 0);
            init_color(UI_BRIGHT_FG, 1000, 1000, 1000);
            init_color(UI_FG, 850, 850, 850);
            init_color(UI_BORDER, 118, 565, 1000);
            init_color(UI_TITLE, 992, 980, 365);
            init_color(UI_KEY, 0, 843, 0);
        break;
    }

    for (int fg = 0; fg < 256; fg++) {
        switch(fg) {
            // Init UI colors
            case UI_BRIGHT_FG:
                init_pair(UI_BRIGHT_FG, UI_BRIGHT_FG, UI_BG);
                break;
            case UI_FG:
                init_pair(UI_FG, UI_FG, UI_BG);
                break;
            case UI_BORDER:
                init_pair(UI_BORDER, UI_BORDER, UI_BG);
                break;
            case UI_TITLE:
                init_pair(UI_TITLE, UI_TITLE, UI_BG);
                break;
            case UI_KEY:
                init_pair(UI_KEY, UI_KEY, UI_BG);
                break;
            case UI_FG_REVERSE:
                init_pair(UI_FG_REVERSE, UI_BG, UI_FG);
                break;
            case UI_HL_REVERSE:
                init_pair(UI_HL_REVERSE, UI_BG, UI_BRIGHT_FG);
                break;
            default:
                // Init play field colors
                init_pair(fg, fg, PF_BG);
        }
    }
}

int colour_lookup(const char *colour_name, int bg) {
    if (bg == UI_BG) {
        if (!strcmp(colour_name, "EMPH"))  return UI_BRIGHT_FG;
        if (!strcmp(colour_name, "TITLE")) return UI_TITLE;
        if (!strcmp(colour_name, "KEY"))   return UI_KEY;
    } else if (bg == PF_BG) {
        return colour_value(colour_name);
    }

    return 0;
}
