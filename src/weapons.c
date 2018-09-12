/*
 * weapons.c
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

#include "display.h"
#include "items.h"
#include "map.h"
#include "monsters.h"
#include "nlarn.h"
#include "player.h"
#include "random.h"
#include "weapons.h"

const ammo_data ammos[AMT_MAX] =
{
    /*  type       name            ac           dam   acc  mat        we  pr  ob */
    { AMT_STONE,   "pebble",       AMMO_SLING,     2,   1, IM_STONE, 100,  1, FALSE, },
    { AMT_SBULLET, "sling bullet", AMMO_SLING,     4,   2, IM_LEAD,   50,  3,  TRUE, },
    { AMT_ARROW,   "arrow",        AMMO_BOW,       8,   3, IM_WOOD,   80,  5,  TRUE, },
    { AMT_BOLT,    "bolt",         AMMO_CROSSBOW, 10,   4, IM_IRON,  100, 10, FALSE, },
};

const char *ammo_class_name[AMMO_MAX] =
{
    "sling",
    "bow",
    "crossbow",
};

const weapon_data weapons[WT_MAX] =
{
    /* weapon_t         name                         short name,       class          ammo      dam  acc  material    we     pr tw un ar ob th*/
    { WT_ODAGGER,       "orcish dagger",             "orc dagger",      WC_MELEE,     AMMO_MAX,  2,   3, IM_IRON,   850,     6, 0, 0, 1, 0, 1 },
    { WT_DAGGER,        "dagger",                    "dagger",          WC_MELEE,     AMMO_MAX,  3,   5, IM_IRON,   600,    10, 0, 0, 1, 1, 1 },
    { WT_SLING,         "sling",                     "sling",          WC_RANGED,   AMMO_SLING,  1,   3, IM_CLOTH,  200,    15, 1, 0, 1, 1, 0 },
    { WT_OSHORTSWORD,   "orcish short sword",        "orc short sword", WC_MELEE,     AMMO_MAX,  5,   2, IM_IRON,  1350,    30, 0, 0, 1, 0, 0 },
    { WT_SHORTSWORD,    "short sword",               "short sword",     WC_MELEE,     AMMO_MAX,  6,   3, IM_IRON,  1000,    60, 0, 0, 1, 1, 0 },
    { WT_ESHORTSWORD,   "elven short sword",         "elf short sword", WC_MELEE,     AMMO_MAX,  7,   4, IM_WOOD,  1180,    68, 0, 0, 1, 0, 0 },
    { WT_OSPEAR,        "orcish spear",              "orc spear",       WC_MELEE,     AMMO_MAX,  8,   2, IM_IRON,  2100,    75, 0, 0, 1, 0, 1 },
    { WT_SPEAR,         "spear",                     "spear",           WC_MELEE,     AMMO_MAX, 10,   3, IM_IRON,  1800,   120, 0, 0, 1, 1, 1 },
    { WT_ESPEAR,        "elven spear",               "elf spear",       WC_MELEE,     AMMO_MAX, 11,   4, IM_WOOD,  1600,   140, 0, 0, 1, 0, 1 },
    { WT_BOW,           "bow",                       "bow",            WC_RANGED,     AMMO_BOW,  3,   4, IM_WOOD,  1000,   220, 1, 0, 1, 1, 0 },
    { WT_CLUB,          "heavy club",                "club",            WC_MELEE,     AMMO_MAX, 10,   2, IM_WOOD,  3500,    50, 0, 0, 1, 0, 0 },
    { WT_MACE,          "mace",                      "mace",            WC_MELEE,     AMMO_MAX, 12,   3, IM_IRON,  2600,   160, 0, 0, 1, 1, 0 },
    { WT_FLAIL,         "flail",                     "flail",           WC_MELEE,     AMMO_MAX, 14,   2, IM_IRON,  2900,   195, 1, 0, 1, 1, 0 },
    { WT_BATTLEAXE,     "battle-axe",                "battle-axe",      WC_MELEE,     AMMO_MAX, 18,   2, IM_IRON,  2700,   350, 1, 0, 1, 1, 0 },
    { WT_CROSSBOW,      "crossbow",                  "crossbow",       WC_RANGED, AMMO_CROSSBOW, 5,   3, IM_WOOD,  3500,   600, 1, 0, 1, 0, 0 },
    { WT_LONGSWORD,     "longsword",                 "longsword",       WC_MELEE,     AMMO_MAX, 22,   3, IM_IRON,  1950,   550, 0, 0, 1, 1, 0 },
    { WT_2SWORD,        "two-handed sword",          "2-handed sword",  WC_MELEE,     AMMO_MAX, 26,   4, IM_IRON,  3600,  1000, 1, 0, 1, 1, 0 },
    { WT_SWORDSLASHING, "sword of slashing",         "slashing",        WC_MELEE,     AMMO_MAX, 30,   5, IM_STEEL, 2200,  6000, 0, 0, 1, 0, 0 },
    /* unique weapons */
    { WT_LANCEOFDEATH,  "lance of death",            "lance of death",  WC_MELEE,     AMMO_MAX, 20,   3, IM_WOOD,  2900, 65000, 1, 1, 1, 1, 0 },
    { WT_VORPALBLADE,   "Vorpal blade",              "Vorpal blade",    WC_MELEE,     AMMO_MAX, 22,   3, IM_STEEL, 1600,  3800, 0, 1, 1, 0, 0 },
    { WT_SLAYER,        "Slayer",                    "Slayer",          WC_MELEE,     AMMO_MAX, 30,   5, IM_STEEL, 1800,  6800, 0, 1, 0, 0, 0 },
    { WT_SUNSWORD,      "Sunsword",                  "Sunsword",        WC_MELEE,     AMMO_MAX, 32,   6, IM_STEEL, 1800,  7000, 0, 1, 1, 0, 0 },
    { WT_BESSMAN,       "Bessman's flailing hammer", "Bessman's",       WC_MELEE,     AMMO_MAX, 35,   6, IM_STEEL, 5800, 10000, 1, 1, 0, 0, 0 },
};

/* static functions */
damage *weapon_get_ranged_damage(player *p, item *weapon, item *ammo);
gboolean weapon_ammo_drop(map *m, item *ammo, const GList *traj);

static gboolean weapon_pos_hit(const GList *traj,
        const damage_originator *damo,
        gpointer data1, gpointer data2);

int weapon_calc_to_hit(struct player *p, struct _monster *m, item *weapon, item *ammo)
{
    g_assert (p != NULL && m != NULL);

    const int to_hit = p->level
                       + max(0, player_get_dex(p) - 12)
                       + (weapon ? weapon_acc(weapon) : 0)
                       + (ammo ? ammo_accuracy(ammo) : 0)
                       + (player_get_speed(p) / 25)
                       /* the rule below gives a -3 for tiny monsters and a +4
                          for gargantuan monsters */
                       + ((monster_size(m) - ESIZE_MEDIUM) / 25)
                       - monster_ac(m)
                       - (monster_speed(m) / 25)
                       - (!monster_in_sight(m) ? 5 : 0);

    if (to_hit < 1)
        return 0;

    if (to_hit >= 20)
        return 100;

    /* roll the dice */
    return (5 * to_hit);
}

int weapon_fire(struct player *p)
{
    g_assert(p != NULL);

    map *pmap = game_map(nlarn, Z(p->pos));
    position target;             /* the selected target */
    damage_originator damo = { DAMO_PLAYER, p };
    gchar *wdesc;                /* the weapon description */
    item *weapon = p->eq_weapon; /* the equipped weapon */
    item *ammo   = p->eq_quiver; /* the quivered ammo */
    monster *m = NULL;           /* the targeted monster */

    /* Check if the player is able to move. */
    if (!player_movement_possible(p))
        return FALSE;

    /* check if the player wields a weapon */
    if (weapon == NULL)
    {
        log_add_entry(nlarn->log, "You do not wield any weapon!");
        return FALSE;
    }

    /* wielding a weapon, describe it */
    wdesc = item_describe(weapon, player_item_known(p, weapon), TRUE, TRUE);

    /* check if it is a ranged weapon */
    if (!weapon_is_ranged(weapon))
    {
        wdesc[0] = g_ascii_toupper(wdesc[0]);
        log_add_entry(nlarn->log, "%s is not a ranged weapon!", wdesc);
        g_free(wdesc);
        return FALSE;
    }

    /* check if ammo is quivered */
    if (!ammo)
    {
        log_add_entry(nlarn->log, "You have no ammunition in your quiver!");
        g_free(wdesc);
        return FALSE;
    }

    /* check if the quivered ammo matches the weapon */
    if (weapon_ammo(weapon) != ammo_class(ammo))
    {
        gchar *adesc = item_describe(ammo, player_item_known(p, ammo),
                                     TRUE, FALSE);
        log_add_entry(nlarn->log, "You cannot fire %s with %s.", adesc, wdesc);

        g_free(wdesc);
        g_free(adesc);
        return FALSE;
    }

    /* all checks are successful */
    target = display_get_position(p, "Select a target", TRUE, FALSE, 0,
                                  FALSE, TRUE);

    /* is the target a valid position? */
    if(!pos_valid(target))
    {
        log_add_entry(nlarn->log, "You did not fire %s.", wdesc);

        g_free(wdesc);
        return FALSE;
    }

    /* get the targeted monster  */
    m = map_get_monster_at(pmap, target);

    /* check if there is a monster at the targeted position */
    if (m == NULL)
    {
        log_add_entry(nlarn->log, "I see no monster there.");

        g_free(wdesc);
        return FALSE;
    }

    /* protect townsfolk from aggressive players */
    if (monster_type(m) == MT_TOWN_PERSON)
    {
        log_add_entry(nlarn->log, "Gosh! How dare you!");

        g_free(wdesc);
        return FALSE;
    }

    /* log the event */
    log_add_entry(nlarn->log, "You fire %s at the %s.", wdesc,
            monster_get_name(m));
    g_free(wdesc);

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
        player_item_unequip(p, NULL, ammo, TRUE);
        inv_del_element(&p->inventory, ammo);
    }

    /* mark the ammo as fired */
    ammo->fired = TRUE;

    /* paint a ray to follow the path of the bullet */
    if (map_trajectory(p->pos, target, &damo, weapon_pos_hit,
                weapon, ammo, FALSE, item_glyph(ammo->type),
                item_colour(ammo), FALSE))
        return TRUE; /* one of the callbacks succeeded */

    return TRUE;
}

void weapon_swap(struct player *p)
{
    g_assert(p != NULL);

    item *pweapon = p->eq_weapon;
    item *sweapon = p->eq_sweapon;
    gchar *pdesc = NULL;
    gchar *sdesc = NULL;

    if (pweapon == sweapon && pweapon == NULL)
    {
        log_add_entry(nlarn->log, "You have no weapon ready.");
        return;
    }

    if (pweapon && pweapon->cursed)
    {
        pdesc = item_describe(pweapon, player_item_known(p, pweapon),
                              TRUE, TRUE);
        log_add_entry(nlarn->log, "You can't put %s away, "
                      "it is weld into your hand!", pdesc);
        g_free(pdesc);

        return;
    }

    if (sweapon && p->eq_shield && weapon_is_twohanded(sweapon))
    {
        /* variable abuse: pdesc describes secondary weapon */
        pdesc = item_describe(sweapon, player_item_known(p, sweapon),
                              TRUE, TRUE);
        /* variable abuse: sdesc describes held shield */
        sdesc = item_describe(p->eq_shield, player_item_known(p,
                              p->eq_shield), TRUE, TRUE);

        log_add_entry(nlarn->log, "You can't ready %s while "
                      "holding the %s!", pdesc, sdesc);
        g_free(pdesc);
        g_free(sdesc);

        return;
    }

    if (!player_make_move(p, 2, TRUE, "swapping your weapons"))
        return; /* interrupted */

    p->eq_weapon  = sweapon;
    p->eq_sweapon = pweapon;

    if (pweapon)
        pdesc = item_describe(pweapon, player_item_known(p, pweapon),
                              TRUE, FALSE);
    if (sweapon)
        sdesc = item_describe(sweapon, player_item_known(p, sweapon),
                              TRUE, FALSE);

    log_add_entry(nlarn->log, "You have swapped your weapons: Primary weapon:"
                  " %s, secondary weapon, not wielded: %s.",
                  (sdesc ? sdesc : "none"), (pdesc ? pdesc : "none"));

    g_free(pdesc);
    g_free(sdesc);
}

char *weapon_shortdesc(item *weapon, guint available_space)
{
    /* a little sanity check */
    if (available_space == 0) return NULL;

    GString *desc = g_string_new(NULL);
    if (weapon->bonus_known)
    {
        g_string_append_printf(desc, "%+d ", weapon->bonus);
    }

    const gboolean need_bonus
    = (weapon->burnt || weapon->corroded || weapon->rusty
       || (weapon->blessed_known
           && (weapon->blessed || weapon->cursed)));

    g_string_append_printf(desc, "%s", need_bonus
                           ? weapon_short_name(weapon)
                           : weapon_name(weapon));

    // Add corrosion/curse status in brackets.
    // Alternatively, convey that information with colours.
    if (need_bonus && desc->len < available_space - 4)
    {
        GString *bonus = g_string_new(NULL);

        gboolean need_comma = FALSE;
        if (weapon->burnt == 2)
        {
            g_string_append_printf(bonus, "v. burnt, ");
            need_comma = TRUE;
        }

        if (weapon->corroded == 2)
        {
            g_string_append_printf(bonus, "%sv. corroded",
                                   need_comma ? ", " : "");
            need_comma = TRUE;
        }
        if (weapon->rusty == 2)
        {
            g_string_append_printf(bonus, "%sv. rusty",
                                   need_comma ? ", " : "");
            need_comma = TRUE;
        }

        if (weapon->burnt == 1)
        {
            g_string_append_printf(bonus, "%sburnt",
                                   need_comma ? ", " : "");
            need_comma = TRUE;
        }
        if (weapon->corroded == 1)
        {
            g_string_append_printf(bonus, "%scorroded",
                                   need_comma ? ", " : "");
            need_comma = TRUE;
        }
        if (weapon->rusty == 1)
        {
            g_string_append_printf(bonus, "%srusty",
                                   need_comma ? ", " : "");
            need_comma = TRUE;
        }

        if (weapon->blessed_known)
        {
            if (weapon->blessed)
            {
                g_string_append_printf(bonus, "%sblessed",
                                       need_comma ? ", " : "");
            }
            else if (weapon->cursed)
            {
                g_string_append_printf(bonus, "%scursed",
                                       need_comma ? ", " : "");
            }
        }

        g_string_append_printf(desc, " (%s)", bonus->str);
        g_string_free(bonus, TRUE);
    }

    if (desc->len > available_space)
    {
        if (desc->str[available_space - 1] != ' ')
            desc->str[available_space - 1] = '.';
        desc->str[available_space] = '\0';
    }

    /* free the temporary string */
    return g_string_free(desc, FALSE);
}

damage *weapon_get_ranged_damage(player *p, item *weapon, item *ammo)
{
    g_assert (p != NULL && weapon != NULL && ammo != NULL);

    damage *dam = damage_new(DAM_PHYSICAL, ATT_WEAPON, 0, DAMO_PLAYER, p);
    dam->amount = weapon_damage(weapon) + ammo_damage(ammo);

    return dam;
}

gboolean weapon_ammo_drop(map *m, item *ammo, const GList *traj)
{
    /* Due to the recursive usage of this function traj may be NULL
       (e.g. when the player is wall-walking and shooting at a xorn). */
    if (traj == NULL)
    {
        item_destroy(ammo);
        return TRUE;
    }

    position pos;
    pos_val(pos) = GPOINTER_TO_UINT(traj->data);
    map_tile_t tt = map_tiletype_at(m, pos);

    /* If the ammo comes to stop on a solid tile it has to be dropped on
       the last tile that is not solid, i.e. the floor before a wall tile. */
    if (!map_pos_transparent(m, pos))
        return weapon_ammo_drop(m, ammo, g_list_previous(traj));

    /* check if the ammo survives usage */
    if (chance(item_fragility(ammo) + 15)
            || (tt == LT_DEEPWATER)
            || (tt == LT_LAVA))
        item_destroy(ammo);
    else
        inv_add(map_ilist_at(m, pos), ammo);

    return TRUE;
}

static gboolean weapon_pos_hit(const GList *traj,
        const damage_originator *damo __attribute__((unused)),
        gpointer data1,
        gpointer data2)
{
    position cpos;
    pos_val(cpos) = GPOINTER_TO_UINT(traj->data);

    map *cmap = game_map(nlarn, Z(cpos));
    item *weapon = (item *)data1;
    item *ammo = (item *)data2;
    monster *m = map_get_monster_at(cmap, cpos);
    gboolean retval = FALSE;
    gboolean ammo_handled = FALSE;

    /* need a definite description for the ammo */
    gchar *adesc = item_describe(ammo, player_item_known(nlarn->p, ammo), TRUE, TRUE);
    adesc[0] = g_ascii_toupper(adesc[0]);

    if (m != NULL)
    {
        /* there is a monster at the position */
        /* the bullet might have hit the monster */
        if (chance(weapon_calc_to_hit(nlarn->p, m, weapon, ammo)))
        {
            /* hit */
            damage *dam = weapon_get_ranged_damage(nlarn->p, weapon, ammo);

            if (monster_in_sight(m))
                log_add_entry(nlarn->log, "%s hits the %s.",
                        adesc, monster_name(m));

            monster_damage_take(m, dam);

            ammo_handled = weapon_ammo_drop(cmap, ammo, traj);
            retval = TRUE;
        }
        else
        {
            /* missed */
            if (monster_in_sight(m))
                log_add_entry(nlarn->log, "%s misses the %s.",
                        adesc, monster_name(m));
        }
    }
    else if (pos_identical(nlarn->p->pos, cpos))
    {
        /* The bullet may hit the player */
        /* TODO: implement */
    }

    if (!ammo_handled && !map_pos_transparent(cmap, cpos))
    {
        /* The ammo hit some map feature -> stop its movement */
        weapon_ammo_drop(cmap, ammo, traj);

        retval = TRUE;
    }

    g_free(adesc);
    return retval;
}
