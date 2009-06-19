/*
 * monsters.c
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

static const monster_data monsters[MT_MAX] =
{
    /* ID                 NAME                  LV   AC DAM INT    GO  HP      EXP IMG he no ha sl fa  fl sp un in iv */
    { MT_NONE,            "",                    0,  0,  0,  0,    0,   0,      0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_BAT,             "bat",                 1,  0,  1,  3,    0,   1,      1, 'B', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_GNOME,           "gnome",               1, 10,  1,  8,   30,   2,      2, 'G', 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
    { MT_HOBGOBLIN,       "hobgoblin",           1, 14,  2,  5,   25,   3,      2, 'H', 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    { MT_JACKAL,          "jackal",              1, 17,  1,  4,    0,   1,      1, 'J', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_KOBOLD,          "kobold",              1, 20,  1,  7,   10,   1,      1, 'K', 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
    { MT_ORC,             "orc",                 2, 12,  1,  9,   40,   4,      2, 'O', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_SNAKE,           "snake",               2, 15,  1,  3,    0,   3,      1, 'S', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_CENTIPEDE,       "giant centipede",     2, 14,  0,  2,    0,   1,      2, 'c', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_JACULI,          "jaculi",              2, 20,  1,  3,    0,   2,      1, 'j', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_TROGLODYTE,      "troglodyte",          2, 10,  2,  5,   80,   4,      3, 't', 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    { MT_GIANT_ANT,       "giant ant",           2,  8,  1,  3,    0,   5,      5, 'A', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_FLOATING_EYE,    "floating eye",        3,  8,  1,  3,    0,   5,      2, 'E', 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_LEPRECHAUN,      "leprechaun",          3,  3,  0,  6, 1500,  13,     45, 'L', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_NYMPH,           "nymph",               3,  3,  0,  9,    0,  18,     45, 'N', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_QUASIT,          "quasit",              3,  5,  3,  3,    0,  10,     15, 'Q', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_RUST_MONSTER,    "rust monster",        3,  4,  0,  3,    0,  18,     25, 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_ZOMBIE,          "zombie",              3, 12,  2,  3,    0,   6,      7, 'Z', 1, 0, 1, 0, 0, 0, 0, 1, 0, 0 },
    { MT_ASSASSIN_BUG,    "assassin bug",        4,  9,  3,  3,    0,  20,     15, 'a', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_BUGBEAR,         "bugbear",             4,  5,  4,  5,   40,  20,     35, 'b', 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
    { MT_HELLHOUND,       "hell hound",          4,  5,  2,  6,    0,  16,     35, 'h', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_ICE_LIZARD,      "ice lizard",          4, 11,  2,  6,   50,  16,     25, 'i', 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { MT_CENTAUR,         "centaur",             4,  6,  4, 10,   40,  24,     45, 'C', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    /* ID                 NAME                  LV   AC DAM  INT   GO   HP     EXP IMG he no ha sl fa  fl sp un in iv */
    { MT_TROLL,           "troll",               5,  4,  5,  9,   80,  50,    300, 'T', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_YETI,            "yeti",                5,  6,  4,  5,   50,  35,    100, 'Y', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_ELF,             "elf",                 5,  8,  1, 15,   50,  22,     35, 'e', 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
    { MT_GELATINOUSCUBE,  "gelatinous cube",     5,  9,  1,  3,    0,  22,     45, 'g', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_METAMORPH,       "metamorph",           6,  7,  3,  3,    0,  30,     40, 'm', 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { MT_VORTEX,          "vortex",              6,  4,  3,  3,    0,  30,     55, 'v', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_ZILLER,          "ziller",              6, 15,  3,  3,    0,  30,     35, 'z', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_VIOLET_FUNGUS,   "violet fungi",        6, 12,  3,  3,    0,  38,    100, 'F', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_WRAITH,          "wraith",              6,  3,  1,  3,    0,  30,    325, 'w', 1, 0, 1, 0, 0, 1, 0, 1, 0, 0 },
    { MT_FORVALAKA,       "forvalaka",           6,  2,  5,  7,    0,  50,    280, 'f', 1, 0, 0, 0, 1, 0, 0, 1, 0, 1 },
    { MT_LAMA_NOBE,       "lama nobe",           7,  7,  3,  6,    0,  35,     80, 'l', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_OSEQUIP,         "osequip",             7,  4,  3,  4,    0,  35,    100, 'o', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_ROTHE,           "rothe",               7, 15,  5,  3,  100,  50,    250, 'r', 1, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
    { MT_XORN,            "xorn",                7,  0,  6, 13,    0,  60,    300, 'X', 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    { MT_VAMPIRE,         "vampire",             7,  3,  4, 17,    0,  50,   1000, 'v', 1, 0, 1, 0, 0, 1, 0, 1, 0, 0 },
    { MT_STALKER,         "invisible stalker",   7,  3,  6,  5,    0,  50,    350, 'I', 1, 0, 0, 1, 0, 0, 0, 0, 1, 0 },
    { MT_POLTERGEIST,     "poltergeist",         8,  1,  4,  5,    0,  50,    450, 'p', 0, 0, 0, 0, 0, 0, 1, 0, 1, 0 },
    { MT_DISENCHANTRESS,  "disenchantress",      8,  3,  0,  5,    0,  50,    500, 'q', 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_SHAMBLINGMOUND,  "shambling mound",     8,  2,  5,  6,    0,  45,    400, 's', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_YELLOW_MOLD,     "yellow mold",         8, 12,  4,  3,    0,  35,    250, 'y', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_UMBER_HULK,      "umber hulk",          8,  3,  7, 14,    0,  65,    600, 'U', 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
    /*  ID                NAME                  LV  AC  DAM INT   GO   HP     EXP IMG  he no ha sl fa  fl sp un in iv */
    { MT_GNOME_KING,      "gnome king",          9, -1, 10, 18, 2000, 100,   3000, 'k', 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 },
    { MT_MIMIC,           "mimic",               9,  5,  6,  8,    0,  55,     99, 'M', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_WATER_LORD,      "water lord",          9,-10, 15, 20,    0, 150,  15000, 'w', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { MT_PURPLE_WORM,     "purple worm",         9, -1, 11,  3,  100, 120,  15000, 'P', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_XVART,           "xvart",               9, -2, 12, 13,    0,  90,   1000, 'x', 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    { MT_WHITE_DRAGON,    "white dragon",        5,  2,  4, 16,  500,  55,   1000, 'd', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_BRONCE_DRAGON,   "bronze dragon",       9,  2,  9, 16,  300,  80,   4000, 'D', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_GREEN_DRAGON,    "green dragon",        9,  3,  8, 15,  200,  70,   2500, 'D', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_SILVER_DRAGON,   "silver dragon",      10, -1, 12, 20,  700, 100,  10000, 'D', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_PLATINUM_DRAGON, "platinum dragon",    10, -5, 15, 22, 1000, 130,  24000, 'D', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_RED_DRAGON,      "red dragon",         10, -2, 13, 19,  800, 110,  14000, 'D', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { MT_SPIRIT_NAGA,     "spirit naga",        10,-20, 12, 23,    0,  95,  20000, 'n', 1, 1, 0, 0, 0, 1, 1, 0, 0, 1 },
    { MT_GREEN_URCHIN,    "green urchin",       10, -3, 12,  3,    0,  85,   5000, 'u', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { MT_DEMONLORD_I,     "type I demon lord",  12,-30, 18, 20,    0, 140,  50000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DEMONLORD_II,    "type II demon lord", 13,-30, 18, 21,    0, 160,  75000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DEMONLORD_III,   "type III demon lord",14,-30, 18, 22,    0, 180, 100000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DEMONLORD_IV,    "type IV demon lord", 15,-35, 20, 23,    0, 200, 125000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DEMONLORD_V,     "type V demon lord",  16,-40, 22, 24,    0, 220, 150000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DEMONLORD_VI,    "type VI demon lord", 17,-45, 24, 25,    0, 240, 175000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DEMONLORD_VII,   "type VII demon lord",18,-70, 27, 26,    0, 260, 200000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 },
    { MT_DAEMON_PRINCE,   "demon prince",       25,-127, 30, 28,   0, 345, 300000, '&', 1, 1, 1, 0, 1, 1, 0, 0, 1, 1 }
};

static int monster_genocided[MT_MAX] = { 1, 0 };

monster *monster_new(int monster_type)
{
    monster *nmonster;
    int iid, icount;    /* item id, item count */

    assert(monster_type > MT_NONE && monster_type < MT_MAX);

    /* make room for monster */
    nmonster = g_malloc0(sizeof(monster));

    nmonster->type = monster_type;
    nmonster->hp = divert(monsters[monster_type].hp_max, 10);
    nmonster->effects = g_ptr_array_new();
    nmonster->inventory = inv_new();

    /* fill monsters inventory */
    if (monster_get_gold(nmonster) > 0)
    {
        /* add gold to monster's inventory, ramdomize the amount */
        icount = max(divert(monster_get_gold(nmonster), 10), 1);
        inv_add(nmonster->inventory, item_new(IT_GOLD, icount, 0));
    }

    switch (monster_type)
    {
    case MT_LEPRECHAUN:
        if (chance(25))
            inv_add(nmonster->inventory,
                    item_new(IT_GEM, rand_m_n(GT_NONE + 1, GT_MAX -1), 0));
        break;

    case MT_ORC:
    case MT_ELF:
    case MT_TROLL:
        iid = rand_1n(WT_SWORDSLASHING);
        inv_add(nmonster->inventory,
                item_new(IT_WEAPON, iid, 0));
        break;

    case MT_NYMPH:
    case MT_TROGLODYTE:
    case MT_ROTHE:
    case MT_VIOLET_FUNGUS:
    case MT_PLATINUM_DRAGON:
    case MT_RED_DRAGON:
    case MT_GNOME_KING:
        inv_add(nmonster->inventory,
                item_create_random(rand_m_n(IT_NONE + 1, IT_MAX -1)));
        break;
    }

    /* position outside map */
    nmonster->pos = pos_new(G_MAXUINT16, G_MAXUINT16);

    /* initialize AI */
    nmonster->action = MA_WANDER;
    nmonster->lastseen = -1;
    nmonster->player_pos = pos_new(G_MAXUINT16, G_MAXUINT16);

    return nmonster;
}

monster *monster_new_by_level(int level)
{
    const int monster_level[] = { 5, 11, 17, 22, 27, 33, 39, 42, 46, 50, 53, 56, MT_MAX - 1 };
    int monster_id = 0;
    int monster_id_min;
    int monster_id_max;

    assert(level > 0 && level < LEVEL_MAX);

    if (level < 5)
    {
        monster_id_min = 1;
        monster_id_max = monster_level[level - 1];
    }
    else
    {
        monster_id_min = monster_level[level - 4] + 1;
        monster_id_max = monster_level[level - 1];
    }

    while (monster_genocided[monster_id] || (monster_id <= MT_NONE) || (monster_id >= MT_MAX))
    {
        monster_id = rand_m_n(monster_id_min, monster_id_max);
    }

    return monster_new(monster_id);
}

void monster_destroy(monster *m)
{
    assert(m != NULL);

    /* free effects */
    while (m->effects->len > 0)
        effect_destroy(g_ptr_array_remove_index_fast(m->effects, m->effects->len - 1));

    g_ptr_array_free(m->effects, TRUE);

    /* free inventory */
    if (m->inventory)
        inv_destroy(m->inventory);

    g_free(m);
}

/**
 * Substract monsters hitpoints
 * @param monster
 * @param amount of hitpoints to withdraw
 * @return TRUE if monster has been killed, FALSE if it survived
 */
gboolean monster_hp_lose(monster *m, int amount)
{
    assert(m != NULL);

    m->hp -= amount;

    if (m->hp < 1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void monster_drop_items(monster *m, inventory *floor)
{

    assert(m != NULL && floor != NULL);

    while (inv_length(m->inventory) > 0)
    {
        inv_add(floor, inv_get(m->inventory, 0));
        inv_del(m->inventory, 0);
    }
}

void monster_pickup_items(monster *m, inventory *floor, message_log *log)
{
    int i;
    item *it;

    assert(m != NULL && log != NULL);

    if (!floor)
        return;

    for (i = 1; i <= inv_length(floor); i++)
    {
        it = inv_get(floor, i - 1);

        if (m->type == MT_LEPRECHAUN
                && ((it->type == IT_GEM) || (it->type == IT_GOLD)))
        {
            if (m->m_visible)
            {
                log_add_entry(log,
                              "The %s picks up %s %s.",
                              monster_get_name(m),
                              (it->count == 1) ? "a" : "some",
                              (it->count == 1) ? item_get_name_sg(it->type) : item_get_name_pl(it->type));
            }

            inv_del_element(floor, it);
            inv_add(m->inventory, it);
        }

    }
}

/**
 * Determine a monster's action.
 *
 * @param the monster
 * @return TRUE if the action has changed
 */
gboolean monster_update_action(monster *m)
{
    monster_action_t naction;   /* new action */
    int time;
    gboolean low_hp;
    gboolean smart;

    /* FIXME: should include difficulty here */
    time   = 25 + monster_get_int(m);;
    low_hp = (m->hp < (monster_get_hp_max(m) / 4 ));
    smart  = (monster_get_int(m) > 4);

    /* stationary monsters */
    if (m->type == MT_MIMIC)
        naction = MA_REMAIN;

    /* low HP or very scared => FLEE player */
    else if ((low_hp && smart)
             || (monster_effect(m, ET_SCARE_MONSTER) > monster_get_int(m)))
    {
        naction = MA_FLEE;
    }
    /* after having spotted the player, agressive monster will follow
       the player for a certain amount of time turns, afterwards loose
       interest. More peaceful monsters will do something else. */
    else if (m->lastseen
             && (m->lastseen < time)
             && !monster_effect(m, ET_CHARM_MONSTER))
    {
        /* TODO: need to test for agressiveness */
        naction = MA_ATTACK;
    }

    /* no action if monster is held or sleeping */
    else if (monster_effect(m, ET_HOLD_MONSTER)
             || monster_effect(m, ET_SLEEP))
    {
        naction = MA_REMAIN;
    }

    /* if no action could be found, return to original behaviour */
    else
        naction = MA_WANDER;


    if (naction != m->action)
    {
        m->action = naction;
        return TRUE;
    }

    return FALSE;
}

gboolean monster_regenerate(monster *m, time_t gtime, int difficulty, message_log *log)
{
    /* number of turns between occasions */
    int frequency;

    /* temporary var for effect */
    effect *e;

    assert(m != NULL && log != NULL);

    /* modify frequency by difficulty: more regeneration, less poison */
    frequency = difficulty << 3;

    /* handle regeneration */
    if (m->type == MT_TROLL)
    {
        /* regenerate every (22- frequency) turns */
        if (gtime % (22 - frequency) == 0)
            m->hp = min(monster_get_hp_max(m), m->hp++);

    }

    /* handle poison */
    if ((e = monster_effect_get(m, ET_POISON)))
    {
        if ((gtime - e->start) % (22 + frequency) == 0)
        {
            m->hp -= e->amount;
        }

        if (m->hp < 1)
            return FALSE;
    }

    return TRUE;
}

inline char *monster_get_name(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].name;
}

inline char *monster_get_name_by_type(monster_t type)
{
    assert(type > MT_NONE && type < MT_MAX);
    return monsters[type].name;
}

inline int monster_get_level(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].level;
}

inline int monster_get_ac(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].ac;
}

inline int monster_get_dam(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].dam;
}

inline int monster_get_int(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].intelligence;
}

inline int monster_get_gold(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].gold;
}

inline int monster_get_hp_max(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].hp_max;
}

inline int monster_get_exp(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].exp;
}

inline char monster_get_image(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].image;
}

inline int monster_has_head(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].head;
}

inline int monster_is_beheadable(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return !monsters[m->type].nobehead;
}

inline int monster_has_hands(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].hands;
}

inline int monster_is_slow(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].slow;
}

inline int monster_is_fast(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].fast;
}

inline int monster_can_fly(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].fly;
}

inline int monster_is_spirit(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].spirit;
}

inline int monster_is_undead(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].undead;
}

inline int monster_is_invisible(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].invisible;
}

inline int monster_has_infravision(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return monsters[m->type].infravision;
}

inline int monster_genocide(int monster_id)
{
    assert(monster_id > MT_NONE && monster_id < MT_MAX);

    monster_genocided[monster_id] = TRUE;
    return monster_genocided[monster_id];
}

inline int monster_is_genocided(int monster_id)
{
    assert(monster_id > MT_NONE && monster_id < MT_MAX);
    return monster_genocided[monster_id];
}

void monster_effect_expire(monster *m, message_log *log)
{
    int i = 1;
    effect *e;

    assert(m != NULL && log != NULL);

    while (i <= m->effects->len)
    {
        e = g_ptr_array_index(m->effects, i - 1);

        if (effect_expire(e, 1) == -1)
        {
            /* effect has expired */
            monster_effect_del(m, e);

            /* log info */
            if (m->m_visible && effect_get_msg_m_stop(e))
                log_add_entry(log,
                              effect_get_msg_m_stop(e),
                              monster_get_name(m));

            g_free(e);
        }
        else
        {
            i++;
        }
    }
}
