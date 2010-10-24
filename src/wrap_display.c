/*
 * wrap_display.c
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

#include <lua.h>
#include <lauxlib.h>

#include "display.h"

void wrap_display(lua_State *L)
{
    int i;

    assert (L != NULL);

    struct
    {
        char *name;
        int value;
    } constants[] =
    {
        /* color definitions  */
        { "BLACK",         DC_BLACK },
        { "RED",           DC_RED },
        { "GREEN",         DC_GREEN },
        { "BROWN",         DC_BROWN },
        { "BLUE",          DC_BLUE },
        { "MAGENTA",       DC_MAGENTA },
        { "CYAN",          DC_CYAN },
        { "LIGHTGRAY",     DC_LIGHTGRAY },
        { "DARKGRAY",      DC_DARKGRAY },
        { "LIGHTRED",      DC_LIGHTRED },
        { "LIGHTGREEN",    DC_LIGHTGREEN },
        { "YELLOW",        DC_YELLOW },
        { "LIGHTBLUE",     DC_LIGHTBLUE },
        { "LIGHTMAGENTA",  DC_LIGHTMAGENTA },
        { "LIGHTCYAN",     DC_LIGHTCYAN },
        { "WHITE",         DC_WHITE },

        { NULL, 0 },
    };

    for (i = 0; constants[i].name != NULL; i++)
    {
        lua_pushinteger(L, constants[i].value);
        lua_setglobal(L, constants[i].name);
    }
}
