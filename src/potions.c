/*
 * potions.c
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

const potion_data potions[PO_MAX] =
{
    /* id               name                  effect            price */
    { PO_NONE,          "",                   ET_NONE,              0 },
    { PO_WATER,         "water",              ET_NONE,             20 },
    { PO_SLEEP,         "sleep",              ET_SLEEP,            20 },
    { PO_HEAL,          "healing",            ET_INC_HP,           90 },
    { PO_INC_LEVEL,     "raise level",        ET_INC_LEVEL,       520 },
    { PO_INC_RND,       "increase ability",   ET_INC_RND,         100 },
    { PO_INC_STR,       "gain strength",      ET_INC_STR,         150 },
    { PO_LEARNING,      "learning",           ET_INC_INT,         200 },
    { PO_INC_WIS,       "gain wisdom",        ET_INC_WIS,          50 },
    { PO_INC_CON,       "sturdiness",         ET_INC_CON,          90 },
    { PO_INC_CHA,       "raise charisma",     ET_INC_CHA,          70 },
    { PO_DIZZINESS,     "dizziness",          ET_DIZZINESS,        30 },
    { PO_OBJ_DETECT,    "object detection",   ET_NONE,             50 },
    { PO_MON_DETECT,    "monster detection",  ET_DETECT_MONSTER,   80 },
    { PO_AMNESIA,       "forgetfulness",      ET_DEC_LEVEL,        30 }, /* FIXME: wrong effect? */
    { PO_BLINDNESS,     "blindness",          ET_BLINDNESS,        40 },
    { PO_CONFUSION,     "confusion",          ET_CONFUSION,        35 },
    { PO_HEROISM,       "heroism",            ET_HEROISM,         520 },
    { PO_GIANT_STR,     "giant strength",     ET_INC_STR,         200 }, /* value is only temporary */
    { PO_FIRE_RES,      "fire resistance",    ET_FIRE_RESISTANCE, 220 },
    { PO_TRE_DETECT,    "treasure finding",   ET_NONE,             80 },
    { PO_MAX_HP,        "instant healing",    ET_MAX_HP,          370 },
    { PO_POISON,        "poison",             ET_POISON,           50 },
    { PO_SEE_INVISIBLE, "see invisible",      ET_INFRAVISION,     150 },
    { PO_CURE_DIANTHR,  "cure dianthroritis", ET_NONE,              0 },
};

static int potion_desc_mapping[PO_MAX - 1] = { 0 };

static const char *_potion_desc[PO_MAX - 1] =
{
    "clear",
    "bubbly",
    "clotted",
    "smoky",
    "milky",
    "fizzy",
    "murky",
    "effervescent",
    "dark",
    "turbid",
    "mucilaginous",
    "gluey",
    "gooey",
    "coagulated",
    "white",
    "red",
    "blue",
    "green",
    "yellow",
    "orange",
    "polychrome",
    "dichroic",
    "tricoloured",
    "black",
};

void potion_desc_shuffle()
{
    shuffle(potion_desc_mapping, PO_MAX - 1, 1);
}

char *potion_desc(int potion_id)
{
    assert(potion_id > PO_NONE && potion_id < PO_MAX);
    return (char *)_potion_desc[potion_desc_mapping[potion_id - 1]];
}

int potion_with_effect(struct player *p, item *potion)
{
    int identified = TRUE;
    effect *eff;

    assert(p != NULL && potion != NULL);

    if (potion_effect(potion) > ET_NONE)
    {
        eff = effect_new(potion_effect(potion), game_turn(p->game));

        /* silly potion of giant strength */
        if (potion->id == PO_GIANT_STR)
        {
            eff->turns = divert(250, 20);
            eff->amount = 10;
        }

        /* this has to precede p_e_add as eff might be destroyed */
        if (!effect_get_msg_start(eff))
        {
            identified = FALSE;
        }

        player_effect_add(p, eff);
    }

    return identified;
}

int potion_detect_item(player *p, item *potion)
{
    position pos;
    int i;
    int count = 0; /* count detected items */
    inventory *inv;
    item *it;

    assert(p != NULL && potion != NULL);

    if (potion->id == PO_TRE_DETECT)
    {
        log_add_entry(p->log, "You sense the presence of treasure.");
    }
    else
    {
        log_add_entry(p->log, "You sense the presence of objects.");
    }

    for (pos.y = 0; pos.y < LEVEL_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < LEVEL_MAX_X; pos.x++)
        {
            if ((inv = level_ilist_at(p->level, pos)))
            {
                for (i = 0; i <  inv_length(inv); i++)
                {
                    it = inv_get(inv, i);

                    if (potion->id == PO_TRE_DETECT)
                    {
                        if ((it->type == IT_GOLD) || (it->type == IT_GEM))
                        {
                            player_memory_of(p, pos).item = it->type;
                            count++;
                        }
                    }
                    else
                    {
                        if ((it->type != IT_GOLD) && (it->type != IT_GEM))
                        {
                            player_memory_of(p, pos).item = it->type;
                            count++;
                        }
                    }
                }
            }
        }
    }

    if (count && (potion->id == PO_TRE_DETECT))
    {
        log_add_entry(p->log, "You sense the presence of treasure.");
    }
    else if (count && (potion->id == PO_OBJ_DETECT))
    {
        log_add_entry(p->log, "You sense the presence of objects.");
    }

    return count;
}
