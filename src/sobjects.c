/*
 * sobjects.c
 * Copyright (C) 2009-2026 Joachim de Groot <jdegroot@web.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib/gi18n.h>

#include "colours.h"
#include "display.h"
#include "game.h"
#include "map.h"
#include "extdefs.h"
#include "sobjects.h"
#include "player.h"
#include "random.h"

DEFINE_ENUM(sobject_t, SOBJECT_TYPE_ENUM)

const sobject_data sobjects[LS_MAX] =
{
    /* type             gly   color           desc                                       pa tr */
    { LS_NONE,          ' ',  COLOURLESS,     NULL,                                      1, 1, },
    { LS_ALTAR,         '_',  GREY93,         N_("a holy altar"),                        1, 1, },
    { LS_THRONE,        '\\', VIOLET_PURPLE,  N_("a handsome, jewel-encrusted throne"),  1, 1, },
    { LS_THRONE2,       '\\', VIOLET_PURPLE,  N_("a handsome, jewel-encrusted throne"),  1, 1, },
    { LS_DEADTHRONE,    '\\', GREY_GOOSE,     N_("a massive throne"),                    1, 1, },
    { LS_STAIRSDOWN,    '>',  LIGHT_GREY,     N_("a circular staircase"),                1, 1, },
    { LS_STAIRSUP,      '<',  LIGHT_GREY,     N_("a circular staircase"),                1, 1, },
    { LS_ELEVATORDOWN,  'I',  GREY_GOOSE,     N_("a volcanic shaft leading downward"),   1, 1, },
    { LS_ELEVATORUP,    'I',  GREY93,         N_("the base of a volcanic shaft"),        1, 1, },
    { LS_FOUNTAIN,      '{',  WATER_BLUE,     N_("a bubbling fountain"),                 1, 1, },
    { LS_DEADFOUNTAIN,  '{',  GREY_GOOSE,     N_("a dead fountain"),                     1, 1, },
    { LS_STATUE,        '|',  LIGHT_GREY,     N_("a great marble statue"),               1, 1, },
    { LS_URN,           'u',  YELLOW,         N_("a golden urn"),                        1, 1, },
    { LS_MIRROR,        '|',  ICE_COLD_GREEN, N_("a mirror"),                            1, 1, },
    { LS_OPENDOOR,      '/',  ELM_BROWN_RED,  N_("an open door"),                        1, 1, },
    { LS_CLOSEDDOOR,    '+',  ELM_BROWN_RED,  N_("a closed door"),                       0, 0, },
    { LS_CAVERNS_ENTRY, 'O',  GREY_GOOSE,     N_("the entrance to the caverns"),         1, 1, },
    { LS_CAVERNS_EXIT,  'O',  GREY93,         N_("the exit to town"),                    1, 1, },
    { LS_HOME,          'H',  GREY_GOOSE,     N_("your home"),                           1, 0, },
    { LS_DNDSTORE,      'D',  GREY_GOOSE,     N_("a DND store"),                         1, 0, },
    { LS_TRADEPOST,     'T',  GREY_GOOSE,     N_("the Larn trading Post"),               1, 0, },
    { LS_LRS,           'L',  GREY_GOOSE,     N_("an LRS office"),                       1, 0, },
    { LS_SCHOOL,        'S',  GREY_GOOSE,     N_("the College of Larn"),                 1, 0, },
    { LS_BANK,          'B',  GREY_GOOSE,     N_("the bank of Larn"),                    1, 0, },
    { LS_BANK2,         'B',  GREY93,         N_("a branch office of the bank of Larn"), 1, 0, },
    { LS_MONASTERY,     'M',  GREY93,         N_("the Monastery of Larn"),               1, 0, },
};

static void monster_appear(monster_t type, position mpos);
static void flood_affect_area(position pos, int radius, int type, int duration);
static bool sobject_blast_hit(position pos, const damage_originator *damo,
                              gpointer data1, gpointer data2);

int player_altar_desecrate(player *p)
{
    g_assert (p != NULL);

    map *current = game_map(nlarn, Z(p->pos));

    if (map_sobject_at(current, p->pos) != LS_ALTAR)
    {
        log_add_entry(nlarn->log, _("There is no altar here."));
        return false;
    }

    log_add_entry(nlarn->log, _("You try to desecrate the altar."));

    /* Decrease godly goodwill significantly for desecration */
    p->godly_goodwill -= 50;
    log_add_entry(nlarn->log, _("Your god is displeased!"));

    if (chance(60))
    {
        /* try to find a space for the monster near the altar */
        position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, false);

        if (pos_valid(mpos))
        {
            /* create a monster - should be very dangerous */
            monster_appear(MT_RED_DRAGON, mpos);
        }

        effect *e = effect_new(ET_AGGRAVATE_MONSTER);
        e->turns = 2500;
        player_effect_add(p, e);
    }
    else if (chance(30))
    {
        /* destroy altar */
        log_add_entry(nlarn->log, _("The altar crumbles into a pile of dust before your eyes."));
        map_sobject_set(current, p->pos, LS_NONE);

        /* Additional penalty for destroying the altar */
        p->godly_goodwill -= 100;
        log_add_entry(nlarn->log, _("Your god is very displeased!"));
    }
    else
    {
        log_add_entry(nlarn->log, _("You fail to destroy the altar."));
    }

    return true;
}

int player_altar_pray(player *p)
{
    effect *e = NULL;
    item **armour = NULL;

    g_assert (p != NULL);

    map *current = game_map(nlarn, Z(p->pos));

    if (map_sobject_at(current, p->pos) != LS_ALTAR)
    {
        log_add_entry(nlarn->log, _("There is no altar here."));
        return false;
    }

    const guint player_gold = player_get_gold(p);
    const guint total_gold  = player_gold + p->bank_account;
    if (total_gold == 0)
    {
        log_add_entry(nlarn->log, _("You don't have any money to donate."));
        return false;
    }

    // Use a sensible default value, so you don't anger the gods without
    // meaning to.
    const guint donation = display_get_count(_("How much gold do you want to donate?"),
            total_gold >= 200 ? 200 : 0);

    /* 0 gold donations are likely to be the result of escaping the prompt */
    if (!donation)
    {
        log_add_entry(nlarn->log, _("So you decide not to donate anything."));
        return false;
    }

    if (donation > total_gold)
    {
        log_add_entry(nlarn->log, _("You don't have that much money!"));
        return false;
    }

    // First pay with carried gold, then pay the rest from the bank account.
    if (donation >= player_gold)
    {
        player_remove_gold(p, player_gold);
        p->bank_account -= (donation - player_gold);
    }
    else
    {
        player_remove_gold(p, donation);
    }
    p->stats.gold_spent_donation += donation;

    /* Increase godly goodwill based on donation amount */
    /* Small donations: +1 per 10 gold, large donations: +1 per 5 gold (capped) */
    int goodwill_increase = donation / (donation > 100 ? 5 : 10);
    p->godly_goodwill += goodwill_increase;

    log_add_entry(nlarn->log, _("You donate %d gold at the altar and pray."),
                  donation);

    log_add_entry(nlarn->log, _("Thank you!"));

    if (p->godly_goodwill > 0) {
        log_add_entry(nlarn->log, _("Your god is pleased with you!"));
    } else {
    log_add_entry(nlarn->log, _("The gods are displeased with you."));
    }

    // The higher the donation, the more likely is a favourable outcome.
    // Ensure at least 2 to allow for positive and negative outcomes
    const int event = min(8, rand_0n(max(2, donation/50 + 1)));

    int afflictions = 0;
    bool cured_affliction = false;
    switch (event)
    {
    case 8:
        if (!player_effect(p, ET_UNDEAD_PROTECTION))
        {
            log_add_entry(nlarn->log, _("You have been heard!"));
            e = effect_new(ET_UNDEAD_PROTECTION);
            player_effect_add(p, e);
            break;
        }
        // intentional fall through
    case 7:
        afflictions += rand_1n(5);
        // intentional fall through
    case 6:
        afflictions += rand_1n(5);
        // intentional fall through
    case 5:
        afflictions++;

        while (afflictions-- > 0)
        {
            if ((e = player_effect_get(p, ET_PARALYSIS)))
            {
                player_effect_del(p, e);
                cured_affliction = true;
                continue;
            }
            if ((e = player_effect_get(p, ET_BLINDNESS)))
            {
                player_effect_del(p, e);
                cured_affliction = true;
                continue;
            }
            if (afflictions >= 5 && (e = player_effect_get(p, ET_DIZZINESS)))
            {
                player_effect_del(p, e);
                cured_affliction = true;
                afflictions -= 5;
                continue;
            }
            if ((e = player_effect_get(p, ET_CONFUSION)))
            {
                player_effect_del(p, e);
                cured_affliction = true;
                continue;
            }
            if ((e = player_effect_get(p, ET_POISON)))
            {
                player_effect_del(p, e);
                cured_affliction = true;
                continue;
            }

            effect_t et;
            for (et = ET_DEC_CON; et <= ET_DEC_WIS; et++)
            {
                if ((e = player_effect_get(p, et)))
                {
                    player_effect_del(p, e);
                    cured_affliction = true;
                    break;
                }
            }
            if (et <= ET_DEC_WIS)
                continue;

            /* nothing else to cure */
            break;
        }

        if (cured_affliction)
            break;

        /* fall-through */
    case 4:
        if (chance(10) && p->eq_weapon)
        {
            if (p->eq_weapon->bonus < 3)
            {
                /* enchant weapon */
                log_add_entry(nlarn->log, _("%s vibrates for a moment."),
                              weapon_name_art(p->eq_weapon,
                                  ART_POSS, GC_NOM, true));

                item_enchant(p->eq_weapon);
                break;
            }
        }
        // intentional fall through
    case 3:
        if (chance(10) && (armour = player_get_random_armour(p, true)))
        {
            if ((*armour)->bonus < 3)
            {
                log_add_entry(nlarn->log, _("%s vibrates for a moment."),
                              armour_name_art(*armour, ART_POSS, GC_NOM, true));

                item_enchant(*armour);
                break;
            }
        }
        // intentional fall through
    case 2:
        if (chance(50) && !player_effect(p, ET_PROTECTION))
        {
            e = effect_new(ET_PROTECTION);
            e->turns = 500;
            player_effect_add(p, e);
            break;
        }
        // intentional fall through
    case 1:
        log_add_entry(nlarn->log, _("Otherwise, nothing seems to have happened."));
        break;
    case 0:
        {
        /* Only create a monster if the gods are truly angered (goodwill < 0) */
        if (p->godly_goodwill < 0)
        {
            /* create a monster, it should be very dangerous */
            position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                              LE_MONSTER, false);

            if (pos_valid(mpos))
                monster_appear(MT_MAX, mpos);

            if (donation < rand_1n(100) || !pos_valid(mpos))
            {
                e = effect_new(ET_AGGRAVATE_MONSTER);
                e->turns = 200;
                player_effect_add(p, e);
            }
        }
        else
        {
            log_add_entry(nlarn->log, _("Otherwise, nothing seems to have happened."));
        }
        }
    }

    return true;
}

int player_building_enter(player *p)
{
    int moves_count = 0;

    switch (map_sobject_at(game_map(nlarn, Z(p->pos)), p->pos))
    {
    case LS_BANK:
    case LS_BANK2:
        moves_count = building_bank(p);
        break;

    case LS_DNDSTORE:
        moves_count = building_dndstore(p);
        break;

    case LS_HOME:
        moves_count = building_home(p);
        break;

    case LS_LRS:
        moves_count = building_lrs(p);
        break;

    case LS_SCHOOL:
        moves_count = building_school(p);
        break;

    case LS_TRADEPOST:
        moves_count = building_tradepost(p);
        break;

    case LS_MONASTERY:
        moves_count = building_monastery(p);
        break;

    default:
        log_add_entry(nlarn->log, _("There is nothing to enter here."));
    }

    return moves_count;
}

int player_door_close(player *p)
{
    if (!player_movement_possible(p))
        return 0;

    /* direction of action */
    direction dir = GD_NONE;

    /* a counter and another one */
    int count, num;

    /* the current map */
    map *pmap = game_map(nlarn, Z(p->pos));

    /* possible directions of actions */
    int *dirs = map_get_surrounding(pmap, p->pos, LS_OPENDOOR);

    for (count = 0, num = 1; num < GD_MAX; num++)
    {
        if (dirs[num])
        {
            count++;
            dir = num;
        }
    }

    if (count > 1)
    {
        dir = display_get_direction(_("Close which door?"), dirs);
    }
    /* dir has been set in the for loop above if count == 1 */
    else if (count == 0)
    {
        dir = GD_NONE;
    }

    g_free(dirs);

    /* select random direction if player is confused */
    if (player_effect(p, ET_CONFUSION))
    {
        dir = rand_0n(GD_MAX);
    }

    if (dir)
    {
        position pos = pos_move(p->pos, dir);
        if (pos_valid(pos) && (map_sobject_at(pmap, pos) == LS_OPENDOOR))
        {

            /* check if player is standing in the door */
            if (pos_identical(pos, p->pos))
            {
                log_add_entry(nlarn->log, _("Please step out of the doorway."));
                return 0;
            }

            /* check for monster in the doorway */
            monster *m = map_get_monster_at(pmap, pos);

            if (m && monster_in_sight(m))
            {
                bool visible = monster_in_sight(m);

                log_add_entry(nlarn->log,
                              _("You cannot close the door. %s is in the way."),
                              monster_get_name_art(m,
                                  visible ? ART_DEF : ART_INDEF, GC_NOM, true));
                return 0;
            }

            /* check for items in the doorway */
            if (m || *map_ilist_at(pmap, pos))
            {
                log_add_entry(nlarn->log,
                              _("You cannot close the door. There is something in the way."));
                return 0;
            }

            map_sobject_set(pmap, pos, LS_CLOSEDDOOR);
            log_add_entry(nlarn->log, _("You close the door."));
        }
        else
        {
            log_add_entry(nlarn->log, _("Huh?"));
            return 0;
        }
    }
    else
    {
        log_add_entry(nlarn->log, _("Which door are you talking about?"));
        return 0;
    }

    return 1;
}

int player_door_open(player *p, int dir)
{
    if (!player_movement_possible(p))
        return 0;

    /* position used to interact with stationaries */

    /* the current map */
    map *pmap = game_map(nlarn, Z(p->pos));

    if (dir == GD_NONE)
    {
        /* number of valid directions for actions */
        int count = 0;

        /* possible directions of actions */
        int *dirs = map_get_surrounding(pmap, p->pos, LS_CLOSEDDOOR);

        for (int num = 1; num < GD_MAX; num++)
        {
            if (dirs[num])
            {
                count++;
                dir = num;
            }
        }

        if (count > 1)
        {
            dir = display_get_direction(_("Open which door?"), dirs);
        }
        /* dir has been set in the for loop above if count == 1 */
        else if (count == 0)
        {
            dir = GD_NONE;
        }

        g_free(dirs);

        /* select random direction if player is confused */
        if (player_effect(p, ET_CONFUSION))
        {
            dir = rand_1n(GD_MAX);
        }
    }

    if (dir)
    {
        position pos = pos_move(p->pos, dir);

        if (pos_valid(pos) && (map_sobject_at(pmap, pos) == LS_CLOSEDDOOR))
        {
            map_sobject_set(pmap, pos, LS_OPENDOOR);
            log_add_entry(nlarn->log, _("You open the door."));
        }
        else
        {
            log_add_entry(nlarn->log, _("Huh?"));
            return 0;
        }
    }
    else
    {
        log_add_entry(nlarn->log, _("What exactly do you want to open?"));
        return 0;
    }

    return 1;
}

int player_fountain_drink(player *p)
{
    g_assert (p != NULL);

    map *pmap = game_map(nlarn, Z(p->pos));

    if (map_sobject_at(pmap, p->pos) == LS_DEADFOUNTAIN)
    {
        log_add_entry(nlarn->log, _("There is no water to drink."));
        return 0;
    }

    if (map_sobject_at(pmap, p->pos) != LS_FOUNTAIN)
    {
        log_add_entry(nlarn->log, _("There is no fountain to drink from here."));
        return 0;
    }

    log_add_entry(nlarn->log, _("You drink from the fountain."));

    gint event = rand_1n(101);
    int amount = 0;
    effect_t et = ET_NONE;

    if (event < 7)
    {
        et = ET_SICKNESS;
    }
    else if (event < 13)
    {
        /* see invisible */
        et = ET_INFRAVISION;
    }
    else if (event < 45)
    {
        log_add_entry(nlarn->log, _("Nothing seems to have happened."));
    }
    else if (chance(67))
    {
        /* positive effect */
        switch (rand_1n(9))
        {
        case 1:
            et = ET_INC_STR;
            break;

        case 2:
            et = ET_INC_INT;
            break;

        case 3:
            et = ET_INC_WIS;
            break;

        case 4:
            et = ET_INC_CON;
            break;

        case 5:
            et = ET_INC_DEX;
            break;

        case 6:
            amount = rand_1n(Z(p->pos) + 1);
            log_add_entry(nlarn->log,
                          ngettext("You gain %d hit point.",
                                   "You gain %d hit points.", amount),
                          amount);

            player_hp_gain(p, amount);
            break;

        case 7:
            amount = rand_1n(Z(p->pos) + 1);
            log_add_entry(nlarn->log,
                          ngettext("You just gained %d mana point.",
                                   "You just gained %d mana points.", amount),
                          amount);

            player_mp_gain(p, amount);
            break;

        case 8:
            amount = 5 * rand_1n((Z(p->pos) + 1) * (Z(p->pos) + 1));

            log_add_entry(nlarn->log, _("You just gained experience."));
            player_exp_gain(p, amount);
            break;
        }
    }
    else
    {
        /* negative effect */
        switch (rand_1n(9))
        {
        case 1:
            et = ET_DEC_STR;
            break;

        case 2:
            et = ET_DEC_INT;
            break;

        case 3:
            et = ET_DEC_WIS;
            break;

        case 4:
            et = ET_DEC_CON;
            break;

        case 5:
            et = ET_DEC_DEX;
            break;

        case 6:
            amount = rand_1n(Z(p->pos) + 1);
            log_add_entry(nlarn->log,
                          ngettext("You lose %d hit point!",
                                   "You lose %d hit points!", amount),
                          amount);

            player_hp_lose(p, amount, PD_SOBJECT, LS_FOUNTAIN);
            break;
        case 7:
            amount = rand_1n(Z(p->pos) + 1);
            log_add_entry(nlarn->log,
                          ngettext("You just lost %d mana point.",
                                   "You just lost %d mana points.", amount),
                          amount);

            player_mp_lose(p, amount);
            break;

        case 8:
            amount = 5 * rand_1n((Z(p->pos) + 1) * (Z(p->pos) + 1));

            log_add_entry(nlarn->log, _("You just lost experience."));
            player_exp_lose(p, amount);
            break;
        }
    }

    /* Create an effect if the RNG decided to */
    if (et)
    {
        player_effect_add(p, effect_new(et));
    }

    if (chance(25))
    {
        log_add_entry(nlarn->log, _("The fountain's bubbling slowly quiets."));
        map_sobject_set(pmap, p->pos, LS_DEADFOUNTAIN);
    }

    return 1;
}

int player_fountain_wash(player *p)
{
    g_assert (p != NULL);

    map *pmap = game_map(nlarn, Z(p->pos));

    if (map_sobject_at(pmap, p->pos) == LS_DEADFOUNTAIN)
    {
        log_add_entry(nlarn->log, _("The fountain is dry."));
        return 0;
    }

    if (map_sobject_at(pmap, p->pos) != LS_FOUNTAIN)
    {
        log_add_entry(nlarn->log, _("There is no fountain to wash at here."));
        return 0;
    }

    log_add_entry(nlarn->log, _("You wash yourself at the fountain."));

    if (chance(10))
    {
        log_add_entry(nlarn->log, _("Oh no! The water was foul!"));

        damage *dam = damage_new(DAM_POISON, ATT_NONE,
                                 rand_1n((Z(p->pos) << 2) + 2),
                                 DAMO_SOBJECT, NULL);

        player_damage_take(p, dam, PD_SOBJECT, LS_FOUNTAIN);

        return 1;
    }
    else if (chance(60))
    {
        effect *e = NULL;
        if ((e = player_effect_get(p, ET_ITCHING)))
        {
            if (chance(50))
            {
                log_add_entry(nlarn->log, _("You got the dirt off!"));
                player_effect_del(p, e);
            }
            else
            {
                log_add_entry(nlarn->log,
                      _("This water seems to be hard water! "
                        "The dirt didn't come off!"));
            }

            return 1;
        }
    }
    else if (chance(35))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(pmap, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, false);

        if (pos_valid(mpos))
        {
            /* make water lord */
            monster_appear(MT_WATER_LORD, mpos);
        }

        return 1;
    }

    log_add_entry(nlarn->log, _("Nothing seems to have happened."));

    return 1;
}

int player_stairs_down(player *p)
{
    map *nlevel = NULL;
    bool show_msg = false;
    map *pmap = game_map(nlarn, Z(p->pos));
    sobject_t ms = map_sobject_at(pmap, p->pos);

    if (!player_movement_possible(p))
        return false;

    /* the stairs down are unreachable while levitating */
    if (player_effect(p, ET_LEVITATION))
    {
        log_add_entry(nlarn->log, _("You cannot reach the stairs."));
        return false;
    }

    switch (ms)
    {
    case LS_STAIRSDOWN:
        show_msg = true;
        nlevel = game_map(nlarn, Z(p->pos) + 1);;
        break;

    case LS_ELEVATORDOWN:
        /* first volcano map */
        show_msg = true;
        nlevel = game_map(nlarn, MAP_CMAX);
        break;

    case LS_CAVERNS_ENTRY:
        if (Z(p->pos) == 0)
            nlevel = game_map(nlarn, 1);
        else
            log_add_entry(nlarn->log, _("Climb up to return to town."));
        break;

    default:
    {
        trap_t trap = map_trap_at(pmap, p->pos);
        if (trap == TT_TRAPDOOR || trap == TT_TELEPORT)
            return player_trap_trigger(p, trap, true);
        else
            return false;
    }
    }

    /* display additional message */
    if (show_msg)
    {
        log_add_entry(nlarn->log, _("You climb down %s."),
                noun_phrase(so_get_desc_raw(ms), ART_NONE, GC_ACC, false, false));
    }

    /* if told to switch level, do so */
    if (nlevel != NULL)
    {
        /* check if the player is burdened and cause some damage if so */
        int bval = player_effect(p, ET_BURDENED);

        if (bval > 0)
        {
            log_add_entry(nlarn->log, _("You slip!"));
            damage *dam = damage_new(DAM_PHYSICAL, ATT_NONE,
                                     rand_1n(bval + nlevel->nlevel),
                                     DAMO_SOBJECT, NULL);

            player_damage_take(p, dam, PD_SOBJECT, ms);
        }

        return player_map_enter(p, nlevel, false);
    }

    return 0;
}

int player_stairs_up(player *p)
{
    map *nlevel = NULL;
    bool show_msg = false;
    sobject_t ms = map_sobject_at(game_map(nlarn, Z(p->pos)), p->pos);

    if (!player_movement_possible(p))
        return 0;

    switch (ms)
    {
    case LS_STAIRSUP:
        show_msg = true;
        nlevel = game_map(nlarn, Z(p->pos) - 1);
        break;

    case LS_ELEVATORUP:
    case LS_CAVERNS_EXIT:
        /* return to town */
        show_msg = true;
        nlevel = game_map(nlarn, 0);
        break;

    case LS_CAVERNS_ENTRY:
        log_add_entry(nlarn->log, _("Climb down to enter the caverns."));
        return 0;

    default:
        log_add_entry(nlarn->log, _("I see no stairway up here."));
        return 0;
    }

    /* display additional message */
    if (show_msg)
    {
        log_add_entry(nlarn->log, _("You climb up %s."),
                noun_phrase(so_get_desc_raw(ms), ART_NONE, GC_ACC, false, false));
    }

    /* if told to switch level, do so */
    if (nlevel != NULL)
    {
        return player_map_enter(p, nlevel, false);
    }

    return 0;
}

int player_throne_pillage(player *p)
{
    g_assert (p != NULL);

    int count = 0; /* gems created */

    /* current map */
    map *pmap = game_map(nlarn, Z(p->pos));

    /* type of object at player's position */
    sobject_t ms = map_sobject_at(pmap, p->pos);

    if ((ms != LS_THRONE) && (ms != LS_THRONE2) && (ms != LS_DEADTHRONE))
    {
        log_add_entry(nlarn->log, _("There is no throne here."));
        return 0;
    }

    log_add_entry(nlarn->log, _("You try to remove the gems from the throne."));

    if (ms == LS_DEADTHRONE)
    {
        log_add_entry(nlarn->log, _("There are no gems left on this throne."));
        return 0;
    }

    if (chance(2 * player_get_dex(p)))
    {
        for (guint i = 0; i < rand_1n(4); i++)
        {
            /* gems pop off the throne */
            inv_add(map_ilist_at(pmap, p->pos), item_new_random(IT_GEM, false));
            count++;
        }

        log_add_entry(nlarn->log,
                      count > 1 ? _("You manage to pry off some gems.")
                                : _("You manage to pry off a gem."));

        map_sobject_set(pmap, p->pos, LS_DEADTHRONE);
        p->stats.vandalism++;
    }
    else if (chance(40) && (ms == LS_THRONE))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(pmap, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, false);

        if (pos_valid(mpos))
        {
            /* make gnome king */
            monster_appear(MT_GNOME_KING, mpos);

            /* next time there will be no gnome king */
            map_sobject_set(pmap, p->pos, LS_THRONE2);
        }
    }
    else
    {
        log_add_entry(nlarn->log, _("You fail to remove the gems."));
    }

    return 1 + count;
}

int player_throne_sit(player *p)
{
    g_assert (p != NULL);

    map *pmap = game_map(nlarn, Z(p->pos));
    sobject_t st = map_sobject_at(pmap, p->pos);

    if ((st != LS_THRONE) && (st != LS_THRONE2) && (st != LS_DEADTHRONE))
    {
        log_add_entry(nlarn->log, _("There is no throne here."));
        return 0;
    }

    log_add_entry(nlarn->log, _("You sit on the throne."));

    if (chance(30) && (st == LS_THRONE))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(pmap, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, false);

        if (pos_valid(mpos))
        {
            /* make a gnome king */
            monster_appear(MT_GNOME_KING, mpos);

            /* next time there will be no gnome king */
            map_sobject_set(pmap, p->pos, LS_THRONE2);
        }
    }
    else if (chance(35))
    {
        log_add_entry(nlarn->log, _("Zaaaappp! You've been teleported!"));
        p->pos = map_find_space(pmap, LE_MONSTER, false);
    }
    else
    {
        log_add_entry(nlarn->log, _("Nothing seems to have happened."));
    }

    return 1;
}

void sobject_destroy_at(player *p, map *dmap, position pos)
{
    const char *desc = NULL;

    /* position for a monster that might be generated */
    position mpos = map_find_space_in(dmap, rect_new_sized(pos, 1), LE_MONSTER, false);

    switch (map_sobject_at(dmap, pos))
    {
    case LS_NONE:
        /* NOP */
        break;

    case LS_ALTAR:
    {
        log_add_entry(nlarn->log, _("You destroy the altar."));
        map_sobject_set(dmap, pos, LS_NONE);
        p->stats.vandalism++;

        log_add_entry(nlarn->log, _("Lightning comes crashing down from above!"));

        /* flood the area surrounding the altar with lightning */
        damage_originator damo = { DAMO_GOD, NULL };
        damage *dam = damage_new(DAM_ELECTRICITY, ATT_MAGIC,
                                 25 + p->level + rand_0n(25 + p->level),
                                 damo.ot, damo.originator);

        area_blast(pos, 3, &damo, sobject_blast_hit, dam, NULL, '*', ELECTRIC_INDIGO);
        damage_free(dam);

        break;
    }

    case LS_FOUNTAIN:
        log_add_entry(nlarn->log, _("You destroy the fountain."));
        map_sobject_set(dmap, pos, LS_NONE);
        p->stats.vandalism++;

        /* create a permanent shallow pool and place a water lord */
        log_add_entry(nlarn->log, _("A flood of water gushes forth!"));
        flood_affect_area(pos, 3 + rand_0n(2), LT_WATER, 0);
        if (pos_valid(mpos))
            monster_new(MT_WATER_LORD, mpos, NULL);
        break;

    case LS_STATUE:
        /* chance of finding a book:
           diff 0-1: 100%, diff 2: 2/3, diff 3: 50%, ..., diff N: 2/(N+1) */
        if (rand_0n(game_difficulty(nlarn)+1) <= 1)
        {
            item *it = item_new(IT_BOOK, rand_0n(item_max_id(IT_BOOK)));
            inv_add(map_ilist_at(dmap, pos), it);
        }

        desc = N_("statue");
        break;

    case LS_THRONE:
    case LS_THRONE2:
        if (pos_valid(mpos))
            monster_new(MT_GNOME_KING, mpos, NULL);

        desc = N_("throne");
        break;

    case LS_DEADFOUNTAIN:
    case LS_DEADTHRONE:
        map_sobject_set(dmap, pos, LS_NONE);
        break;

    case LS_CLOSEDDOOR:
        map_sobject_set(dmap, pos, LS_NONE);

        if(fov_get(p->fv, pos))
        {
            log_add_entry(nlarn->log, _("The door shatters!"));
        }
        break;

    default:
        log_add_entry(nlarn->log, _("Somehow that did not work."));
        /* NOP */
    }

    if (desc)
    {
        log_add_entry(nlarn->log, _("You destroy the %s."), _(desc));
        map_sobject_set(dmap, pos, LS_NONE);
        p->stats.vandalism++;
    }
}

static void monster_appear(monster_t type, position mpos)
{
    monster *m;

    if (type == MT_MAX)
    {
        m = monster_new_by_level(mpos);
    }
    else
    {
        m = monster_new(type, mpos, NULL);
    }

    if (m && monster_in_sight(m))
    {
        log_add_entry(nlarn->log, _("%s appears, looking angry!"),
                      monster_name_art(m, ART_INDEF, GC_NOM, true));
    }
    else
        log_add_entry(nlarn->log, _("Nothing seems to have happened."));
}

static void flood_affect_area(position pos, int radius, int type, int duration)
{
    area *obstacles = map_get_obstacles(game_map(nlarn, Z(pos)), pos, radius, false);
    area *range = area_new_circle_flooded(pos, radius, obstacles);

    map_set_tiletype(game_map(nlarn, Z(pos)), range, type, duration);
    area_destroy(range);
}

static bool sobject_blast_hit(position pos,
                              const damage_originator *damo,
                              gpointer data1,
                              gpointer data2 __attribute__((unused)))
{
    damage *dam = (damage *)data1;
    map *cmap = game_map(nlarn, Z(pos));
    sobject_t mst = map_sobject_at(cmap, pos);
    monster *m = map_get_monster_at(cmap, pos);

    if (mst == LS_STATUE)
    {
        /* The blast hit a statue. */
        sobject_destroy_at(damo->originator, cmap, pos);
        return true;
    }
    else if (m != NULL)
    {
        /* the blast hit a monster */
        if (monster_in_sight(m))
            log_add_entry(nlarn->log, _("The lightning hits %s."),
                          monster_get_name_art(m, ART_DEF, GC_ACC, false));

        monster_damage_take(m, damage_copy(dam));

        return true;
    }
    else if (pos_identical(nlarn->p->pos, pos))
    {
        /* the blast hit the player */
        int evasion = nlarn->p->level/(2+game_difficulty(nlarn)/2)
                      + player_get_dex(nlarn->p)
                      - 10
                      - game_difficulty(nlarn);

        // Automatic hit if paralysed or overstrained.
        if (player_effect(nlarn->p, ET_PARALYSIS)
            || player_effect(nlarn->p, ET_OVERSTRAINED))
            evasion = 0;
        else
        {
            if (player_effect(nlarn->p, ET_BLINDNESS))
                evasion /= 4;
            if (player_effect(nlarn->p, ET_CONFUSION))
                evasion /= 2;
            if (player_effect(nlarn->p, ET_BURDENED))
                evasion /= 2;
        }

        if (evasion >= (int)rand_1n(21))
        {
            if (!player_effect(nlarn->p, ET_BLINDNESS))
                log_add_entry(nlarn->log, _("The lightning whizzes by you!"));

            /* missed */
            return false;
        }

        log_add_entry(nlarn->log, _("The lightning hits you!"));
        /* FIXME: correctly state that the player has been killed by the
                  wrath of a god */
        player_damage_take(nlarn->p, damage_copy(dam), PD_SPELL, SP_LIT);

        /* hit */
        return true;
    }

    return false;
}
