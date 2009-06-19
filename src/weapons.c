/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * weapons.c
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
static const weapon_data weapons[WT_MAX] =
{
    /* ID               name                         wc  material    we     pr tw un */
    { WT_NONE,          "",                           0, IM_NONE,     0,     0, 0, 0 },
    { WT_DAGGER,        "dagger",                     3, IM_IRON,   600,     2, 0, 0 },
    { WT_SPEAR,         "spear",                     10, IM_IRON,  1800,    20, 0, 0 },
    { WT_FLAIL,         "flail",                     14, IM_IRON,  2900,    80, 1, 0 },
    { WT_BATTLEAXE,     "battle-axe",                17, IM_IRON,  2700,   150, 0, 0 },
    { WT_LONGSWORD,     "longsword",                 22, IM_IRON,  1950,   450, 0, 0 },
    { WT_2SWORD,        "two-handed sword",          26, IM_IRON,  3600,  1000, 1, 0 },
    { WT_SWORDSLASHING, "sword of slashing",         30, IM_IRON,  2200,  6000, 0, 0 },
    /* unique weapons */
    { WT_LANCEOFDEATH,  "lance of death",            20, IM_WOOD,  2900, 16500, 1, 1 },
    { WT_VORPALBLADE,   "Vorpal blade",              22, IM_STEEL, 1600,     0, 0, 1 },
    { WT_SLAYER,        "Slayer",                    30, IM_STEEL, 1800,     0, 0, 1 },
    { WT_SUNSWORD,      "Sunsword",                  32, IM_STEEL, 1800,  5000, 0, 1 },
    { WT_BESSMAN,       "Bessman's flailing hammer", 35, IM_STEEL, 5800, 10000, 1, 1 },
};

static int weapon_created[WT_MAX] = { 1, 0 };

weapon *weapon_new(int weapon_type, int bonus)
{
    weapon *nweapon;

    assert(weapon_type > WT_NONE && weapon_type < WT_MAX);

    if (weapons[weapon_type].unique)
    {
        if (weapon_created[weapon_type])
        {
            /* create another random weapon instead */
            return(weapon_new(rand_1n(WT_MAX - 1), bonus));
        }
        else
        {
            weapon_created[weapon_type] = TRUE;
        }
    }

    /* has to be zeroed or memcmp will fail */
    nweapon = (weapon *)g_malloc0(sizeof(weapon));

    nweapon->type = weapon_type;
    nweapon->wc_bonus = bonus;

    return nweapon;
}

void weapon_destroy(weapon *w)
{
    assert(w != NULL);
    g_free(w);
}

inline char *weapon_get_name(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    return weapons[w->type].name;
}

inline int weapon_get_wc(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    /* FIXME: consider damage */
    return weapons[w->type].wc + w->wc_bonus;
}

inline int weapon_get_material(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    return weapons[w->type].material;
}

inline int weapon_get_weight(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    return weapons[w->type].weight;
}

inline int weapon_get_price(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    return weapons[w->type].price;
}

inline gboolean weapon_is_twohanded(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    return weapons[w->type].twohanded;
}

inline gboolean weapon_is_uniqe(weapon *w)
{
    assert(w != NULL && w->type > WT_NONE && w->type < WT_MAX);
    return weapons[w->type].unique;
}

int weapon_bless(weapon *w)
{
	assert(w != NULL);

    if (w->blessedness == 2)
    {
        /* holy already */
        return FALSE;
    }
    else
    {
        /* first blessing removes curse (if any) */
        w->blessedness++;
    }

    return TRUE;
}

int weapon_curse(weapon *w)
{
	assert(w != NULL);

    if (!w->blessedness)
    {
        /* already cursed */
        return FALSE;
    }
    else
    {
        /* first curse removes blessing (if any) */
        w->blessedness--;
    }

    return TRUE;
}

int weapon_enchant(weapon *w)
{
	assert(w != NULL);

	w->wc_bonus++;

	return TRUE;
}

int weapon_disenchant(weapon *w)
{
	assert(w != NULL);

	w->wc_bonus--;

	return TRUE;
}

int weapon_rust(weapon *w)
{
    assert(w != NULL);

    if ((weapons[w->type].material == IM_IRON) || (weapons[w->type].material == IM_STEEL))
    {
        if (w->rusty == 2)
        {
            /* it's been very rusty already -> destroy */
            return PI_DESTROYED;
        }
        else
        {
            w->rusty++;
            return PI_ENFORCED;
        }
    }
    else
    {
        /* item cannot rust. */
        return PI_NONE;
    }
}

int weapon_burn(weapon *w)
{
    assert(w != NULL);

    if (weapons[w->type].material <= IM_BONE)
    {
        if (w->burnt == 2)
        {
            return PI_DESTROYED;
        }
        else
        {
            w->burnt++;
            return PI_ENFORCED;
        }
    }
    else
    {
        return PI_NONE;
    }
}

int weapon_corrode(weapon *w)
{
    assert(w != NULL);

    if (weapons[w->type].material == IM_IRON)
    {
        if (w->corroded == 2)
        {
            return PI_DESTROYED;
        }
        else
        {
            w->corroded++;
            return PI_ENFORCED;
        }
    }
    else
    {
        return PI_NONE;
    }
}
