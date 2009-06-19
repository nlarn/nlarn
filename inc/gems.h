/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gems.h
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

#ifndef __GEMS_H_
#define __GEMS_H_

enum gem_types
{
    GT_NONE,
    GT_DIAMOND,
    GT_RUBY,
    GT_EMERALD,
    GT_SAPPHIRE,
/*  GT_EYE_OF_LARN, TODO: implement this */
    GT_MAX
};

typedef struct gem_data
{
    int id;
    char *name;
	int	material;       /* material type from item_materials */
	int colour;
	int price;          /* price per carat in the shops */
} gem_data;

typedef struct gem
{
    int type;
    int carat;          /* base price modifier */
} gem;

/* function declarations */

gem *gem_new(int gem_type, int carat);
void gem_destroy(gem *g);

inline char *gem_get_name(gem *g);
inline int gem_get_material(gem *g);
inline int gem_get_colour(gem *g);
inline int gem_get_weight(gem *g);
inline int gem_get_price(gem *g);
inline int gem_get_size(gem *g);

#endif
