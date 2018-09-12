/*
 * wrap_utils.c
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

#include "defines.h"
#include "nlarn.h"
#include "random.h"

/* local functions */
static int wrap_log(lua_State *L);
static int wrap_rand(lua_State *L);
static int wrap_chance(lua_State *L);
static gboolean luaN_data_query(const char *table, guint idx, const char *attrib);

void wrap_utils(lua_State *L)
{
    g_assert (L != NULL);

    /* register constants */
    struct
    {
        const char *name;
        int value;
    } constants[] =
    {
        /* speed definitions */
        { "XSLOW",  SPEED_XSLOW },
        { "VSLOW",  SPEED_VSLOW },
        { "SLOW",   SPEED_SLOW },
        { "NORMAL", SPEED_NORMAL },
        { "FAST",   SPEED_FAST },
        { "VFAST",  SPEED_VFAST },
        { "XFAST",  SPEED_XFAST },
        { "DOUBLE", SPEED_DOUBLE },

        /*  size definitions */
        { "TINY",       ESIZE_TINY },
        { "SMALL",      ESIZE_SMALL },
        { "MEDIUM",     ESIZE_MEDIUM },
        { "LARGE",      ESIZE_LARGE },
        { "HUGE",       ESIZE_HUGE },
        { "GARGANTUAN", ESIZE_GARGANTUAN },

        /* attack types */
        { "WEAPON", ATT_WEAPON },
        { "MAGIC",  ATT_MAGIC },
        { "CLAW",   ATT_CLAW },
        { "BITE",   ATT_BITE },
        { "STING",  ATT_STING },
        { "SLAM",   ATT_SLAM },
        { "KICK",   ATT_KICK },
        { "TOUCH",  ATT_TOUCH },
        { "BREATH", ATT_BREATH },
        { "GAZE",   ATT_GAZE },

        /*  damage types */
        { "PHYSICAL",    DAM_PHYSICAL },
        { "MAGICAL",     DAM_MAGICAL },
        { "FIRE",        DAM_FIRE },
        { "COLD",        DAM_COLD },
        { "ACID",        DAM_ACID },
        { "WATER",       DAM_WATER },
        { "ELECTRICITY", DAM_ELECTRICITY },
        { "POISON",      DAM_POISON },
        { "BLINDNESS",   DAM_BLINDNESS },
        { "CONFUSION",   DAM_CONFUSION },
        { "PARALYSIS",   DAM_PARALYSIS },
        { "DEC_CON",     DAM_DEC_CON },
        { "DEC_DEX",     DAM_DEC_DEX },
        { "DEC_INT",     DAM_DEC_INT },
        { "DEC_STR",     DAM_DEC_STR },
        { "DEC_WIS",     DAM_DEC_WIS },
        { "DEC_RND",     DAM_DEC_RND },
        { "DRAIN_LIFE",  DAM_DRAIN_LIFE },
        { "STEAL_GOLD",  DAM_STEAL_GOLD },
        { "STEAL_ITEM",  DAM_STEAL_ITEM },
        { "RUST",        DAM_RUST },
        { "REM_ENCH",    DAM_REM_ENCH },
        { "RANDOM",      DAM_RANDOM },

        { NULL, 0 },
    };

    for (int i = 0; constants[i].name != NULL; i++)
    {
        lua_pushinteger(L, constants[i].value);
        lua_setglobal(L, constants[i].name);
    }

    /* register functions */
    lua_register(L, "log", wrap_log);
    lua_register(L, "rand", wrap_rand);
    lua_register(L, "chance", wrap_chance);
}

const char *luaN_query_string(const char *table, guint idx, const char *attrib)
{
    const char *name = NULL;

    if (luaN_data_query(table, idx, attrib))
    {
        name = luaL_checkstring(nlarn->L, -1);
        lua_pop(nlarn->L, 3);
    }

    return name;
}

char luaN_query_char(const char *table, guint idx, const char *attrib)
{
    const char *str = NULL;

    if (luaN_data_query(table, idx, attrib))
    {
        str = luaL_checkstring(nlarn->L, -1);
        lua_pop(nlarn->L, 3);
    }

    return (str != NULL) ? str[0] : 0;
}

int luaN_query_int(const char *table, guint idx, const char *attrib)
{
    int val = 0;

    if (luaN_data_query(table, idx, attrib))
    {
        val = luaL_checkinteger(nlarn->L, -1);
        lua_pop(nlarn->L, 3);
    }

    return val;
}

int luaN_push_table(const char *table, guint idx, const char *tname)
{
    if (!luaN_data_query(table, idx, tname))
        return FALSE;

    if (!lua_istable(nlarn->L, -1))
    {
        lua_pop(nlarn->L, 3);
        return FALSE;
    }

    return TRUE;
}

static int wrap_log(lua_State *L)
{
    int nargs = lua_gettop(L);    /* number of arguments */

    for (int i = 1; i <= nargs; i++)
    {
        log_add_entry(nlarn->log, "%s", luaL_checkstring(L, i));
    }

    return 0;
}

static int wrap_rand(lua_State *L)
{
    int nargs = lua_gettop(L);
    int result;

    if (nargs == 2)
    {
        result = rand_m_n(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
    }
    else
    {
        result = rand_0n(luaL_checkinteger(L, 1));
    }

    lua_pushinteger(L, result);

    return 1;
}

static int wrap_chance(lua_State *L)
{
    lua_pushboolean(L, chance(luaL_checkinteger(L, 1)));
    return 1;
}

static gboolean luaN_data_query(const char *table, guint idx, const char *attrib)
{
    lua_getglobal(nlarn->L, table);
    if (!lua_istable(nlarn->L, -1))
    {
        lua_pop(nlarn->L, 1);
        return FALSE;
    }

    lua_rawgeti(nlarn->L, -1, idx);
    if (!lua_istable(nlarn->L, -1))
    {
        lua_pop(nlarn->L, 2);
        return FALSE;
    }

    lua_getfield(nlarn->L, -1, attrib);
    if (lua_isnil(nlarn->L, -1))
    {
        lua_pop(nlarn->L, 3);
        return FALSE;
    }

    return TRUE;
}
