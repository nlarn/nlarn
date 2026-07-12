/*
 * buildings.c
 * Copyright (C) 2009-2026 Joachim de Groot <jdegroot@web.de>
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
#include <glib/gi18n.h>

#include "config.h"
#include "container.h"
#include "display.h"
#include "extdefs.h"
#include "game.h"
#include "items.h"
#include "player.h"
#include "scrolls.h"
#include "weapons.h"

static const char msg_outstanding[] = N_("The Larn Revenue Service has ordered "
                                      "us to not do business with tax evaders. "
                                      "They have also told us that you owe back "
                                      "taxes and, as we must comply with the "
                                      "law, we cannot serve you at this time."
                                      "\n\nSo Sorry.");

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

    /* add the interest to the bank account... */
    g->p->bank_account += interest;

    /* ...and the statistics... */
    g->p->stats.gold_bank_interest += interest;

    /* ...and keep track of the amount paid since the player's last visit to the bank */
    g->p->bank_ieslvtb += interest;

    /* calculate the tax debt */
    g->p->outstanding_taxes += calc_tax_debt(interest);

}

int building_bank(player *p)
{
    guint amount = 0;
    GString *greeting;

    const char *msg_title = _("First National Bank of Larn");
    const char *msg_greet = _("`EMPH`Welcome to the First National Bank of Larn.`end`\n\n");

    const char *msg_branch = _("Welcome to the 5th level branch office of the "
                               "First National Bank of Larn.\n\n");

    const char *msg_frozen = _("The Larn Revenue Service has ordered that your "
                               "account be frozen until all levied taxes have "
                               "been paid.  They have also told us that you "
                               "owe %d gold in taxes, and we must comply with "
                               "them. We cannot serve you at this time. Sorry.\n\n"
                               "We suggest you go to the LRS office and pay your taxes.");

    g_assert(p != NULL);

    if (Z(p->pos) == 0)
        greeting = g_string_new(msg_greet);
    else
        greeting = g_string_new(msg_branch);

    if (p->bank_ieslvtb > 0)
    {
        /* show the earned interest */
        g_string_append_printf(greeting, _("We have paid you an interest of %d "
                               "gold since your last visit.\n"), p->bank_ieslvtb);

        /* add it to the game log for later reference */
        log_add_entry(nlarn->log, _("The bank has paid you an interest of %d "
                      "gold since your last visit."), p->bank_ieslvtb);

        /* reset the value */
        p->bank_ieslvtb = 0;
    }

    /* leave the town bank when taxes are unpaid */
    if (0 == Z(p->pos) && p->outstanding_taxes)
    {
        g_string_append_printf(greeting, msg_frozen, p->outstanding_taxes);
        display_show_message(msg_title, greeting->str, 0);
        g_string_free(greeting, true);

        return 2; /* turns */
    }

    bool leaving = false;
    while (!leaving)
    {
        GString *text = g_string_new(greeting->str);

        g_string_append_printf(text,
                _("You have %d gold pieces in the bank.\n"
                  "You have %d gold pieces.\n\n"),
                p->bank_account, player_get_gold(p));

        g_string_append(text, _("Your wish? "));

        /* Assemble the list of currently available actions. The switch
           below is keyed by the action letter, not by the (translated)
           hotkey, which display_menu() derives from each label. */
        const char *labels[3];
        int actions[3];
        guint n = 0;

        if (player_get_gold(p) > 0)
        {
            labels[n] = _("`KEY`d`end`)eposit ");
            actions[n++] = 'd';
        }

        if (p->bank_account > 0)
        {
            labels[n] = _("`KEY`w`end`)ithdraw ");
            actions[n++] = 'w';
        }

        /* if player has gems, enable selling them */
        if (inv_length_filtered(p->inventory, item_filter_gems))
        {
            labels[n] = _("`KEY`s`end`)ell a gem");
            actions[n++] = 's';
        }

        int choice = display_menu(msg_title, text->str, labels, n);

        g_string_free(text, true);

        int cmd = (choice < 0) ? KEY_ESC : actions[choice];

        switch (cmd)
        {
        case 'd': /* deposit */
            if (inv_length_filtered(p->inventory, item_filter_gold) == 0)
                break;

            amount = display_get_count(_("How many gold pieces do you wish to deposit?"),
                                       player_get_gold(p));

            if (amount && (amount <= player_get_gold(p)))
            {
                p->bank_account += amount;
                player_remove_gold(p, amount);
                log_add_entry(nlarn->log, _("You deposited %d gold."), amount);

                /* income tax for money earned in the caverns */
                p->outstanding_taxes += calc_tax_debt(amount);
            }
            else if (amount)
            {
                log_add_entry(nlarn->log, _("You don't have that much."));
            }

            break;

        case 'w': /* withdraw */
            if (p->bank_account == 0)
                break;

            amount = display_get_count(_("How many gold pieces do you wish to withdraw?"),
                                       p->bank_account);

            if (amount && (amount <= p->bank_account))
            {
                item *gold = item_new(IT_GOLD, amount);

                /* adding the gold might fail (too heavy) */
                if (inv_add(&p->inventory, gold))
                {
                    p->bank_account -= amount;
                    log_add_entry(nlarn->log, _("You withdraw %d gold."), amount);
                }
                else
                {
                    /* this item is no longer required */
                    item_destroy(gold);
                }
            }
            else if (amount)
            {
                log_add_entry(nlarn->log, _("You don't have that much in the bank!"));
            }

            break;

        case 's': /* sell gem */
            {
                if (inv_length_filtered(p->inventory, item_filter_gems) == 0)
                    break;

                /* define callback functions */
                GPtrArray *callbacks = g_ptr_array_new();

                display_inv_callback *callback = g_malloc(sizeof(display_inv_callback));
                callback->description = _("(`KEY`s`end`)ell");
                callback->helpmsg = _("Sell the currently selected gem.");
                callback->key = 's';
                callback->inv = &nlarn->store_stock;
                callback->function = &building_item_buy;
                callback->checkfun = &player_item_is_sellable;
                callback->active = false;
                g_ptr_array_add(callbacks, callback);

                display_inventory(_("Sell gems"), p, &p->inventory, callbacks,
                        true, false, true, &item_filter_gems);

                display_inv_callbacks_clean(callbacks);
            }
            break;

        case KEY_ESC:
            leaving = true;
            break;

        default:
            /* do nothing */
            break;
        }

        /* every interaction in the bank takes two turns */
        player_make_move(p, 2, false, NULL);
    }

    g_string_free(greeting, true);

    return 0;
}

int building_dndstore(player *p)
{
    int turns = 2;
    const char *title = _("DND store");
    const char *msg_welcome = _("`EMPH`Welcome to the Larn Thrift Shoppe.`end`\n\n"
                                "We stock many items explorers find useful in "
                                "their adventures. Feel free to browse to your "
                                "heart's content. Also be advised that if you "
                                "break 'em, you pay for 'em.");

    g_assert(p != NULL);

    /* no business if player has outstanding taxes */
    if (p->outstanding_taxes)
    {
        display_show_message(title, _(msg_outstanding), 0);
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
                it->bonus_known = true;
                it->blessed_known = true;
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

    const char *title = _("Your home");

    const char *msg_home = _("`EMPH`Welcome home, %s.`end`\n\nLatest word from the doctor "
                             "is not good. The diagnosis is confirmed as "
                             "dianthroritis. He guesses that your daughter "
                             "has only %d mobuls left in this world. It's "
                             "up to you, %s, to find the only hope for your "
                             "daughter, the very rare potion of cure "
                             "dianthroritis.  It is rumored that only deep in "
                             "the depths of the caves can this potion be found.\n");

    const char *msg_found = _("`EMPH`Congratulations!`end` You found the potion of cure "
                              "dianthroritis! Frankly, No one thought you "
                              "could do it. Boy! Did you surprise them!\n\n");

    const char *msg_died = _("The doctor has the sad duty to inform you that "
                             "your daughter died before your return. There was "
                             "nothing he could do without the potion.\n\n"
                             "You didn't make it in time. Too bad...");

    const char *msg_won = _("The doctor is now administering the potion and, in "
                            "a few moments, your daughter should be well on "
                            "her way to recovery.\n\nThe potion is working! "
                            "The doctor thinks that your daughter will recover "
                            "in a few days.\n\nCongratulations!");

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
            display_show_message(_("You saved your daughter!"), text->str, 0);
            g_string_free(text, true);

            /* remove the potion from the inventory as it has been used up */
            item *pcd = inv_get_filtered(p->inventory, 0, item_filter_pcd);
            inv_del_element(&p->inventory, pcd);
            item_destroy(pcd);

            /* increase difficulty level */
            config.difficulty += 1;

            /* write back modified config */
            write_ini_file(nlarn_inifile, &config);

            player_die(p, PD_WON, 0);
        }
        else
        {
            /* lost the game */
            g_string_append(text, msg_died);
            display_show_message(_("You were too late!"), text->str, 0);
            g_string_free(text, true);

            player_die(p, PD_TOO_LATE, 0);
        }
    }

    if (game_turn(nlarn) > TIMELIMIT)
    {
        /* too late, no potion */
        text = g_string_new(msg_died);
        display_show_message(_("You were too late!"), text->str, 0);
        g_string_free(text, true);

        player_die(p, PD_LOST, 0);
    }

    /* casual visit, report remaining time */
    bool leaving = false;

    while (!leaving)
    {
        GPtrArray *callbacks;
        display_inv_callback *callback;

        text = g_string_new(NULL);
        g_string_printf(text, msg_home, p->name,
                        gtime2mobuls(game_remaining_turns(nlarn)),
                        p->name);

        /* Assemble the list of currently available actions. The switch
           below is keyed by the action letter, not by the (translated)
           hotkey, which display_menu() derives from each label. */
        const char *labels[2];
        int actions[2];
        guint n = 0;

        if (inv_length_filtered(p->inventory, player_item_not_equipped) > 0)
        {
            labels[n] = _("`KEY`d`end`)eposit something here");
            actions[n++] = 'd';
        }

        if (inv_length(nlarn->player_home) > 0)
        {
            labels[n] = _("`KEY`t`end`)ake something with you");
            actions[n++] = 't';
        }

        int choice = display_menu(title, text->str, labels, n);
        g_string_free(text, true);

        int cmd = (choice < 0) ? KEY_ESC : actions[choice];

        switch (cmd)
        {
            /* deposit something */
        case 'd':
            if (inv_length_filtered(p->inventory, player_item_not_equipped) > 0)
            {
                /* prepare callback functions */
                callbacks = g_ptr_array_new();

                callback = g_malloc0(sizeof(display_inv_callback));
                callback->description = _("(`KEY`d`end`)eposit");
                callback->helpmsg = _("Deposit the selected item in "
                                      "your storage room at home.");
                callback->key = 'd';
                callback->inv = &nlarn->player_home;
                callback->function = &container_item_add;
                callback->active = true;

                g_ptr_array_add(callbacks, callback);

                display_inventory(title, p, &p->inventory, callbacks, false,
                                  true, false, player_item_not_equipped);

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
                callback->description = _("(`KEY`t`end`)ake");
                callback->helpmsg = _("Take the selected item out of your "
                                      "storage room and put it into your pack.");
                callback->key = 't';
                callback->inv = &nlarn->player_home;
                callback->function = &container_item_unpack;
                callback->active = true;

                g_ptr_array_add(callbacks, callback);

                callback = g_malloc0(sizeof(display_inv_callback));
                callback->description = _("take (`KEY`a`end`)ll");
                callback->helpmsg = _("Take all items from your storage room. "
                                      "Only available when you can carry the total weight.");
                callback->key = 'a';
                callback->inv = &nlarn->player_home;
                callback->checkfun = &player_can_carry_all;
                callback->function = &container_items_unpack_all;

                g_ptr_array_add(callbacks, callback);

                display_inventory(title, p, &nlarn->player_home, callbacks,
                                  false, true, false, NULL);

                display_inv_callbacks_clean(callbacks);
            }
            break;
            /* leave */
        case KEY_ESC:
            leaving = true;
            break;
        }
    }

    return turns;
}

int building_lrs(player *p)
{
    int turns = 2;

    const char *title = _("Larn Revenue Service");
    const char *msg_greet = _("Welcome to the Larn Revenue Service district office.\n\n");
    const char *msg_taxes = _("You presently owe %d gold in taxes.\n\n");
    const char *msg_notax = _("You do not owe us any taxes.");

    g_assert(p != NULL);

    GString *text = g_string_new(msg_greet);

    if (p->outstanding_taxes)
    {
        g_string_append_printf(text, msg_taxes, p->outstanding_taxes);

        /* offer to pay taxes if player can afford to */
        if (building_player_check(p, p->outstanding_taxes))
        {
            g_string_append(text, _("Do you want to pay your taxes?"));

            if (display_get_yesno(text->str, title, NULL, NULL))
            {
                building_player_charge(p, p->outstanding_taxes);
                p->stats.gold_spent_taxes += p->outstanding_taxes;
                p->outstanding_taxes = 0;
                log_add_entry(nlarn->log, _("You have paid your taxes."));
            }
            else
            {
                log_add_entry(nlarn->log, _("You chose not to pay your taxes."));
            }
        }
        else
        {
            g_string_append(text, _("Unfortunately, it seems that you cannot "
                    "afford to pay your taxes at this time."));
            display_show_message(title, text->str, 0);
        }
    }
    else
    {
        g_string_append(text, msg_notax);
        display_show_message(title, text->str, 0);
    }

    g_string_free(text, true);

    return turns;
}

static int building_scribe_scroll(player *p)
{
    const int turns = 2;
    int i;
    bool split = false;
    char question[81] = { 0 };

    /* check if the player owns a blank scroll */
    if (!inv_length_filtered(p->inventory, item_filter_blank_scroll))
    {
        display_show_message(_("School"), _("To craft a scroll, "
            "the scribes must first be given a blank one."), 0);

        return turns;
    }

    item *blank = display_inventory(_("Choose a scroll to inscribe"), p,
                                      &p->inventory, NULL, false, false,
                                      false, item_filter_blank_scroll);

    if (!blank)
    {
        log_add_entry(nlarn->log, _("Okay then."));
        return turns;
    }

    char *new_scroll = display_get_string(_("School"),
        _("Which incantation shall we bind in ink?"), NULL, 45);

    if (new_scroll == NULL)
    {
        display_show_message(_("School"), _("Okay then."), 0);
        return turns;
    }

    for (i = 0; i < ST_MAX; i++)
    {
        if (g_strcmp0(new_scroll, scrolls[i].name) == 0
                || g_strcmp0(new_scroll,
                    noun_phrase(g_dpgettext2(NULL, "scroll", scrolls[i].name),
                                ART_NONE, GC_NOM, false, false)) == 0)
            break;
    }

    /* free memory allocated by display_get_string */
    g_free(new_scroll);

    if (i == ST_MAX)
    {
        display_show_message(_("School"),
            _("Even the oldest scribes know nothing of a scroll like that. "
              "Alas, the scroll must remain blank - for even the greatest "
              "scribes cannot write what does not exist."), 0);

        return turns;
    }

    /* jesters might want to get a scroll of blank paper */
    if (i == ST_BLANK)
    {
        display_show_message(_("School"), _("A blank scroll, "
            "by definition, needs no scribe."), 0);

        return turns;
    }

    /* player has chosen which scroll to write, check if they can afford it */
    const int price = 2 * scrolls[i].price;
    if (!building_player_check(p, price))
    {
        char *msg = g_strdup_printf(_("You do not possess the %d gold necessary "
            "to acquire the scroll of %s. For now, the scribes shall keep "
            "their quills dry."), price,
            noun_phrase(g_dpgettext2(NULL, "scroll", scrolls[i].name),
                        ART_NONE, GC_NOM, false, false));

        display_show_message(_("School"), msg, 0);
        g_free(msg);

        return turns;
    }

    g_snprintf(question, 80, _("The crafting of a scroll of %s demands a fee of"
        " %d gold.\nWould that be agreeable to you?"),
        noun_phrase(g_dpgettext2(NULL, "scroll", scrolls[i].name),
                    ART_NONE, GC_NOM, false, false), price);

    if (!display_get_yesno(question, _("School"), NULL, NULL))
    {
        return turns;
    }

    /** Okay, we write the scroll. */

    // If necessary, split a stack of scrolls.
    if (blank->count > 1)
    {
        blank = item_split(blank, 1);
        split = true;
    }

    blank->id = i;
    p->identified_scrolls[i] = true;

    building_player_charge(p, price);
    p->stats.gold_spent_shop += price;

    /* writing a scroll takes 10 mobuls */
    player_make_move(p, 10 * MOBUL, false, _("waiting for the scribes to write a "
            "scroll of %s for you"), scroll_name_gen(blank));

    log_add_entry(nlarn->log,
            _("The scribes have completed a scroll of %s on your behalf."),
            scroll_name_gen(blank));

    if (split)
        inv_add(&p->inventory, blank);

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
    { 10, -1, N_("Fighters Training I"), N_("You feel stronger!") },
    { 15,  0, N_("Fighters Training II"), N_("You feel much stronger!") },
    { 10, -1, N_("Introduction to Wizardry"), N_("The task before you now seems more attainable!") },
    { 20,  2, N_("Applied Wizardry"), N_("The task before you now seems very attainable!") },
    { 10, -1, N_("Faith for Today"), N_("You now feel more confident that you can find the potion in time!") },
    { 10, -1, N_("Contemporary Dance"), N_("You feel like dancing!") },
    {  5, -1, N_("History of Larn"), N_("Your instructor told you that the Eye of Larn is rumored to be guarded by an invisible demon lord.") },
};

static void building_school_take_course(player *p, int course, guint price)
{

    /* charge the player */
    building_player_charge(p, price);
    p->stats.gold_spent_college += price;

    /* time usage */
    guint course_turns = mobuls2gtime(school_courses[course].course_time);
    player_make_move(p, course_turns, false, _("taking the course \"%s\""),
            _(school_courses[course].description));

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
            _("You successfully complete the course \"%s\". %s"),
            _(school_courses[course].description),
            _(school_courses[course].message));
}

int building_school(player *p)
{
    const char *msg_greet = _("`EMPH`Welcome to the College of Larn!`end`\n\n"
                              "We offer the exciting opportunity of higher "
                              "education to all inhabitants of the caves. "
                              "Here is a list of the class schedule:\n\n");

    const char *msg_prerequisite = _("Sorry, but this class has a prerequisite of \"%s\".");

    g_assert(p != NULL);

    bool leaving = false;
    while (!leaving)
    {
        GString *text = g_string_new(msg_greet);

        for (int idx = 0; idx < SCHOOL_COURSE_COUNT; idx++)
        {
            /* pre-pad the course name to 24 display columns; the
               field width in the translated format counts bytes and
               would pad multi-byte characters short */
            const char *cdesc = _(school_courses[idx].description);
            g_autofree gchar *course = g_strdup_printf("%-*s",
                    utf8_pad(cdesc, 24), cdesc);

            if (!p->school_courses_taken[idx])
            {
                /* courses become more expensive with rising difficulty */
                guint price = school_courses[idx].course_time
                              * (game_difficulty(nlarn) + 1) * 100;

                g_string_append_printf(text,
                        _(" `KEY`%c`end`) `EMPH`%-24s`end` - %2d mobuls, %4d gold\n"),
                        idx + 'a', course,
                        school_courses[idx].course_time, price);
            }
            else
            {
                g_string_append_printf(text, _("    %-24s - attended\n"),
                        course);
            }
        }

        const char *commission = _("Commission a scroll");
        g_autofree gchar *commission_pad = g_strdup_printf("%-*s",
                utf8_pad(commission, 24), commission);
        g_string_append_printf(text, _("\nAlternatively,\n"
                " `KEY`%c`end`) %-24s - 10 mobuls\n\n"),
                SCHOOL_COURSE_COUNT + 'a', commission_pad);

        int selection = display_show_message(_("School"), text->str, 0);
        g_string_free(text, true);

        switch (selection)
        {
            case 'a' + SCHOOL_COURSE_COUNT:
                player_make_move(p, building_scribe_scroll(p), false, NULL);
                break;

            case KEY_ESC:
                leaving = true;
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
                    char *msg = g_strdup_printf(_("You cannot afford "
                            "the %d gold for the course."), price);
                    display_show_message(_("School"), msg, 0);
                    g_free(msg);
                }
                /* check if the selected course has a prerequisite
                   and if the player has taken that course */
                else if ((school_courses[course].prerequisite >= 0) &&
                        !p->school_courses_taken[school_courses[course].prerequisite])
                {
                    char *msg = g_strdup_printf(msg_prerequisite,
                            _(school_courses[school_courses[course].prerequisite]
                            .description));
                    display_show_message(_("School"), msg, 0);
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

    const char *title = _("Trade Post");

    const char *msg_welcome = _("`EMPH`Welcome to the Larn Trading Post.`end`\n\nWe buy "
                                "items that explorers no longer find useful.\n"
                                "Since the condition of the items you bring in "
                                "is not certain, and we incur great expense in "
                                "reconditioning the items, we usually pay only "
                                "20% of their value were they to be new.\n"
                                "If the items are badly damaged, we will pay "
                                "only 10% of their new value.");

    g_assert(p != NULL);

    /* no business if player has outstanding taxes */
    if (p->outstanding_taxes)
    {
        display_show_message(title, _(msg_outstanding), 0);
        return turns;
    }

    /* define callback functions */
    GPtrArray *callbacks = g_ptr_array_new();

    display_inv_callback *callback = g_malloc(sizeof(display_inv_callback));
    callback->description = _("(`KEY`s`end`)ell");
    callback->helpmsg = _("Sell the selected item to the Trade Post.");
    callback->key = 's';
    callback->inv = &nlarn->store_stock;
    callback->function = &building_item_buy;
    callback->checkfun = &player_item_is_sellable;
    callback->active = false;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = _("(`KEY`i`end`)dentify");
    callback->helpmsg = _("Have the Trade Post's experts identify your item.");
    callback->key = 'i';
    callback->function = &building_item_identify;
    callback->checkfun = &player_item_is_identifiable;
    callback->active = false;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = _("(`KEY`r`end`)epair");
    callback->helpmsg = _("Have the Trade Post's experts revert any damage done to the selected item.");
    callback->key = 'r';
    callback->function = &building_item_repair;
    callback->checkfun = &player_item_is_damaged;
    callback->active = false;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = _("(`KEY`e`end`)quip");
    callback->helpmsg = _("Equip the selected item.");
    callback->key = 'e';
    callback->function = &player_item_equip;
    callback->checkfun = &player_item_is_equippable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = _("(`KEY`u`end`)nequip");
    callback->helpmsg = _("Unequip the selected item.");
    callback->key = 'u';
    callback->function = &player_item_unequip_wrapper;
    callback->checkfun = &player_item_is_unequippable;
    callback->active = false;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = _("(`KEY`n`end`)ote");
    callback->helpmsg = _("Add a note to the item.");
    callback->key = 'n';
    callback->function = &player_item_notes;
    callback->checkfun = NULL;
    callback->active = false;
    g_ptr_array_add(callbacks, callback);

    display_show_message(title, msg_welcome, 0);
    display_inventory(title, p, &p->inventory, callbacks, false,
                      false, true, &item_filter_not_gold);

    /* clean up */
    display_inv_callbacks_clean(callbacks);

    return turns;
}

int building_monastery(struct player *p)
{
    const char *title = _("The Monastery of Larn");
    const char *msg_welcome =
        _("`EMPH`Welcome, traveler, to the Monastery of Larn.`end`\n\n"
          "Peace be upon you. The walls of the monastery offer respite to all "
          "who seek it.\n\nSpeak your need, and if it lies within our calling, "
          "it shall be granted... for a fair offering. We tend the wounded and "
          "weary, and offer a small selection of tools and provisions to aid "
          "those who walk the road of trials.");

    const char *ayfwt = _("Shall we proceed, then?");

    bool leaving = false;

    while (!leaving)
    {
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
            curable_diseases[disease_count++].desc = N_("poison");
        }

        if (player_effect(p, ET_BLINDNESS))
        {
            curable_diseases[disease_count].et = ET_BLINDNESS;
            curable_diseases[disease_count++].desc = N_("blindness");
        }

        if (player_effect(p, ET_SICKNESS))
        {
            curable_diseases[disease_count].et = ET_SICKNESS;
            curable_diseases[disease_count++].desc = N_("sickness");
        }

        if (player_effect(p, ET_CLUMSINESS))
        {
            curable_diseases[disease_count].et = ET_CLUMSINESS;
            curable_diseases[disease_count++].desc = N_("clumsiness");
        }

        if (player_effect(p, ET_DIZZINESS))
        {
            curable_diseases[disease_count].et = ET_DIZZINESS;
            curable_diseases[disease_count++].desc = N_("dizziness");
        }

        if (player_effect(p, ET_DEC_CON))
        {
            curable_diseases[disease_count].et = ET_DEC_CON;
            curable_diseases[disease_count++].desc = N_("incapacitation");
        }

        if (player_effect(p, ET_DEC_DEX))
        {
            curable_diseases[disease_count].et = ET_DEC_DEX;
            curable_diseases[disease_count++].desc = N_("awkwardness");
        }

        if (player_effect(p, ET_DEC_INT))
        {
            curable_diseases[disease_count].et = ET_DEC_INT;
            curable_diseases[disease_count++].desc = N_("mental deficiency");
        }

        if (player_effect(p, ET_DEC_STR))
        {
            curable_diseases[disease_count].et = ET_DEC_STR;
            curable_diseases[disease_count++].desc = N_("weakness");
        }

        if (player_effect(p, ET_DEC_WIS))
        {
            curable_diseases[disease_count].et = ET_DEC_WIS;
            curable_diseases[disease_count++].desc = N_("ignorance");
        }

        /* Assemble the menu: three fixed options followed by one per
           healable disease. display_menu() returns the position of the
           chosen option, so the switch dispatches by index. */
        const char *labels[3 + G_N_ELEMENTS(curable_diseases)];
        char *disease_labels[G_N_ELEMENTS(curable_diseases)];
        guint n = 0;

        labels[n++] = _("`KEY`a`end`) buy something");
        labels[n++] = _("`KEY`b`end`) ask for curse removal");
        labels[n++] = _("`KEY`c`end`) receive healing");

        for (int idx = 0; idx < disease_count; idx++)
        {
            disease_labels[idx] = g_strdup_printf(_("`KEY`%c`end`) heal from %s"),
                                                  'd' + idx, _(curable_diseases[idx].desc));
            labels[n++] = disease_labels[idx];
        }

        int selection = display_menu(title, msg_welcome, labels, n);

        for (int idx = 0; idx < disease_count; idx++)
            g_free(disease_labels[idx]);

        switch (selection)
        {
        /* buy something */
        case 0:
            building_shop(p, &nlarn->monastery_stock, title);
            break;

        /* remove curse */
        case 1:
        {
            if (inv_length_filtered(p->inventory, item_filter_cursed_or_unknown) == 0)
            {
                display_show_message(title, _("There is no sign of curse upon "
                    "your possessions."), 0);
                break;
            }

            item *it = display_inventory(_("Choose an item to uncurse"), p, &p->inventory,
                                         NULL, false, false, false, item_filter_cursed_or_unknown);

            /* It is possible to abort the selection with ESC. */
            if (it == NULL)
            {
                display_show_message(title, _("Very well - your belongings "
                    "shall remain as they were."), 0);

                break;
            }

            /* The cost of uncursing is 10 percent of item value.
               The item value for cursed items is reduced by 50%,
               hence divide by 5 */
            int price = (item_price(it) / 5) * (game_difficulty(nlarn) + 1);
            /* some items are too cheap. */
            price = max(price, 1);
            /* Item stacks cost per item. */
            price *= it->count;

            /* the item is governed by "from" (German: dative) */
            gchar *desc = item_describe_gc(it, player_item_identified(p, it),
                                           false, true, GC_DAT);
            char *question = g_strdup_printf(_("To unbind the curse from %s, we "
                "ask but %d gold in humble support of the abbey. %s"),
                desc, price, ayfwt);

            int choice = display_get_yesno(question, NULL, NULL, NULL);
            g_free(question);

            if (!choice)
            {
                char *msg = g_strdup_printf(_("Thus, you choose to let %s remain unaltered."), desc);
                display_show_message(title, msg, 0);
                g_free(msg);
                break;
            }

            /* The player may choose to uncurse items that are actually only of
               unknown blessedness. */
            if (it->cursed)
            {
                char *msg = g_strdup_printf(_("The monks remove the curse on %s."), desc);
                display_show_message(title, msg, 0);
                g_free(msg);
                item_remove_curse(it);
            }
            else
            {
                char *msg = g_strdup_printf((it->count == 1)
                        ? _("The monks tell you that %s wasn't cursed. You may "
                            "now hold this knowledge with certainty.")
                        : _("The monks tell you that %s weren't cursed. You may "
                            "now hold this knowledge with certainty."), desc);
                display_show_message(title, msg, 0);
                g_free(msg);
                it->blessed_known = true;
            }
            g_free(desc);

            building_player_charge(p, price);
            p->stats.gold_spent_donation += price;
        }
        break;

        /* receive healing */
        case 2:
        {
            /* the price for healing depends on the severity of the injury */
            int price = (player_get_hp_max(p) - p->hp) * (game_difficulty(nlarn) + 1);

            if (p->hp == player_get_hp_max(p))
            {
                display_show_message(title, _("You bear no wounds that require "
                    "our aid."), 0);
                break;
            }

            char *question = g_strdup_printf(_("To grant you healing, we humbly "
                "request a donation of %d gold to our monastery. %s"), price, ayfwt);
            int choice = display_get_yesno(question, NULL, NULL, NULL);
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
                display_show_message(title, _("So be it, the healing shall wait."), 0);
            }
        }
        break;

        /* leave the monastery */
        case -1:
            leaving = true;
            break;

        default:
        {
            /* healing of various negative effects; options 3.. map to
               the curable diseases 0.. */
            int sel = selection - 3;
            if (sel >= 0 && sel < disease_count)
            {
                effect *e = player_effect_get(p, curable_diseases[sel].et);
                int price = e->turns * (game_difficulty(nlarn) + 1);

                char *question = g_strdup_printf(_("To cleanse you of %s, we "
                    "humbly request a donation of %d gold to our monastery. "
                    "%s"), _(curable_diseases[sel].desc), price, ayfwt);

                int choice = display_get_yesno(question, NULL, NULL, NULL);
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
                    char *msg = g_strdup_printf(_("So be it, you chose not to "
                        "be cured from %s."), _(curable_diseases[sel].desc));
                    display_show_message(title, msg, 0);
                    g_free(msg);
                }
            }
        }
        break;
        }

        /* track time usage */
        player_make_move(p, 2, false, NULL);
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
    if (inv_length(*inv) == 0)
    {
        log_add_entry(nlarn->log, _("Unfortunately we are sold out."));
        return;
    }

    /* define callback functions */
    GPtrArray *callbacks = g_ptr_array_new();

    display_inv_callback *callback = g_malloc(sizeof(display_inv_callback));
    callback->description = _("(`KEY`b`end`)uy");
    callback->helpmsg = _("Buy the selected item. If the available quantity exceeds one, you may select the amount you want to purchase.");
    callback->key = 'b';
    callback->inv = inv;
    callback->function = &building_item_sell;
    callback->checkfun = &player_item_is_affordable;
    callback->active = false;
    g_ptr_array_add(callbacks, callback);

    display_inventory(title, p, inv, callbacks, true,
                      false, true, NULL);

    /* clean up */
    display_inv_callbacks_clean(callbacks);
}

static int building_player_check(player *p, guint amount)
{
    guint player_gold = player_get_gold(p);

    if (player_gold >= amount)
        return true;

    if (p->bank_account >= amount)
        return true;

    return false;
}

static void building_player_charge(player *p, guint amount)
{
    if (p->bank_account >= amount)
    {
        p->bank_account -= amount;
        log_add_entry(nlarn->log, _("We have debited %d gold from your bank "
                "account."), amount);
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
            it->cursed = false;

        if (it->rusty)
            it->rusty = false;

        if (it->burnt)
            it->burnt = false;

        if (it->corroded)
            it->corroded = false;

        if (it->bonus < 0)
            it->bonus = 0;

        /* identify item */
        it->bonus_known = true;
        it->blessed_known = true;

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
    gpointer ioid = NULL; /* oid of purchased item */
    char text[81];
    gchar *name;

    /* The item the player actually bought. Usually this is the passed
     * item, in the case of item stacks it might be a new item. */
    item *bought_itm = it;

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    guint price = item_price(it);

    if (it->count > 1)
    {
        g_snprintf(text, 80, _("How many %s do you want to buy?"),
                it->type == IT_AMMO ? ammo_name_pl(it) : item_name_pl(it->type));

        /* get count */
        guint count = display_get_count(text, it->count);

        if (count > it->count)
        {
            /* desired amount is larger than the available amount */
            log_add_entry(nlarn->log, _("Wouldn't it be nice if the store "
                          "had %d of those?"), count);
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
            name = item_describe(bought_itm, true, false, true);
            g_snprintf(text, 80, _("You cannot afford the %d gold for %s."),
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
        name = item_describe(it, true, true, true);
        g_snprintf(text, 80, _("Do you want to buy %s for %d gold?"),
                   name, price);
        g_free(name);

        if (!display_get_yesno(text, NULL, NULL, NULL))
            return;
    }

    /* prepare the item description for logging later */
    name = item_describe(bought_itm, true, false, false);

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
    log_add_entry(nlarn->log, _("You buy %s. Thank you for your purchase."), name);
    g_free(name);

    /* charge player for this purchase */
    building_player_charge(p, price);

    player_make_move(p, 2, false, NULL);
}

static void building_item_identify(player *p, inventory **inv __attribute__((unused)), item *it)
{
    char message[81];

    const char *title = _("Identify item");

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* The cost for identification is 10% of the item's base price. */
    guint price = (item_base_price(it) / 10) * (game_difficulty(nlarn) + 1);

    /* Ensure it costs at least 50gp... */
    price = max(50, price);

    gchar *name_unknown = item_describe(it, player_item_known(p, it), false, true);

    if (building_player_check(p, price))
    {
        g_snprintf(message, 80, _("Pay %d gold to identify %s?"),
                   price, name_unknown);

        if (display_get_yesno(message, NULL, NULL, NULL))
        {
            player_item_identify(p, NULL, it);

            gchar *name_nom = item_describe_gc(it, false, false, true, GC_NOM);
            name_nom[0] = g_ascii_toupper(name_nom[0]);
            gchar *name_known = item_describe_gc(it, player_item_known(p, it),
                                                 true, false, GC_NOM);

            log_add_entry(nlarn->log, _("%s is %s."), name_nom, name_known);
            g_free(name_nom);
            g_free(name_known);

            building_player_charge(p, price);
            p->stats.gold_spent_id_repair += price;
            player_make_move(p, 1, false, NULL);
        }
    }
    else
    {
        g_snprintf(message, 80, _("Identifying %s costs %d gold."),
                   name_unknown, price);
        display_show_message(title, message, 0);
    }

    g_free(name_unknown);
}

static void building_item_repair(player *p, inventory **inv __attribute__((unused)), item *it)
{
    int damages = 0;
    char message[81];

    const char *title = _("Repair item");

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* determine how much the item is damaged */
    damages += it->burnt;
    damages += it->corroded;
    damages += it->rusty;

    /* The cost of repairing an item is 10% of the item's base price. */
    guint price = (item_base_price(it) / 10) * (game_difficulty(nlarn) + 1);

    /* Take the level of damage into account. */
    price *= damages;

    /* Charge repair of item stacks depending on the quantity. */
    if (item_is_stackable(it->type)) price *= it->count;

    /* If an item is blessed and the player know it, use this and charge :) */
    if (it->blessed && it->blessed_known) price <<=1;

    /* Ensure this costs at least 50gp... */
    price = max(50, price);

    gchar *name = item_describe(it, player_item_known(p, it), false, true);

    if (building_player_check(p, price))
    {
        g_snprintf(message, 80, _("Pay %d gold to repair %s?"), price, name);

        if (display_get_yesno(message, NULL, NULL, NULL))
        {
            it->burnt = 0;
            it->corroded = 0;
            it->rusty = 0;

            gchar *name_nom = item_describe_gc(it, player_item_known(p, it),
                                               false, true, GC_NOM);
            name_nom[0] = g_ascii_toupper(name_nom[0]);
            log_add_entry(nlarn->log, _("%s has been repaired."), name_nom);
            g_free(name_nom);
            building_player_charge(p, price);

            p->stats.gold_spent_id_repair += price;
            player_make_move(p, 1, false, NULL);
        }
    }
    else
    {
        g_snprintf(message, 80, _("Repairing the %s costs %d gold."), name, price);
        display_show_message(title, message, 0);
    }

    g_free(name);
}

static void building_item_buy(player *p, inventory **inv, item *it)
{
    guint count = 0;
    char question[121];
    gchar *name;

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    int price = item_price(it);

    /* modify price if player sells stuff at the trading post */
    if (map_sobject_at(game_map(nlarn, Z(p->pos)), p->pos) == LS_TRADEPOST)
    {
        if (!player_item_is_damaged(p, NULL, it))
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
        g_snprintf(question, 120, _("How many %s do you want to sell for %d gold?"),
                it->type == IT_AMMO ? ammo_name_pl(it) : item_name_pl(it->type), price);

        /* get count */
        count = display_get_count(question, it->count);

        if (count > it->count)
        {
            log_add_entry(nlarn->log, _("Wouldn't it be nice to have %d of those?"), count);
            return;
        }

        if (count == 0)
            return;

        price *= count;
    }
    else
    {
        count = 1;
        name = item_describe(it, player_item_known(p, it), true, true);
        g_snprintf(question, 120, _("Do you want to sell %s for %d gold?"),
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

    name = item_describe(it, player_item_known(p, it), false, false);
    log_add_entry(nlarn->log, (price == 1)
                  ? _("You sell %s. The %d gold has been "
                      "transferred to your bank account.")
                  : _("You sell %s. The %d gold have been "
                      "transferred to your bank account."),
                  name, price);

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

    player_make_move(p, 1, false, NULL);
}

static guint calc_tax_debt(guint income)
{
    /* charge 5% income tax per difficulty level */
    return (income * min(game_difficulty(nlarn), 4) / 20);
}
