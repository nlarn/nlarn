/*
 * random.h
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

#ifndef RANDOM_H
#define RANDOM_H

#include <glib.h>

#include "cJSON.h"

/* function definitions */

cJSON* rand_serialize();
void rand_deserialize(cJSON *r);

/* The following function use a global state
 * which is automatically seeded on first usage. */

guint32 rand_0n(guint32 n);

/* returns a value x with m <= x < n. */
static inline guint32 rand_m_n(const guint32 m, const guint32 n)
{
    g_assert(m < n);
    return rand_0n(n - m) + m;
}

static inline guint32 rand_1n(const guint32 n)
{
    return (n <= 1) ? 1 : rand_m_n(1, n);
}

static inline gboolean chance(const guint32 percent)
{
    g_assert(percent < 101);
    return (percent >= rand_1n(101));
}

int divert(int value, int percent);

/**
 * Shuffle an array of integers
 *
 * @param array pointer to integer array
 * @param length of array
 * @param skip how many fields should be skipped
 */
void shuffle(int array[], int length, int skip);

/**
 * Return a random number processed by the Lévy probability density function
 * See https://en.wikipedia.org/wiki/L%C3%A9vy_distribution for details.
 *
 * @param c The scale parameter
 * @param mu The location parameter
 * @return a random number in
 */
double levy_random(double c, double mu);

/*
 * Pick a random integer processed by the Lévy probability density
 * function in a range between 0 and max.
 *
 * @param the upper limit
 * @param scale parameter
 * @param location parameter
 * @return random integer < max
 */
int levy_element(int max, double c, double mu);

#endif
