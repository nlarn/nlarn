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

#include <assert.h>
#include <stdlib.h>

#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "scrolls.h"

const magic_scroll_data scrolls[ST_MAX] =
{
    /* ID                   name                  effect               price obtainable */
    { ST_NONE,              "",                   ET_NONE,                 0, FALSE },
    { ST_ENCH_ARMOUR,       "enchant armour",     ET_NONE,               100,  TRUE },
    { ST_ENCH_WEAPON,       "enchant weapon",     ET_NONE,               125,  TRUE },
    { ST_ENLIGHTENMENT,     "enlightenment",      ET_ENLIGHTENMENT,       60,  TRUE },
    { ST_BLANK,             "blank paper",        ET_NONE,                10, FALSE },
    { ST_CREATE_MONSTER,    "create monster",     ET_NONE,               100, FALSE },
    { ST_CREATE_ARTIFACT,   "create artifact",    ET_NONE,               200, FALSE },
    { ST_AGGRAVATE_MONSTER, "aggravate monsters", ET_AGGRAVATE_MONSTER,  110, FALSE },
    { ST_TIMEWARP,          "time warp",          ET_NONE,               500, FALSE },
    { ST_TELEPORT,          "teleportation",      ET_NONE,               200,  TRUE },
    { ST_AWARENESS,         "expanded awareness", ET_AWARENESS,          250,  TRUE },
    { ST_SPEED,             "speed",              ET_SPEED,              200, FALSE },
    { ST_HEAL_MONSTER,      "monster healing",    ET_NONE,                30, FALSE },
    { ST_SPIRIT_PROTECTION, "spirit protection",  ET_SPIRIT_PROTECTION,  340,  TRUE },
    { ST_UNDEAD_PROTECTION, "undead protection",  ET_UNDEAD_PROTECTION,  340,  TRUE },
    { ST_STEALTH,           "stealth",            ET_STEALTH,            300,  TRUE },
    { ST_MAPPING,           "magic mapping",      ET_NONE,               400,  TRUE },
    { ST_HOLD_MONSTER,      "hold monsters",      ET_HOLD_MONSTER,       500, FALSE },
    { ST_GEM_PERFECTION,    "gem perfection",     ET_NONE,              1000, FALSE },
    { ST_SPELL_EXTENSION,   "spell extension",    ET_NONE,               500, FALSE },
    { ST_IDENTIFY,          "identify",           ET_NONE,               340,  TRUE },
    { ST_REMOVE_CURSE,      "remove curse",       ET_NONE,               220,  TRUE },
    { ST_ANNIHILATION,      "annihilation",       ET_NONE,              3900, FALSE },
    { ST_PULVERIZATION,     "pulverization",      ET_NONE,               610,  TRUE },
    { ST_LIFE_PROTECTION,   "life protection",    ET_LIFE_PROTECTION,   3000, FALSE },
    { ST_GENOCIDE_MONSTER,  "genocide monster",   ET_NONE,              3800, FALSE },
};

static int scroll_with_effect(player *p, item *scroll);
static int scroll_annihilate(player *p, item *scroll);
static int scroll_create_artefact(player *p, item *scroll);
static int scroll_enchant_armour(player *p, item *scroll);
static int scroll_enchant_weapon(player *p, item *scroll);
static int scroll_gem_perfection(player *p, item *scroll);
static int scroll_genocide_monster(player *p, item *scroll);
static int scroll_heal_monster(player *p, item *scroll);
static int scroll_identify(player *p, item *scroll);
static int scroll_remove_curse(player *p, item *scroll);
static int scroll_spell_extension(player *p, item *scroll);
static int scroll_teleport(player *p, item *scroll);
static int scroll_timewarp(player *p, item *scroll);

static const char *_scroll_desc[ST_MAX - 1] =
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
    "Xodil Keterulo",
};

char *scroll_desc(int scroll_id)
{
    assert(scroll_id > ST_NONE && scroll_id < ST_MAX);
    return (char *)_scroll_desc[nlarn->scroll_desc_mapping[scroll_id - 1]];
}

item_usage_result scroll_read(struct player *p, item *scroll)
{
    char description[61];
    item_usage_result result;

    result.time = 2;
    result.used_up = TRUE;

    item_describe(scroll, player_item_known(p, scroll),
                  TRUE, TRUE, description, 60);

    if (player_effect(p, ET_BLINDNESS))
    {
        log_add_entry(p->log, "As you are blind you can't read %s.",
                      description);

        result.identified = FALSE;
        result.used_up = FALSE;

        return result;
    }

    log_add_entry(p->log, "You read %s.", description);

    /* increase number of scrolls read */
    p->stats.scrolls_read++;

    if (scroll->cursed)
    {
        damage *dam = damage_new(DAM_FIRE, ATT_NONE, rand_1n(p->hp), NULL);
        log_add_entry(p->log, "The Scroll explodes!");
        player_damage_take(p, dam, PD_CURSE, scroll->type);
    }
    else
    {
        switch (scroll->id)
        {
        case ST_ENCH_ARMOUR:
            result.identified = scroll_enchant_armour(p, scroll);
            break;

        case ST_ENCH_WEAPON:
            result.identified = scroll_enchant_weapon(p, scroll);
            break;

        case ST_BLANK:
            result.used_up = FALSE;
            result.identified = TRUE;
            log_add_entry(p->log, "This scroll is blank.");
            break;

        case ST_CREATE_MONSTER:
            result.identified = spell_create_monster(p);
            break;

        case ST_CREATE_ARTIFACT:
            result.identified = scroll_create_artefact(p, scroll);
            break;

        case ST_TIMEWARP:
            result.identified = scroll_timewarp(p, scroll);
            break;

        case ST_TELEPORT:
            result.identified = scroll_teleport(p, scroll);
            break;

        case ST_HEAL_MONSTER:
            result.identified = scroll_heal_monster(p, scroll);
            break;

        case ST_MAPPING:
            log_add_entry(p->log, "There is a map on the scroll!");
            result.identified = scroll_mapping(p, scroll);
            break;

        case ST_GEM_PERFECTION:
            result.identified = scroll_gem_perfection(p, scroll);
            break;

        case ST_SPELL_EXTENSION:
            result.identified = scroll_spell_extension(p, scroll);
            break;

        case ST_IDENTIFY:
            result.identified = scroll_identify(p, scroll);
            break;

        case ST_REMOVE_CURSE:
            result.identified = scroll_remove_curse(p, scroll);
            break;

        case ST_ANNIHILATION:
            result.identified = scroll_annihilate(p, scroll);
            break;

        case ST_PULVERIZATION:
            if (!p->identified_scrolls[ST_PULVERIZATION])
            {
                log_add_entry(p->log, "This is a scroll of %s. ",
                              scroll_name(scroll));
            }

            if (spell_vaporize_rock(p))
            {
                /* recalc fov if something has been vaporised */
                player_update_fov(p);
            }

            result.identified = TRUE;
            break;

        case ST_GENOCIDE_MONSTER:
            scroll_genocide_monster(p, scroll);
            result.identified = TRUE;
            break;

        default:
            result.identified = scroll_with_effect(p, scroll);
            break;
        }

        if (!result.identified)
        {
            log_add_entry(p->log, "Nothing happens.");
        }
    }

    return result;
}

static int scroll_with_effect(struct player *p, item *scroll)
{
    effect *eff;

    assert(p != NULL && scroll != NULL);

    eff = effect_new(scroll_effect(scroll), game_turn(nlarn));
    player_effect_add(p, eff);

    if (!effect_get_msg_start(eff))
    {
        return FALSE;
    }

    return TRUE;
}

static int scroll_annihilate(struct player *p, item *scroll)
{
    guint idx;
    int count = 0;

    GPtrArray *mlist;
    monster *m;

    assert(p != NULL && scroll != NULL);

    mlist = map_get_monsters_in(game_map(nlarn, p->pos.z), rect_new_sized(p->pos, 1));

    for (idx = 0; idx < mlist->len; idx++)
    {
        m = g_ptr_array_index(mlist, idx);

        /* FIXME: remove this special case here and give a good resistance to
         * the demon lords */
        if (monster_type(m) < MT_DEMONLORD_II)
        {
            m = monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_NONE, 2000, p));

            /* check if the monster has been killed */
            if (!m) count++;
        }
        else
        {
            log_add_entry(p->log,
                          "The %s barely escapes being annihilated.",
                          monster_name(m));

            /* lose half hit points*/
            damage *dam = damage_new(DAM_MAGICAL, ATT_NONE, monster_hp(m) / 2, p);
            monster_damage_take(m, dam);
        }
    }

    if (count)
    {
        log_add_entry(p->log, "You hear loud screams of agony!");
    }

    g_ptr_array_free(mlist, FALSE);

    return count;
}

static int scroll_create_artefact(player *p, item *scroll)
{
    item *it;
    char buf[61];

    assert(p != NULL && scroll != NULL);

    it = item_new_by_level(rand_1n(IT_MAX), p->pos.z);
    inv_add(map_ilist_at(game_map(nlarn, p->pos.z), p->pos), it);

    item_describe(it, player_item_known(p, it), (it->count == 1),
                  FALSE, buf, 60);

    log_add_entry(p->log, "You find %s below your feet.", buf);

    return TRUE;
}

static int scroll_enchant_armour(player *p, item *scroll)
{
    item *it;

    assert(p != NULL && scroll != NULL);

    /* get a random piece of armour to enchant */
    if ((it = player_random_armour(p)))
    {
        log_add_entry(p->log,
                      "Your %s glows for a moment.",
                      armour_name(it));

        item_enchant(it);

        return TRUE;
    }

    return FALSE;
}

static int scroll_enchant_weapon(player *p, item *scroll)
{
    assert(p != NULL && scroll != NULL);

    if (p->eq_weapon)
    {
        log_add_entry(p->log,
                      "Your %s glisters for a moment.",
                      weapon_name(p->eq_weapon));

        item_enchant(p->eq_weapon);

        return TRUE;
    }

    return FALSE;
}

static int scroll_gem_perfection(player *p, item *scroll)
{
    int count = 0;
    guint idx;

    item *it;

    assert(p != NULL && scroll != NULL);

    /* FIXME: too simple. should give the ability to choose a single gem instead */
    for (idx = 0; idx < inv_length(p->inventory); idx++)
    {
        it = inv_get(p->inventory, idx);
        if (it->type == IT_GEM)
        {
            /* double gem value */
            it->bonus <<= 1;
            count++;
        }
    }

    return count;
}

static int scroll_genocide_monster(player *p, item *scroll)
{
    char *in;
    int id;
    GString *msg;

    assert(p != NULL);

    msg = g_string_new(NULL);

    if (!p->identified_scrolls[ST_GENOCIDE_MONSTER])
    {
        g_string_append_printf(msg, "This is a scroll of %s. ",
                               scroll_name(scroll));
    }

    g_string_append(msg, "Which monster do you want to genocide (type letter)?");

    in = display_get_string(msg->str, NULL, 1);

    g_string_free(msg, TRUE);

    if (!in)
    {
        log_add_entry(p->log, "You chose not to genocide any monster.");
        return FALSE;
    }

    for (id = 1; id < MT_MAX; id++)
    {
        if (monster_type_image(id) == in[0])
        {
            if (!monster_is_genocided(id))
            {
                p->stats.monsters_killed[id] += monster_genocide(id);
                log_add_entry(p->log, "Wiped out all %ss.",
                              monster_type_name(id));

                g_free(in);

                return TRUE;
            }
        }
    }

    g_free(in);

    log_add_entry(p->log, "No such monster.");
    return FALSE;
}

static int scroll_heal_monster(player *p, item *scroll)
{
    GList *mlist;
    int count = 0;
    monster *m;

    assert(p != NULL && scroll != NULL);

    mlist = g_hash_table_get_values(nlarn->monsters);

    /* purge genocided monsters */
    do
    {
        m = (monster *)mlist->data;
        position mpos = monster_pos(m);

        /* find monsters on the same level */
        if (mpos.z == p->pos.z)
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
        log_add_entry(p->log, "You feel uneasy.");
    }

    g_list_free(mlist);

    return count;
}

static int scroll_identify(player *p, item *scroll)
{
    /*
     * FIXME: too simple and too powerful.
     * should give the ability to choose a single item instead
     */

    guint idx;
    int count = 0; /* how many items have been identified */
    item *it;

    assert(p != NULL && scroll != NULL);

    for (idx = 0; idx < inv_length(p->inventory); idx++)
    {
        it = inv_get(p->inventory, idx);
        if (!player_item_identified(p, it))
        {
            player_item_identify(p, NULL, it);
        }

        count++;
    }

    if (count)
    {
        log_add_entry(p->log, "You identify your possessions.");
    }
    else
    {
        log_add_entry(p->log, "Nothing happens.");
    }

    return count;
}

int scroll_mapping(player *p, item *scroll)
{
    position pos;
    map *m;

    /* scroll can be null as I use this to fake a known level */
    assert(p != NULL);

    m = game_map(nlarn, p->pos.z);
    pos.z = p->pos.z;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            player_memory_of(p, pos).type = map_tiletype_at(m, pos);
            player_memory_of(p, pos).sobject = map_sobject_at(m, pos);
        }
    }

    return TRUE;
}

static int scroll_remove_curse(player *p, item *scroll)
{
    int count_done = 0; /* how many curses have been removed */
    int count_avail = 0; /* how many curses can be removed */

    guint idx;
    item *item;
    char buf[61];

    assert(p != NULL && scroll != NULL);

    /* determine how many curses can be removed */
    if (scroll && scroll->blessed)
    {
        count_avail = inv_length(p->inventory);
    }
    else
    {
        count_avail = 1;
    }

    for (idx = 0; (idx < inv_length(p->inventory)) && (count_avail > 0); idx++)
    {
        item = inv_get(p->inventory, idx);

        if (item->cursed)
        {
            item_remove_curse(item);

            if (item->count > 1)
            {
                log_add_entry(p->log, "The %s glow in a white light.",
                              item_describe(item, player_item_known(p, item),
                                            FALSE, TRUE, buf, 60));
            }
            else
            {
                log_add_entry(p->log, "The %s glows in a white light.",
                              item_describe(item, player_item_known(p, item),
                                            TRUE, TRUE, buf, 60));
            }

            count_done++;
            count_avail--;
        }
    }

    return count_done;
}

static int scroll_spell_extension(player *p, item *scroll)
{
    guint idx;
    spell *sp;

    assert(p != NULL && scroll != NULL);

    for (idx = 0; idx < p->known_spells->len; idx++)
    {
        sp = g_ptr_array_index(p->known_spells, idx);

        /* double spell knowledge */
        sp->knowledge <<=1;
    }

    /* give a message if any spell has been extended */
    if (p->known_spells->len > 0)
    {
        log_add_entry(p->log, "You feel your magic skills improve.");
        return TRUE;
    }

    return FALSE;
}

static int scroll_teleport(player *p, item *scroll)
{
    guint nlevel;

    assert(p != NULL);

    if (p->pos.z == 0)
        nlevel = 0;
    else if (p->pos.z < MAP_DMAX)
        nlevel = rand_0n(MAP_DMAX);
    else
        nlevel = rand_m_n(MAP_DMAX, MAP_MAX);

    if (nlevel != p->pos.z)
    {
        player_map_enter(p, game_map(nlarn, nlevel), TRUE);
        return TRUE;
    }

    return FALSE;
}

static int scroll_timewarp(player *p, item *scroll)
{
    /* number of mobuls */
    gint32 mobuls = 0;
    gint32 turns;

    assert(p != NULL && scroll != NULL);

    turns = (rand_1n(1000) - 850);

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
    log_add_entry(p->log,
                  "You go %sward in time by %d mobul%s.",
                  (mobuls < 0) ? "back" : "for",
                  abs(mobuls),
                  (abs(mobuls) == 1) ? "" : "s");

    /* adjust effects for time warping */
    /* FIXME: have a close look at this when improving game time management */
    player_effects_expire(p, turns);

    return TRUE;
}
