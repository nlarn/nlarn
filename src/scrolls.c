/*
 * scrolls.c
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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
#include <stdlib.h>

#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "random.h"
#include "scrolls.h"

const magic_scroll_data scrolls[ST_MAX] =
{
    /* ID                   name                  effect               price store_stock */
    { ST_BLANK,             "blank paper",        ET_NONE,               100, 0 },
    { ST_ENCH_ARMOUR,       "enchant armour",     ET_NONE,               100, 1 },
    { ST_ENCH_WEAPON,       "enchant weapon",     ET_NONE,               100, 1 },
    { ST_ENLIGHTENMENT,     "enlightenment",      ET_ENLIGHTENMENT,      800, 0 },
    { ST_CREATE_MONSTER,    "create monster",     ET_NONE,               100, 0 },
    { ST_CREATE_ARTIFACT,   "create artifact",    ET_NONE,               400, 0 },
    { ST_AGGRAVATE_MONSTER, "aggravate monsters", ET_AGGRAVATE_MONSTER,  100, 0 },
    { ST_TIMEWARP,          "time warp",          ET_NONE,               800, 0 },
    { ST_TELEPORT,          "teleportation",      ET_NONE,               250, 3 },
    { ST_AWARENESS,         "expanded awareness", ET_AWARENESS,          250, 1 },
    { ST_SPEED,             "speed",              ET_SPEED,              250, 0 },
    { ST_HEAL_MONSTER,      "monster healing",    ET_NONE,               100, 0 },
    { ST_SPIRIT_PROTECTION, "spirit protection",  ET_SPIRIT_PROTECTION,  400, 0 },
    { ST_UNDEAD_PROTECTION, "undead protection",  ET_UNDEAD_PROTECTION,  400, 0 },
    { ST_STEALTH,           "stealth",            ET_STEALTH,            400, 0 },
    { ST_MAPPING,           "magic mapping",      ET_NONE,               250, 3 },
    { ST_HOLD_MONSTER,      "hold monsters",      ET_NONE,               800, 0 },
    { ST_GEM_PERFECTION,    "gem perfection",     ET_NONE,              3000, 0 },
    { ST_SPELL_EXTENSION,   "spell extension",    ET_NONE,               800, 0 },
    { ST_IDENTIFY,          "identify",           ET_NONE,               400, 3 },
    { ST_REMOVE_CURSE,      "remove curse",       ET_NONE,               250, 0 },
    { ST_ANNIHILATION,      "annihilation",       ET_NONE,              3000, 0 },
    { ST_PULVERIZATION,     "pulverization",      ET_NONE,               800, 0 },
    { ST_LIFE_PROTECTION,   "life protection",    ET_LIFE_PROTECTION,   3000, 0 },
    { ST_GENOCIDE_MONSTER,  "genocide monster",   ET_NONE,              3000, 0 },
};

static int scroll_with_effect(player *p, item *r_scroll);
static int scroll_annihilate(player *p, item *r_scroll);
static int scroll_create_artefact(player *p, item *r_scroll);
static int scroll_enchant_armour(player *p, item *r_scroll);
static int scroll_enchant_weapon(player *p, item *r_scroll);
static int scroll_gem_perfection(player *p, item *r_scroll);
static int scroll_genocide_monster(player *p, item *r_scroll);
static int scroll_heal_monster(player *p, item *r_scroll);
static int scroll_hold_monster(player *p, item *r_scroll);
static int scroll_identify(player *p, item *r_scroll);
static int scroll_remove_curse(player *p, item *r_scroll);
static int scroll_spell_extension(player *p, item *r_scroll);
static int scroll_teleport(player *p, item *r_scroll);
static int scroll_timewarp(player *p, item *r_scroll);

static const char *_scroll_desc[ST_MAX] =
{
    "",
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
    /* "Xodil Keterulo", spare label */
};

char *scroll_desc(scroll_t id)
{
    g_assert(id < ST_MAX);
    return (char *)_scroll_desc[nlarn->scroll_desc_mapping[id]];
}

item_usage_result scroll_read(struct player *p, item *r_scroll)
{
    item_usage_result result  = { FALSE, FALSE };

    gchar *desc = item_describe(r_scroll, player_item_known(p, r_scroll),
                                TRUE, FALSE);

    if (player_effect(p, ET_BLINDNESS))
    {
        log_add_entry(nlarn->log, "As you are blind you can't read %s.", desc);

        g_free(desc);
        return result;
    }

    if (r_scroll->cursed && r_scroll->blessed_known)
    {
        log_add_entry(nlarn->log, "You'd rather not read this cursed scroll.");

        g_free(desc);
        return result;
    }

    log_add_entry(nlarn->log, "You read %s.", desc);

    /* try to complete reading the scroll */
    if (!player_make_move(p, 2, TRUE, "reading %s", desc))
    {
        /* the action has been aborted */
        g_free(desc);
        return result;
    }

    g_free(desc);

    /* the scroll has successfully been read */
    result.used_up = TRUE;

    /* increase number of scrolls read */
    p->stats.scrolls_read++;

    if (r_scroll->cursed)
    {
        damage *dam = damage_new(DAM_FIRE, ATT_NONE, rand_1n(p->hp),
                                 DAMO_ITEM, NULL);

        log_add_entry(nlarn->log, "The scroll explodes!");
        player_damage_take(p, dam, PD_CURSE, r_scroll->type);

        return result;
    }

    switch (r_scroll->id)
    {
    case ST_ENCH_ARMOUR:
        result.identified = scroll_enchant_armour(p, r_scroll);
        break;

    case ST_ENCH_WEAPON:
        result.identified = scroll_enchant_weapon(p, r_scroll);
        break;

    case ST_BLANK:
        result.used_up = FALSE;
        result.identified = TRUE;
        log_add_entry(nlarn->log, "This scroll is blank.");
        break;

    case ST_CREATE_MONSTER:
        result.identified = spell_create_monster(p);
        break;

    case ST_CREATE_ARTIFACT:
        result.identified = scroll_create_artefact(p, r_scroll);
        break;

    case ST_TIMEWARP:
        result.identified = scroll_timewarp(p, r_scroll);
        break;

    case ST_TELEPORT:
        result.identified = scroll_teleport(p, r_scroll);
        break;

    case ST_HEAL_MONSTER:
        result.identified = scroll_heal_monster(p, r_scroll);
        break;

    case ST_MAPPING:
        log_add_entry(nlarn->log, "There is a map on the scroll!");
        result.identified = scroll_mapping(p, r_scroll);
        break;

    case ST_GEM_PERFECTION:
        result.identified = scroll_gem_perfection(p, r_scroll);
        break;

    case ST_SPELL_EXTENSION:
        result.identified = scroll_spell_extension(p, r_scroll);
        break;

    case ST_HOLD_MONSTER:
        result.identified = scroll_hold_monster(p, r_scroll);
        break;

    case ST_IDENTIFY:
        result.identified = scroll_identify(p, r_scroll);
        break;

    case ST_REMOVE_CURSE:
        result.identified = scroll_remove_curse(p, r_scroll);
        break;

    case ST_ANNIHILATION:
        result.identified = scroll_annihilate(p, r_scroll);
        break;

    case ST_PULVERIZATION:
        if (!p->identified_scrolls[ST_PULVERIZATION])
        {
            log_add_entry(nlarn->log, "This is a scroll of %s. ",
                          scroll_name(r_scroll));
        }

        if (spell_vaporize_rock(p))
        {
            /* recalc fov if something has been vaporised */
            player_update_fov(p);
        }

        result.identified = TRUE;
        break;

    case ST_GENOCIDE_MONSTER:
        scroll_genocide_monster(p, r_scroll);
        result.identified = TRUE;
        break;

    default:
        result.identified = scroll_with_effect(p, r_scroll);
        break;
    }

    if (!result.identified)
    {
        log_add_entry(nlarn->log, "Nothing happens.");
    }

    return result;
}

static int scroll_with_effect(struct player *p, item *r_scroll)
{
    effect *eff;

    g_assert(p != NULL && r_scroll != NULL);

    eff = effect_new(scroll_effect(r_scroll));
    // Blessed scrolls last longer.
    if (r_scroll->blessed)
    {
        // Life protection is permanent.
        if (r_scroll->id == ST_LIFE_PROTECTION)
            eff->turns = 0;
        else
            eff->turns *= 2;
    }

    eff = player_effect_add(p, eff);

    if (eff && !effect_get_msg_start(eff))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * Scroll "annihilation".
 *
 * @param the player
 * @param the scroll just read
 *
 */
static int scroll_annihilate(struct player *p, item *r_scroll __attribute__((unused)))
{
    g_assert(p != NULL);

    int count = 0;
    area *blast, *obsmap;
    position cursor = p->pos;
    monster *m;
    map *cmap = game_map(nlarn, Z(p->pos));

    obsmap = map_get_obstacles(cmap, p->pos, 2, FALSE);
    blast = area_new_circle_flooded(p->pos, 2, obsmap);

    for (Y(cursor) = blast->start_y; Y(cursor) < blast->start_y + blast->size_y; Y(cursor)++)
    {
        for (X(cursor) = blast->start_x; X(cursor) < blast->start_x + blast->size_x; X(cursor)++)
        {
            if (area_pos_get(blast, cursor) && (m = map_get_monster_at(cmap, cursor)))
            {
                if (monster_flags(m, MF_DEMON))
                {
                    m = monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_NONE,
                                                          2000, DAMO_PLAYER, p));

                    /* check if the monster has been killed */
                    if (!m) count++;
                }
                else
                {
                    log_add_entry(nlarn->log, "The %s barely escapes being annihilated.",
                                  monster_get_name(m));

                    /* lose half hit points */
                    damage *dam = damage_new(DAM_MAGICAL, ATT_NONE, monster_hp(m) / 2,
                                             DAMO_PLAYER, p);

                    monster_damage_take(m, dam);
                }
            }
        }
    }

    area_destroy(blast);

    if (count)
    {
        log_add_entry(nlarn->log, "You hear loud screams of agony!");
    }

    return count;
}

static int scroll_create_artefact(player *p, item *r_scroll)
{
    g_assert(p != NULL && r_scroll != NULL);

    const int magic_item_type[] = { IT_AMULET, IT_BOOK, IT_POTION, IT_SCROLL };
    const int type = magic_item_type[rand_0n(4)];

    int level = Z(p->pos);
    if (r_scroll->blessed)
    {
        if (type == IT_AMULET)
            level = max(10, Z(p->pos));
        else
            level = rand_m_n(Z(p->pos), MAP_MAX);
    }

    item *it = item_new_by_level(type, level);

    it->cursed = FALSE;
    if (it->bonus < 0)
        it->bonus = 0;

    // Blessed scrolls tend to hand out better items.
    if (r_scroll->blessed)
    {
        // Rings get some kind of bonus.
        if (it->bonus < 1 && item_is_optimizable(it->type))
            it->bonus = rand_1n(3);

        it->burnt    = 0;
        it->corroded = 0;
        it->rusty    = 0;

        if (it->type == IT_POTION || it->type == IT_SCROLL)
        {
            // The low-price, unobtainable items are more likely to be bad.
            while (item_base_price(it) < 200
                    && !item_obtainable(it->type, it->id))
            {
                it->id = rand_1n(item_max_id(it->type));
            }
            // holy water should always be blessed
            if (it->type == IT_POTION && it->id == PO_WATER && !it->blessed)
                it->blessed = TRUE;
        }
        else if (it->type == IT_BOOK)
        {
            // Roll again for an unknown, reasonably high-level book.
            while ((spell_known(p, it->id) && chance(80))
                    || (it->id < (item_max_id(IT_BOOK) * level) / (MAP_MAX - 1)
                        && chance(50)))
            {
                it->id = rand_1n(item_max_id(it->type));
            }
        }
    }
    inv_add(map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos), it);

    gchar *buf = item_describe(it, player_item_known(p, it), FALSE, FALSE);
    log_add_entry(nlarn->log, "You find %s below your feet.", buf);
    g_free(buf);

    return TRUE;
}

/**
 * Scroll "enchant armour".
 *
 * @param the player
 * @param the scroll just read
 *
 */
static int scroll_enchant_armour(player *p, item *r_scroll)
{
    item **armour;

    g_assert(p != NULL && r_scroll != NULL);

    /* get a random piece of armour to enchant */
    /* try to get one not already fully enchanted */
    if ((armour = player_get_random_armour(p, TRUE)))
    {
        if (r_scroll->blessed)
        {
            log_add_entry(nlarn->log, "Your %s glows brightly for a moment.",
                          armour_name(*armour));

            (*armour)->rusty = FALSE;
            (*armour)->burnt = FALSE;
            (*armour)->corroded = FALSE;
            if ((*armour)->bonus < 0)
            {
                (*armour)->bonus = 0;
                if (chance(50))
                    return TRUE;
            }
            /* 50% chance of bonus enchantment */
            else if (chance(50) && (*armour)->bonus < 3)
                item_enchant(*armour);
        }
        else
        {
            log_add_entry(nlarn->log, "Your %s glows for a moment.",
                          armour_name(*armour));
        }

        /* blessed scroll never overenchants */
        if (!r_scroll->blessed || (*armour)->bonus < 3)
            item_enchant(*armour);

        return TRUE;
    }

    return FALSE;
}

static int scroll_enchant_weapon(player *p, item *r_scroll)
{
    g_assert(p != NULL && r_scroll != NULL);

    if (p->eq_weapon)
    {
        if (r_scroll->blessed)
        {
            log_add_entry(nlarn->log,
                          "Your %s glows brightly for a moment.",
                          weapon_name(p->eq_weapon));

            p->eq_weapon->rusty = FALSE;
            p->eq_weapon->burnt = FALSE;
            p->eq_weapon->corroded = FALSE;
            if (p->eq_weapon->bonus < 0)
            {
                p->eq_weapon->bonus = 0;
                return TRUE;
            }
        }
        else
        {
            log_add_entry(nlarn->log,
                          "Your %s glisters for a moment.",
                          weapon_name(p->eq_weapon));
        }

        /* blessed scroll never overenchants */
        if (!r_scroll->blessed || p->eq_weapon->bonus < 3)
            item_enchant(p->eq_weapon);

        return TRUE;
    }

    return FALSE;
}

static int scroll_gem_perfection(player *p, item *r_scroll)
{
    item *it;

    g_assert(p != NULL && r_scroll != NULL);

    if (inv_length_filtered(p->inventory, item_filter_gems) == 0)
    {
        return FALSE;
    }

    log_add_entry(nlarn->log, "This is a scroll of gem perfection.");

    if (r_scroll->blessed)
    {
        for (guint idx = 0; idx < inv_length_filtered(p->inventory, item_filter_gems); idx++)
        {
            it = inv_get_filtered(p->inventory, idx, item_filter_gems);
            /* double gem value */
            it->bonus <<= 1;
        }
        log_add_entry(nlarn->log, "You bring all your gems to perfection.");
    }
    else
    {
        it = display_inventory("Choose a gem to make perfect", p, &p->inventory, NULL,
                               FALSE, FALSE, FALSE, item_filter_gems);

        if (it)
        {
            gchar *desc = item_describe(it, TRUE, FALSE, TRUE);
            log_add_entry(nlarn->log, "You make %s perfect.", desc);

            /* double gem value */
            it->bonus <<= 1;
        }
    }

    return TRUE;
}

static int scroll_genocide_monster(player *p, item *r_scroll)
{
    char *glyph;
    guint candidates[10] = { 0 };
    guint found = 0;
    monster_t which = MT_NONE;
    GString *msg = g_string_new(NULL);

    g_assert(p != NULL);

    if (!player_item_known(p, r_scroll))
    {
        g_string_append_printf(msg, "This is a scroll of %s. ",
                               scroll_name(r_scroll));
    }

    g_string_append(msg, "Which monster do you want to genocide (enter its glyph)?");

    /* get a single character */
    glyph = display_get_string(msg->str, NULL, 1);

    /* release memory acquired for the message */
    g_string_free(msg, TRUE);

    if (!glyph)
    {
        /* player hit ESC */
        log_add_entry(nlarn->log, "You chose not to genocide any monster.");
        return FALSE;
    }

    /* look for the monster id for the glyph entered */
    for (monster_t id = 1; id < MT_MAX; id++)
    {
        if (monster_type_glyph(id) == glyph[0])
        {
            /* character entered matches the monster's glyph */
            if (!monster_is_genocided(id))
            {
                /* monster has not yet been genocided */
                candidates[found] = id;
                if (found++ == 10)
                    break;
            }
        }
    }

    /* release the memory acquired by display_get_string() */
    g_free(glyph);

    if (found == 0)
    {
        log_add_entry(nlarn->log, "No such monster.");
        return FALSE;
    }

    /* blessed scrolls allow a choice of same-glyph monsters */
    if (!r_scroll->blessed || found == 1)
    {
        /* the monster to genocide is the first monster found */
        which = candidates[0];
    }
    else
    {
        /* offer a selection of monsters that share the glyph entered */
        msg = g_string_new("");

        for (guint id = 0; id < found; id++)
        {
            g_string_append_printf(msg, "  %c) %-30s\n",
                                   id + 'a', monster_type_name(candidates[id]));
        }

        /* get the monster to be genocided */
        do
        {
            which = display_show_message("Genocide which monster?",
                                         msg->str, 0);
        }
        while (which < 'a' || which >= found + 'a');

        /* release memory acquired for the dialogue text */
        g_string_free(msg, TRUE);

        which -= 'a';
        g_assert(which < found);

        which = candidates[which];
    }

    /* genocide the selected monster */
    monster_genocide(which);

    log_add_entry(nlarn->log, "Wiped out all %s.",
                  monster_type_plural_name(which, 2));

    /* player genocided h[im|er]self? */
    if (which == MT_TOWN_PERSON) // Oops!
        player_die(p, PD_GENOCIDE, 0);

    return TRUE;
}

static int scroll_heal_monster(player *p, item *r_scroll __attribute__((unused)))
{
    GList *mlist;
    int count = 0;

    g_assert(p != NULL);

    mlist = g_hash_table_get_values(nlarn->monsters);

    do
    {
        monster *m = (monster *)mlist->data;
        position mpos = monster_pos(m);

        /* find monsters on the same level */
        if (Z(mpos) == Z(p->pos))
        {
            if (monster_hp(m) < monster_hp_max(m))
            {
                monster_hp_inc(m, monster_hp_max(m));
                count++;
            }
        }
    }
    while ((mlist = mlist->next));

    if (count > 0)
    {
        log_add_entry(nlarn->log, "You feel uneasy.");
    }

    g_list_free(mlist);

    return count;
}

static int scroll_hold_monster(player *p, item *r_scroll __attribute__((unused)))
{
    g_assert(p != NULL);

    area *blast, *obsmap;
    position cursor = p->pos;
    monster *m;
    gboolean success = FALSE;
    map *cmap = game_map(nlarn, Z(p->pos));

    obsmap = map_get_obstacles(cmap, p->pos, 2, FALSE);
    blast = area_new_circle_flooded(p->pos, 2, obsmap);

    for (Y(cursor) = blast->start_y; Y(cursor) < blast->start_y + blast->size_y; Y(cursor)++)
    {
        for (X(cursor) = blast->start_x; X(cursor) < blast->start_x + blast->size_x; X(cursor)++)
        {
            if (area_pos_get(blast, cursor) && (m = map_get_monster_at(cmap, cursor)))
            {
                effect *e = effect_new(ET_HOLD_MONSTER);
                monster_effect_add(m, e);

                success = TRUE;
            }
        }
    }
    area_destroy(blast);

    return success;
}

static int scroll_identify(player *p, item *r_scroll)
{
    item *it;

    g_assert(p != NULL && r_scroll != NULL);

    /* When the player has not identified the scroll of identify,
     * it fails when it is the only unidentified item in the inventory. */
    guint allowed_unid = player_item_known(p, r_scroll) ? 0 : 1;
    guint unid_count = inv_length_filtered(p->inventory, item_filter_unid);

    if (unid_count == allowed_unid)
    {
        /* player has no unidentified items */
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }

    log_add_entry(nlarn->log, "This is a scroll of identify.");

    if (r_scroll->blessed)
    {
        /* identify all items */
        log_add_entry(nlarn->log, "You identify your possessions.");

        while (inv_length_filtered(p->inventory, item_filter_unid))
        {
            it = inv_get_filtered(p->inventory, 0, item_filter_unid);
            player_item_identify(p, NULL, it);
        }
    }
    else
    {
        /* identify the scroll being read, otherwise it would show up here */
        player_item_identify(p, NULL, r_scroll);

        /* may identify up to 3 items */
        int tries = rand_1n(min(unid_count, 4));
        while (tries-- > 0)
        {
            /* choose a single item to identify */
            it = display_inventory("Choose an item to identify", p, &p->inventory,
                                   NULL, FALSE, FALSE, FALSE, item_filter_unid);

            if (it == NULL)
                break;

            gchar *desc = item_describe(it, FALSE, FALSE, TRUE);
            log_add_entry(nlarn->log, "You identify %s.", desc);
            g_free(desc);
            player_item_identify(p, NULL, it);

            desc = item_describe(it, TRUE, FALSE, FALSE);
            log_add_entry(nlarn->log, "%s %s.", (it->count > 1) ? "These are" :
                          "This is", desc);
            g_free(desc);

            if (inv_length_filtered(p->inventory, item_filter_unid) == 0)
                break;
        }
    }

    return TRUE;
}

int scroll_mapping(player *p, item *r_scroll)
{
    position pos = pos_invalid;
    map *m;

    /* scroll can be null as I use this to fake a known level */
    g_assert(p != NULL);

    m = game_map(nlarn, Z(p->pos));
    Z(pos) = Z(p->pos);

    const gboolean map_traps = (r_scroll != NULL && r_scroll->blessed);

    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            map_tile_t tile = map_tiletype_at(m, pos);
            if (r_scroll == NULL || tile != LT_FLOOR)
                player_memory_of(p, pos).type = tile;
            player_memory_of(p, pos).sobject = map_sobject_at(m, pos);

            if (map_traps)
            {
                trap_t trap = map_trap_at(m, pos);
                if (trap)
                    player_memory_of(p, pos).trap = trap;
            }

        }
    }

    return TRUE;
}

static int scroll_remove_curse(player *p, item *r_scroll)
{
    g_assert(p != NULL && r_scroll != NULL);

    if (inv_length_filtered(p->inventory, item_filter_cursed) == 0)
    {
        /* player has no cursed items */
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }

    log_add_entry(nlarn->log, "This is a scroll of remove curse.");

    if (r_scroll->blessed)
    {
        /* remove curses on all items */
        log_add_entry(nlarn->log, "You remove curses on your possessions.");

        while (inv_length_filtered(p->inventory, item_filter_cursed) > 0)
        {
            item *it = inv_get_filtered(p->inventory, 0, item_filter_cursed);

            // Get the description before uncursing the item.
            gchar *buf = item_describe(it, player_item_known(p, it), FALSE, TRUE);
            buf[0] = g_ascii_toupper(buf[0]);
            log_add_entry(nlarn->log, "%s glow%s in a white light.",
                          buf, it->count == 1 ? "s" : "");

            g_free(buf);

            item_remove_curse(it);
        }
    }
    else
    {
        /* choose a single item to uncurse */
        item *it = display_inventory("Choose an item to uncurse", p, &p->inventory,
                                     NULL, FALSE, FALSE, FALSE,
                                     item_filter_cursed_or_unknown);

        if (it != NULL)
        {
            if (!it->cursed)
            {
                log_add_entry(nlarn->log, "Nothing happens.");
                return TRUE;
            }
            // Get the description before uncursing the item.
            gchar *buf = item_describe(it, player_item_known(p, it), FALSE, TRUE);
            buf[0] = g_ascii_toupper(buf[0]);

            log_add_entry(nlarn->log, "%s glow%s in a white light.",
                          buf, it->count == 1 ? "s" : "");

            g_free(buf);
            item_remove_curse(it);
        }
    }

    return TRUE;
}

static int scroll_spell_extension(player *p, item *r_scroll)
{
    g_assert(p != NULL && r_scroll != NULL);

    /* return early if no spells memorised */
    if (p->known_spells->len == 0)
        return FALSE;

    if (r_scroll->blessed)
    {
        /* blessed scroll: increase knowledge of all spells */
        for (guint idx = 0; idx < p->known_spells->len; idx++)
        {
            spell *sp = g_ptr_array_index(p->known_spells, idx);

            sp->knowledge++;
        }
    }
    else
    {
        /* uncursed scroll: increase knowledge of one random spell */
        spell *sp = g_ptr_array_index(p->known_spells,
                rand_0n(p->known_spells->len));

        sp->knowledge++;
    }

    /* give a message if any spell has been extended */
    log_add_entry(nlarn->log, "You feel your magic skills %simprove.",
            r_scroll->blessed ? "greatly " : "");
    return TRUE;
}

static int scroll_teleport(player *p, item *r_scroll)
{
    guint nlevel;

    g_assert(p != NULL);

    if (r_scroll->blessed)
    {
        /* blessed scrolls of teleport always teleport to town level */
        nlevel = 0;
    }
    else
    {
        if (Z(p->pos) == 0)
            /* teleporting in town does not work */
            nlevel = 0;
        else if (Z(p->pos) < MAP_DMAX)
            /* choose a cavern level if the player is in the caverns*/
            nlevel = rand_1n(MAP_DMAX);
        else
            /* choose a volcano level if the player is in the volcano */
            nlevel = rand_m_n(MAP_DMAX, MAP_MAX);
    }

    if (nlevel != Z(p->pos))
    {
        player_map_enter(p, game_map(nlarn, nlevel), TRUE);
        return TRUE;
    }

    return FALSE;
}

static int scroll_timewarp(player *p, item *r_scroll)
{
    gint mobuls = 0; /* number of mobuls */
    gint turns;      /* number of turns */
    guint idx = 0;   /* position inside player's effect list */

    g_assert(p != NULL && r_scroll != NULL);

    turns = (rand_1n(1000) - 850);

    // For blessed scrolls, use the minimum turn count of three tries.
    if (r_scroll->blessed)
    {
        turns = min(turns, (rand_1n(1000) - 850));
        turns = min(turns, (rand_1n(1000) - 850));
    }

    if (turns == 0)
        turns = 1;

    if ((gint32)(game_turn(nlarn) + turns) < 0)
    {
        turns = 1 - game_turn(nlarn);
    }

    mobuls = gtime2mobuls(turns);

    /* rare case that time has not been modified */
    if (!mobuls)
    {
        return FALSE;
    }

    game_turn(nlarn) += turns;
    log_add_entry(nlarn->log,
                  "You go %sward in time by %d mobul%s.",
                  (mobuls < 0) ? "back" : "for",
                  abs(mobuls), plural(abs(mobuls)));

    /* adjust effects for time warping */
    while (idx < p->effects->len)
    {
        /* loop over all effects which affect the player */
        effect *e = game_effect_get(nlarn, g_ptr_array_index(p->effects, idx));

        if (e->turns == 0)
        {
            /* leave permanent effects alone */
            idx++;
            continue;
        }

        if (turns > 0)
        {
            /* gone forward in time */
            if ((gint)e->turns < turns)
            {
                /* the effect's remaining turns are smaller
                   than the number of turns the player moved into the future,
                   thus the effect is no longer valid. */
                player_effect_del(p, e);
            }
            else
            {
                /* reduce the number of remaining turns for this effect */
                e->turns -= turns;

                /* proceed to next effect */
                idx++;
            }
        }
        else if (turns < 0)
        {
            /* gone backward in time */
            if (e->start > game_turn(nlarn))
            {
                /* the effect started after the new game turn,
                   thus it is no longer (in fact not yet) valid */
                player_effect_del(p, e);
            }
            else
            {
                /* increase the number of remaining turns */
                e->turns += abs(turns);

                /* proceed to next effect */
                idx++;
            }
        }
    }

    return TRUE;
}
