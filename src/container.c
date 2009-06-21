/*
 * container.c
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

#include "nlarn.h"

const container_data containers[CT_MAX] =
{
    { CT_NONE,   "",          0, IM_NONE,  },
    { CT_BAG,    "bag",     375, IM_CLOTH, },
    { CT_CASKET, "casket", 3900, IM_WOOD,  },
    { CT_CHEST,  "chest", 13500, IM_WOOD,  },
    { CT_CRATE,  "crate", 65000, IM_WOOD,  },
};
