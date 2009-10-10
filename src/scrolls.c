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

#include "game.h"
#include "nlarn.h"
#include "scrolls.h"

const magic_scroll_data scrolls[ST_MAX] =
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
    { ST_SPEED,             "speed",              ET_SPEED,              200 },
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
};

char *scroll_desc(int scroll_id)
{
    assert(scroll_id > ST_NONE && scroll_id < ST_MAX);
    return (char *)_scroll_desc[nlarn->scroll_desc_mapping[scroll_id - 1]];
}

int scroll_with_effect(struct player *p, item *scroll)
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

int scroll_annihilate(struct player *p, item *scroll)
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
            m = monster_damage_take(m, damage_new(DAM_MAGICAL, 2000, p));

            /* check if the monster has been killed */
            if (!m)
            {
                count++;
                idx--;
            }
        }
        else
        {
            log_add_entry(p->log,
                          "The %s barely escapes being annihilated.",
                          monster_name(m));

            /* lose half hit points*/
            damage *dam = damage_new(DAM_MAGICAL, monster_hp(m) / 2, p);
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

int scroll_create_artefact(player *p, item *scroll)
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

int scroll_enchant_armour(player *p, item *scroll)
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

int scroll_enchant_weapon(player *p, item *scroll)
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

int scroll_gem_perfection(player *p, item *scroll)
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

int scroll_heal_monster(player *p, item *scroll)
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

    if (count)
    {
        log_add_entry(p->log, "You feel uneasy.");
    }

    g_list_free(mlist);

    return count;
}

int scroll_identify(player *p, item *scroll)
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

    /* scroll can be null as I use this to fake a known level */
    assert(p != NULL);

    pos.z = p->pos.z;

    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            player_memory_of(p, pos).type = map_tiletype_at(game_map(nlarn, p->pos.z), pos);
            player_memory_of(p, pos).stationary = map_stationary_at(game_map(nlarn, p->pos.z), pos);
        }
    }

    return TRUE;
}

int scroll_remove_curse(player *p, item *scroll)
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

int scroll_spell_extension(player *p, item *scroll)
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

int scroll_teleport(player *p, item *scroll)
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

int scroll_timewarp(player *p, item *scroll)
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
