/*
 * lua_wrappers.h
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

#ifndef __LUA_WRAPPERS_H_
#define __LUA_WRAPPERS_H_

#include <lua.h>

/* functions implemented in wrap_display.c */
void wrap_display(lua_State *L);

/* functions implemented in wrap_monsters.c */
void wrap_monsters(lua_State *L);

/* functions implemented in wrap_utils.c */
void wrap_utils(lua_State *L);
const char *luaN_query_string(const char *table, guint idx, const char *attrib);
char luaN_query_char(const char *table, guint idx, const char *attrib);
int luaN_query_int(const char *table, guint idx, const char *attrib);
int luaN_push_table(const char *table, guint idx, const char *tname);

#endif
