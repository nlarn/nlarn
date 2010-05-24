/*
 * monsters.c
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
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "game.h"
#include "map.h"
#include "monsters.h"
#include "nlarn.h"

/* monster information hiding */
struct _monster
{
    monster_t type;
    gpointer oid;            /* monsters id inside the monster hash */
    gint32 hp_max;
    gint32 hp;
    position pos;
    int movement;
    monster_action_t action; /* current action */
    guint32 lastseen;        /* number of turns since when player was last seen; 0 = never */
    position player_pos;     /* last known position of player */
    inventory *inventory;
    gpointer weapon;
    GPtrArray *effects;
    int colour;          /* for the mimic: the colour the disguised mimic chose */
    guint32
        item_type: 8,    /* item type monster is displayed as */
        unknown: 1;      /* monster is unknown */
};

const char *monster_ai_desc[MA_MAX] =
{
    NULL,               /* MA_NONE */
    "fleeing",          /* MA_FLEE */
    "standing still",   /* MA_REMAIN */
    "wandering",        /* MA_WANDER */
    "attacking"         /* MA_ATTACK */
};

const char *monster_attack_verb[ATT_MAX] =
{
    NULL,
    "hits",         /* ATT_WEAPON */
    "points at",    /* ATT_MAGIC */
    "claws",        /* ATT_CLAW */
    "bites",        /* ATT_BITE */
    "stings",       /* ATT_STING */
    "slams",        /* ATT_SLAM */
    "kicks",        /* ATT_KICK */
    "touches",      /* ATT_TOUCH */
    "breathes at",  /* ATT_BREATH */
    "gazes at",     /* ATT_GAZE */
};


static gboolean monster_player_visible(monster *m);
static gboolean monster_attack_available(monster *m, attack_t type);
static item *monster_weapon_select(monster *m);
static void monster_weapon_wield(monster *m, item *weapon);
static gboolean monster_item_disenchant(monster *m, struct player *p);
static gboolean monster_item_rust(monster *m, struct player *p);
static gboolean monster_player_rob(monster *m, struct player *p, item_t item_type);
static position monster_move_wander(monster *m, struct player *p);
static position monster_move_attack(monster *m, struct player *p);
static position monster_move_flee(monster *m, struct player *p);

monster *monster_new(monster_t type, position pos)
{
    monster *nmonster;
    int it, icount;     /* item type, item id, item count */

    assert(type > MT_NONE && type < MT_MAX && pos_valid(pos));

    /* check if supplied position is suitable for a monster */
    if (!map_pos_validate(game_map(nlarn, pos.z), pos, LE_MONSTER, FALSE))
    {
        return NULL;
    }

    /* make room for monster */
    nmonster = g_malloc0(sizeof(monster));

    nmonster->type = type;

    /* determine max hp; prevent the living dead */
    nmonster->hp_max = nmonster->hp = max(1, divert(monster_type_hp_max(type), 10));

    nmonster->effects = g_ptr_array_new();
    nmonster->inventory = inv_new(nmonster);

    /* fill monsters inventory */
    if (monster_gold_amount(nmonster) > 0)
    {
        const int gold_chance = monster_gold_chance(nmonster);
        if (gold_chance == 0 || chance(gold_chance))
        {
            /* add gold to monster's inventory, randomize the amount */
            icount = max(divert(monster_gold_amount(nmonster), 30), 1);
            inv_add(&nmonster->inventory, item_new(IT_GOLD, icount));
        }
    }

    /* add special items */
    switch (type)
    {
    case MT_LEPRECHAUN:
        if (chance(25))
        {
            inv_add(&nmonster->inventory, item_new_random(IT_GEM));
        }
        break;

    case MT_TROGLODYTE:
    case MT_NYMPH:
    case MT_PLATINUM_DRAGON:
    case MT_RED_DRAGON:
    case MT_GNOME_KING:
        /* add something that is not a container */
        do
        {
            it = rand_1n(IT_MAX);
        }
        while (it == IT_CONTAINER);

        inv_add(&nmonster->inventory, item_new_random(it));
        break;

    case MT_TOWN_PERSON:
        // initialize name counter
        nmonster->colour = rand_1n(40);
        break;

    default:
        /* no fancy stuff... */
        break;
    }

    /* generate a weapon if monster can use it */
    if (monster_attack_available(nmonster, ATT_WEAPON))
    {
        int weapon_count = 3;
        int weapons[3]; /* choice of weapon types */
        item *weapon;

        /* preset weapon types */
        switch (type)
        {
        case MT_HOBGOBLIN:
        case MT_ORC:
        case MT_TROLL:
            weapons[0] = WT_ODAGGER;
            weapons[1] = WT_OSHORTSWORD;
            weapons[2] = WT_OSPEAR;
            break;

        case MT_ELF:
            weapons[0] = WT_ESHORTSWORD;
            weapons[1] = WT_ESPEAR;
            weapon_count = 2;
            break;

        case MT_BUGBEAR:
        case MT_CENTAUR:
        case MT_POLTERGEIST:
            weapons[0] = WT_MACE;
            weapons[1] = WT_FLAIL;
            weapons[2] = WT_BATTLEAXE;
            break;

        case MT_VAMPIRE:
        case MT_GNOME_KING:
        case MT_WATER_LORD:
        case MT_XVART:
            weapons[0] = WT_LONGSWORD;
            weapons[1] = WT_2SWORD;
            weapons[2] = WT_SWORDSLASHING;
            break;

        default:
            weapons[0] = WT_DAGGER;
            weapons[1] = WT_SPEAR;
            weapons[2] = WT_SHORTSWORD;
            break;
        }

        weapon = item_new(IT_WEAPON, weapons[rand_0n(weapon_count)]);
        item_new_finetouch(weapon);

        inv_add(&nmonster->inventory, weapon);

        /* wield the new weapon */
        monster_weapon_wield(nmonster, weapon);
    } /* finished initializing weapons */

    /* initialize mimics */
    if (monster_flags(nmonster, MF_MIMIC))
    {
        struct
        {
            item_t type;
            int colour;
        }
        possible_types[] =
        {
            { IT_AMULET,    DC_YELLOW, },
            { IT_GOLD,      DC_YELLOW, },
            { IT_RING,      DC_WHITE,  },
            { IT_GEM,       DC_RED,    },
            { IT_CONTAINER, DC_BROWN   },
        };

        int chosen_type = rand_0n(5);

        /* determine how the mimic will be displayed */
        nmonster->item_type = possible_types[chosen_type].type;
        nmonster->colour = possible_types[chosen_type].colour;

        /* the mimic is not known to be a monster */
        nmonster->unknown = TRUE;
    }

    /* initialize AI */
    nmonster->action = MA_WANDER;
    nmonster->player_pos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

    /* register monster with game */
    nmonster->oid = game_monster_register(nlarn, nmonster);

    /* set position */
    nmonster->pos = pos;

    /* link monster to tile */
    map_set_monster_at(game_map(nlarn, pos.z), pos, nmonster);

    /* increment monster count */
    game_map(nlarn, pos.z)->mcount++;

    return nmonster;
}

monster *monster_new_by_level(position pos)
{
    const int monster_level[] = { MT_KOBOLD,           // D1:   5
                                  MT_GIANT_ANT,        // D2:  11
                                  MT_ZOMBIE,           // D3:  17
                                  MT_CENTAUR,          // D4:  22
                                  MT_WHITE_DRAGON,     // D5:  27
                                  MT_FORVALAKA,        // D6:  33
                                  MT_STALKER,          // D7:  39
                                  MT_SHAMBLINGMOUND,   // D8:  42
                                  MT_MIMIC,            // D9:  46
                                  MT_BRONZE_DRAGON,    // D10: 50
                                  MT_PLATINUM_DRAGON,  // V1:  53
                                  MT_GREEN_URCHIN,     // V2:  56
                                  MT_DEMON_PRINCE      // V3
                                };
    int monster_id = MT_NONE;
    int monster_id_min;
    int monster_id_max;
    int nlevel = pos.z;

    assert(pos_valid(pos));

    if (nlevel == 0)
    {
        monster_id_min = 1;
        monster_id_max = monster_level[0];
    }
    else if (nlevel < 5)
    {
        monster_id_min = 1;
        monster_id_max = monster_level[nlevel - 1];
    }
    else
    {
        monster_id_min = monster_level[nlevel - 4] + 1;
        monster_id_max = monster_level[nlevel - 1];
    }

    do
    {
        monster_id = rand_m_n(monster_id_min, monster_id_max);
    }
    while ((monster_id <= MT_NONE)
            || (monster_id >= MT_MAX)
            || nlarn->monster_genocided[monster_id]
            || chance(monster_type_reroll_chance(monster_id)));

    return monster_new(monster_id, pos);
}

void monster_destroy(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);

    /* free effects */
    while (m->effects->len > 0)
    {
        gpointer effect_id = g_ptr_array_remove_index(m->effects, m->effects->len - 1);
        effect *e = game_effect_get(nlarn, effect_id);
        effect_destroy(e);
    }

    g_ptr_array_free(m->effects, TRUE);

    /* remove monster from map (if it is on the map) */
    map_set_monster_at(game_map(nlarn, m->pos.z), m->pos, NULL);

    /* free inventory */
    if (m->inventory)
        inv_destroy(m->inventory, TRUE);

    /* unregister monster */
    if (m->oid > 0)
    {
        /* the oid is set to 0 when the monster is destroyed by the
        g_hash_table_foreach_remove callback */
        game_monster_unregister(nlarn, m->oid);
    }

    /* decrement monster count */
    game_map(nlarn, m->pos.z)->mcount--;

    g_free(m);
}

void monsters_wrap(lua_State *L)
{
    int i;

    assert (L != NULL);

    struct
    {
        char *name;
        int value;
    } constants[] =
    {
        /* monster actions */
        { "FLEE",   MA_FLEE },
        { "REMAIN", MA_REMAIN },
        { "WANDER", MA_WANDER },
        { "ATTACK", MA_ATTACK },

        /* monster flags */
        { "HEAD",        MF_HEAD },
        { "NOBEHEAD",    MF_NOBEHEAD },
        { "HANDS",       MF_HANDS },
        { "FLY",         MF_FLY },
        { "SPIRIT",      MF_SPIRIT },
        { "UNDEAD",      MF_UNDEAD },
        { "INVISIBLE",   MF_INVISIBLE },
        { "INFRAVISION", MF_INFRAVISION },
        { "REGENERATE",  MF_REGENERATE },
        { "METALLIVORE", MF_METALLIVORE },
        { "DEMON",       MF_DEMON },
        { "DRAGON",      MF_DRAGON },
        { "MIMIC",       MF_MIMIC },
        { "RES_FIRE",    MF_RES_FIRE },
        { "RES_SLEEP",   MF_RES_SLEEP },
        { "RES_POISON",  MF_RES_POISON },

        /* monster types */
        { "MT_GIANT_BAT",       MT_GIANT_BAT },
        { "MT_GNOME",           MT_GNOME },
        { "MT_HOBGOBLIN",       MT_HOBGOBLIN },
        { "MT_JACKAL",          MT_JACKAL },
        { "MT_KOBOLD",          MT_KOBOLD },
        { "MT_ORC",             MT_ORC },
        { "MT_SNAKE",           MT_SNAKE },
        { "MT_CENTIPEDE",       MT_CENTIPEDE },
        { "MT_JACULUS",         MT_JACULUS },
        { "MT_TROGLODYTE",      MT_TROGLODYTE },
        { "MT_GIANT_ANT",       MT_GIANT_ANT },
        { "MT_FLOATING_EYE",    MT_FLOATING_EYE },
        { "MT_LEPRECHAUN",      MT_LEPRECHAUN },
        { "MT_NYMPH",           MT_NYMPH },
        { "MT_QUASIT",          MT_QUASIT },
        { "MT_RUST_MONSTER",    MT_RUST_MONSTER },
        { "MT_ZOMBIE",          MT_ZOMBIE },
        { "MT_ASSASSIN_BUG",    MT_ASSASSIN_BUG },
        { "MT_BUGBEAR",         MT_BUGBEAR },
        { "MT_HELLHOUND",       MT_HELLHOUND },
        { "MT_ICE_LIZARD",      MT_ICE_LIZARD },
        { "MT_CENTAUR",         MT_CENTAUR },
        { "MT_TROLL",           MT_TROLL },
        { "MT_YETI",            MT_YETI },
        { "MT_ELF",             MT_ELF },
        { "MT_GELATINOUSCUBE",  MT_GELATINOUSCUBE },
        { "MT_WHITE_DRAGON",    MT_WHITE_DRAGON },
        { "MT_METAMORPH",       MT_METAMORPH },
        { "MT_VORTEX",          MT_VORTEX },
        { "MT_ZILLER",          MT_ZILLER },
        { "MT_VIOLET_FUNGUS",   MT_VIOLET_FUNGUS },
        { "MT_WRAITH",          MT_WRAITH },
        { "MT_FORVALAKA",       MT_FORVALAKA },
        { "MT_LAMA_NOBE",       MT_LAMA_NOBE },
        { "MT_OSQUIP",          MT_OSQUIP },
        { "MT_ROTHE",           MT_ROTHE },
        { "MT_XORN",            MT_XORN },
        { "MT_VAMPIRE",         MT_VAMPIRE },
        { "MT_STALKER",         MT_STALKER },
        { "MT_POLTERGEIST",     MT_POLTERGEIST },
        { "MT_DISENCHANTRESS",  MT_DISENCHANTRESS },
        { "MT_SHAMBLINGMOUND",  MT_SHAMBLINGMOUND },
        { "MT_YELLOW_MOLD",     MT_YELLOW_MOLD },
        { "MT_UMBER_HULK",      MT_UMBER_HULK },
        { "MT_GNOME_KING",      MT_GNOME_KING },
        { "MT_MIMIC",           MT_MIMIC },
        { "MT_WATER_LORD",      MT_WATER_LORD },
        { "MT_PURPLE_WORM",     MT_PURPLE_WORM },
        { "MT_XVART",           MT_XVART },
        { "MT_BRONZE_DRAGON",   MT_BRONZE_DRAGON },
        { "MT_GREEN_DRAGON",    MT_GREEN_DRAGON },
        { "MT_SILVER_DRAGON",   MT_SILVER_DRAGON },
        { "MT_PLATINUM_DRAGON", MT_PLATINUM_DRAGON },
        { "MT_RED_DRAGON",      MT_RED_DRAGON },
        { "MT_SPIRIT_NAGA",     MT_SPIRIT_NAGA },
        { "MT_GREEN_URCHIN",    MT_GREEN_URCHIN },
        { "MT_DEMONLORD_I",     MT_DEMONLORD_I },
        { "MT_DEMONLORD_II",    MT_DEMONLORD_II },
        { "MT_DEMONLORD_III",   MT_DEMONLORD_III },
        { "MT_DEMONLORD_IV",    MT_DEMONLORD_IV },
        { "MT_DEMONLORD_V",     MT_DEMONLORD_V },
        { "MT_DEMONLORD_VI",    MT_DEMONLORD_VI },
        { "MT_DEMONLORD_VII",   MT_DEMONLORD_VII },
        { "MT_DEMON_PRINCE",    MT_DEMON_PRINCE },
        { "MT_TOWN_PERSON",     MT_TOWN_PERSON },

        { NULL, 0 },
    };

    for (i = 0; constants[i].name != NULL; i++)
    {
        lua_pushinteger(L, constants[i].value);
        lua_setglobal(L, constants[i].name);
    }

    /* read monster data */
    gchar *filename = g_strdup_printf("%s%c%s", nlarn->libdir,
                                      G_DIR_SEPARATOR, "monsters.lua");

    if (luaL_dofile(L, filename) == 1)
    {
        fprintf(stderr, "Error opening '%s': %s\n", filename,
                lua_tostring(L, -1));

        exit(EXIT_FAILURE);
    }
    g_free(filename);

}

void monster_serialize(gpointer oid, monster *m, cJSON *root)
{
    cJSON *mval;

    cJSON_AddItemToArray(root, mval = cJSON_CreateObject());
    cJSON_AddNumberToObject(mval, "type", monster_type(m));
    cJSON_AddNumberToObject(mval, "oid", GPOINTER_TO_UINT(oid));
    cJSON_AddNumberToObject(mval, "hp_max", m->hp_max);
    cJSON_AddNumberToObject(mval, "hp", m->hp);
    cJSON_AddItemToObject(mval,"pos", pos_serialize(m->pos));
    cJSON_AddNumberToObject(mval, "movement", m->movement);
    cJSON_AddNumberToObject(mval, "action", m->action);

    if (m->weapon != NULL)
        cJSON_AddNumberToObject(mval, "weapon", GPOINTER_TO_UINT(m->weapon));

    if (m->colour)
        cJSON_AddNumberToObject(mval, "colour", m->colour);

    if (m->unknown)
        cJSON_AddTrueToObject(mval, "unknown");

    if (m->item_type)
        cJSON_AddNumberToObject(mval, "item_type", m->item_type);

    if (m->lastseen != 0)
    {
        cJSON_AddNumberToObject(mval,"lastseen", m->lastseen);
        cJSON_AddItemToObject(mval,"player_pos", pos_serialize(m->player_pos));
    }

    /* inventory */
    if (inv_length(m->inventory) > 0)
    {
        cJSON_AddItemToObject(mval, "inventory", inv_serialize(m->inventory));
    }

    /* effects */
    if (m->effects->len > 0)
    {
        cJSON_AddItemToObject(mval, "effects", effects_serialize(m->effects));
    }
}

void monster_deserialize(cJSON *mser, game *g)
{
    cJSON *obj;
    guint oid;
    monster *m = g_malloc0(sizeof(monster));

    m->type = cJSON_GetObjectItem(mser, "type")->valueint;
    oid = cJSON_GetObjectItem(mser, "oid")->valueint;
    m->oid = GUINT_TO_POINTER(oid);
    m->hp_max = cJSON_GetObjectItem(mser, "hp_max")->valueint;
    m->hp = cJSON_GetObjectItem(mser, "hp")->valueint;
    m->pos = pos_deserialize(cJSON_GetObjectItem(mser, "pos"));
    m->movement = cJSON_GetObjectItem(mser, "movement")->valueint;
    m->action = cJSON_GetObjectItem(mser, "action")->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "weapon")))
        m->weapon = GUINT_TO_POINTER(obj->valueint);

    if ((obj = cJSON_GetObjectItem(mser, "colour")))
        m->colour = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "unknown")))
        m->unknown = TRUE;

    if ((obj = cJSON_GetObjectItem(mser, "item_type")))
        m->item_type = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "lastseen")))
        m->lastseen = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "player_pos")))
        m->player_pos = pos_deserialize(obj);

    /* inventory */
    if ((obj = cJSON_GetObjectItem(mser, "inventory")))
        m->inventory = inv_deserialize(obj);
    else
        m->inventory = inv_new(m);

    /* effects */
    if ((obj = cJSON_GetObjectItem(mser, "effects")))
        m->effects = effects_deserialize(obj);
    else
        m->effects = g_ptr_array_new();

    /* add monster to game */
    g_hash_table_insert(g->monsters, m->oid, m);

    /* increase max_id to match used ids */
    if (oid > g->monster_max_id)
        g->monster_max_id = oid;
}

int monster_hp_max(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->hp_max;
}

int monster_hp(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->hp;
}

void monster_hp_inc(monster *m, int amount)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    m->hp = min(m->hp + amount, m->hp_max);
}

item_t monster_item_type(monster *m)
{
    assert (m != NULL);
    return m->item_type;
}

gpointer monster_oid(monster *m)
{
    assert (m != NULL);
    return m->oid;
}

void monster_oid_set(monster *m, gpointer oid)
{
    assert (m != NULL);
    m->oid = oid;
}

position monster_pos(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->pos;
}

int monster_pos_set(monster *m, map *map, position target)
{
    assert(m != NULL && map != NULL && pos_valid(target));

    if (map_pos_validate(map, target, LE_MONSTER, FALSE))
    {
        /* remove current reference to monster from tile */
        map_set_monster_at(monster_map(m), m->pos, NULL);

        /* set new position */
        m->pos = target;

        /* set reference to monster on tile */
        map_set_monster_at(map, target, m);

        return TRUE;
    }

    return FALSE;
}

monster_t monster_type(monster *m)
{
    return (m != NULL) ? m->type : MT_NONE;
}

gboolean monster_unknown(monster *m)
{
    assert (m != NULL);
    return m->unknown;
}

void monster_unknown_set(monster *m, gboolean what)
{
    assert (m != NULL);
    m->unknown = what;
}

static gboolean monster_nearby(monster *m)
{
    /* different level */
    if (m->pos.z != nlarn->p->pos.z)
        return FALSE;

    return player_pos_visible(nlarn->p, m->pos);
}

gboolean monster_in_sight(monster *m)
{
    assert (m != NULL);

    /* different level */
    if (m->pos.z != nlarn->p->pos.z)
        return FALSE;

    /* invisible monster, player has no infravision */
    if (monster_flags(m, MF_INVISIBLE) && !player_effect(nlarn->p, ET_INFRAVISION))
        return FALSE;

    return player_pos_visible(nlarn->p, m->pos);
}

static const char *get_town_person_name(int value)
{
    // various jobs
    const char *npc_desc[] = { "peasant woman", "old man", "old woman",
                               "little boy", "young girl", "fisherman",
                               "midwife", "errand boy", "bar maid",
                               "stable-lad", "innkeeper", "woodcutter",
                               "carpenter", "clerk", "barber",
                               "teacher", "town guard", "postman",
                               "cobbler", "baker", "merchant",
                               "clergyman", "student", "blacksmith",
                               "nurse", "seamstress", "cartwright",
                               "student", "sales clerk", "miller"
                               };
    if (value >= 30)
        return "peasant";

    return npc_desc[value];
}

// Takes into account visibility.
// For the real name, use monster_name() directly.
const char *monster_get_name(monster *m)
{
    if (!game_wizardmode(nlarn) && !monster_in_sight(m))
        return ("unseen monster");

    if (monster_type(m) == MT_TOWN_PERSON)
        return get_town_person_name(m->colour);

    return (monster_name(m));
}

const char* monster_type_plural_name(const int montype, const int count)
{
    /* need a static buffer to return to calling functions */
    static char buf[61] = { 0 };

    if (count > 1)
    {
        if (g_strlcpy(buf, luaN_query_string("monsters", montype, "plural_name"), 60) == 0)
        {
            g_snprintf(buf, 60, "%ss", monster_type_name(montype));
        }

        return buf;
    }

    return monster_type_name(montype);
}

void monster_die(monster *m, struct player *p)
{
    assert(m != NULL);

    /* if the player can see the monster describe the event */
    /* Also give a message for invisible monsters you killed yourself
       (the xp gain gives this away anyway). */
    if (monster_in_sight(m) || (p != NULL && monster_nearby(m)))
    {
        char *message = "The %s dies!";

        log_add_entry(nlarn->log, message, monster_get_name(m));
    }

    /* drop stuff the monster carries */
    if (inv_length(m->inventory))
    {
        inventory **floor = map_ilist_at(monster_map(m), monster_pos(m));
        while (inv_length(m->inventory) > 0)
        {
            inv_add(floor, inv_get(m->inventory, 0));
            inv_del(&m->inventory, 0);
        }
    }

    if (p != NULL)
    {
        player_exp_gain(p, monster_exp(m));
        p->stats.monsters_killed[m->type] += 1;
    }

    monster_destroy(m);
}

void monster_level_enter(monster *m, struct map *l)
{
    assert (m != NULL && l != NULL);

    map_sobject_t source = map_sobject_at(monster_map(m), m->pos);
    map_sobject_t target;
    position npos;
    char *what;
    char *how = "comes";

    /* check if the monster used the stairs */
    switch (source)
    {
    case LS_DNGN_EXIT:
        target = LS_DNGN_ENTRANCE;
        what = "through";
        break;

    case LS_DNGN_ENTRANCE:
        target = LS_DNGN_EXIT;
        what = "through";
        break;

    case LS_STAIRSDOWN:
        target = LS_STAIRSUP;
        what = "down";
        break;

    case LS_STAIRSUP:
        target = LS_STAIRSDOWN;
        what = "up";
        break;

    case LS_ELEVATORDOWN:
        target = LS_ELEVATORUP;
        what = "down";
        break;

    case LS_ELEVATORUP:
        target = LS_ELEVATORDOWN;
        what = "up";
        break;

    default:
        target = LS_NONE;
    }

    /* determine new position */
    if (target)
    {
        /* monster came through a map entrance */
        npos = map_find_sobject(l, target);
    }
    else
    {
        /* monster fell through a trap door */
        npos = map_find_space(l, LE_MONSTER, FALSE);
    }

    /* validate new position */
    if (pos_identical(nlarn->p->pos, npos))
    {
        /* player is standing at the target position */
        how = "squeezes past";
        npos = map_find_space_in(l, rect_new_sized(npos, 1), LE_MONSTER, FALSE);
    }

    if (!map_pos_validate(l, npos, LE_MONSTER, FALSE))
    {
        /* the position somehow isn't valid */
        return;
    }

    /* remove monster from old map  */
    map *oldmap = game_map(nlarn, m->pos.z);
    map_set_monster_at(oldmap, m->pos, NULL);

    /* put monster into map */
    monster_pos_set(m, l, npos);

    /* reset the information of the player's last known position */
    m->lastseen = 0;

    /* log the event */
    if (monster_in_sight(m) && target)
    {
        log_add_entry(nlarn->log, "The %s %s %s %s.", monster_name(m),
                      how, what, ls_get_desc(target));
    }
}

void monster_move(monster *m, struct player *p)
{
    /* monster's new position */
    position m_npos;

    /* update monster's knowledge of player's position */
    if (monster_player_visible(m)
            || (player_effect(p, ET_AGGRAVATE_MONSTER)
                && pos_distance(m->pos, p->pos) < 15))
    {
        monster_update_player_pos(m, p->pos);
    }

    /* add the monster's speed to the monster's movement points */
    m->movement += monster_speed(m);

    /* let the monster make a move as long it has movement points left */
    while (m->movement >= SPEED_NORMAL)
    {
        /* reduce the monster's movement points */
        m->movement -= SPEED_NORMAL;

        /* update monsters action */
        if (monster_update_action(m) && monster_in_sight(m))
        {
            /* the monster has chosen a new action and the player
               can see the new action, so let's describe it */

            if (m->action == MA_ATTACK)
            {
                /* TODO: certain monster types will make a sound when attacking the player */
                /*
                log_add_entry(nlarn->log,
                              "The %s has spotted you and heads towards you!",
                              monster_name(m));
                 */
            }
            else if (m->action == MA_FLEE)
            {
                log_add_entry(nlarn->log, "The %s turns to flee!", monster_name(m));
            }
        }

        /* let the monster have a look at the items at it's current position
           if it chose to pick up something, the turn is over */
        if (monster_items_pickup(m))
            return;

        /* determine monster's next move */
        m_npos = monster_pos(m);

        switch (m->action)
        {
        case MA_FLEE:
            m_npos = monster_move_flee(m, p);
            break;

        case MA_REMAIN:
            /* Sgt. Stan Still - do nothing */
            break;

        case MA_WANDER:
            m_npos = monster_move_wander(m, p);
            break;

        case MA_ATTACK:
            /* monster tries a ranged attack */
            if (monster_player_visible(m) && monster_player_ranged_attack(m, p))
                return;

            m_npos = monster_move_attack(m, p);
            break;

        case MA_NONE:
        case MA_MAX:
            /* possibly a bug */
            break;
        }

        /* ******** if new position has been found - move the monster ********* */
        map_sobject_t target_st = map_sobject_at(monster_map(m), m_npos);

        if (!pos_identical(m_npos, monster_pos(m)))
        {
            /* vampires won't step onto mirrors */
            if ((m->type == MT_VAMPIRE) && (target_st == LS_MIRROR))
            {
                /* FIXME: should try to move around it */
                m_npos = monster_pos(m);
            }

            else if (pos_identical(p->pos, m_npos))
            {
                /* bump into invisible player */
                monster_update_player_pos(m, p->pos);
                m_npos = monster_pos(m);

                log_add_entry(nlarn->log, "The %s bumped into you.", monster_get_name(m));
            }

            /* check for door */
            else if ((target_st == LS_CLOSEDDOOR)
                     /* lock out zombies */
                     && monster_flags(m, MF_HANDS) && monster_int(m) > 3)
            {
                /* open the door */
                map_sobject_set(monster_map(m), m_npos, LS_OPENDOOR);

                /* notify the player if the door is visible */
                if (monster_in_sight(m))
                {
                    log_add_entry(nlarn->log, "The %s opens the door.", monster_name(m));
                }
            }

            /* move towards player; check for monsters */
            else if (map_pos_validate(monster_map(m), m_npos, LE_MONSTER, FALSE))
            {
                monster_pos_set(m, monster_map(m), m_npos);

                /* check for traps */
                if (map_trap_at(monster_map(m), monster_pos(m)))
                {
                    if (!monster_trap_trigger(m))
                        return; /* trap killed the monster */
                }

            } /* end new position */
        } /* end monster repositioning */
    } /* while movement >= SPEED_NORMAL */

    /* increment count of turns since when player was last seen */
    if (m->lastseen) m->lastseen++;
}

monster *monster_trap_trigger(monster *m)
{
    /* original and new position of the monster */
    position opos, npos;

    /* the trap */
    trap_t trap;

    /* effect monster might have gained during the move */
    effect *eff = NULL;

    assert (m != NULL);

    trap = map_trap_at(monster_map(m), m->pos);

    /* flying monsters are only affected by sleeping gas traps */
    if (monster_flags(m, MF_FLY) && (trap != TT_SLEEPGAS))
    {
        return m;
    }

    /* return if the monster has not triggered the trap */
    if (!chance(trap_chance(trap)))
    {
        return m;
    }

    opos = monster_pos(m);

    if (monster_in_sight(m))
    {
        log_add_entry(nlarn->log, trap_m_message(trap), monster_name(m));

        /* set player's knowledge of trap */
        player_memory_of(nlarn->p, opos).trap = trap;
    }

    /* monster triggered the trap */
    switch (trap)
    {
    case TT_TRAPDOOR:
        monster_level_enter(m, game_map(nlarn, m->pos.z + 1));
        break;

    case TT_TELEPORT:
        npos = map_find_space(game_map(nlarn, m->pos.z), LE_MONSTER, FALSE);
        monster_pos_set(m, monster_map(m), npos);
        break;

    case TT_SPIKEDPIT:
        {
            const trap_t trap2 = TT_PIT;

            if (trap_effect(trap2) && chance(trap_effect_chance(trap2)))
            {
                effect *e;
                e = effect_new(trap_effect(trap2));
                e = monster_effect_add(m, e);
            }
        }
        // intentional fall-through

    default:
        /* if there is an effect on the trap add it to the
         * monster's list of effects. */
        if (trap_effect(trap))
        {
            eff = effect_new(trap_effect(trap));
            eff = monster_effect_add(m, eff);
        }
    } /* switch (trap) */

    /* inflict damage caused by the trap */
    if (trap_damage(trap))
    {
        damage *dam = damage_new(DAM_PHYSICAL, ATT_NONE,
                                 rand_1n(trap_damage(trap)), NULL);
        m = monster_damage_take(m, dam);
    }

    return m;
}

void monster_polymorph(monster *m)
{
    assert (m != NULL);

    do
    {
        m->type = rand_1n(MT_MAX_GENERATED);
    }
    while (monster_is_genocided(m->type));

    m->hp = monster_hp_max(m);
}

int monster_items_pickup(monster *m)
{
    // The town people never take your stuff.
    if (monster_type(m) == MT_TOWN_PERSON)
        return FALSE;

    /* TODO: gelatious cube digests items, rust monster eats metal stuff */
    /* FIXME: time management */

    gboolean pick_up = FALSE;
    guint idx;
    item *it;
    char buf[61] = { 0 };

    assert(m != NULL);

    for (idx = 0; idx < inv_length(*map_ilist_at(monster_map(m), m->pos)); idx++)
    {
        it = inv_get(*map_ilist_at(monster_map(m), m->pos), idx);

        if (m->type == MT_LEPRECHAUN
                && ((it->type == IT_GEM) || (it->type == IT_GOLD)))
        {
            /* leprechauns collect treasures */
            pick_up = TRUE;
        }
        else if (it->type == IT_WEAPON && monster_attack_available(m, ATT_WEAPON))
        {
            /* monster can attack with weapons */
            item *mweapon = game_item_get(nlarn, m->weapon);

            /* compare this weapon with the weapon the monster wields */
            if (mweapon == NULL || (weapon_wc(mweapon) < weapon_wc(it)))
                pick_up = TRUE;
        }

        if (pick_up)
        {
            /* item has been picked up */
            if (monster_in_sight(m))
            {
                item_describe(it, player_item_identified(nlarn->p, it),
                              (it->count == 1), FALSE, buf, 60);
                log_add_entry(nlarn->log, "The %s picks up %s.", monster_name(m), buf);
            }

            inv_del_element(map_ilist_at(monster_map(m), m->pos), it);
            inv_add(&m->inventory, it);

            /* go back one item as the following items lowered their number */
            idx--;

            if (it->type == IT_WEAPON)
            {
                /* find out if the new weapon is better than the old one */
                item *best = monster_weapon_select(m);

                if (it == best)
                {
                    monster_weapon_wield(m, best);
                }
            }
            /* finish this turn after picking up an item */
            return TRUE;
        } /* end if pick_up */
    } /* end foreach item */

    return FALSE;
}

int monster_attack_count(monster *m)
{
    int count = 0;

    if (luaN_push_table("monsters", m->type, "attacks"))
    {
        /* attacks table has been found; query lenght */
        count = lua_objlen(nlarn->L, -1);

        /* clean up */
        lua_pop(nlarn->L, 3);
    }

    return count;
}

attack monster_attack(monster *m, int num)
{
    attack att = { ATT_NONE, DAM_NONE, 0, 0 };

    assert (m != NULL && num <= monster_attack_count(m));

    if (luaN_push_table("monsters", m->type, "attacks"))
    {
        lua_rawgeti(nlarn->L, -1, num);

        if (lua_istable(nlarn->L, -1))
        {
            /* inside the attack table */
            lua_getfield(nlarn->L, -1, "type");
            att.type = lua_tointeger(nlarn->L, -1);

            lua_getfield(nlarn->L, -2, "damage");
            att.damage = lua_tointeger(nlarn->L, -1);

            lua_getfield(nlarn->L, -3, "base");
            att.base = lua_tointeger(nlarn->L, -1);

            lua_getfield(nlarn->L, -4, "rand");
            att.rand = lua_tointeger(nlarn->L, -1);

            /* remove the queried values from the stack */
            lua_pop(nlarn->L, 4);
        }

        /* remove the table and ancestors from the stack */
        lua_pop(nlarn->L, 4);
    }

    return att;
}

static int handle_breath_attack(monster *m, player *p, attack att)
{
    assert(att.type == ATT_BREATH);

    damage *dam;
    spell *sp = NULL;
    switch (att.damage)
    {
    case DAM_FIRE:
        sp = spell_new(SP_MON_FIRE);
        break;
    default:
        break;
    }

    if (sp == NULL)
        return FALSE;

    if (monster_effect(m, ET_CHARM_MONSTER)
            && (rand_m_n(5, 30) * monster_level(m) - player_get_wis(p) < 30))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s is awestruck at your magnificence!",
                          monster_get_name(m));
        }
        return TRUE;
    }

    if (monster_in_sight(m))
    {
        log_add_entry(nlarn->log, "The %s breathes a %s!",
                      monster_get_name(m), spell_name(sp));
    }
    else
    {
        log_add_entry(nlarn->log, "A %s spews forth from nowhere!",
                      spell_name(sp));
    }

    /* generate damage */
    dam = damage_new(att.damage, att.type, att.base + game_difficulty(nlarn), m);

    // store monster position in case the monster dies.
    position source   = m->pos;
    position last_pos = throw_ray(sp, p, m->pos, m->player_pos, dam->amount,
                                  FALSE);

    if (map_sobject_at(game_map(nlarn, last_pos.z), last_pos) == LS_MIRROR)
    {
        log_add_entry(nlarn->log, "The mirror reflects the %s!",
                      spell_name(sp));

        throw_ray(sp, p, last_pos, source, dam->amount, FALSE);
    }
    /* attack reflected by player */
    else if (pos_identical(p->pos, last_pos) && player_effect(p, ET_REFLECTION))
    {
        throw_ray(sp, p, last_pos, source, dam->amount, TRUE);
    }
    return TRUE;
}

void monster_player_attack(monster *m, player *p)
{
    damage *dam;
    attack att = { ATT_NONE };

    assert(m != NULL && p != NULL);

    /* the player is invisible and the monster bashes into thin air */
    if (!pos_identical(m->player_pos, p->pos))
    {
        if (!map_is_monster_at(game_map(nlarn, p->pos.z), p->pos)
                && monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s bashes into thin air.",
                          monster_name(m));
        }

        m->lastseen++;

        return;
    }

    /* player is invisible and monster tries to hit player */
    if (player_effect(p, ET_INVISIBILITY) && !monster_flags(m, MF_INFRAVISION)
            && chance(65))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s misses wildly.", monster_get_name(m));
        }
        return;
    }

    if (monster_effect(m, ET_CHARM_MONSTER)
            && (rand_m_n(5, 30) * monster_level(m) - player_get_wis(p) < 30))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s is awestruck at your magnificence!",
                          monster_get_name(m));
        }
        return;
    }

    /* choose a random attack type */
    att = monster_attack(m, rand_1n(monster_attack_count(m) + 1));

    /* no attack has been found. return to calling function. */
    if (att.type == ATT_NONE) return;

    /* generate damage */
    dam = damage_new(att.damage, att.type, att.base + game_difficulty(nlarn), m);

    /* deal with random damage (spirit naga) */
    if (dam->type == DAM_RANDOM) dam->type = rand_1n(DAM_MAX);

    if (att.type == ATT_BREATH)
    {
        handle_breath_attack(m, p, att);
        return;
    }

    /* half damage if player is protected against spirits */
    if (player_effect(p, ET_SPIRIT_PROTECTION) && monster_flags(m, MF_SPIRIT))
    {
        if (dam->type == DAM_PHYSICAL)
        {
            /* half physical damage */
            dam->amount >>= 1;
        }
        else
        {
            /* FIXME: give log message */
            damage_free(dam);

            return;
        }
    }

    /* set damage for weapon attacks */
    if ((att.type == ATT_WEAPON) && (m->weapon != NULL))
    {
        item *weapon = game_item_get(nlarn, m->weapon);
        dam->amount = rand_1n(weapon_wc(weapon) + game_difficulty(nlarn));
    }

    /* add variable damage */
    if (att.rand) dam->amount += rand_1n(att.rand);

    /* handle some damage types here */
    switch (dam->type)
    {
    case DAM_STEAL_GOLD:
    case DAM_STEAL_ITEM:
        if (monster_player_rob(m, p, (dam->type == DAM_STEAL_GOLD) ? IT_GOLD : IT_ALL))
        {
            /* teleport away */
            monster_pos_set(m, game_map(nlarn, m->pos.z),
                            map_find_space(game_map(nlarn, m->pos.z), LE_MONSTER, FALSE));
        }

        damage_free(dam);
        break;

    case DAM_RUST:
        log_add_entry(nlarn->log, "The %s %s you.", monster_get_name(m),
                      monster_attack_verb[att.type]);

        monster_item_rust(m, p);
        damage_free(dam);
        break;

    case DAM_REM_ENCH:
        monster_item_disenchant(m, p);

        damage_free(dam);
        break;

    default:
        if (att.type != ATT_GAZE || !player_effect(p, ET_BLINDNESS))
        {
            /* log the attack */
            log_add_entry(nlarn->log, "The %s %s you.", monster_get_name(m),
                          monster_attack_verb[att.type]);
        }
        /* 50% chance of reflecting adjacent gazes */
        if (att.type == ATT_GAZE && player_effect(p, ET_REFLECTION)
            && chance(50))
        {
            if (!player_effect(p, ET_BLINDNESS))
                log_add_entry(nlarn->log, "The gaze is reflected harmlessly.");
        }
        else
            player_damage_take(p, dam, PD_MONSTER, m->type);
        break;
    }
}

int monster_player_ranged_attack(monster *m, player *p)
{
    damage *dam;
    attack att = { ATT_NONE };

    assert(m != NULL && p != NULL);

    /* choose a random attack type */
    att = monster_attack(m, rand_1n(monster_attack_count(m) + 1));
    if (att.type == ATT_GAZE && chance(att.base/3))
    {
        if (!player_effect(p, ET_BLINDNESS))
        {
            log_add_entry(nlarn->log, "The %s %s you.", monster_get_name(m),
                          monster_attack_verb[att.type]);
        }
        if (player_effect(p, ET_REFLECTION))
        {
            if (!player_effect(p, ET_BLINDNESS))
                log_add_entry(nlarn->log, "The gaze is reflected harmlessly.");
        }
        else
        {
            dam = damage_new(att.damage, att.type,
                             att.base + game_difficulty(nlarn), m);
            player_damage_take(p, dam, PD_MONSTER, m->type);
        }
        return TRUE;
    }
    if (att.type != ATT_BREATH)
        return FALSE;

    return handle_breath_attack(m, p, att);
}

monster *monster_damage_take(monster *m, damage *dam)
{
    struct player *p;
    int hp_orig;

    assert(m != NULL && dam != NULL);

    p = (player *)dam->originator;
    hp_orig = m->hp;

    /* FIXME: implement resistances */
    switch (dam->type)
    {
    case DAM_PHYSICAL:
        /* FIXME: the following does currently not work as the combat system sucks */
        /* dam->amount -= monster_ac(m); */
        break;

    case DAM_FIRE:
        if (monster_flags(m, MF_RES_FIRE))
        {
            dam->amount /= 2;
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s %sresists the flames.",
                              monster_name(m),
                              dam->amount > 0 ? "partly " : "");
            }
        }
        break;

    default:
        break;
    }

    /* substract damage from HP;
     * prevent adding to HP after resistance has lowered damage amount */
    m->hp -= max(0, dam->amount);

    if (m->hp < hp_orig)
    {
        /* monster has been hit */
        if (p)
        {
            /* notify player */
        }

        /* metamorph transforms if HP is low*/
        if (m->type == MT_METAMORPH)
        {
            if ((m->hp < 25) && (m->hp > 0))
            {
                gboolean seen_old = monster_in_sight(m);
                m->type = MT_BRONZE_DRAGON + rand_0n(9);
                gboolean seen_new = monster_in_sight(m);

                if (p && (seen_old || seen_new))
                {
                    if (seen_old && seen_new)
                    {
                        log_add_entry(nlarn->log, "The metamorph turns into a %s!",
                                      monster_name(m));
                    }
                    else if (seen_old)
                        log_add_entry(nlarn->log, "The metamorph vanishes!");
                    else
                    {
                        log_add_entry(nlarn->log, "A %s suddenly appears!",
                                      monster_name(m));
                    }
                }
            }
        }
    }
    else
    {
        /* monster is not affected */
        if (p)
        {
            /* notify player */
        }
    }

    if (m->hp < 1)
    {
        /* monster dies */
        monster_die(m, p);
        m = NULL;
    }

    g_free(dam);

    return m;
}

gboolean monster_update_action(monster *m)
{
    monster_action_t naction;   /* new action */
    guint time;
    gboolean low_hp;
    gboolean smart;

    /* FIXME: should include difficulty here */
    time   = monster_int(m) + 25;
    low_hp = (m->hp < (monster_hp_max(m) / 4 ));
    smart  = (monster_int(m) > 4);

    if (monster_flags(m, MF_MIMIC) && m->unknown)
    {
        /* stationary monsters */
        naction = MA_REMAIN;
    }
    else if (monster_effect(m, ET_HOLD_MONSTER) || monster_effect(m, ET_SLEEP)
             || monster_effect(m, ET_TRAPPED))
    {
        /* no action if monster is held or sleeping */
        naction = MA_REMAIN;
    }
    else if ((low_hp && smart) || (monster_effect(m, ET_SCARE_MONSTER) > monster_int(m)))
    {
        /* low HP or very scared => FLEE from player */
        naction = MA_FLEE;
    }
    /* town people never attack the player */
    else if (monster_type(m) != MT_TOWN_PERSON
                && m->lastseen && (m->lastseen < time))
    {
        /* after having spotted the player, agressive monster will follow
           the player for a certain amount of time turns, afterwards loose
           interest. More peaceful monsters will do something else. */
        /* TODO: need to test for agressiveness */
        naction = MA_ATTACK;
    }
    else
    {
        /* if no action could be found, return to original behaviour */
        naction = MA_WANDER;
    }

    if (naction != m->action)
    {
        m->action = naction;
        return TRUE;
    }

    return FALSE;
}

void monster_update_player_pos(monster *m, position ppos)
{
    assert (m != NULL);

    m->player_pos = ppos;
    m->lastseen = 1;
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
    if (monster_flags(m, MF_REGENERATE))
    {
        /* regenerate every (22- frequency) turns */
        if (gtime % (22 - frequency) == 0)
            m->hp = min(monster_hp_max(m), m->hp++);

    }

    /* handle poison */
    if ((e = monster_effect_get(m, ET_POISON)))
    {
        if ((gtime - e->start) % (22 + frequency) == 0)
        {
            m->hp -= e->amount;
        }

        if (m->hp < 1)
        {
            /* monster died from poison */
            return FALSE;
        }
    }

    return TRUE;
}

char *monster_desc(monster *m)
{
    int hp_rel;
    GString *desc;
    char *injury, *effects = NULL;

    assert (m != NULL);

    hp_rel = (((float)m->hp / (float)monster_hp_max(m)) * 100);

    /* prepare health status description */
    if (m->hp == monster_hp_max(m))
        injury = "uninjured";
    else if (hp_rel > 80)
        injury = "slightly injured";
    else if (hp_rel > 20)
        injury = "injured";
    else if (hp_rel > 10)
        injury = "heavily injured";
    else
        injury = "critically injured";

    desc = g_string_new(NULL);

    if (monster_unknown(m))
    {
        /* an undiscovered mimic will be described as the item it mimics */
        g_string_append_printf(desc, "%s %s",
                               a_an(item_data[monster_item_type(m)].name_sg),
                               item_data[monster_item_type(m)].name_sg);

        return g_string_free(desc, FALSE);
    }

    g_string_append_printf(desc, "%s %s %s, %s", a_an(injury),
                           injury, monster_get_name(m),
                           monster_ai_desc[m->action]);

    /* add effect description */
    if (m->effects->len > 0)
    {
        char **desc_list = strv_new();

        int i;
        for (i = 0; i < m->effects->len; i++)
        {
            effect *e = game_effect_get(nlarn, g_ptr_array_index(m->effects, i));

            if (effect_get_desc(e))
            {
                strv_append_unique(&desc_list, effect_get_desc(e));
            }
        }

        effects = g_strjoinv(", ", desc_list);
        g_strfreev(desc_list);

        g_string_append_printf(desc, " (%s)", effects);

        g_free(effects);
    }


    return g_string_free(desc, FALSE);
}

char monster_glyph(monster *m)
{
    assert (m != NULL);

    if (m->unknown)
    {
        return item_glyph(m->item_type);
    }
    else
    {
        return luaN_query_char("monsters", m->type, "glyph");
    }
}

int monster_color(monster *m)
{
    assert (m != NULL);

    if (m->unknown)
    {
        return m->colour;
    }
    else
    {
        return luaN_query_int("monsters", m->type, "color");
    }
}

void monster_genocide(int monster_id)
{
    GList *mlist;
    monster *monst;

    assert(monster_id > MT_NONE && monster_id < MT_MAX);

    nlarn->monster_genocided[monster_id] = TRUE;
    mlist = g_hash_table_get_values(nlarn->monsters);

    /* purge genocided monsters */
    do
    {
        monst = (monster *)mlist->data;
        if (monster_is_genocided(monst->type))
            monster_destroy(monst);
    }
    while ((mlist = mlist->next));

    g_list_free(mlist);
}

int monster_is_genocided(int monster_id)
{
    assert(monster_id > MT_NONE && monster_id < MT_MAX);
    return nlarn->monster_genocided[monster_id];
}

effect *monster_effect_add(monster *m, effect *e)
{
    assert(m != NULL && e != NULL);

    if (e->type == ET_SLEEP && monster_flags(m, MF_RES_SLEEP))
        return NULL;

    if (e->type == ET_POISON && monster_flags(m, MF_RES_POISON))
        return NULL;

    e = effect_add(m->effects, e);

    /* show message if monster is visible */
    if (e && monster_in_sight(m) && effect_get_msg_m_start(e))
    {
        log_add_entry(nlarn->log, effect_get_msg_m_start(e),
                      monster_name(m));
    }

    return e;
}

int monster_effect_del(monster *m, effect *e)
{
    int result;

    assert(m != NULL && e != NULL);

    /* log info if the player can see the monster */
    if (monster_in_sight(m) && effect_get_msg_m_stop(e))
    {
        log_add_entry(nlarn->log, effect_get_msg_m_stop(e), monster_name(m));
    }

    if ((result = effect_del(m->effects, e)))
    {
        effect_destroy(e);
    }

    return result;
}

effect *monster_effect_get(monster *m , effect_type type)
{
    assert(m != NULL && type < ET_MAX);
    return effect_get(m->effects, type);
}

int monster_effect(monster *m, effect_type type)
{
    assert(m != NULL && type < ET_MAX);
    return effect_query(m->effects, type);
}

void monster_effects_expire(monster *m)
{
    guint idx = 0;

    assert(m != NULL);

    while (idx < m->effects->len)
    {
        gpointer effect_id = g_ptr_array_index(m->effects, idx);;
        effect *e = game_effect_get(nlarn, effect_id);

        if (monster_effect(m, ET_TRAPPED))
        {
            if (monster_effect(m, ET_HOLD_MONSTER)
                    || monster_effect(m, ET_SLEEP))
            {
                continue;
            }
        }

        if (effect_expire(e) == -1)
        {
            /* effect has expired */
            monster_effect_del(m, e);
        }
        else
        {
            idx++;
        }
    }
}

static gboolean monster_player_visible(monster *m)
{
    /* FIXME: this ought to be different per monster type */
    int monster_visrange = 7;

    if (player_effect(nlarn->p, ET_STEALTH))
    {
        /* if the player is stealthy monsters will only recognize him when
           standing next to him */
        monster_visrange = 1;
    }

    /* determine if the monster can see the player */
    if (pos_distance(monster_pos(m), nlarn->p->pos) > monster_visrange)
        return FALSE;

    if (player_effect(nlarn->p, ET_INVISIBILITY) && !monster_flags(m, MF_INFRAVISION))
        return FALSE;

    /* monster is blinded */
    if (monster_effect(m, ET_BLINDNESS))
        return FALSE;

    /* determine if player's position is visible from monster's position */
    return map_pos_is_visible(monster_map(m), m->pos, nlarn->p->pos);
}

static gboolean monster_attack_available(monster *m, attack_t type)
{
    gboolean available = FALSE;
    int pos = 1;
    int c = monster_attack_count(m);

    while (pos <= c)
    {
        attack att = monster_attack(m, pos);

        if (att.type == type)
        {
            available = TRUE;
            break;
        }

        pos++;
    }

    return available;
}

static item *monster_weapon_select(monster *m)
{
    item *best = NULL, *curr = NULL;
    guint idx = 0;

    for (idx = 0; idx < inv_length(m->inventory); idx++)
    {
        curr = inv_get(m->inventory, idx);

        if (curr->type == IT_WEAPON)
        {
            if (best == NULL)
            {
                best = curr;
            }
            else if (weapon_wc(curr) > weapon_wc(best))
            {
                best = curr;
            }
        }
    }

    return best;
}

static void monster_weapon_wield(monster *m, item *weapon)
{
    char buf[61] = { 0 };

    /* FIXME: time management */
    m->weapon = weapon->oid;

    /* show message if monster is visible */
    if (monster_in_sight(m))
    {
        item_describe(weapon, player_item_identified(nlarn->p, weapon),
                      TRUE, FALSE, buf, 60);

        log_add_entry(nlarn->log, "The %s wields %s.",
                      monster_name(m), buf);
    }
}

static gboolean monster_item_disenchant(monster *m, struct player *p)
{
    item *it;

    assert (m != NULL && p != NULL);

    /* disenchant random item */
    if (!inv_length(p->inventory))
    {
        /* empty inventory */
        return FALSE;
    }

    it = inv_get(p->inventory, rand_0n(inv_length(p->inventory)));

    /* log the attack */
    log_add_entry(nlarn->log, "The %s hits you.",
                  monster_get_name(m));

    /* Don't destroy the potion of cure dianthroritis. */
    if (it->type == IT_POTION && it->id == PO_CURE_DIANTHR)
        return (inv_length(p->inventory) > 1);

    // Blessed items have a 50% chance of resisting the disenchantment.
    if (it->blessed && chance(50))
    {
        char desc[81] = { 0 };
        item_describe(it, player_item_known(nlarn->p, it),
                      (it->count == 1), TRUE, desc, 80);

        desc[0] = g_ascii_toupper(desc[0]);
        log_add_entry(nlarn->log, "%s resist%s the attack.",
                      desc, (it->count == 1) ? "s" : "");

        it->blessed_known = TRUE;
        return TRUE;
    }
    log_add_entry(nlarn->log, "You feel a sense of loss.");

    if (it->type == IT_WEAPON
            || it->type == IT_ARMOUR
            || it->type == IT_RING
            || it->type == IT_AMULET)
    {
        item_disenchant(it);
    }
    else
    {
        player_item_destroy(p, it);
    }

    return TRUE;
}

/**
 * Special monster attack: rust players armour.
 *
 * @param the attacking monster
 * @param the player
 *
 */
static gboolean monster_item_rust(monster *m, struct player *p)
{
    item **it;

    assert(m != NULL && p != NULL);

    /* get a random piece of armour to damage */
    if ((it = player_get_random_armour(p)))
    {
        *it = item_erode(&p->inventory, *it, IET_RUST, TRUE);
        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }
}

static gboolean monster_player_rob(monster *m, struct player *p, item_t item_type)
{
    guint player_gold = 0;
    item *it = NULL;

    assert (m != NULL && p != NULL);

    /* if player has a device of no theft abort the theft */
    if (player_effect(p, ET_NOTHEFT))
        return FALSE;

    /* Leprechaun robs only gold */
    if (item_type == IT_GOLD)
    {
        if ((player_gold = player_get_gold(p)))
        {
            if (player_gold > 32767)
            {
                it = item_new(IT_GOLD, player_gold >> 1);
                player_set_gold(p, player_gold >> 1);
            }
            else
            {
                it = item_new(IT_GOLD, rand_1n(1 + (player_gold >> 1)));
                player_set_gold(p, player_gold - it->count);
            }

            log_add_entry(nlarn->log, "The %s picks your pocket. " \
                          "Your purse feels lighter.", monster_get_name(m));
        }
        else
        {
            inventory *inv = *map_ilist_at(monster_map(m), p->pos);

            if (inv != NULL)
            {
                int idx = 0;
                for (; idx < inv_length(inv); idx++)
                {
                    item *i = inv_get(inv, idx);
                    if (i->type == IT_GOLD)
                    {
                        it = inv_get(inv, idx);
                        inv_del_element(map_ilist_at(monster_map(m), p->pos), it);
                        if (monster_in_sight(m))
                        {
                            log_add_entry(nlarn->log, "The %s picks up some gold at your feet. ",
                                          monster_get_name(m));
                        }
                        break;
                    }
                }
            }
        }
    }
    else if (item_type == IT_ALL) /* must be the nymph */
    {
        if (inv_length(p->inventory))
        {
            it = inv_get(p->inventory, rand_0n(inv_length(p->inventory)));

            char buf[61];
            item_describe(it, player_item_known(p, it), it->count, FALSE, buf,
                          60);

            if (player_item_is_equipped(p, it))
            {
                if (it->cursed)
                {
                    /* cursed items can't be stolen.. */
                    log_add_entry(nlarn->log, "The %s tries to steal %s but failed.",
                                  monster_get_name(m), buf);

                    /* return true as there actually are things to steal */
                    return TRUE;
                }

                log_disable(nlarn->log);
                player_item_unequip(p, NULL, it);
                log_enable(nlarn->log);
            }

            inv_del_element(&p->inventory, it);
            log_add_entry(nlarn->log, "The %s picks your pocket and steals %s.",
                          monster_get_name(m), buf);
        }
    }

    /* if item / gold has been stolen, add it to the monster's inv */
    if (it)
    {
        inv_add(&m->inventory, it);
        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "The %s couldn't find anything to steal.",
                      monster_get_name(m));

        return FALSE;
    }
}

static char *monsters_get_fortune(char *fortune_file)
{
    /* array of pointers to fortunes */
    static GPtrArray *fortunes = NULL;

    if (!fortunes) {

        /* read in the fortunes */

        size_t len = 0;
        char buffer[80];
        char *tmp = 0;
        FILE *fortune_fd;

        fortunes = g_ptr_array_new();

        /* open the file */
        fortune_fd = fopen(fortune_file, "r");
        if (fortune_fd == NULL) {
            /* can't find file */
            tmp = "Help me! I can't find the fortune file!";
            g_ptr_array_add(fortunes, tmp);
        }
        else
        {
            /* read in the entire fortune file */
            while((fgets(buffer, 79, fortune_fd))) {
                /* replace EOL with \0 */
                len = (size_t)(strchr(buffer, '\n') - (char *)&buffer);
                buffer[len] = '\0';

                /* keep the line */
                tmp = g_malloc((len + 1) * sizeof(char));
                memcpy(tmp, &buffer, (len + 1));
                g_ptr_array_add(fortunes, tmp);
            }

            fclose(fortune_fd);
        }
    }

    return g_ptr_array_index(fortunes, rand_0n(fortunes->len));
}

static position monster_move_wander(monster *m, struct player *p)
{
    if (monster_type(m) == MT_TOWN_PERSON)
    {
        if (pos_adjacent(monster_pos(m), p->pos))
        {
            // talk
            log_add_entry(nlarn->log, "The %s says, \"%s\"",
                          monster_get_name(m),
                          monsters_get_fortune(game_fortunes(nlarn)));
        }
        else if (m->lastseen > 50)
        {
            m->lastseen = 2;
            if (chance(20))
                m->colour = rand_1n(40);
        }
    }

    int tries = 0;
    position npos = monster_pos(m);

    do
    {
        npos = pos_move(m->pos, rand_1n(GD_MAX));
        tries++;
    }
    while ((!pos_valid(npos)
            || !lt_is_passable(map_tiletype_at(monster_map(m), npos))
            || map_is_monster_at(monster_map(m), npos))
            && (tries < GD_MAX));

    /* new position has not been found, reset to current position */
    if (tries == GD_MAX) npos = monster_pos(m);

    return npos;
}

static position monster_move_attack(monster *m, struct player *p)
{
    /* path to player */
    map_path *path = NULL;
    map_path_element *el = NULL;

    position npos = monster_pos(m);

    /* monster is standing next to player */
    if (pos_adjacent(monster_pos(m), m->player_pos) && (m->lastseen == 1))
    {
        monster_player_attack(m, p);

        /* monster's position might have changed (teleport) */
        if (!pos_identical(npos, monster_pos(m)))
        {
            log_add_entry(nlarn->log, "The %s vanishes.", monster_name(m));
        }

        return monster_pos(m);
    }

    /* monster is standing on a map exit and the player has left the map */
    if (pos_identical(monster_pos(m), m->player_pos)
            && map_is_exit_at(monster_map(m), monster_pos(m)))
    {
        int newmap;

        switch (map_sobject_at(monster_map(m), monster_pos(m)))
        {
        case LS_STAIRSDOWN:
        case LS_DNGN_ENTRANCE:
            newmap = m->pos.z + 1;
            break;

        case LS_STAIRSUP:
        case LS_DNGN_EXIT:
            newmap = m->pos.z - 1;
            break;

        case LS_ELEVATORDOWN:
            /* move into the volcano from the town */
            newmap = MAP_DMAX + 1;
            break;

        case LS_ELEVATORUP:
            /* volcano monster enters the town */
            newmap = 0;
            break;

        default:
            newmap = m->pos.z;
            break;
        }

        /* change the map */
        monster_level_enter(m, game_map(nlarn, newmap));

        return monster_pos(m);
    }

    /* monster heads into the direction of the player. */

    path = map_find_path(monster_map(m), monster_pos(m), m->player_pos);

    if (path && !g_queue_is_empty(path->path))
    {
        el = g_queue_pop_head(path->path);
        npos = el->pos;
    }
    else
    {
        /* no path found. stop following player */
        m->lastseen = 0;
    }

    /* cleanup */
    if (path)
    {
        map_path_destroy(path);
    }

    return npos;

}

static position monster_move_flee(monster *m, struct player *p)
{
    int tries;
    int dist = 0;
    position npos_tmp = monster_pos(m);
    position npos = monster_pos(m);

    for (tries = 1; tries < GD_MAX; tries++)
    {
        /* try all fields surrounding the monster if the
         * distance between monster & player is greater */
        if (tries == GD_CURR)
            continue;

        npos_tmp = pos_move(monster_pos(m), tries);

        if (pos_valid(npos_tmp)
                && lt_is_passable(map_tiletype_at(monster_map(m), npos_tmp))
                && !map_is_monster_at(monster_map(m), npos_tmp)
                && (pos_distance(p->pos, npos_tmp) > dist))
        {
            /* distance is bigger than current distance */
            npos = npos_tmp;
            dist = pos_distance(m->player_pos, npos_tmp);
        }
    }

    return npos;
}

