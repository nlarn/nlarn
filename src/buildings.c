/*
 * buildings.c
 * Copyright (C) 2009-2020 Joachim de Groot <jdegroot@web.de>
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
#include <glib.h>

#include "config.h"
#include "container.h"
#include "display.h"
#include "game.h"
#include "gems.h"
#include "items.h"
#include "nlarn.h"
#include "player.h"
#include "scrolls.h"

static const char msg_outstanding[] = "The Larn Revenue Service has ordered " \
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
static guint calc_tax_debt(guint income);

void building_bank_calc_interest(game *g)
{
    guint interest = 0;

    /* pay interest every 1000 turns */
    if (game_turn(g) % 1000 != 0 || g->p->bank_account <= 250)
        return;

    /* the bank pays an interest of 2.5% every ten mobuls */
    interest = g->p->bank_account / 250;

    /* add the interest to the bank account.. */
    g->p->bank_account += interest;

    /* ..and the statistics.. */
    g->p->stats.gold_bank_interest += interest;

    /* ..and keep track of the amount paid since the player's last visit to the bank */
    g->p->bank_ieslvtb += interest;

    /* calculate the tax debt */
    g->p->outstanding_taxes += calc_tax_debt(interest);

}

int building_bank(player *p)
{
    guint amount = 0;
    GString *greeting;

    const char msg_title[] = "First National Bank of Larn";
    const char msg_greet[] = "`white`Welcome to the First National Bank of Larn.`end`\n\n";

    const char msg_branch[] = "Welcome to the 5th level branch office of the " \
                              "First National Bank of Larn.\n\n";

    const char msg_frozen[] = "The Larn Revenue Service has ordered that your " \
                              "account be frozen until all levied taxes have " \
                              "been paid.  They have also told us that you " \
                              "owe %d gold in taxes, and we must comply with " \
                              "them. We cannot serve you at this time.  Sorry.\n\n" \
                              "We suggest you go to the LRS office and pay your taxes.";

    g_assert(p != NULL);

    if (Z(p->pos) == 0)
        greeting = g_string_new(msg_greet);
    else
        greeting = g_string_new(msg_branch);

    if (p->bank_ieslvtb > 0)
    {
        /* show the earned interest */
        g_string_append_printf(greeting, "We have paid you an interest of %d " \
                               "gold since your last visit.\n", p->bank_ieslvtb);

        /* add it to the game log for later reference */
        log_add_entry(nlarn->log, "The bank has paid you an interest of %d "
                      "gold since your last visit.", p->bank_ieslvtb);

        /* reset the value */
        p->bank_ieslvtb = 0;
    }

    /* leave bank when taxes are unpaid */
    if (p->outstanding_taxes)
    {
        g_string_append_printf(greeting, msg_frozen, p->outstanding_taxes);
        display_show_message(msg_title, greeting->str, 0);
        g_string_free(greeting, TRUE);

        return 2; /* turns */
    }

    gboolean leaving = FALSE;
    while (!leaving)
    {
        GString *text = g_string_new(greeting->str);

        g_string_append_printf(text,
                "You have %d gold pieces in the bank.\n" \
                "You have %d gold pieces.\n\n",
                p->bank_account, player_get_gold(p));

        g_string_append(text, "Your wish? ");

        if (player_get_gold(p) > 0)
            g_string_append(text, "`lightgreen`d`end`)eposit ");

        if (p->bank_account > 0)
            g_string_append(text, "`lightgreen`w`end`)ithdraw ");

        /* if player has gems, enable selling them */
        if (inv_length_filtered(p->inventory, item_filter_gems))
            g_string_append(text, "`lightgreen`s`end`)ell a gem");

        display_window *bwin = display_popup(COLS / 2 - 23, LINES / 2 - 3,
                47, msg_title, text->str, 0);

        g_string_free(text, TRUE);

        int cmd = display_getch(bwin->window);

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
                player_remove_gold(p, amount);
                log_add_entry(nlarn->log, "You deposited %d gold.", amount);

                /* income tax for money earned in the caverns */
                p->outstanding_taxes += calc_tax_debt(amount);
            }
            else if (amount)
            {
                log_add_entry(nlarn->log, "You don't have that much.");
            }

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

            break;

        case 's': /* sell gem */
            {
                if (inv_length_filtered(p->inventory, item_filter_gems) == 0)
                    break;

                /* define callback functions */
                GPtrArray *callbacks = g_ptr_array_new();

                display_inv_callback *callback = g_malloc(sizeof(display_inv_callback));
                callback->description = "(s)ell";
                callback->helpmsg = "Sell the currently selected gem.";
                callback->key = 's';
                callback->inv = &nlarn->store_stock;
                callback->function = &building_item_buy;
                callback->checkfun = &player_item_is_sellable;
                callback->active = FALSE;
                g_ptr_array_add(callbacks, callback);

                display_inventory("Sell gems", p, &p->inventory, callbacks,
                        TRUE, FALSE, TRUE, &item_filter_gems);

                display_inv_callbacks_clean(callbacks);
            }
            break;

        case KEY_ESC:
            leaving = TRUE;
            break;

        default:
            /* do nothing */
            break;
        }

        /* every interaction in the bank takes two turns */
        player_make_move(p, 2, FALSE, NULL);
        display_window_destroy(bwin);
    }

    g_string_free(greeting, TRUE);

    return 0;
}

int building_dndstore(player *p)
{
    int turns = 2;
    const char title[] = "DND store";
    const char msg_welcome[] = "`white`Welcome to the Larn Thrift Shoppe.`end`\n\n" \
                               "We stock many items explorers find useful in " \
                               "their adventures. Feel free to browse to your " \
                               "heart's content. Also be advised that if you " \
                               "break 'em, you pay for 'em.";

    g_assert(p != NULL);

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
    for (item_t type = IT_AMULET; type < IT_MAX; type++)
    {
        /*never generate gems or gold */
        if (type == IT_GEM || type == IT_GOLD)
            continue;

        for (guint id = 0; id < item_max_id(type); id++)
        {
            int count;

            /* do not generate unobtainable items except in wizard mode */
            if (!game_wizardmode(nlarn) && !item_obtainable(type, id))
                continue;

            switch (type)
            {
            case IT_AMMO:
                count = 50;
                break;

            case IT_BOOK:
                count = 1;
                break;

            case IT_SCROLL:
                count = game_wizardmode(nlarn) ? 10 : scroll_type_store_stock(id);
                break;

            case IT_POTION:
                count = game_wizardmode(nlarn) ? 10 : potion_type_store_stock(id);
                break;

            default:
                {
                    if (item_is_stackable(type))
                        count = game_wizardmode(nlarn) ? 10 : 3;
                    else
                        count = 1;
                }
            }

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
}

int building_home(player *p)
{
    int turns = 2;
    GString *text;

    const char title[] = "Your home";

    const char msg_home[] = "`white`Welcome home, %s.`end`\n\nLatest word from the doctor " \
                            "is not good. The diagnosis is confirmed as " \
                            "dianthroritis. He guesses that your daughter " \
                            "has only %d mobuls left in this world.  It's " \
                            "up to you, %s, to find the only hope for your " \
                            "daughter, the very rare potion of cure " \
                            "dianthroritis.  It is rumored that only deep in " \
                            "the depths of the caves can this potion be found.\n";

    const char msg_found[] = "`white`Congratulations!`end` You found the potion of cure " \
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

    g_assert(p != NULL);

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

            /* increase difficulty level */
            config_increase_difficulty(nlarn_inifile, nlarn->difficulty + 1);

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

        int choice;
        GPtrArray *callbacks;
        display_inv_callback *callback;

        text = g_string_new(NULL);
        g_string_printf(text, msg_home, p->name,
                        gtime2mobuls(game_remaining_turns(nlarn)),
                        p->name);

        /* check if the player can deposit something
           at home or has already done so */
        if ((inv_length_filtered(p->inventory, player_item_not_equipped) > 0)
            || (inv_length(nlarn->player_home) > 0))
        {
            g_string_append_printf(text, "\n\nYou may\n");

            if (inv_length_filtered(p->inventory, player_item_not_equipped) > 0)
                g_string_append_printf(text, "  `lightgreen`d`end`) "
                                       "Deposit something here\n");

            if (inv_length(nlarn->player_home) > 0)
                g_string_append_printf(text, "  `lightgreen`t`end`) "
                                       "Take something with you\n");

            g_string_append_c(text, '\n');
        }

        choice = display_show_message(title, text->str, 0);
        g_string_free(text, TRUE);

        switch (choice)
        {
            /* deposit something */
        case 'd':
            if (inv_length_filtered(p->inventory, player_item_not_equipped) > 0)
            {
                /* prepare callback functions */
                callbacks = g_ptr_array_new();

                callback = g_malloc0(sizeof(display_inv_callback));
                callback->description = "(d)eposit";
                callback->helpmsg = "Deposit the selected item in "
                                    "your storage room at home.";
                callback->key = 'd';
                callback->inv = &nlarn->player_home;
                callback->function = &container_item_add;
                callback->active = TRUE;

                g_ptr_array_add(callbacks, callback);

                display_inventory(title, p, &p->inventory, callbacks, FALSE,
                                  TRUE, FALSE, player_item_not_equipped);

                display_inv_callbacks_clean(callbacks);
            }
            break;

            /* take something */
        case 't':
            if (inv_length(nlarn->player_home) > 0)
            {
                /* prepare callback functions */
                callbacks = g_ptr_array_new();

                callback = g_malloc0(sizeof(display_inv_callback));
                callback->description = "(t)ake";
                callback->helpmsg = "Take the selected item out of your "
                                    "storage room and put it into your pack.";
                callback->key = 't';
                callback->inv = &nlarn->player_home;
                callback->function = &container_item_unpack;
                callback->active = TRUE;

                g_ptr_array_add(callbacks, callback);

                display_inventory(title, p, &nlarn->player_home, callbacks,
                                  FALSE, TRUE, FALSE, NULL);

                display_inv_callbacks_clean(callbacks);
            }
            break;
        }
    }

    return turns;
}

int building_lrs(player *p)
{
    int turns = 2;
    GString *text;

    const char title[] ="Larn Revenue Service";
    const char msg_greet[] = "Welcome to the Larn Revenue Service district office.\n\n";
    const char msg_taxes[] = "You presently owe %d gold in taxes.\n\n";
    const char msg_notax[] = "You do not owe us any taxes.";

    g_assert(p != NULL);

    text = g_string_new(msg_greet);

    if (p->outstanding_taxes)
    {
        g_string_append_printf(text, msg_taxes, p->outstanding_taxes);

        /* offer to pay taxes if player can afford to */
        if (building_player_check(p, p->outstanding_taxes))
        {
            g_string_append(text, "Do you want to pay your taxes?");

            if (display_get_yesno(text->str, title, NULL, NULL))
            {
                building_player_charge(p, p->outstanding_taxes);
                p->stats.gold_spent_taxes += p->outstanding_taxes;
                p->outstanding_taxes = 0;
                log_add_entry(nlarn->log, "You have paid your taxes.");
            }
            else
            {
                log_add_entry(nlarn->log, "You chose not to pay your taxes.");
            }
        }
        else
        {
            g_string_append(text, "Unfortunately, it seems that you cannot "
                    "afford to pay your taxes at this time.");
            display_show_message(title, text->str, 0);
        }
    }
    else
    {
        g_string_append(text, msg_notax);
        display_show_message(title, text->str, 0);
    }

    g_string_free(text, TRUE);

    return turns;
}

static int building_scribe_scroll(player *p)
{
    int price;
    int turns = 2;
    int i;
    gboolean split = FALSE;
    item *bscroll;
    char question[81] = { 0 };

    /* check if the player owns a blank scroll */
    if (!inv_length_filtered(p->inventory, item_filter_blank_scroll))
    {
        display_show_message("School", "To write a scroll, the scribes "
                "require a blank scroll.", 0);

        return turns;
    }

    bscroll = display_inventory("Choose a scroll to inscribe", p,
                                &p->inventory, NULL, FALSE, FALSE,
                                FALSE, item_filter_blank_scroll);

    if (!bscroll)
    {
        log_add_entry(nlarn->log, "Okay then.");
        return turns;
    }

    char *new_scroll = display_get_string("School", "Write what scroll?",
            NULL, 45);
    if (new_scroll == NULL)
    {
        display_show_message("School", "Okay then.", 0);
        return turns;
    }

    for (i = 0; i < ST_MAX; i++)
    {
        if (g_strcmp0(new_scroll, scrolls[i].name) == 0)
            break;
    }

    /* free memory alloc'd by display_get_string */
    g_free(new_scroll);

    if (i == ST_MAX)
    {
        display_show_message("School", "The scribes haven't ever heard of any "
                "such scroll!", 0);
        return turns;
    }

    /* jesters might want to get a scroll of blank paper */
    if (i == ST_BLANK)
    {
        display_show_message("School", "The scribes can only write something "
                "written!", 0);
        return turns;
    }

    /* player has chosen which scroll to write, check if (s)he can afford it */
    price = 2 * scrolls[i].price;
    if (!building_player_check(p, price))
    {
        char *msg = g_strdup_printf("You cannot afford the %d gold for the "
                "scroll of %s.", price, scrolls[i].name);
        display_show_message("School", msg, 0);
        g_free(msg);

        return turns;
    }

    g_snprintf(question, 80, "Writing a scroll of %s costs %d gold.\n"
               "Are you fine with that?", scrolls[i].name, price);

    if (!display_get_yesno(question, "School", NULL, NULL))
    {
        return turns;
    }

    /** Okay, we write the scroll. */

    // If necessary, split a stack of scrolls.
    if (bscroll->count > 1)
    {
        bscroll = item_split(bscroll, 1);
        split = TRUE;
    }

    bscroll->id = i;
    p->identified_scrolls[i] = TRUE;

    building_player_charge(p, price);
    p->stats.gold_spent_shop += price;

    /* writing a scroll takes 10 mobuls */
    player_make_move(p, 1000, FALSE, "waiting for the scribes to write a "
            "scroll of %s for you", scroll_name(bscroll));

    log_add_entry(nlarn->log,
            "The scribes finished writing a scroll of %s for you.",
            scroll_name(bscroll));

    if (split)
        inv_add(&p->inventory, bscroll);

    return 0;
}

/* school courses */
typedef struct school_course {
    int course_time;
    int prerequisite;
    const char *description;
    const char *message;
} school_course;

static const school_course school_courses[SCHOOL_COURSE_COUNT] =
{
    { 10, -1, "Fighters Training I", "You feel stronger!" },
    { 15,  0, "Fighters Training II", "You feel much stronger!" },
    { 10, -1, "Introduction to Wizardry", "The task before you now seems more attainable!" },
    { 20,  2, "Applied Wizardry", "The task before you now seems very attainable!" },
    { 10, -1, "Faith for Today", "You now feel more confident that you can find the potion in time!" },
    { 10, -1, "Contemporary Dance", "You feel like dancing!" },
    {  5, -1, "History of Larn", "Your instructor told you that the Eye of Larn is rumored to be guarded by an invisible demon lord." },
};

static void building_school_take_course(player *p, int course, guint price)
{

    /* charge the player */
    building_player_charge(p, price);
    p->stats.gold_spent_college += price;

    /* time usage */
    guint course_turns = mobuls2gtime(school_courses[course].course_time);
    player_make_move(p, course_turns, FALSE, "taking the course \"%s\"",
            school_courses[course].description);

    /* add the bonus gained by this course */
    switch (course)
    {
    case 0:
        p->strength += 2;
        p->constitution++;
        /* strength has been modified -> recalc burdened status */
        player_inv_weight_recalc(p->inventory, NULL);
        break;

    case 1:
        p->strength += 2;
        p->constitution += 2;
        /* strength has been modified -> recalc burdened status */
        player_inv_weight_recalc(p->inventory, NULL);
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
    p->school_courses_taken[course] = 1;

    log_add_entry(nlarn->log,
            "You successfully complete the course \"%s\". %s",
            school_courses[course].description,
            school_courses[course].message);
}

int building_school(player *p)
{
    const char msg_greet[] = "`white`Welcome to the College of Larn!`end`\n\n" \
                             "We offer the exciting opportunity of higher " \
                             "education to all inhabitants of the caves. " \
                             "Here is a list of the class schedule:\n\n";

    const char msg_prerequisite[] = "Sorry, but this class has a prerequisite of \"%s\".";

    g_assert(p != NULL);

    gboolean leaving = FALSE;
    while (!leaving)
    {
        GString *text = g_string_new(msg_greet);

        for (int idx = 0; idx < SCHOOL_COURSE_COUNT; idx++)
        {
            if (!p->school_courses_taken[idx])
            {
                /* courses become more expensive with rising difficulty */
                guint price = school_courses[idx].course_time
                              * (game_difficulty(nlarn) + 1) * 100;

                g_string_append_printf(text,
                        " `lightgreen`%c`end`) %-24s - %2d mobuls, %4d gold\n",
                        idx + 'a', school_courses[idx].description,
                        school_courses[idx].course_time, price);
            }
            else
            {
                g_string_append(text, "\n");
            }
        }

        g_string_append_printf(text, "\nAlternatively,\n"
                " `lightgreen`%c`end`) %-24s - 10 mobuls\n\n",
                SCHOOL_COURSE_COUNT + 'a', "Commission a scroll");

        int selection = display_show_message("School", text->str, 0);
        g_string_free(text, TRUE);

        switch (selection)
        {
            case 'a' + SCHOOL_COURSE_COUNT:
                player_make_move(p, building_scribe_scroll(p), FALSE, NULL);
                break;

            case KEY_ESC:
                leaving = TRUE;
                break;

            default:
            {
                int course = selection - 'a';
                if ((course < 0) || (course > SCHOOL_COURSE_COUNT)
                        || p->school_courses_taken[course])
                {
                    /* invalid course or course already taken */
                    break;
                }

                /* course prices increase with rising difficulty */
                guint price = school_courses[course].course_time
                              * (game_difficulty(nlarn) + 1) * 100;

                if (!building_player_check(p, price))
                {
                    char *msg = g_strdup_printf("You cannot afford "
                            "the %d gold for the course.", price);
                    display_show_message("School", msg, 0);
                    g_free(msg);
                }
                /* check if the selected course has a prerequisite
                   and if the player has taken that course */
                else if ((school_courses[course].prerequisite >= 0) &&
                        !p->school_courses_taken[school_courses[course].prerequisite])
                {
                    char *msg = g_strdup_printf(msg_prerequisite,
                            school_courses[school_courses[course].prerequisite]
                            .description);
                    display_show_message("School", msg, 0);
                    g_free(msg);
                }
                else
                {
                    building_school_take_course(p, course, price);
                }
            }
        }

    }

    return 0;
}

int building_tradepost(player *p)
{
    int turns = 2;
    GPtrArray *callbacks;
    display_inv_callback *callback;

    const char title[] = "Trade Post";

    const char msg_welcome[] = "`white`Welcome to the Larn Trading Post.`end`\n\nWe buy " \
                               "items that explorers no longer find useful.\n" \
                               "Since the condition of the items you bring in " \
                               "is not certain, and we incur great expense in " \
                               "reconditioning the items, we usually pay only " \
                               "20% of their value were they to be new.\n" \
                               "If the items are badly damaged, we will pay " \
                               "only 10% of their new value.";

    g_assert(p != NULL);

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
    callback->helpmsg = "Sell the selected item to the Trade Post.";
    callback->key = 's';
    callback->inv = &nlarn->store_stock;
    callback->function = &building_item_buy;
    callback->checkfun = &player_item_is_sellable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(i)dentify";
    callback->helpmsg = "Have the Trade Post's experts identify your item.";
    callback->key = 'i';
    callback->function = &building_item_identify;
    callback->checkfun = &player_item_is_identifiable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(r)epair";
    callback->helpmsg = "Have the Trade Post's experts revert any damage done to the selected item.";
    callback->key = 'r';
    callback->function = &building_item_repair;
    callback->checkfun = &player_item_is_damaged;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(e)quip";
    callback->helpmsg = "Equip the selected item.";
    callback->key = 'e';
    callback->function = &player_item_equip;
    callback->checkfun = &player_item_is_equippable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(u)nequip";
    callback->helpmsg = "Unequip the selected item.";
    callback->key = 'u';
    callback->function = &player_item_unequip_wrapper;
    callback->checkfun = &player_item_is_unequippable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(n)ote";
    callback->helpmsg = "Add a note to the item.";
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
    const char msg_welcome[] = "`white`Welcome to the Monastery of Larn!`end`\n\n" \
                               "We are here to help you when you are in need of " \
                               "care and offer a fine selection of items that might "
                               "be useful for your quests.\n\n" \
                               "Here you may\n\n" \
                               "  `lightgreen`a`end`) buy something\n" \
                               "  `lightgreen`b`end`) ask for curse removal\n" \
                               "  `lightgreen`c`end`) receive healing\n";

    const char ayfwt[] = "Are you fine with that?";

    gboolean leaving = FALSE;

    while (!leaving)
    {
        GString *msg = g_string_new(msg_welcome);
        int disease_count = 0;

        /* buffer to store all diseases the player currently suffers from */
        struct
        {
            effect_t et;
            const char *desc;
        } curable_diseases[10] = { { 0, NULL } };

        /* fill the list of curable diseases */
        if (player_effect(p, ET_POISON))
        {
            curable_diseases[disease_count].et = ET_POISON;
            curable_diseases[disease_count++].desc = "poison";
        }

        if (player_effect(p, ET_BLINDNESS))
        {
            curable_diseases[disease_count].et = ET_BLINDNESS;
            curable_diseases[disease_count++].desc = "blindness";
        }

        if (player_effect(p, ET_SICKNESS))
        {
            curable_diseases[disease_count].et = ET_SICKNESS;
            curable_diseases[disease_count++].desc = "sickness";
        }

        if (player_effect(p, ET_CLUMSINESS))
        {
            curable_diseases[disease_count].et = ET_CLUMSINESS;
            curable_diseases[disease_count++].desc = "clumsiness";
        }

        if (player_effect(p, ET_DIZZINESS))
        {
            curable_diseases[disease_count].et = ET_DIZZINESS;
            curable_diseases[disease_count++].desc = "dizziness";
        }

        if (player_effect(p, ET_DEC_CON))
        {
            curable_diseases[disease_count].et = ET_DEC_CON;
            curable_diseases[disease_count++].desc = "incapacitation";
        }

        if (player_effect(p, ET_DEC_DEX))
        {
            curable_diseases[disease_count].et = ET_DEC_DEX;
            curable_diseases[disease_count++].desc = "awkwardness";
        }

        if (player_effect(p, ET_DEC_INT))
        {
            curable_diseases[disease_count].et = ET_DEC_INT;
            curable_diseases[disease_count++].desc = "mental deficiency";
        }

        if (player_effect(p, ET_DEC_STR))
        {
            curable_diseases[disease_count].et = ET_DEC_STR;
            curable_diseases[disease_count++].desc = "weakness";
        }

        if (player_effect(p, ET_DEC_WIS))
        {
            curable_diseases[disease_count].et = ET_DEC_WIS;
            curable_diseases[disease_count++].desc = "ignorance";
        }

        /* add found diseases to the menu */
        for (int idx = 0; idx < disease_count; idx++)
        {
            g_string_append_printf(msg, "  `lightgreen`%c`end`) heal from %s\n",
                                   'd' + idx, curable_diseases[idx].desc);
        }

        /* an empty line for the eye */
        g_string_append_c(msg, '\n');

        /* offer the choices to the player */
        display_window *mwin = display_popup(COLS / 2 - 23, LINES / 2 - 3,
                47, title, msg->str, 0);

        int selection = display_getch(mwin->window);

        /* get rid of the temporary string */
        g_string_free(msg, TRUE);

        switch (selection)
        {
        /* shop items */
        case 'a':
            building_shop(p, &nlarn->monastery_stock, title);
            break;

        /* remove curse */
        case 'b':
        {
            item *it;
            int price;
            int choice;
            char *question;
            gchar *desc;

            if (inv_length_filtered(p->inventory, item_filter_cursed_or_unknown) == 0)
            {
                display_show_message(title, "You do not possess any cursed item.", 0);
                break;
            }

            it = display_inventory("Choose an item to uncurse", p, &p->inventory,
                    NULL, FALSE, FALSE, FALSE,item_filter_cursed_or_unknown);

            /* It is possible to abort the selection with ESC. */
            if (it == NULL)
            {
                display_show_message(title, "You chose to leave your items as "
                        "they are.", 0);
                break;
            }

            /* The cost of uncursing is 10 percent of item value.
               The item value for cursed items is reduced by 50%,
               hence divide by 5 */
            price = (item_price(it) / 5) * (game_difficulty(nlarn) + 1);
            /* some items are too cheap. */
            price = max(price, 1);
            /* Item stacks cost per item. */
            price *= it->count;

            desc = item_describe(it, player_item_identified(p, it), FALSE, TRUE);
            question = g_strdup_printf("To remove the curse on %s, we ask you to "
                                       "donate %d gold for our abbey. %s", desc,
                                       price, ayfwt);

            choice = display_get_yesno(question, NULL, NULL, NULL);
            g_free(question);

            if (!choice)
            {
                char *msg = g_strdup_printf("You chose leave the curse on %s.", desc);
                display_show_message(title, msg, 0);
                g_free(msg);
                break;
            }

            /* The player may chose to uncurse items that are actually only of
               unknown blessedness. */
            if (it->cursed)
            {
                char *msg = g_strdup_printf("The monks remove the curse on %s.", desc);
                display_show_message(title, msg, 0);
                g_free(msg);
                item_remove_curse(it);
            }
            else
            {
                char *msg = g_strdup_printf("The monks tell you that %s %sn't "
                        "cursed. Well, now you know for sure...", desc,
                        (it->count == 1) ? "was" : "were");
                display_show_message(title, msg, 0);
                g_free(msg);
                it->blessed_known = TRUE;
            }
            g_free(desc);

            building_player_charge(p, price);
            p->stats.gold_spent_donation += price;
        }
        break;

        /* healing */
        case 'c':
        {
            int choice;
            char *question;
            /* the price for healing depends on the severity of the injury */
            int price = (player_get_hp_max(p) - p->hp) * (game_difficulty(nlarn) + 1);

            if (p->hp == player_get_hp_max(p))
            {
                display_show_message(title, "You are not in need of healing.", 0);
                break;
            }

            question = g_strdup_printf("For healing you, we ask that you "
                                       "donate %d gold for our monastery. %s",
                                       price, ayfwt);
            choice = display_get_yesno(question, NULL, NULL, NULL);
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
                display_show_message(title, "You chose not to be healed.", 0);
            }
        }
        break;

        /* leave the monastery */
        case KEY_ESC:
            leaving = TRUE;
            break;

        default:
            selection -= 'd';
            /* healing of varous negative effects */
            if (selection >= 0 && selection < disease_count)
            {
                int choice;
                char *question;
                effect *e = player_effect_get(p, curable_diseases[selection].et);
                int price = e->turns * (game_difficulty(nlarn) + 1);

                question = g_strdup_printf("For healing you from %s, we ask that you "
                                           "donate %d gold for our monastery. %s",
                                           curable_diseases[selection].desc, price, ayfwt);

                choice = display_get_yesno(question, NULL, NULL, NULL);
                g_free(question);

                if (choice)
                {
                    /* player chose to be healed */
                    player_effect_del(p, e);
                    building_player_charge(p, price);
                    p->stats.gold_spent_donation += price;

                }
                else
                {
                    /* no, thanks */
                    char *msg = g_strdup_printf("You chose not to be cured from %s.",
                                  curable_diseases[selection].desc);
                    display_show_message(title, msg, 0);
                    g_free(msg);
                }

            }
            break;
        }

        /* track time usage */
        player_make_move(p, 2, FALSE, NULL);

        display_window_destroy(mwin);
    }

    return 0;
}

void building_monastery_init()
{
    int i = 0;

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
        for (int j = 0; j < igen[i].count; j++)
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
    callback->helpmsg = "Buy the selected item. If the available quantity exceeds one, you may select the amount you want to purchase.";
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
        log_add_entry(nlarn->log, "We have debited %d gold from your bank "
                "account.", amount);
    }
    else
    {
        player_remove_gold(p, amount);
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
    gpointer ioid = NULL; /* oid of purchased item */
    char text[81];
    gchar *name;

    /* The item the player actually bought. Usually this is the passed
     * item, in the case of item stacks it might be a new item. */
    item *bought_itm = it;

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    price = item_price(it);

    if (it->count > 1)
    {
        g_snprintf(text, 80, "How many %s do you want to buy?",
                it->type == IT_AMMO ? ammo_name(it) : item_name_pl(it->type));

        /* get count */
        guint count = display_get_count(text, it->count);

        if (count > it->count)
        {
            /* desired amount is larger than the available amount */
            log_add_entry(nlarn->log, "Wouldn't it be nice if the store "
                          "had %d of those?", count);
            return;
        }
        else if (count == 0)
        {
            /* not buying anything */
            return;
        }
        else if (count < it->count)
        {
            /* player wants part of the stock */
            bought_itm = item_split(it, count);
        }

        price *= count;

        if (!building_player_check(p, price))
        {
            name = item_describe(bought_itm, TRUE, FALSE, TRUE);
            g_snprintf(text, 80, "You cannot afford the %d gold for %s.",
                       price, name);
            g_free(name);

            display_show_message(NULL, text, 0);

            /* if the item has been split add it to the shop */
            if (it != bought_itm) inv_add(inv, bought_itm);

            return;
        }
    }
    else
    {
        name = item_describe(it, TRUE, TRUE, TRUE);
        g_snprintf(text, 80, "Do you want to buy %s for %d gold?",
                   name, price);
        g_free(name);

        if (!display_get_yesno(text, NULL, NULL, NULL))
            return;
    }

    /* prepare the item description for logging later */
    name = item_describe(bought_itm, TRUE, FALSE, FALSE);

    /* try to transfer the item to the player's inventory */
    ioid = bought_itm->oid;

    if (inv_add(&p->inventory, bought_itm))
    {
        /* the item has been added to player's inventory */
        if (it == bought_itm)
        {
            /* remove the item from the shop as the player has bought
               the entire stock. this has to be done by the oid as it_clone
               may have been destroyed if it was a stackable item. */
            inv_del_oid(inv, ioid);
        }

        p->stats.items_bought    += bought_itm->count;
        p->stats.gold_spent_shop += price;

        /* identify the item */
        player_item_identify(p, &p->inventory, bought_itm);
    }
    else
    {
        /* item has not been added to player's inventory */
        /* if the item has been split, return it to the shop */
        if (it != bought_itm) inv_add(inv, bought_itm);
        return;
    }

    /* log the event */
    log_add_entry(nlarn->log, "You buy %s. Thank you for your purchase.", name);
    g_free(name);

    /* charge player for this purchase */
    building_player_charge(p, price);

    player_make_move(p, 2, FALSE, NULL);
}

static void building_item_identify(player *p, inventory **inv __attribute__((unused)), item *it)
{
    guint price;
    gchar *name_unknown;
    char message[81];

    const char title[] = "Identify item";

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* The cost for identification is 10% of the item's base price. */
    price = (item_base_price(it) / 10) * (game_difficulty(nlarn) + 1);

    /* Ensure it costs at least 50gp... */
    price = max(50, price);

    name_unknown = item_describe(it, player_item_known(p, it), FALSE, TRUE);

    if (building_player_check(p, price))
    {
        g_snprintf(message, 80, "Pay %d gold to identify %s?",
                   price, name_unknown);

        if (display_get_yesno(message, NULL, NULL, NULL))
        {
            player_item_identify(p, NULL, it);
            /* upper case first letter */
            name_unknown[0] = g_ascii_toupper(name_unknown[0]);
            gchar *name_known = item_describe(it, player_item_known(p, it), TRUE, FALSE);

            log_add_entry(nlarn->log, "%s is %s.", name_unknown, name_known);
            g_free(name_known);

            building_player_charge(p, price);
            p->stats.gold_spent_id_repair += price;
            player_make_move(p, 1, FALSE, NULL);
        }
    }
    else
    {
        g_snprintf(message, 80, "Identifying %s costs %d gold.",
                   name_unknown, price);
        display_show_message(title, message, 0);
    }

    g_free(name_unknown);
}

static void building_item_repair(player *p, inventory **inv __attribute__((unused)), item *it)
{
    int damages = 0;
    guint price;
    gchar *name;
    char message[81];

    const char title[] = "Repair item";

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* determine how much the item is damaged */
    damages += it->burnt;
    damages += it->corroded;
    damages += it->rusty;

    /* The cost of repairing an item is 10% of the item's base price. */
    price = (item_base_price(it) / 10) * (game_difficulty(nlarn) + 1);

    /* Take the level of damage into account. */
    price *= damages;

    /* Charge repair of item stacks depending on the quantity. */
    if (item_is_stackable(it->type)) price *= it->count;

    /* If an item is blessed and the player know it, use this and charge :) */
    if (it->blessed && it->blessed_known) price <<=1;

    /* Ensure this costs at least 50gp... */
    price = max(50, price);

    name = item_describe(it, player_item_known(p, it), FALSE, TRUE);

    if (building_player_check(p, price))
    {
        g_snprintf(message, 80, "Pay %d gold to repair %s?", price, name);

        if (display_get_yesno(message, NULL, NULL, NULL))
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

    g_free(name);
}

static void building_item_buy(player *p, inventory **inv, item *it)
{
    int price;
    guint count = 0;
    char question[121];
    gchar *name;

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    price = item_price(it);

    /* modify price if player sells stuff at the trading post */
    if (map_sobject_at(game_map(nlarn, Z(p->pos)), p->pos) == LS_TRADEPOST)
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
        g_snprintf(question, 120, "How many %s do you want to sell for %d gold?",
                it->type == IT_AMMO ? ammo_name(it) : item_name_pl(it->type), price);

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
        name = item_describe(it, player_item_known(p, it), TRUE, TRUE);
        g_snprintf(question, 120, "Do you want to sell %s for %d gold?",
                   name, price);

        g_free(name);

        if (!display_get_yesno(question, NULL, NULL, NULL))
            return;
    }

    /* put the money on the player's bank account */
    p->bank_account += price;
    /* and charge income taxes... */
    p->outstanding_taxes += calc_tax_debt(price);

    guint count_orig = it->count;
    it->count = count;

    name = item_describe(it, player_item_known(p, it), FALSE, FALSE);
    log_add_entry(nlarn->log, "You sell %s. The %d gold %s been "
                  "transferred to your bank account.",
                  name, price, (price == 1) ? "has" : "have");

    g_free(name);
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

static guint calc_tax_debt(guint income)
{
    /* charge 5% income tax per difficulty level */
    return (income * min(game_difficulty(nlarn), 4) / 20);
}
