/*
 * potions.c
 * Copyright (C) 2009-2026 Joachim de Groot <jdegroot@web.de>
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

#include <glib.h>
#include <glib/gi18n.h>

#include "colours.h"
#include "display.h"
#include "game.h"
#include "extdefs.h"
#include "player.h"
#include "potions.h"
#include "random.h"

DEFINE_ENUM(potion_t, POTION_TYPE_ENUM)

const potion_data potions[PO_MAX] =
{
    /* id               name                  effect            price store_stock */
    { PO_WATER,         N_("holy water"),         ET_NONE,             500, 0 },
    { PO_SLEEP,         N_("sleep"),              ET_SLEEP,             50, 0 },
    { PO_HEAL,          N_("healing"),            ET_INC_HP,           100, 0 },
    { PO_INC_LEVEL,     N_("raise level"),        ET_INC_LEVEL,        500, 0 },
    { PO_INC_RND,       N_("increase ability"),   ET_INC_RND,           50, 0 },
    { PO_INC_STR,       N_("gain strength"),      ET_INC_STR,          100, 0 },
    { PO_LEARNING,      N_("learning"),           ET_INC_INT,           50, 0 },
    { PO_INC_WIS,       N_("gain wisdom"),        ET_INC_WIS,           50, 0 },
    { PO_INC_CON,       N_("sturdiness"),         ET_INC_CON,          100, 0 },
    { PO_RECOVERY,      N_("recovery"),           ET_NONE,             200, 0 },
    { PO_DIZZINESS,     N_("dizziness"),          ET_DIZZINESS,        100, 0 },
    { PO_OBJ_DETECT,    N_("object detection"),   ET_NONE,             100, 2 },
    { PO_MON_DETECT,    N_("monster detection"),  ET_DETECT_MONSTER,   100, 2 },
    { PO_AMNESIA,       N_("forgetfulness"),      ET_NONE,             100, 0 },
    { PO_BLINDNESS,     N_("blindness"),          ET_BLINDNESS,         50, 0 },
    { PO_CONFUSION,     N_("confusion"),          ET_CONFUSION,         50, 0 },
    { PO_HEROISM,       N_("heroism"),            ET_HEROISM,          500, 0 },
    { PO_GIANT_STR,     N_("giant strength"),     ET_INC_STR,          200, 0 },
    { PO_FIRE_RES,      N_("fire resistance"),    ET_RESIST_FIRE,      200, 0 },
    { PO_TRE_DETECT,    N_("treasure finding"),   ET_NONE,             100, 1 },
    { PO_MAX_HP,        N_("instant healing"),    ET_MAX_HP,           500, 0 },
    { PO_INC_MP,        N_("power"),              ET_INC_MP,           200, 5 },
    { PO_POISON,        N_("poison"),             ET_POISON,            50, 0 },
    { PO_SEE_INVISIBLE, N_("see invisible"),      ET_INFRAVISION,      200, 1 },
    { PO_LEVITATION,    N_("levitation"),         ET_LEVITATION,       200, 0 },
    { PO_CURE_DIANTHR,  N_("cure dianthroritis"), ET_NONE,           10000, 0 },
};

static int potion_with_effect(struct player *p, item *potion);
static int potion_amnesia(struct player *p, item *potion);
static int potion_detect_item(struct player *p, item *potion);
static int potion_recovery(struct player *p, item *potion);
static int potion_holy_water(player *p, item *potion);

struct potion_obfuscation_s
{
    const char* desc;
    const colour_t fg;
}
potion_obfuscation[PO_MAX] =
{
    { N_("clear"),          ICE_COLD_GREEN,  },
    { N_("bubbly"),         MANGO_ORANGE,    },
    { N_("clotted"),        NATURE_APRICOT,  },
    { N_("smoky"),          ASH_GREY,        },
    { N_("milky"),          BUTTER,          },
    { N_("fizzy"),          AQUAMARINE,      },
    { N_("murky"),          GLADIOLA_BLUE,   },
    { N_("effervescent"),   AZUL,            },
    { N_("dark"),           GRAVEL,          },
    { N_("turbid"),         CLAM_SHELL,      },
    { N_("mucilaginous"),   TEALISH_GREEN,   },
    { N_("gluey"),          BRINK_PINK,      },
    { N_("gooey"),          PALE_PURPLE,     },
    { N_("coagulated"),     PALE_OAK,        },
    { N_("white"),          WHITE,           },
    { N_("red"),            LUMINOUS_RED,    },
    { N_("blue"),           BRIGHT_BLUE,     },
    { N_("green"),          LIME_JUICE_GREEN },
    { N_("yellow"),         SULFUR_YELLOW,   },
    { N_("orange"),         LUMINOUS_ORANGE, },
    { N_("polychrome"),     FRESH_GREEN,     },
    { N_("dichroic"),       PERSIAN_PINK,    },
    { N_("tricoloured"),    CANDY_PINK,      },
    { N_("black"),          BALTIC_SEA,      },
    { N_("turquoise"),      TURQUOISE_BLUE,  },
    { N_("foaming"),        FRESH_GREEN,     },
};

char *potion_desc(potion_t potion_id)
{
    g_assert(potion_id < PO_MAX);
    return (char *)_(potion_obfuscation[nlarn->potion_desc_mapping[potion_id]].desc);
}

colour_t potion_colour(potion_t potion_id)
{
    g_assert(potion_id < PO_MAX);
    return potion_obfuscation[nlarn->potion_desc_mapping[potion_id]].fg;
}

item_usage_result potion_quaff(struct player *p, item *potion)
{
    item_usage_result result = { false, false };

    // These potions aren't drunk.
    bool drunk = !(potion->id == PO_CURE_DIANTHR || potion->id == PO_WATER);

    if (potion->cursed && potion->blessed_known)
    {
        log_add_entry(nlarn->log, drunk
                ? _("You'd rather not drink this cursed potion.")
                : _("You'd rather not use this cursed potion."));
        return result;
    }

    /* prepare item description */
    gchar *description = item_describe(potion, player_item_known(p, potion),
                                       true, potion->count == 1);

    log_add_entry(nlarn->log, drunk ? _("You drink %s.") : _("You use %s."),
                  description);

    /* try to complete quaffing the potion */
    if (!player_make_move(p, 2, true,
                          drunk ? _("drinking %s") : _("using %s"), description))
    {
        /* the action has been aborted */
        g_free(description);
        return result;
    }

    g_free(description);

    /* the potion has successfully been quaffed */
    result.used_up = true;

    if (potion->cursed)
    {
        damage *dam = damage_new(DAM_POISON, ATT_NONE, rand_1n(p->hp),
                                 DAMO_ITEM, NULL);

        log_add_entry(nlarn->log, _("The potion is foul!"));

        log_add_entry(nlarn->log, _("You spit gore!"));
        player_damage_take(p, dam, PD_CURSE, potion->type);
    }
    else
    {
        effect *e;

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
            result.identified = true;
            result.used_up = potion_holy_water(p, potion);
            break;

        case PO_RECOVERY:
            result.identified = potion_recovery(p, potion);
            break;

        case PO_MAX_HP:
            result.identified = potion_with_effect(p, potion);

            // Potions of instant healing can cure poisoning and sickness
            if ((e = player_effect_get(nlarn->p, ET_SICKNESS)))
            {
                player_effect_del(nlarn->p, e);
                result.identified = true;
            }
            if ((e = player_effect_get(nlarn->p, ET_POISON)))
            {
                player_effect_del(nlarn->p, e);
                result.identified = true;
            }
            break;

        case PO_SEE_INVISIBLE:
            // The potion of see invisible can cure blindness,
            // but then doesn't grant invisibility.
            if ((e = player_effect_get(nlarn->p, ET_BLINDNESS)))
            {
                player_effect_del(nlarn->p, e);
                result.identified = true;
            } else {
                result.identified = potion_with_effect(p, potion);
            }
            break;

        case PO_CURE_DIANTHR:
            log_add_entry(nlarn->log, _("You really want to keep the potion for your daughter."));
            result.used_up = false;
            break;

        default:
            result.identified = potion_with_effect(p, potion);
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
    int identified = true;

    g_assert(p != NULL && potion != NULL);

    if (potion_effect(potion) > ET_NONE)
    {
        effect *eff = effect_new(potion_effect(potion));
        /* allow referencing the effect back to the potion */
        eff->item = potion->oid;

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

        /* this has to precede player_effect_add() as eff might be destroyed
           (e.g. potion of sleep) */
        if (!effect_get_msg_start(eff))
            identified = false;

        player_effect_add(p, eff);
    }

    return identified;
}

static int potion_amnesia(player *p, item *potion __attribute__((unused)))
{
    position pos = pos_invalid;

    g_assert (p != NULL);

    /* set position's level to player's position */
    Z(pos) = Z(p->pos);

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            player_memory_of(p, pos).type = LT_NONE;
            player_memory_of(p, pos).sobject = LS_NONE;
            player_memory_of(p, pos).item = IT_NONE;
            player_memory_of(p, pos).trap = TT_NONE;
        }
    }

    log_add_entry(nlarn->log, _("You stagger for a moment..."));

    return true;
}

static int potion_detect_item(player *p, item *potion)
{
    position pos = pos_invalid;
    int count = 0; /* count detected items */
    inventory *inv;

    g_assert(p != NULL && potion != NULL);

    Z(pos) = Z(p->pos);
    map *pmap = game_map(nlarn, Z(pos));

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            bool found_item = false;
            if ((inv = *map_ilist_at(pmap, pos)))
            {
                for (guint idx = 0; idx < inv_length(inv); idx++)
                {
                    item *it = inv_get(inv, idx);

                    if (potion->id == PO_TRE_DETECT)
                    {
                        if ((it->type == IT_GOLD) || (it->type == IT_GEM))
                        {
                            player_memory_of(p, pos).item = it->type;
                            player_memory_of(p, pos).item_colour = item_colour(it);
                            found_item = true;
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
                            found_item = true;
                            count++;
                            break;
                        }
                    }
                }
            }

            /* blessed potions also detect items carried by monsters */
            if (!found_item && potion->blessed)
            {
                monster *m = map_get_monster_at(pmap, pos);

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
                    for (item_t type = IT_AMULET; type < IT_MAX; type++)
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
                    player_memory_of(p, pos).item_colour = DOVE_GREY;
                    count++;
                }
            }
        }
    }

    if (pmap->nlevel == 5 && potion->id == PO_TRE_DETECT)
    {
        pos = map_find_sobject(pmap, LS_BANK2);
        player_memory_of(p, pos).sobject = LS_BANK2;
    }

    if (count && (potion->id == PO_TRE_DETECT))
    {
        log_add_entry(nlarn->log, _("You sense the presence of treasure."));
    }
    else if (count && (potion->id == PO_OBJ_DETECT))
    {
        log_add_entry(nlarn->log, _("You sense the presence of objects."));
    }

    return count;
}

static int potion_recovery(player *p, item *potion __attribute__((unused)))
{
    g_assert (p != NULL);

    bool success = false;
    effect *e;

    for (effect_t et = ET_DEC_CON; et <= ET_DEC_WIS; et++)
    {
        if ((e = player_effect_get(p, et)))
        {
            player_effect_del(p, e);
            success = true;
        }
    }
    if ((e = player_effect_get(p, ET_DIZZINESS)))
    {
        player_effect_del(p, e);
        success = true;
    }

    if (success)
        log_add_entry(nlarn->log, _("You feel more capable."));

    return success;
}

static int potion_holy_water(player *p, item *potion __attribute__((unused)))
{
    g_assert (p != NULL);

    if (inv_length_filtered(p->inventory, item_filter_nonblessed) == 0)
    {
        /* player has no cursed items */
        log_add_entry(nlarn->log, _("You're not carrying any items in need of a "
                                  "blessing."));
        return false;
    }

    item *it = display_inventory(_("Choose an item to bless"), p, &p->inventory,
                                 NULL, false, false, false, item_filter_nonblessed);

    if (it != NULL)
    {
        // Get the description before blessing the item.
        gchar *buf = item_describe(it, player_item_known(p, it), false, true);

        if (it->blessed)
        {
            it->blessed_known = true;
            log_add_entry(nlarn->log, it->count == 1
                          ? _("Nothing happens. Apparently, %s was "
                              "already blessed.")
                          : _("Nothing happens. Apparently, %s were "
                              "already blessed."), buf);

            g_free(buf);
            return true;
        }

        buf[0] = g_ascii_toupper(buf[0]);

        log_add_entry(nlarn->log, it->count == 1
                      ? _("%s glows in a white light.")
                      : _("%s glow in a white light."), buf);

        g_free(buf);

        if (it->cursed)
            it->cursed = false;
        else if (!it->blessed)
            it->blessed = true;

        return true;
    }
    return false;
}

bool potion_pos_hit(const GList *traj,
    const damage_originator *damo __attribute__((unused)),
    gpointer data1,
    gpointer data2 __attribute__((unused)))
{
    item *potion = (item *)data1;
    position pos; pos_val(pos) = GPOINTER_TO_UINT(traj->data);
    map *pmap = game_map(nlarn, Z(pos));
    map_tile_t mtt = map_tiletype_at(pmap, pos);
    sobject_t mst = map_sobject_at(pmap, pos);
    monster *m = map_get_monster_at(pmap, pos);

    /* get the description of the potion */
    gchar *desc = item_describe(potion, player_item_known(nlarn->p, potion), true, true);
    /* upper case first letter of the description */
    desc[0] = g_ascii_toupper(desc[0]);

    if (mst > LS_NONE && !so_is_passable(mst))
    {
        /* The potion hit a sobject. */
        log_add_entry(nlarn->log, _("%s shatters at %s."),
                      desc, so_get_desc(mst));

        map_spill_set(pmap, pos, potion_colour(potion->id));
    }
    else if (!map_pos_passable(pmap, pos))
    {
        /* The potion hit a wall or something similar. */
        log_add_entry(nlarn->log, (mtt <= LT_FLOOR
                      ? _("%s shatters on the %s.")
                      : _("%s splashes into the %s.")), desc,
                      mt_get_desc(mtt));

        if (mtt <= LT_FLOOR)
        {
            map_spill_set(pmap, pos, potion_colour(potion->id));
        }
    }
    else if (m != NULL)
    {
        /* there is a monster at the position the potion might have hit */
        if (!chance(combat_chance_player_to_monster_hit(nlarn->p, m, false)))
        {
            g_free(desc);
            return false;
        }

        log_add_entry(nlarn->log, _("%s shatters on the %s."),
                  desc, monster_get_name(m));

        if (potion_effect(potion))
            monster_effect_add(m, effect_new(potion_effect(potion)));

        if (potion->id == PO_WATER && potion->blessed && monster_flags(m, UNDEAD))
        {
            /* this is supposed to hurt really nasty */
            log_add_entry(nlarn->log, _("Smoke emerges where %s pours over the %s."),
                          desc, monster_get_name(m));

            damage *dam = damage_new(DAM_PHYSICAL, ATT_TOUCH, rand_1n(monster_hp(m) + 1),
                                     DAMO_PLAYER, nlarn->p);

            monster_damage_take(m, dam);
        }
    }
    else if (pos_identical(nlarn->p->pos, pos))
    {
        /* TODO: potion hit the player */
    }
    else
    {
        /* the potion passed unhindered */
        g_free(desc);
        return false;
    }

    item_destroy(potion);
    g_free(desc);

    return true;
}
