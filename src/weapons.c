/*
 * weapons.c
 * Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
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

/* $Id$ */

#include <assert.h>

#include "display.h"
#include "items.h"
#include "map.h"
#include "monsters.h"
#include "nlarn.h"
#include "player.h"
#include "weapons.h"

const ammo_data ammos[AMT_MAX] =
{
    /*  type       name            ac           dam   acc  mat        we  pr  ob */
    { AMT_NONE,    NULL,           AMMO_NONE,      0,   0, IM_NONE,    0,  0, 0, },
    { AMT_STONE,   "pebble",       AMMO_SLING,     2,   1, IM_STONE, 100,  1, 0, },
    { AMT_SBULLET, "sling bullet", AMMO_SLING,     4,   2, IM_LEAD,   50,  3, 1, },
    { AMT_ARROW,   "arrow",        AMMO_BOW,       8,   3, IM_WOOD,   80,  5, 1, },
    { AMT_BOLT,    "bolt",         AMMO_CROSSBOW, 10,   4, IM_IRON,  100, 10, 1, },
};

const char *ammo_class_name[AMMO_MAX] =
{
    NULL,
    "sling",
    "bow",
    "crossbow",
};

const weapon_data weapons[WT_MAX] =
{
    /* weapon_t         name                         short name,       class          ammo      dam  acc  material    we     pr tw un ar ob */
    { WT_NONE,          "",                          "",		         WC_NONE,     AMMO_NONE,  0,   0, IM_NONE,     0,     0, 0, 0, 0, 0 },
    { WT_ODAGGER,       "orcish dagger",             "orc dagger",      WC_MELEE,     AMMO_NONE,  2,   2, IM_IRON,   850,     6, 0, 0, 1, 0 },
    { WT_DAGGER,        "dagger",                    "dagger",          WC_MELEE,     AMMO_NONE,  3,   3, IM_IRON,   600,    10, 0, 0, 1, 1 },
    { WT_SLING,         "sling",                     "sling",          WC_RANGED,    AMMO_SLING,  1,   2, IM_CLOTH,  200,    15, 0, 0, 1, 1 },
    { WT_OSHORTSWORD,   "orcish short sword",        "orc short sword", WC_MELEE,     AMMO_NONE,  5,   1, IM_IRON,  1350,    30, 0, 0, 1, 0 },
    { WT_SHORTSWORD,    "short sword",               "short sword",     WC_MELEE,     AMMO_NONE,  6,   2, IM_IRON,  1000,    60, 0, 0, 1, 1 },
    { WT_ESHORTSWORD,   "elven short sword",         "elf short sword", WC_MELEE,     AMMO_NONE,  7,   2, IM_WOOD,  1180,    68, 0, 0, 1, 0 },
    { WT_OSPEAR,        "orcish spear",              "orc spear",       WC_MELEE,     AMMO_NONE,  8,   2, IM_IRON,  2100,    75, 0, 0, 1, 0 },
    { WT_SPEAR,         "spear",                     "spear",           WC_MELEE,     AMMO_NONE, 10,   2, IM_IRON,  1800,   120, 0, 0, 1, 1 },
    { WT_ESPEAR,        "elven spear",               "elf spear",       WC_MELEE,     AMMO_NONE, 11,   3, IM_WOOD,  1600,   140, 0, 0, 1, 0 },
    { WT_BOW,           "bow",                       "bow",            WC_RANGED,     AMMO_BOW,   3,   3, IM_WOOD,  1000,   220, 0, 0, 1, 1 },
    { WT_MACE,          "mace",                      "mace",            WC_MELEE,     AMMO_NONE, 12,   1, IM_IRON,  2600,   160, 0, 0, 1, 1 },
    { WT_FLAIL,         "flail",                     "flail",           WC_MELEE,     AMMO_NONE, 14,   1, IM_IRON,  2900,   195, 1, 0, 1, 1 },
    { WT_BATTLEAXE,     "battle-axe",                "battle-axe",      WC_MELEE,     AMMO_NONE, 18,   2, IM_IRON,  2700,   350, 1, 0, 1, 1 },
    { WT_CROSSBOW,      "crossbow",                  "crossbow",       WC_RANGED, AMMO_CROSSBOW,  5,   3, IM_WOOD,  3500,   600, 1, 0, 1, 1 },
    { WT_LONGSWORD,     "longsword",                 "longsword",       WC_MELEE,     AMMO_NONE, 22,   3, IM_IRON,  1950,   550, 0, 0, 1, 1 },
    { WT_2SWORD,        "two-handed sword",          "2-handed sword",  WC_MELEE,     AMMO_NONE, 26,   4, IM_IRON,  3600,  1000, 1, 0, 1, 1 },
    { WT_SWORDSLASHING, "sword of slashing",         "slashing",        WC_MELEE,     AMMO_NONE, 30,   5, IM_IRON,  2200,  6000, 0, 0, 1, 1 },
    /* unique weapons */
    { WT_LANCEOFDEATH,  "lance of death",            "lance of death",  WC_MELEE,     AMMO_NONE, 20,   3, IM_WOOD,  2900, 65000, 1, 1, 1, 1 },
    { WT_VORPALBLADE,   "Vorpal blade",              "Vorpal blade",    WC_MELEE,     AMMO_NONE, 22,   3, IM_STEEL, 1600,  3800, 0, 1, 1, 1 },
    { WT_SLAYER,        "Slayer",                    "Slayer",          WC_MELEE,     AMMO_NONE, 30,   5, IM_STEEL, 1800,  6800, 0, 1, 0, 1 },
    { WT_SUNSWORD,      "Sunsword",                  "Sunsword",        WC_MELEE,     AMMO_NONE, 32,   6, IM_STEEL, 1800,  7000, 0, 1, 1, 1 },
    { WT_BESSMAN,       "Bessman's flailing hammer", "Bessman's",       WC_MELEE,     AMMO_NONE, 35,   6, IM_STEEL, 5800, 10000, 1, 1, 0, 1 },
};

/* static functions */
damage *weapon_get_ranged_damage(player *p, item *weapon, item *ammo);
void weapon_ammo_drop(map *m, item *ammo, position pos);

static gboolean weapon_pos_hit(position pos, const damage_originator *damo,
                               gpointer data1, gpointer data2);

int weapon_calc_to_hit(struct player *p, struct _monster *m, item *weapon, item *ammo)
{
    assert (p != NULL && m != NULL);

    const int to_hit = p->level
                       + max(0, player_get_dex(p) - 12)
                       + (weapon ? weapon_acc(weapon) : 0)
                       + (ammo ? ammo_accuracy(ammo) : 0)
                       + (player_get_speed(p) / 25)
                       - monster_ac(m)
                       - (monster_speed(m) / 25);

    if (to_hit < 1)
        return 0;

    if (to_hit >= 20)
        return 100;

    /* roll the dice */
    return (5 * to_hit);
}

int weapon_fire(struct player *p)
{
    assert(p != NULL);

    map *pmap = game_map(nlarn, Z(p->pos));
    position target;             /* the selected target */
    damage_originator damo = { DAMO_PLAYER, p };
    char wdesc[81] = {};         /* the weapon description */
    char adesc[81] = {};         /* the ammo description */
    item *weapon = p->eq_weapon; /* the equipped weapon */
    item *ammo   = p->eq_quiver; /* the quivered ammo */
    monster *m = NULL;           /* the targeted monster */

    /* check if the player wields a weapon */
    if (weapon == NULL)
    {
        log_add_entry(nlarn->log, "You do not wield any weapon!");
        return FALSE;
    }

    /* wielding a weapon, describe it */
    item_describe(weapon, player_item_known(p, weapon), TRUE, TRUE, wdesc, 80);

    /* check if it is a ranged weapon */
    if (!weapon_is_ranged(weapon))
    {
        wdesc[0] = g_ascii_toupper(wdesc[0]);
        log_add_entry(nlarn->log, "%s is not a ranged weapon!", wdesc);
        return FALSE;
    }

    /* check if ammo is quivered */
    if (!ammo)
    {
        log_add_entry(nlarn->log, "You do not have any ammunition in your quiver!");
        return FALSE;
    }

    /* ammo is quivered, describe it */
    item_describe(ammo, player_item_known(p, ammo), TRUE, FALSE, adesc, 80);

    /* check if the quivered ammo matches the weapon */
    if (weapon_ammo(weapon) != ammo_class(ammo))
    {
        log_add_entry(nlarn->log, "You cannot fire %s with %s.", adesc, wdesc);
        return FALSE;
    }

    /* all checks are successful */
    target = display_get_position(p, "Select a target", FALSE, FALSE, 0, FALSE, TRUE);

    /* is the target a valid position? */
    if(!pos_valid(target))
    {
        log_add_entry(nlarn->log, "You did not fire %s.", wdesc);
        return FALSE;
    }

    /* get the targeted monster  */
    m = map_get_monster_at(pmap, target);

    /* check if there is a monster at the targeted position */
    if (!m || !monster_in_sight(m))
    {
        log_add_entry(nlarn->log, "I see no monster there.");
        return FALSE;
    }

    /* protect townsfolk from agressive players */
    if (monster_type(m) == MT_TOWN_PERSON)
    {
        log_add_entry(nlarn->log, "Gosh! How dare you!");
        return FALSE;
    }

    /* log the event */
    log_add_entry(nlarn->log, "You fire %s at the %s.", wdesc, monster_name(m));

    /* --- finally shooting the weapon --- */

    /* get a piece of ammo */
    if (ammo->count > 1)
    {
        /* get a new piece of ammo of the quivered stack */
        ammo = item_split(ammo, 1);
    }
    else
    {
        /* shooting the last piece of ammo from the quiver */
        p->eq_quiver = NULL;
    }

    /* paint a ray to follow the path of the bullet */
    if (area_ray_trajectory(p->pos, target, &damo, weapon_pos_hit, weapon, ammo, FALSE,
                            item_glyph(ammo->type), item_colour(ammo), FALSE))
        return TRUE; /* one of the callbacks succeeded */

    /* none of the callbacks succeeded -> drop the ammo at the target position */
    weapon_ammo_drop(pmap, ammo, target);

    return TRUE;
}

void weapon_swap(struct player *p)
{
    assert(p != NULL);

    item *pweapon = p->eq_weapon;
    item *sweapon = p->eq_sweapon;
    char pdesc[81] = {};
    char sdesc[81] = {};

    if (!player_make_move(p, 2, TRUE, "swapping your weapons"))
        return; /* interrupted */

    p->eq_weapon  = sweapon;
    p->eq_sweapon = pweapon;

    if (pweapon)
        item_describe(pweapon, player_item_known(p, pweapon), TRUE, FALSE, pdesc, 80);

    if (sweapon)
        item_describe(sweapon, player_item_known(p, sweapon), TRUE, FALSE, sdesc, 80);

    log_add_entry(nlarn->log, "You have swapped your weapons: Primary weapon: %s, secondary weapon, not wielded: %s.",
                  (sdesc[0] ? sdesc : "none"), (pdesc[0] ? pdesc : "none"));
}

damage *weapon_get_ranged_damage(player *p, item *weapon, item *ammo)
{
    assert (p != NULL && weapon != NULL && ammo != NULL);

    damage *dam = damage_new(DAM_PHYSICAL, ATT_WEAPON, 0, DAMO_PLAYER, p);
    dam->amount = weapon_damage(weapon) + ammo_damage(ammo);

    return dam;
}

void weapon_ammo_drop(map *m, item *ammo, position pos)
{
    /* there is a 2/3 chance that the bullet survived the usage */
    if (chance(66))
        inv_add(map_ilist_at(m, pos), ammo);
    else
        item_destroy(ammo);
}

static gboolean weapon_pos_hit(position pos,
                               const damage_originator *damo __attribute__((unused)),
                               gpointer data1,
                               gpointer data2)
{
    map *cmap = game_map(nlarn, Z(pos));
    item *weapon = (item *)data1;
    item *ammo = (item *)data2;
    monster *m = map_get_monster_at(cmap, pos);
    char adesc[81] = {};

    /* need a definite description for the ammo */
    item_describe(ammo, player_item_known(nlarn->p, ammo), TRUE, TRUE, adesc, 80);
    adesc[0] = g_ascii_toupper(adesc[0]);

    if (!map_pos_passable(cmap, pos))
    {
        /* the ammo hit some map feature -> drop it at the position */
        weapon_ammo_drop(cmap, ammo, pos);
        return TRUE;
    }
    else if (m != NULL)
    {
        /* there is a monster at the position */
        /* the bullet might have hit the monster */
        if (chance(weapon_calc_to_hit(nlarn->p, m, weapon, ammo)))
        {
            /* hit */
            damage *dam = weapon_get_ranged_damage(nlarn->p, weapon, ammo);

            if (monster_in_sight(m))
                log_add_entry(nlarn->log, "%s hits the %s.", adesc, monster_name(m));

            monster_damage_take(m, dam);
            weapon_ammo_drop(cmap, ammo, pos);

            return TRUE;
        }
        else
        {
            /* missed */
            if (monster_in_sight(m))
                log_add_entry(nlarn->log, "%s misses the %s.", adesc, monster_name(m));
        }
    }
    else if (pos_identical(nlarn->p->pos, pos))
    {
        /* the bullet may hit the player */
        /* TODO: implement */
    }

    /* the bullet passed unhindered */
    return FALSE;
}
