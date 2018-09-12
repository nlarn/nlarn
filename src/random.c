/*
 * random.c
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
#include <stdint.h>

#ifdef __linux__
# define __USE_XOPEN_EXTENDED
#endif
#ifdef G_OS_WIN32
# define _CRT_RAND_S
#endif
#include <stdlib.h>

#include "random.h"

/* The following code is taken from xoshiro128starstar.c,
 * which is to be found on http://vigna.di.unimi.it/xorshift/ */

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro128** 1.0, our 32-bit all-purpose, rock-solid generator. It
   has excellent (sub-ns) speed, a state size (128 bits) that is large
   enough for mild parallelism, and it passes all tests we are aware of.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */


static inline uint32_t rotl(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}


static uint32_t s[4];

uint32_t next(void) {
	const uint32_t result_starstar = rotl(s[0] * 5, 7) * 9;

	const uint32_t t = s[1] << 9;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 11);

	return result_starstar;
}

/* end xoshiro128starstar.c excerpt */

static gboolean seeded = FALSE;

/* initialize RNG */
static void rand_seed()
{
    g_assert(seeded == FALSE);

    /* On Windows, use rand_s to seed out RNG; otherwise use random() */
#ifndef G_OS_WIN32
    srandom(g_get_monotonic_time());
#endif

    for (int i = 0; i < 4; i++)
    {
#ifdef G_OS_WIN32
        rand_s(&s[i]);
#else
        s[i] = random();
#endif
    }

    seeded = TRUE;
}

cJSON* rand_serialize()
{
    g_assert(seeded == TRUE);

    return cJSON_CreateIntArray((int*)&s, 4);
}

void rand_deserialize(cJSON *r)
{
    g_assert(r != NULL);
    g_assert(cJSON_GetArraySize(r) == 4);

    for (int i = 0; i < 4; i++)
    {
        cJSON* it = cJSON_GetArrayItem(r, i);
        g_assert(cJSON_IsNumber(it));
        s[i] = (guint64)it->valuedouble;
    }

    seeded = TRUE;
}

guint32 rand_0n(guint32 n)
{
    if (!seeded)
    {
        rand_seed();
    }

    guint32 min = -n % n;
    guint32 result;

    switch (n)
    {
        case 0:
        case 1:
            return 0;
            break;
        case UINT32_MAX:
            return next();
            break;
        default:
            while ((result = next()) < min);

            return result % n;
            break;
    }
}

int divert(int value, int percent)
{
    int lower, upper;

    g_assert(value > 0 && percent > 0);

    lower = value - (value / percent);
    upper = value + (value / percent);
    if (lower == upper)
        return value;

    return rand_m_n(lower, upper);
}

void shuffle(int array[], int length, int skip)
{
    for (int i = 0; i < length; i++)
    {
        /* fill the array in order */
        array[i] = i;
    }

    for (int i = skip; i < (length / 2); i++)
    {
        /* randomize positions */
        int npos = i + rand_0n(length - i);
        int temp = array[i];
        array[i] = array[npos];
        array[npos] = temp;
    }
}

