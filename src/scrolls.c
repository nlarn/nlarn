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

static int scroll_desc_mapping[ST_MAX - 1] = { 0 };

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

void scroll_desc_shuffle()
{
    shuffle(scroll_desc_mapping, ST_MAX - 1, 1);
}

char *scroll_desc(int scroll_id)
{
    assert(scroll_id > ST_NONE && scroll_id < ST_MAX);
    return (char *)_scroll_desc[scroll_desc_mapping[scroll_id - 1]];
}

int scroll_with_effect(struct player *p, item *scroll)
{
    effect *eff;

    assert(p != NULL && scroll != NULL);

    eff = effect_new(scroll_effect(scroll), game_turn(p->game));
    player_effect_add(p, eff);

    if (!effect_get_msg_start(eff))
    {
        return FALSE;
    }

    return TRUE;
}

int scroll_annihilate(struct player *p, item *scroll)
{
    int i;
    int experience = 0;

    GPtrArray *mlist;
    monster *m;

    assert(p != NULL && scroll != NULL);

    mlist = level_get_monsters_in(p->level, rect_new_sized(p->pos, 1));

    for (i = 1; i <= mlist->len; i++)
    {
        m = g_ptr_array_index(mlist, i - 1);

        if (m->type < MT_DEMONLORD_II)
        {
            experience = monster_exp(m);
            player_monster_kill(p, m, NULL);
        }
        else
        {
            log_add_entry(p->log,
                          "The %s barely escapes being annihilated.",
                          monster_name(m));

            /* lose half hit points*/
            m->hp >>=2 +1;
        }
    }

    if (experience)
    {
        log_add_entry(p->log, "You hear loud screams of agony!");
    }

    g_ptr_array_free(mlist, FALSE);

    return experience;
}

int scroll_create_artefact(player *p, item *scroll)
{
    item *it;
    char buf[61];

    assert(p != NULL && scroll != NULL);

    /* FIXME: this is not what it should be */
    if (!level_ilist_at(p->level, p->pos))
    {
        level_ilist_at(p->level, p->pos) = inv_new();
    }

    it = item_new_by_level(rand_1n(IT_MAX), p->level->nlevel);
    inv_add(level_ilist_at(p->level, p->pos), it);

    item_describe(it, player_item_known(p, it), (it->count == 1),
                  FALSE, buf, 60);

    log_add_entry(p->log, "You find %s below your feet.", buf);

    return TRUE;
}

int scroll_enchant_armour(player *p, item *scroll)
{
    assert(p != NULL && scroll != NULL);

    if (p->eq_suit)
    {
        log_add_entry(p->log,
                      "Your %s glows for a moment.",
                      armour_name(p->eq_suit));

        item_enchant(p->eq_suit);

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
    int i;

    item *it;

    assert(p != NULL && scroll != NULL);

    /* FIXME: too simple. should give the ability to choose a single gem instead */
    for (i = 1; i <= inv_length(p->inventory); i++)
    {
        it = inv_get(p->inventory, i - 1);
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
    int i;
    int count = 0;
    monster *m;

    assert(p != NULL && scroll != NULL);

    for (i = 1; i <= p->level->mlist->len; i++)
    {
        m = g_ptr_array_index(p->level->mlist, i - 1);

        if (m->hp < monster_hp_max(m))
        {
            m->hp = monster_hp_max(m);
            count++;
        }
    }

    if (count)
    {
        log_add_entry(p->log, "You feel uneasy.");
    }

    return count;
}

int scroll_identify(player *p, item *scroll)
{
    /*
     * FIXME: too simple and too powerful.
     * should give the ability to choose a single item instead
     */

    int pos;
    int count = 0; /* how many items have been identified */
    item *it;

    assert(p != NULL && scroll != NULL);

    for (pos = 1; pos <= p->inventory->len; pos++)
    {
        it = inv_get(p->inventory, pos - 1);
        if (!player_item_identified(p, it))
        {
            player_item_identify(p, it);
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

    /* scroll can be null as I use this tp fake a known level */
    assert(p != NULL);

    for (pos.y = 0; pos.y < LEVEL_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < LEVEL_MAX_X; pos.x++)
        {
            player_memory_of(p, pos).type = level_tiletype_at(p->level, pos);
            player_memory_of(p, pos).stationary = level_stationary_at(p->level, pos);
        }
    }

    return TRUE;
}

int scroll_remove_curse(player *p, item *scroll)
{
    int count_done = 0; /* how many curses have been removed */
    int count_avail = 0; /* how many curses can be removed */

    int pos;
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

    for (pos = 1; (pos <= inv_length(p->inventory)) && (count_avail > 0); pos++)
    {
        item = inv_get(p->inventory, pos - 1);

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
    int i;
    spell *sp;

    assert(p != NULL && scroll != NULL);

    for (i = 1; i <= p->known_spells->len; i++)
    {
        sp = g_ptr_array_index(p->known_spells, i - 1);

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
    int nlevel;
    assert(p != NULL);

    if (p->level->nlevel == 0)
        nlevel = 0;
    else if (p->level->nlevel < LEVEL_DMAX)
        nlevel = rand_0n(LEVEL_DMAX);
    else
        nlevel = rand_m_n(LEVEL_DMAX, LEVEL_MAX);

    if (nlevel != p->level->nlevel)
    {
        player_level_enter(p, p->game->levels[nlevel]);

        p->pos = level_find_space(p->level, LE_MONSTER);

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

    if ((gint32)(game_turn(p->game) + turns) < 0)
    {
        turns = 1 - game_turn(p->game);
    }

    mobuls = gtime2mobuls(turns);

    /* rare case that time has not been modified */
    if (!mobuls)
    {
        return FALSE;
    }

    game_turn(p->game) += turns;
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
