/*
 * potions.c
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
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
    { PO_WATER,         "holy water",         ET_NONE,             500, FALSE },
    { PO_SLEEP,         "sleep",              ET_SLEEP,             50, FALSE },
    { PO_HEAL,          "healing",            ET_INC_HP,           100, FALSE },
    { PO_INC_LEVEL,     "raise level",        ET_INC_LEVEL,        500, FALSE },
    { PO_INC_RND,       "increase ability",   ET_INC_RND,           50, FALSE },
    { PO_INC_STR,       "gain strength",      ET_INC_STR,          100, FALSE },
    { PO_LEARNING,      "learning",           ET_INC_INT,           50, FALSE },
    { PO_INC_WIS,       "gain wisdom",        ET_INC_WIS,           50, FALSE },
    { PO_INC_CON,       "sturdiness",         ET_INC_CON,          100, FALSE },
    { PO_RECOVERY,      "recovery",           ET_NONE,             200, FALSE },
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
    { PO_INC_MP,        "power",              ET_INC_MP,           200,  TRUE },
    { PO_POISON,        "poison",             ET_POISON,            50, FALSE },
    { PO_SEE_INVISIBLE, "see invisible",      ET_INFRAVISION,      200,  TRUE },
    { PO_LEVITATION,    "levitation",         ET_LEVITATION,       200, FALSE },
    { PO_CURE_DIANTHR,  "cure dianthroritis", ET_NONE,           10000, FALSE },
};

static int potion_with_effect(struct player *p, item *potion);
static int potion_amnesia(struct player *p, item *potion);
static int potion_detect_item(struct player *p, item *potion);
static int potion_recovery(struct player *p, item *potion);
static int potion_holy_water(player *p, item *potion);
static void potion_monster_hit(item *potion, monster *m);

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
    { "turquoise",      DC_CYAN,        },
    { "foaming",        DC_WHITE,       },
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

int potion_throw(struct player *p)
{
    map *map = game_map(nlarn, Z(p->pos));
    position target;             /* the selected target */
    area *ray = NULL;            /* the path of the bullet */
    char desc[81] = {};          /* the potion's description */
    item *potion;                /* the potion from the inventory */
    monster *m = NULL;           /* the targeted monster */

    if (inv_length_filtered(p->inventory, item_filter_potions) == 0)
    {
        log_add_entry(nlarn->log, "You do not have any potions.");
        return FALSE;
    }

    potion = display_inventory("Select a potion to throw", p, &p->inventory,
                               NULL, FALSE, FALSE, FALSE, item_filter_potions);

    if (!potion)
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    /* get the description of the potion */
    item_describe(potion, player_item_known(p, potion), TRUE, TRUE, desc, 80);

    gchar *msg = g_strdup_printf("Choose a target for %s.", desc);
    target = display_get_position(p, msg, TRUE, FALSE, 0, FALSE, TRUE);
    g_free(msg);

    if (!pos_valid(target))
    {
        /* the player pressed ESC */
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    /* get the targeted monster  */
    m = map_get_monster_at(map, target);

    /* check if there is a monster at the targeted position */
    if (!m || !monster_in_sight(m))
    {
        log_add_entry(nlarn->log, "I see no monster there.");
        return FALSE;
    }

    /* protect townsfolk from agressive players */
    if (monster_type(m) == MT_TOWN_PERSON)
    {
        log_add_entry(nlarn->log, "Gosh! How dare you!");
        return FALSE;
    }

    /* determine trajectory of the potion */
    area *obstacles = map_get_obstacles(map, p->pos, pos_distance(p->pos, target));
    ray = area_new_ray(p->pos, target, obstacles);

    if (ray == NULL)
    {
        /* it's not possible to draw a line from the player's position to the target */
        log_add_entry(nlarn->log, "That wont work..");
        return FALSE;
    }

    /* get the actual item to throw */
    if (potion->count > 1)
    {
        /* potion is actually a stack of potions => get one of them */
        potion = item_split(potion, 1);
    }
    else
    {
        /* delete the potion from the inventory */
        inv_del_element(&p->inventory, potion);
    }

    /* upper case the first letter of the description */
    desc[0] = g_ascii_toupper(desc[0]);

    /* follow the ray to determine if the bullet hits something else */
    position previous = p->pos, cursor = p->pos;
    int start_x, start_y, stop_x, stop_y;
    start_x = min(X(p->pos), X(target));
    start_y = min(Y(p->pos), Y(target));
    stop_x  = max(X(p->pos), X(target));
    stop_y  = max(Y(p->pos), Y(target));

    for(Y(cursor) = start_y; Y(cursor) <= stop_y; Y(cursor)++)
    {
        for(X(cursor) = start_x; X(cursor) <= stop_x; X(cursor)++)
        {
            monster *drive_by = NULL;

            /* check if the current position is part of the ray */
            if (!area_pos_get(ray, cursor))
                continue;

            if (!map_pos_passable(map, cursor))
            {
                /* the potion hit something - destroy it */
                map_tile_t mtt = map_tiletype_at(map, cursor);
                map_sobject_t mst = map_sobject_at(map, cursor);

                if (mst > LS_NONE && !ls_is_passable(mst))
                {
                    log_add_entry(nlarn->log, "%s shatters at %s.",
                                  ls_get_desc(mst));
                }
                else
                {
                    log_add_entry(nlarn->log, "%s %s %s.", desc,
                                  (mtt <= LT_FLOOR ? "shatters on the" : "splashes into the"),
                                  lt_get_desc(mtt));
                }

                return TRUE;
            }

            /* check if there is a monster at the position the bullet passes */
            if ((drive_by = map_get_monster_at(map, cursor)))
            {
                /* bullet might have hit the other monster */
                if (chance(weapon_calc_to_hit(p, drive_by, NULL, NULL)))
                {
                    potion_monster_hit(potion, drive_by);
                    return TRUE;
                }
            }

            /* bullet moves unhindered */
            previous = cursor;
        }
    }

    /* arrived at the target position */
    if (chance(weapon_calc_to_hit(p, m, NULL, NULL)))
    {
        potion_monster_hit(potion, m);
    }
    else
    {
        map_tile_t mtt = map_tiletype_at(map, target);
        log_add_entry(nlarn->log, "%s misses the %s and %s the %s.", desc, monster_name(m),
                      (mtt <= LT_FLOOR ? "shatters on" : "splashes into"), lt_get_desc(mtt));

        item_destroy(potion);
    }

    return TRUE;
}

item_usage_result potion_quaff(struct player *p, item *potion)
{
    item_usage_result result = { FALSE, FALSE };
    char *verb;
    char description[61];

    // These potions aren't drunk.
    if (potion->id == PO_CURE_DIANTHR || potion->id == PO_WATER)
        verb = "use";
    else
        verb = "drink";

    if (potion->cursed && potion->blessed_known)
    {
        log_add_entry(nlarn->log, "You'd rather not %s this cursed potion.", verb);
        return result;
    }

    /* prepare item description */
    item_describe(potion, player_item_known(p, potion),
                  TRUE, potion->count == 1, description, 60);

    log_add_entry(nlarn->log, "You %s %s.", verb, description);

    /* try to complete quaffing the potion */
    if (!player_make_move(p, 2, TRUE, "%sing %s", verb, description))
    {
        /* the action has been aborted */
        return result;
    }

    /* the potion has successfully been quaffed */
    result.used_up = TRUE;

    if (potion->cursed)
    {
        damage *dam = damage_new(DAM_POISON, ATT_NONE, rand_1n(p->hp), NULL);

        log_add_entry(nlarn->log, "The potion is foul!");

        log_add_entry(nlarn->log, "You spit gore!");
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
            result.identified = TRUE;
            result.used_up = potion_holy_water(p, potion);
            break;

        case PO_RECOVERY:
            result.identified = potion_recovery(p, potion);
            break;

        case PO_CURE_DIANTHR:
            log_add_entry(nlarn->log, "You really want to keep the potion for your daughter.");
            result.used_up = FALSE;
            break;

        default:
            result.identified = potion_with_effect(p, potion);

            if (potion->id == PO_MAX_HP && potion->blessed)
            {
                int done = FALSE;
                effect *e;
                if ((e = player_effect_get(nlarn->p, ET_POISON)))
                {
                    player_effect_del(nlarn->p, e);
                    done = TRUE;
                }
                if ((e = player_effect_get(nlarn->p, ET_CONFUSION)))
                {
                    player_effect_del(nlarn->p, e);
                    done = TRUE;
                }
                if ((e = player_effect_get(nlarn->p, ET_BLINDNESS)))
                {
                    player_effect_del(nlarn->p, e);
                    done = TRUE;
                }
                if (!result.identified)
                    result.identified = done;
            }
            break;
        }
    }

    if (result.used_up)
    {
        /* increase number of potions quaffed */
        p->stats.potions_quaffed++;
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

        // Blessed potions last longer or have a stronger effect.
        // And yes, this also holds for bad effects!
        if (potion->blessed)
        {
            if (eff->turns > 1)
                eff->turns *= 2;
            else
                eff->amount *= 2;
        }

        /* this has to precede p_e_add as eff might be destroyed (e.g. potion of sleep) */
        if (!effect_get_msg_start(eff))
            identified = FALSE;

        player_effect_add(p, eff);
    }

    return identified;
}

static int potion_amnesia(player *p, item *potion)
{
    position pos;

    assert (p != NULL && potion != NULL);

    /* set position's level to player's position */
    Z(pos) = Z(p->pos);

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            player_memory_of(p, pos).type = LT_NONE;
            player_memory_of(p, pos).sobject = LS_NONE;
            player_memory_of(p, pos).item = LT_NONE;
            player_memory_of(p, pos).trap = LT_NONE;
        }
    }

    log_add_entry(nlarn->log, "You stagger for a moment...");

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

    Z(pos) = Z(p->pos);
    map *map = game_map(nlarn, Z(pos));

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            gboolean found_item = FALSE;
            if ((inv = *map_ilist_at(map, pos)))
            {
                for (idx = 0; idx < inv_length(inv); idx++)
                {
                    it = inv_get(inv, idx);

                    if (potion->id == PO_TRE_DETECT)
                    {
                        if ((it->type == IT_GOLD) || (it->type == IT_GEM))
                        {
                            player_memory_of(p, pos).item = it->type;
                            player_memory_of(p, pos).item_colour = item_colour(it);
                            found_item = TRUE;
                            count++;
                            break;
                        }
                    }
                    else
                    {
                        if ((it->type != IT_GOLD) && (it->type != IT_GEM))
                        {
                            player_memory_of(p, pos).item = it->type;
                            player_memory_of(p, pos).item_colour = item_colour(it);
                            found_item = TRUE;
                            count++;
                            break;
                        }
                    }
                }
            }

            /* blessed potions also detect items carried by monsters */
            if (!found_item && potion->blessed)
            {
                monster *m = map_get_monster_at(map, pos);

                if (m == NULL)
                    continue;

                int item_type = IT_NONE;
                if (potion->id == PO_TRE_DETECT)
                {
                    if (monster_is_carrying_item(m, IT_GEM))
                        item_type = IT_GEM;
                    else if (monster_is_carrying_item(m, IT_GOLD))
                        item_type = IT_GOLD;
                }
                else
                {
                    item_t type = IT_NONE;
                    for (type = IT_AMULET; type < IT_MAX; type++)
                    {
                        if (type == IT_GOLD || type == IT_GEM)
                            continue;

                        if (monster_is_carrying_item(m, type))
                        {
                            item_type = type;
                            break;
                        }
                    }
                }

                if (item_type != IT_NONE)
                {
                    /* colour these differently from items that
                       don't move around */

                    player_memory_of(p, pos).item = item_type;
                    player_memory_of(p, pos).item_colour = DC_DARKGRAY;
                    count++;
                }
            }
        }
    }

    if (count && (potion->id == PO_TRE_DETECT))
    {
        log_add_entry(nlarn->log, "You sense the presence of treasure.");
    }
    else if (count && (potion->id == PO_OBJ_DETECT))
    {
        log_add_entry(nlarn->log, "You sense the presence of objects.");
    }

    return count;
}

static int potion_recovery(player *p, item *potion)
{
    assert (p != NULL && potion != NULL);

    gboolean success = FALSE;
    gint32 et;
    effect *e;
    for (et = ET_DEC_CON; et <= ET_DEC_WIS; et++)
    {
        if ((e = player_effect_get(p, et)))
        {
            player_effect_del(p, e);
            success = TRUE;
        }
    }
    if ((e = player_effect_get(p, ET_DIZZINESS)))
    {
        player_effect_del(p, e);
        success = TRUE;
    }

    if (success)
        log_add_entry(nlarn->log, "You feel more capable.");

    return success;
}

static int potion_holy_water(player *p, item *potion)
{
    char buf[61];
    item *it;

    assert (p != NULL && potion != NULL);

    if (inv_length_filtered(p->inventory, item_filter_nonblessed) == 0)
    {
        /* player has no cursed items */
        log_add_entry(nlarn->log, "You're not carrying any items in need of a "
                                  "blessing.");
        return FALSE;
    }

    it = display_inventory("Choose an item to bless", p, &p->inventory,
                           NULL, FALSE, FALSE, FALSE, item_filter_nonblessed);

    if (it != NULL)
    {
        // Get the description before blessing the item.
        item_describe(it, player_item_known(p, it),
                      FALSE, TRUE, buf, 60);

        if (it->blessed)
        {
            it->blessed_known = TRUE;
            log_add_entry(nlarn->log, "Nothing happens. Apparently, %s %s "
                                      "already blessed.",
                          buf, it->count == 1 ? "was" : "were");
            return TRUE;
        }

        buf[0] = g_ascii_toupper(buf[0]);

        log_add_entry(nlarn->log, "%s glow%s in a white light.",
                      buf, it->count == 1 ? "s" : "");

        if (it->cursed)
            it->cursed = FALSE;
        else if (!it->blessed)
            it->blessed = TRUE;

        return TRUE;
    }
    return FALSE;
}

static void potion_monster_hit(item *potion, monster *m)
{
    char desc[81] = {};
    effect *e = NULL;

    assert (potion != NULL && m != NULL);

    item_describe(potion, player_item_known(nlarn->p, potion), TRUE, TRUE, desc, 80);
    desc[0] = g_ascii_toupper(desc[0]);

    log_add_entry(nlarn->log, "%s shatters on the %s.",
                  desc, monster_get_name(m));

    if (potion_effect(potion))
        e = monster_effect_add(m, effect_new(potion_effect(potion)));

    if (potion_effect(potion) && !e)
    {
        /* the monster is resistant to the effect */
        log_add_entry(nlarn->log, "The %s is not affected.", monster_get_name(m));
    }

    if (potion->id == PO_WATER && potion->blessed && monster_flags(m, MF_UNDEAD))
    {
        /* this is supposed to hurt really nasty */
        log_add_entry(nlarn->log, "Smoke emerges where the water pours over the %s.",
                      monster_get_name(m));

        damage *dam = damage_new(DAM_PHYSICAL, ATT_TOUCH, rand_1n(monster_hp(m) + 1), nlarn->p);
        monster_damage_take(m, dam);
    }

    item_destroy(potion);
}
