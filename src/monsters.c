/*
 * monsters.c
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
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "fov.h"
#include "game.h"
#include "items.h"
#include "map.h"
#include "monsters.h"
#include "nlarn.h"
#include "random.h"

/* monster information hiding */
struct _monster
{
    monster_t type;
    gpointer oid;            /* monsters id inside the monster hash */
    gint32 hp_max;
    gint32 hp;
    position pos;
    fov *fv;
    int movement;
    monster_action_t action; /* current action */
    guint32 lastseen;        /* number of turns since when player was last seen; 0 = never */
    position player_pos;     /* last known position of player */
    inventory *inv;
    item *eq_weapon;
    GPtrArray *effects;
    guint number;        /* random value for some monsters */
    guint32
        unknown: 1;      /* monster is unknown (mimic) */
};

const char *monster_ai_desc[MA_MAX] =
{
    NULL,               /* MA_NONE */
    "fleeing",          /* MA_FLEE */
    "standing still",   /* MA_REMAIN */
    "wandering",        /* MA_WANDER */
    "attacking",        /* MA_ATTACK */
    "puzzled",          /* MA_CONFUSION */
    "serving you",      /* MA_SERVE */
    "doing something boring", /* MA_CIVILIAN */
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

static struct _monster_breath_data
{
    const char *desc;
    const char glyph;
    int colour;
} monster_breath_data[] =
{
    { NULL, 0, 0 },                                 /* DAM_NONE */
    { NULL, 0, 0 },                                 /* DAM_PHYSICAL */
    { "psionic blast", '*', DC_WHITE },             /* DAM_MAGICAL */
    { "burst of fire", '~', DC_RED },               /* DAM_FIRE */
    { "beam of frost", '*', DC_LIGHTCYAN },         /* DAM_COLD */
    { "gush of acid", '*', DC_LIGHTGREEN },         /* DAM_ACID */
    { "flood of water", '~', DC_BLUE },             /* DAM_WATER */
    { "ray of lightning", '*', DC_YELLOW },         /* DAM_ELECTRICITY */
    { "burst of noxious fumes", '%', DC_GREEN },    /* DAM_POISON */
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
static position monster_move_confused(monster *m, struct player *p);
static position monster_move_flee(monster *m, struct player *p);
static position monster_move_serve(monster *m, struct player *p);
static position monster_move_civilian(monster *m, struct player *p);

static gboolean monster_breath_hit(const GList *traj,
        const damage_originator *damo,
        gpointer data1, gpointer data2);

monster *monster_new(monster_t type, position pos)
{
    g_assert(type > MT_NONE && type < MT_MAX && pos_valid(pos));

    monster *nmonster;
    item_t itype;      /* item type */

    /* check if supplied position is suitable for a monster */
    if (!map_pos_validate(game_map(nlarn, Z(pos)), pos, LE_MONSTER, FALSE))
    {
        return NULL;
    }

    /* don't create genocided monsters */
    if (monster_is_genocided(type))
    {
        /* try to find a replacement for the demon prince */
        if (type == MT_DEMON_PRINCE)
            return monster_new(MT_DEMONLORD_I + rand_0n(7), pos);
        else
            return NULL;
    }

    /* make room for monster */
    nmonster = g_malloc0(sizeof(monster));

    nmonster->type = type;

    /* determine max hp; prevent the living dead */
    nmonster->hp_max = nmonster->hp = max(1, divert(monster_type_hp_max(type), 10));

    nmonster->effects = g_ptr_array_new();
    nmonster->inv = inv_new(nmonster);

    /* fill monsters inventory */
    if (monster_gold_amount(nmonster) > 0)
    {
        const int gold_chance = monster_gold_chance(nmonster);
        if (gold_chance == 0 || chance(gold_chance))
        {
            /* add gold to monster's inventory, randomize the amount */
            int gcount = max(divert(monster_gold_amount(nmonster), 30), 1);
            inv_add(&nmonster->inv, item_new(IT_GOLD, gcount));
        }
    }

    /* add special items */
    switch (type)
    {
    case MT_LEPRECHAUN:
        if (chance(25))
        {
            inv_add(&nmonster->inv, item_new_random(IT_GEM, FALSE));
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
            itype = rand_1n(IT_MAX);
        }
        while (itype == IT_CONTAINER);

        inv_add(&nmonster->inv, item_new_by_level(itype, Z(pos)));
        break;

    case MT_TOWN_PERSON:
        /* initialize name counter */
        nmonster->number = rand_1n(40);
        /* set the AI to civilian */
        nmonster->action = MA_CIVILIAN;
        break;

    default:
        /* no fancy stuff... */
        break;
    }

    /* generate a weapon if monster can use it */
    if (monster_attack_available(nmonster, ATT_WEAPON))
    {
        int weapon_count = 3;
        int wpns[3]; /* choice of weapon types */
        item *weapon;

        /* preset weapon types */
        switch (type)
        {
        case MT_HOBGOBLIN:
        case MT_ORC:
            wpns[0] = WT_ODAGGER;
            wpns[1] = WT_OSHORTSWORD;
            wpns[2] = WT_OSPEAR;
            break;

        case MT_TROLL:
            wpns[0] = WT_CLUB;
            wpns[1] = WT_CLUB;
            wpns[2] = WT_BATTLEAXE;
            break;

        case MT_ELF:
            wpns[0] = WT_ESHORTSWORD;
            wpns[1] = WT_ESPEAR;
            weapon_count = 2;
            break;

        case MT_BUGBEAR:
        case MT_CENTAUR:
        case MT_POLTERGEIST:
            wpns[0] = WT_MACE;
            wpns[1] = WT_FLAIL;
            wpns[2] = WT_BATTLEAXE;
            break;

        case MT_VAMPIRE:
        case MT_GNOME_KING:
        case MT_WATER_LORD:
        case MT_XVART:
            wpns[0] = WT_LONGSWORD;
            wpns[1] = WT_2SWORD;
            wpns[2] = WT_SWORDSLASHING;
            break;

        default:
            wpns[0] = WT_DAGGER;
            wpns[1] = WT_SPEAR;
            wpns[2] = WT_SHORTSWORD;
            break;
        }

        weapon = item_new(IT_WEAPON, wpns[rand_0n(weapon_count)]);
        item_new_finetouch(weapon);

        inv_add(&nmonster->inv, weapon);

        /* wield the new weapon */
        monster_weapon_wield(nmonster, weapon);
    } /* finished initializing weapons */

    /* initialize mimics */
    if (monster_flags(nmonster, MF_MIMIC))
    {
        const int possible_types[] = { IT_AMULET, IT_GOLD, IT_RING, IT_GEM,
                                       IT_CONTAINER, IT_BOOK, IT_POTION,
                                       IT_SCROLL
                                     };

        /* put mimicked item into monster inventory */
        const int chosen_type = possible_types[rand_0n(8)];
        item *itm = item_new_by_level(chosen_type, Z(nmonster->pos));
        inv_add(&nmonster->inv, itm);

        /* the mimic is not known to be a monster */
        nmonster->unknown = TRUE;
    }

    /* initialize AI */
    if (nmonster->action == MA_NONE)
        nmonster->action = MA_WANDER;
    nmonster->player_pos = pos_invalid;

    /* register monster with game */
    nmonster->oid = game_monster_register(nlarn, nmonster);

    /* set position */
    nmonster->pos = pos;

    /* link monster to tile */
    map_set_monster_at(game_map(nlarn, Z(pos)), pos, nmonster);

    /* increment monster count */
    game_map(nlarn, Z(pos))->mcount++;

    return nmonster;
}

monster *monster_new_by_level(position pos)
{
    g_assert(pos_valid(pos));

    const int mlevel[] = { MT_KOBOLD,           // D1:   5
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

    const int nlevel = Z(pos);
    int monster_id = MT_NONE;

    if (nlevel == 0)
    {
        /* only town persons in town */
        monster_id = MT_TOWN_PERSON;
    }
    else
    {
        /* everything else in the dungeons */
        int minstep = nlevel - 4;
        int maxstep = nlevel - 1;

        int monster_id_min;
        int monster_id_max;

        if (chance(2*game_difficulty(nlarn)))
            maxstep += 2;
        else if (chance(7*(game_difficulty(nlarn) + 1)))
            maxstep++;
        else if (chance(10))
            minstep--;

        if (minstep < 0)
            monster_id_min = 1;
        else
            monster_id_min = mlevel[minstep] + 1;

        if (maxstep < 0)
            maxstep = 0;
        else if (maxstep > MAP_MAX - 2)
            maxstep = MAP_MAX - 2;

        monster_id_max = mlevel[maxstep];

        do
        {
            monster_id = rand_m_n(monster_id_min, monster_id_max);
        }
        while ((monster_id <= MT_NONE)
                || (monster_id >= MT_MAX)
                || nlarn->monster_genocided[monster_id]
                || chance(monster_type_reroll_chance(monster_id)));
    }

    return monster_new(monster_id, pos);
}

void monster_destroy(monster *m)
{
    g_assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);

    /* free effects */
    while (m->effects->len > 0)
    {
        gpointer effect_id = g_ptr_array_remove_index(m->effects, m->effects->len - 1);
        effect *e = game_effect_get(nlarn, effect_id);
        effect_destroy(e);
    }

    g_ptr_array_free(m->effects, TRUE);

    /* free inventory */
    if (m->inv)
        inv_destroy(m->inv, TRUE);

    /* unregister monster */
    game_monster_unregister(nlarn, m->oid);

    /* decrement monster count */
    game_map(nlarn, Z(m->pos))->mcount--;

    /* free monster's FOV if existing */
    if (m->fv)
        fov_free(m->fv);

    g_free(m);
}

void monster_serialize(gpointer oid, monster *m, cJSON *root)
{
    cJSON *mval;

    cJSON_AddItemToArray(root, mval = cJSON_CreateObject());
    cJSON_AddNumberToObject(mval, "type", monster_type(m));
    cJSON_AddNumberToObject(mval, "oid", GPOINTER_TO_UINT(oid));
    cJSON_AddNumberToObject(mval, "hp_max", m->hp_max);
    cJSON_AddNumberToObject(mval, "hp", m->hp);
    cJSON_AddNumberToObject(mval,"pos", pos_val(m->pos));
    cJSON_AddNumberToObject(mval, "movement", m->movement);
    cJSON_AddNumberToObject(mval, "action", m->action);

    if (m->eq_weapon != NULL)
        cJSON_AddNumberToObject(mval, "eq_weapon",
                        GPOINTER_TO_UINT(m->eq_weapon->oid));

    if (m->number)
        cJSON_AddNumberToObject(mval, "number", m->number);

    if (m->unknown)
        cJSON_AddTrueToObject(mval, "unknown");

    if (m->lastseen != 0)
    {
        cJSON_AddNumberToObject(mval,"lastseen", m->lastseen);
        cJSON_AddNumberToObject(mval,"player_pos", pos_val(m->player_pos));
    }

    /* inventory */
    if (inv_length(m->inv) > 0)
    {
        cJSON_AddItemToObject(mval, "inventory", inv_serialize(m->inv));
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
    pos_val(m->pos) = cJSON_GetObjectItem(mser, "pos")->valueint;
    m->movement = cJSON_GetObjectItem(mser, "movement")->valueint;
    m->action = cJSON_GetObjectItem(mser, "action")->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "eq_weapon")))
        m->eq_weapon = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    if ((obj = cJSON_GetObjectItem(mser, "number")))
        m->number = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "unknown")))
        m->unknown = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "lastseen")))
        m->lastseen = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "player_pos")))
        pos_val(m->player_pos) = obj->valueint;

    /* inventory */
    if ((obj = cJSON_GetObjectItem(mser, "inventory")))
        m->inv = inv_deserialize(obj);
    else
        m->inv = inv_new(m);

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

    /* increment the count of monsters of the map the monster is on */
    game_map(g, Z(m->pos))->mcount++;
}

int monster_hp_max(monster *m)
{
    g_assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->hp_max;
}

int monster_hp(monster *m)
{
    g_assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->hp;
}

void monster_hp_inc(monster *m, int amount)
{
    g_assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    m->hp = min(m->hp + amount, m->hp_max);
}

gpointer monster_oid(monster *m)
{
    g_assert (m != NULL);
    return m->oid;
}

position monster_pos(monster *m)
{
    g_assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->pos;
}

static int monster_map_element(monster *m)
{
    if (monster_type(m) == MT_XORN)
        return LE_XORN;

    if (monster_flags(m, MF_FLY))
        return LE_FLYING_MONSTER;

    if (monster_flags(m, MF_SWIM))
        return LE_SWIMMING_MONSTER;

    return LE_MONSTER;
}

int monster_valid_dest(map *m, position pos, int map_elem)
{
    /* only civilians use LE_GROUND and can't move through the player */
    if (map_elem == LE_GROUND && pos_identical(pos, nlarn->p->pos))
        return FALSE;

    switch (map_tiletype_at(m, pos))
    {
    case LT_WALL:
        return (map_elem == LE_XORN);

    case LT_DEEPWATER:
        if (map_elem == LE_SWIMMING_MONSTER)
            return TRUE;
        // else fall through
    case LT_LAVA:
        return (map_elem == LE_FLYING_MONSTER);

    default:
        /* the map tile must be passable and there must be no monster on it*/
        return (map_pos_passable(m, pos) && !map_is_monster_at(m, pos));
    }
}

int monster_pos_set(monster *m, map *mp, position target)
{
    g_assert(m != NULL && mp != NULL && pos_valid(target));

    if (map_pos_validate(mp, target, monster_map_element(m), FALSE))
    {
        /* remove current reference to monster from tile */
        map_set_monster_at(monster_map(m), m->pos, NULL);

        /* set new position */
        m->pos = target;

        /* set reference to monster on tile */
        map_set_monster_at(mp, target, m);

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
    g_assert (m != NULL);
    return m->unknown;
}

void monster_unknown_set(monster *m, gboolean what)
{
    g_assert (m != NULL);
    m->unknown = what;
}

inventory **monster_inv(monster *m)
{
    g_assert (m != NULL);
    return &m->inv;
}

static gboolean monster_nearby(monster *m)
{
    /* different level */
    if (Z(m->pos) != Z(nlarn->p->pos))
        return FALSE;

    return fov_get(nlarn->p->fv, m->pos);
}

gboolean monster_in_sight(monster *m)
{
    g_assert (m != NULL);

    /* player is blind */
    if (player_effect(nlarn->p, ET_BLINDNESS))
        return FALSE;

    /* different level */
    if (Z(m->pos) != Z(nlarn->p->pos))
        return FALSE;

    /* invisible monster, player has no infravision */
    if (monster_flags(m, MF_INVISIBLE) && !player_effect(nlarn->p, ET_INFRAVISION))
        return FALSE;

    return fov_get(nlarn->p->fv, m->pos);
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

monster_action_t monster_action(monster *m)
{
    g_assert (m != NULL);

    return m->action;
}

// Takes visibility into account.
// For the real name, use monster_name() directly.
const char *monster_get_name(monster *m)
{
    /* only show real names of invisible monsters in
     * wizard mode when full visibility is enabled */
    if (!monster_in_sight(m) && !game_fullvis(nlarn))
        return ("unseen monster");

    if (monster_type(m) == MT_TOWN_PERSON)
        return get_town_person_name(m->number);

    return (monster_name(m));
}

const char* monster_type_plural_name(const int montype, const int count)
{
    /* result of Lua data query; monster's plural name */
    const char *mpn = luaN_query_string("monsters", montype, "plural_name");

    if (count > 1)
    {
        /* need a static buffer to return to calling functions */
        static char buf[61] = { 0 };

        if (mpn == NULL)
        {
            g_snprintf(buf, 60, "%ss", monster_type_name(montype));
        }
        else
        {
            g_strlcpy(buf, mpn, 60);
        }

        return buf;
    }

    return monster_type_name(montype);
}

void monster_die(monster *m, struct player *p)
{
    g_assert(m != NULL);

    /* if the player can see the monster describe the event */
    /* Also give a message for invisible monsters you killed yourself
       (the xp gain gives this away anyway). */
    if (monster_in_sight(m)
            || (p != NULL && map_pos_is_visible(monster_map(m),
                    p->pos, monster_pos(m))))
    {
        const char *message;

        /* give a different message if a servant is expired */
        if (monster_action(m) == MA_SERVE && m->number == 0)
            message = "The %s disappears!";
        else
            message = "The %s dies!";

        log_add_entry(nlarn->log, message, monster_get_name(m));
    }

    /* make sure mimics never leave the mimicked item behind */
    if (monster_flags(m, MF_MIMIC) && inv_length(m->inv) > 0)
    {
        inv_del(&m->inv, 0);
    }

    /* drop stuff the monster carries */
    if (inv_length(m->inv))
    {
        /* Did it fall into water? */
        const int tile = map_tiletype_at(monster_map(m), monster_pos(m));
        if (tile == LT_DEEPWATER || tile == LT_LAVA)
        {
            int count = 0;
            while (inv_length(m->inv) > 0)
            {
                item *it = inv_get(m->inv, 0);
                if (item_is_unique(it))
                {
                    /* teleport the item to safety */
                    inv_del_element(&m->inv, it);
                    map_item_add(game_map(nlarn, Z(nlarn->p->pos)), it);
                }
                else
                {
                    inv_del(&m->inv, 0);
                    count++;
                }
            }
            if (count && monster_nearby(m))
                log_add_entry(nlarn->log, "You hear a splash!");
        }
        else
        {
            /* dump items on the floor */
            inventory **floor = map_ilist_at(monster_map(m), monster_pos(m));
            while (inv_length(m->inv) > 0)
            {
                inv_add(floor, inv_get(m->inv, 0));
                inv_del(&m->inv, 0);
            }
        }
    }

    /* reward experience, but not for summoned monsters */
    if (p != NULL && (monster_action(m) != MA_SERVE))
    {
        player_exp_gain(p, monster_exp(m));
        p->stats.monsters_killed[m->type] += 1;
    }

    /* unlink the monster from its map */
    map_set_monster_at(monster_map(m), m->pos, NULL);

    /* assure that the monster's hp indicates that the monster is dead */
    if (m->hp > 0)
        m->hp = 0;

    /* add the monster to the list of dead monsters */
    g_ptr_array_add(nlarn->dead_monsters, m);
}

void monster_level_enter(monster *m, struct map *l)
{
    g_assert (m != NULL && l != NULL);

    sobject_t source = map_sobject_at(monster_map(m), m->pos);
    sobject_t target;
    position npos;
    const char *what = NULL;
    const char *how  = "comes";

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
    else

    if (!map_pos_validate(l, npos, LE_MONSTER, FALSE))
    {
        /* the position somehow isn't valid */
        return;
    }

    /* remove monster from old map  */
    map *oldmap = game_map(nlarn, Z(m->pos));
    map_set_monster_at(oldmap, m->pos, NULL);

    /* put monster into map */
    monster_pos_set(m, l, npos);

    /* reset the information of the player's last known position */
    m->lastseen = 0;

    /* log the event */
    if (monster_in_sight(m) && target)
    {
        log_add_entry(nlarn->log, "The %s %s %s %s.", monster_name(m),
                      how, what, so_get_desc(target));
    }
}

void monster_move(gpointer *oid __attribute__((unused)), monster *m, game *g)
{
    /* monster's new position */
    position m_npos;

    /* expire summoned monsters */
    if (monster_action(m) == MA_SERVE)
    {
        m->number--;

        if (m->number == 0)
        {
            /* expired */
            monster_die(m, g->p);
            return;
        }
    }

    if (monster_hp(m) < 1)
        /* Monster is already dead. */
        return;

    position mpos = monster_pos(m);

    /* modify effects */
    monster_effects_expire(m);

    /* regenerate / inflict poison upon monster. */
    if (!monster_regenerate(m, g->gtime, g->difficulty))
        /* the monster died */
        return;

    /* damage caused by map effects */
    damage *dam = map_tile_damage(monster_map(m), monster_pos(m),
                                  monster_flags(m, MF_FLY)
                                  || monster_effect(m, ET_LEVITATION));

    /* deal damage caused by floor effects */
    if ((dam != NULL) && !(m = monster_damage_take(m, dam)))
        /* the monster died */
        return;

    /* move the monster only if it is on the same map as the player or
       an adjacent map */
    gboolean map_adjacent = (Z(mpos) == Z(g->p->pos)
                             || (Z(mpos) == Z(g->p->pos) - 1)
                             || (Z(mpos) == Z(g->p->pos) + 1)
                             || (Z(mpos) == MAP_DMAX && Z(g->p->pos) == 0)
                            );
    if (!map_adjacent)
        return;

    /* Update the monster's knowledge of player's position.
       Not for civilians or servants: the first don't care,
       the latter just know. This allows to use player_pos
       and lastseen for other purposes. */
    monster_action_t ma = monster_action(m);

    if ((ma != MA_SERVE && ma != MA_CIVILIAN)
        && (monster_player_visible(m)
            || (player_effect(g->p, ET_AGGRAVATE_MONSTER)
                && pos_distance(m->pos, g->p->pos) < 15)))
    {
        monster_update_player_pos(m, g->p->pos);
    }

    /* add the monster's speed to the monster's movement points */
    m->movement += monster_speed(m);

    /* let the monster make a move as long it has movement points left */
    while (m->movement >= SPEED_NORMAL)
    {
        /* reduce the monster's movement points */
        m->movement -= SPEED_NORMAL;

        /* update monsters action */
        if (monster_update_action(m, MA_NONE) && monster_in_sight(m))
        {
            /* the monster has chosen a new action and the player
               can see the new action, so let's describe it */

            if (m->action == MA_ATTACK)
            {
                /* TODO: certain monster types will make a sound when attacking the player */
                /*
                log_add_entry(g->log,
                              "The %s has spotted you and heads towards you!",
                              monster_name(m));
                 */
            }
            else if (m->action == MA_FLEE)
            {
                log_add_entry(g->log, "The %s turns to flee!", monster_name(m));
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
            m_npos = monster_move_flee(m, g->p);
            break;

        case MA_REMAIN:
            /* Sgt. Stan Still - do nothing */
            break;

        case MA_WANDER:
            m_npos = monster_move_wander(m, g->p);
            break;

        case MA_ATTACK:
            /* monster tries a ranged attack */
            if (monster_player_visible(m)
                    && monster_player_ranged_attack(m, g->p))
                return;

            m_npos = monster_move_attack(m, g->p);
            break;

        case MA_CONFUSION:
            m_npos = monster_move_confused(m, g->p);
            break;

        case MA_SERVE:
            m_npos = monster_move_serve(m, g->p);
            break;

        case MA_CIVILIAN:
            m_npos = monster_move_civilian(m, g->p);
            break;

        case MA_NONE:
        case MA_MAX:
            /* possibly a bug */
            break;
        }

        /* ******** if new position has been found - move the monster ********* */
        if (!pos_identical(m_npos, monster_pos(m)))
        {
            /* get the monster's current map */
            map *mmap = monster_map(m);

            /* get stationary object at the monster's target position */
            sobject_t target_st = map_sobject_at(mmap, m_npos);

            /* vampires won't step onto mirrors */
            if ((m->type == MT_VAMPIRE) && (target_st == LS_MIRROR))
            {
                /* No movement - FIXME: should try to move around it */
            }

            else if (pos_identical(g->p->pos, m_npos))
            {
                /* The monster bumps into the player who is invisible to the
                   monster. Thus the monster gains knowledge over the player's
                   current position. */
                monster_update_player_pos(m, g->p->pos);

                log_add_entry(g->log, "The %s bumps into you.", monster_get_name(m));
            }

            /* check for door */
            else if ((target_st == LS_CLOSEDDOOR) && monster_flags(m, MF_HANDS))
            {
                /* dim-witted or confused monster are unable to open doors */
                if (monster_int(m) < 4 || monster_effect_get(m, ET_CONFUSION))
                {
                    /* notify the player if the door is visible */
                    if (monster_in_sight(m))
                    {
                        log_add_entry(g->log, "The %s bumps into the door.",
                                      monster_name(m));
                    }
                }
                else
                {
                    /* the monster is capable of opening the door */
                    map_sobject_set(mmap, m_npos, LS_OPENDOOR);

                    /* notify the player if the door is visible */
                    if (monster_in_sight(m))
                    {
                        log_add_entry(g->log, "The %s opens the door.",
                                      monster_name(m));
                    }
                }
            }

            /* set the monsters new position */
            else
            {
                /* check if the new position is valid for this monster */
                if (map_pos_validate(mmap, m_npos, monster_map_element(m), FALSE))
                {
                    /* the new position is valid -> reposition the monster */
                    monster_pos_set(m, mmap, m_npos);
                }
                else
                {
                    /* the new position is invalid */
                    map_tile_t nle = map_tiletype_at(mmap, m_npos);

                    switch (nle)
                    {
                        case LT_TREE:
                        case LT_WALL:
                            if (monster_in_sight(m))
                            {
                                log_add_entry(g->log, "The %s bumps into %s.",
                                              monster_name(m), mt_get_desc(nle));
                            }
                            break;

                        case LT_LAVA:
                        case LT_DEEPWATER:
                            if (monster_in_sight(m)) {
                                log_add_entry(g->log, "The %s sinks into %s.",
                                              monster_name(m), mt_get_desc(nle));
                            }
                            monster_die(m, g->p);
                            break;

                        default:
                            /* just do not move.. */
                            break;
                    }
                }

                /* check for traps */
                if (map_trap_at(mmap, monster_pos(m)))
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

void monster_polymorph(monster *m)
{
    g_assert (m != NULL);

    /* make sure mimics never leave the mimicked item behind */
    if (monster_flags(m, MF_MIMIC) && inv_length(m->inv) > 0)
    {
        inv_del(&m->inv, 0);
    }

    const map_element_t old_elem = monster_map_element(m);
    do
    {
        m->type = rand_1n(MT_MAX_GENERATED);
    }
    while (monster_is_genocided(m->type));

    /* if the new monster can't survive in this terrain, kill it */
    const map_element_t new_elem = monster_map_element(m);

    /* We need to temporarily remove the monster from it's tile
       as monster_valid_dest() tests if there is a monster on
       the tile and hence would always return false. */
    map_set_monster_at(monster_map(m), m->pos, NULL);

    /* check if the position would be valid.. */
    gboolean valid_pos = monster_valid_dest(monster_map(m), m->pos, new_elem);

    /* ..and restore the monster to it's position */
    map_set_monster_at(monster_map(m), m->pos, m);

    if (!valid_pos)
    {
        if (monster_in_sight(m))
        {
            /* briefly display the new monster before it dies */
            display_paint_screen(nlarn->p);
            g_usleep(250000);

            switch (old_elem)
            {
            case LE_FLYING_MONSTER:
                log_add_entry(nlarn->log, "The %s falls into the %s!",
                              monster_get_name(m),
                              mt_get_desc(map_tiletype_at(monster_map(m), m->pos)));
                break;
            case LE_SWIMMING_MONSTER:
                log_add_entry(nlarn->log, "The %s sinks like a rock!",
                              monster_get_name(m));
                break;
            case LE_XORN:
                log_add_entry(nlarn->log, "The %s is trapped in the wall!",
                              monster_get_name(m));
                break;
            default:
                break;
            }
        }
        monster_die(m, nlarn->p);
    }
    else
    {
        /* get the relative amount of hp left */
        float relative_hp = (float)m->hp / (float)m->hp_max;

        /* Determine the new maximum hitpoints for the new monster
           type and set the monster's current hit points to the
           relative value of the monster's remaining hit points. */
        m->hp_max = divert(monster_type_hp_max(m->type), 10);
        m->hp = (int)(m->hp_max * relative_hp);
    }
}

int monster_items_pickup(monster *m)
{
    g_assert(m != NULL);

    // The town people never take your stuff.
    if (monster_type(m) == MT_TOWN_PERSON)
        return FALSE;

    /* monsters affected by levitation can't pick up stuff */
    if (monster_effect(m, ET_LEVITATION))
        return FALSE;

    /* TODO: gelatinous cube digests items, rust monster eats metal stuff */
    /* FIXME: time management */

    gboolean pick_up = FALSE;
    item *it;

    for (guint idx = 0; idx < inv_length(*map_ilist_at(monster_map(m), m->pos)); idx++)
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
            if (m->eq_weapon == NULL)
                pick_up = TRUE;
            else
            {
                /* compare this weapon with the weapon the monster wields */
                if (m->eq_weapon == NULL || (weapon_damage(m->eq_weapon)
                                        < weapon_damage(it)))
                    pick_up = TRUE;
            }
        }

        if (pick_up)
        {
            /* The monster has picked up the item.

               Determine if the item is a weapon.
               This has to be done before adding the item to the monster's
               inventory as the item might be destroyed after calling inv_add().
               (Stackable items get destroyed if an item of the kind exists
                in the target inventory!).
            */
            gboolean new_weapon = (it->type == IT_WEAPON);

            if (monster_in_sight(m))
            {
                gchar *buf = item_describe(it, player_item_identified(nlarn->p, it),
                                           FALSE, FALSE);

                log_add_entry(nlarn->log, "The %s picks up %s.", monster_name(m), buf);
                g_free(buf);
            }

            inv_del_element(map_ilist_at(monster_map(m), m->pos), it);
            inv_add(&m->inv, it);

            /* go back one item as the following items lowered their number */
            idx--;

            if (new_weapon)
            {
                /* find out if the new weapon is better than the old one */
                item *best = monster_weapon_select(m);

                /* If the new item is a weapon, 'it' is still a valid pointer
                   to the item picked up at this point as weapons are not
                   stackable. */
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
        /* attacks table has been found; query length */
        count = lua_rawlen(nlarn->L, -1);

        /* clean up */
        lua_pop(nlarn->L, 3);
    }

    return count;
}

attack monster_attack(monster *m, int num)
{
    attack att = { ATT_NONE, DAM_NONE, 0, 0 };

    g_assert (m != NULL && num <= monster_attack_count(m));

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

static int monster_breath_attack(monster *m, player *p, attack att)
{
    g_assert(att.type == ATT_BREATH);

    /* FIXME: charm monster is extremely broken. This should be handled totally different */
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

    /* generate damage */
    damage *dam = damage_new(att.damage, att.type, att.base + game_difficulty(nlarn),
                             DAMO_MONSTER, m);

    /* the attack might have a random amount */
    if (att.rand > 0)
        dam->amount += rand_0n(att.rand);

    if (monster_in_sight(m))
    {
        log_add_entry(nlarn->log, "The %s breathes a %s!", monster_get_name(m),
                      monster_breath_data[att.damage].desc);
    }
    else
    {
        log_add_entry(nlarn->log, "A %s spews forth from nowhere!",
                      monster_breath_data[att.damage].desc);
    }

    /* handle the breath */
    map_trajectory(m->pos, p->pos, &(dam->dam_origin),
                   monster_breath_hit, dam, NULL, TRUE,
                   monster_breath_data[att.damage].glyph,
                   monster_breath_data[att.damage].colour, TRUE);

    /* the damage is copied in monster_breath_hit(), thus destroy the
       original damage here */
    damage_free(dam);

    return FALSE;
}

static int modified_attack_amount(int amount, int damage_type)
{
    if (damage_type == DAM_POISON)
        return amount + (game_difficulty(nlarn) + 1)/2;

    return amount + game_difficulty(nlarn)/2;
}

void monster_player_attack(monster *m, player *p)
{
    g_assert(m != NULL && p != NULL);

    damage *dam;
    map *mmap = game_map(nlarn, Z(m->pos));
    attack att = { ATT_NONE, DAM_NONE, 0, 0 };

    /* the player is invisible and the monster bashes into thin air */
    if (!pos_identical(m->player_pos, p->pos))
    {
        if (!map_is_monster_at(mmap, p->pos) && monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s bashes into thin air.",
                          monster_name(m));
        }

        m->lastseen++;

        return;
    }

    /* player is invisible and monster tries to hit player */
    if (player_effect(p, ET_INVISIBILITY) && !(monster_flags(m, MF_INFRAVISION)
                                               || monster_effect(m, ET_INFRAVISION))
            && chance(65))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s misses wildly.",
                          monster_get_name(m));
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

    /* No attack has been found. Return to calling function. */
    if (att.type == ATT_NONE) return;

    /* handle breath attacks separately */
    if (att.type == ATT_BREATH)
    {
        monster_breath_attack(m, p, att);
        return;
    }

    /* generate damage */
    dam = damage_new(att.damage, att.type,
                     modified_attack_amount(att.base, att.damage),
                     DAMO_MONSTER, m);

    /* deal with random damage (spirit naga) */
    if (dam->type == DAM_RANDOM)
        dam->type = rand_1n(DAM_MAX);

    if (dam->type == DAM_DEC_RND)
        dam->type = rand_m_n(DAM_DEC_CON, DAM_DEC_RND);

    /* set damage for weapon attacks */
    if (att.type == ATT_WEAPON)
    {
        /* make monster size affect weapon damage */
        /* FIXME: handle the vorpal blade */
        dam->amount  = (m->eq_weapon != NULL) ? weapon_damage(m->eq_weapon) : 1
                        + (int)rand_0n(game_difficulty(nlarn) + 2)
                        + monster_level(m)
                        + 2 * ((monster_size(m) - ESIZE_MEDIUM)) / 25;
    }
    else if (dam->type == DAM_PHYSICAL)
    {
        /* increase damage with difficulty */
        dam->amount = att.base
                      + monster_level(m)
                      + rand_0n(game_difficulty(nlarn) + 2);
    }

    /* add variable damage */
    if (att.rand) dam->amount += rand_1n(att.rand);

    /* half damage if player is protected against spirits */
    if (player_effect(p, ET_SPIRIT_PROTECTION) && monster_flags(m, MF_SPIRIT))
    {
        if (dam->type == DAM_PHYSICAL)
        {
            /* halve physical damage */
            dam->amount >>= 1;
        }
        else
        {
            /* FIXME: give log message */
            damage_free(dam);

            return;
        }
    }

    /* handle some damage types here */
    switch (dam->type)
    {
    case DAM_STEAL_GOLD:
    case DAM_STEAL_ITEM:
        if (monster_player_rob(m, p, (dam->type == DAM_STEAL_GOLD) ? IT_GOLD : IT_ALL))
        {
            /* teleport away */
            monster_pos_set(m, mmap, map_find_space(mmap, LE_MONSTER, FALSE));
        }

        damage_free(dam);
        break;

    case DAM_RUST:
        log_add_entry(nlarn->log, "The %s %s you.", monster_get_name(m),
                      monster_attack_verb[att.type]);

        monster_item_rust(m, p);
        p->attacked = TRUE;
        damage_free(dam);
        break;

    case DAM_REM_ENCH:
        monster_item_disenchant(m, p);
        p->attacked = TRUE;
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
    g_assert(m != NULL && p != NULL);

    /* choose a random attack type */
    attack att = monster_attack(m, rand_1n(monster_attack_count(m) + 1));
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
            damage *dam = damage_new(att.damage, att.type,
                    att.base + game_difficulty(nlarn), DAMO_MONSTER, m);

            player_damage_take(p, dam, PD_MONSTER, m->type);
        }
        return TRUE;
    }
    if (att.type != ATT_BREATH)
        return FALSE;

    return monster_breath_attack(m, p, att);
}

monster *monster_damage_take(monster *m, damage *dam)
{
    struct player *p = NULL;

    g_assert(m != NULL && dam != NULL);

    if (dam->dam_origin.ot == DAMO_PLAYER)
        p = (player *)dam->dam_origin.originator;

    if (game_wizardmode(nlarn) && fov_get(nlarn->p->fv, m->pos))
        log_add_entry(nlarn->log, damage_to_str(dam));

    int hp_orig = m->hp;

    switch (dam->type)
    {
    case DAM_PHYSICAL:
        dam->amount -= monster_ac(m);
        if (dam->amount < 1 && monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s isn't hurt.", monster_name(m));
        }
        break;

    case DAM_MAGICAL:
        if (monster_flags(m, MF_RES_MAGIC))
        {
            dam->amount /= monster_level(m);
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s %sresists the magic.",
                        monster_name(m), dam->amount > 0 ? "partly " : "");
            }
        }
        break;

    case DAM_FIRE:
        if (monster_flags(m, MF_RES_FIRE))
        {
            /*
             * The monster's fire resistance reduces the damage taken
             * by 5% per monster level
             */
            dam->amount -= (guint)(((float)dam->amount / 100) *
                 /* prevent uint wrap around for monsters with level > 20 */
                 (min(monster_level(m), 20) * 5));
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s %sresists the flames.",
                        monster_name(m), dam->amount > 0 ? "partly " : "");
            }
        }
        break;

    case DAM_COLD:
        if (monster_flags(m, MF_RES_COLD))
        {
            dam->amount = 0;
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s loves the cold!",
                        monster_name(m));
            }
        }
        break;

    case DAM_WATER:
        if (monster_flags(m, MF_SWIM))
            dam->amount = 0;
        break;

    case DAM_ELECTRICITY:
        if (monster_flags(m, MF_RES_ELEC))
        {
            dam->amount = 0;
            log_add_entry(nlarn->log, "The %s is not affected!",
                          monster_name(m));
        }
        /* double damage for flying monsters */
        else if (monster_flags(m, MF_FLY) || monster_effect(m, ET_LEVITATION))
        {
            dam->amount *= 2;
            // special message?
        }
        break;

    default:
        break;
    }

    /* subtract damage from HP;
     * prevent adding to HP after resistance has lowered damage amount */
    m->hp -= max(0, dam->amount);

    if (game_wizardmode(nlarn) && fov_get(nlarn->p->fv, m->pos))
        log_add_entry(nlarn->log, "[applied: %d]", hp_orig - m->hp);

    if (m->hp < hp_orig)
    {
        /* monster has been hit */
        if (m->type == MT_METAMORPH)
        {
            /*
             * The metamorph transforms if HP is low.
             * Get the percentage of hitpoints the metamorph has left.
             * If this is less than 80%, the metamorph will turn into
             * another monster that will usually be more dangerous.
             */
            float relative_hp = (float)m->hp / (float)m->hp_max;

            if ((m->hp > 0) && (relative_hp < 0.8))
            {
                char *wdesc = NULL;
                const char *old_name = monster_name(m);
                gboolean seen_old = monster_in_sight(m);
                m->type = MT_BRONZE_DRAGON + rand_0n(9);
                gboolean seen_new = monster_in_sight(m);

                /* Determine the new maximum hitpoints for the new monster
                   type and set the monster's current hit points to the
                   relative value of the metamorphs remaining hit points. */
                if (relative_hp < 0.1) relative_hp = 0.1;
                m->hp_max = divert(monster_type_hp_max(m->type), 10);
                m->hp = (int)(m->hp_max * relative_hp);

                /* Drop the weapon if the monster the metamorph turned
                   into can not handle weapons. */
                if (m->eq_weapon && !monster_attack_available(m, ATT_WEAPON))
                {
                    /* If the monster stepped on a trap p is NULL, thus we
                       need to use nlarn->p here. */
                    wdesc = item_describe(m->eq_weapon,
                                    player_item_known(nlarn->p, m->eq_weapon),
                                    FALSE, FALSE);

                    inv_del_element(&m->inv, m->eq_weapon);
                    inv_add(map_ilist_at(monster_map(m), m->pos), m->eq_weapon);
                    m->eq_weapon = NULL;
                }

                if (p && (seen_old || seen_new))
                {
                    if (seen_old && wdesc != NULL)
                    {
                        log_add_entry(nlarn->log, "The %s drops %s.",
                                        old_name, wdesc);
                    }

                    if (seen_old && seen_new)
                    {
                        log_add_entry(nlarn->log, "The %s turns into "
                                        "a %s!", old_name, monster_name(m));
                    }
                    else if (seen_old)
                    {
                        log_add_entry(nlarn->log, "The %s vanishes!",
                                        old_name);
                    }
                    else
                    {
                        log_add_entry(nlarn->log, "A %s suddenly appears!",
                                        monster_name(m));
                    }
                }

                if (wdesc != NULL) g_free(wdesc);
            }
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

gboolean monster_update_action(monster *m, monster_action_t override)
{
    monster_action_t naction; /* new action */
    guint mtime; /* max. number of turns a monster will look for the player */
    gboolean low_hp;
    gboolean smart;

    if (override > MA_NONE)
    {
        /* set the monster's action to the requested value */
        m->action = override;

        /* if the monster is to be a servant, set its lifetime */
        if (override == MA_SERVE)
        {
            /* FIXME: it would be nice to have a variable amount of turns */
            m->number = 100;
        }
        return TRUE;
    }

    /* handle some easy action updates before the more difficult decisions */
    switch (m->action)
    {
        case MA_SERVE:     /* once servant, forever servant */
        case MA_CIVILIAN:  /* town people never change their behaviour */
        case MA_CONFUSION: /* confusion is removed by monster_effect_del() */
            return FALSE;
            break;

        default:
            /* continue to evaluate... */
            break;
    }

    mtime  = monster_int(m) + 25 + (5 * game_difficulty(nlarn));
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
    else if ((low_hp && smart) || monster_effect(m, ET_SCARED))
    {
        /* low HP or very scared => FLEE from player */
        naction = MA_FLEE;
    }
    else if (m->lastseen && (m->lastseen < mtime))
    {
        /* after having spotted the player, aggressive monster will follow
           the player for a certain amount of time turns, afterwards loose
           interest. More peaceful monsters will do something else. */
        /* TODO: need to test for aggressiveness */
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
    g_assert (m != NULL);

    m->player_pos = ppos;
    m->lastseen = 1;
}

gboolean monster_regenerate(monster *m, time_t gtime, int difficulty)
{
    /* number of turns between occasions */
    int frequency;

    /* temporary var for effect */
    effect *e;

    g_assert(m != NULL);

    /* modify frequency by difficulty: more regeneration, less poison */
    frequency = difficulty << 1;

    /* handle regeneration */
    if (monster_flags(m, MF_REGENERATE) && (m->hp < monster_hp_max(m)))
    {
        /* regenerate every (10 - difficulty) turns */
        if (gtime % (10 - difficulty) == 0)
            m->hp++;
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
            monster_die(m, NULL);
            return FALSE;
        }
    }

    return TRUE;
}

item *get_mimic_item(monster *m)
{
    g_assert(m && monster_flags(m, MF_MIMIC));

    /* polymorphed mimics may not pose as items */
    if (inv_length(m->inv) > 0)
        return inv_get(m->inv, 0);

    return NULL;
}

char *monster_desc(monster *m)
{
    int hp_rel;
    GString *desc;
    const char *injury = NULL;

    g_assert (m != NULL);

    desc = g_string_new(NULL);

    /* describe mimic as mimicked item */
    if (monster_unknown(m) && inv_length(m->inv) > 0)
    {
        item *it = get_mimic_item(m);
        gchar *item_desc= item_describe(it, player_item_known(nlarn->p, it),
                                        FALSE, FALSE);

        g_string_append_printf(desc, "You see %s there", item_desc);
        g_free(item_desc);

        return g_string_free(desc, FALSE);
    }

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

    g_string_append_printf(desc, "%s %s, %s %s", a_an(injury), injury,
                           monster_ai_desc[m->action], monster_get_name(m));

    if (game_wizardmode(nlarn))
    {
        /* show monster's hp and max hp in wizard mode */
        g_string_append_printf(desc, " (%d/%d hp)", m->hp, m->hp_max);
    }

    if (m->eq_weapon != NULL)
    {
        /* describe the weapon the monster wields */
        gchar *weapon_desc = item_describe(m->eq_weapon,
                        player_item_known(nlarn->p, m->eq_weapon),
                        TRUE, FALSE);

        g_string_append_printf(desc, ", armed with %s", weapon_desc);
        g_free(weapon_desc);
    }

    /* add effect description */
    if (m->effects->len > 0)
    {
        char **desc_list = strv_new();

        for (guint i = 0; i < m->effects->len; i++)
        {
            effect *e = game_effect_get(nlarn, g_ptr_array_index(m->effects, i));

            if (effect_get_desc(e))
            {
                strv_append_unique(&desc_list, effect_get_desc(e));
            }
        }

        char *effects = g_strjoinv(", ", desc_list);
        g_strfreev(desc_list);

        g_string_append_printf(desc, " (%s)", effects);

        g_free(effects);
    }

    return g_string_free(desc, FALSE);
}

char monster_glyph(monster *m)
{
    g_assert (m != NULL);

    if (m->unknown && inv_length(m->inv) > 0)
    {
        item *it = inv_get(m->inv, 0);
        return item_glyph(it->type);
    }
    else
    {
        return luaN_query_char("monsters", m->type, "glyph");
    }
}

int monster_color(monster *m)
{
    g_assert (m != NULL);

    if (m->unknown && inv_length(m->inv) > 0)
    {
        item *it = inv_get(m->inv, 0);
        return item_colour(it);
    }
    else
    {
        return luaN_query_int("monsters", m->type, "color");
    }
}

void monster_genocide(monster_t monster_id)
{
    GList *mlist;

    g_assert(monster_id > MT_NONE && monster_id < MT_MAX);

    nlarn->monster_genocided[monster_id] = TRUE;
    mlist = g_hash_table_get_values(nlarn->monsters);

    /* purge genocided monsters */
    for (GList *iter = mlist; iter != NULL; iter = iter->next)
    {
        monster *monst = (monster *)iter->data;
        if (monster_is_genocided(monst->type))
        {
            /* add the monster to the game's list of dead monsters */
            g_ptr_array_add(nlarn->dead_monsters, monst);
        }
    }

    /* free the memory returned by g_hash_table_get_values() */
    g_list_free(mlist);

    /* destroy all monsters that have been genocided */
    game_remove_dead_monsters(nlarn);
}

int monster_is_genocided(monster_t monster_id)
{
    g_assert(monster_id > MT_NONE && monster_id < MT_MAX);
    return nlarn->monster_genocided[monster_id];
}

effect *monster_effect_add(monster *m, effect *e)
{
    g_assert(m != NULL && e != NULL);
    gboolean vis_effect = FALSE;

    if (e->type == ET_SLEEP && monster_flags(m, MF_RES_SLEEP))
    {
        /* the monster is resistant to sleep */
        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_POISON && monster_flags(m, MF_RES_POISON))
    {
        /* the monster is poison resistant */
        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_LEVITATION && monster_flags(m, MF_FLY))
    {
        /* levitation has no effect on flying monsters */
        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_CONFUSION && monster_flags(m, MF_RES_CONF))
    {
        /* the monster is resistant against confusion */
        effect_destroy(e);
        e = NULL;
    }

    /* one time effects */
    if (e && e->turns == 1)
    {
        switch (e->type)
        {
        case ET_INC_HP:
            {
                int hp_orig = m->hp;
                m->hp += min ((m->hp_max  * e->amount) / 100, m->hp_max);

                if (m->hp > hp_orig)
                    vis_effect = TRUE;
            }

            break;

        case ET_MAX_HP:
            if (m->hp < m->hp_max)
            {
                m->hp = m->hp_max;
                vis_effect = TRUE;
            }
            break;

        default:
            /* nothing happens.. */
            break;
        }
    }
    else if (e)
    {
        /* multi-turn effects */
        e = effect_add(m->effects, e);

        /* if it's confusion, set the monster's "AI" accordingly */
        if (e && e->type == ET_CONFUSION) {
            monster_update_action(m, MA_CONFUSION);
        }
    }

    /* show message if monster is visible */
    if (e && monster_in_sight(m)
        && effect_get_msg_m_start(e)
        && (e->turns > 0 || vis_effect))
    {
        log_add_entry(nlarn->log, effect_get_msg_m_start(e),
                      monster_name(m));
    }

    /* clean up one-time effects */
    if (e && e->turns == 1)
    {
        effect_destroy(e);
        e = NULL;
    }

    return e;
}

int monster_effect_del(monster *m, effect *e)
{
    int result;

    g_assert(m != NULL && e != NULL);

    /* log info if the player can see the monster */
    if (monster_in_sight(m) && effect_get_msg_m_stop(e))
    {
        log_add_entry(nlarn->log, effect_get_msg_m_stop(e), monster_name(m));
    }

    if ((result = effect_del(m->effects, e)))
    {
        /* if confusion is finished, set the AI back to the default */
        if ((e->type) == ET_CONFUSION) {
            monster_update_action(m, MA_WANDER);
        }

        effect_destroy(e);
    }

    return result;
}

effect *monster_effect_get(monster *m , effect_t type)
{
    g_assert(m != NULL && type < ET_MAX);
    return effect_get(m->effects, type);
}

int monster_effect(monster *m, effect_t type)
{
    g_assert(m != NULL && type < ET_MAX);
    return effect_query(m->effects, type);
}

void monster_effects_expire(monster *m)
{
    guint idx = 0;

    g_assert(m != NULL);

    while (idx < m->effects->len)
    {
        gpointer effect_id = g_ptr_array_index(m->effects, idx);;
        effect *e = game_effect_get(nlarn, effect_id);

        if (e->type == ET_TRAPPED)
        {
            /* if the monster is incapable of movement don't decrease
               trapped counter */
            if (monster_effect(m, ET_HOLD_MONSTER)
                    || monster_effect(m, ET_SLEEP))
            {
                idx++;
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
    /* monster is blinded */
    if (monster_effect(m, ET_BLINDNESS))
        return FALSE;

    /* FIXME: this ought to be different per monster type */
    int monster_visrange = 7;

    if (player_effect(nlarn->p, ET_STEALTH))
    {
        /* if the player is stealthy monsters will only recognize him when
           standing next to him */
        monster_visrange = 1;
    }
    else if (monster_effect(m, ET_TRAPPED))
        monster_visrange = 2;

    /* determine if the monster can see the player */
    if (pos_distance(monster_pos(m), nlarn->p->pos) > monster_visrange)
        return FALSE;

    /* check if the player is invisible and if the monster has infravision */
    if (player_effect(nlarn->p, ET_INVISIBILITY)
        && !(monster_flags(m, MF_INFRAVISION) || monster_effect(m, ET_INFRAVISION)))
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
    item *best = NULL;

    for (guint idx = 0; idx < inv_length(m->inv); idx++)
    {
        item *curr = inv_get(m->inv, idx);

        if (curr->type == IT_WEAPON)
        {
            if (best == NULL)
            {
                best = curr;
            }
            else if (weapon_damage(curr) > weapon_damage(best))
            {
                best = curr;
            }
        }
    }

    return best;
}

static void monster_weapon_wield(monster *m, item *weapon)
{
    /* FIXME: time management */
    /* FIXME: weapon effects */
    m->eq_weapon = weapon;

    /* show message if monster is visible */
    if (monster_in_sight(m))
    {
        gchar *buf = item_describe(weapon, player_item_identified(nlarn->p,
                                weapon), TRUE, FALSE);

        log_add_entry(nlarn->log, "The %s wields %s.", monster_name(m), buf);
        g_free(buf);
    }
}

static gboolean monster_item_disenchant(monster *m, struct player *p)
{
    item *it;

    g_assert (m != NULL && p != NULL);

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
        gchar *desc = item_describe(it, player_item_known(nlarn->p, it),
                                    (it->count == 1), TRUE);

        desc[0] = g_ascii_toupper(desc[0]);
        log_add_entry(nlarn->log, "%s resist%s the attack.",
                      desc, (it->count == 1) ? "s" : "");

        it->blessed_known = TRUE;
        g_free(desc);
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
static gboolean monster_item_rust(monster *m __attribute__((unused)), struct player *p)
{
    item **it;

    g_assert(m != NULL && p != NULL);

    /* get a random piece of armour to damage */
    if ((it = player_get_random_armour(p, FALSE)))
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
    item *it = NULL;

    g_assert (m != NULL && p != NULL);

    /* if player has a device of no theft abort the theft */
    if (player_effect(p, ET_NOTHEFT))
        return FALSE;

    /* Leprechaun robs only gold */
    if (item_type == IT_GOLD)
    {
        /* get amount of gold pieces carried by the player */
        guint player_gold = player_get_gold(p);

        if (player_gold > 0)
        {
            it = item_new(IT_GOLD, rand_1n(1 + (player_gold >> 1)));
            player_remove_gold(p, it->count);

            log_add_entry(nlarn->log, "The %s picks your pocket. " \
                          "Your purse feels lighter.", monster_get_name(m));
        }
        else
        {
            inventory *inv = *map_ilist_at(monster_map(m), p->pos);

            if (inv != NULL)
            {
                for (guint idx = 0; idx < inv_length(inv); idx++)
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
            gboolean was_equipped = FALSE;

            it = inv_get(p->inventory, rand_0n(inv_length(p->inventory)));
            gchar *buf = item_describe(it, player_item_known(p, it), FALSE, FALSE);

            if ((was_equipped = player_item_is_equipped(p, it)))
            {
                if (it->cursed)
                {
                    /* cursed items can't be stolen.. */
                    log_add_entry(nlarn->log, "The %s tries to steal %s but fails.",
                                  monster_get_name(m), buf);

                    it->blessed_known = TRUE;
                    g_free(buf);

                    /* return true as there are things to steal */
                    return TRUE;
                }

                player_item_unequip(p, NULL, it, TRUE);
            }

            if (it->count > 1)
            {
                /* the player has multiple items. Steal only one. */
                it = item_split(it, rand_1n(it->count));
                g_free(buf);
                buf = item_describe(it, player_item_known(p, it), FALSE, FALSE);
            }
            else
            {
                /* this item's count is one. Steal exactly this one. */
                inv_del_element(&p->inventory, it);
            }

            if (was_equipped)
            {
                log_add_entry(nlarn->log, "The %s nimbly removes %s and steals it.",
                              monster_get_name(m), buf);
            }
            else
            {
                log_add_entry(nlarn->log, "The %s picks your pocket and steals %s.",
                              monster_get_name(m), buf);
            }

            g_free(buf);
        }
    }

    /* if item / gold has been stolen, add it to the monster's inventory */
    if (it)
    {
        inv_add(&m->inv, it);
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

    if (!fortunes)
    {
        /* read in the fortunes */
        char buffer[80];
        char *tmp = 0;
        FILE *fortune_fd;

        fortunes = g_ptr_array_new();

        /* open the file */
        fortune_fd = fopen(fortune_file, "r");
        if (fortune_fd == NULL)
        {
            /* can't find file */
            tmp = "Help me! I can't find the fortune file!";
            g_ptr_array_add(fortunes, tmp);
        }
        else
        {
            /* read in the entire fortune file */
            while((fgets(buffer, 79, fortune_fd)))
            {
                /* replace EOL with \0 */
                size_t len = (size_t)(strchr(buffer, '\n') - (char *)&buffer);
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

static position monster_move_wander(monster *m, struct player *p __attribute__((unused)))
{
    int tries = 0;
    position npos;

    do
    {
        npos = pos_move(m->pos, rand_1n(GD_MAX));
        tries++;
    }
    while (tries < GD_MAX
            && !map_pos_validate(monster_map(m), npos, monster_map_element(m),
                                 FALSE));

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
        /* We need to store the monster's name here, otherwise it would
           always be 'unseen monster' for monsters that teleport away. */
        const char *mname = monster_get_name(m);

        monster_player_attack(m, p);

        /* monster's position might have changed (teleport) */
        if (!pos_identical(npos, monster_pos(m)))
        {
            log_add_entry(nlarn->log, "The %s vanishes.", mname);
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
            newmap = Z(m->pos) + 1;
            break;

        case LS_STAIRSUP:
        case LS_DNGN_EXIT:
            newmap = Z(m->pos) - 1;
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
            newmap = Z(m->pos);
            break;
        }

        /* change the map */
        monster_level_enter(m, game_map(nlarn, newmap));

        return monster_pos(m);
    }

    /* monster heads into the direction of the player. */
    path = map_find_path(monster_map(m), monster_pos(m), m->player_pos,
                         monster_map_element(m));

    if (path && !g_queue_is_empty(path->path))
    {
        el = g_queue_pop_head(path->path);
        npos = el->pos;
    }
    else
    {
        /* No path found. Stop following player */
        m->lastseen = 0;
    }

    /* clean up */
    if (path)
        map_path_destroy(path);

    return npos;

}

static position monster_move_confused(monster *m,
                                      __attribute__ ((unused)) struct player *p)
{
    /* as the monster is confused, choose a random movement direction */
    return pos_move(monster_pos(m), rand_1n(GD_MAX));
}

static position monster_move_flee(monster *m, struct player *p)
{
    int dist = 0;
    position npos_tmp;
    position npos = monster_pos(m);

    for (int tries = 1; tries < GD_MAX; tries++)
    {
        /* try all fields surrounding the monster if the
         * distance between monster & player is greater */
        if (tries == GD_CURR)
            continue;

        npos_tmp = pos_move(monster_pos(m), tries);

        if (map_pos_validate(monster_map(m), npos_tmp, monster_map_element(m),
                             FALSE)
                && pos_distance(p->pos, npos_tmp) > dist)
        {
            /* distance is bigger than current distance */
            npos = npos_tmp;
            dist = pos_distance(m->player_pos, npos_tmp);
        }
    }

    return npos;
}

static position monster_move_serve(monster *m, struct player *p)
{
    position npos = pos_invalid;

    /* generate a fov structure if not yet available */
    if (!m->fv)
        m->fv = fov_new();

    /* calculate the monster's fov */
    /* the monster gets a fov radius of 6 for now*/
    fov_calculate(m->fv, monster_map(m), m->pos, 6,
                  monster_flags(m, MF_INFRAVISION)
                  || monster_effect(m, ET_INFRAVISION));

    /* a good servant always knows the masters position */
    if (pos_distance(monster_pos(m), p->pos) > 5)
    {
        /* if the distance to the player is too large, follow */
        map_path *path = map_find_path(monster_map(m), monster_pos(m), p->pos,
                                       monster_map_element(m));

        if (path && !g_queue_is_empty(path->path))
        {
            map_path_element *pe = g_queue_pop_head(path->path);
            npos = pe->pos;
        }

        if (path != NULL)
            map_path_destroy(path);
    }
    else
    {
        /* look for worthy foes */
        /* TODO: implement */
        npos = m->pos;
    }

    return npos;
}

static position monster_move_civilian(monster *m, struct player *p)
{
    position npos = m->pos;

    /* civilians will pick a random location on the map, travel and remain
       there for the number of turns that is determined by their town
       person number. */

    if (!pos_valid(m->player_pos))
    {
        /* No target set -> find a new location to travel to.
           Civilians stay inside the town area. */
        rectangle town = rect_new(3, 4, MAP_MAX_X, MAP_MAX_Y);
        do
        {
            /* Ensure that the townsfolk do not loiter in locations
               important for the player. */
            m->player_pos = map_find_space_in(monster_map(m), town,
                    LE_GROUND, FALSE);
        } while (map_sobject_at(monster_map(m), m->player_pos) != LS_NONE);
    }

    if (pos_identical(m->pos, m->player_pos)
        && m->lastseen >= (m->number + 25))
    {
        /* The person has stayed at the target position for long
           enough, thus reset the target position. */
        m->player_pos = pos_invalid;
    }
    else if (pos_valid(m->player_pos) && !pos_identical(m->pos, m->player_pos))
    {
        /* travel to the selected location */
        map_path *path = map_find_path(monster_map(m), m->pos, m->player_pos, LE_GROUND);

        if (path && !g_queue_is_empty(path->path))
        {
            map_path_element *pe = g_queue_pop_head(path->path);
            npos = pe->pos;

            if (pos_identical(npos, m->player_pos))
            {
                /* the new position is identical to the target, thus reset
                   the lastseen counter */
                m->lastseen = 1;
            }
        }

        if (path != NULL)
        {
            /* clean up */
            map_path_destroy(path);
        }
        else
        {
            /* it seems that there is no path to the target, thus get a new one */
            m->player_pos = pos_invalid;
        }
    }

    /* check if the player is next to the civilian and not inside a building */
    if (pos_adjacent(m->pos, p->pos) && chance(40)
        && so_is_transparent(map_sobject_at(monster_map(m), p->pos)))
    {
        /* talk */
        log_add_entry(nlarn->log, "The %s says, \"%s\"",
                      monster_get_name(m),
                      monsters_get_fortune(game_fortunes(nlarn)));
    }

    /* change the town person's name from time to time */
    if (!fov_get(p->fv, m->pos) && chance(10))
    {
        m->number = rand_1n(40);
    }

    return npos;
}

int monster_is_carrying_item(monster *m, item_t type)
{
    inventory *inv = m->inv;

    for (guint idx = 0; idx < inv_length(inv); idx++)
    {
        item *it = inv_get(inv, idx);
        if (it->type == type)
            return TRUE;
    }
    return FALSE;
}

static gboolean monster_breath_hit(const GList *traj,
                                   const damage_originator *damo __attribute__((unused)),
                                   gpointer data1,
                                   gpointer data2 __attribute__((unused)))
{
    monster *m;
    damage *dam = (damage *)data1;
    item_erosion_type iet;
    gboolean terminated = FALSE;
    position pos; pos_val(pos) = GPOINTER_TO_UINT(traj->data);
    map *mp = game_map(nlarn, Z(pos));

    /* determine if items should be eroded */
    switch (dam->type)
    {
    case DAM_FIRE:
        iet = IET_BURN;
        break;

    case DAM_ACID:
        iet = IET_CORRODE;
        break;

    case DAM_WATER:
        iet = IET_RUST;
        break;

    default:
        iet = IET_NONE;
        break;
    }

    if ((m = map_get_monster_at(mp, pos)))
    {
        /* The breath hit a monster. */
        if (monster_in_sight(m))
            log_add_entry(nlarn->log, "The %s hits the %s.",
                          monster_breath_data[dam->type].desc,
                          monster_get_name(m));

        /* erode the monster's inventory */
        if (iet && monster_inv(m))
            inv_erode(monster_inv(m), iet, FALSE, NULL);

        monster_damage_take(m, damage_copy(dam));

        /* the breath will sweep over small monsters */
        if (monster_size(m) >= ESIZE_LARGE)
            terminated = TRUE;
    }

    if (pos_identical(pos, nlarn->p->pos))
    {
        /* The breath hit the player */
        if (player_effect(nlarn->p, ET_REFLECTION))
        {
            /* The player reflects the breath. Actual handling of the reflection
               is done in map_trajectory, just give a message here. */
            log_add_entry(nlarn->log, "Your amulet reflects the %s!",
                          monster_breath_data[dam->type].desc);
        }
        else
        {
            /* No reflection -> player takes the damage */
            /* TODO: evasion!!! */
            log_add_entry(nlarn->log, "The %s hits you!",
                          monster_breath_data[dam->type].desc);
            player_damage_take(nlarn->p, damage_copy(dam), PD_MONSTER,
                               monster_type(dam->dam_origin.originator));

            /* erode the player's inventory */
            if (iet != IET_NONE)
            {
                /*
                 * Filter equipped and exposed items, e.g.
                 * a body armour will not be affected by erosion
                 * when the player wears a cloak over it.
                 */
                inv_erode(&nlarn->p->inventory, iet, TRUE,
                        player_item_filter_unequippable);
            }

            terminated = TRUE;
        }
    }

    if (iet > IET_NONE && map_ilist_at(mp, pos))
    {
        /* erode affected items */
        inv_erode(map_ilist_at(mp, pos), iet, fov_get(nlarn->p->fv, pos), NULL);
    }


    if (map_sobject_at(mp, pos) == LS_MIRROR && fov_get(nlarn->p->fv, pos))
    {
        /* A mirror will reflect the breath. Actual handling of the reflection
           is done in map_trajectory, just give a message here if the
           mirror is visible by the player. */
        log_add_entry(nlarn->log, "The mirror reflects the %s!",
                      monster_breath_data[dam->type].desc);
    }

    return terminated;
}
