/*
 * colours.h
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

#ifndef __COLOURS_H_
#define __COLOURS_H_

#include <curses.h>

typedef struct _colset
{
    const char *name;
    const int  val;
} colset;

/* colour pairs */
enum colour_pairs
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

enum default_colours
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

enum dialog_colours
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

int colour_lookup(const colset *colset, const char *name);

#endif
