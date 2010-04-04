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
#include <stdlib.h>

#include "display.h"
#include "game.h"
#include "map.h"
#include "monsters.h"
#include "nlarn.h"

#define EMPTY_ATTACK { ATT_NONE, DAM_NONE, 0, 0 }

/* monster information hiding */
struct _monster
{
    monster_t type;
    gpointer oid; /* monsters id inside the monster hash */
    gint32 hp;
    position pos;
    int movement;
    monster_action_t action;    /* current action */

    /* number of turns since when player was last seen; 0 = never */
    guint32 lastseen;

    /* last known position of player */
    position player_pos;

    /* attacks already unsuccessfully tried */
    gboolean attacks_failed[MONSTER_ATTACK_COUNT];

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
    "stung",        /* ATT_STING */
    "slams",        /* ATT_SLAM */
    "kicks",        /* ATT_KICK */
    "touches",      /* ATT_TOUCH */
    "breathes at",  /* ATT_BREATH */
    "gazes at",     /* ATT_GAZE */
};

const monster_data monsters[MT_MAX] =
{
    /* ID NAME
     * level ac intelligence gold hitpoints experience image */
    {
        MT_NONE, "",
        0, 0, 0, 0, 0, 0, ' ', DC_BLACK,
        SPEED_NONE, ESIZE_NONE,
        MF_NONE,
        {
            EMPTY_ATTACK,
            EMPTY_ATTACK,
        }

    },
    {
        MT_GIANT_BAT, "giant bat",
        1, 0, 3, 0, 1, 1, 'b', DC_RED,
        SPEED_XFAST, ESIZE_SMALL,
        MF_HEAD | MF_FLY | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_GNOME, "gnome",
        1, 10, 8, 30, 2, 2, 'g', DC_BROWN,
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_HOBGOBLIN, "hobgoblin",
        1, 14, 5, 25, 3, 2, 'H', DC_BROWN,
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_JACKAL, "jackal",
        1, 17, 4, 0, 1, 1, 'J', DC_BROWN,
        SPEED_FAST, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_KOBOLD, "kobold",
        1, 20, 7, 10, 1, 1, 'K', DC_BROWN,
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ORC, "orc",
        2, 12, 9, 40, 4, 2, 'O', DC_RED,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_SNAKE, "snake",
        2, 15, 3, 0, 3, 1, 'S', DC_LIGHTGREEN,
        SPEED_NORMAL, ESIZE_TINY,
        MF_HEAD,
        {
            { ATT_BITE, DAM_POISON, 2, 0 },
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
        }
    },
    {
        MT_CENTIPEDE, "giant centipede",
        2, 14, 2, 0, 1, 2, 'c', DC_YELLOW,
        SPEED_NORMAL, ESIZE_TINY,
        MF_HEAD,
        {
            { ATT_BITE, DAM_DEC_STR, 50, 0 },
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
        }
    },
    {
        /* a winged, leaping snake */
        MT_JACULUS, "jaculus",
        2, 20, 3, 0, 2, 1, 'j', DC_GREEN,
        SPEED_XFAST, ESIZE_MEDIUM,
        MF_HEAD | MF_FLY,
        {
            { ATT_BITE, DAM_PHYSICAL, 2, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_TROGLODYTE, "troglodyte",
        2, 10, 5, 80, 4, 3, 't', DC_BROWN,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_GIANT_ANT, "giant ant",
        2, 8, 3, 0, 5, 5, 'A', DC_BROWN,
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BITE, DAM_DEC_STR, 75, 0 },
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
        }
    },
    {
        MT_FLOATING_EYE, "floating eye",
        3, 8, 3, 0, 5, 2, 'E', DC_BLUE,
        SPEED_XSLOW, ESIZE_MEDIUM,
        MF_FLY,
        {
            { ATT_GAZE, DAM_PARALYSIS, 66, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_LEPRECHAUN, "leprechaun",
        3, 3, 6, 1500, 13, 45, 'L', DC_GREEN,
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_TOUCH, DAM_STEAL_GOLD, 0, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_NYMPH, "nymph",
        3, 3, 9, 0, 18, 45, 'n', DC_RED,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS,
        {
            { ATT_TOUCH, DAM_STEAL_ITEM, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_QUASIT, "quasit",
        3, 5, 3, 0, 10, 15, 'Q', DC_BLUE,
        SPEED_FAST, ESIZE_SMALL,
        MF_HEAD | MF_HANDS | MF_DEMON,
        {
            { ATT_CLAW, DAM_DEC_DEX, 66, 0 },
            { ATT_BITE, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_RUST_MONSTER, "rust monster",
        3, 4, 3, 0, 18, 25, 'R', DC_BROWN,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_METALLIVORE,
        {
            { ATT_TOUCH, DAM_RUST, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_ZOMBIE, "zombie",
        3, 12, 3, 0, 6, 7, 'Z', DC_LIGHTGRAY,
        SPEED_VSLOW, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_UNDEAD,
        {
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
            { ATT_BITE, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_ASSASSIN_BUG, "assassin bug",
        4, 9, 3, 0, 20, 15, 'a', DC_LIGHTGRAY,
        SPEED_FAST, ESIZE_TINY,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_BUGBEAR, "bugbear",
        4, 5, 5, 40, 20, 35, 'B', DC_BROWN,
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 5, 10 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_HELLHOUND, "hell hound",
        4, 5, 6, 0, 16, 35, 'h', DC_LIGHTRED,
        SPEED_FAST, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BREATH, DAM_FIRE, 8, 15 },
            { ATT_BITE, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_ICE_LIZARD, "ice lizard",
        4, 11, 6, 0, 16, 25, 'i', DC_LIGHTCYAN,
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD,
        {
            { ATT_SLAM, DAM_PHYSICAL, 14, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_CENTAUR, "centaur",
        4, 6, 10, 40, 24, 45, 'C', DC_BROWN,
        SPEED_FAST, ESIZE_LARGE,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            { ATT_KICK, DAM_PHYSICAL, 6, 0 },
        }
    },
    {
        MT_TROLL, "troll",
        5, 4, 9, 80, 50, 300, 'T', DC_BROWN,
        SPEED_SLOW, ESIZE_LARGE,
        MF_HEAD | MF_HANDS | MF_REGENERATE,
        {
            { ATT_CLAW, DAM_PHYSICAL, 5, 0 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_YETI, "yeti",
        5, 6, 5, 50, 35, 100, 'Y', DC_LIGHTGRAY,
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_HANDS,
        {
            { ATT_CLAW, DAM_PHYSICAL, 4, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ELF, "elf",
        5, 8, 15, 50, 22, 35, 'e', DC_WHITE,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_GELATINOUSCUBE, "gelatinous cube",
        5, 9, 3, 0, 22, 45, 'g', DC_CYAN,
        SPEED_XSLOW, ESIZE_LARGE,
        MF_METALLIVORE,
        {
            { ATT_SLAM, DAM_ACID, 1, 0 },
        }
    },
    {
        MT_WHITE_DRAGON, "white dragon",
        5, 2, 16,  500, 55, 1000, 'd', DC_WHITE,
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_DRAGON,
        {
            { ATT_BITE, DAM_PHYSICAL, 4, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 4, 0 },
        }
    },
    {
        MT_METAMORPH, "metamorph",
        6, 7, 3, 0, 30, 40, 'm', DC_WHITE,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_NONE,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_VORTEX, "vortex",
        6, 4, 3, 0, 30, 55, 'v', DC_DARKGRAY,
        SPEED_VFAST, ESIZE_TINY,
        MF_NONE,
        {
            { ATT_SLAM, DAM_PHYSICAL, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ZILLER, "ziller",
        6, 15, 3, 0, 30, 35, 'z', DC_CYAN,
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD,
        {
            { ATT_CLAW, DAM_PHYSICAL, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    { /* FIXME: I don't want no silly mushrooms */
        MT_VIOLET_FUNGUS, "violet fungi",
        6, 12, 3, 0, 38, 100, 'F', DC_MAGENTA,
        SPEED_XSLOW, ESIZE_MEDIUM,
        MF_NONE,
        {
            { ATT_SLAM, DAM_POISON, 2, 0 },
            { ATT_SLAM, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_WRAITH, "wraith",
        6, 3, 3, 0, 30, 325, 'W', DC_LIGHTGRAY,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_UNDEAD,
        {
            { ATT_TOUCH, DAM_DRAIN_LIFE, 50, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_FORVALAKA, "forvalaka",
        6, 2, 7, 0, 50, 280, 'f', DC_DARKGRAY,
        SPEED_DOUBLE, ESIZE_MEDIUM,
        MF_HEAD | MF_UNDEAD | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 5, 0 },
            EMPTY_ATTACK,
        }
    },
    { /* TODO: get rid of this beast */
        MT_LAMA_NOBE, "lama nobe",
        7, 7, 6, 0, 35, 80, 'l', DC_BROWN,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD,
        {
            { ATT_BITE, DAM_NONE, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_OSQUIP, "osquip",
        7, 4, 4, 0, 35, 100, 'o', DC_BROWN,
        SPEED_VFAST, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 10, 15 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ROTHE, "rothe",
        7, 15, 3, 0, 50, 250, 'r', DC_BROWN,
        SPEED_VFAST, ESIZE_LARGE,
        MF_HEAD | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 5, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_XORN, "xorn",
        7, 0, 13, 0, 60, 300, 'X', DC_DARKGRAY,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 6, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_VAMPIRE, "vampire",
        7, 3, 17, 0, 50, 1000, 'V', DC_RED,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_FLY | MF_UNDEAD | MF_INFRAVISION | MF_REGENERATE,
        {
            { ATT_BITE, DAM_DRAIN_LIFE, 75, 0 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_STALKER, "invisible stalker",
        7, 3, 14, 0, 50, 350, 'I', DC_LIGHTGRAY,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_FLY | MF_INVISIBLE,
        {
            { ATT_SLAM, DAM_PHYSICAL, 6, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_POLTERGEIST, "poltergeist",
        8, 1, 5, 0, 50, 450, 'p', DC_WHITE,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_UNDEAD | MF_FLY | MF_INVISIBLE,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_DISENCHANTRESS, "disenchantress",
        8, 3, 5, 0, 50, 500, 'q', DC_WHITE,
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_METALLIVORE,
        {
            { ATT_TOUCH, DAM_REM_ENCH, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_SHAMBLINGMOUND, "shambling mound",
        8, 2, 6, 0, 45, 400, 's', DC_GREEN,
        SPEED_VSLOW, ESIZE_HUGE,
        MF_NONE,
        {
            { ATT_SLAM, DAM_PHYSICAL, 5, 0 },
            EMPTY_ATTACK,
        }
    },
    { /* FIXME: replace this beast! */
        MT_YELLOW_MOLD, "yellow mold",
        8, 12, 3, 0, 35, 250, 'y', DC_YELLOW,
        SPEED_XSLOW, ESIZE_SMALL,
        MF_NONE,
        {
            { ATT_TOUCH, DAM_PHYSICAL, 4, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_UMBER_HULK, "umber hulk",
        8, 3, 14, 0, 65, 600, 'U', DC_YELLOW,
        SPEED_SLOW, ESIZE_HUGE,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_GAZE, DAM_CONFUSION, 75, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 7, 0 },
        }
    },
    {
        MT_GNOME_KING, "gnome king",
        9, -1, 18, 2000, 100,   3000, 'G', DC_RED,
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_MIMIC, "mimic",
        9, 5, 8, 0, 55, 99, 'M', DC_BROWN,
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_MIMIC,
        {
            { ATT_SLAM, DAM_PHYSICAL, 6, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_WATER_LORD, "water lord",
        9,-10, 20, 0, 150, 15000, 'w', DC_BLUE,
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_PURPLE_WORM, "purple worm",
        9, -1, 3, 0, 120, 15000, 'P', DC_MAGENTA,
        SPEED_VSLOW, ESIZE_GARGANTUAN,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 11, 0 },
            { ATT_STING, DAM_POISON, 6, 0 },
        }
    },
    {
        MT_XVART, "xvart",
        9, -2, 13, 0, 90, 1000, 'x', DC_DARKGRAY,
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_BRONCE_DRAGON, "bronze dragon",
        9, 2, 16, 300, 80, 4000, 'D', DC_BROWN,
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY | MF_DRAGON,
        {
            { ATT_BITE, DAM_PHYSICAL, 9, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 9, 0 },
        }
    },
    {
        MT_GREEN_DRAGON, "green dragon",
        9, 3, 15, 200, 70, 2500, 'D', DC_GREEN,
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY | MF_DRAGON,
        {
            { ATT_SLAM, DAM_PHYSICAL, 25, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 8, 0 },
        }
    },
    { /* TODO: replace this beast */
        MT_SILVER_DRAGON, "silver dragon",
        10, -1, 20, 700, 100, 10000, 'D', DC_LIGHTGRAY,
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY | MF_DRAGON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 12, 0 },
            { ATT_BITE, DAM_PHYSICAL, 12, 0 },
        }
    },
    {
        MT_PLATINUM_DRAGON, "platinum dragon",
        10, -5, 22, 1000, 130, 24000, 'D', DC_WHITE,
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY | MF_DRAGON,
        {
            { ATT_MAGIC, DAM_MAGICAL, 15, 30 },
            { ATT_BITE, DAM_PHYSICAL, 15, 0 },
        }
    },
    {
        MT_RED_DRAGON, "red dragon",
        10, -2, 19, 800, 110, 14000, 'D', DC_LIGHTRED,
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY | MF_DRAGON,
        {
            { ATT_BREATH, DAM_FIRE, 20, 25 },
            { ATT_CLAW, DAM_PHYSICAL, 13, 0 },
        }
    },
    {
        MT_SPIRIT_NAGA, "spirit naga",
        10, -20, 23, 0, 95, 20000, 'N', DC_MAGENTA,
        SPEED_FAST, ESIZE_LARGE,
        MF_HEAD | MF_NOBEHEAD | MF_FLY | MF_SPIRIT | MF_INFRAVISION,
        {
            { ATT_MAGIC, DAM_RANDOM, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 12, 0 },
        }
    },
    { /* TODO: relplace this beast */
        MT_GREEN_URCHIN, "green urchin",
        10, -3, 3, 0, 85, 5000, 'u', DC_GREEN,
        SPEED_SLOW, ESIZE_SMALL,
        MF_NONE,
        {
            { ATT_STING, DAM_PHYSICAL, 12, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_DEMONLORD_I, "type I demon lord",
        12, -30, 20, 0, 140, 50000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 18, 0 },
            { ATT_BITE, DAM_PHYSICAL, 18, 0 },
        }
    },
    {
        MT_DEMONLORD_II, "type II demon lord",
        13, -30, 21, 0, 160, 75000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 18, 0 },
            { ATT_BITE, DAM_PHYSICAL, 18, 0 },
        }
    },
    {
        MT_DEMONLORD_III, "type III demon lord",
        14, -30,  22, 0, 180, 100000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 18, 0 },
            { ATT_BITE, DAM_PHYSICAL, 18, 0 },
        }
    },
    {
        MT_DEMONLORD_IV, "type IV demon lord",
        15, -35, 23, 0, 200, 125000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 20, 0 },
            { ATT_BITE, DAM_PHYSICAL, 20, 0 },
        }
    },
    {
        MT_DEMONLORD_V, "type V demon lord",
        16, -40, 24, 0, 220, 150000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 22, 0 },
            { ATT_BITE, DAM_PHYSICAL, 22, 0 },
        }
    },
    {
        MT_DEMONLORD_VI, "type VI demon lord",
        17, -45, 25, 0, 240, 175000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_LARGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 24, 0 },
            { ATT_BITE, DAM_PHYSICAL, 24, 0 },
        }
    },
    {
        MT_DEMONLORD_VII, "type VII demon lord",
        18, -70, 26, 0, 260, 200000, '&', DC_LIGHTRED,
        SPEED_FAST, ESIZE_HUGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 27, 0 },
            { ATT_BITE, DAM_PHYSICAL, 27, 0 },
        }
    },
    {
        MT_DAEMON_PRINCE, "demon prince",
        25, -127, 28, 0, 345, 300000, '&', DC_RED,
        SPEED_FAST, ESIZE_HUGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION | MF_DEMON,
        {
            { ATT_CLAW, DAM_PHYSICAL, 30, 0 },
            { ATT_BITE, DAM_PHYSICAL, 30, 0 },
        }
    }
};

static gboolean monster_player_visible(monster *m);
static gboolean monster_attack_available(monster *m, attack_t type);
static const attack *monster_attack_get(monster *m, attack_t type);
static void monster_attack_disable(monster *m, const attack *att);
static item *monster_weapon_select(monster *m);
static void monster_weapon_wield(monster *m, item *weapon);
static gboolean monster_item_disenchant(monster *m, struct player *p);
static gboolean monster_item_rust(monster *m, struct player *p);
static gboolean monster_player_rob(monster *m, struct player *p, item_t item_type);

monster *monster_new(int type, position pos)
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
    nmonster->hp = divert(monsters[type].hp_max, 10);

    /* prevent the living dead */
    if (nmonster->hp < 1)
        nmonster->hp = 1;

    nmonster->effects = g_ptr_array_new();
    nmonster->inventory = inv_new(nmonster);

    /* fill monsters inventory */
    if (monster_gold(nmonster) > 0)
    {
        /* add gold to monster's inventory, randomize the amount */
        icount = max(divert(monster_gold(nmonster), 10), 1);
        inv_add(&nmonster->inventory, item_new(IT_GOLD, icount));
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
    if (monster_is_mimic(nmonster))
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
    const int monster_level[] = { 5, 11, 17, 22, 27, 33, 39, 42, 46, 50, 53, 56, MT_MAX - 1 };
    int monster_id = 0;
    int monster_id_min;
    int monster_id_max;
    int nlevel = pos.z;

    assert(pos_valid(pos));

    if (nlevel < 5)
    {
        monster_id_min = 1;
        monster_id_max = monster_level[nlevel - 1];
    }
    else
    {
        monster_id_min = monster_level[nlevel - 4] + 1;
        monster_id_max = monster_level[nlevel - 1];
    }

    while (nlarn->monster_genocided[monster_id]
            || (monster_id <= MT_NONE)
            || (monster_id >= MT_MAX))
    {
        monster_id = rand_m_n(monster_id_min, monster_id_max);
    }

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

void monster_serialize(gpointer oid, monster *m, cJSON *root)
{
    cJSON *mval;

    cJSON_AddItemToArray(root, mval = cJSON_CreateObject());
    cJSON_AddNumberToObject(mval, "type", monster_type(m));
    cJSON_AddNumberToObject(mval, "oid", GPOINTER_TO_UINT(oid));
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

int monster_hp(monster *m)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);
    return m->hp;
}

void monster_hp_inc(monster *m, int amount)
{
    assert(m != NULL && m->type > MT_NONE && m->type < MT_MAX);

    m->hp += amount;
    if (m->hp > monsters[m->type].hp_max)
        m->hp = monsters[m->type].hp_max;
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

gboolean monster_in_sight(monster *m)
{
    assert (m != NULL);

    /* different level */
    if (m->pos.z != nlarn->p->pos.z)
        return FALSE;

    /* invisible monster, player has no infravision */
    if (monster_is_invisible(m) && !player_effect(nlarn->p, ET_INFRAVISION))
        return FALSE;

    return player_pos_visible(nlarn->p, m->pos);
}

// Takes into account visibility.
// For the real name, use monster_name() directly.
char *monster_get_name(monster *m)
{
    if (!game_wizardmode(nlarn) && !monster_in_sight(m))
        return ("unseen monster");

    return (monster_name(m));
}

const char* monster_type_plural_name(const int montype, const int count)
{
    /* need a static buffer to return to calling functions */
    static char buf[61] = { 0 };

    if (count > 1)
    {
        switch (montype)
        {
        case MT_JACULUS:
            return "jaculi";
            break;

        case MT_ELF:
            return "elves";
            break;

        case MT_DISENCHANTRESS:
            return "disenchantresses";
            break;

        default:
            g_snprintf(buf, 60, "%ss", monster_type_name(montype));
            return buf;
            break;
        }
    }

    return monster_type_name(montype);
}

void monster_die(monster *m, struct player *p)
{
    char *message = "The %s died!";

    assert(m != NULL);

    /* if the player can see the monster describe the event */
    if (monster_in_sight(m))
    {
        log_add_entry(nlarn->p->log, message, monster_name(m));
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
    char *how;

    /* check if the monster used the stairs */
    switch (source)
    {
    case LS_DNGN_EXIT:
        target = LS_DNGN_ENTRANCE;
        how = "through";
        break;

    case LS_DNGN_ENTRANCE:
        target = LS_DNGN_EXIT;
        how = "through";
        break;

    case LS_STAIRSDOWN:
        target = LS_STAIRSUP;
        how = "down";
        break;

    case LS_STAIRSUP:
        target = LS_STAIRSDOWN;
        how = "up";
        break;

    default:
        target = LS_NONE;
    }

    /* determine new position */
    if (target)
    {
        npos = map_find_sobject(l, target);
    }
    else
    {
        npos = map_find_space(l, LE_MONSTER, FALSE);
    }

    /* validate new position */
    if (!map_pos_validate(l, npos, LE_MONSTER, FALSE))
    {
        /* the position somehow isn't valid,
           e.g. someone is standing on the entrance */
        return;
    }

    /* remove monster from old map  */
    map *oldmap = game_map(nlarn, m->pos.z);
    map_set_monster_at(oldmap, m->pos, NULL);

    /* put monster into map */
    monster_pos_set(m, l, npos);

    /* log the event */
    if (monster_in_sight(m) && target)
    {
        log_add_entry(nlarn->p->log, "The %s comes %s %s.", monster_name(m),
                      how, ls_get_desc(target));
    }
}

void monster_move(monster *m, struct player *p)
{
    /* monster's new position / temporary position */
    position m_npos, m_npos_tmp;

    /* number of tries to find m_npos */
    int tries;

    /* distance between player and m_npos_tmp */
    int dist;

    /* path to player */
    map_path *path = NULL;

    map_path_element *el = NULL;

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
                log_add_entry(p->log,
                              "The %s has spotted you and heads towards you!",
                              monster_name(m));
                 */
            }
            else if (m->action == MA_FLEE)
            {
                log_add_entry(p->log, "The %s turns to flee!", monster_name(m));
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
            dist = 0;

            for (tries = 1; tries < GD_MAX; tries++)
            {
                /* try all fields surrounding the monster if the
                 * distance between monster & player is greater */
                if (tries == GD_CURR)
                    continue;

                m_npos_tmp = pos_move(monster_pos(m), tries);

                if (pos_valid(m_npos_tmp)
                        && lt_is_passable(map_tiletype_at(monster_map(m),m_npos_tmp))
                        && !map_is_monster_at(monster_map(m), m_npos_tmp)
                        && (pos_distance(p->pos, m_npos_tmp) > dist))
                {
                    /* distance is bigger than current distance */
                    m_npos = m_npos_tmp;
                    dist = pos_distance(m->player_pos, m_npos_tmp);
                }
            }

            break; /* end MA_FLEE */

        case MA_REMAIN:
            /* Sgt. Stan Still - do nothing */
            break;

        case MA_WANDER:
            tries = 0;

            do
            {
                m_npos = pos_move(m->pos, rand_1n(GD_MAX));
                tries++;
            }
            while ((!pos_valid(m_npos)
                    || !lt_is_passable(map_tiletype_at(monster_map(m),m_npos))
                    || map_is_monster_at(monster_map(m), m_npos))
                    && (tries < GD_MAX));

            /* new position has not been found, reset to current position */
            if (tries == GD_MAX)
                m_npos = monster_pos(m);

            break; /* end MA_WANDER */

        case MA_ATTACK:
            if (pos_adjacent(monster_pos(m), m->player_pos) && (m->lastseen == 1))
            {
                /* monster is standing next to player */
                monster_player_attack(m, p);

                /* monster's position might have changed (teleport) */
                if (!pos_identical(m_npos, monster_pos(m)))
                {
                    m_npos = monster_pos(m);
                    log_add_entry(p->log, "The %s vanishes.", monster_name(m));
                }
            }
            else if (pos_identical(monster_pos(m), m->player_pos)
                     && ((map_sobject_at(monster_map(m), m->pos) == LS_STAIRSDOWN)
                         || (map_sobject_at(monster_map(m), m->pos) == LS_DNGN_ENTRANCE
                             && monster_pos(m).z == 0)))
            {
                /* go level down */
                monster_level_enter(m, game_map(nlarn, m->pos.z + 1));
            }
            else if (pos_identical(monster_pos(m), m->player_pos)
                     && (map_sobject_at(monster_map(m), m->pos) == LS_STAIRSUP
                         || (map_sobject_at(monster_map(m), m->pos) == LS_DNGN_EXIT
                             && monster_pos(m).z == 1)))
            {
                /* go level up */
                monster_level_enter(m, game_map(nlarn, m->pos.z - 1));
            }
            else
            {
                /* monster heads into the direction of the player. */

                /* if the monster is on a different map than the player,
                   try to find the staircase to reach the player's map */
                if (m->pos.z != m->player_pos.z)
                {
                    map_sobject_t what;
                    if (m->pos.z > m->player_pos.z)
                        what = LS_STAIRSUP;
                    else
                        what = LS_STAIRSDOWN;

                    m->player_pos = map_find_sobject(monster_map(m), what);
                }

                path = map_find_path(monster_map(m), monster_pos(m), m->player_pos);

                if (path && !g_queue_is_empty(path->path))
                {
                    el = g_queue_pop_head(path->path);
                    m_npos = el->pos;
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
            }
            break; /* end MA_ATTACK */

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

                log_add_entry(p->log, "The %s bumped into you.", monster_get_name(m));
            }

            /* check for door */
            else if ((target_st == LS_CLOSEDDOOR)
                     /* lock out zombies */
                     && monster_has_hands(m) && monster_int(m) > 3)
            {
                /* open the door */
                map_sobject_set(monster_map(m), m_npos, LS_OPENDOOR);

                /* notify the player if the door is visible */
                if (monster_in_sight(m))
                {
                    log_add_entry(p->log, "The %s opens the door.", monster_name(m));
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
    if (monster_can_fly(m) && (trap != TT_SLEEPGAS))
    {
        return m;
    }

    /* return if the monster has not triggered the trap */
    if (!chance(trap_chance(trap)))
    {
        return m;
    }

    opos = monster_pos(m);

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

    default:
        /* if there is an effect on the trap add it to the
         * monster's list of effects. */
        if (trap_effect(trap))
        {
            eff = effect_new(trap_effect(trap));
            eff = monster_effect_add(m, eff);
        }

    } /* switch (trap) */

    if (monster_in_sight(m))
    {
        log_add_entry(nlarn->p->log, trap_m_message(trap), monster_name(m));

        /* set player's knowlege of trap */
        player_memory_of(nlarn->p, opos).trap = trap;
    }

    /* inflict damage caused ba the trap */
    if (trap_damage(trap))
    {
        damage *dam = damage_new(DAM_PHYSICAL, ATT_NONE, rand_1n(trap_damage(trap)), NULL);
        m = monster_damage_take(m, dam);
    }

    return m;
}

void monster_polymorph(monster *m)
{
    assert (m != NULL);

    do
    {
        m->type = rand_1n(MT_MAX);
    }
    while (monster_is_genocided(m->type));

    m->hp = monster_hp_max(m);
}

/**
 * check stash at monster's position for something desired
 * @param a monster
 * @return TRUE if something has been picked up, FALSE if not
 *
 */
int monster_items_pickup(monster *m)
{
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
                log_add_entry(nlarn->p->log, "The %s picks up %s.", monster_name(m), buf);
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

/**
 * Returns the number of attack type a monster can choose from
 *
 * @param a monster
 * @return the number of attacks
 *
 */
int monster_attack_count(monster *m)
{
    assert (m != NULL && m->type < MT_MAX);

    /* currently very simple */
    if (monsters[m->type].attacks[1].type)
        return 2;
    else
        return 1;
}

void monster_player_attack(monster *m, player *p)
{
    damage *dam;
    const attack *att = NULL;
    int idx;

    assert(m != NULL && p != NULL);

    /* the player is invisible and the monster bashes into thin air */
    if (!pos_identical(m->player_pos, p->pos))
    {
        if (!map_is_monster_at(game_map(nlarn, p->pos.z), p->pos)
                && monster_in_sight(m))
        {
            log_add_entry(p->log, "The %s bashes into thin air.",
                          monster_name(m));
        }

        m->lastseen++;

        return;
    }

    /* player is invisible and monster tries to hit player */
    if (player_effect(p, ET_INVISIBILITY) && !monster_has_infravision(m)
            && chance(65))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(p->log, "The %s misses wildly.", monster_get_name(m));
        }
        return;
    }

    if (monster_effect(m, ET_CHARM_MONSTER)
            && (rand_m_n(5, 30) * monster_level(m) - player_get_cha(p) < 30))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(p->log, "The %s is awestruck at your magnificence!",
                          monster_get_name(m));
        }
        return;
    }

    /* choose attack type */
    const gboolean has_weap_attack
        = (m->weapon != NULL && monster_attack_available(m, ATT_WEAPON));

    /* prefer weapon attack */
    if (has_weap_attack && chance(80))
    {
        att = monster_attack_get(m, ATT_WEAPON);
    }
    else
    {
        /* choose an attack which is known to work */
        for (idx = 0; idx < monster_attack_count(m); idx++)
        {
            /* if player is resistant to an attack choose next attack type. */
            if (monsters[m->type].attacks[idx].type && !m->attacks_failed[idx])
            {
                att = &monsters[m->type].attacks[idx];
                break;
            }
        }
        if (!att && has_weap_attack)
            att = monster_attack_get(m, ATT_WEAPON);
    }

    /* no attack has been found. return to calling function. */
    if (!att) return;

    /* generate damage */
    dam = damage_new(att->damage, att->type, att->base + game_difficulty(nlarn), m);

    /* deal with random damage (spirit naga) */
    if (dam->type == DAM_RANDOM) dam->type = rand_1n(DAM_MAX);

    /* half damage if player is protected against spirits */
    if (player_effect(p, ET_SPIRIT_PROTECTION) && monster_is_spirit(m))
    {
        if (dam->type == DAM_PHYSICAL)
        {
            /* half physical damage */
            dam->amount >>= 1;
        }
        else
        {
            /* cancel special attacks - mark attack type as useless */
            monster_attack_disable(m, att);
            damage_free(dam);

            return;
        }
    }

    /* set damage for weapon attacks */
    if ((att->type == ATT_WEAPON) && (m->weapon != NULL))
    {
        item *weapon = game_item_get(nlarn, m->weapon);
        dam->amount = rand_1n(weapon_wc(weapon) + game_difficulty(nlarn));
    }

    /* add variable damage */
    if (att->rand) dam->amount += rand_1n(att->rand);

    /* handle some damage types here */
    switch (dam->type)
    {
    case DAM_STEAL_GOLD:
    case DAM_STEAL_ITEM:
        if (monster_player_rob(m, p, (dam->type == DAM_STEAL_GOLD)
                               ? IT_GOLD : IT_ALL))
        {
            /* teleport away */
            monster_pos_set(m, game_map(nlarn, m->pos.z),
                            map_find_space(game_map(nlarn, m->pos.z), LE_MONSTER, FALSE));
        }
        else
        {
            /* if robbery fails mark attack type as useless */
            monster_attack_disable(m, att);
        }
        break;

    case DAM_RUST:
        log_add_entry(p->log, "The %s %s you.", monster_get_name(m),
                      monster_attack_verb[att->type]);

        if (!monster_item_rust(m, p))
        {
            /* a failed attack causes frustration */
            monster_attack_disable(m, att);
        }
        break;

    case DAM_REM_ENCH:
        if (!monster_item_disenchant(m, p))
            monster_attack_disable(m, att);
        break;

    default:
        /* log the attack */
        log_add_entry(p->log, "The %s %s you.", monster_get_name(m),
                      monster_attack_verb[att->type]);

        player_damage_take(p, dam, PD_MONSTER, m->type);
        break;
    }
}

/**
 * Deal damage to a monster
 * @param monster
 * @param damage to be dealt
 * @return the monster if it has survived, othewise NULL
 */
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
                m->type = MT_BRONCE_DRAGON + rand_0n(9);
                gboolean seen_new = monster_in_sight(m);

                if (p && (seen_old || seen_new))
                {
                    if (seen_old && seen_new)
                    {
                        log_add_entry(p->log, "The metamorph turns into a %s!",
                                      monster_name(m));
                    }
                    else if (seen_old)
                        log_add_entry(p->log, "The metamorph vanishes!");
                    else
                    {
                        log_add_entry(p->log, "A %s suddenly appears!",
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

/**
 * Determine a monster's action.
 *
 * @param the monster
 * @return TRUE if the action has changed
 */
gboolean monster_update_action(monster *m)
{
    monster_action_t naction;   /* new action */
    guint time;
    gboolean low_hp;
    gboolean smart;
    gboolean all_attacks_failed = TRUE;

    int idx;

    /* FIXME: should include difficulty here */
    time   = monster_int(m) + 25;
    low_hp = (m->hp < (monster_hp_max(m) / 4 ));
    smart  = (monster_int(m) > 4);

    /* check if monster has an attack that is known to work */
    for (idx = 0; idx < monster_attack_count(m); idx++)
    {
        if (!m->attacks_failed[idx])
        {
            all_attacks_failed = FALSE;
            break;
        }
    }

    if (monster_is_mimic(m) && m->unknown)
    {
        /* sobject monsters */
        naction = MA_REMAIN;
    }
    else if (monster_effect(m, ET_HOLD_MONSTER)
             || monster_effect(m, ET_SLEEP))
    {
        /* no action if monster is held or sleeping */
        naction = MA_REMAIN;
    }
    else if ((low_hp && smart)
             || (monster_effect(m, ET_SCARE_MONSTER) > monster_int(m))
             || all_attacks_failed)
    {
        /* low HP or very scared => FLEE player */
        naction = MA_FLEE;
    }
    else if (m->lastseen && (m->lastseen < time))
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
    if (monster_can_regenerate(m))
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
    g_string_append_printf(desc, "%s %s %s, %s", a_an(injury),
                           injury, monster_name(m),
                           monster_ai_desc[m->action]);

    /* add effect desctiption */
    if (m->effects->len > 0)
    {
        char **desc_list = strv_new();

        int i;
        for (i = 0; i < m->effects->len; i++)
        {
            effect *e = game_effect_get(nlarn, g_ptr_array_index(m->effects, i));

            if(effect_get_desc(e))
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

char monster_image(monster *m)
{
    assert (m != NULL);

    if (m->unknown)
    {
        return item_image(m->item_type);
    }
    else
    {
        return monsters[m->type].image;
    }
}

int monster_colour(monster *m)
{
    assert (m != NULL);

    if (m->unknown)
    {
        return m->colour;
    }
    else
    {
        return monsters[m->type].colour;
    }
}

int monster_genocide(int monster_id)
{
    GList *mlist;
    monster *monst;
    int count = 0;

    assert(monster_id > MT_NONE && monster_id < MT_MAX);

    nlarn->monster_genocided[monster_id] = TRUE;
    mlist = g_hash_table_get_values(nlarn->monsters);

    /* purge genocided monsters */
    do
    {
        monst = (monster *)mlist->data;
        if (monster_is_genocided(monst->type))
        {
            monster_destroy(monst);
            count++;
        }
    }
    while ((mlist = mlist->next));

    g_list_free(mlist);

    return count;
}

int monster_is_genocided(int monster_id)
{
    assert(monster_id > MT_NONE && monster_id < MT_MAX);
    return nlarn->monster_genocided[monster_id];
}

effect *monster_effect_add(monster *m, effect *e)
{
    assert(m != NULL && e != NULL);

    e = effect_add(m->effects, e);

    /* show message if monster is visible */
    if (e && monster_in_sight(m) && effect_get_msg_m_start(e))
    {
        log_add_entry(nlarn->p->log, effect_get_msg_m_start(e),
                      monster_name(m));
    }

    return e;
}

int monster_effect_del(monster *m, effect *e)
{
    assert(m != NULL && e != NULL);

    /* log info if the player can see the monster */
    if (monster_in_sight(m) && effect_get_msg_m_stop(e))
    {
        log_add_entry(nlarn->p->log, effect_get_msg_m_stop(e),
                      monster_name(m));
    }

    return effect_del(m->effects, e);
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

        if (effect_expire(e) == -1)
        {
            /* effect has expired */
            monster_effect_del(m, e);
            effect_destroy(e);
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

    if (player_effect(nlarn->p, ET_INVISIBILITY) && !monster_has_infravision(m))
        return FALSE;

    /* monster is blinded */
    if (monster_effect(m, ET_BLINDNESS))
        return FALSE;

    /* determine if player's position is visible from monster's position */
    return map_pos_is_visible(monster_map(m), m->pos, nlarn->p->pos);
}

static gboolean monster_attack_available(monster *m, attack_t type)
{
    int idx;

    for (idx = 0; idx < MONSTER_ATTACK_COUNT; idx++)
    {
        if (monster_attack(m, idx).type == type)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static const attack *monster_attack_get(monster *m, attack_t type)
{
    int idx;

    for (idx = 0; idx < MONSTER_ATTACK_COUNT; idx++)
    {
        if (monster_attack(m, idx).type == type)
        {
            return &monster_attack(m, idx);
        }
    }

    return NULL;
}

static void monster_attack_disable(monster *m, const attack *att)
{
    int idx;

    for (idx = 0; idx < MONSTER_ATTACK_COUNT; idx++)
    {
        if (monsters[m->type].attacks[idx].type == att->type)
            m->attacks_failed[idx] = TRUE;
    }
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

        log_add_entry(nlarn->p->log, "The %s wields %s.",
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

    /* Don't destroy the potion of cure dianthroritis. */
    if (it->type == IT_POTION && it->id == PO_CURE_DIANTHR)
        return (inv_length(p->inventory) > 1);

    /* log the attack */
    log_add_entry(p->log, "The %s hits you. You feel a sense of loss.",
                  monster_get_name(m));

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
        log_add_entry(p->log, "Nothing happens.");
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

            log_add_entry(p->log, "The %s picks your pocket. " \
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
                            log_add_entry(p->log, "The %s picks up some gold at your feet. ",
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
            item_describe(it, player_item_known(p, it), TRUE, TRUE, buf, 60);

            if (player_item_is_equipped(p, it))
            {
                if (it->cursed)
                {
                    /* cursed items can't be stolen.. */
                    log_add_entry(p->log, "The %s tries to steal %s but failed.",
                                  monster_get_name(m), buf);

                    /* return true as there actually are things to steal */
                    return TRUE;
                }

                log_disable(p->log);
                player_item_unequip(p, NULL, it);
                log_enable(p->log);
            }

            inv_del_element(&p->inventory, it);
            log_add_entry(p->log, "The %s picks your pocket and steals %s.",
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
        log_add_entry(p->log, "The %s couldn't find anything to steal.",
                      monster_get_name(m));

        return FALSE;
    }
}
