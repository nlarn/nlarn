/*
 * wrap_monsters.c
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
#include <stdlib.h>

#include "display.h"
#include "monsters.h"
#include "nlarn.h"

void wrap_monsters(lua_State *L)
{
    g_assert (L != NULL);

    /* monster flags */
    for (int mfs = 0; mfs <= MONSTER_FLAG_COUNT; mfs++)
    {
        /* shift with the current loop iteration to get the enum value */
        int mfv = 1 << mfs;
        lua_pushinteger(L, mfv);
        lua_setglobal(L, monster_flag_string(mfv));
    }

    /* monster types */
    for (monster_t mt = 0; mt < MT_MAX; mt++)
    {
        lua_pushinteger(L, mt);
        lua_setglobal(L, monster_t_string(mt));
    }

    /* read monster data */
    gchar *filename = g_strdup_printf("%s%c%s", nlarn->libdir,
                                      G_DIR_SEPARATOR, "monsters.lua");

    if (luaL_dofile(L, filename) == 1)
    {
        display_shutdown();
        g_printerr("Failed to load monster data: %s\n",
                   lua_tostring(L, -1));

        exit(EXIT_FAILURE);
    }
    g_free(filename);

}
