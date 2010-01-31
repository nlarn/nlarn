/*
 * potions.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

#include <assert.h>

#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "potions.h"

const potion_data potions[PO_MAX] =
{
    /* id               name                  effect            price obtainable */
    { PO_NONE,          "",                   ET_NONE,              0, FALSE },
    { PO_WATER,         "water",              ET_NONE,             20, FALSE },
    { PO_SLEEP,         "sleep",              ET_SLEEP,            20, FALSE },
    { PO_HEAL,          "healing",            ET_INC_HP,           90,  TRUE },
    { PO_INC_LEVEL,     "raise level",        ET_INC_LEVEL,       520, FALSE },
    { PO_INC_RND,       "increase ability",   ET_INC_RND,         100, FALSE },
    { PO_INC_STR,       "gain strength",      ET_INC_STR,         150, FALSE },
    { PO_LEARNING,      "learning",           ET_INC_INT,         200, FALSE },
    { PO_INC_WIS,       "gain wisdom",        ET_INC_WIS,          50, FALSE },
    { PO_INC_CON,       "sturdiness",         ET_INC_CON,          90, FALSE },
    { PO_INC_CHA,       "raise charisma",     ET_INC_CHA,          70, FALSE },
    { PO_DIZZINESS,     "dizziness",          ET_DIZZINESS,        30, FALSE },
    { PO_OBJ_DETECT,    "object detection",   ET_NONE,             50,  TRUE },
    { PO_MON_DETECT,    "monster detection",  ET_DETECT_MONSTER,   80,  TRUE },
    { PO_AMNESIA,       "forgetfulness",      ET_NONE,             30, FALSE },
    { PO_BLINDNESS,     "blindness",          ET_BLINDNESS,        40, FALSE },
    { PO_CONFUSION,     "confusion",          ET_CONFUSION,        35, FALSE },
    { PO_HEROISM,       "heroism",            ET_HEROISM,         520,  TRUE },
    { PO_GIANT_STR,     "giant strength",     ET_INC_STR,         200,  TRUE },
    { PO_FIRE_RES,      "fire resistance",    ET_RESIST_FIRE,     220, FALSE },
    { PO_TRE_DETECT,    "treasure finding",   ET_NONE,             80,  TRUE },
    { PO_MAX_HP,        "instant healing",    ET_MAX_HP,          370,  TRUE },
    { PO_POISON,        "poison",             ET_POISON,           50, FALSE },
    { PO_SEE_INVISIBLE, "see invisible",      ET_INFRAVISION,     150,  TRUE },
    { PO_CURE_DIANTHR,  "cure dianthroritis", ET_NONE,              0, FALSE },
};

static int potion_with_effect(struct player *p, item *potion);
static int potion_amnesia(struct player *p, item *potion);
static int potion_detect_item(struct player *p, item *potion);

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

char *potion_desc(int potion_id)
{
    assert(potion_id > PO_NONE && potion_id < PO_MAX);
    return (char *)_potion_desc[nlarn->potion_desc_mapping[potion_id - 1]];
}

item_usage_result potion_quaff(struct player *p, item *potion)
{
    item_usage_result result;
    char description[61];

    item_describe(potion, player_item_known(p, potion),
                  TRUE, FALSE, description, 60);

    log_add_entry(p->log, "You drink %s.", description);

    /* increase number of potions quaffed */
    p->stats.potions_quaffed++;

    result.time = 2;
    result.identified = TRUE;
    result.used_up = TRUE;

    if (potion->cursed)
    {
        damage *dam = damage_new(DAM_POISON, ATT_NONE, rand_1n(p->hp), NULL);

        log_add_entry(p->log, "The Potion is foul!");

        log_add_entry(p->log, "You spit gore!");
        player_damage_take(p, dam, PD_CURSE, potion->type);
    }
    else
    {

        switch (potion->id)
        {
        case PO_AMNESIA:
            result.identified = potion_amnesia(p, potion);
            break;

        case PO_OBJ_DETECT:
        case PO_TRE_DETECT:
            result.identified = potion_detect_item(p, potion);
            break;

        case PO_WATER:
            log_add_entry(p->log, "This tastes like water..");
            break;

        case PO_CURE_DIANTHR:
            log_add_entry(p->log, "You really want to keep the potion for your daughter.");
            result.used_up = FALSE;
            break;

        default:
            result.identified = potion_with_effect(p, potion);
            break;
        }
    }

    return result;
}

static int potion_with_effect(struct player *p, item *potion)
{
    int identified = TRUE;
    effect *eff;

    assert(p != NULL && potion != NULL);

    if (potion_effect(potion) > ET_NONE)
    {
        eff = effect_new(potion_effect(potion), game_turn(nlarn));

        /* silly potion of giant strength */
        if (potion->id == PO_GIANT_STR)
        {
            eff->turns = divert(250, 20);
            eff->amount = 10;
        }

        /* this has to precede p_e_add as eff might be destroyed (e.g. potion of sleep) */
        if (!effect_get_msg_start(eff))
        {
            identified = FALSE;
        }

        player_effect_add(p, eff);
    }

    return identified;
}

static int potion_amnesia(player *p, item *potion)
{
    position pos;

    assert (p != NULL && potion != NULL);

    /* set position's level to player's position */
    pos.z = p->pos.z;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            player_memory_of(p, pos).type = LT_NONE;
            player_memory_of(p, pos).sobject = LS_NONE;
            player_memory_of(p, pos).item = LT_NONE;
            player_memory_of(p, pos).trap = LT_NONE;
        }
    }

    log_add_entry(p->log, "You stagger for a moment...");

    return TRUE;
}

static int potion_detect_item(player *p, item *potion)
{
    position pos;
    guint idx;
    int count = 0; /* count detected items */
    inventory *inv;
    item *it;

    assert(p != NULL && potion != NULL);

    pos.z = p->pos.z;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            if ((inv = *map_ilist_at(game_map(nlarn, p->pos.z), pos)))
            {
                for (idx = 0; idx <  inv_length(inv); idx++)
                {
                    it = inv_get(inv, idx);

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
