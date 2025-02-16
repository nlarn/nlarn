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

#include <string.h>

#include "colours.h"
#include "enumFactory.h"

DEFINE_ENUM(colour, COLOUR_ENUM)

// Colour pair initialisation
void colours_init() {

    for (int fg = 0; fg < 256; fg++) {
        switch(fg) {
            // Init UI colors
            case UI_BRIGHT_FG:
                init_pair(UI_BRIGHT_FG, WHITE, UI_BG);
                break;
            case UI_FG:
                init_pair(UI_FG, SILVER, UI_BG);
                break;
            case UI_BORDER:
                init_pair(UI_BORDER, LED_BLUE, UI_BG);
                break;
            case UI_YELLOW:
                init_pair(UI_YELLOW, DYNAMIC_YELLOW, UI_BG);
                break;
            case UI_GREEN:
                init_pair(UI_GREEN, VIVID_GREEN, UI_BG);
                break;
            case UI_FG_REVERSE:
                init_pair(UI_FG_REVERSE, UI_BG, PLATINUM);
                break;
            case GREY_WHITE:
                init_pair(GREY_WHITE, GRANITE, WHITE);
                break;
            case BLACK_WHITE:
                init_pair(BLACK_WHITE, BLACK, WHITE);
                break;
            default:
                // Init play field colors
                init_pair(fg, fg, PF_BG);
        }
    }
}

int colour_lookup(const char *colour_name, int bg) {
    if (bg == UI_BG) {
        if (!strcmp(colour_name, "WHITE"))  return UI_BRIGHT_FG;
        if (!strcmp(colour_name, "BLUE"))   return UI_BORDER;
        if (!strcmp(colour_name, "YELLOW")) return UI_YELLOW;
        if (!strcmp(colour_name, "GREEN"))  return UI_GREEN;
    } else if (bg == PF_BG) {
        return colour_value(colour_name);
    }

    return 0;
}
