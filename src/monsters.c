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

#include <assert.h>
#include <stdlib.h>

#include "game.h"
#include "level.h"
#include "monsters.h"
#include "nlarn.h"

#define EMPTY_ATTACK { ATT_NONE, DAM_NONE, 0, 0 }

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
        0, 0, 0, 0, 0, 0, ' ',
        SPEED_NONE, ESIZE_NONE,
        MF_NONE,
        {
            EMPTY_ATTACK,
            EMPTY_ATTACK,
        }

    },
    {
        MT_GIANT_BAT, "giant bat",
        1, 0, 3, 0, 1, 1, 'B',
        SPEED_FAST, ESIZE_SMALL,
        MF_HEAD | MF_FLY | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_GNOME, "gnome",
        1, 10, 8, 30, 2, 2, 'G',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_HOBGOBLIN, "hobgoblin",
        1, 14, 5, 25, 3, 2, 'H',
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_JACKAL, "jackal",
        1, 17, 4, 0, 1, 1, 'J',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_KOBOLD, "kobold",
        1, 20, 7, 10, 1, 1, 'K',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ORC, "orc",
        2, 12, 9, 40, 4, 2, 'O',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_SNAKE, "snake",
        2, 15, 3, 0, 3, 1, 'S',
        SPEED_NORMAL, ESIZE_TINY,
        MF_HEAD,
        {
            { ATT_BITE, DAM_POISON, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
        }
    },
    {
        MT_CENTIPEDE, "giant centipede",
        2, 14, 2, 0, 1, 2, 'c',
        SPEED_NORMAL, ESIZE_TINY,
        MF_HEAD,
        {
            { ATT_BITE, DAM_DEC_STR, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
        }
    },
    {
        /* a winged, leaping snake */
        MT_JACULUS, "jaculus",
        2, 20, 3, 0, 2, 1, 'j',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_FLY,
        {
            { ATT_BITE, DAM_PHYSICAL, 2, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_TROGLODYTE, "troglodyte",
        2, 10, 5, 80, 4, 3, 't',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_GIANT_ANT, "giant ant",
        2, 8, 3, 0, 5, 5, 'A',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BITE, DAM_DEC_STR, 2, 0 },
            { ATT_BITE, DAM_PHYSICAL, 1, 0 },
        }
    },
    {
        MT_FLOATING_EYE, "floating eye",
        3, 8, 3, 0, 5, 2, 'E',
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_FLY,
        {
            { ATT_GAZE, DAM_PARALYSIS, 1, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_LEPRECHAUN, "leprechaun",
        3, 3, 6, 1500, 13, 45, 'L',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_TOUCH, DAM_STEAL_GOLD, 0, 0 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_NYMPH, "nymph",
        3, 3, 9, 0, 18, 45, 'N',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS,
        {
            { ATT_TOUCH, DAM_STEAL_ITEM, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_QUASIT, "quasit",
        3, 5, 3, 0, 10, 15, 'Q',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_CLAW, DAM_DEC_DEX, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_RUST_MONSTER, "rust monster",
        3, 4, 3, 0, 18, 25, 'R',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_METALLIVORE,
        {
            { ATT_TOUCH, DAM_RUST, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_ZOMBIE, "zombie",
        3, 12, 3, 0, 6, 7, 'Z',
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_UNDEAD,
        {
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
            { ATT_BITE, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_ASSASSIN_BUG, "assassin bug",
        4, 9, 3, 0, 20, 15, 'a',
        SPEED_NORMAL, ESIZE_TINY,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_BUGBEAR, "bugbear",
        4, 5, 5, 40, 20, 35, 'b',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 5, 10 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_HELLHOUND, "hell hound",
        4, 5, 6, 0, 16, 35, 'h',
        SPEED_FAST, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BREATH, DAM_FIRE, 8, 15 },
            { ATT_BITE, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_ICE_LIZARD, "ice lizard",
        4, 11, 6, 50, 16, 25, 'i',
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_HEAD,
        {
            { ATT_SLAM, DAM_PHYSICAL, 14, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 2, 0 },
        }
    },
    {
        MT_CENTAUR, "centaur",
        4, 6, 10, 40, 24, 45, 'C',
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            { ATT_KICK, DAM_PHYSICAL, 6, 0 },
        }
    },
    {
        MT_TROLL, "troll",
        5, 4, 9, 80, 50, 300, 'T',
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_HANDS | MF_REGENERATE,
        {
            { ATT_CLAW, DAM_PHYSICAL, 5, 0 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_YETI, "yeti",
        5, 6, 5, 50, 35, 100, 'Y',
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_HANDS,
        {
            { ATT_CLAW, DAM_PHYSICAL, 4, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ELF, "elf",
        5, 8, 15, 50, 22, 35, 'e',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_GELATINOUSCUBE, "gelatinous cube",
        5, 9, 3, 0, 22, 45, 'g',
        SPEED_SLOW, ESIZE_LARGE,
        MF_METALLIVORE,
        {
            { ATT_SLAM, DAM_ACID, 1, 0 },
        }
    },
    {
        MT_METAMORPH, "metamorph",
        6, 7, 3, 0, 30, 40, 'm',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_NONE,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_VORTEX, "vortex",
        6, 4, 3, 0, 30, 55, 'v',
        SPEED_NORMAL, ESIZE_TINY,
        MF_NONE,
        {
            { ATT_NONE, DAM_NONE, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ZILLER, "ziller",
        6, 15, 3, 0, 30, 35, 'z',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD,
        {
            { ATT_NONE, DAM_NONE, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    { /* FIXME: I don't want no silly mushrooms */
        MT_VIOLET_FUNGUS, "violet fungi",
        6, 12, 3, 0, 38, 100, 'F',
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_NONE,
        {
            { ATT_SLAM, DAM_POISON, 1, 0 },
            { ATT_SLAM, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_WRAITH, "wraith",
        6, 3, 3, 0, 30, 325, 'w',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_UNDEAD,
        {
            { ATT_TOUCH, DAM_DRAIN_LIFE, 1, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_FORVALAKA, "forvalaka",
        6, 2, 7, 0, 50, 280, 'f',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_UNDEAD | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 5, 0 },
            EMPTY_ATTACK,
        }
    },
    { /* TODO: get rid of this beast */
        MT_LAMA_NOBE, "lama nobe",
        7, 7, 6, 0, 35, 80, 'l',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD,
        {
            { ATT_BITE, DAM_NONE, 3, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_OSQUIP, "osquip",
        7, 4, 4, 0, 35, 100, 'o',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 10, 15 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_ROTHE, "rothe",
        7, 15, 3, 100, 50, 250, 'r',
        SPEED_FAST, ESIZE_LARGE,
        MF_HEAD | MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 5, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 3, 0 },
        }
    },
    {
        MT_XORN, "xorn",
        7, 0, 13, 0, 60, 300, 'X',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_INFRAVISION,
        {
            { ATT_BITE, DAM_PHYSICAL, 6, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_VAMPIRE, "vampire",
        7, 3, 17, 0, 50, 1000, 'v',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_FLY | MF_UNDEAD | MF_INFRAVISION | MF_REGENERATE,
        {
            { ATT_TOUCH, DAM_DRAIN_LIFE, 1, 0 },
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
        }
    },
    {
        MT_STALKER, "invisible stalker",
        7, 3, 14, 0, 50, 350, 'I',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_FLY | MF_INVISIBLE,
        {
            { ATT_SLAM, DAM_PHYSICAL, 6, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_POLTERGEIST, "poltergeist",
        8, 1, 5, 0, 50, 450, 'p',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_UNDEAD | MF_FLY | MF_INVISIBLE,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_DISENCHANTRESS, "disenchantress",
        8, 3, 5, 0, 50, 500, 'q',
        SPEED_NORMAL, ESIZE_MEDIUM,
        MF_HEAD | MF_HANDS | MF_METALLIVORE,
        {
            { ATT_TOUCH, DAM_REM_ENCH, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_SHAMBLINGMOUND, "shambling mound",
        8, 2, 6, 0, 45, 400, 's',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_NONE,
        {
            { ATT_SLAM, DAM_PHYSICAL, 5, 0 },
            EMPTY_ATTACK,
        }
    },
    { /* FIXME: replace this beast! */
        MT_YELLOW_MOLD, "yellow mold",
        8, 12, 3, 0, 35, 250, 'y',
        SPEED_NONE, ESIZE_SMALL,
        MF_NONE,
        {
            { ATT_TOUCH, DAM_PHYSICAL, 4, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_UMBER_HULK, "umber hulk",
        8, 3, 14, 0, 65, 600, 'U',
        SPEED_SLOW, ESIZE_HUGE,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_GAZE, DAM_CONFUSION, 0, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 7, 0 },
        }
    },
    {
        MT_GNOME_KING, "gnome king",
        9, -1, 18, 2000, 100,   3000, 'k',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_MIMIC, "mimic",
        9, 5, 8, 0, 55, 99, 'M',
        SPEED_SLOW, ESIZE_MEDIUM,
        MF_NONE,
        {
            { ATT_SLAM, DAM_PHYSICAL, 6, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_WATER_LORD, "water lord",
        9,-10, 20, 0, 150, 15000, 'w',
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_PURPLE_WORM, "purple worm",
        9, -1, 3, 100, 120, 15000, 'P',
        SPEED_SLOW, ESIZE_GARGANTUAN,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 11, 0 },
            { ATT_STING, DAM_POISON, 3, 0 },
        }
    },
    {
        MT_XVART, "xvart",
        9, -2, 13, 0, 90, 1000, 'x',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_HEAD | MF_HANDS | MF_INFRAVISION,
        {
            { ATT_WEAPON, DAM_PHYSICAL, 0, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_WHITE_DRAGON, "white dragon",
        5, 2, 16,  500, 55, 1000, 'd',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD,
        {
            { ATT_BITE, DAM_PHYSICAL, 4, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 4, 0 },
        }
    },
    { /* TODO: replace this beast */
        MT_BRONCE_DRAGON, "bronze dragon",
        9, 2, 16, 300, 80, 4000, 'D',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY,
        {
            { ATT_BITE, DAM_PHYSICAL, 9, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 9, 0 },
        }
    },
    {
        MT_GREEN_DRAGON, "green dragon",
        9, 3, 15, 200, 70, 2500, 'D',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY,
        {
            { ATT_SLAM, DAM_PHYSICAL, 25, 0 },
            { ATT_CLAW, DAM_PHYSICAL, 8, 0 },
        }
    },
    { /* TODO: replace this beast */
        MT_SILVER_DRAGON, "silver dragon",
        10, -1, 20, 700, 100, 10000, 'D',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY,
        {
            { ATT_CLAW, DAM_PHYSICAL, 12, 0 },
            { ATT_BITE, DAM_PHYSICAL, 12, 0 },
        }
    },
    {
        MT_PLATINUM_DRAGON, "platinum dragon",
        10, -5, 22, 1000, 130, 24000, 'D',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY,
        {
            { ATT_MAGIC, DAM_MAGICAL, 15, 30 },
            { ATT_BITE, DAM_PHYSICAL, 15, 0 },
        }
    },
    {
        MT_RED_DRAGON, "red dragon",
        10, -2, 19, 800, 110, 14000, 'D',
        SPEED_NORMAL, ESIZE_HUGE,
        MF_HEAD | MF_FLY,
        {
            { ATT_BREATH, DAM_FIRE, 20, 25 },
            { ATT_CLAW, DAM_PHYSICAL, 13, 0 },
        }
    },
    {
        MT_SPIRIT_NAGA, "spirit naga",
        10, -20, 23, 0, 95, 20000, 'n',
        SPEED_NORMAL, ESIZE_LARGE,
        MF_HEAD | MF_NOBEHEAD | MF_FLY | MF_SPIRIT | MF_INFRAVISION,
        {
            { ATT_MAGIC, DAM_RANDOM, 1, 0 },
            { ATT_BITE, DAM_PHYSICAL, 12, 0 },
        }
    },
    { /* TODO: relplace this beast */
        MT_GREEN_URCHIN, "green urchin",
        10, -3, 3, 0, 85, 5000, 'u',
        SPEED_NORMAL, ESIZE_SMALL,
        MF_NONE,
        {
            { ATT_STING, DAM_PHYSICAL, 12, 0 },
            EMPTY_ATTACK,
        }
    },
    {
        MT_DEMONLORD_I, "type I demon lord",
        12, -30, 20, 0, 140, 50000, '&',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 18, 0 },
            { ATT_BITE, DAM_PHYSICAL, 18, 0 },
        }
    },
    {
        MT_DEMONLORD_II, "type II demon lord",
        13, -30, 21, 0, 160, 75000, '&',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 18, 0 },
            { ATT_BITE, DAM_PHYSICAL, 18, 0 },
        }
    },
    {
        MT_DEMONLORD_III, "type III demon lord",
        14, -30,  22, 0, 180, 100000, '&',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 18, 0 },
            { ATT_BITE, DAM_PHYSICAL, 18, 0 },
        }
    },
    {
        MT_DEMONLORD_IV, "type IV demon lord",
        15, -35, 23, 0, 200, 125000, '&',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 20, 0 },
            { ATT_BITE, DAM_PHYSICAL, 20, 0 },
        }
    },
    {
        MT_DEMONLORD_V, "type V demon lord",
        16, -40, 24, 0, 220, 150000, '&',
        SPEED_FAST, ESIZE_MEDIUM,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 22, 0 },
            { ATT_BITE, DAM_PHYSICAL, 22, 0 },
        }
    },
    {
        MT_DEMONLORD_VI, "type VI demon lord",
        17, -45, 25, 0, 240, 175000, '&',
        SPEED_FAST, ESIZE_LARGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 24, 0 },
            { ATT_BITE, DAM_PHYSICAL, 24, 0 },
        }
    },
    {
        MT_DEMONLORD_VII, "type VII demon lord",
        18, -70, 26, 0, 260, 200000, '&',
        SPEED_FAST, ESIZE_HUGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 27, 0 },
            { ATT_BITE, DAM_PHYSICAL, 27, 0 },
        }
    },
    {
        MT_DAEMON_PRINCE, "demon prince",
        25, -127, 28, 0, 345, 300000, '&',
        SPEED_FAST, ESIZE_HUGE,
        MF_HEAD | MF_NOBEHEAD | MF_HANDS | MF_FLY | MF_INVISIBLE | MF_INFRAVISION,
        {
            { ATT_CLAW, DAM_PHYSICAL, 30, 0 },
            { ATT_BITE, DAM_PHYSICAL, 30, 0 },
        }
    }
};

static gboolean monster_attack_available(monster *m, attack_t type);
static const attack *monster_attack_get(monster *m, attack_t type);
static void monster_attack_disable(monster *m, const attack *att);
static item *monster_weapon_select(monster *m);
static void monster_weapon_wield(monster *m, item *weapon);
static void monster_die(monster *m);
static gboolean monster_item_disenchant(monster *m, struct player *p);
static gboolean monster_item_rust(monster *m, struct player *p);
static gboolean monster_player_rob(monster *m, struct player *p, item_t item_type);

monster *monster_new(int monster_type)
{
    monster *nmonster;
    int it, icount;     /* item type, item id, item count */

    assert(monster_type > MT_NONE && monster_type < MT_MAX);

    /* make room for monster */
    nmonster = g_malloc0(sizeof(monster));

    nmonster->type = monster_type;
    nmonster->hp = divert(monsters[monster_type].hp_max, 10);

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
        inv_add(&nmonster->inventory, item_new(IT_GOLD, icount, 0));
    }

    /* add special items */
    switch (monster_type)
    {
    case MT_LEPRECHAUN:
        if (chance(25))
        {
            inv_add(&nmonster->inventory,
                    item_new(IT_GEM, rand_1n(item_max_id(IT_GEM)), 0));
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
        switch (monster_type)
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

        weapon = item_new(IT_WEAPON, weapons[rand_0n(weapon_count)], rand_m_n(-2,2));
        inv_add(&nmonster->inventory, weapon);

        /* wield the new weapon */
        monster_weapon_wield(nmonster, weapon);
    } /* finished initializing weapons */

    /* initialize mimics */
    if (monster_type == MT_MIMIC)
    {
        /* determine how the mimic will be displayed */
        nmonster->item_type = rand_1n(IT_MAX);
        /* the mimic is not known to be a monster */
        nmonster->unknown = TRUE;
    }

    /* initialize AI */
    nmonster->action = MA_WANDER;
    nmonster->lastseen = -1;
    nmonster->player_pos = pos_new(G_MAXINT16, G_MAXINT16, G_MAXINT16);

    /* register monster with game */
    game_monster_register(nlarn, nmonster);

    return nmonster;
}

monster *monster_new_by_level(int nlevel)
{
    const int monster_level[] = { 5, 11, 17, 22, 27, 33, 39, 42, 46, 50, 53, 56, MT_MAX - 1 };
    int monster_id = 0;
    int monster_id_min;
    int monster_id_max;

    assert(nlevel > 0 && nlevel < LEVEL_MAX);

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

    return monster_new(monster_id);
}

void monster_destroy(monster *m)
{
    assert(m != NULL);

    /* free effects */
    while (m->effects->len > 0)
    {
        effect *e = g_ptr_array_remove_index(m->effects, m->effects->len - 1);
        effect_destroy(e);
    }

    g_ptr_array_free(m->effects, TRUE);

    /* remove monster from level (if the level existst) */
    if (nlarn->levels[m->pos.z] != NULL)
        g_ptr_array_remove_fast(nlarn->levels[m->pos.z]->mlist, m);

    /* free inventory */
    if (m->inventory)
        inv_destroy(m->inventory);

    /* unregister monster */
    game_monster_unregister(nlarn, m);

    g_free(m);
}

void monster_level_enter(monster *m, struct level *l)
{
    assert (m != NULL && l != NULL);

    /* remove monster from old level if it has been somewhere before */
    if (m->level != NULL)
    {
        g_ptr_array_remove_fast(m->level->mlist, m);
    }

    /* put monster into level */
    m->level = l;
    m->pos.z = l->nlevel;

    if (!monster_position(m, level_find_space(l, LE_MONSTER)))
    {
        /* no free space could be found for the monstert -> abort */
        monster_destroy(m);
    }

    g_ptr_array_add(l->mlist, m);
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
    level_path *path = NULL;

    level_path_element *el = NULL;

    /* damage caused by level effects */
    damage *dam = NULL;

    int monster_visrange = 7;

    if (player_effect(p, ET_STEALTH))
    {
        /* if the player is stealthy monsters will only recognize him when
           standing next to him */
        monster_visrange = 1;
    }

    /* determine if the monster can see the player */
    if (pos_distance(m->pos, p->pos) > 7)
    {
        m->p_visible = FALSE;
    }
    else if (player_effect(p, ET_INVISIBILITY) && !monster_has_infravision(m))
    {
        m->p_visible = FALSE;
    }
    else if (monster_effect(m, ET_BLINDNESS))
    {
        /* monster is blinded */
        m->p_visible = FALSE;
    }
    else
    {
        /* determine if player's position is visible from monster's position */
        m->p_visible = level_pos_is_visible(m->level, m->pos, p->pos);
    }

    if (m->p_visible)
    {
        /* update monster's knowledge of player's position */
        monster_update_player_pos(m, p->pos);
    }

    /*
     * regenerate / inflict poison upon monster.
     * as monster might be killed during the process we need
     * to exit the loop in this case
     */
    if (!monster_regenerate(m, game_turn(nlarn), game_difficulty(nlarn), p->log))
    {
        return;
    }

    /* deal damage caused by floor effects */
    if ((dam = level_tile_damage(m->level, m->pos)))
    {
        if (!(m = monster_damage_take(m, dam)))
            return;
    }

    /* update monsters action */
    if (monster_update_action(m) && m->m_visible)
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

    /* determine monster's next move */
    m_npos = m->pos;

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

            m_npos_tmp = pos_move(m->pos, tries);

            if (pos_valid(m_npos_tmp)
                    && lt_is_passable(level_tiletype_at(m->level,m_npos_tmp))
                    && !level_is_monster_at(m->level, m_npos_tmp)
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
                || !lt_is_passable(level_tiletype_at(m->level,m_npos))
                || level_is_monster_at(m->level, m_npos))
                && (tries < GD_MAX));

        /* new position has not been found, reset to current position */
        if (tries == GD_MAX)
            m_npos = m->pos;

        break; /* end MA_WANDER */

    case MA_ATTACK:
        if (pos_adjacent(m->pos, m->player_pos) && (m->lastseen == 1))
        {
            /* monster is standing next to player */
            monster_player_attack(m, p);

            /* monster's position might have changed (teleport) */
            if (!pos_identical(m_npos, m->pos))
            {
                m_npos = m->pos;
                log_add_entry(p->log, "The %s vanishes.", monster_name(m));
            }
        }
        else
        {
            /* monster heads into the direction of the player. */

            /* if the monster is on a different level than the player,
               try to find the staircase to reach the player's level */
            if (m->pos.z != m->player_pos.z)
            {
                level_stationary_t what;
                if (m->pos.z > m->player_pos.z)
                    what = LS_STAIRSUP;
                else
                    what = LS_STAIRSDOWN;

                m->player_pos = level_find_stationary(m->level, what);
            }

            path = level_find_path(m->level, m->pos, m->player_pos);

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
                level_path_destroy(path);
            }
        }
        break; /* end MA_ATTACK */

    case MA_NONE:
    case MA_MAX:
        /* possibly a bug */
        break;
    }

    /* can the player see the new position? */
    m->m_visible = player_pos_visible(p, m_npos);

    /* *** if new position has been found - move the monster *** */
    if (!pos_identical(m_npos, m->pos))
    {
        /* vampires won't step onto mirrors */
        if ((m->type == MT_VAMPIRE)
                && (level_stationary_at(m->level, m_npos) == LS_MIRROR))
        {
            /* FIXME: should try to move around it */
            m_npos = m->pos;
        }

        if (pos_identical(p->pos, m_npos))
        {
            /* bump into invisible player */
            monster_update_player_pos(m, p->pos);
            m_npos = m->pos;

            log_add_entry(p->log, "The %s bumped into you.", monster_name(m));
        }

        /* check for door */
        if ((level_stationary_at(m->level, m_npos) == LS_CLOSEDDOOR)
                && monster_has_hands(m)
                && monster_int(m) > 3) /* lock out zombies */
        {
            /* open the door */
            level_stationary_set(m->level, m_npos, LS_OPENDOOR);

            /* notify the player if the door is visible */
            if (m->m_visible)
            {
                log_add_entry(p->log, "The %s opens the door.", monster_name(m));
            }
        }

        /* move towards player; check for monsters */
        else if (!level_is_monster_at(m->level, m_npos)
                 && ls_is_passable(level_stationary_at(m->level, m_npos)))
        {
            monster_position(m, m_npos);

            /* check for traps */
            if (level_trap_at(m->level, m->pos))
            {
                if (!monster_trap_trigger(m, p))
                    return; /* trap killed the monster */
            }

        } /* end new position */
    } /* end monster repositioning */

    monster_items_pickup(m, p);

    /* increment count of turns since when player was last seen */
    if (m->lastseen) m->lastseen++;
}

int monster_position(monster *m, position target)
{
    assert(m != NULL);

    if (level_validate_position(m->level, target, LE_MONSTER))
    {
        m->pos = target;
        return TRUE;
    }

    return FALSE;
}

monster *monster_trap_trigger(monster *m, struct player *p)
{
    /* original position of the monster */
    position pos_orig;

    /* the trap */
    trap_t trap;

    /* effect monster might have gained during the move */
    effect *eff = NULL;

    assert (m != NULL && p != NULL);

    trap = level_trap_at(m->level, m->pos);

    /* return if the monster has not triggered the trap */
    if (!chance(trap_chance(trap)))
    {
        return m;
    }

    /* flying monsters are only affected by sleeping gas traps */
    if (monster_can_fly(m) && (trap != TT_SLEEPGAS))
    {
        return m;
    }

    pos_orig = m->pos;

    /* monster triggered the trap */
    switch (trap)
    {
    case TT_TRAPDOOR:
        monster_level_enter(m, nlarn->levels[m->pos.z + 1]);
        break;

    case TT_TELEPORT:
        monster_position(m, level_find_space(m->level, LE_MONSTER));
        break;

    default:
        /* if there is an effect on the trap add it to the
         * monster's list of effects. */
        if (trap_effect(trap))
        {
            eff = effect_new(trap_effect(trap), game_turn(nlarn));
            monster_effect_add(m, eff);
        }

    } /* switch (trap) */

    if (m->m_visible)
    {
        log_add_entry(p->log, trap_m_message(trap), monster_name(m));

        if (eff)
        {
            log_add_entry(p->log, effect_get_msg_m_start(eff),
                          monster_name(m));
        }

        /* set player's knowlege of trap */
        player_memory_of(p, pos_orig).trap = trap;
    }

    /* inflict damage caused ba the trap */
    if (trap_damage(trap))
    {
        damage *dam = damage_new(DAM_PHYSICAL, rand_1n(trap_damage(trap)), NULL);
        m = monster_damage_take(m, dam);
    }

    return m;
}

void monster_items_drop(monster *m, inventory **floor)
{
    assert(m != NULL && floor != NULL);

    while (inv_length(m->inventory) > 0)
    {
        inv_add(floor, inv_get(m->inventory, 0));
        inv_del(&m->inventory, 0);
    }
}

void monster_items_pickup(monster *m, struct player *p)
{
    /* TODO: gelatious cube digests items, rust monster eats metal stuff */
    /* FIXME: time management */

    gboolean pick_up = FALSE;
    guint idx;
    item *it;
    char buf[61] = { 0 };

    assert(m != NULL && p != NULL);

    for (idx = 0; idx < inv_length(*level_ilist_at(m->level, m->pos)); idx++)
    {
        it = inv_get(*level_ilist_at(m->level, m->pos), idx);

        if (m->type == MT_LEPRECHAUN
                && ((it->type == IT_GEM) || (it->type == IT_GOLD)))
        {
            /* leprechauns collect treasures */
            pick_up = TRUE;
        }
        else if (it->type == IT_WEAPON && monster_attack_available(m, ATT_WEAPON))
        {
            /* monster can attack with weapons, get the weapon */
            pick_up = TRUE;
        }

        if (pick_up)
        {
            /* item has been picked up */
            if (m->m_visible)
            {
                item_describe(it, player_item_identified(m->level->player, it),
                              (it->count == 1), FALSE, buf, 60);
                log_add_entry(p->log, "The %s picks up %s.", monster_name(m), buf);
            }

            inv_del_element(level_ilist_at(m->level, m->pos), it);
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

            pick_up = FALSE;
        }
    }
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
        if (!level_is_monster_at(p->level, p->pos) && m->m_visible)
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
        log_add_entry(p->log, "The %s misses wildly.", monster_name(m));
        return;
    }

    if (player_effect(p, ET_CHARM_MONSTER)
            && (rand_m_n(5, 30) * monster_level(m) - player_get_cha(p) < 30))
    {
        log_add_entry(p->log, "The %s is awestruck at your magnificence!",
                      monster_name(m));
        return;
    }

    /* choose attack type */
    if (m->weapon != NULL && monster_attack_available(m, ATT_WEAPON))
    {
        /* prefer weapon attack */
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
    }

    /* no attack has been found. return to calling function. */
    if (!att) return;

    /* generate damage */
    dam = damage_new(att->damage, att->base + game_difficulty(nlarn), m);

    /* deal with random damage (spirit naga) */
    if (dam->type == DAM_RANDOM)
        dam->type = rand_1n(DAM_MAX);

    /* set damage for weapon attacks */
    if ((att->type == ATT_WEAPON) && (m->weapon != NULL))
        dam->amount = rand_1n(weapon_wc(m->weapon)
                              + game_difficulty(nlarn));

    /* add variable damage */
    if (att->rand)
        dam->amount += rand_1n(att->rand);

    /* handle some damage types here */
    switch (dam->type)
    {
    case DAM_STEAL_GOLD:
    case DAM_STEAL_ITEM:
        if (monster_player_rob(m, p, (dam->type == DAM_STEAL_GOLD)
                               ? IT_GOLD : IT_ALL))
        {
            /* teleport away */
            monster_position(m, level_find_space(p->level, LE_MONSTER));
        }
        else
        {
            /* if robbery fails mark attack type as useless */
            monster_attack_disable(m, att);
        }
        break;

    case DAM_RUST:
        log_add_entry(p->log, "The %s %s you.", monster_name(m),
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
        log_add_entry(p->log, "The %s %s you.", monster_name(m),
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
            /* notifiy player */
        }

        /* metamorph transforms if HP is low*/
        if (m->type == MT_METAMORPH)
        {
            if ((m->hp < 25) && (m->hp > 0))
            {
                m->type = MT_BRONCE_DRAGON + rand_0n(9);

                if (p)
                {
                    log_add_entry(p->log, "The metamorph turns into a %s!",
                                  monster_name(m));
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
        /* if the monster has been killed by the player give experience */
        if (p)
        {
            player_exp_gain(p, monster_exp(m));
            p->stats.monsters_killed[m->type] += 1;
        }

        monster_die(m);
        m = NULL;
    }

    g_free(dam);

    return m;
}

void monsters_genocide(level *l)
{
    guint idx;
    monster *monst;

    /* purge genocided monsters */
    for (idx = 0; idx < l->mlist->len; idx++)
    {
        monst = g_ptr_array_index(l->mlist, idx);

        if (monster_is_genocided(monst->type))
        {
            g_ptr_array_remove_index_fast(l->mlist, idx);
            monster_destroy(monst);
            idx--;
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

    if (m->type == MT_MIMIC && m->unknown)
    {
        /* stationary monsters */
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
            return FALSE;
    }

    return TRUE;
}

int monster_genocide(int monster_id)
{
    assert(monster_id > MT_NONE && monster_id < MT_MAX);

    nlarn->monster_genocided[monster_id] = TRUE;
    return nlarn->monster_genocided[monster_id];
}

int monster_is_genocided(int monster_id)
{
    assert(monster_id > MT_NONE && monster_id < MT_MAX);
    return nlarn->monster_genocided[monster_id];
}

void monster_effect_expire(monster *m, message_log *log)
{
    guint idx = 0;
    effect *e;

    assert(m != NULL && log != NULL);

    while (idx < m->effects->len)
    {
        e = g_ptr_array_index(m->effects, idx);

        if (effect_expire(e, 1) == -1)
        {
            /* effect has expired */
            monster_effect_del(m, e);

            /* log info */
            if (m->m_visible && effect_get_msg_m_stop(e))
                log_add_entry(log, effect_get_msg_m_stop(e),
                              monster_name(m));

            effect_destroy(e);
        }
        else
        {
            idx++;
        }
    }
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
    m->weapon = weapon;

    /* show message if monster is visible */
    if (m->m_visible)
    {
        item_describe(weapon, player_item_identified(m->level->player, weapon),
                      TRUE, FALSE, buf, 60);

        log_add_entry(m->level->player->log, "The %s wields %s.",
                      monster_name(m), buf);
    }
}


static void monster_die(monster *m)
{
    char *message = NULL;
    struct player *p;

    assert(m != NULL);

    if (!message)
    {
        message = "The %s died!";
    }

    p = m->level->player;

    /* if the player can see the monster describe the event */
    if (m->m_visible && p)
    {
        log_add_entry(p->log, message, monster_name(m));
    }

    /* drop stuff the monster carries */
    if (inv_length(m->inventory))
    {
        monster_items_drop(m, level_ilist_at(m->level, m->pos));
    }

    monster_destroy(m);
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
    log_add_entry(p->log, "The %s hits you. You feel a sense of loss.",
                  monster_name(m));

    if (it->type == IT_WEAPON
            || it->type == IT_ARMOUR
            || it->type == IT_RING
            || it->type == IT_AMULET)
    {
        item_disenchant(it);
    }
    else
    {
        inv_del_element(&p->inventory, it);
        item_destroy(it);
    }

    return TRUE;
}

static gboolean monster_item_rust(monster *m, struct player *p)
{
    item *it;

    assert(m != NULL && p != NULL);

    /* get a random piece of armour to damage */
    if ((it = player_random_armour(p)))
    {
        int pi;

        pi = item_rust(it);

        if (pi == PI_ENFORCED)
        {
            log_add_entry(p->log, "Your %s is dulled.", armour_name(it));
            return TRUE;
        }
        else if (pi == PI_DESTROYED)
        {
            /* armour has been destroyed */
            log_add_entry(p->log, "Your %s disintegrates!",
                          armour_name(it));

            log_disable(p->log);
            player_item_unequip(p, NULL, it);
            log_enable(p->log);

            inv_del_element(&p->inventory, it);
            item_destroy(it);
            return TRUE;
        }
        else
        {
            log_add_entry(p->log, "Your %s is not affected.", armour_name(it));
            return FALSE;
        }
    }
    else
    {
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
                it = item_new(IT_GOLD, player_gold >> 1, 0);
                player_set_gold(p, player_gold >> 1);
            }
            else
            {
                it = item_new(IT_GOLD, rand_1n(1 + (player_gold >> 1)), 0);
                player_set_gold(p, player_gold - it->count);
            }

            log_add_entry(p->log, "The %s picks your pocket. " \
                          "Your purse feels lighter", monster_name(m));
        }
    }
    else if (item_type == IT_ALL) /* must be the nymph */
    {
        if (inv_length(p->inventory))
        {
            it = inv_get(p->inventory, rand_0n(inv_length(p->inventory)));

            if (player_item_is_equipped(p, it))
            {
                log_disable(p->log);
                player_item_unequip(p, NULL, it);
                log_enable(p->log);
            }

            inv_del_element(&p->inventory, it);
            log_add_entry(p->log, "The %s picks your pocket.", monster_name(m));
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
                      monster_name(m));

        return FALSE;
    }
}
