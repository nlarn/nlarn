/*
 * utils.c
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

#include <assert.h>
#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

#include "nlarn.h"
#include "utils.h"

static void log_entry_destroy(message_log_entry *entry);

static gboolean luaN_data_query(const char *table, guint idx, const char *attrib);
static int wrap_log(lua_State *L);
static int wrap_rand(lua_State *L);
static int wrap_chance(lua_State *L);

int divert(int value, int percent)
{
    int lower, upper;

    assert(value > 0 && percent > 0);

    lower = value - (value / percent);
    upper = value + (value / percent);
    if (lower == upper)
        return value;

    return rand_m_n(lower, upper);
}

void shuffle(int array[], int length, int skip)
{
    int i;
    int npos;
    int temp;

    for (i = 0; i < length; i++)
    {
        /* fill the array in order */
        array[i] = i;
    }

    for (i = skip; i < (length / 2); i++)
    {
        /* randomize positions */
        npos = i + g_random_int_range(0, length - i);
        temp = array[i];
        array[i] = array[npos];
        array[npos] = temp;
    }
}

char *str_replace(char *string, char *orig, char *replace)
{
    static char buffer[1024];
    char *ch;

    if (!(ch = strstr(string, orig)))
        return string;

    strncpy(buffer, string, ch - string);
    buffer[ch - string] = 0;
    sprintf(buffer + (ch - string),
            "%s%s",
            replace,
            ch + strlen(orig));

    return buffer;
}

char *str_capitalize(char *string)
{
    int i;

    if (string == NULL)
    {
        return NULL;
    }

    for (i = 0; i < strlen(string); ++i)
    {
        if (i == 0 || string[i - 1] == ' ')
        {
            string[i] = g_ascii_toupper(string[i]);
        }
    }

    return string;
}

message_log *log_new()
{
    message_log *log;

    log = g_malloc0(sizeof(message_log));

    log->active = TRUE;
    log->buffer = g_string_new(NULL);
    log->entries = g_ptr_array_new();

    return log;
}

void log_destroy(message_log *log)
{
    guint idx;

    assert(log != NULL);

    /* free log entries */
    for (idx = 0; idx < log_length(log); idx++)
    {
        log_entry_destroy(log_get_entry(log, idx));
    }

    g_ptr_array_free(log->entries, TRUE);

    if (log->lastmsg != NULL)
    {
        g_free(log->lastmsg);
    }

    g_string_free(log->buffer, TRUE);
    g_free(log);
}

int log_add_entry(message_log *log, const char *fmt, ...)
{
    va_list argp;
    gchar *msg;

    if (log == NULL || log->active == FALSE)
        return FALSE;

    /* assemble message and append it to the buffer */
    va_start(argp, fmt);
    msg = g_strdup_vprintf(fmt, argp);
    va_end(argp);

    /* compare new message to previous messages to avoid duplicates */
    if (log->lastmsg)
    {
        if (g_strcmp0(msg, log->lastmsg) == 0)
        {
            /* message is equal to previous message */
            g_free(msg);
            return FALSE;
        }
        else
        {
            /* msg is not equal to previous message */
            g_free(log->lastmsg);
            log->lastmsg = msg;
        }
    }
    else
    {
        log->lastmsg = msg;
    }

    /* if there is already text in the buffer, append a space first */
    if (log->buffer->len)
    {
        g_string_append_c(log->buffer, ' ');
    }

    g_string_append(log->buffer, msg);

    return TRUE;
}

void log_set_time(message_log *log, int gtime)
{
    message_log_entry *entry;

    assert(log != NULL);

    /* flush pending entry */
    if ((log->buffer)->len)
    {
        entry = g_malloc(sizeof(message_log_entry));
        entry->gtime = log->gtime;
        entry->message = (log->buffer)->str;

        /* append the entry to the message log */
        g_ptr_array_add(log->entries, entry);

        /* destroy buffer and add prepare new one */
        g_string_free(log->buffer, FALSE);
        log->buffer = g_string_new(NULL);
    }

    /* cleanup previous message buffer */
    if (log->lastmsg)
    {
        g_free(log->lastmsg);
        log->lastmsg = NULL;
    }

    /* assure the log does not grow too much */
    while (log_length(log) > LOG_MAX_LENGTH)
    {
        /* remove the first entry */
        entry = g_ptr_array_remove_index(log->entries, 0);
        log_entry_destroy(entry);
    }

    log->gtime = gtime;
}

message_log_entry *log_get_entry(message_log *log, guint id)
{
    assert(log != NULL && id < log_length(log));
    return g_ptr_array_index(log->entries, id);
}

cJSON *log_serialize(message_log *log)
{
    int idx;
    cJSON *le, *lser = cJSON_CreateArray();

    for (idx = 0; idx < log_length(log); idx++)
    {
        message_log_entry *entry = log_get_entry(log, idx);

        cJSON_AddItemToArray(lser, le = cJSON_CreateObject());
        cJSON_AddNumberToObject(le, "gtime", entry->gtime);
        cJSON_AddStringToObject(le, "message", entry->message);
    }

    return lser;
}

message_log *log_deserialize(cJSON *lser)
{
    int idx;
    message_log *log = g_malloc0(sizeof(message_log));

    log->active = TRUE;
    log->buffer = g_string_new(NULL);
    log->entries = g_ptr_array_new();

    for (idx = 0; idx < cJSON_GetArraySize(lser); idx++)
    {
        cJSON *le = cJSON_GetArrayItem(lser, idx);
        message_log_entry *entry = g_malloc(sizeof(message_log_entry));

        entry->gtime = cJSON_GetObjectItem(le, "gtime")->valueint;
        entry->message = g_strdup(cJSON_GetObjectItem(le, "message")->valuestring);

        g_ptr_array_add(log->entries, entry);
    }

    return log;
}

GPtrArray *text_wrap(const char *str, int width, int indent)
{
    GPtrArray *text;
    size_t pos = 0;     /* position in string */
    int lp;             /* last position of whitespace */
    int len;            /* current line length */
    char *line;         /* copy of line */
    char *spaces = NULL;

    text = g_ptr_array_new();

    /* prepare indentaion */
    if (indent)
    {
        /* allocate an empty string */
        spaces = g_malloc0((indent + 1) * sizeof(char));

        /* fill the string with spaces */
        for (lp = 0; lp < indent; lp++) spaces[lp] = ' ';
    }

    while (pos < strlen(str))
    {
        /* reset target string lenght and position of last whitespace */
        len = lp = 0;

        /* scan the next line */
        while (len <= width)
        {
            /* scan for a space to wrap the current line at */
            if ((str[pos + len] == '\0') || isspace(str[pos + len]))
            {
                lp = len;
                if (str[pos + lp] == '\n')
                {
                    break;
                }
            }
            len++;
        }

        /* allocate space for the new, shortened line */
        line = g_malloc((lp + 1) * sizeof(char));

        /* copy the text to the new line */
        memcpy(line, &(str[pos]), lp);

        /* terminate the newly copied string */
        line[lp] = '\0';

        /* reduce width to make space for indentation after the first line */
        if (indent && text->len == 1)
            width -= indent;


        /* indent lines if not on the first line or the first
           line of a new paragraph */
        if (indent && text->len && str[pos - 1] != '\n')
        {
            /* prepend empty string to line (via temporary string) */
            char *tmp = g_strconcat(spaces, line, NULL);

            g_free(line);
            line = tmp;
        }

        /* append new line to the array of lines */
        g_ptr_array_add(text, line);

        /* move position in source string beyond the end of the last line */
        pos += (lp + 1);
    }

    /* free indentation string */
    if (spaces)
        g_free(spaces);

    return text;
}

GPtrArray *text_append(GPtrArray *first, GPtrArray *second)
{
    assert(first != NULL && second != NULL);

    while (second->len > 0)
        g_ptr_array_add(first,
                        g_ptr_array_remove_index(second, 0));

    text_destroy(second);

    return first;
}

void text_destroy(GPtrArray *text)
{
    assert(text != NULL);

    while (text->len > 0)
        g_free(g_ptr_array_remove_index_fast(text, 0));

    g_ptr_array_free(text, TRUE);
}

/**
 * create a new NULL-terminated string array
 */
char **strv_new()
{
    char **list = g_new(char *, 1);
    list[0] = NULL;

    return list;
}

/**
 * adds a copy of str to the list
 */
int strv_append(char ***list, const char *str)
{
    int len = g_strv_length(*list) + 1;

    assert(list != NULL);
    assert(str != NULL);

    *list = g_realloc (*list, sizeof(char*) * (len + 1));

    (*list)[len - 1] = g_strdup(str);
    (*list)[len] = NULL;

    return len;
}

/**
 * add a copy of str to the list if it is not yet part of the list
 */
int strv_append_unique(char ***list, const char *str)
{
    int len = 0;

    /* compare elements to the new string and return FALSE if the element existed */
    for (len = 0; (*list)[len]; len++)
        if (strcmp((*list)[len], str) == 0) return FALSE;

    return strv_append(list, str);
}

int str_starts_with_vowel(const char *str)
{
    const char vowels[] = "aeiouAEIOU";

    assert (str != NULL);

    if (strchr(vowels, str[0])) return TRUE;
    else return FALSE;
}

const char *int2str(int val)
{
    static char buf[21];
    const char *count_desc[] = { "no", "one", "two", "three", "four", "five",
                                 "six", "seven", "eight", "nine", "ten",
                                 "eleven", "twelve", "thirteen", "fourteen",
                                 "fifteen", "sixteen", "seventeen", "eighteen",
                                 "nineteen", "twenty"
                               };

    if (val <= 20)
    {
        return count_desc[val];
    }
    else
    {
        g_snprintf(buf, 20, "%d", val);
        return buf;
    }

}

damage *damage_new(damage_t type, attack_t attack, int amount, gpointer originator)
{
    damage *dam = g_malloc0(sizeof(damage));

    dam->type = type;
    dam->attack = attack;
    dam->amount = amount;
    dam->originator = originator;

    return dam;
}

void utils_wrap(lua_State *L)
{
    int i;

    assert (L != NULL);

    /* register constants */
    struct
    {
        char *name;
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

    for (i = 0; constants[i].name != NULL; i++)
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

    return str[0];
}

int luaN_query_int(const char *table, guint idx, const char *attrib)
{
    int val = 0;

    if (luaN_data_query(table, idx, attrib))
    {
        val = luaL_checkint(nlarn->L, -1);
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

static void log_entry_destroy(message_log_entry *entry)
{
    assert(entry != NULL);
    g_free(entry->message);
    g_free(entry);
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

static int wrap_log(lua_State *L)
{
    int nargs = lua_gettop(L);    /* number of arguments */
    int i;

    for (i = 1; i <= nargs; i++)
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
        result = rand_m_n(luaL_checkint(L, 1), luaL_checkint(L, 2));
    }
    else
    {
        result = rand_0n(luaL_checkint(L, 1));
    }

    lua_pushinteger(L, result);

    return 1;
}

static int wrap_chance(lua_State *L)
{
    lua_pushboolean(L, chance(luaL_checkint(L, 1)));
    return 1;
}
