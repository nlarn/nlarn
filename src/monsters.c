/*
 * monsters.c
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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
#include <string.h>

#include "colours.h"
#include "display.h"
#include "fov.h"
#include "game.h"
#include "items.h"
#include "map.h"
#include "monsters.h"
#include "extdefs.h"
#include "pathfinding.h"
#include "random.h"

DEFINE_ENUM(monster_flag, MONSTER_FLAG_ENUM)
DEFINE_ENUM(monster_t, MONSTER_TYPE_ENUM)
DEFINE_ENUM(monster_action_t, MONSTER_ACTION_TYPE_ENUM)

/* local monster definition for data storage */
typedef struct {
    const char *name;        /* monster's name */
    const char *plural_name;
    const char glyph;
    int colour;
    int exp;                 /* xp granted to player */
    int gold_chance;
    int gold;
    int reroll_chance;
    int ac;
    int hp_max;
    int level;
    int intelligence;        /* used to choose movement */
    speed speed;
    size size;
    int flags;
    attack attacks[2];
    monster_action_t default_ai;
    const char *sound;
} monster_data_t;

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
    gpointer leader;    /* for pack monsters: ID of the leader */
    guint32
        unknown: 1;      /* monster is unknown (mimic) */
};

const char *monster_ai_desc[] =
{
    NULL,               /* MA_NONE */
    "fleeing",          /* MA_FLEE */
    "idling",           /* MA_REMAIN */
    "wandering",        /* MA_WANDER */
    "attacking",        /* MA_ATTACK */
    "puzzled",          /* MA_CONFUSION */
    "obedient",         /* MA_SERVE */
    "working",          /* MA_CIVILIAN */
};

const char *monster_attack_verb[] =
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
    colour fg;
} monster_breath_data[] =
{
    { NULL, 0, 0 },                                 /* DAM_NONE */
    { NULL, 0, 0 },                                 /* DAM_PHYSICAL */
    { "psionic blast", '*', WHITE },                /* DAM_MAGICAL */
    { "burst of fire", '~', LAVA },                 /* DAM_FIRE */
    { "beam of frost", '*', ICE_COLD_GREEN },       /* DAM_COLD */
    { "gush of acid", '*', ELECTRIC_BLUE },         /* DAM_ACID */
    { "flood of water", '~', BRIGHT_BLUE },         /* DAM_WATER */
    { "ray of lightning", '*', DYNAMIC_YELLOW },    /* DAM_ELECTRICITY */
    { "burst of noxious fumes", '%', VENOM_GREEN }, /* DAM_POISON */
};

monster_data_t monster_data[] = {
    { /* MT_GIANT_BAT */
        .name = "giant bat", .glyph = 'b', .colour = AUTUMN_LEAF_BROWN,
        .exp = 1, .ac = 0, .hp_max = 2,
        .level = 1, .intelligence = 3, .speed = XFAST, .size = SMALL,
        .flags = HEAD | FLY | INFRAVISION,
        .attacks = {
            { .type = ATT_BITE, .base = 1, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_GNOME */
        .name = "gnome", .glyph = 'g', .colour = CONIFER,
        .exp = 2, .gold_chance = 80, .gold = 30, .ac = 0, .hp_max = 6,
        .level = 1, .intelligence = 8, .speed = NORMAL, .size = SMALL,
        .flags = HEAD | HANDS,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_HOBGOBLIN */
        .name = "hobgoblin", .glyph = 'H', .colour = GREEN_BROWN,
        .exp = 2, .gold_chance = 30, .gold = 40, .ac = 1, .hp_max = 8,
        .level = 1, .intelligence = 5, .speed = SLOW, .size = MEDIUM,
        .flags = HEAD | HANDS | INFRAVISION,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_JACKAL */
        .name = "jackal", .glyph = 'J', .colour = ELM_BROWN_RED,
        .exp = 1, .ac = 0, .hp_max = 3,
        .level = 1, .intelligence = 4, .speed = FAST, .size = SMALL,
        .flags = HEAD,
        .attacks = {
            { .type = ATT_BITE, .base = 1, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER, .sound = "growl"
    },
    { /* MT_KOBOLD */
        .name = "kobold", .glyph = 'k', .colour = DEEP_SEA_GREEN,
        .exp = 1, .gold_chance = 10, .gold = 100, .ac = 0, .hp_max = 4,
        .level = 1, .intelligence = 7, .speed = NORMAL, .size = SMALL,
        .flags = HEAD | HANDS | INFRAVISION,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_ORC */
        .name = "orc", .glyph = 'O', .colour = DUSTY_GREY,
        .exp = 2, .gold_chance = 50, .gold = 80, .ac = 3, .hp_max = 12,
        .level = 2, .intelligence = 9, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS | INFRAVISION | PACK,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER, .sound = "shout"
    },
    { /* MT_SNAKE */
        .name = "snake", .glyph = 'S', .colour = APPLE_GREEN,
        .exp = 1, .ac = 1, .hp_max = 3,
        .level = 2, .intelligence = 3, .speed = NORMAL, .size = TINY,
        .flags = HEAD,
        .attacks = {
            { .type = ATT_BITE, .base = 1, .damage = DAM_PHYSICAL },
            { .type = ATT_BITE, .base = 2, .damage = DAM_POISON },
        }, .default_ai = MA_WANDER, .sound = "hiss"
    },
    { /* MT_CENTIPEDE */
        .name = "giant centipede", .glyph = 'c', .colour = ALSIKE_CLOVER_RED,
        .exp = 2, .ac = 1, .hp_max = 5,
        .level = 2, .intelligence = 2, .speed = NORMAL, .size = SMALL,
        .flags = HEAD,
        .attacks = {
            { .type = ATT_BITE, .base = 50, .damage = DAM_DEC_STR },
            { .type = ATT_BITE, .base = 1, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_JACULUS */
        .name = "jaculus", .plural_name = "jaculi", .glyph = 'j', .colour = FOAM_GREEN,
        .exp = 1, .ac = 3, .hp_max = 8,
        .level = 2, .intelligence = 3, .speed = XFAST, .size = MEDIUM,
        .flags = HEAD | FLY,
        .attacks = {
            { .type = ATT_BITE, .base = 2, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 2, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_TROGLODYTE */
        .name = "troglodyte", .glyph = 't', .colour = CARBON_GREY,
        .exp = 3, .gold_chance = 25, .gold = 320, .ac = 4, .hp_max = 10,
        .level = 2, .intelligence = 5, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_GIANT_ANT */
        .name = "giant ant", .glyph = 'A', .colour = DARK_SAND,
        .exp = 5, .ac = 2, .hp_max = 6,
        .level = 2, .intelligence = 3, .speed = NORMAL, .size = SMALL,
        .flags = HEAD | PACK,
        .attacks = {
            { .type = ATT_BITE, .base = 75, .damage = DAM_DEC_STR },
            { .type = ATT_BITE, .base = 1, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_FLOATING_EYE */
        .name = "floating eye", .glyph = 'E', .colour = PALE_GREEN_ONION,
        .exp = 2, .ac = 2, .hp_max = 12,
        .level = 3, .intelligence = 3, .speed = XSLOW, .size = MEDIUM,
        .flags = FLY | INFRAVISION | RES_CONF,
        .attacks = {
            { .type = ATT_GAZE, .base = 66, .damage = DAM_PARALYSIS },
        }, .default_ai = MA_WANDER
    },
    { /* MT_LEPRECHAUN */
        .name = "leprechaun", .glyph = 'L', .colour = IRELAND_GREEN,
        .exp = 45, .gold = 1500, .ac = 6, .hp_max = 13,
        .level = 3, .intelligence = 6, .speed = NORMAL, .size = SMALL,
        .flags = HEAD | HANDS,
        .attacks = {
            { .type = ATT_CLAW, .base = 2, .damage = DAM_PHYSICAL },
            { .type = ATT_TOUCH, .damage = DAM_STEAL_GOLD },
        }, .default_ai = MA_WANDER
    },
    { /* MT_NYMPH */
        .name = "nymph", .glyph = 'n', .colour = DUSTY_PINK,
        .exp = 45, .ac = 5, .hp_max = 18,
        .level = 3, .intelligence = 9, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS,
        .attacks = {
            { .type = ATT_TOUCH, .damage = DAM_STEAL_ITEM },
        }, .default_ai = MA_WANDER
    },
    { /* MT_QUASIT */
        .name = "quasit", .glyph = 'Q', .colour = DENTIST_GREEN,
        .exp = 15, .ac = 6, .hp_max = 10,
        .level = 3, .intelligence = 3, .speed = FAST, .size = SMALL,
        .flags = HEAD | HANDS | DEMON,
        .attacks = {
            { .type = ATT_BITE, .base = 3, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 66, .damage = DAM_DEC_DEX },
        }, .default_ai = MA_WANDER
    },
    { /* MT_RUST_MONSTER */
        .name = "rust monster", .glyph = 'R', .colour = AUTUMN_LEAF_ORANGE,
        .exp = 25, .ac = 6, .hp_max = 18,
        .level = 3, .intelligence = 3, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | METALLIVORE,
        .attacks = {
            { .type = ATT_BITE, .base = 3, .damage = DAM_PHYSICAL },
            { .type = ATT_TOUCH, .base = 1, .damage = DAM_RUST },
        }, .default_ai = MA_WANDER
    },
    { /* MT_ZOMBIE */
        .name = "zombie", .glyph = 'Z', .colour = OLIVE_GREY,
        .exp = 7, .ac = 2, .hp_max = 12,
        .level = 3, .intelligence = 3, .speed = VSLOW, .size = MEDIUM,
        .flags = HEAD | HANDS | UNDEAD | RES_SLEEP | RES_POISON | RES_CONF,
        .attacks = {
            { .type = ATT_BITE, .base = 2, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 2, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER, .sound = "groan"
    },
    { /* MT_ASSASSIN_BUG */
        .name = "assassin bug", .glyph = 'a', .colour = DEEP_ORANGE,
        .exp = 15, .ac = 1, .hp_max = 20,
        .level = 4, .intelligence = 3, .speed = FAST, .size = TINY,
        .flags = HEAD | RES_POISON,
        .attacks = {
            { .type = ATT_BITE, .base = 3, .damage = DAM_PHYSICAL },
            { .type = ATT_BITE, .base = 3, .damage = DAM_POISON },
        }, .default_ai = MA_WANDER
    },
    { /* MT_BUGBEAR */
        .name = "bugbear", .glyph = 'B', .colour = DEEP_ORANGE,
        .exp = 35, .gold_chance = 10, .gold = 400, .ac = 5, .hp_max = 20,
        .level = 4, .intelligence = 5, .speed = SLOW, .size = MEDIUM,
        .flags = HEAD | HANDS | INFRAVISION,
        .attacks = {
            { .type = ATT_BITE, .base = 5, .damage = DAM_PHYSICAL, .rand = 10 },
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_HELLHOUND */
        .name = "hell hound", .glyph = 'h', .colour = LAVA,
        .exp = 35, .ac = 5, .hp_max = 16,
        .level = 4, .intelligence = 6, .speed = FAST, .size = SMALL,
        .flags = HEAD | RES_FIRE | RES_MAGIC,
        .attacks = {
            { .type = ATT_BITE, .base = 2, .damage = DAM_PHYSICAL },
            { .type = ATT_BREATH, .base = 8, .damage = DAM_FIRE, .rand = 15 },
        }, .default_ai = MA_WANDER, .sound = "bark"
    },
    { /* MT_ICE_LIZARD */
        .name = "ice lizard", .glyph = 'i', .colour = CHALKY_BLUE_WHITE,
        .exp = 25, .ac = 4, .hp_max = 20,
        .level = 4, .intelligence = 6, .speed = SLOW, .size = MEDIUM,
        .flags = HEAD | SWIM | RES_COLD,
        .attacks = {
            { .type = ATT_CLAW, .base = 2, .damage = DAM_PHYSICAL },
            { .type = ATT_SLAM, .base = 14, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_CENTAUR */
        .name = "centaur", .glyph = 'C', .colour = AUTUMN_LEAF_BROWN,
        .exp = 45, .gold_chance = 50, .gold = 80, .ac = 6, .hp_max = 24,
        .level = 4, .intelligence = 10, .speed = FAST, .size = LARGE,
        .flags = HEAD | HANDS | PACK,
        .attacks = {
            { .type = ATT_KICK, .base = 6, .damage = DAM_PHYSICAL },
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_TROLL */
        .name = "troll", .glyph = 'T', .colour = SMOOTHIE_GREEN,
        .exp = 300, .gold_chance = 20, .gold = 400, .ac = 8, .hp_max = 50,
        .level = 5, .intelligence = 9, .speed = SLOW, .size = LARGE,
        .flags = HEAD | HANDS | REGENERATE,
        .attacks = {
            { .type = ATT_CLAW, .base = 5, .damage = DAM_PHYSICAL },
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER, .sound = "grunt"
    },
    { /* MT_YETI */
        .name = "yeti", .glyph = 'Y', .colour = GREY_GOOSE,
        .exp = 100, .gold_chance = 10, .gold = 200, .ac = 4, .hp_max = 35,
        .level = 5, .intelligence = 5, .speed = NORMAL, .size = LARGE,
        .flags = HEAD | HANDS | RES_COLD,
        .attacks = {
            { .type = ATT_CLAW, .base = 4, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_ELF */
        .name = "elf", .plural_name = "elves", .glyph = 'e', .colour = GREY_CLOUD,
        .exp = 35, .gold_chance = 50, .gold = 150, .ac = 6, .hp_max = 22,
        .level = 5, .intelligence = 15, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | HANDS | INFRAVISION,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_GELATINOUSCUBE */
        .name = "gelatinous cube", .glyph = 'g', .colour = ALABASTER_GREEN,
        .exp = 45, .ac = 1, .hp_max = 22,
        .level = 5, .intelligence = 3, .speed = XSLOW, .size = LARGE,
        .flags = METALLIVORE | RES_SLEEP | RES_POISON | RES_CONF,
        .attacks = {
            { .type = ATT_SLAM, .base = 1, .damage = DAM_ACID },
        }, .default_ai = MA_WANDER
    },
    { /* MT_WHITE_DRAGON */
        .name = "white dragon", .glyph = 'd', .colour = WHITE,
        .exp = 1000, .gold = 500, .ac = 8, .hp_max = 55,
        .level = 5, .intelligence = 16, .speed = NORMAL, .size = GIANT,
        .flags = HEAD | DRAGON | RES_COLD,
        .attacks = {
            { .type = ATT_BITE, .base = 4, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 4, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_METAMORPH */
        .name = "metamorph", .glyph = 'm', .colour = SILVER,
        .exp = 40, .ac = 3, .hp_max = 30,
        .level = 6, .intelligence = 3, .speed = NORMAL, .size = MEDIUM,
        .flags = 0,
        .attacks = {
            { .type = ATT_WEAPON, .base = 3, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_VORTEX */
        .name = "vortex", .plural_name = "vortexes", .glyph = 'v', .colour = SKY_BLUE,
        .exp = 55, .ac = 6, .hp_max = 30,
        .level = 6, .intelligence = 3, .speed = VFAST, .size = TINY,
        .flags = RES_SLEEP | RES_POISON | FLY | RES_ELEC | RES_CONF,
        .attacks = {
            { .type = ATT_SLAM, .base = 3, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_ZILLER */
        .name = "ziller", .glyph = 'z', .colour = WOOL_VIOLET,
        .exp = 35, .ac = 8, .hp_max = 30,
        .level = 6, .intelligence = 3, .speed = SLOW, .size = MEDIUM,
        .flags = HEAD | RES_CONF,
        .attacks = {
            { .type = ATT_CLAW, .base = 3, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 70, .damage = DAM_DEC_RND },
        }, .default_ai = MA_WANDER
    },
    { /* MT_VIOLET_FUNGUS */
        .name = "violet fungus", .plural_name = "violet fungi", .glyph = 'F', .colour = VIOLET,
        .exp = 100, .ac = 0, .hp_max = 38,
        .level = 6, .intelligence = 3, .speed = XSLOW, .size = MEDIUM,
        .flags = RES_SLEEP | RES_POISON | RES_CONF,
        .attacks = {
            { .type = ATT_SLAM, .base = 3, .damage = DAM_PHYSICAL },
            { .type = ATT_SLAM, .base = 4, .damage = DAM_POISON },
        }, .default_ai = MA_WANDER
    },
    { /* MT_WRAITH */
        .name = "wraith", .glyph = 'W', .colour = VAMPIRE_GREY,
        .exp = 325, .ac = 7, .hp_max = 30,
        .level = 6, .intelligence = 3, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS | UNDEAD | RES_SLEEP | RES_POISON | RES_CONF,
        .attacks = {
            { .type = ATT_TOUCH, .base = 50, .damage = DAM_DRAIN_LIFE },
        }, .default_ai = MA_WANDER
    },
    { /* MT_FORVALAKA */
        .name = "forvalaka", .glyph = 'f', .colour = IRIDIUM,
        .exp = 280, .ac = 4, .hp_max = 50,
        .level = 6, .intelligence = 7, .speed = DOUBLE, .size = MEDIUM,
        .flags = HEAD | UNDEAD | INFRAVISION | RES_POISON,
        .attacks = {
            { .type = ATT_BITE, .base = 5, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_LAMA_NOBE */
        .name = "lama nobe", .glyph = 'l', .colour = LIGHT_FUCHSIA,
        .exp = 80, .ac = 3, .hp_max = 35,
        .level = 7, .intelligence = 6, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD,
        .attacks = {
            { .type = ATT_BITE, .base = 3, .damage = DAM_PHYSICAL },
            { .type = ATT_GAZE, .base = 25, .damage = DAM_BLINDNESS },
        }, .default_ai = MA_WANDER
    },
    { /* MT_OSQUIP */
        .name = "osquip", .glyph = 'o', .colour = SAND,
        .exp = 100, .ac = 5, .hp_max = 35,
        .level = 7, .intelligence = 4, .speed = VFAST, .size = SMALL,
        .flags = HEAD | PACK,
        .attacks = {
            { .type = ATT_BITE, .base = 10, .damage = DAM_PHYSICAL, .rand = 15 },
        }, .default_ai = MA_WANDER
    },
    { /* MT_ROTHE */
        .name = "rothe", .glyph = 'r', .colour = AUTUMN_LEAF_BROWN,
        .exp = 250, .ac = 5, .hp_max = 50,
        .level = 7, .intelligence = 5, .speed = VFAST, .size = LARGE,
        .flags = HEAD | INFRAVISION | PACK,
        .attacks = {
            { .type = ATT_BITE, .base = 5, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 3, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_XORN */
        .name = "xorn", .glyph = 'X', .colour = PALE_RED,
        .exp = 300, .ac = 10, .hp_max = 60,
        .level = 7, .intelligence = 13, .speed = NORMAL, .size = MEDIUM,
        .flags = INFRAVISION | RES_COLD | RES_FIRE,
        .attacks = {
            { .type = ATT_BITE, .base = 6, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_VAMPIRE */
        .name = "vampire", .glyph = 'V', .colour = DRIED_BLOOD,
        .exp = 1000, .ac = 7, .hp_max = 50,
        .level = 7, .intelligence = 17, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS | FLY | UNDEAD | INFRAVISION | REGENERATE | RES_SLEEP | RES_POISON | RES_CONF,
        .attacks = {
            { .type = ATT_BITE, .base = 75, .damage = DAM_DRAIN_LIFE },
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_STALKER */
        .name = "invisible stalker", .glyph = 'I', .colour = ASH_GREY,
        .exp = 350, .ac = 7, .hp_max = 50,
        .level = 7, .intelligence = 14, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | FLY | INVISIBLE,
        .attacks = {
            { .type = ATT_SLAM, .base = 6, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_POLTERGEIST */
        .name = "poltergeist", .glyph = 'p', .colour = REGENT_GREY,
        .exp = 450, .ac = 6, .hp_max = 50,
        .level = 8, .intelligence = 5, .speed = NORMAL, .size = MEDIUM,
        .flags = FLY | SPIRIT | INVISIBLE | RES_SLEEP | RES_POISON,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER, .sound = "laugh"
    },
    { /* MT_DISENCHANTRESS */
        .name = "disenchantress", .plural_name = "disenchantresses", .glyph = 'q', .colour = PALE_TURQUOISE,
        .exp = 500, .ac = 7, .hp_max = 50,
        .level = 8, .intelligence = 5, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS | METALLIVORE,
        .attacks = {
            { .type = ATT_TOUCH, .damage = DAM_REM_ENCH },
        }, .default_ai = MA_WANDER
    },
    { /* MT_SHAMBLINGMOUND */
        .name = "shambling mound", .glyph = 's', .colour = GREEN_BROWN,
        .exp = 400, .ac = 8, .hp_max = 45,
        .level = 8, .intelligence = 6, .speed = VSLOW, .size = GIANT,
        .flags = RES_SLEEP | RES_POISON | RES_ELEC,
        .attacks = {
            { .type = ATT_SLAM, .base = 5, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_YELLOW_MOLD */
        .name = "yellow mold", .glyph = 'y', .colour = HONEY_YELLOW,
        .exp = 250, .ac = 0, .hp_max = 35,
        .level = 8, .intelligence = 3, .speed = XSLOW, .size = SMALL,
        .flags = RES_SLEEP | RES_POISON | RES_CONF,
        .attacks = {
            { .type = ATT_TOUCH, .base = 4, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_UMBER_HULK */
        .name = "umber hulk", .glyph = 'U', .colour = DARK_ORANGE,
        .exp = 600, .ac = 12, .hp_max = 65,
        .level = 8, .intelligence = 14, .speed = SLOW, .size = GIANT,
        .flags = HEAD | HANDS | INFRAVISION | RES_CONF,
        .attacks = {
            { .type = ATT_CLAW, .base = 7, .damage = DAM_PHYSICAL },
            { .type = ATT_GAZE, .base = 75, .damage = DAM_CONFUSION },
        }, .default_ai = MA_WANDER
    },
    { /* MT_GNOME_KING */
        .name = "gnome king", .glyph = 'G', .colour = CONIFER,
        .exp = 3000, .gold = 2000, .reroll_chance = 80, .ac = 11, .hp_max = 100,
        .level = 9, .intelligence = 18, .speed = NORMAL, .size = SMALL,
        .flags = HEAD | HANDS | RES_SLEEP,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_MIMIC */
        .name = "mimic", .glyph = 'M', .colour = AUTUMN_LEAF_BROWN,
        .exp = 99, .ac = 5, .hp_max = 55,
        .level = 9, .intelligence = 8, .speed = SLOW, .size = MEDIUM,
        .flags = MIMIC,
        .attacks = {
            { .type = ATT_SLAM, .base = 6, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_WATER_LORD */
        .name = "water lord", .glyph = 'w', .colour = DEEP_SEA_BLUE,
        .exp = 15000, .ac = 12, .hp_max = 150,
        .level = 9, .intelligence = 20, .speed = NORMAL, .size = LARGE,
        .flags = HEAD | NOBEHEAD | HANDS | RES_SLEEP | SWIM,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_PURPLE_WORM */
        .name = "purple worm", .glyph = 'P', .colour = DEEP_PURPLE,
        .exp = 15000, .ac = 12, .hp_max = 120,
        .level = 9, .intelligence = 3, .speed = VSLOW, .size = GARGANTUAN,
        .flags = HEAD | RES_POISON,
        .attacks = {
            { .type = ATT_BITE, .base = 11, .damage = DAM_PHYSICAL },
            { .type = ATT_STING, .base = 6, .damage = DAM_POISON },
        }, .default_ai = MA_WANDER
    },
    { /* MT_XVART */
        .name = "xvart", .glyph = 'x', .colour = CHALCEDONY_VIOLET,
        .exp = 1000, .ac = 11, .hp_max = 90,
        .level = 9, .intelligence = 13, .speed = NORMAL, .size = SMALL,
        .flags = HEAD | HANDS | INFRAVISION,
        .attacks = {
            { .type = ATT_WEAPON, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_BRONZE_DRAGON */
        .name = "bronze dragon", .glyph = 'D', .colour = GREEN_BROWN,
        .exp = 4000, .gold = 300, .ac = 10, .hp_max = 80,
        .level = 9, .intelligence = 16, .speed = NORMAL, .size = GIANT,
        .flags = HEAD | FLY | DRAGON,
        .attacks = {
            { .type = ATT_BITE, .base = 9, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 9, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_GREEN_DRAGON */
        .name = "green dragon", .glyph = 'D', .colour = GREEN_HAZE,
        .exp = 2500, .gold = 200, .ac = 12, .hp_max = 70,
        .level = 9, .intelligence = 15, .speed = NORMAL, .size = GIANT,
        .flags = HEAD | FLY | DRAGON | RES_POISON,
        .attacks = {
            { .type = ATT_BREATH, .base = 8, .damage = DAM_POISON },
            { .type = ATT_SLAM, .base = 25, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_SILVER_DRAGON */
        .name = "silver dragon", .glyph = 'D', .colour = SILVER,
        .exp = 10000, .gold = 700, .ac = 13, .hp_max = 100,
        .level = 10, .intelligence = 20, .speed = NORMAL, .size = GIANT,
        .flags = HEAD | FLY | DRAGON,
        .attacks = {
            { .type = ATT_BITE, .base = 12, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 12, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_PLATINUM_DRAGON */
        .name = "platinum dragon", .glyph = 'D', .colour = PLATINUM,
        .exp = 24000, .gold = 1000, .ac = 15, .hp_max = 130,
        .level = 11, .intelligence = 22, .speed = NORMAL, .size = GIANT,
        .flags = HEAD | FLY | DRAGON | RES_CONF,
        .attacks = {
            { .type = ATT_BITE, .base = 15, .damage = DAM_PHYSICAL },
            { .type = ATT_BREATH, .base = 15, .damage = DAM_MAGICAL, .rand = 30 },
        }, .default_ai = MA_WANDER
    },
    { /* MT_RED_DRAGON */
        .name = "red dragon", .glyph = 'D', .colour = BLOOD_RED,
        .exp = 14000, .gold = 800, .ac = 14, .hp_max = 110,
        .level = 11, .intelligence = 19, .speed = NORMAL, .size = GIANT,
        .flags = HEAD | FLY | DRAGON | RES_FIRE,
        .attacks = {
            { .type = ATT_BREATH, .base = 20, .damage = DAM_FIRE, .rand = 25 },
            { .type = ATT_CLAW, .base = 13, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_SPIRIT_NAGA */
        .name = "spirit naga", .glyph = 'N', .colour = BLUE_LOTUS,
        .exp = 20000, .ac = 16, .hp_max = 95,
        .level = 11, .intelligence = 23, .speed = FAST, .size = LARGE,
        .flags = HEAD | NOBEHEAD | FLY | SPIRIT | INFRAVISION | RES_SLEEP | RES_POISON | RES_CONF | RES_MAGIC,
        .attacks = {
            { .type = ATT_BITE, .base = 12, .damage = DAM_PHYSICAL },
            { .type = ATT_MAGIC, .base = 1, .damage = DAM_RANDOM },
        }, .default_ai = MA_WANDER
    },
    { /* MT_GREEN_URCHIN */
        .name = "green urchin", .glyph = 'u', .colour = TEALISH_GREEN,
        .exp = 5000, .ac = 17, .hp_max = 85,
        .level = 10, .intelligence = 3, .speed = SLOW, .size = SMALL,
        .flags = 0,
        .attacks = {
            { .type = ATT_STING, .base = 12, .damage = DAM_PHYSICAL },
            { .type = ATT_STING, .base = 50, .damage = DAM_BLINDNESS },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_I */
        .name = "type I demon lord", .glyph = '&', .colour = WOOL_VIOLET,
        .exp = 50000, .ac = 17, .hp_max = 140,
        .level = 12, .intelligence = 20, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 18, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 18, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_II */
        .name = "type II demon lord", .glyph = '&', .colour = CHALCEDONY_VIOLET,
        .exp = 75000, .ac = 18, .hp_max = 160,
        .level = 13, .intelligence = 21, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 18, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 18, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_III */
        .name = "type III demon lord", .glyph = '&', .colour = DEEP_MAGENTA,
        .exp = 100000, .ac = 19, .hp_max = 180,
        .level = 14, .intelligence = 22, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 18, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 18, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_IV */
        .name = "type IV demon lord", .glyph = '&', .colour = BLOOD_RED,
        .exp = 125000, .ac = 20, .hp_max = 200,
        .level = 15, .intelligence = 23, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 20, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 20, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_V */
        .name = "type V demon lord", .glyph = '&', .colour = RED_LILAC,
        .exp = 150000, .ac = 21, .hp_max = 220,
        .level = 16, .intelligence = 24, .speed = FAST, .size = MEDIUM,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 22, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 22, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_VI */
        .name = "type VI demon lord", .glyph = '&', .colour = SUNSET_RED,
        .exp = 175000, .ac = 22, .hp_max = 240,
        .level = 17, .intelligence = 25, .speed = FAST, .size = LARGE,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 24, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 24, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMONLORD_VII */
        .name = "type VII demon lord", .glyph = '&', .colour = DARK_RED,
        .exp = 200000, .ac = 23, .hp_max = 260,
        .level = 18, .intelligence = 26, .speed = FAST, .size = GIANT,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_POISON | RES_CONF | RES_MAGIC | RES_SLEEP,
        .attacks = {
            { .type = ATT_BITE, .base = 27, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 27, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_DEMON_PRINCE */
        .name = "demon prince", .glyph = '&', .colour = DRIED_BLOOD,
        .exp = 300000, .ac = 25, .hp_max = 345,
        .level = 25, .intelligence = 28, .speed = FAST, .size = GIANT,
        .flags = HEAD | NOBEHEAD | HANDS | FLY | INVISIBLE | INFRAVISION | DEMON | RES_FIRE | RES_SLEEP | RES_POISON | RES_CONF | RES_MAGIC,
        .attacks = {
            { .type = ATT_BITE, .base = 30, .damage = DAM_PHYSICAL },
            { .type = ATT_CLAW, .base = 30, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_WANDER
    },
    { /* MT_TOWN_PERSON */
        .name = "human", .glyph = '@', .colour = LAVENDER,
        .exp = 0, .ac = 0, .hp_max = 10,
        .level = 1, .intelligence = 10, .speed = NORMAL, .size = MEDIUM,
        .flags = HEAD | HANDS,
        .attacks = {
            { .type = ATT_SLAM, .base = 0, .damage = DAM_PHYSICAL },
        }, .default_ai = MA_CIVILIAN
    },
};

static inline monster_action_t monster_default_ai(monster *m);
static gboolean monster_player_visible(monster *m);
static gboolean monster_attack_available(monster *m, attack_t type);
static bool monster_weapon_wield(monster *m);
static gboolean monster_item_disenchant(monster *m, struct player *p);
static gboolean monster_item_rust(monster *m, struct player *p);
static gboolean monster_player_rob(monster *m, struct player *p, item_t item_type);

static position monster_find_next_pos_to(monster *m, position dest);
static position monster_move_wander(monster *m, struct player *p);
static position monster_move_attack(monster *m, struct player *p);
static position monster_move_confused(monster *m, struct player *p);
static position monster_move_flee(monster *m, struct player *p);
static position monster_move_serve(monster *m, struct player *p);
static position monster_move_civilian(monster *m, struct player *p);

static gboolean monster_breath_hit(const GList *traj,
        const damage_originator *damo,
        gpointer data1, gpointer data2);

monster *monster_new(monster_t type, position pos, gpointer leader)
{
    g_assert(type < MT_MAX && pos_valid(pos));

    monster *nmonster;
    item_t itype;      /* item type */

    /* check if supplied position is suitable for a monster */
    if (!map_pos_validate(game_map(nlarn, Z(pos)), pos, LE_MONSTER, false))
    {
        return NULL;
    }

    /* don't create genocided monsters */
    if (monster_is_genocided(type))
    {
        /* try to find a replacement for the demon prince */
        if (type == MT_DEMON_PRINCE)
            return monster_new(MT_DEMONLORD_I + rand_0n(7), pos, NULL);
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
            inv_add(&nmonster->inv, item_new_random(IT_GEM, false));
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
        while (!item_is_desirable(itype));

        inv_add(&nmonster->inv, item_new_by_level(itype, Z(pos)));
        break;

    case MT_TOWN_PERSON:
        /* initialize name counter */
        nmonster->number = rand_1n(40);
        break;

    default:
        /* no fancy stuff... */
        break;
    }

    /* generate a weapon if monster can use it */
    if (monster_attack_available(nmonster, ATT_WEAPON))
    {
        int weapon_count = 3;
        int wpns[weapon_count]; /* choice of weapon types */
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
            wpns[2] = WT_ELONGSWORD;
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
            wpns[1] = WT_LONGSWORD;
            wpns[2] = WT_2SWORD;
            break;

        default:
            wpns[0] = WT_DAGGER;
            wpns[1] = WT_SHORTSWORD;
            wpns[2] = WT_SPEAR;
            break;
        }

        /* focus on the weakest weapon on the set */
        int weapon_idx = levy_element(weapon_count, 0.5, 1.0);

        weapon = item_new(IT_WEAPON, wpns[weapon_idx]);
        item_new_finetouch(weapon);

        inv_add(&nmonster->inv, weapon);

        /* wield the new weapon */
        nmonster->eq_weapon = weapon;
    } /* finished initializing weapons */

    /* initialize mimics */
    if (monster_flags(nmonster, MIMIC))
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
        nmonster->unknown = true;
    }

    /* initialize AI */
    nmonster->action = monster_default_ai(nmonster);
    nmonster->player_pos = pos_invalid;
    nmonster->leader = leader;

    /* register monster with game */
    nmonster->oid = game_monster_register(nlarn, nmonster);

    /* set position */
    nmonster->pos = pos;

    /* link monster to tile */
    map_set_monster_at(game_map(nlarn, Z(pos)), pos, nmonster);

    /* add some members to the pack if we created a pack monster */
    if (monster_flags(nmonster, PACK) && !leader)
    {
        guint count = rand_1n(5);
        while (count > 0)
        {
            position mpos = map_find_space_in(game_map(nlarn, Z(pos)),
                                    rect_new_sized(pos, 4), LE_MONSTER, false);
            /* no space left? */
            if (!pos_valid(mpos)) break;

            /* valid position returned, place a pack member there */
            monster_new(type, mpos, nmonster->oid);
            count--;
        }
    }

    /* increment monster count */
    game_map(nlarn, Z(pos))->mcount++;

    return nmonster;
}

monster *monster_new_by_level(position pos)
{
    g_assert(pos_valid(pos));

    const int mlevel[] = {
        MT_KOBOLD,           // D1:   5
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
    int monster_id;

    if (nlevel == 0)
    {
        /* only town persons in town */
        monster_id = MT_TOWN_PERSON;
    }
    else
    {
        /* everything else in the caverns */
        int minstep = nlevel - 4;
        int maxstep = nlevel - 1;

        int monster_id_min;
        int monster_id_max;

        if (chance(2 * game_difficulty(nlarn)))
            maxstep += 2;
        else if (chance(7 * (game_difficulty(nlarn) + 1)))
            maxstep++;
        else if (chance(10))
            minstep--;

        if (minstep < 0)
            monster_id_min = MT_GIANT_BAT;
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
        while ((monster_id < 0)
                || (monster_id >= MT_MAX)
                || nlarn->monster_genocided[monster_id]
                || chance(monster_type_reroll_chance(monster_id)));
    }

    return monster_new(monster_id, pos, NULL);
}

void monster_destroy(monster *m)
{
    g_assert(m != NULL && m->type < MT_MAX);

    /* free effects */
    while (m->effects->len > 0)
    {
        gpointer effect_id = g_ptr_array_remove_index(m->effects, m->effects->len - 1);
        effect *e = game_effect_get(nlarn, effect_id);
        effect_destroy(e);
    }

    g_ptr_array_free(m->effects, true);

    /* free inventory */
    if (m->inv)
        inv_destroy(m->inv, true);

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
    cJSON_AddStringToObject(mval, "type", monster_t_string(monster_type(m)));
    cJSON_AddNumberToObject(mval, "oid", GPOINTER_TO_UINT(oid));
    cJSON_AddNumberToObject(mval, "hp_max", m->hp_max);
    cJSON_AddNumberToObject(mval, "hp", m->hp);
    cJSON_AddNumberToObject(mval,"pos", pos_val(m->pos));
    cJSON_AddNumberToObject(mval, "movement", m->movement);
    cJSON_AddStringToObject(mval, "action", monster_action_t_string(m->action));

    if (m->eq_weapon != NULL)
        cJSON_AddNumberToObject(mval, "eq_weapon",
                        GPOINTER_TO_UINT(m->eq_weapon->oid));

    if (m->number)
        cJSON_AddNumberToObject(mval, "number", m->number);

    if (m->leader)
        cJSON_AddNumberToObject(mval, "leader", GPOINTER_TO_UINT(m->leader));

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

    m->type = monster_t_value(cJSON_GetObjectItem(mser, "type")->valuestring);
    oid = cJSON_GetObjectItem(mser, "oid")->valueint;
    m->oid = GUINT_TO_POINTER(oid);
    m->hp_max = cJSON_GetObjectItem(mser, "hp_max")->valueint;
    m->hp = cJSON_GetObjectItem(mser, "hp")->valueint;
    pos_val(m->pos) = cJSON_GetObjectItem(mser, "pos")->valueint;
    m->movement = cJSON_GetObjectItem(mser, "movement")->valueint;
    m->action = monster_action_t_value(cJSON_GetObjectItem(mser, "action")->valuestring);

    if ((obj = cJSON_GetObjectItem(mser, "eq_weapon")))
        m->eq_weapon = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    if ((obj = cJSON_GetObjectItem(mser, "number")))
        m->number = obj->valueint;

    if ((obj = cJSON_GetObjectItem(mser, "leader")))
    {
        guint leader = obj->valueint;
        m->leader = GUINT_TO_POINTER(leader);
    }

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
    g_assert(m != NULL && m->type < MT_MAX);
    return m->hp_max;
}

int monster_hp(monster *m)
{
    g_assert(m != NULL && m->type < MT_MAX);
    return m->hp;
}

void monster_hp_inc(monster *m, int amount)
{
    g_assert(m != NULL && m->type < MT_MAX);
    m->hp = min(m->hp + amount, m->hp_max);
}

gpointer monster_oid(monster *m)
{
    g_assert (m != NULL);
    return m->oid;
}

position monster_pos(monster *m)
{
    g_assert(m != NULL && m->type < MT_MAX);
    return m->pos;
}

static int monster_map_element(monster *m)
{
    if (monster_type(m) == MT_XORN)
        return LE_XORN;

    if (monster_flags(m, FLY))
        return LE_FLYING_MONSTER;

    if (monster_flags(m, SWIM))
        return LE_SWIMMING_MONSTER;

    return LE_MONSTER;
}

int monster_valid_dest(map *m, position pos, int map_elem)
{
    /* only civilians use LE_GROUND and can't move through the player */
    if (map_elem == LE_GROUND && pos_identical(pos, nlarn->p->pos))
        return false;

    switch (map_tiletype_at(m, pos))
    {
    case LT_WALL:
        return (map_elem == LE_XORN);

    case LT_DEEPWATER:
        if (map_elem == LE_SWIMMING_MONSTER)
            return true;
        // else fall through
    case LT_LAVA:
        return (map_elem == LE_FLYING_MONSTER);

    default:
        /* the map tile must be passable */
        return map_pos_passable(m, pos);
    }
}

int monster_pos_set(monster *m, map *mp, position target)
{
    g_assert(m != NULL && mp != NULL && pos_valid(target));

    if (map_pos_validate(mp, target, monster_map_element(m), false))
    {
        /* remove current reference to monster from tile */
        map_set_monster_at(monster_map(m), m->pos, NULL);

        /* set new position */
        m->pos = target;

        /* set reference to monster on tile */
        map_set_monster_at(mp, target, m);

        return true;
    }

    return false;
}

monster_t monster_type(monster *m)
{
    g_assert(m != NULL);
    return m->type;
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
        return false;

    return fov_get(nlarn->p->fv, m->pos);
}

gboolean monster_in_sight(monster *m)
{
    g_assert (m != NULL);

    /* player is blind */
    if (player_effect(nlarn->p, ET_BLINDNESS))
        return false;

    /* different level */
    if (Z(m->pos) != Z(nlarn->p->pos))
        return false;

    /* invisible monster, player has no infravision */
    if (monster_flags(m, INVISIBLE) && !player_effect(nlarn->p, ET_INFRAVISION))
        return false;

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

const char* monster_type_plural_name(monster_t mt, const int count)
{
    if (count > 1)
    {
        if (monster_data[mt].plural_name == NULL)
        {
            /* need a static buffer to return to calling functions */
            static char buf[61] = { 0 };
            g_snprintf(buf, 60, "%ss", monster_type_name(mt));
            return buf;
        }
        else
        {
            return monster_data[mt].plural_name;
        }
    }

    return monster_type_name(mt);
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
    if (monster_flags(m, MIMIC) && inv_length(m->inv) > 0)
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
                item *i = inv_get(m->inv, 0);
                // Drop all non-weapon items, all weapons picked up by the
                // monster, all unique weapons, but only one third of the
                // starting weapons.
                if ((i->type != IT_WEAPON) || i->picked_up || weapon_is_unique(i) || chance(33)) {
                    inv_add(floor, i);
                    // ensure the flag is always reset
                    i->picked_up = false;
                } else {
                    item_destroy(i);
                }

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
    case LS_CAVERNS_EXIT:
        target = LS_CAVERNS_ENTRY;
        what = "through";
        break;

    case LS_CAVERNS_ENTRY:
        target = LS_CAVERNS_EXIT;
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
        npos = map_find_space(l, LE_MONSTER, false);
    }

    /* validate new position */
    if (pos_identical(nlarn->p->pos, npos))
    {
        /* player is standing at the target position */
        how = "squeezes past";
        npos = map_find_space_in(l, rect_new_sized(npos, 1), LE_MONSTER, false);
    }
    else

    if (!map_pos_validate(l, npos, LE_MONSTER, false))
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
        log_add_entry(nlarn->log, "The %s %s %s %s.", monster_get_name(m),
                      how, what, so_get_desc(target));
    }
}

void monster_move(gpointer *oid __attribute__((unused)), monster *m, game *g)
{
    /* monster's new position */
    position m_npos;

    /* expire summoned monsters */
    if (monster_action(m) == MA_SERVE
            && !monster_effect(m, ET_CHARM_MONSTER))
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
                                  monster_flags(m, FLY)
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
                             || (Z(mpos) == MAP_CMAX && Z(g->p->pos) == 0)
                            );
    if (!map_adjacent)
        return;

    // Corrode items engulfed by gelatinuous cubes every three turns
    if (MT_GELATINOUSCUBE == monster_type(m) && game_turn(nlarn) % 3 == 0)
        inv_erode(&m->inv, IET_CORRODE, monster_in_sight(m), NULL);

    // Update the monster's knowledge of player's position.
    if (monster_player_visible(m)
            || (player_effect(g->p, ET_AGGRAVATE_MONSTER)
                && pos_distance(m->pos, g->p->pos) < 15))
    {
        monster_update_player_pos(m, g->p->pos);
    }

    /* add the monster's speed to the monster's movement points */
    m->movement += monster_speed(m);

    /* let the monster make a move as long it has movement points left */
    while (m->movement >= NORMAL)
    {
        /* reduce the monster's movement points */
        m->movement -= NORMAL;

        /* update monsters action */
        if (monster_update_action(m, MA_NONE) && monster_in_sight(m))
        {
            /* the monster has chosen a new action and the player
               can see the new action, so let's describe it */

            if (m->action == MA_ATTACK && monster_sound(m))
            {
                const char *sound = monster_sound(m);
                log_add_entry(g->log, "The %s %s%ss!",
                              monster_name(m), sound,
                              sound[strlen(sound) - 1] == 's' ? "e": "");
            }
            else if (m->action == MA_FLEE)
            {
                log_add_entry(g->log, "The %s turns to flee!",
                        monster_get_name(m));
            }
        }

        /* let the monster have a look at the items at it's current position
           if it chose to pick up something, the turn is over */
        if (monster_items_pickup(m))
            return;

        /* let the monster equip a weapon:
           if it returns true, the turn is over */
        if (monster_attack_available(m, ATT_WEAPON) &&
                inv_length_filtered(m->inv, item_filter_weapon) > 1 &&
                monster_weapon_wield(m))
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
            else if ((target_st == LS_CLOSEDDOOR) && monster_flags(m, HANDS))
            {
                /* dim-witted or confused monster are unable to open doors */
                if (monster_int(m) < 4 || monster_effect_get(m, ET_CONFUSION))
                {
                    /* notify the player if the door is visible */
                    if (monster_in_sight(m))
                    {
                        log_add_entry(g->log, "The %s bumps into the door.",
                                      monster_get_name(m));
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
                                      monster_get_name(m));
                    }
                }
            }

            /* set the monsters new position */
            else
            {
                /* check if the new position is valid for this monster */
                if (map_pos_validate(mmap, m_npos, monster_map_element(m), false))
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
                                        monster_get_name(m), mt_get_desc(nle));
                            }
                            break;

                        case LT_LAVA:
                        case LT_DEEPWATER:
                            if (monster_in_sight(m)) {
                                log_add_entry(g->log, "The %s sinks into %s.",
                                        monster_get_name(m), mt_get_desc(nle));
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
    } /* while movement >= NORMAL */

    /* increment count of turns since when player was last seen */
    if (m->lastseen) m->lastseen++;
}

void monster_polymorph(monster *m)
{
    g_assert (m != NULL);

    /* make sure mimics never leave the mimicked item behind */
    if (monster_flags(m, MIMIC) && inv_length(m->inv) > 0)
    {
        inv_del(&m->inv, 0);
    }

    const map_element_t old_elem = monster_map_element(m);
    do
    {
        m->type = rand_1n(MT_DEMON_PRINCE);
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
        return false;

    /* monsters affected by levitation can't pick up stuff */
    if (monster_effect(m, ET_LEVITATION))
        return false;

    gboolean pick_up = false;
    inventory **tinv = map_ilist_at(monster_map(m), m->pos);
    for (guint idx = 0; idx < inv_length(*tinv); idx++)
    {
        item *it = inv_get(*tinv, idx);
        item_t typ = it->type;
        const char *pickup_desc = "The %s picks up %s.";

        /*  rust monster eats metal stuff */
        if (m->type == MT_RUST_MONSTER && IM_IRON == item_material(it))
        {
            if (monster_in_sight(m))
            {
                gchar *buf = item_describe(it,
                        player_item_identified(nlarn->p, it), false, false);

                log_add_entry(nlarn->log, "The %s touches %s with its antennae.",
                        monster_get_name(m), buf);
                g_free(buf);
            }

            item_erode(tinv, it, IET_RUST, monster_in_sight(m));

            return true;
        }

        if (m->type == MT_LEPRECHAUN && ((typ == IT_GEM) || (typ == IT_GOLD)))
        {
            /* leprechauns collect treasures */
            pick_up = true;
        }
        else if (it->type == IT_WEAPON && monster_attack_available(m, ATT_WEAPON))
        {
            /* monster can attack with weapons */
            if (m->eq_weapon == NULL)
                pick_up = true;
            /* compare this weapon with the weapon the monster wields */
            else if (weapon_damage(m->eq_weapon) < weapon_damage(it))
                pick_up = true;
        }
        else if (m->type == MT_GELATINOUSCUBE)
        {
            /* gelatinous cubes engulf all items */
            pick_up = true;
            pickup_desc = "The %s engulfs %s.";
        }

        if (pick_up)
        {
            // The monster will picked up the item.
            if (monster_in_sight(m))
            {
                gchar *buf = item_describe(it, player_item_identified(nlarn->p, it),
                                           false, false);

                log_add_entry(nlarn->log, pickup_desc,
                        monster_get_name(m), buf);
                g_free(buf);
            }

            inv_del_element(tinv, it);
            inv_add(&m->inv, it);
            // flag the item as picked up to ensure we drop it on death
            it->picked_up = true;

            /* finish this turn after picking up an item */
            return true;
        } /* end if pick_up */
    } /* end foreach item */

    return false;
}

guint monster_attack_count(monster *m)
{
    guint count = 0;

    while (count < G_N_ELEMENTS(monster_data[m->type].attacks)
            && monster_data[m->type].attacks[count].type != ATT_NONE)
    {
        count++;
    }

    return count;
}

attack monster_attack(monster *m, guint num)
{
    g_assert (m != NULL && num <= monster_attack_count(m));

    return monster_data[m->type].attacks[num - 1];
}

static int monster_breath_attack(monster *m, player *p, attack att)
{
    g_assert(att.type == ATT_BREATH);

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
                   monster_breath_hit, dam, NULL, true,
                   monster_breath_data[att.damage].glyph,
                   monster_breath_data[att.damage].fg, true);

    /* the damage is copied in monster_breath_hit(), thus destroy the
       original damage here */
    damage_free(dam);

    return false;
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

    map *mmap = game_map(nlarn, Z(m->pos));

    /* the player is invisible and the monster bashes into thin air */
    if (!pos_identical(m->player_pos, p->pos))
    {
        if (!map_is_monster_at(mmap, p->pos) && monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s bashes into thin air.",
                    monster_get_name(m));
        }

        m->lastseen++;

        return;
    }

    /* player is invisible and monster tries to hit player */
    if (player_effect(p, ET_INVISIBILITY) && !(monster_flags(m, INFRAVISION)
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

    /* choose a random attack type */
    attack att = monster_attack(m, rand_1n(monster_attack_count(m) + 1));

    /* No attack has been found. Return to calling function. */
    if (att.type == ATT_NONE) return;

    /* handle breath attacks separately */
    if (att.type == ATT_BREATH)
    {
        monster_breath_attack(m, p, att);
        return;
    }

    /* generate damage */
    damage *dam = damage_new(att.damage, att.type,
                        modified_attack_amount(att.base, att.damage),
                        DAMO_MONSTER, m);

    /* deal with random damage (spirit naga) */
    if (dam->type == DAM_RANDOM)
    {
        dam->type = rand_1n(DAM_MAX);

        // we need to adjust the damage amount for the different attack types
        if (dam->type < DAM_BLINDNESS)
        {
            // these expect the amount of removed HPs
            dam->amount = rand_m_n(15, 35);
        }
        else if (dam->type < DAM_STEAL_ITEM)
        {
            // these expect a probability
            dam->amount = 50;
        }
    }

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
                        + 2 * ((monster_size(m) - MEDIUM)) / 25;
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
    if (player_effect(p, ET_SPIRIT_PROTECTION) && monster_flags(m, SPIRIT))
    {
        if (dam->type == DAM_PHYSICAL)
        {
            /* halve physical damage */
            dam->amount >>= 1;
        }
        else
        {
            log_add_entry(nlarn->log, "The %s %s you, but nothing happens.",
                monster_get_name(m), monster_attack_verb[att.type]);

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
            monster_pos_set(m, mmap, map_find_space(mmap, LE_MONSTER, false));
        }

        damage_free(dam);
        break;

    case DAM_RUST:
        monster_item_rust(m, p);
        p->attacked = true;
        damage_free(dam);
        break;

    case DAM_REM_ENCH:
        monster_item_disenchant(m, p);
        p->attacked = true;
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
        return true;
    }
    if (att.type != ATT_BREATH)
        return false;

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
        gboolean monster_bleeds = monster_flags(m, HEAD)
            && (!monster_flags(m, UNDEAD))
            && (!monster_flags(m, DEMON));

        if (dam->amount < 1 && monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s isn't hurt.",
                    monster_get_name(m));
        }
        else if(dam->amount > 0 && monster_bleeds)
        {
            position spill_pos = pos_invalid;
            // FIXME: this needs to be extended once we enable others
            // to attack monsters
            if (p)
            {
                // spill blood in the direction of the blow
                direction dir = pos_direction(p->pos, monster_pos(m));
                spill_pos = pos_move(monster_pos(m), dir);
            }
            else
            {
                // spill blood at the monster's position
                spill_pos = monster_pos(m);
            }

            map_spill_set(game_map(nlarn, Z(monster_pos(m))), spill_pos, BLOOD_RED);
        }
        break;

    case DAM_MAGICAL:
        if (monster_flags(m, RES_MAGIC))
        {
            dam->amount /= monster_level(m);
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s %sresists the magic.",
                        monster_get_name(m), dam->amount > 0 ? "partly " : "");
            }
        }
        break;

    case DAM_FIRE:
        if (monster_flags(m, RES_FIRE))
        {
            /*
             * The monster's fire resistance reduces the damage taken
             * by 10% per monster level
             */
            dam->amount -= (guint)(((float)dam->amount / 100) *
                 /* prevent uint wrap around for monsters with level > 10 */
                 (min(monster_level(m), 10) * 10));
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s %sresists the flames.",
                        monster_get_name(m), dam->amount > 0 ? "partly " : "");
            }
        }
        break;

    case DAM_COLD:
        if (monster_flags(m, RES_COLD))
        {
            dam->amount = 0;
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s loves the cold!",
                        monster_get_name(m));
            }
        }
        break;

    case DAM_WATER:
        if (monster_flags(m, SWIM))
            dam->amount = 0;
        break;

    case DAM_ELECTRICITY:
        if (monster_flags(m, RES_ELEC))
        {
            dam->amount = 0;
            if (monster_in_sight(m))
            {
                log_add_entry(nlarn->log, "The %s is not affected!",
                        monster_get_name(m));
            }
        }
        /* double damage for flying monsters */
        else if (monster_flags(m, FLY) || monster_effect(m, ET_LEVITATION))
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
                                    false, false);

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
                        log_add_entry(nlarn->log, "The %s turns into a %s!",
                                old_name, monster_get_name(m));
                    }
                    else if (seen_old)
                    {
                        log_add_entry(nlarn->log, "The %s vanishes!",
                                        old_name);
                    }
                    else
                    {
                        log_add_entry(nlarn->log, "A %s suddenly appears!",
                                monster_get_name(m));
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
        return true;
    }

    /* handle some easy action updates before the more difficult decisions */
    switch (m->action)
    {
        case MA_SERVE:     /* once servant, forever servant */
        case MA_CIVILIAN:  /* town people never change their behaviour */
        case MA_CONFUSION: /* confusion is removed by monster_effect_del() */
        case MA_REMAIN:    /* status set by hold monster/sleep/being trapped */
            return false;
            break;

        default:
            /* continue to evaluate... */
            break;
    }

    mtime  = monster_int(m) + 25 + (5 * game_difficulty(nlarn));
    low_hp = (m->hp < (monster_hp_max(m) / 4 ));

    if (monster_flags(m, MIMIC) && m->unknown)
    {
        /* stationary monsters */
        naction = MA_REMAIN;
    }
    else if ((low_hp && (monster_int(m) > 4)) || monster_effect(m, ET_SCARED))
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
        return true;
    }

    return false;
}

void monster_update_player_pos(monster *m, position ppos)
{
    g_assert (m != NULL);

    /* Not for civilians or servants: the first don't care,
       the latter just know. This allows to use player_pos
       and lastseen for other purposes. */
    monster_action_t ma = monster_action(m);
    if (ma == MA_SERVE || ma == MA_CIVILIAN)
        return;

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
    if (monster_flags(m, REGENERATE) && (m->hp < monster_hp_max(m)))
    {
        // regenerate every (10 - difficulty) turns,
        // or every turn starting on difficulty 10
        if ( (difficulty >= 10) || (gtime % (10 - difficulty) == 0) )
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
            return false;
        }
    }

    return true;
}

item *get_mimic_item(monster *m)
{
    g_assert(m && monster_flags(m, MIMIC));

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
                                        false, false);

        g_string_append_printf(desc, "You see %s there", item_desc);
        g_free(item_desc);

        return g_string_free(desc, false);
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
        g_string_append_printf(desc, " %s(%d/%d hp)",
            m->leader ? "(pack member) " : "",
            m->hp, m->hp_max);
    }

    if (m->eq_weapon != NULL)
    {
        /* describe the weapon the monster wields */
        gchar *weapon_desc = item_describe(m->eq_weapon,
                        player_item_known(nlarn->p, m->eq_weapon),
                        true, false);

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

    return g_string_free(desc, false);
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
        return monster_data[m->type].glyph;
    }
}

colour monster_color(monster *m)
{
    g_assert (m != NULL);

    if (m->unknown && inv_length(m->inv) > 0)
    {
        item *it = inv_get(m->inv, 0);
        return item_colour(it);
    }
    else
    {
        return monster_data[m->type].colour;
    }
}

void monster_genocide(monster_t monster_id)
{
    GList *mlist;

    g_assert(monster_id < MT_MAX);

    nlarn->monster_genocided[monster_id] = true;
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
    g_assert(monster_id < MT_MAX);
    return nlarn->monster_genocided[monster_id];
}

effect *monster_effect_add(monster *m, effect *e)
{
    g_assert(m != NULL && e != NULL);
    gboolean vis_effect = false;

    if (e->type == ET_SLEEP && monster_flags(m, RES_SLEEP))
    {
        /* the monster is resistant to sleep */
        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_POISON && monster_flags(m, RES_POISON))
    {
        /* the monster is poison resistant */
        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_LEVITATION && monster_flags(m, FLY))
    {
        /* levitation has no effect on flying monsters */
        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_CONFUSION && monster_flags(m, RES_CONF))
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
                    vis_effect = true;
            }

            break;

        case ET_MAX_HP:
            if (m->hp < m->hp_max)
            {
                m->hp = m->hp_max;
                vis_effect = true;
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

        /* charm monster turns monsters into servants */
        if (e && e->type == ET_CHARM_MONSTER) {
            monster_update_action(m, MA_SERVE);
        }

        /* no action if monster is held or sleeping */
        if (e && (e->type == ET_HOLD_MONSTER || e->type == ET_SLEEP || e->type == ET_TRAPPED))
        {
            monster_update_action(m, MA_REMAIN);
        }
    }

    /* show message if monster is visible */
    if (e && monster_in_sight(m)
        && effect_get_msg_m_start(e)
        && (e->turns > 0 || vis_effect))
    {
        log_add_entry(nlarn->log, effect_get_msg_m_start(e),
                      monster_get_name(m));
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
        log_add_entry(nlarn->log, effect_get_msg_m_stop(e), monster_get_name(m));
    }

    if ((result = effect_del(m->effects, e)))
    {
        /* if confusion or charm is finished, set the AI back to the default */
        if (e->type == ET_CONFUSION || e->type == ET_CHARM_MONSTER) {
            monster_update_action(m, monster_default_ai(m));
        }

        /* end of holding, sleeping or being trapped */
        if (e->type == ET_HOLD_MONSTER || e->type == ET_SLEEP || e->type == ET_TRAPPED)
        {
            /* only reset the AI when no other "idling" effect is left */
            if (!monster_effect(m, ET_HOLD_MONSTER)
                    && !monster_effect(m, ET_SLEEP)
                    && !monster_effect(m, ET_TRAPPED))
            {
                monster_update_action(m, monster_default_ai(m));
            }
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
        gpointer effect_id = g_ptr_array_index(m->effects, idx);
        effect *e = game_effect_get(nlarn, effect_id);

        if (e->type == ET_TRAPPED)
        {
            /* if the monster is incapable of movement don't decrease
               trapped counter */
            if (monster_effect(m, ET_HOLD_MONSTER)
                    || monster_effect(m, ET_SLEEP))
            {
                idx++;
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

static inline monster_action_t monster_default_ai(monster *m)
{
    return monster_data[m->type].default_ai;
}

static gboolean monster_player_visible(monster *m)
{
    /* monster is blinded */
    if (monster_effect(m, ET_BLINDNESS))
        return false;

    /* FIXME: this ought to be different per monster type */
    int monster_visrange = 7;

    if (player_effect(nlarn->p, ET_STEALTH) || monster_effect(m, ET_TRAPPED))
        // If the player is stealthy, monsters will only recognize them when
        // standing next to them. Also being trapped reduces the range of
        // visibility to one surrounding tile.
        monster_visrange = 1;

    /* determine if the monster can see the player */
    if (pos_distance(monster_pos(m), nlarn->p->pos) > monster_visrange)
        return false;

    /* check if the player is invisible and if the monster has infravision */
    if (player_effect(nlarn->p, ET_INVISIBILITY)
        && !(monster_flags(m, INFRAVISION) || monster_effect(m, ET_INFRAVISION)))
        return false;

    /* determine if player's position is visible from monster's position */
    return map_pos_is_visible(monster_map(m), m->pos, nlarn->p->pos);
}

static gboolean monster_attack_available(monster *m, attack_t type)
{
    gboolean available = false;
    int pos = 1;
    int c = monster_attack_count(m);

    while (pos <= c)
    {
        attack att = monster_attack(m, pos);

        if (att.type == type)
        {
            available = true;
            break;
        }

        pos++;
    }

    return available;
}

static bool monster_weapon_wield(monster *m)
{
    item *orig_weapon = m->eq_weapon;

    for (guint idx = 0; idx < inv_length_filtered(m->inv, item_filter_weapon); idx++)
    {
        item *w = inv_get_filtered(m->inv, idx, item_filter_weapon);
        /* compare this weapon with the weapon the monster wields */
        if (!(m->eq_weapon) || weapon_damage(m->eq_weapon) < weapon_damage(w)) {
            /* FIXME: weapon effects */
            m->eq_weapon = w;
        }
    }

    if (orig_weapon == m->eq_weapon)
    {
        /* nothing changed */
        return false;
    }

    /* show message if the monster swapped the weapon and is visible */
    if (monster_in_sight(m))
    {
        gchar *buf = item_describe(m->eq_weapon, player_item_identified(nlarn->p,
                                m->eq_weapon), true, false);

        log_add_entry(nlarn->log, "The %s wields %s.", monster_get_name(m), buf);
        g_free(buf);
    }

    return true;
}

static gboolean monster_item_disenchant(monster *m, struct player *p)
{
    item *it;

    g_assert (m != NULL && p != NULL);

    /* disenchant random item */
    if (!inv_length(p->inventory))
    {
        /* empty inventory */
        return false;
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
                                    (it->count == 1), true);

        desc[0] = g_ascii_toupper(desc[0]);
        log_add_entry(nlarn->log, "%s resist%s the attack.",
                      desc, (it->count == 1) ? "s" : "");

        it->blessed_known = true;
        g_free(desc);
        return true;
    }
    log_add_entry(nlarn->log, "You feel a sense of loss.");

    if (item_is_optimizable(it->type))
    {
        item_disenchant(it);
    }
    else
    {
        player_item_destroy(p, it);
    }

    return true;
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

    g_assert(m != NULL && p != NULL);

    /* get a random piece of armour to damage */
    if ((it = player_get_random_armour(p, false)))
    {
        gchar *buf = item_describe(*it,
                player_item_identified(nlarn->p, *it), false, true);

        log_add_entry(nlarn->log, "The %s touches %s with its antennae.",
                monster_get_name(m), buf);
        g_free(buf);

        *it = item_erode(&p->inventory, *it, IET_RUST, true);
        return true;
    }
    else
    {
        log_add_entry(nlarn->log, "Nothing happens.");
        return false;
    }
}

static gboolean monster_player_rob(monster *m, struct player *p, item_t item_type)
{
    item *it = NULL;

    g_assert (m != NULL && p != NULL);

    /* if player has a device of no theft abort the theft */
    if (player_effect(p, ET_NOTHEFT))
        return false;

    /* Leprechaun robs only gold */
    if (item_type == IT_GOLD)
    {
        /* get amount of gold pieces carried by the player */
        guint player_gold = player_get_gold(p);

        if (player_gold > 0)
        {
            /* steel gold carried by the player */
            it = item_new(IT_GOLD, rand_1n(1 + (player_gold >> 1)));
            player_remove_gold(p, it->count);

            log_add_entry(nlarn->log, "The %s picks your pocket. " \
                          "Your purse feels lighter.", monster_get_name(m));
        }
        else
        {
            /* grab gold at player's position */
            inventory **floor = map_ilist_at(monster_map(m), p->pos);
            if (inv_length_filtered(*floor, item_filter_gold))
            {
                it = inv_get_filtered(*floor, 0, item_filter_gold);
                inv_del_element(floor, it);

                if (monster_in_sight(m))
                {
                    log_add_entry(nlarn->log,
                            "The %s picks up some gold at your feet.",
                            monster_get_name(m));
                }
            }
        }
    }
    else if (item_type == IT_ALL) /* must be the nymph */
    {
        if (inv_length(p->inventory))
        {
            gboolean was_equipped = false;

            it = inv_get(p->inventory, rand_0n(inv_length(p->inventory)));
            gchar *buf = item_describe(it, player_item_known(p, it), false, false);

            if ((was_equipped = player_item_is_equipped(p, it)))
            {
                if (it->cursed)
                {
                    /* cursed items can't be stolen.. */
                    log_add_entry(nlarn->log, "The %s tries to steal %s but fails.",
                                  monster_get_name(m), buf);

                    it->blessed_known = true;
                    g_free(buf);

                    /* return true as there are things to steal */
                    return true;
                }

                player_item_unequip(p, NULL, it, true);
            }

            if (it->count > 1)
            {
                /* the player has multiple items. Steal only one. */
                it = item_split(it, rand_1n(it->count));
                g_free(buf);
                buf = item_describe(it, player_item_known(p, it), false, false);
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
        return true;
    }
    else
    {
        log_add_entry(nlarn->log, "The %s couldn't find anything to steal.",
                      monster_get_name(m));

        return false;
    }
}

static char *monster_get_fortune(const char *fortune_file)
{
    /* array of pointers to fortunes */
    static GPtrArray *fortunes = NULL;

    if (!fortunes)
    {
        /* read in the fortunes */
        char buffer[80];
        FILE *fortune_fd;

        /* open the file */
        fortune_fd = fopen(fortune_file, "r");
        if (fortune_fd == NULL)
        {
            /* can't find file */
            return "Help me! I can't find the fortune file!";
        }

        fortunes = g_ptr_array_new();

        /* read in the entire fortune file */
        while((fgets(buffer, 79, fortune_fd)))
        {
            /* replace EOL with \0 */
            size_t len = (size_t)(strchr(buffer, '\n') - (char *)&buffer);
            buffer[len] = '\0';

            /* keep the line */
            char *tmp = g_malloc((len + 1) * sizeof(char));
            memcpy(tmp, &buffer, (len + 1));
            g_ptr_array_add(fortunes, tmp);
        }

        fclose(fortune_fd);
    }

    return g_ptr_array_index(fortunes, rand_0n(fortunes->len));
}

static position monster_find_next_pos_to(monster *m, position dest)
{
    g_assert(m != NULL);
    g_assert(pos_valid(dest));

    /* next position */
    position npos = monster_pos(m);

    /* find the next step in the direction of dest */
    path *path = path_find(monster_map(m), monster_pos(m), dest,
                           monster_map_element(m));

    if (path && !g_queue_is_empty(path->path))
    {
        path_element *el = g_queue_pop_head(path->path);
        npos = el->pos;
    }

    /* clean up */
    if (path) path_destroy(path);

    return npos;
}

static position monster_move_wander(monster *m, struct player *p __attribute__((unused)))
{
    int tries = 0;
    position npos;

    if (m->leader)
    {
        /* monster is part of a pack */
        monster *leader = game_monster_get(nlarn, m->leader);

        if (!leader)
        {
            /* is seems that the leader was killed.
               From now on, wander aimlessly */
            m->leader = NULL;
        } else {
            /* stay close to the pack leader */
            if (pos_distance(monster_pos(m), monster_pos(leader)) > 4)
                return monster_find_next_pos_to(m, monster_pos(leader));
        }
    }

    do
    {
        npos = pos_move(m->pos, rand_1n(GD_MAX));
        tries++;
    }
    while (tries < GD_MAX
            && !map_pos_validate(monster_map(m), npos, monster_map_element(m),
                                 false));

    /* new position has not been found, reset to current position */
    if (tries == GD_MAX) npos = monster_pos(m);

    return npos;
}

static position monster_move_attack(monster *m, struct player *p)
{
    position npos = monster_pos(m);

    /* monster is standing next to player */
    if (pos_adjacent(monster_pos(m), m->player_pos) && (m->lastseen == 1))
    {
        monster_player_attack(m, p);

        /* monster's position might have changed (teleport) */
        if (!pos_identical(npos, monster_pos(m)))
            log_add_entry(nlarn->log, "The %s vanishes.", monster_name(m));

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
        case LS_CAVERNS_ENTRY:
            newmap = Z(m->pos) + 1;
            break;

        case LS_STAIRSUP:
        case LS_CAVERNS_EXIT:
            newmap = Z(m->pos) - 1;
            break;

        case LS_ELEVATORDOWN:
            /* move into the volcano from the town */
            newmap = MAP_CMAX + 1;
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
    npos = monster_find_next_pos_to(m, m->player_pos);

    /* No path found. Stop following player */
    if (!pos_valid(npos)) m->lastseen = 0;

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
                             false)
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
    /* If a new position cannot be found, keep the current position */
    position npos = m->pos;

    /* generate a fov structure if not yet available */
    if (!m->fv)
        m->fv = fov_new();

    /* calculate the monster's fov */
    /* the monster gets a fov radius of 6 for now*/
    fov_calculate(m->fv, monster_map(m), m->pos, 6,
                  monster_flags(m, INFRAVISION)
                  || monster_effect(m, ET_INFRAVISION));

    /* a good servant always knows the masters position */
    if (pos_distance(monster_pos(m), p->pos) > 5)
    {
         /* if the distance to the player is too large, follow */
        npos = monster_find_next_pos_to(m, p->pos);
    }
    else
    {
        /* look for worthy foes */
        /* TODO: implement */
    }

    return npos;
}

static position monster_move_civilian(monster *m, struct player *p)
{
    position npos = m->pos;

    /* civilians will pick a random location on the map, travel and remain
       there for the number of turns that is determined by their town person
       number. They use the player_pos attribute to store that location. */

    if (!pos_valid(m->player_pos))
    {
        /* No target set -> find a new location to travel to.
           Civilians stay inside the town area. */
        rectangle town = rect_new(3, 4, MAP_MAX_X - 25, MAP_MAX_Y - 3);
        do
        {
            /* Ensure that the townsfolk do not loiter in locations
               important for the player. */
            m->player_pos = map_find_space_in(monster_map(m), town, LE_GROUND, false);
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
        npos = monster_find_next_pos_to(m, m->player_pos);

        if (pos_identical(npos, m->player_pos))
        {
            /* arrived at target, thus reset the lastseen counter */
            m->lastseen = 1;
        }
        else if (pos_identical(npos, monster_pos(m)) || map_is_monster_at(monster_map(m), npos))
        {
            /* it seems there is no path to the target, or the destianation
               is blocked by another townsperson, thus get a new destination */
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
                      monster_get_fortune(nlarn_fortunes));
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
            return true;
    }
    return false;
}

inline const char *monster_name(monster *m) {
    return monster_data[m->type].name;
}

inline int monster_level(monster *m)
{
    return monster_data[m->type].level;
}

inline int monster_ac(monster *m)
{
    return monster_data[m->type].ac;
}

inline guint monster_int(monster *m)
{
    return monster_data[m->type].intelligence
            + monster_effect(m, ET_HEROISM)
            - monster_effect(m, ET_DIZZINESS);
}

inline int monster_gold_chance(monster *m)
{
    return monster_data[m->type].gold_chance;
}

inline int monster_gold_amount(monster *m)
{
    return monster_data[m->type].gold;
}

inline int monster_exp(monster *m)
{
    return monster_data[m->type].exp;
}

inline int monster_size(monster *m)
{
    return monster_data[m->type].size;
}

inline int monster_speed(monster *m)
{
    return monster_data[m->type].speed
            + monster_effect(m, ET_SPEED)
            + (monster_effect(m, ET_HEROISM) * 5)
            - monster_effect(m, ET_SLOWNESS)
            - (monster_effect(m, ET_DIZZINESS) * 5);
}

inline int monster_flags(monster *m, monster_flag f)
{
    return monster_data[m->type].flags & f;
}

inline const char *monster_sound(monster *m) {
    return monster_data[m->type].sound;
}

inline const char *monster_type_name(monster_t type)
{
    return monster_data[type].name;
}

inline int monster_type_ac(monster_t type)
{
    return monster_data[type].ac;
}

inline int monster_type_size(monster_t type)
{
    return monster_data[type].size;
}

inline int monster_type_speed(monster_t type)
{
    return monster_data[type].speed;
}

inline int monster_type_flags(monster_t type, monster_flag f)
{
    return monster_data[type].flags & f;
}

inline int monster_type_hp_max(monster_t type)
{
    return monster_data[type].hp_max;
}

inline char monster_type_glyph(monster_t type)
{
    return monster_data[type].glyph;
}

inline int monster_type_reroll_chance(monster_t type)
{
    return monster_data[type].reroll_chance;
}


static gboolean monster_breath_hit(const GList *traj,
                                   const damage_originator *damo __attribute__((unused)),
                                   gpointer data1,
                                   gpointer data2 __attribute__((unused)))
{
    monster *m;
    damage *dam = (damage *)data1;
    item_erosion_type iet;
    gboolean terminated = false;
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
            inv_erode(monster_inv(m), iet, false, NULL);

        monster_damage_take(m, damage_copy(dam));

        /* the breath will sweep over small monsters */
        if (monster_size(m) >= LARGE)
            terminated = true;
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
        else if (player_evade(nlarn->p))
        {
            if (!player_effect(nlarn->p, ET_BLINDNESS))
            {
                log_add_entry(nlarn->log, "The %s whizzes by you!",
                        monster_breath_data[dam->type].desc);
            }
        }
        else
        {
            /* Player failed to evade and takes the damage */
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
                inv_erode(&nlarn->p->inventory, iet, true,
                        player_item_filter_unequippable);
            }

            terminated = true;
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
