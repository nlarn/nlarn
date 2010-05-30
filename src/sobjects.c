/*
 * sobjects.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

#include <assert.h>

#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "sobjects.h"
#include "player.h"

static void monster_appear(monster_t type, position mpos);


int player_altar_desecrate(player *p)
{
    effect *e = NULL;
    map *current;

    assert (p != NULL);

    current = game_map(nlarn, p->pos.z);

    if (map_sobject_at(current, p->pos) != LS_ALTAR)
    {
        log_add_entry(nlarn->log, "There is no altar here.");
        return FALSE;
    }

    log_add_entry(nlarn->log, "You try to desecrate the altar.");

    if (chance(60))
    {
        /* try to find a space for the monster near the altar */
        position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, FALSE);

        if (pos_valid(mpos))
        {
            /* create a monster - should be very dangerous */
            monster_appear(MT_RED_DRAGON, mpos);
        }

        e = effect_new(ET_AGGRAVATE_MONSTER);
        e->turns = 2500;
        player_effect_add(p, e);
    }
    else if (chance(30))
    {
        /* destroy altar */
        log_add_entry(nlarn->log, "The altar crumbles into a pile of dust before your eyes.");
        map_sobject_set(current, p->pos, LS_NONE);
    }
    else
    {
        log_add_entry(nlarn->log, "You fail to destroy the altar.");
    }

    return TRUE;
}

int player_altar_pray(player *p)
{
    effect *e = NULL;
    item **armour = NULL;
    map *current;

    assert (p != NULL);

    current = game_map(nlarn, p->pos.z);

    if (map_sobject_at(current, p->pos) != LS_ALTAR)
    {
        log_add_entry(nlarn->log, "There is no altar here.");
        return FALSE;
    }

    const int player_gold = player_get_gold(p);
    const int total_gold  = player_gold + p->bank_account;
    if (total_gold == 0)
    {
        log_add_entry(nlarn->log, "You don't have any money to donate.");
        return FALSE;
    }

    // Use a sensible default value, so you don't anger the gods without
    // meaning to.
    const int donation = display_get_count("How much gold do you want to donate?",
                                           200);

    /* 0 gold donations are likely to be the result of escaping the prompt */
    if (!donation)
    {
        log_add_entry(nlarn->log, "So you decide not to donate anything.");
        return FALSE;
    }

    if (donation > total_gold)
    {
        log_add_entry(nlarn->log, "You don't have that much money!");
        return FALSE;
    }

    // First pay with carried gold, then pay the rest from the bank account.
    if (donation >= player_gold)
    {
        player_set_gold(p, 0);
        p->bank_account -= (donation - player_gold);
    }
    else
    {
        player_set_gold(p, player_gold - donation);
    }
    p->stats.gold_spent_donation += donation;

    log_add_entry(nlarn->log, "You donate %d gold at the altar and pray.",
                  donation);
    display_paint_screen(p);

    // The higher the donation, the more likely is a favourable outcome.
    const int event = min(8, rand_0n(donation/50));

    if (event > 0)
        log_add_entry(nlarn->log, "Thank you!");
    else
        log_add_entry(nlarn->log, "The gods are displeased with you.");

    int afflictions = 0;
    gboolean cured_affliction = FALSE;
    switch (event)
    {
    case 8:
        if (!player_effect(p, ET_UNDEAD_PROTECTION))
        {
            log_add_entry(nlarn->log, "You have been heard!");
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
                cured_affliction = TRUE;
                continue;
            }
            if ((e = player_effect_get(p, ET_BLINDNESS)))
            {
                player_effect_del(p, e);
                cured_affliction = TRUE;
                continue;
            }
            if (afflictions >= 5 && (e = player_effect_get(p, ET_DIZZINESS)))
            {
                player_effect_del(p, e);
                cured_affliction = TRUE;
                afflictions -= 5;
                continue;
            }
            if ((e = player_effect_get(p, ET_CONFUSION)))
            {
                player_effect_del(p, e);
                cured_affliction = TRUE;
                continue;
            }
            if ((e = player_effect_get(p, ET_POISON)))
            {
                player_effect_del(p, e);
                cured_affliction = TRUE;
                continue;
            }

            gint32 et;
            for (et = ET_DEC_CON; et <= ET_DEC_WIS; et++)
            {
                if ((e = player_effect_get(p, et)))
                {
                    player_effect_del(p, e);
                    cured_affliction = TRUE;
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

        // else intentional fall through
    case 4:
        if (chance(10) && p->eq_weapon)
        {
            if (p->eq_weapon->bonus < 3)
            {
                /* enchant weapon */
                log_add_entry(nlarn->log, "Your %s vibrates for a moment.",
                              weapon_name(p->eq_weapon));

                item_enchant(p->eq_weapon);
                break;
            }
        }
        // intentional fall through
    case 3:
        if (chance(10) && (armour = player_get_random_armour(p)))
        {
            if ((*armour)->bonus < 3)
            {
                log_add_entry(nlarn->log, "Your %s vibrates for a moment.",
                              armour_name(*armour));

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
        log_add_entry(nlarn->log, "Nothing seems to have happened.");
        break;
    case 0:
        {
        /* create a monster, it should be very dangerous */
        position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, FALSE);

        if (pos_valid(mpos))
            monster_appear(MT_MAX, mpos);

        if (donation < rand_1n(100) || !pos_valid(mpos))
        {
            e = effect_new(ET_AGGRAVATE_MONSTER);
            e->turns = 200;
            player_effect_add(p, e);
        }
        }
    }

    return TRUE;
}

int player_building_enter(player *p)
{
    int moves_count = 0;

    switch (map_sobject_at(game_map(nlarn, p->pos.z), p->pos))
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

    default:
        log_add_entry(nlarn->log, "There is nothing to enter here.");
    }

    return moves_count;
}

int player_door_close(player *p)
{
    if (!player_movement_possible(p))
        return 0;

    /* position used to interact with stationaries */
    position pos;

    /* possible directions of actions */
    int *dirs;

    /* direction of action */
    direction dir = GD_NONE;

    /* a counter and another one */
    int count, num;

    /* a monster */
    monster *m;

    /* the current map */
    map *map = game_map(nlarn, p->pos.z);

    dirs = map_get_surrounding(map, p->pos, LS_OPENDOOR);

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
        dir = display_get_direction("Close which door?", dirs);
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
        pos = pos_move(p->pos, dir);
        if (pos_valid(pos) && (map_sobject_at(map, pos) == LS_OPENDOOR))
        {

            /* check if player is standing in the door */
            if (pos_identical(pos, p->pos))
            {
                log_add_entry(nlarn->log, "Please step out of the doorway.");
                return 0;
            }

            /* check for monster in the doorway */
            m = map_get_monster_at(map, pos);

            if (m && monster_in_sight(m))
            {
                gboolean visible = monster_in_sight(m);

                log_add_entry(nlarn->log,
                              "You cannot close the door. %s %s is in the way.",
                              visible ? "The" : "An", monster_get_name(m));
                return 0;
            }

            /* check for items in the doorway */
            if (m || *map_ilist_at(map, pos))
            {
                log_add_entry(nlarn->log,
                              "You cannot close the door. There is something in the way.");
                return 0;
            }

            map_sobject_set(map, pos, LS_CLOSEDDOOR);
            log_add_entry(nlarn->log, "You close the door.");
        }
        else
        {
            log_add_entry(nlarn->log, "Huh?");
            return 0;
        }
    }
    else
    {
        log_add_entry(nlarn->log, "Which door are you talking about?");
        return 0;
    }

    return 1;
}

int player_door_open(player *p, int dir)
{
    if (!player_movement_possible(p))
        return 0;

    /* position used to interact with stationaries */
    position pos;

    /* a counter and another one */
    int count, num;

    /* the current map */
    map *map = game_map(nlarn, p->pos.z);

    if (dir == GD_NONE)
    {
        /* possible directions of actions */
        int *dirs = map_get_surrounding(map, p->pos, LS_CLOSEDDOOR);

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
            dir = display_get_direction("Open which door?", dirs);
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
        pos = pos_move(p->pos, dir);

        if (pos_valid(pos) && (map_sobject_at(map, pos) == LS_CLOSEDDOOR))
        {
            map_sobject_set(map, pos, LS_OPENDOOR);
            log_add_entry(nlarn->log, "You open the door.");
        }
        else
        {
            log_add_entry(nlarn->log, "Huh?");
            return 0;
        }
    }
    else
    {
        log_add_entry(nlarn->log, "What exactly do you want to open?");
        return 0;
    }

    return 1;
}

int player_fountain_drink(player *p)
{
    effect *e = NULL;

    int fntchange = 0;
    int amount = 0;
    int et = ET_NONE;
    map *map = game_map(nlarn, p->pos.z);

    assert (p != NULL);

    if (map_sobject_at(map, p->pos) == LS_DEADFOUNTAIN)
    {
        log_add_entry(nlarn->log, "There is no water to drink.");
        return 0;
    }

    if (map_sobject_at(map, p->pos) != LS_FOUNTAIN)
    {
        log_add_entry(nlarn->log, "There is no fountain to drink from here.");
        return 0;
    }

    log_add_entry(nlarn->log, "You drink from the fountain.");

    if (chance(7))
    {
        e = effect_new(ET_SICKNESS);
        player_effect_add(p, e);
    }
    else if (chance(13))
    {
        /* see invisible */
        e = effect_new(ET_INFRAVISION);
        player_effect_add(p, e);
    }
    else if (chance(45))
    {
        log_add_entry(nlarn->log, "Nothing seems to have happened.");
    }
    else if (chance(15))
    {
        /* positive effect from list below */
        fntchange = 1;
    }
    else
    {
        /* negative effect from list below */
        fntchange = -1;
    }

    if (fntchange != 0)
    {
        switch (rand_1n(9))
        {
        case 1:
            if (fntchange > 0)
                et = ET_INC_STR;
            else
                et = ET_DEC_STR;
            break;

        case 2:
            if (fntchange > 0)
                et = ET_INC_INT;
            else
                et = ET_DEC_INT;
            break;

        case 3:
            if (fntchange > 0)
                et = ET_INC_WIS;
            else
                et = ET_DEC_WIS;
            break;

        case 4:
            if (fntchange > 0)
                et = ET_INC_CON;
            else
                et = ET_DEC_CON;
            break;

        case 5:
            if (fntchange > 0)
                et = ET_INC_DEX;
            else
                et = ET_DEC_DEX;
            break;

        case 6:
            amount = rand_1n(p->pos.z + 1);
            if (fntchange > 0)
            {
                log_add_entry(nlarn->log, "You gain %d hit point%s",
                              amount, plural(amount));

                player_hp_gain(p, amount);
            }
            else
            {
                log_add_entry(nlarn->log, "You lose %d hit point%s!",
                              amount, plural(amount));

                damage *dam = damage_new(DAM_NONE, ATT_NONE, amount, NULL);
                player_damage_take(p, dam, PD_SOBJECT, LS_FOUNTAIN);
            }

            break;

        case 7:
            amount = rand_1n(p->pos.z + 1);
            if (fntchange > 0)
            {
                log_add_entry(nlarn->log, "You just gained %d mana point%s.",
                              amount, plural(amount));

                player_mp_gain(p, amount);
            }
            else
            {
                log_add_entry(nlarn->log, "You just lost %d mana point%s.",
                              amount, plural(amount));

                player_mp_lose(p, amount);
            }
            break;

        case 8:
            amount = 5 * rand_1n((p->pos.z + 1) * (p->pos.z + 1));

            if (fntchange > 0)
            {
                log_add_entry(nlarn->log, "You just gained experience.");
                player_exp_gain(p, amount);
            }
            else
            {
                log_add_entry(nlarn->log, "You just lost experience.");
                player_exp_lose(p, amount);
            }
            break;
        }

        /* the rng stated that it wants the players attributes changed */
        if (et)
        {
            e = effect_new(et);
            player_effect_add(p, e);
        }
    }

    if (chance(25))
    {
        log_add_entry(nlarn->log, "The fountains bubbling slowly quiets.");
        map_sobject_set(map, p->pos, LS_DEADFOUNTAIN);
    }

    return 1;
}

int player_fountain_wash(player *p)
{
    map *map = game_map(nlarn, p->pos.z);

    assert (p != NULL);

    if (map_sobject_at(map, p->pos) == LS_DEADFOUNTAIN)
    {
        log_add_entry(nlarn->log, "The fountain is dry.");
        return 0;
    }

    if (map_sobject_at(map, p->pos) != LS_FOUNTAIN)
    {
        log_add_entry(nlarn->log, "There is no fountain to wash at here.");
        return 0;
    }

    log_add_entry(nlarn->log, "You wash yourself at the fountain.");

    if (chance(10))
    {
        log_add_entry(nlarn->log, "Oh no! The water was foul!");

        damage *dam = damage_new(DAM_POISON, ATT_NONE,
                                 rand_1n((p->pos.z << 2) + 2), NULL);

        player_damage_take(p, dam, PD_SOBJECT, LS_FOUNTAIN);
    }
    else if (chance(30))
    {
        effect *e = NULL;
        if ((e = player_effect_get(p, ET_ITCHING)))
        {
            log_add_entry(nlarn->log, "You got the dirt off!");
            player_effect_del(p, e);
        }
    }
    else if (chance(30))
    {
        log_add_entry(nlarn->log, "This water seems to be hard water! " \
                      "The dirt didn't come off!");
    }
    else if (chance(35))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(map, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, FALSE);

        if (pos_valid(mpos))
        {
            /* make water lord */
            monster_appear(MT_WATER_LORD, mpos);
        }
    }
    else
    {
        log_add_entry(nlarn->log, "Nothing seems to have happened.");
    }

    return 1;
}

int player_stairs_down(player *p)
{
    map *nlevel = NULL;
    gboolean show_msg = FALSE;
    map *map = game_map(nlarn, p->pos.z);
    map_sobject_t ms = map_sobject_at(map, p->pos);

    if (!player_movement_possible(p))
        return FALSE;

    /* the stairs down are unreachable while levitating */
    if (player_effect(p, ET_LEVITATION))
    {
        log_add_entry(nlarn->log, "You cannot reach reach the stairs..");
        return FALSE;
    }

    switch (ms)
    {
    case LS_STAIRSDOWN:
        show_msg = TRUE;
        nlevel = game_map(nlarn, p->pos.z + 1);;
        break;

    case LS_ELEVATORDOWN:
        /* first vulcano map */
        show_msg = TRUE;
        nlevel = game_map(nlarn, MAP_DMAX);
        break;

    case LS_DNGN_ENTRANCE:
        if (p->pos.z == 0)
            nlevel = game_map(nlarn, 1);
        else
            log_add_entry(nlarn->log, "Climb up to return to town.");
        break;

    default:
    {
        trap_t trap = map_trap_at(map, p->pos);
        if (trap == TT_TRAPDOOR)
            return player_trap_trigger(p, trap, TRUE);
        else
            return FALSE;
    }
    }

    /* display additional message */
    if (show_msg)
    {
        log_add_entry(nlarn->log, "You climb down %s.", ls_get_desc(ms));
    }

    /* if told to switch level, do so */
    if (nlevel != NULL)
    {
        return player_map_enter(p, nlevel, FALSE);
    }

    return 0;
}

int player_stairs_up(player *p)
{
    map *nlevel = NULL;
    gboolean show_msg = FALSE;
    map_sobject_t ms = map_sobject_at(game_map(nlarn, p->pos.z), p->pos);

    if (!player_movement_possible(p))
        return 0;

    switch (ms)
    {
    case LS_STAIRSUP:
        show_msg = TRUE;
        nlevel = game_map(nlarn, p->pos.z - 1);
        break;

    case LS_ELEVATORUP:
    case LS_DNGN_EXIT:
        /* return to town */
        show_msg = TRUE;
        nlevel = game_map(nlarn, 0);
        break;

    case LS_DNGN_ENTRANCE:
        log_add_entry(nlarn->log, "Climb down to enter the caverns.");
        return 0;

    default:
        log_add_entry(nlarn->log, "I see no stairway up here.");
        return 0;
    }

    /* display additional message */
    if (show_msg)
    {
        log_add_entry(nlarn->log, "You climb up %s.", ls_get_desc(ms));
    }

    /* if told to switch level, do so */
    if (nlevel != NULL)
    {
        return player_map_enter(p, nlevel, FALSE);
    }

    return 0;
}

int player_throne_pillage(player *p)
{
    int i;
    int count = 0; /* gems created */

    /* current map */
    map *map = game_map(nlarn, p->pos.z);

    /* type of object at player's position */
    map_sobject_t ms = map_sobject_at(map, p->pos);

    assert (p != NULL);

    if ((ms != LS_THRONE) && (ms != LS_THRONE2) && (ms != LS_DEADTHRONE))
    {
        log_add_entry(nlarn->log, "There is no throne here.");
        return 0;
    }

    log_add_entry(nlarn->log, "You try to remove the gems from the throne.");

    if (ms == LS_DEADTHRONE)
    {
        log_add_entry(nlarn->log, "There are no gems left on this throne.");
        return 0;
    }

    if (chance(2*player_get_dex(p)))
    {
        for (i = 0; i < rand_1n(4); i++)
        {
            /* gems pop off the throne */
            inv_add(map_ilist_at(map, p->pos), item_new_random(IT_GEM));
            count++;
        }

        log_add_entry(nlarn->log, "You manage to pry off %s gem%s.",
                      count > 1 ? "some" : "a", plural(count));

        map_sobject_set(map, p->pos, LS_DEADTHRONE);
        p->stats.vandalism++;
    }
    else if (chance(40) && (ms == LS_THRONE))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(map, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, FALSE);

        if (pos_valid(mpos))
        {
            /* make gnome king */
            monster_appear(MT_GNOME_KING, mpos);

            /* next time there will be no gnome king */
            map_sobject_set(map, p->pos, LS_THRONE2);
        }
    }
    else
    {
        log_add_entry(nlarn->log, "You fail to remove the gems.");
    }

    return 1 + count;
}

int player_throne_sit(player *p)
{
    map *map = game_map(nlarn, p->pos.z);
    map_sobject_t st = map_sobject_at(map, p->pos);

    assert (p != NULL);

    if ((st != LS_THRONE) && (st != LS_THRONE2) && (st != LS_DEADTHRONE))
    {
        log_add_entry(nlarn->log, "There is no throne here.");
        return 0;
    }

    log_add_entry(nlarn->log, "You sit on the throne.");

    if (chance(30) && (st == LS_THRONE))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(map, rect_new_sized(p->pos, 1),
                                          LE_MONSTER, FALSE);

        if (pos_valid(mpos))
        {
            /* make a gnome king */
            monster_appear(MT_GNOME_KING, mpos);

            /* next time there will be no gnome king */
            map_sobject_set(map, p->pos, LS_THRONE2);
        }
    }
    else if (chance(35))
    {
        log_add_entry(nlarn->log, "Zaaaappp! You've been teleported!");
        p->pos = map_find_space(map, LE_MONSTER, FALSE);
    }
    else
    {
        log_add_entry(nlarn->log, "Nothing seems to have happened.");
    }

    return 1;
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
        m = monster_new(type, mpos);
    }

    if (m && monster_in_sight(m))
    {
        log_add_entry(nlarn->log, "An angry %s appears!",
                      monster_name(m));
    }
    else
        log_add_entry(nlarn->log, "Nothing seems to have happened.");
}
