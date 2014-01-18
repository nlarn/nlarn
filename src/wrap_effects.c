/*
 * wrap_effects.c
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
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

/* $Id$ */

#include <glib.h>
#include <lua.h>
#include <lauxlib.h>

#include "effects.h"

static const char EFFECT[] = "effect";

static effect *check_effect(lua_State *L, int index);
static void push_effect(lua_State *L, effect *eff);
static int wrap_new(lua_State *L);
static int wrap_destroy(lua_State *L);
static int wrap_tostring(lua_State *L);
static int wrap_desc(lua_State *L);
static int wrap_msg_start(lua_State *L);
static int wrap_msg_stop(lua_State *L);
static int wrap_msg_m_start(lua_State *L);
static int wrap_msg_m_stop(lua_State *L);
static int wrap_amount(lua_State *L);

#if LUA_VERSION_NUM > 501
static const luaL_Reg effect_methods[] =
#else
static const luaL_reg effect_methods[] =
#endif
{
    { "new",         wrap_new },
    { "desc",        wrap_desc },
    { "msg_start",   wrap_msg_start },
    { "msg_stop",    wrap_msg_stop },
    { "msg_m_start", wrap_msg_m_start },
    { "msg_m_stop",  wrap_msg_m_stop },
    { "amount",      wrap_amount },
    { 0, 0 }
};

#if LUA_VERSION_NUM > 501
static const luaL_Reg effect_metamethods[] =
#else
static const luaL_reg effect_metamethods[] =
#endif
{
    { "__gc",        wrap_destroy },
    { "__tostring",  wrap_tostring },
    { 0, 0 }
};

void wrap_effects(lua_State *L)
{
    g_assert (L != NULL);

    /* create methods table, add it to the globals */
#if LUA_VERSION_NUM > 501
    lua_newtable(L);
    luaL_setfuncs(L,effect_methods,0);
    lua_pushvalue(L,-1);
    lua_setglobal(L,EFFECT);
#else
    luaL_register(L, EFFECT, effect_methods);
#endif

    /* create metatable for Image, add it to the Lua registry */
    luaL_newmetatable(L, EFFECT);

    /* fill metatable */
#if LUA_VERSION_NUM > 501
    lua_newtable(L);
    luaL_setfuncs(L, effect_metamethods, 0);
    lua_setglobal(L,"__index");
#else
    luaL_openlib(L, 0, effect_metamethods, 0);
    lua_pushliteral(L, "__index");
#endif
    /* dup methods table*/
    lua_pushvalue(L, -3);

    /* metatable.__index = methods */
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");

    /* dup methods table*/
    lua_pushvalue(L, -3);

    /* hide metatable: metatable.__metatable = methods */
    lua_rawset(L, -3);

    /* drop metatable */
    lua_pop(L, 1);

    /* register constants */
    for (effect_t type = ET_NONE; type < ET_MAX; type++)
    {
        lua_pushinteger(L, type);
        lua_setglobal(L, effect_type_name(type));
    }
}

static effect *check_effect(lua_State *L, int index)
{
    luaL_checkudata(L, index, EFFECT);

    return (effect *)lua_topointer(L, index);
}

static void push_effect(lua_State *L, effect *eff)
{
    /* push the pointer to the effect to Lua */
    lua_pushlightuserdata(L, eff);

    /* attach the meta table to the effect */
    luaL_getmetatable(L, EFFECT);
    lua_setmetatable(L, -2);
}

static int wrap_new(lua_State *L)
{
    effect *e;

    e = effect_new(luaL_checkint(L, 1));
    push_effect(L, e);

    return 1;
}

static int wrap_destroy(lua_State *L)
{
    effect *e = check_effect(L, 1);
    effect_destroy(e);

    return 0;
}

static int wrap_tostring(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushfstring(L, "effect: %s (s: %d - t: %d - a: %d)",
                    effect_type_name(e->type), e->start, e->turns, e->amount);

    return 1;
}

static int wrap_desc(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushstring(L, effect_get_desc(e));

    return 1;
}

static int wrap_msg_start(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushstring(L, effect_get_msg_start(e));

    return 1;
}

static int wrap_msg_stop(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushstring(L, effect_get_msg_stop(e));

    return 1;
}

static int wrap_msg_m_start(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushstring(L, effect_get_msg_m_start(e));

    return 1;
}

static int wrap_msg_m_stop(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushstring(L, effect_get_msg_m_stop(e));

    return 1;
}

static int wrap_amount(lua_State *L)
{
    effect *e = check_effect(L, 1);
    lua_pushinteger(L, effect_get_amount(e));

    return 1;
}
