/*
 * utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include "cJSON.h"

/* game messaging */
typedef struct message_log_entry
{
    guint32 gtime;      /* game time of log entry */
    char *message;
} message_log_entry;

typedef struct message_log
{
    guint32 gtime;      /* current game time */
    gint32 active;      /* flag to disable logging onto this log */
    GString *buffer;    /* space to assemble a turn's messages */
    char *lastmsg;      /* copy of last message */
    GPtrArray *entries;
} message_log;

/* windef.h defines these */
#ifdef WIN32
#undef min
#undef max
#endif

static inline int min(int x, int y) { return x > y ? y : x; }
static inline int max(int x, int y) { return x > y ? x : y; }

/* function definitions */
char *str_capitalize(char *string);

/* message log handling */
message_log *log_new();
void log_destroy(message_log *log);
int log_add_entry(message_log *log, const char *fmt, ...);

/**
 * Update the game time. This function flushes the message buffer and appends
 * the collected messages to the log.
 *
 * @param log the log
 * @param gtime the new game time
 */
void log_set_time(message_log *log, int gtime);

message_log_entry *log_get_entry(message_log *log, guint id);
cJSON *log_serialize(message_log *log);
message_log *log_deserialize(cJSON *lser);

static inline guint log_length(const message_log *log) { return log->entries->len; }
static inline void log_enable(message_log *log)  { log->active = true; }
static inline void log_disable(message_log *log) { log->active = false; }

static inline char *log_buffer(const message_log *log)
{
    return log->buffer->len ? log->buffer->str : NULL;
}

/* text array handling */
GPtrArray *text_wrap(const char *str, int width, int indent);

/**
 * append one array of text to another array of text.
 *
 * @param first Multi-line text to append to
 * @param second Multi-line text to append. This will be freed.
 * @return A pointer to the combined array.
 */
GPtrArray *text_append(GPtrArray *first, GPtrArray *second);

/**
 * Determine the length of longest text line
 *
 * @param text An array of strings
 * @return the length of the longest string in the array
 */
int text_get_longest_line(GPtrArray *text);

void text_destroy(GPtrArray *text);

/* helpers for NULL-terminated string arrays */
/* glib offers g_strfreev, g_strdupv, g_strjoinv and g_strv_length */
char **strv_new();
size_t strv_append(char ***list, const char *str);
size_t strv_append_unique(char ***list, const char *str);

/* misc. text functions */
char *str_strip(const char *str);
char *str_prepare_for_saving(const char *str);
int str_starts_with_vowel(const char *str);
const char *int2str(guint val);
const char *int2time_str(guint val);

static inline const char *a_an(const char *str)
{
    return str_starts_with_vowel(str) ? "an" : "a";
}

static inline const char *is_are(const guint i)
{
    return (i == 1) ? "is" : "are";
}

static inline const char *plural(const guint i)
{
    return (i > 1) ? "s" : "";
}

#endif
