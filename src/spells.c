/*
 * spells.c
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

/* TODO: add prices */
const spell_data spells[SP_MAX] =
{
    {
        SP_NONE,
        NULL,
        NULL,
        SC_NONE,
        ET_NONE,
        NULL,
        NULL,
        NULL,
        0,
        0
    },
    {
        SP_PRO,
        "pro",
        "protection",
        SC_PLAYER,
        ET_PROTECTION,
        "Generates a protection field",
        NULL,
        NULL,
        1,
        0
    },
    {
        SP_MLE,
        "mle",
        "magic missile",
        SC_RAY,
        ET_NONE,
        "Creates and hurls a magic missile equivalent to a + 1 magic arrow",
        "Your missiles hit the %s.",
        NULL,
        1,
        0
    },
    {
        SP_DEX,
        "dex",
        "dexterity",
        SC_PLAYER,
        ET_INC_DEX,
        "Improves the casters dexterity",
        NULL,
        NULL,
        1,
        0
    },
    {
        SP_SLE,
        "sle",
        "sleep",
        SC_POINT,
        ET_SLEEP,
        "causes some monsters to go to sleep",
        NULL,
        NULL,
        1,
        0
    },
    {
        SP_CHM,
        "chm",
        "charm monster",
        SC_POINT,
        ET_CHARM_MONSTER,
        "some monsters may be awed at your magnificence",
        NULL,
        NULL,
        1,
        0
    },
    {
        SP_SSP,
        "ssp",
        "sonic spear",
        SC_RAY,
        ET_NONE,
        "causes your hands to emit a screeching sound toward what they point",
        "The sound damages the %s.",
        NULL,
        1,
        0
    },
    {
        SP_STR,
        "str",
        "strength",
        SC_PLAYER,
        ET_INC_STR,
        "Increase the casters strength for a short term",
        NULL,
        NULL,
        2,
        0
    },
    {
        SP_ENL,
        "enl",
        "enlightenment",
        SC_PLAYER,
        ET_ENLIGHTENMENT,
        "the caster becomes aware of things around him",
        NULL,
        NULL,
        2,
        0
    },
    {
        SP_HEL,
        "hel",
        "healing",
        SC_PLAYER,
        ET_INC_HP,
        "restores some hp to the caster",
        NULL,
        NULL,
        2,
        0
    },
    {
        SP_CBL,
        "cbl",
        "cure blindness",
        SC_PLAYER,
        ET_NONE,
        "restores sight to one so unfortunate as to be blinded",
        NULL,
        NULL,
        2,
        0
    },
    {
        SP_CRE,
        "cre",
        "create monster",
        SC_OTHER,
        ET_NONE,
        "creates a monster near the caster appropriate for the location",
        NULL,
        NULL,
        2,
        0
    },
    {
        SP_PHA,
        "pha",
        "phantasmal forces",
        SC_POINT,
        ET_NONE,
        "creates illusions, and if believed, monsters die",
        "The %s believed!",
        "The %s didn't believe the illusions!",
        2,
        0
    },
    {
        SP_INV,
        "inv",
        "invisibility",
        SC_PLAYER,
        ET_INVISIBILITY,
        "the caster becomes invisible",
        NULL,
        NULL,
        2,
        0
    },
    {
        SP_BAL,
        "bal",
        "fireball",
        SC_BLAST,
        ET_NONE,
        "makes a ball of fire that burns on what it hits",
        "The fireball hits the %s.",
        NULL,
        3,
        0
    },
    {
        SP_CLD,
        "cld",
        "cone of cold",
        SC_RAY,
        ET_NONE,
        "sends forth a cone of cold which freezes what it touches",
        "Your cone of cold strikes the %s.",
        NULL,
        3,
        0
    },
    {
        SP_PLY,
        "ply",
        "polymorph",
        SC_POINT,
        ET_NONE,
        "you can find out what this does for yourself",
        NULL,
        NULL,
        3,
        0
    },
    {
        SP_CAN,
        "can",
        "cancellation",
        SC_PLAYER,
        ET_CANCELLATION,
        "negates the ability of a monster to use his special abilities",
        NULL,
        NULL,
        3,
        0
    },
    {
        SP_HAS,
        "has",
        "haste self",
        SC_PLAYER,
        ET_SPEED,
        "speeds up the casters movements",
        NULL,
        NULL,
        3,
        0
    },
    {
        SP_CKL,
        "ckl",
        "killing cloud",
        SC_FLOOD,
        ET_NONE,
        "creates a fog of poisonous gas which kills all that is within it",
        "The %s gasps for air.",
        NULL,
        3,
        0
    },
    {
        SP_VPR,
        "vpr",
        "vaporize rock",
        SC_OTHER,
        ET_NONE,
        "this changes rock to air",
        NULL,
        NULL,
        3,
        0
    },
    {
        SP_DRY,
        "dry",
        "dehydration",
        SC_POINT,
        ET_NONE,
        "dries up water in the immediate vicinity",
        "The %s shrivels up.",
        NULL,
        4,
        0
    },
    {
        SP_LIT,
        "lit",
        "lightning",
        SC_RAY,
        ET_NONE,
        "you finger will emit a lightning bolt when this spell is cast",
        "A lightning bolt hits the %s.",
        NULL,
        4,
        0
    },
    {
        SP_DRL,
        "drl",
        "drain life",
        SC_POINT,
        ET_NONE,
        "subtracts hit points from both you and a monster",
        NULL,
        NULL,
        4,
        0
    },
    {
        SP_GLO,
        "glo",
        "invulnerability",
        SC_PLAYER,
        ET_INVULNERABILITY,
        "this globe helps to protect the player from physical attack",
        NULL,
        NULL,
        4,
        0
    },
    {
        SP_FLO,
        "flo",
        "flood",
        SC_FLOOD,
        ET_NONE,
        "this creates an avalanche of H2O to flood the immediate chamber",
        "The %s struggles for air in your flood!",
        NULL,
        4,
        0
    },
    {
        SP_FGR,
        "fgr",
        "finger of death",
        SC_POINT,
        ET_NONE,
        "this is a holy spell and calls upon your god to back you up",
        "The %s's heart stopped.",
        NULL,
        4,
        0
    },
    {
        SP_SCA,
        "sca",
        "scare monster",
        SC_POINT,
        ET_SCARE_MONSTER,
        "terrifies the monster so that hopefully he wont hit the magic user",
        NULL,
        NULL,
        5,
        0
    },
    {
        SP_HLD,
        "hld",
        "hold monster",
        SC_POINT,
        ET_HOLD_MONSTER,
        "the monster is frozen in his tracks if this is successful",
        NULL,
        NULL,
        5,
        0
    },
    {
        SP_STP,
        "stp",
        "time stop",
        SC_OTHER,
        ET_TIMESTOP,
        "all movement in the caverns ceases for a limited duration",
        NULL,
        NULL,
        5,
        0
    },
    {
        SP_TEL,
        "tel",
        "teleport away",
        SC_POINT,
        ET_NONE,
        "moves a particular monster around in the dungeon",
        NULL,
        NULL,
        5,
        0
    },
    {
        SP_MFI,
        "mfi",
        "magic fire",
        SC_FLOOD,
        ET_NONE,
        "this causes a curtain of fire to appear all around you",
        "The %s cringes from the flame.",
        NULL,
        5,
        0
    },
    {
        SP_MKW,
        "mkw",
        "make wall",
        SC_OTHER,
        ET_NONE,
        "Makes a wall in the specified place",
        NULL,
        NULL,
        6,
        0
    },
    {
        SP_SPH,
        "sph",
        "sphere of annihilation",
        SC_OTHER,
        ET_NONE,
        "anything caught in this sphere is instantly killed.",
        NULL,
        NULL,
        6,
        0
    },
    {
        SP_GEN,
        "gen",
        "genocide",
        SC_OTHER,
        ET_NONE,
        "eliminates a species of monster from the game -- use sparingly",
        NULL,
        NULL,
        6,
        0
    },
    {
        SP_SUM,
        "sum",
        "summon demon",
        SC_OTHER,
        ET_NONE,
        "summons a demon who hopefully helps you out",
        NULL,
        NULL,
        6,
        0
    },
    {
        SP_WTW,
        "wtw",
        "walk through walls",
        SC_PLAYER,
        ET_WALL_WALK,
        "allows the caster to walk through walls for a short period of time",
        NULL,
        NULL,
        6,
        0
    },
    {
        SP_ALT,
        "alt",
        "alter reality",
        SC_OTHER,
        ET_NONE,
        "god only knows what this will do",
        NULL,
        "Polinneaus won't let you mess with his dungeon!",
        6,
        0
    },
    {
        SP_PER,
        "per",
        "permanence",
        SC_OTHER,
        ET_NONE,
        "makes a character spell permanent, i. e. protection, strength, etc.",
        NULL,
        NULL,
        6,
        0
    },
};

char *spelmes[] =
{
    "",
    /*  1 */    "the web had no effect on the %s",
    /*  2 */    "the %s changed shape to avoid the web",
    /*  3 */    "the %s isn't afraid of you",
    /*  4 */    "the %s isn't affected",
    /*  5 */    "the %s can see you with his infravision",
    /*  6 */    "the %s vaporizes your missile",
    /*  7 */    "your missile bounces off the %s",
    /*  8 */    "the %s doesn't sleep",
    /*  9 */    "the %s resists",
    /* 10 */    "the %s can't hear the noise",
    /* 11 */    "the %s's tail cuts it free of the web",
    /* 12 */    "the %s burns through the web",
    /* 13 */    "your missiles pass right through the %s",
    /* 14 */    "the %s sees through your illusions",
    /* 15 */    "the %s loves the cold!",
    /* 16 */	"the %s loves the water!",
    /* 17 */	"the demon is terrified of the %s!",
    /* 18 */	"the %s loves fire and lightning!"
};


int spelweird[MT_MAX+8][SP_MAX] =
{
    /*                      p m d s c s    w s e h c c p i    b c p c h c v    d l d g f f    s h s t m    s g s w a p */
    /*                      r l e l h s    e t n e b r h n    a l l a a k p    r i r l l g    c l t e f    p e u t l e */
    /*                      o e x e m p    b r l l l e a v    l d y n s l r    y t l o o r    a d p l i    h n m w t r */
    /*            bat */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          gnome */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*      hobgoblin */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         jackal */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         kobold */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*            orc */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   4,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          snake */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*giant centipede */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         jaculi */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     troglodyte */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*      giant ant */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   floating eye */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     leprechaun */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          nymph */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         quasit */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   rust monster */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   4,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         zombie */ {  0,0,0,8,0,4,   0,0,0,0,0,0,0,0,   0,0,0,0,0,4,0,   4,0,0,0,0,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   assassin bug */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*        bugbear */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     hell hound */ {  0,6,0,0,0,0,   12,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     ice lizard */ {  0,0,0,0,0,0,   11,0,0,0,0,0,0,0,  0,15,0,0,0,0,0,  0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*        centaur */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          troll */ {  0,7,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   4,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*           yeti */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,15,0,0,0,0,0,  0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   white dragon */ {  0,0,0,0,0,0,   0,0,0,0,0,0,14,0,  0,15,0,0,0,0,0,  4,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*            elf */ {  0,0,0,0,0,0,   0,0,0,0,0,0,14,5,  0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*gelatinous cube */ {  0,0,0,0,0,0,   2,0,0,0,0,0,0,0,   0,0,0,0,0,4,0,   0,0,0,0,0,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*      metamorph */ {  0,13,0,0,0,0,  2,0,0,0,0,0,0,0,   0,0,0,0,0,4,0,   4,0,0,0,0,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         vortex */ {  0,13,0,0,0,10, 1,0,0,0,0,0,0,0,   0,0,0,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         ziller */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   violet fungi */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*         wraith */ {  0,0,0,8,0,4,   0,0,0,0,0,0,0,0,   0,0,0,0,0,4,0,   4,0,0,0,0,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*      forvalaka */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*      lama nobe */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*        osequip */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          rothe */ {  0,7,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*           xorn */ {  0,7,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   4,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*        vampire */ {  0,0,0,8,0,4,   0,0,0,0,0,0,0,0,   0,0,0,0,0,4,0,   0,0,0,0,0,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*invisible staker*/ {  0,0,0,0,0,0,   1,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*    poltergeist */ {  0,13,0,8,0,4,  1,0,0,0,0,0,0,0,   0,4,0,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /* disenchantress */ {  0,0,0,8,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*shambling mound */ {  0,0,0,0,0,10,  0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*    yellow mold */ {  0,0,0,8,0,0,   1,0,0,0,0,0,4,0,   0,0,0,0,0,4,0,   0,0,0,0,0,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     umber hulk */ {  0,7,0,0,0,0,   0,0,0,0,0,0,0,5,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     gnome king */ {  0,7,0,0,3,0,   0,0,0,0,0,0,0,5,   0,0,9,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          mimic */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     water lord */ {  0,13,0,8,3,4,  1,0,0,0,0,0,0,0,   0,0,9,0,0,4,0,   0,0,0,0,16,4,  0,0,0,0,0,   0,0,0,0,0,0 },
    /*  bronze dragon */ {  0,7,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   green dragon */ {  0,7,0,0,0,0,   11,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*    purple worm */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*          xvart */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*    spirit naga */ {  0,13,0,8,3,4,  1,0,0,0,0,0,0,5,   0,4,9,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*  silver dragon */ {  0,6,0,9,0,0,   12,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*platinum dragon */ {  0,7,0,9,0,0,   11,0,0,0,0,0,14,0, 0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*   green urchin */ {  0,0,0,0,0,0,   0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },
    /*     red dragon */ {  0,6,0,0,0,0,   12,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,   0,0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,0 },

    /*                      p m d s c s    w s e h c c p i    b c p c h c v    d l d g f f    s h s t m    s g s w a p */
    /*                      r l e l h s    e t n e b r h n    a l l a a k p    r i r l l g    c l t e f    p e u t l e */
    /*                      o e x e m p    b r l l l e a v    l d y n s l r    y t l o o r    a d p l i    h n m w t r */
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*     demon lord */ {  0,7,0,4,3,0,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   0,0,0,0,0,   9,0,0,0,0,0 },
    /*   demon prince */ {  0,7,0,4,3,9,   1,0,0,0,0,0,14,5,  0,0,4,0,0,4,0,   4,0,0,0,4,4,   4,0,0,0,4,   9,0,0,0,0,0 }
};

static int book_desc_mapping[SP_MAX - 1] = { 0 };

static const char *book_desc[SP_MAX - 1] =
{
    "parchment-bound",
    "thick",
    "dusty",
    "leather-bound",
    "heavy",
    "ancient",
    "buckram",
    "gilded",
    "embossed",
    "old",
    "thin",
    "light",
    "large",
    "vellum",
    "tan",
    "papyrus",
    "linen",
    "paper",
    "musty",
    "faded",
    "antique",
    "worn out",
    "tattered",
    "aged",
    "ornate",
    "inconspicuous",
    "awe-inspiring",
    "stained",
    "mottled",
    "plaid",
    "wax-lined",
    "bamboo",
    "clasped",
    "ragged",
    "dull",
    "canvas",
    "well-thumbed",
    "chambray",
};


spell *spell_new(int id)
{
    spell *nspell;

    assert(id > SP_NONE && id < SP_MAX);

    nspell = g_malloc0(sizeof(spell));
    nspell->id = id;
    nspell->knowledge = 1;

    return nspell;
}

void spell_destroy(spell *s)
{
    assert(s != NULL);
    g_free(s);
}

int spell_sort(gconstpointer a, gconstpointer b)
{
    gint order;
    spell *spell_a = *((spell**)a);
    spell *spell_b = *((spell**)b);

    if (spell_a->id == spell_b->id)
        order = 0;
    else
        order = strcmp(spell_code(spell_a), spell_code(spell_b));

    return order;
}

void book_desc_shuffle()
{
    shuffle(book_desc_mapping, SP_MAX - 1, 0);
}

book *book_new(int spell_id)
{
    book *nbook;
    int blessedness_status;

    assert(spell_id > SP_NONE && spell_id < SP_MAX);

	/* has to be zeroed or memcmp will fail */
    nbook = g_malloc0(sizeof(book));

    nbook->type = spell_id;

    blessedness_status = rand_1n(100);
    if (blessedness_status <= 10)
        nbook->blessedness = BT_CURSED;
    else if (blessedness_status >= 90)
        nbook->blessedness = BT_BLESSED;
    else
        nbook->blessedness = BT_UNCURSED;

    return nbook;
}

book *book_new_by_level(int level)
{
    int spell_id;
    /* TODO: do some random wizardry here */
    spell_id = rand_m_n(SP_NONE + 1, SP_MAX -1);

    return(book_new(spell_id));
}

void book_destroy(book *b)
{
    assert(b != NULL);
    g_free(b);
}

char *book_get_desc(int book_id)
{
    assert(book_id > SP_NONE && book_id < SP_MAX);
    return (char *)book_desc[book_desc_mapping[book_id - 1]];
}
