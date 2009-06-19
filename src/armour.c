/*
 * armour.c
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

/* TODO: sanitize prices */
static const armour_data armours[AT_MAX] =
{
    /* id            name                      ac  category     material       we   pr  un */
    { AT_NONE,       "",                        0, AC_NONE,     IM_NONE,        0,    0, 0 },
    { AT_SHIELD,     "shield",                  2, AC_SHIELD,   IM_WOOD,     3500,  150, 0 },
    { AT_LEATHER,    "leather armour",          2, AC_SUIT,     IM_LEATHER,  4000,    2, 0 },
    { AT_SLEATHER,   "studded leather armour",  3, AC_SUIT,     IM_LEATHER,  7500,   10, 0 },
    { AT_RIGNMAIL,   "ring mail",               5, AC_SUIT,     IM_IRON,    10000,   40, 0 },
    { AT_CHAINMAIL,  "chain mail",              6, AC_SUIT,     IM_IRON,    11500,   85, 0 },
    { AT_SPLINTMAIL, "splint mail",             7, AC_SUIT,     IM_IRON,    13000,  220, 0 },
    { AT_PLATEMAIL,  "plate mail",              9, AC_SUIT,     IM_IRON,    17500,  400, 0 },
    { AT_PLATE,      "plate armour",           10, AC_SUIT,     IM_IRON,    20000,  900, 0 },
    { AT_SPLATE,     "stainless plate armour", 12, AC_SUIT,     IM_STEEL,   20000, 2600, 0 },
    { AT_ELVENCHAIN, "elven chain",            15, AC_SUIT,     IM_MITHRIL,  7500,    0, 0 },
};

armour *armour_new(int armour_type, int bonus)
{
    armour *narmour;

	assert(armour_type > AT_NONE && armour_type < AT_MAX);

	/* has to be zeroed or item_compare will fail */
    narmour = g_malloc0(sizeof(armour));
    assert(narmour != NULL);

    narmour->type = armour_type;
    narmour->ac_bonus = bonus;
    narmour->blessedness = 1;

    return narmour;
}

void armour_destroy(armour *a)
{
    assert(a != NULL);
    g_free(a);
}

inline char *armour_get_name(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].name;
}

inline int armour_get_ac(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].ac + a->ac_bonus;
}

inline armour_cat armour_get_category(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].category;
}

inline int armour_get_material(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].material;
}

inline int armour_get_weight(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].weight;
}

inline int armour_get_price(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].price;
}

inline gboolean armour_is_uniqe(armour *a)
{
    assert(a != NULL && a->type > AT_NONE && a->type < AT_MAX);
    return armours[a->type].unique;
}

int armour_bless(armour *a)
{
	assert(a != NULL);

    if (a->blessedness == 2)
    {
        /* holy already */
        return FALSE;
    }
    else
    {
        /* first blessing removes curse (if any) */
        a->blessedness++;
    }

    return TRUE;
}

int armour_curse(armour *a)
{
	assert(a != NULL);

    if (!a->blessedness)
    {
        /* already cursed */
        return FALSE;
    }
    else
    {
        /* first curse removes blessing (if any) */
        a->blessedness--;
    }

    return TRUE;
}

int armour_enchant(armour *a)
{
	assert(a != NULL);

	a->ac_bonus++;

	return TRUE;
}

int armour_disenchant(armour *a)
{
	assert(a != NULL);

	a->ac_bonus--;

	return TRUE;
}

int armour_rust(armour *a)
{
    assert(a != NULL);

    if ((armours[a->type].material == IM_IRON) || (armours[a->type].material == IM_STEEL))
    {
        if (a->rusty == 2)
        {
            /* it's been very rusty already -> destroy */
            return PI_DESTROYED;
        }
        else
        {
            a->rusty++;
            return PI_ENFORCED;
        }
    }
    else
    {
        /* item cannot rust. */
        return PI_NONE;
    }
}

int armour_corrode(armour *a)
{
    assert(a != NULL);

    if (armours[a->type].material == IM_IRON)
    {
        if (a->corroded == 2)
        {
            return PI_DESTROYED;
        }
        else
        {
            a->corroded++;
            return PI_ENFORCED;
        }
    }
    else
    {
        return PI_NONE;
    }
}

int armour_burn(armour *a)
{
    assert(a != NULL);

    if (armours[a->type].material <= IM_BONE)
    {
        if (a->burnt == 2)
        {
            return PI_DESTROYED;
        }
        else
        {
            a->burnt++;
            return PI_ENFORCED;
        }
    }
    else
    {
        return PI_NONE;
    }
}
