/*
 * random.h
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

#ifndef __RANDOM_H_
#define __RANDOM_H_

#include <glib.h>

#include "cJSON.h"

/* function definitions */

cJSON* rand_serialize();
void rand_deserialize(cJSON *r);

/* The following function use a global state
 * which is automatically seeded on first usage. */

guint32 rand_0n(guint32 n);

/* returns a value x with m <= x < n. */
static inline guint32 rand_m_n(guint32 m, guint32 n)
{
	g_assert(m < n);
    return rand_0n(n - m) + m;
}

static inline guint32 rand_1n(guint32 n)
{
    return (n <= 1) ? 1 : rand_m_n(1, n);
}

static inline gboolean chance(guint32 percent)
{
    return (percent >= rand_1n(101));
}

int divert(int value, int percent);

/**
 * Shuffle an array of integers
 *
 * @param pointer to integer array
 * @param length of array
 * @param how many fields should be skipped
 */
void shuffle(int array[], int length, int skip);

#endif
