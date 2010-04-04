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

#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "potions.h"

const potion_data potions[PO_MAX] =
{
    /* id               name                  effect            price obtainable */
    { PO_NONE,          "",                   ET_NONE,               0, FALSE },
    { PO_WATER,         "water",              ET_NONE,              20, FALSE },
    { PO_SLEEP,         "sleep",              ET_SLEEP,             50, FALSE },
    { PO_HEAL,          "healing",            ET_INC_HP,           100,  TRUE },
    { PO_INC_LEVEL,     "raise level",        ET_INC_LEVEL,        500, FALSE },
    { PO_INC_RND,       "increase ability",   ET_INC_RND,           50, FALSE },
    { PO_INC_STR,       "gain strength",      ET_INC_STR,          100, FALSE },
    { PO_LEARNING,      "learning",           ET_INC_INT,           50, FALSE },
    { PO_INC_WIS,       "gain wisdom",        ET_INC_WIS,           50, FALSE },
    { PO_INC_CON,       "sturdiness",         ET_INC_CON,          100, FALSE },
    { PO_INC_CHA,       "raise charisma",     ET_INC_CHA,           50, FALSE },
    { PO_DIZZINESS,     "dizziness",          ET_DIZZINESS,        100, FALSE },
    { PO_OBJ_DETECT,    "object detection",   ET_NONE,             100,  TRUE },
    { PO_MON_DETECT,    "monster detection",  ET_DETECT_MONSTER,   100,  TRUE },
    { PO_AMNESIA,       "forgetfulness",      ET_NONE,             100, FALSE },
    { PO_BLINDNESS,     "blindness",          ET_BLINDNESS,         50, FALSE },
    { PO_CONFUSION,     "confusion",          ET_CONFUSION,         50, FALSE },
    { PO_HEROISM,       "heroism",            ET_HEROISM,          500,  TRUE },
    { PO_GIANT_STR,     "giant strength",     ET_INC_STR,          200,  TRUE },
    { PO_FIRE_RES,      "fire resistance",    ET_RESIST_FIRE,      200, FALSE },
    { PO_TRE_DETECT,    "treasure finding",   ET_NONE,             100,  TRUE },
    { PO_MAX_HP,        "instant healing",    ET_MAX_HP,           500,  TRUE },
    { PO_POISON,        "poison",             ET_POISON,            50, FALSE },
    { PO_SEE_INVISIBLE, "see invisible",      ET_INFRAVISION,      200,  TRUE },
    { PO_CURE_DIANTHR,  "cure dianthroritis", ET_NONE,           10000, FALSE },
};

static int potion_with_effect(struct player *p, item *potion);
static int potion_amnesia(struct player *p, item *potion);
static int potion_detect_item(struct player *p, item *potion);

struct potion_obfuscation_s
{
    const char* desc;
    const int colour;
}
potion_obfuscation[PO_MAX - 1] =
{
    { "clear",          DC_WHITE,       },
    { "bubbly",         DC_LIGHTGRAY,   },
    { "clotted",        DC_DARKGRAY,    },
    { "smoky",          DC_LIGHTGRAY    },
    { "milky",          DC_WHITE,       },
    { "fizzy",          DC_LIGHTBLUE,   },
    { "murky",          DC_DARKGRAY,    },
    { "effervescent",   DC_LIGHTBLUE,   },
    { "dark",           DC_DARKGRAY,    },
    { "turbid",         DC_DARKGRAY,    },
    { "mucilaginous",   DC_BROWN,       },
    { "gluey",          DC_BROWN,       },
    { "gooey",          DC_BROWN,       },
    { "coagulated",     DC_DARKGRAY,    },
    { "white",          DC_WHITE,       },
    { "red",            DC_RED,         },
    { "blue",           DC_BLUE,        },
    { "green",          DC_GREEN        },
    { "yellow",         DC_YELLOW,      },
    { "orange",         DC_LIGHTRED,    },
    { "polychrome",     DC_LIGHTGREEN,  },
    { "dichroic",       DC_LIGHTMAGENTA,},
    { "tricoloured",    DC_LIGHTCYAN,   },
    { "black",          DC_DARKGRAY,    },
};

char *potion_desc(int potion_id)
{
    assert(potion_id > PO_NONE && potion_id < PO_MAX);
    return (char *)potion_obfuscation[nlarn->potion_desc_mapping[potion_id - 1]].desc;
}

int potion_colour(int potion_id)
{
    assert(potion_id > PO_NONE && potion_id < PO_MAX);
    return potion_obfuscation[nlarn->potion_desc_mapping[potion_id - 1]].colour;
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

        log_add_entry(p->log, "The potion is foul!");

        log_add_entry(p->log, "You spit gore!");
        player_damage_take(p, dam, PD_CURSE, potion->type);
        result.identified = FALSE;
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
        eff = effect_new(potion_effect(potion));

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
