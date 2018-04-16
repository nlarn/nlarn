/*
 * wrap_display.c
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

#include <glib.h>
#include <lua.h>
#include <lauxlib.h>

#include "display.h"
#include "nlarn.h"

static int wrap_draw(lua_State *L);
static int wrap_get_count(lua_State *L);
static int wrap_get_yesno(lua_State *L);
static int wrap_paint(lua_State *L);

static const luaL_Reg display_functions[] =
{
    { "draw",       wrap_draw },
    { "get_count",  wrap_get_count },
    { "get_yesno",  wrap_get_yesno },
    { "paint",      wrap_paint },
    { 0, 0 }
};

void wrap_display(lua_State *L)
{
    g_assert (L != NULL);

    struct
    {
        const char *name;
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

    for (int i = 0; constants[i].name != NULL; i++)
    {
        lua_pushinteger(L, constants[i].value);
        lua_setglobal(L, constants[i].name);
    }

    for (int i = 0; display_functions[i].name != NULL; i++)
    {
        lua_register(L, display_functions[i].name, display_functions[i].func);
    }
}

static int wrap_draw(lua_State *L __attribute__((unused)))
{
    display_draw();

    return 0;
}

static int wrap_get_count(lua_State *L)
{
    int res = 0;

    res = display_get_count(luaL_checkstring(L, 1), luaL_checkinteger(L, 2));
    lua_pushinteger(L, res);

    return 1;
}

static int wrap_get_yesno(lua_State *L)
{
    int res = 0;
    int nargs = lua_gettop(L);

    res = display_get_yesno(luaL_checkstring(L, 1),
                            nargs > 1 ? luaL_checkstring(L, 2) : NULL,
                            nargs > 2 ? luaL_checkstring(L, 3) : NULL);

    lua_pushboolean(L, res);

    return 1;
}

static int wrap_paint(lua_State *L __attribute__((unused)))
{
    g_assert(nlarn != NULL);

    display_paint_screen(nlarn->p);
    return 0;
}
