/*
 * utils.h
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

#ifndef __UTILS_H_
#define __UTILS_H_

#include <time.h>
#include "cJSON.h"
#include "defines.h"

/* game messaging */
typedef struct _message_log_entry
{
    guint32 gtime;      /* game time of log entry */
    char *message;
} message_log_entry;

typedef struct _message_log
{
    guint32 gtime;      /* current game time */
    gint32 active;      /* flag to disable logging onto this log */
    GString *buffer;    /* space to assemble a turn's messages */
    char *lastmsg;      /* copy of last message */
    GPtrArray *entries;
} message_log;

/* macros */

/* NOTE: g_random_int_range(m,n) returns a value x with m <= x < n. */
#define rand_1n(n)      (((n) <= 1) ? 1 : g_random_int_range(1,(n)))
#define rand_0n(n)      (((n) <= 0) ? 0 : g_random_int_range(0,(n)))
#define rand_m_n(m,n)   ((m) == (n) ? (m) : g_random_int_range((m),(n)))
#define chance(percent) ((percent) >= rand_1n(101))

/* windef.h defines these */
#ifdef WIN32
#undef min
#undef max
#endif

static inline int min(int x, int y) { return x > y ? y : x; }
static inline int max(int x, int y) { return x > y ? x : y; }

/* function definitions */
int divert(int value, int percent);

/**
 * Shuffle an array of integers
 *
 * @param pointer to integer array
 * @param length of array
 * @param how many fields should be skipped
 */
void shuffle(int array[], int length, int skip);

char *str_replace(char *string, char *orig, char *replace);
char *str_capitalize(char *string);

/* message log handling */
message_log *log_new();
void log_destroy(message_log *log);
int log_add_entry(message_log *log, const char *fmt, ...);

/**
 * Update the game time. This function flushes the message buffer and appends
 * the collected messages to the log.
 *
 * @param the log
 * @param the new game time
 */
void log_set_time(message_log *log, int gtime);

message_log_entry *log_get_entry(message_log *log, guint id);
cJSON *log_serialize(message_log *log);
message_log *log_deserialize(cJSON *lser);

#define LOG_MAX_LENGTH   100
#define log_length(log)  ((log)->entries->len)
#define log_enable(log)  ((log)->active = TRUE)
#define log_disable(log) ((log)->active = FALSE)
#define log_buffer(log)  ((log)->buffer->len ? (log)->buffer->str : NULL)

/* text array handling */
GPtrArray *text_wrap(const char *str, int width, int indent);

/**
 * append one array of text to another array of text.
 *
 * @param text to append to
 * @param text to append. this will be freed.
 * @return pointer to combined array.
 */
GPtrArray *text_append(GPtrArray *first, GPtrArray *second);

void text_destroy(GPtrArray *text);

/* helpers for NULL-terminated string arrays */
/* glib offers g_strfreev, g_strdupv, g_strjoinv and g_strv_length */
char **strv_new();
int strv_append(char ***list, const char *str);
int strv_append_unique(char ***list, const char *str);

/* misc. text functions */
int str_starts_with_vowel(const char *str);
const char *int2str(int val);
#define a_an(str) (str_starts_with_vowel((str)) ? "an" : "a")
#define plural(i) (((i) > 1) ? "s" : "")


/* regarding stuff defined in defines.h */
damage *damage_new(damage_t type, attack_t attack, int amount,
                   damage_originator_t damo, gpointer originator);

damage *damage_copy(damage *dam);

#define damage_free(dam)    g_free((dam))
#define INSTANT_KILL    10000

#endif
