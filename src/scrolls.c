/*
 * scrolls.c
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

/* TODO: sanitize price */
static const magic_scroll_data scrolls[ST_MAX] =
{
    /* ID                   name                  effect               price */
    { ST_NONE,              "",                   ET_NONE,                 0 },
    { ST_ENCH_ARMOUR,       "enchant armour",     ET_NONE,               100 },
    { ST_ENCH_WEAPON,       "enchant weapon",     ET_NONE,               125 },
    { ST_ENLIGHTENMENT,     "enlightenment",      ET_ENLIGHTENMENT,       60 },
    { ST_BLANK,             "blank paper",        ET_NONE,                10 },
    { ST_CREATE_MONSTER,    "create monster",     ET_NONE,               100 },
    { ST_CREATE_ARTIFACT,   "create artifact",    ET_NONE,               200 },
    { ST_AGGRAVATE_MONSTER, "aggravate monsters", ET_AGGRAVATE_MONSTER,  110 },
    { ST_TIMEWARP,          "time warp",          ET_NONE,               500 },
    { ST_TELEPORT,          "teleportation",      ET_NONE,               200 },
    { ST_AWARENESS,         "expanded awareness", ET_AWARENESS,          250 },
    { ST_HASTE_MONSTER,     "haste monsters",     ET_HASTE_MONSTER,       20 },
    { ST_HEAL_MONSTER,      "monster healing",    ET_NONE,                30 },
    { ST_SPIRIT_PROTECTION, "spirit protection",  ET_SPIRIT_PROTECTION,  340 },
    { ST_UNDEAD_PROTECTION, "undead protection",  ET_UNDEAD_PROTECTION,  340 },
    { ST_STEALTH,           "stealth",            ET_STEALTH,            300 },
    { ST_MAPPING,           "magic mapping",      ET_NONE,               400 },
    { ST_HOLD_MONSTER,      "hold monsters",      ET_HOLD_MONSTER,       500 },
    { ST_GEM_PERFECTION,    "gem perfection",     ET_NONE,              1000 },
    { ST_SPELL_EXTENSION,   "spell extension",    ET_NONE,               500 },
    { ST_IDENTIFY,          "identify",           ET_NONE,               340 },
    { ST_REMOVE_CURSE,      "remove curse",       ET_NONE,               220 },
    { ST_ANNIHILATION,      "annihilation",       ET_NONE,              3900 },
    { ST_PULVERIZATION,     "pulverization",      ET_NONE,               610 },
    { ST_LIFE_PROTECTION,   "life protection",    ET_LIFE_PROTECTION,   3000 },
};

static int scroll_desc_mapping[ST_MAX - 1] = { 0 };

static const char *scroll_desc[ST_MAX - 1] =
{
    "Ssyliir Wyleeum",
    "Etzak Biqolix",
    "Tzaqa Chanim",
    "Lanaj Lanyesaj",
    "Azayal Ixasich",
    "Assossasda",
    "Sondassasta",
    "Mindim Lanak",
    "Sudecke Chadosia",
    "L'sal Chaj Izjen",
    "Assosiala",
    "Lassostisda",
    "Bloerdadarsya",
    "Chadosia",
    "Iskim Namaj",
    "Chamote Ajaqa",
    "Lirtilsa",
    "Undim Jiskistast",
    "Lirtosiala",
    "Frichassaya",
    "Undast Kabich",
    "Fril Ajich Lsosa",
    "Chados Azil Tzos",
    "Ixos Tzek Ajak",
};

void scroll_desc_shuffle()
{
    shuffle(scroll_desc_mapping, ST_MAX - 1, 1);
}

magic_scroll *scroll_new(int scroll_type)
{
    magic_scroll *nscroll;

    nscroll = g_malloc(sizeof(magic_scroll));
    assert(nscroll != NULL);

    nscroll->type = scroll_type;

    return(nscroll);
}

void scroll_destroy(magic_scroll *s)
{
	assert(s != NULL);

	g_free(s);
}

inline char *scroll_get_name(magic_scroll *s)
{
    assert(s != NULL && s->type > ST_NONE && s->type < ST_MAX);
    return scrolls[s->type].name;
}

inline char *scroll_get_desc(magic_scroll *s)
{
    assert(s != NULL && s->type > ST_NONE && s->type < ST_MAX);
    return (char *)scroll_desc[scroll_desc_mapping[s->type - 1]];
}

inline int scroll_get_effect(magic_scroll *s)
{
    assert(s != NULL && s->type > ST_NONE && s->type < ST_MAX);
    return scrolls[s->type].effect_type;
}

inline int scroll_get_price(magic_scroll *s)
{
    assert(s != NULL && s->type > ST_NONE && s->type < ST_MAX);
    return scrolls[s->type].price;
}
