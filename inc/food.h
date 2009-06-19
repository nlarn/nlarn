/*
 * food.h
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

#ifndef __FOOD_H_
#define __FOOD_H_

enum food_types
{
    FT_NONE,
    FT_FORTUNE_COOKIE,
    FT_MAX
};

typedef struct food {
	int type;
} food;

typedef struct food_data {
    int id;
    char *name;
    int weight;
} food_data;

/* function declarations */

food *food_new(int food_type);
void food_destroy(food *f);

inline char *food_get_name(food *f);
inline int food_get_weight(food *f);
char *food_get_fortune(food *f, char *fortune_file);
#endif
