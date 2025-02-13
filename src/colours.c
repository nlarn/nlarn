/*
 * colours.c
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

#include <string.h>

#include "colours.h"

int colour_lookup(const colset *colset, const char *name)
{
    int colour = 0;
    int pos = 0;

    while (colset[pos].name != NULL)
    {
        if (strcmp(name, colset[pos].name) == 0)
        {
            /* colour found */
            colour = colset[pos].val;
            break;
        }

        pos++;
    }

    return colour;
}
