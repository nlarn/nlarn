/*
 * memorial.c
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

#include <glib.h>
#include <glib/gi18n.h>

#include "display.h"
#include "extdefs.h"
#include "memorial.h"

/* appends a section header ruled with dashes to a common line width, so
   translated titles of differing length still line up tidily */
static void memorial_append_title(GString *text, const char *title)
{
    const char dashes[] = "------------------------------------------";
    const int line_width = 39;
    int pad = line_width - 4 - (int)g_utf8_strlen(title, -1);

    if (pad < 3)
        pad = 3;
    if (pad > (int)sizeof(dashes) - 1)
        pad = sizeof(dashes) - 1;

    g_string_append_printf(text, "\n\n`TITLE`-- %s %.*s`end`\n\n",
                           title, pad, dashes);
}

char *memorial_create(player *p, score_t *score, GList *scores)
{
    const bool male = (p->sex == PS_MALE);
    const bool died = (score->cod < PD_TOO_LATE);
    gchar *tmp = score_death_description(score, true);

    /* the obituary */
    GString *text = g_string_new(tmp);
    g_free(tmp);

    /* assemble surrounding scores list */
    g_string_append(text, "\n\n");
    tmp = scores_to_string(scores, score);
    g_string_append(text, tmp);
    g_free(tmp);

    /* some statistics */
    guint mobuls = gtime2mobuls(nlarn->gtime);
    gchar *mobul_str = g_strdup_printf(
            ngettext("%d mobul", "%d mobuls", mobuls), mobuls);

    if (died)
    {
        g_string_append_printf(text, male
                ? _("\nHe died after searching for the potion for %s. ")
                : _("\nShe died after searching for the potion for %s. "),
                mobul_str);
    }
    else
    {
        g_string_append_printf(text, male
                ? _("\nHe returned after searching for the potion for %s. ")
                : _("\nShe returned after searching for the potion for %s. "),
                mobul_str);
    }
    g_free(mobul_str);

    gchar *spells_str  = g_strdup_printf(ngettext("%d spell", "%d spells",
            p->stats.spells_cast), p->stats.spells_cast);
    gchar *potions_str = g_strdup_printf(ngettext("%d potion", "%d potions",
            p->stats.potions_quaffed), p->stats.potions_quaffed);
    gchar *books_str   = g_strdup_printf(ngettext("%d book", "%d books",
            p->stats.books_read), p->stats.books_read);
    gchar *scrolls_str = g_strdup_printf(ngettext("%d scroll", "%d scrolls",
            p->stats.scrolls_read), p->stats.scrolls_read);

    g_string_append_printf(text, male
            ? _("He cast %s, quaffed %s, and read %s and %s. ")
            : _("She cast %s, quaffed %s, and read %s and %s. "),
            spells_str, potions_str, books_str, scrolls_str);

    g_free(spells_str);
    g_free(potions_str);
    g_free(books_str);
    g_free(scrolls_str);

    if (p->stats.weapons_wasted > 0)
    {
        gchar *weapons_str = g_strdup_printf(ngettext("%d weapon", "%d weapons",
                p->stats.weapons_wasted), p->stats.weapons_wasted);

        g_string_append_printf(text, male
                ? _("\nHe wasted %s in combat. ")
                : _("\nShe wasted %s in combat. "),
                weapons_str);

        g_free(weapons_str);
    }

    if (p->stats.vandalism > 0)
    {
        gchar *vandalism_str = g_strdup_printf(ngettext("%d act of vandalism",
                "%d acts of vandalism", p->stats.vandalism), p->stats.vandalism);

        g_string_append_printf(text, male
                ? _("\nHe committed %s. ")
                : _("\nShe committed %s. "),
                vandalism_str);

        g_free(vandalism_str);
    }

    if (p->stats.life_protected > 0)
    {
        g_string_append_printf(text, male
                ? _("\nHis life was protected %s. ")
                : _("\nHer life was protected %s. "),
                int2time_str(p->stats.life_protected));
    }

    if (died)
    {
        g_string_append_printf(text, male
                ? _("\nHe had %s gold on his bank account when he died.")
                : _("\nShe had %s gold on her bank account when she died."),
                int2str(p->bank_account));
    }
    else
    {
        g_string_append_printf(text, male
                ? _("\nHe had %s gold on his bank account when he returned home.")
                : _("\nShe had %s gold on her bank account when she returned home."),
                int2str(p->bank_account));
    }

    gchar *gems_str = g_strdup_printf(ngettext("%d gem", "%d gems",
            p->stats.gems_sold), p->stats.gems_sold);
    gchar *sold_items_str = g_strdup_printf(ngettext("%d other item",
            "%d other items", p->stats.items_sold), p->stats.items_sold);

    g_string_append_printf(text, male
            ? _("\nHe found %d gold in the caverns, sold %s for %d and %s "
                "for %d gold, and earned %d gold as bank interest.")
            : _("\nShe found %d gold in the caverns, sold %s for %d and %s "
                "for %d gold, and earned %d gold as bank interest."),
            p->stats.gold_found, gems_str, p->stats.gold_sold_gems,
            sold_items_str, p->stats.gold_sold_items,
            p->stats.gold_bank_interest);

    g_free(gems_str);
    g_free(sold_items_str);

    gchar *bought_str = g_strdup_printf(ngettext("%d item", "%d items",
            p->stats.items_bought), p->stats.items_bought);

    g_string_append_printf(text, male
            ? _("\nHe bought %s for %d gold, spent %d on item identification "
                "or repair, donated %d gold to charitable causes, and "
                "invested %d gold in his personal education.")
            : _("\nShe bought %s for %d gold, spent %d on item identification "
                "or repair, donated %d gold to charitable causes, and "
                "invested %d gold in her personal education."),
            bought_str, p->stats.gold_spent_shop, p->stats.gold_spent_id_repair,
            p->stats.gold_spent_donation, p->stats.gold_spent_college);

    g_free(bought_str);

    if (p->outstanding_taxes && p->stats.gold_spent_taxes)
    {
        g_string_append_printf(text, male
                ? _(" He owed the tax office %d gold and paid %d gold taxes.")
                : _(" She owed the tax office %d gold and paid %d gold taxes."),
                p->outstanding_taxes, p->stats.gold_spent_taxes);
    }
    else if (p->outstanding_taxes)
    {
        g_string_append_printf(text, male
                ? _(" He owed the tax office %d gold.")
                : _(" She owed the tax office %d gold."),
                p->outstanding_taxes);
    }
    else if (p->stats.gold_spent_taxes)
    {
        g_string_append_printf(text, male
                ? _(" He paid %d gold taxes.")
                : _(" She paid %d gold taxes."),
                p->stats.gold_spent_taxes);
    }

    /* append map of current level if the player is not in the town */
    if (Z(p->pos) > 0)
    {
        memorial_append_title(text, _("The current level"));
        tmp = map_dump(game_map(nlarn, Z(p->pos)), p->pos);
        g_string_append(text, tmp);
        g_free(tmp);
    }

    /* player's attributes */
    memorial_append_title(text, _("Attributes"));

    struct
    {
        const char *label;
        int value;
        int delta;
    } attribs[] =
    {
        { _("Strength:"),     p->strength,     p->strength - p->stats.str_orig },
        { _("Dexterity:"),    p->dexterity,    p->dexterity - p->stats.dex_orig },
        { _("Constitution:"), p->constitution, p->constitution - p->stats.con_orig },
        { _("Intelligence:"), p->intelligence, p->intelligence - p->stats.int_orig },
        { _("Wisdom:"),       p->wisdom,       p->wisdom - p->stats.wis_orig },
    };

    for (guint idx = 0; idx < G_N_ELEMENTS(attribs); idx++)
    {
        g_string_append_printf(text, "%-*s %d (%+2d)\n",
                               utf8_pad(attribs[idx].label, 18),
                               attribs[idx].label,
                               attribs[idx].value, attribs[idx].delta);
    }

    /* effects */
    char **effect_desc = player_effect_text(p);

    if (*effect_desc)
    {
        memorial_append_title(text, _("Effects"));

        for (guint pos = 0; effect_desc[pos]; pos++)
        {
            g_string_append_printf(text, "%s\n", effect_desc[pos]);
        }
    }

    g_strfreev(effect_desc);

    /* append list of known spells */
    if (p->known_spells->len > 0)
    {
        memorial_append_title(text, _("Known Spells"));

        for (guint pos = 0; pos < p->known_spells->len; pos++)
        {
            spell *s = (spell *)g_ptr_array_index(p->known_spells, pos);
            tmp = str_capitalize(g_strdup(spell_name(s)));

            g_string_append_printf(text, _("%-*s (lvl. %2d): %3d\n"),
                                   utf8_pad(tmp, 24), tmp,
                                   s->knowledge, s->used);

            g_free(tmp);
        }
    }

    /* identify entire inventory */
    for (guint pos = 0; pos < inv_length(p->inventory); pos++)
    {
        item *it = inv_get(p->inventory, pos);
        it->blessed_known = true;
        it->bonus_known = true;
    }

    /* equipped items */
    gchar *el = player_equipment_list(p);
    guint equipment_count = 0;

    if (strlen(el) > 0)
    {
        memorial_append_title(text, _("Equipment"));
        g_string_append(text, el);

        for (guint idx = 0; idx < inv_length(p->inventory); idx++)
        {
            item *it = inv_get(p->inventory, idx);
            if (player_item_is_equipped(p, it))
                equipment_count++;
        }
    }
    g_free(el);

    /* inventory */
    if (equipment_count < inv_length(p->inventory))
    {
        memorial_append_title(text, _("Items in pack"));
        for (guint pos = 0; pos < inv_length(p->inventory); pos++)
        {
            item *it = inv_get(p->inventory, pos);
            if (!player_item_is_equipped(p, it))
            {
                gchar *it_desc = item_describe(it, true, false, false);
                g_string_append_printf(text, "%s\n", it_desc);
                g_free(it_desc);

                /* items inside containers */
                if (it->type == IT_CONTAINER && inv_length(it->content)) {
                    for (guint idx = 0; idx < inv_length(it->content); idx++) {
                        item *thing = inv_get(it->content, idx);
                        gchar *desc = item_describe(thing, true, false, false);
                        g_string_append_printf(text, "    %s\n", desc);
                        g_free(desc);
                    }
                }
            }
        }
    }

    /* list monsters killed */
    guint body_count = 0;
    memorial_append_title(text, _("Creatures vanquished"));

    for (guint mnum = 0; mnum < MT_MAX; mnum++)
    {
        if (p->stats.monsters_killed[mnum] > 0)
        {
            guint mcount = p->stats.monsters_killed[mnum];
            tmp = str_capitalize(g_strdup(monster_type_plural_name(mnum,
                                          mcount)));

            g_string_append_printf(text, "%3d %s\n", mcount, tmp);

            g_free(tmp);
            body_count += mcount;
        }
    }
    g_string_append_printf(text, _("\n%3d total\n"), body_count);

    /* genocided monsters */
    bool printed_headline = false;
    for (guint mnum = 0; mnum < MT_MAX; mnum++)
    {
        if (!monster_is_genocided(mnum))
            continue;

        if (!printed_headline)
        {
            memorial_append_title(text, _("Genocided creatures"));
            printed_headline = true;
        }

        tmp = str_capitalize(g_strdup(monster_type_plural_name(mnum, 2)));
        g_string_append_printf(text, "%s\n", tmp);
    }

     /* messages */
    memorial_append_title(text, _("Last messages"));
    for (guint pos = log_length(nlarn->log) - min(10, log_length(nlarn->log));
         pos < log_length(nlarn->log); pos++)
    {
        message_log_entry *entry = log_get_entry(nlarn->log, pos);
        g_string_append_printf(text, "%s\n", entry->message);
    }
    /* print uncommitted messages */
    if (nlarn->log->buffer->len > 0)
    {
        g_string_append_printf(text, "%s\n", nlarn->log->buffer->str);
    }

    return (g_string_free(text, false));
}

void memorial_save(player *p, const char *text)
{
    char *proposal = NULL;
    bool done = false;

    while (!done)
    {
        GError *error = NULL;

        if (proposal == NULL)
        {
            /* When starting, propose a file name. Use the previously
               entered file name when trying to find a new name for an
               existing file. */
            proposal = g_strconcat(p->name, ".txt", NULL);
        }

        char *filename = display_get_string(NULL, _("Enter filename: "),
            proposal, 40);
        g_free(proposal);
        proposal = NULL;

        if (filename == NULL)
        {
            /* user pressed ESC, thus display_get_string() returned NULL */
            done = true;
        } else {
            /* file name has been provided, try to save file */
            char *fullname = g_build_path(G_DIR_SEPARATOR_S,
#ifdef G_OS_WIN32
                    g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS),
#else
                    g_get_home_dir(),
#endif
                    filename, NULL);

            if (g_file_test(fullname, G_FILE_TEST_IS_SYMLINK))
            {
                display_show_message(_("Error"), _("File is a symlink. I won't "
                        "overwrite those..."), 0);
                g_free(fullname);
                continue;
            }

            if (g_file_test(fullname, G_FILE_TEST_IS_DIR))
            {
                display_show_message(_("Error"), _("There is a directory with the "
                        "name you gave. Thus I can't write the file."), 0);
                g_free(fullname);
                continue;
            }

            if (g_file_test(fullname, G_FILE_TEST_IS_REGULAR)
                    && !display_get_yesno(_("File exists!\n"
                        "Do you want to overwrite it?"), NULL, NULL, NULL))
            {
                g_free(fullname);
                proposal = filename;
                continue;
            }

            /* wrap the text and insert line feed for the platform */
            char *wtext = str_prepare_for_saving(text);

            if (g_file_set_contents(fullname, wtext, -1, &error))
            {
                /* successfully saved the memorial file */
                done = true;
            }
            else
            {
                display_show_message(_("Error"), error->message, 0);
                g_error_free(error);
            }

            g_free(wtext);

            g_free(filename);
            g_free(fullname);
        }
    }
}
