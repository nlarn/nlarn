/*
 * buildings.c
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

#include <stdlib.h>
#include <assert.h>

#include "display.h"
#include "gems.h"
#include "items.h"
#include "nlarn.h"
#include "player.h"
#include "scrolls.h"

static const char msg_outstanding[] = "The Nlarn Revenue Service has ordered " \
                                      "us to not do business with tax evaders. " \
                                      "They have also told us that you owe back " \
                                      "taxes and, as we must comply with the " \
                                      "law, we cannot serve you at this time." \
                                      "\n\nSo Sorry.";

static void building_shop(player *p, inventory **inv, const char *title);
static int building_player_check(player *p, guint amount);
static void building_player_charge(player *p, guint amount);

static void building_item_add(inventory **inv, item *it);
static void building_item_sell(player *p, inventory **inv, item *it);
static void building_item_identify(player *p, inventory **inv, item *it);
static void building_item_repair(player *p, inventory **inv, item *it);
static void building_item_buy(player *p, inventory **inv, item *it);

static int handle_bank_interest(player *p)
{
    if (p->bank_account < 250)
        return 0;

    /* pay interest */
    if (nlarn->gtime <= p->interest_lasttime)
        return 0;

    const int mobuls = (nlarn->gtime - p->interest_lasttime) / 100;
    if (mobuls <= 0)
        return 0;

    /* store original account */
    const int orig_account = p->bank_account;

    int i;
    for (i = 1; i <= mobuls; i++)
        p->bank_account += p->bank_account / 250;

    p->interest_lasttime = game_turn(nlarn);

    /* calculate interest paid */
    const int interest = (p->bank_account - orig_account);
    p->stats.gold_bank_interest += interest;

    return interest;
}

int building_bank(player *p)
{
    int turns = 2;
    int cmd;
    guint amount = 0;

    GPtrArray *callbacks = NULL;
    display_inv_callback *callback;

    GString *text;

    const char msg_title[] = "First National Bank of Larn";
    const char msg_greet[] = "Welcome to the First National Bank of Larn.\n\n";

    const char msg_branch[] = "Welcome to the 5th level branch office of the " \
                              "First National Bank of Larn.\n\n";

    const char msg_frozen[] = "The Larn Revenue Service has ordered that your " \
                              "account be frozen until all levied taxes have " \
                              "been paid.  They have also told us that you " \
                              "owe %d gp in taxes, and we must comply with " \
                              "them. We cannot serve you at this time.  Sorry.\n\n" \
                              "We suggest you go to the LRS office and pay your taxes.";

    assert(p != NULL);

    if (p->pos.z == 0)
        text = g_string_new(msg_greet);
    else
        text = g_string_new(msg_branch);

    /* leave bank when taxes are unpaid */
    if (p->outstanding_taxes)
    {
        g_string_append_printf(text, msg_frozen, p->outstanding_taxes);
        display_show_message(msg_title, text->str, 0);
        g_string_free(text, TRUE);

        return turns;
    }

    int interest = handle_bank_interest(p);
    if (interest > 0)
    {
        g_string_append_printf(text, "We have paid you an interest of %d " \
                               "gold since your last visit.\n", interest);
    }

    g_string_append_printf(text,
                           "You have %d gold pieces in the bank.\n" \
                           "You have %d gold pieces.\n\n",
                           p->bank_account,
                           player_get_gold(p));

    g_string_append(text, "Your wish? ");

    if (player_get_gold(p) > 0)
        g_string_append(text, "d)eposit ");

    if (p->bank_account > 0)
        g_string_append(text, "w)ithdraw ");

    /* if player has gems, enable selling them */
    if (inv_length_filtered(p->inventory, item_filter_gems))
    {
        g_string_append(text, "s)ell a gem");

        /* define callback functions */
        callbacks = g_ptr_array_new();

        callback = g_malloc(sizeof(display_inv_callback));
        callback->description = "(s)ell";
        callback->key = 's';
        callback->inv = &nlarn->store_stock;
        callback->function = &building_item_buy;
        callback->checkfun = &player_item_is_sellable;
        callback->active = FALSE;
        g_ptr_array_add(callbacks, callback);
    }

    cmd = display_show_message(msg_title, text->str, 0);
    g_string_free(text, TRUE);

    switch (cmd)
    {
    case 'd': /* deposit */
        if (inv_length_filtered(p->inventory, item_filter_gold) == 0)
            break;

        amount = display_get_count("How many gold pieces do you wish to deposit?",
                                   player_get_gold(p));

        if (amount && (amount <= player_get_gold(p)))
        {
            p->bank_account += amount;
            player_set_gold(p, player_get_gold(p) - amount);
            log_add_entry(nlarn->log, "You deposited %d gp.", amount);
        }
        else if (amount)
        {
            log_add_entry(nlarn->log, "You don't have that much.");
        }

        turns += 2;

        break;

    case 'w': /* withdraw */
        if (p->bank_account == 0)
            break;

        amount = display_get_count("How many gold pieces do you wish to withdraw?",
                                   p->bank_account);

        if (amount && (amount <= p->bank_account))
        {
            item *gold = item_new(IT_GOLD, amount);

            /* adding the gold might fail (too heavy) */
            if (inv_add(&p->inventory, gold))
            {
                p->bank_account -= amount;
                log_add_entry(nlarn->log, "You withdraw %d gold.", amount);
            }
            else
            {
                /* this item is no longer required */
                item_destroy(gold);
            }
        }
        else if (amount)
        {
            log_add_entry(nlarn->log, "You don't have that much in the bank!");
        }

        turns += 2;

        break;

    case 's': /* sell gem */
        if (inv_length_filtered(p->inventory, item_filter_gems) == 0)
            break;

        display_inventory("Sell gems", p, &p->inventory, callbacks, TRUE,
                          FALSE, TRUE, &item_filter_gems);

        break;

    default:
        /* do nothing */
        break;
    }

    if (callbacks)
    {
        /* clean up */
        display_inv_callbacks_clean(callbacks);
    }

    return turns;
}

int building_dndstore(player *p)
{
    int turns = 2;
    const char title[] = "DND store";
    const char msg_welcome[] = "Welcome to the Nlarn Thrift Shoppe.\n" \
                               "We stock many items explorers find useful in " \
                               "their adventures. Feel free to browse to your " \
                               "heart's content. Also be advised that if you " \
                               "break 'em, you pay for 'em.";

    assert(p != NULL);

    /* no business if player has outstanding taxes */
    if (p->outstanding_taxes)
    {
        display_show_message(title, msg_outstanding, 0);
        return turns;
    }

    display_show_message(title, msg_welcome, 0);
    building_shop(p, &nlarn->store_stock, title);

    return turns;
}

void building_dndstore_init()
{
    static int initialized = FALSE;

    /* this is a one-time process! */
    if (initialized) return;

    int count;
    item_t type = IT_NONE;
    for (type = IT_AMULET; type < IT_MAX; type++)
    {
        /*never generate gems or gold */
        if (type == IT_GEM || type == IT_GOLD)
            continue;

        if (item_is_stackable(type) && (type != IT_BOOK))
            count = 3;
        else
            count = 1;

        int id;
        for (id = 1; id < item_max_id(type); id++)
        {
            /* do not generate unobtainable items except in wizard mode */
            if (!game_wizardmode(nlarn) && !item_obtainable(type, id))
                continue;

            item *it = item_new(type, id);

            if (item_is_identifyable(it->type))
            {
                /* make item attributes known */
                it->bonus_known = TRUE;
                it->blessed_known = TRUE;
            }
            it->count = count;

            /* add item to store */
            inv_add(&nlarn->store_stock, it);
        }
    }

    initialized = TRUE;
}

int building_home(player *p)
{
    int turns = 2;
    GString *text;

    const char msg_home[] = "Welcome home, %s.\n\nLatest word from the doctor " \
                            "is not good. The diagnosis is confirmed as " \
                            "dianthroritis. He guesses that your daughter " \
                            "has only %d mobuls left in this world.  It's " \
                            "up to you, %s, to find the only hope for your " \
                            "daughter, the very rare potion of cure " \
                            "dianthroritis.  It is rumored that only deep in " \
                            "the depths of the caves can this potion be found.\n";

    const char msg_found[] = "Congratulations. You found the potion of cure " \
                             "dianthroritis! Frankly, No one thought you " \
                             "could do it. Boy! Did you surprise them!\n\n";

    const char msg_died[] = "The doctor has the sad duty to inform you that " \
                            "your daughter died before your return. There was " \
                            "nothing he could do without the potion.\n\n" \
                            "You didn't make it in time. Too bad...";

    const char msg_won[] = "The doctor is now administering the potion and, in " \
                           "a few moments, your daughter should be well on " \
                           "her way to recovery.\n\nThe potion is working! " \
                           "The doctor thinks that your daughter will recover " \
                           "in a few days.\n\nCongratulations!";

    assert(p != NULL);

    /* look for potion of cure dianthroritis in player's inventory */
    if (inv_length_filtered(p->inventory, item_filter_pcd))
    {
        /* carrying the potion */
        text = g_string_new(msg_found);

        if (game_turn(nlarn) < TIMELIMIT)
        {
            /* won the game */
            g_string_append(text, msg_won);
            display_show_message("You saved your daughter!", text->str, 0);
            g_string_free(text, TRUE);

            /* remove the potion from the inventory as it has been used up */
            item *pcd = inv_get_filtered(p->inventory, 0, item_filter_pcd);
            inv_del_element(&p->inventory, pcd);
            item_destroy(pcd);

            player_die(p, PD_WON, 0);
        }
        else
        {
            /* lost the game */
            g_string_append(text, msg_died);
            display_show_message("You were too late!", text->str, 0);
            g_string_free(text, TRUE);

            player_die(p, PD_TOO_LATE, 0);
        }
    }
    else if (game_turn(nlarn) > TIMELIMIT)
    {
        /* too late, no potion */
        text = g_string_new(msg_died);
        display_show_message("You were too late!", text->str, 0);
        g_string_free(text, TRUE);

        player_die(p, PD_LOST, 0);
    }
    else
    {
        /* casual visit, report remaining time */
        text = g_string_new(NULL);
        g_string_printf(text, msg_home, p->name,
                        gtime2mobuls(game_remaining_turns(nlarn)),
                        p->name);

        display_show_message("Your home", text->str, 0);
        g_string_free(text, TRUE);
    }

    return turns;
}

int building_lrs(player *p)
{
    int turns = 2;
    GString *text;

    const char msg_greet[] = "Welcome to the Larn Revenue Service district office.\n\n";
    const char msg_taxes[] = "You presently owe %d gp in taxes.";
    const char msg_notax[] = "You do not owe us any taxes.";

    assert(p != NULL);

    text = g_string_new(msg_greet);

    if (p->outstanding_taxes)
        g_string_append_printf(text,
                               msg_taxes,
                               p->outstanding_taxes);
    else
        g_string_append(text, msg_notax);

    if (p->outstanding_taxes > player_get_gold(p))
        g_string_append(text, " You cannot afford to pay your taxes this time.");

    display_show_message("Larn Revenue Service", text->str, 0);

    g_string_free(text, TRUE);

    /* offer to pay taxes if player can afford to */
    if (p->outstanding_taxes && (building_player_check(p, p->outstanding_taxes)))
    {
        if (display_get_yesno("Do you want to pay your taxes?", NULL, NULL))
        {
            building_player_charge(p, p->outstanding_taxes);
            p->outstanding_taxes = 0;
            log_add_entry(nlarn->log, "You have paid your taxes.");
        }
        else
        {
            log_add_entry(nlarn->log, "You chose not to pay your taxes.");
        }
    }

    return turns;
}

static int building_scribe_scroll(player *p, int mobuls)
{
    int price;
    int turns = 2;
    int i;
    gboolean split = FALSE;
    item *scroll;
    char question[81] = { 0 };

    /* check if the player owns a blank scroll */
    if (!inv_length_filtered(p->inventory, item_filter_blank_scroll))
    {
        log_add_entry(nlarn->log, "To write a scroll, "
                      "the scribes require a blank scroll.");
        return turns;
    }

    scroll = display_inventory("Choose a scroll to inscribe", p,
                               &p->inventory, NULL, FALSE, FALSE,
                               FALSE, item_filter_blank_scroll);

    if (!scroll)
    {
        log_add_entry(nlarn->log, "Okay then.");
        return turns;
    }

    char *new_scroll = display_get_string("Write what scroll?", NULL, 45);
    if (new_scroll == NULL)
    {
        log_add_entry(nlarn->log, "Okay then.");
        return turns;
    }

    for (i = 1; i < ST_MAX; i++)
    {
        if (g_strcmp0(new_scroll, scrolls[i].name) == 0)
            break;
    }

    /* free memory alloc'd by display_get_string */
    g_free(new_scroll);

    if (i == ST_MAX)
    {
        log_add_entry(nlarn->log,
                      "The scribes haven't ever heard of any such scroll!");
        return turns;
    }

    /* jesters might want to get a scroll of blank paper */
    if (i == ST_BLANK)
    {
        log_add_entry(nlarn->log, "The scribes can only write something written!");
        return turns;
    }

    /* player has chosen which scroll to write, check if (s)he can afford it */
    price = 2 * scrolls[i].price;
    if (!building_player_check(p, price))
    {
        log_add_entry(nlarn->log, "You cannot afford the %d gold for the scroll of %s.",
                      price, scrolls[i].name);

        return turns;
    }

    g_snprintf(question, 80, "Writing a scroll of %s costs %d gold.\n"
               "Are you fine with that?", scrolls[i].name, price);

    if (!display_get_yesno(question, NULL, NULL))
    {
        log_add_entry(nlarn->log, "You refuse to pay %d gold for a scroll of %s.",
                      price, scrolls[i].name);
        return turns;
    }

    /** Okay, we write the scroll. */

    // If necessary, split a stack of scrolls.
    if (scroll->count > 1)
    {
        scroll = item_split(scroll, 1);
        split = TRUE;
    }

    scroll->id = i;
    p->identified_scrolls[i] = TRUE;

    building_player_charge(p, price);
    p->stats.gold_spent_college += price;

    log_add_entry(nlarn->log, "The scribes start writing a scroll of %s for you.",
                  scroll_name(scroll));

    player_make_move(p, mobuls2gtime(mobuls), FALSE, NULL);
    log_add_entry(nlarn->log,
                  "The scribes finished writing a scroll of %s for you.",
                  scroll_name(scroll));

    if (split)
        inv_add(&p->inventory, scroll);

    /* time usage */
    return turns;
}

int building_school(player *p)
{
    /* the number of turns it takes to enter and leave the school */
    int turns = 2;
    guint price;

    GString *text;
    guint idx;
    int selection;

    const char msg_greet[] = "The College of Larn offers the exciting " \
                             "opportunity of higher education to all " \
                             "inhabitants of the caves.\n\n" \
                             "Here is a list of the class schedule:\n\n";

    const char msg_price[] = "\nAll courses cost %d gold pieces.";

    const char msg_prerequisite[] = "Sorry, but this class has a prerequisite of \"%s\".";

    /* school courses */
    const school_course school_courses[SCHOOL_COURSE_COUNT] =
    {
        { 10, -1, "Fighters Training I", "You feel stronger!" },
        { 15,  0, "Fighters Training II", "You feel much stronger!" },
        { 10, -1, "Introduction to Wizardry", "The task before you now seems more attainable!" },
        { 20,  2, "Applied Wizardry", "The task before you now seems very attainable!" },
        { 10, -1, "Faith for Today", "You now feel more confident that you can find the potion in time!" },
        { 10, -1, "Contemporary Dance", "You feel like dancing!" },
        {  5, -1, "History of Larn", "Your instructor told you that the Eye of Larn is rumored to be guarded by an invisible demon lord." },
        { 10, -1,  "Commission a scroll", NULL }
    };

    assert(p != NULL);

    /* courses become more expensive with rising difficulty */
    price = 250 + (game_difficulty(nlarn) * 50);

    text = g_string_new(msg_greet);

    for (idx = 0; idx < SCHOOL_COURSE_COUNT - 1; idx++)
    {
        if (!p->school_courses_taken[idx])
        {
            g_string_append_printf(text, "  %c) %-30s (%2d mobuls)\n",
                                   idx + 'a', school_courses[idx].description,
                                   school_courses[idx].course_time);
        }
        else
        {
            g_string_append(text, "\n");
        }
    }

    g_string_append_printf(text, msg_price, price);
    g_string_append_printf(text, "\n\nAlternatively,\n"
                           "  %c) %-30s (%2d mobuls)\n\n",
                           idx + 'a', school_courses[idx].description,
                           school_courses[idx].course_time);

    selection = display_show_message("School", text->str, 0);
    g_string_free(text, TRUE);

    selection -= 'a';

    if ((selection >= 0)
            && (selection < SCHOOL_COURSE_COUNT)
            && !p->school_courses_taken[(int)selection])
    {
        if (selection == SCHOOL_COURSE_COUNT - 1)
            return building_scribe_scroll(p, school_courses[(int)selection].course_time);

        if (!building_player_check(p, price))
        {
            log_add_entry(nlarn->log,
                          "You cannot afford the %d gold for the course.",
                          price);

            return turns;
        }

        /* check if the selected course has a prerequisite
           and if the player has taken that course */
        if ((school_courses[selection].prerequisite >= 0) &&
                !p->school_courses_taken[school_courses[selection].prerequisite])
        {
            log_add_entry(nlarn->log, msg_prerequisite,
                          school_courses[school_courses[selection].prerequisite].description);

            return turns;
        }

        log_add_entry(nlarn->log, "You take the course \"%s\".",
                      school_courses[(int)selection].description);

        /* charge the player */
        building_player_charge(p, price);
        p->stats.gold_spent_college += price;

        /* time usage */
        guint course_turns = mobuls2gtime(school_courses[(guint)selection].course_time);
        player_make_move(p, course_turns, FALSE, NULL);

        /* add the bonus gained by this course */
        switch (selection)
        {
        case 0:
            p->strength += 2;
            p->constitution++;
            break;

        case 1:
            p->strength += 2;
            p->constitution += 2;
            break;

        case 2:
            p->intelligence += 2;
            break;

        case 3:
            p->intelligence += 2;
            break;

        case 4:
            p->wisdom += 2;
            break;

        case 5:
            p->dexterity += 3;
            break;

        case 6:
            p->intelligence++;
            break;
        }

        /* mark the course as taken */
        p->school_courses_taken[(int)selection] = 1;

        log_add_entry(nlarn->log, "You successfully complete the course \"%s\". %s",
                      school_courses[(int)selection].description,
                      school_courses[(int)selection].message);
    }

    return turns;
}

int building_tradepost(player *p)
{
    int turns = 2;
    GPtrArray *callbacks;
    display_inv_callback *callback;

    const char title[] = "Trade Post";

    const char msg_welcome[] = "Welcome to the Nlarn Trading Post.\n\nWe buy " \
                               "items that explorers no longer find useful.\n" \
                               "Since the condition of the items you bring in " \
                               "is not certain, and we incur great expense in " \
                               "reconditioning the items, we usually pay only " \
                               "20% of their value were they to be new.\n" \
                               "If the items are badly damaged, we will pay " \
                               "only 10% of their new value.";

    assert(p != NULL);

    /* no business if player has outstanding taxes */
    if (p->outstanding_taxes)
    {
        display_show_message(title, msg_outstanding, 0);
        return turns;
    }

    /* define callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(s)ell";
    callback->key = 's';
    callback->inv = &nlarn->store_stock;
    callback->function = &building_item_buy;
    callback->checkfun = &player_item_is_sellable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(i)dentify";
    callback->key = 'i';
    callback->function = &building_item_identify;
    callback->checkfun = &player_item_is_identifiable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(r)epair";
    callback->key = 'r';
    callback->function = &building_item_repair;
    callback->checkfun = &player_item_is_damaged;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(e)quip";
    callback->key = 'e';
    callback->function = &player_item_equip;
    callback->checkfun = &player_item_is_equippable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(u)nequip";
    callback->key = 'u';
    callback->function = &player_item_unequip_wrapper;
    callback->checkfun = &player_item_is_equipped;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(n)ote";
    callback->key = 'n';
    callback->function = &player_item_notes;
    callback->checkfun = NULL;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    display_show_message(title, msg_welcome, 0);
    display_inventory(title, p, &p->inventory, callbacks, FALSE,
                      FALSE, TRUE, &item_filter_not_gold);

    /* clean up */
    display_inv_callbacks_clean(callbacks);

    return turns;
}

int building_monastery(struct player *p)
{
    const char title[] = "The Monastery of Larn";
    const char msg_welcome[] = "Welcome to the Monastery of Larn!\n\n" \
                               "We are here to help you when you are in need of " \
                               "care and offer a fine selection of items that might "
                               "be useful for your quests.\n\n" \
                               "Here you may\n\n" \
                               "  a) recieve healing\n" \
                               "  b) ask for curse removal\n" \
                               "  c) buy something\n\n";
    const char ayfwt[] = "Are you fine with that?";

    int turns = 2;
    int selection;

    selection = display_show_message(title, msg_welcome, 0);

    switch (selection)
    {
        /* healing */
    case 'a':
    {
        int choice;
        char *question;
        /* the price for healing depends on the severity of the injury */
        int price = (player_get_hp_max(p) - p->hp) * (game_difficulty(nlarn) + 1);

        if (p->hp == player_get_hp_max(p))
        {
            log_add_entry(nlarn->log, "You are not in need of healing.");
            break;
        }

        question = g_strdup_printf("For healing you, we ask that you "
                                   "donate %d gold for our monastery. %s",
                                   price, ayfwt);
        choice = display_get_yesno(question, NULL, NULL);
        g_free(question);

        if (choice)
        {
            /* player chose to be healed */
            player_effect_add(p, effect_new(ET_MAX_HP));
            building_player_charge(p, price);
            p->stats.gold_spent_donation += price;

        }
        else
        {
            /* no, thanks */
            log_add_entry(nlarn->log, "You chose not to be healed.");
        }
    }
    break;

    /* remove curse */
    case 'b':
    {
        item *it;
        int price;
        int choice;
        char *question;
        char desc[81];

        if (inv_length_filtered(p->inventory, item_filter_cursed_or_unknown) == 0)
        {
            log_add_entry(nlarn->log, "You do not possess any cursed item.");
            break;
        }

        it = display_inventory("Choose an item to uncurse", p, &p->inventory,
                               NULL, FALSE, FALSE, FALSE, item_filter_cursed_or_unknown);

        /* cost of uncursing is 10 percent of item value */
        /* item value for cursed items is reduced by 50% */
        price = (item_price(it) / 5) * (game_difficulty(nlarn) + 1);

        item_describe(it, player_item_identified(p, it), it->count, TRUE,
                      desc, 80);

        question = g_strdup_printf("To remove the curse on %s, we ask you to "
                                   "donate %d gold for our abbey. %s", desc,
                                   price, ayfwt);

        choice = display_get_yesno(question, NULL, NULL);
        g_free(question);

        if (!choice)
        {
            log_add_entry(nlarn->log, "You chose leave the curse on %s.", desc);
            break;
        }

        log_add_entry(nlarn->log, "The monks remove the curse on %s.", desc);
        item_remove_curse(it);
        building_player_charge(p, price);
        p->stats.gold_spent_donation += price;
    }
    break;

    /* shop items */
    case 'c':
        building_shop(p, &nlarn->monastery_stock, title);

        break;

    default:
        /* invalid choice */
        break;
    }

    return turns;
}

void building_monastery_init()
{
    int i = 0, j;

    /* the list of items available in the monastery */
    struct
    {
        item_t type;
        int id;
        int count;
    } igen[] =
    {
        { IT_POTION, PO_HEAL, 5 },
        { IT_POTION, PO_MAX_HP, 1 },
        { IT_POTION, PO_RECOVERY, 5 },
        { IT_POTION, PO_WATER, 1 },
        { IT_SCROLL, ST_REMOVE_CURSE, 3 },
        { IT_NONE, 0, 0 },
    };

    while (igen[i].type != IT_NONE)
    {
        for (j = 0; j < igen[i].count; j++)
        {
            item *nitem = item_new(igen[i].type, igen[i].id);
            inv_add(&nlarn->monastery_stock, nitem);
        }

        i++;
    };
}

static void building_shop(player *p, inventory **inv, const char *title)
{
    GPtrArray *callbacks;
    display_inv_callback *callback;

    if (inv_length(*inv) == 0)
    {
        log_add_entry(nlarn->log, "Unfortunately we are sold out.");
        return;
    }

    /* define callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(b)uy";
    callback->key = 'b';
    callback->inv = inv;
    callback->function = &building_item_sell;
    callback->checkfun = &player_item_is_affordable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    display_inventory(title, p, inv, callbacks, TRUE,
                      FALSE, TRUE, NULL);

    /* clean up */
    display_inv_callbacks_clean(callbacks);
}

static int building_player_check(player *p, guint amount)
{
    guint player_gold = player_get_gold(p);

    if (player_gold >= amount)
        return TRUE;

    if (p->bank_account >= amount)
        return TRUE;

    return FALSE;
}

static void building_player_charge(player *p, guint amount)
{
    if (p->bank_account >= amount)
    {
        p->bank_account -= amount;
        log_add_entry(nlarn->log, "We have debited your bank account %d gold.",
                      amount);
    }
    else
    {
        guint player_gold = player_get_gold(p);
        player_set_gold(p, player_gold - amount);
    }
}

/**
 *
 * add an item to the dnd store
 *
 * either refurbish and put the item into the store's inventory
 * or destroy it if it is a gem.
 *
 */
static void building_item_add(inventory **inv, item *it)
{
    if (it->type == IT_GEM
            || (it->type == IT_WEAPON && !weapon_is_unique(it)))
    {
        /* gems and non-unique weapons do not appear in the store */
        item_destroy(it);
    }
    else
    {
        /* refurbish item before putting it into the store */
        if (it->cursed)
            it->cursed = FALSE;

        if (it->rusty)
            it->rusty = FALSE;

        if (it->burnt)
            it->burnt = FALSE;

        if (it->corroded)
            it->corroded = FALSE;

        if (it->bonus < 0)
            it->bonus = 0;

        /* identify item */
        it->bonus_known = TRUE;
        it->blessed_known = TRUE;

        /* remove notes */
        if (it->notes)
        {
            g_free(it->notes);
            it->notes = NULL;
        }

        inv_add(inv, it);
    }
}

static void building_item_sell(player *p, inventory **inv, item *it)
{
    guint price;
    guint count = 0;
    gpointer ioid = NULL; /* oid of purchased item */
    char text[81];
    char name[61];

    /* copy of item needed to descibe and transfer original item */
    item *it_clone = NULL;

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    price = item_price(it);

    if (it->count > 1)
    {
        item_describe(it, TRUE, FALSE, TRUE, name, 40);
        g_snprintf(text, 80, "How many %s do you want to buy?", name);

        /* get count */
        count = display_get_count(text, it->count);

        if (count > it->count)
        {
            /* desired amount is larger than the available amount */
            log_add_entry(nlarn->log, "Wouldn't it be nice if the store had %d of those?", count);
            return;
        }
        else if (count == 0)
        {
            return;
        }
        else if (count == it->count)
        {
            /* player wants the entire stock of the item */
            it_clone = it;
        }
        else if (count < it->count)
        {
            /* player wants part of the stock */
            it_clone = item_split(it, count);
        }

        price *= count;

        if (!building_player_check(p, price))
        {
            item_describe(it_clone, TRUE, (count == 1), TRUE, name, 60);
            g_snprintf(text, 80, "You cannot afford the %d gold for %s.",
                       price, name);

            display_show_message(NULL, text, 0);

            /* if the item has been split add it to the shop */
            if (it != it_clone) inv_add(inv, it_clone);

            return;
        }
    }
    else
    {
        count = 1;
        item_describe(it, TRUE, TRUE, TRUE, name, 60);
        g_snprintf(text, 80, "Do you want to buy %s for %d gold?",
                   name, price);

        if (!display_get_yesno(text, NULL, NULL))
            return;

        it_clone = it;
    }

    /* prepare the item description for logging later */
    item_describe(it_clone, TRUE, (count == 1), FALSE, name, 60);

    /* try to transfer the item to the player's inventory */
    ioid = it_clone->oid;

    if (inv_add(&p->inventory, it_clone))
    {
        /* the item has been added to player's inventory */
        if (it == it_clone)
        {
            /* remove the item from the shop as the player has bought the
            entire stock. this has to be done by the oid as it_clone may
            have been destroyed if it was a stackable item. */
            inv_del_oid(inv, ioid);
        }

        p->stats.items_bought    += it_clone->count;
        p->stats.gold_spent_shop += price;

        /* identify the item */
        player_item_identify(p, &p->inventory, it_clone);
    }
    else
    {
        /* item has not been added to player's inventory */
        /* if the item has been split, return it to the shop */
        if (it != it_clone) inv_add(inv, it_clone);
        return;
    }

    /* log the event */
    log_add_entry(nlarn->log, "You buy %s. Thank you for your purchase.", name);

    /* charge player for this purchase */
    building_player_charge(p, price);

    player_make_move(p, 2, FALSE, NULL);
}

static void building_item_identify(player *p, inventory **inv, item *it)
{
    guint price;
    char name_unknown[61];
    char name_known[61];
    char message[81];

    const char title[] = "Identify item";

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* don't need that parameter */
    inv = NULL;

    price = 50 << game_difficulty(nlarn);

    item_describe(it, player_item_known(p, it), TRUE, TRUE, name_unknown, 60);

    if (building_player_check(p, price))
    {
        g_snprintf(message, 80, "Pay %d gold to identify %s?", price, name_unknown);

        if (display_get_yesno(message, NULL, NULL))
        {
            player_item_identify(p, NULL, it);
            /* upper case first letter */
            name_unknown[0] = g_ascii_toupper(name_unknown[0]);
            item_describe(it, player_item_known(p, it), TRUE, FALSE, name_known, 60);

            log_add_entry(nlarn->log, "%s is %s.", name_unknown, name_known);
            building_player_charge(p, price);

            p->stats.gold_spent_id_repair += price;
            player_make_move(p, 1, FALSE, NULL);
        }
    }
    else
    {
        g_snprintf(message, 80, "Identifying %s costs %d gold.", name_unknown, price);
        display_show_message(title, message, 0);
    }
}

static void building_item_repair(player *p, inventory **inv, item *it)
{
    int damages = 0;
    guint price;
    char name[61];
    char message[81];

    const char title[] = "Repair item";

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* don't need that parameter */
    inv = NULL;

    /* determine how much the item is damaged */
    damages += it->burnt;
    damages += it->corroded;
    damages += it->rusty;

    price = (50 << game_difficulty(nlarn)) * damages;

    item_describe(it, player_item_known(p, it), TRUE, TRUE, name, 60);

    if (building_player_check(p, price))
    {
        g_snprintf(message, 80, "Pay %d gold to repair %s?", price, name);

        if (display_get_yesno(message, NULL, NULL))
        {
            it->burnt = 0;
            it->corroded = 0;
            it->rusty = 0;

            name[0] = g_ascii_toupper(name[0]);
            log_add_entry(nlarn->log, "%s has been repaired.", name);
            building_player_charge(p, price);

            p->stats.gold_spent_id_repair += price;
            player_make_move(p, 1, FALSE, NULL);
        }
    }
    else
    {
        g_snprintf(message, 80, "Repairing the %s costs %d gold.", name, price);
        display_show_message(title, message, 0);
    }
}

static void building_item_buy(player *p, inventory **inv, item *it)
{
    int price;
    guint count = 0;
    char question[121];
    char name[61];

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    item_describe(it, player_item_known(p, it), FALSE, FALSE, name, 60);

    price = item_price(it);

    /* modify price if player sells stuff at the trading post */
    if (map_sobject_at(game_map(nlarn, p->pos.z), p->pos) == LS_TRADEPOST)
    {
        if (!player_item_is_damaged(p, it))
        {
            /* good items: 20% of value */
            price /= 5;
        }
        else
        {
            /* damaged items: 10% of value */
            price /= 10;
        }
    }

    if (price < 1)
        price = 1;

    if (it->count > 1)
    {
        item_describe(it, player_item_known(p, it), FALSE, TRUE, name, 60);
        g_snprintf(question, 120, "How many %s do you want to sell for %d gold?",
                   name, price);

        /* get count */
        count = display_get_count(question, it->count);

        if (count > it->count)
        {
            log_add_entry(nlarn->log, "Wouldn't it be nice to have %d of those?", count);
            return;
        }

        if (count == 0)
            return;

        price *= count;
    }
    else
    {
        count = 1;
        item_describe(it, player_item_known(p, it), TRUE, TRUE, name, 40);
        g_snprintf(question, 120, "Do you want to sell %s for %d gold?",
                   name, price);

        if (!display_get_yesno(question, NULL, NULL))
            return;
    }

    p->bank_account += price;

    guint count_orig = it->count;
    it->count = count;

    item_describe(it, player_item_known(p, it), (count == 1), FALSE, name, 60);
    log_add_entry(nlarn->log, "You sell %s. The %d gold %s been transferred to your bank account.",
                  name, price, (price == 1) ? "has" : "have");

    it->count = count_orig;

    if (it->type == IT_GEM)
    {
        p->stats.gems_sold += count;
        p->stats.gold_sold_gems += price;
    }
    else
    {
        p->stats.items_sold += count;
        p->stats.gold_sold_items += price;
    }

    if ((it->count > 1) && (count < it->count))
    {
        building_item_add(inv, item_split(it, count));
    }
    else
    {
        if (!inv_del_element(&p->inventory, it))
        {
            return;
        }
        else
        {
            building_item_add(inv, it);
        }
    }

    player_make_move(p, 1, FALSE, NULL);
}
