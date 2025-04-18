/*
 * gems.h
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

#ifndef __GEMS_H_
#define __GEMS_H_

#include "colours.h"
#include "enumFactory.h"
#include "items.h"

#define GEM_TYPE_ENUM(GEM_TYPE) \
    GEM_TYPE(GT_DIAMOND,) \
    GEM_TYPE(GT_RUBY,) \
    GEM_TYPE(GT_EMERALD,) \
    GEM_TYPE(GT_SAPPHIRE,) \
    GEM_TYPE(GT_MAX,)

DECLARE_ENUM(gem_t, GEM_TYPE_ENUM)

/* functions */

const char *gem_name(item *gem);
colour gem_colour(item *gem);
int gem_weight(item *gem);
int gem_price(item *gem);
int gem_size(item *gem);

#endif
