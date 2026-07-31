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

char *memorial_create(player *p, score_t *score, GList *scores)
{
    const char *pronoun = (p->sex == PS_MALE) ? "He" : "She";
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
    g_string_append_printf(text, "\n%s %s after searching for the potion for %d mobul%s. ",
                           pronoun, score->cod < PD_TOO_LATE ? "died" : "returned",
                           gtime2mobuls(nlarn->gtime), plural(gtime2mobuls(nlarn->gtime)));

    g_string_append_printf(text, "%s cast %s spell%s, ", pronoun,
                           int2str(p->stats.spells_cast),
                           plural(p->stats.spells_cast));
    g_string_append_printf(text, "quaffed %s potion%s, ",
                           int2str(p->stats.potions_quaffed),
                           plural(p->stats.potions_quaffed));
    g_string_append_printf(text, "and read %s book%s ",
                           int2str(p->stats.books_read),
                           plural(p->stats.books_read));
    g_string_append_printf(text, "and %s scroll%s. ",
                           int2str(p->stats.scrolls_read),
                           plural(p->stats.scrolls_read));

    if (p->stats.weapons_wasted > 0)
    {
        g_string_append_printf(text, "\n%s wasted %s weapon%s in combat. ",
                               pronoun, int2str(p->stats.weapons_wasted),
                               plural(p->stats.weapons_wasted));
    }

    if (p->stats.vandalism > 0)
    {
        g_string_append_printf(text, "\n%s committed %s act%s of vandalism. ",
                               pronoun, int2str(p->stats.vandalism),
                               plural(p->stats.vandalism));
    }

    if (p->stats.life_protected > 0)
    {
        g_string_append_printf(text, "\n%s life was protected %s. ",
                               (p->sex == PS_MALE) ? "His" : "Her",
                               int2time_str(p->stats.life_protected));
    }

    g_string_append_printf(text, "\n%s had %s gold on %s bank account "
                           "when %s %s.",
                           pronoun, int2str(p->bank_account),
                           (p->sex == PS_MALE) ? "his" : "her",
                           (p->sex == PS_MALE) ? "he"  : "she",
                           score->cod < PD_TOO_LATE ? "died"
                           : "returned home");

    g_string_append_printf(text, "\n%s found %d gold in the caverns, "
                           "sold %s gem%s for %d and %s other "
                           "item%s for %d gold, and earned %d gold "
                           "as bank interest.",
                           pronoun, p->stats.gold_found,
                           int2str(p->stats.gems_sold),
                           plural(p->stats.gems_sold),
                           p->stats.gold_sold_gems,
                           int2str(p->stats.items_sold),
                           plural(p->stats.items_sold),
                           p->stats.gold_sold_items,
                           p->stats.gold_bank_interest);

    g_string_append_printf(text, "\n%s bought %s item%s for %d gold, spent "
                           "%d on item identification or repair, "
                           "donated %d gold to charitable causes, and "
                           "invested %d gold in %s personal education.",
                           pronoun,
                           int2str(p->stats.items_bought),
                           plural(p->stats.items_bought),
                           p->stats.gold_spent_shop,
                           p->stats.gold_spent_id_repair,
                           p->stats.gold_spent_donation,
                           p->stats.gold_spent_college,
                           (p->sex == PS_MALE) ? "his" : "her");

    if (p->outstanding_taxes)
        g_string_append_printf(text, " %s owed the tax office %d gold%s",
                               pronoun, p->outstanding_taxes,
                               p->stats.gold_spent_taxes ? "" : ".");

    if (p->stats.gold_spent_taxes)
        g_string_append_printf(text, " %s paid %d gold taxes.",
                               p->outstanding_taxes ? "and" : pronoun,
                               p->stats.gold_spent_taxes);

    /* append map of current level if the player is not in the town */
    if (Z(p->pos) > 0)
    {
        g_string_append(text, "\n\n`TITLE`-- The current level ------------------`end`\n\n");
        tmp = map_dump(game_map(nlarn, Z(p->pos)), p->pos);
        g_string_append(text, tmp);
        g_free(tmp);
    }

    /* player's attributes */
    g_string_append(text, "\n\n`TITLE`-- Attributes -------------------------`end`\n\n");
    g_string_append_printf(text, "Strength:     %d (%+2d)\n",
                           p->strength, p->strength - p->stats.str_orig);
    g_string_append_printf(text, "Dexterity:    %d (%+2d)\n",
                           p->dexterity, p->dexterity - p->stats.dex_orig);
    g_string_append_printf(text, "Constitution: %d (%+2d)\n",
                           p->constitution, p->constitution - p->stats.con_orig);
    g_string_append_printf(text, "Intelligence: %d (%+2d)\n",
                           p->intelligence, p->intelligence - p->stats.int_orig);
    g_string_append_printf(text, "Wisdom:       %d (%+2d)\n",
                           p->wisdom, p->wisdom - p->stats.wis_orig);

    /* effects */
    char **effect_desc = player_effect_text(p);

    if (*effect_desc)
    {
        g_string_append(text, "\n\n`TITLE`-- Effects ----------------------------`end`\n\n");

        for (guint pos = 0; effect_desc[pos]; pos++)
        {
            g_string_append_printf(text, "%s\n", effect_desc[pos]);
        }
    }

    g_strfreev(effect_desc);

    /* append list of known spells */
    if (p->known_spells->len > 0)
    {
        g_string_append(text, "\n\n`TITLE`-- Known Spells -----------------------`end`\n\n");

        for (guint pos = 0; pos < p->known_spells->len; pos++)
        {
            spell *s = (spell *)g_ptr_array_index(p->known_spells, pos);
            tmp = str_capitalize(g_strdup(spell_name(s)));

            g_string_append_printf(text, "%-*s (lvl. %2d): %3d\n",
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
        g_string_append(text, "\n\n`TITLE`-- Equipment --------------------------`end`\n\n");
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
        g_string_append(text, "\n\n`TITLE`-- Items in pack ----------------------`end`\n\n");
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
    g_string_append(text, "\n\n`TITLE`-- Creatures vanquished ---------------`end`\n\n");

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
    g_string_append_printf(text, "\n%3d total\n", body_count);

    /* genocided monsters */
    bool printed_headline = false;
    for (guint mnum = 0; mnum < MT_MAX; mnum++)
    {
        if (!monster_is_genocided(mnum))
            continue;

        if (!printed_headline)
        {
                g_string_append(text, "\n\n`TITLE`-- Genocided creatures ---------------`end`\n\n");
                printed_headline = true;
        }

        tmp = str_capitalize(g_strdup(monster_type_plural_name(mnum, 2)));
        g_string_append_printf(text, "%s\n", tmp);
    }

     /* messages */
    g_string_append(text, "\n\n`TITLE`-- Last messages ----------------------`end`\n\n");
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
