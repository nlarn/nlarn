/*
 * utils.c
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

#include <ctype.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "nlarn.h"
#include "utils.h"

static const guint LOG_MAX_LENGTH = 100;

static void log_entry_destroy(message_log_entry *entry);

char *str_capitalize(char *string)
{
    if (string == NULL)
    {
        return NULL;
    }

    for (guint i = 0; i < strlen(string); ++i)
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
    log->entries = g_ptr_array_new_with_free_func(
            (GDestroyNotify)log_entry_destroy);

    return log;
}

void log_destroy(message_log *log)
{
    g_assert(log != NULL);

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

    g_assert(log != NULL);

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

    /* clean up previous message buffer */
    if (log->lastmsg)
    {
        g_free(log->lastmsg);
        log->lastmsg = NULL;
    }

    /* assure the log does not grow too much */
    while (log_length(log) > LOG_MAX_LENGTH)
    {
        /* remove the first entry */
        g_ptr_array_remove_index(log->entries, 0);
    }

    log->gtime = gtime;
}

message_log_entry *log_get_entry(message_log *log, guint id)
{
    g_assert(log != NULL && id < log_length(log));
    return g_ptr_array_index(log->entries, id);
}

cJSON *log_serialize(message_log *log)
{
    cJSON *log_ser = cJSON_CreateObject();
    cJSON *log_entries = cJSON_CreateArray();

    /* create array of log entries */
    for (guint idx = 0; idx < log_length(log); idx++)
    {
        message_log_entry *entry = log_get_entry(log, idx);
        cJSON *log_entry = cJSON_CreateObject();

        cJSON_AddItemToArray(log_entries, log_entry);
        cJSON_AddNumberToObject(log_entry, "gtime", entry->gtime);
        cJSON_AddStringToObject(log_entry, "message", entry->message);
    }

    /* add this turns message if filled */
    if (log->buffer->len > 0)
    {
        cJSON_AddStringToObject(log_ser, "buffer", log->buffer->str);
    }

    /* add last message buffer if filled */
    if (log->lastmsg != NULL)
    {
        cJSON_AddStringToObject(log_ser, "lastmsg", log->lastmsg);
    }

    /* add array of entries to log object */
    cJSON_AddItemToObject(log_ser, "entries", log_entries);

    return log_ser;
}

message_log *log_deserialize(cJSON *lser)
{
    cJSON *obj;

    /* create new message log */
    message_log *log = g_malloc0(sizeof(message_log));

    log->active = TRUE;
    log->entries = g_ptr_array_new_with_free_func(
            (GDestroyNotify)log_entry_destroy);

    /* try to restore this turns message */
    if ((obj = cJSON_GetObjectItem(lser, "buffer")) != NULL)
    {
        /* restore buffer from saved value */
        log->buffer = g_string_new(obj->valuestring);
    }
    else
    {
        /* create empty buffer */
        log->buffer = g_string_new(NULL);
    }

    /* try to restore the last message */
    if ((obj = cJSON_GetObjectItem(lser, "lastmsg")) != NULL)
    {
        log->lastmsg = g_strdup(obj->valuestring);
    }

    /* try to get all log entries from the supplied cJSON object */
    if ((obj = cJSON_GetObjectItem(lser, "entries")) != NULL)
    {
    /* reconstruct message log entries */
        for (int idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        {
            cJSON *le = cJSON_GetArrayItem(obj, idx);
            message_log_entry *entry = g_malloc(sizeof(message_log_entry));

            entry->gtime = cJSON_GetObjectItem(le, "gtime")->valueint;
            entry->message = g_strdup(cJSON_GetObjectItem(le, "message")->valuestring);

            g_ptr_array_add(log->entries, entry);
        }
    }

    return log;
}

GPtrArray *text_wrap(const char *str, int width, int indent)
{
    GPtrArray *text;
    int spos = 0;       /* current starting position in source string */
    int lp;             /* last position of whitespace */
    int len;            /* length of str */
    char *spaces = NULL;

    text = g_ptr_array_new();

    /* prepare indentation */
    if (indent)
    {
        /* allocate an empty string */
        spaces = g_malloc0((indent + 1) * sizeof(char));

        /* fill the string with spaces */
        for (lp = 0; lp < indent; lp++)
            spaces[lp] = ' ';
    }

    len = strlen(str);

    /* scan through source string */
    while (spos < len)
    {
        /* flag to determine if the current char must not be counted */
        gboolean in_tag = FALSE;

        /* current working position in source string */
        int cpos = 0;

        /* length of text excluding tags content on current line */
        int llen = 0;

        /* reset target string length and position of last whitespace */
        lp = 0;

        /* copy of line */
        char *line;

        /* scan the next line */
        while (llen <= width)
        {
            /* toggle the flag if the current character is the tag start / stop symbol */
            if (str[spos + cpos] == '`')
            {
                in_tag = !in_tag;
            }

            /* stop at the end of the string */
            char next = str[spos + cpos];
            if (next == '\0' || next == '\n' || next == '\r')
            {
                lp = cpos;
                break;
            }

            /* scan for a space at which to wrap the current line */
            if (g_ascii_isspace(str[spos + cpos]))
            {
                lp = cpos;
            }

            /* increase the string length if not inside or at the end of a tag */
            if (!in_tag && (str[spos + cpos] != '`'))
                llen++;

            /* move the current working position */
            cpos++;
        }

        /* copy the text to the new line */
        line = g_strndup(&(str[spos]), lp);

        /* skip silly CR chars */
        if (str[spos + lp] == '\r') lp++;

        /* reduce width to make space for indentation after the first line */
        if (indent && text->len == 1)
            width -= indent;

        /* indent lines if not on the first line or the first
           line of a new paragraph */
        if (indent && text->len && str[spos - 1] != '\n')
        {
            /* prepend empty string to line (via temporary string) */
            char *tmp = g_strconcat(spaces, line, NULL);

            g_free(line);
            line = tmp;
        }

        /* append new line to the array of lines */
        g_ptr_array_add(text, line);

        /* move position in source string beyond the end of the last line */
        spos += (lp + 1);
    }

    /* free indentation string */
    if (spaces)
        g_free(spaces);

    return text;
}

GPtrArray *text_append(GPtrArray *first, GPtrArray *second)
{
    g_assert(first != NULL && second != NULL);

    while (second->len > 0)
        g_ptr_array_add(first, g_ptr_array_remove_index(second, 0));

    text_destroy(second);

    return first;
}

void text_destroy(GPtrArray *text)
{
    g_assert(text != NULL);

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
    g_assert(list != NULL);
    g_assert(str != NULL);

    int len = g_strv_length(*list) + 1;

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
    /* compare elements to the new string and return FALSE if the element existed */
    for (int len = 0; (*list)[len]; len++)
        if (strcmp((*list)[len], str) == 0) return FALSE;

    return strv_append(list, str);
}

char *str_prepare_for_saving(const char *str)
{
    if (str == NULL) return NULL;

#ifdef G_OS_WIN32
    const char lend[] = "\r\n";
#else
    const char lend[] = "\n";
#endif

    GPtrArray *wrapped_str = text_wrap(str, 78, 2);
    GString *nstr = g_string_new(NULL);

    for (guint i = 0; i < wrapped_str->len; i++)
    {
        g_string_append(nstr, g_ptr_array_index(wrapped_str, i));
        g_string_append(nstr, lend);
    }

    text_destroy(wrapped_str);

    return g_string_free(nstr, FALSE);
}

int str_starts_with_vowel(const char *str)
{
    const char vowels[] = "aeiouAEIOU";

    g_assert (str != NULL);

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

const char *int2time_str(int val)
{
    if (val <= 3)
    {
        const char *count_desc[] = { "never", "once", "twice", "thrice" };
        return count_desc[val];
    }
    else
    {
        static char buf[21];
        g_snprintf(buf, 20, "%d times", val);
        return buf;
    }
}

static void log_entry_destroy(message_log_entry *entry)
{
    g_assert(entry != NULL);
    g_free(entry->message);
    g_free(entry);
}
