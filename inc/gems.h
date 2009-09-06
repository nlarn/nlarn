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

/* external vars */

extern const gem_data gems[GT_MAX];

/* macros */

#define gem_name(gem)     (gems[(gem)->id].name)
#define gem_material(gem) (gems[(gem)->id].material)
#define gem_colour(gem)   (gems[(gem)->id].colour)
#define gem_weight(gem)   ((gem)->bonus / 1000)
#define gem_price(gem)    ((gem)->bonus * gems[(gem)->id].price)
#define gem_size(gem)     ((gem)->bonus)

#endif
