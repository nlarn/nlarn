/*
 * grammar.c
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

#include <stdbool.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "grammar.h"

/*
 * The article tables are part of the message catalog, so every language
 * defines its own articles, noun classes and declension rules; the rules
 * for the session language are loaded at runtime through gettext.
 *
 * A translated article table consists of one line per noun class:
 *
 *     <class>:<form>[,<form>...]
 *
 * where <class> is the class marker used in the nouns' grammar metadata
 * (see noun_phrase()) and the comma-separated forms are the articles for
 * the grammatical cases, in the order nominative, accusative, dative,
 * genitive; declined plural articles may follow after a pipe symbol
 * ("m:der,den,dem,des|die,die,den,der"). Missing or empty forms fall
 * back to the nearest preceding form. When looking up a
 * class, progressively shorter prefixes of the requested marker are
 * tried, so specialised classes (e.g. "mv" for French masculine nouns
 * requiring elision) fall back to their base class ("m") in tables that
 * do not define them. An article ending in an apostrophe is joined with
 * the noun without a space (French "l'orc").
 *
 * When the table is left untranslated, English rules are applied: the
 * definite article is "the" for all nouns, and the indefinite article
 * is chosen by the noun's initial letter ("a/an").
 */

/* a parsed article table row: class marker and per-case article forms */
typedef struct article_class
{
    char *marker;
    char *forms[GC_MAX];
    char *plural_forms[GC_MAX];
} article_class;

/* a parsed article table */
typedef struct article_table
{
    gboolean loaded;
    GArray *classes;   /* article_class per translated noun class */
} article_table;

static article_table tables[3] =
{
    { false, NULL }, { false, NULL }, { false, NULL }
};

static const char *article_table_spec(article_t article)
{
    if (article == ART_DEF)
    {
        /* TRANSLATORS: The definite article. May be translated to a table
           of noun classes with declined article forms; see grammar.c for
           the format. German example:
           "m:der,den,dem,des\nf:die,die,der,der\nn:das,das,dem,des" */
        return C_("grammar", "the");
    }

    if (article == ART_POSS)
    {
        /* TRANSLATORS: The second person singular possessive pronoun.
           May be translated to a table of noun classes with declined
           forms; see grammar.c for the format. German example:
           "m:dein,deinen,deinem,deines\nf:deine,deine,deiner,deiner\n..." */
        return C_("grammar", "your");
    }

    /* TRANSLATORS: The indefinite article, chosen by the noun's initial
       letter when left untranslated. May be translated to a table of
       noun classes with declined article forms; see grammar.c for the
       format. German example:
       "m:ein,einen,einem,eines\nf:eine,eine,einer,einer\nn:ein,ein,einem,eines" */
    return C_("grammar", "a/an");
}

/* parse the translated article table specification */
static article_table *get_article_table(article_t article)
{
    article_table *table;

    switch (article)
    {
        case ART_DEF:  table = &tables[0]; break;
        case ART_POSS: table = &tables[2]; break;
        default:       table = &tables[1]; break;
    }

    if (table->loaded)
        return table;

    table->classes = g_array_new(false, true, sizeof(article_class));

    const char *spec = article_table_spec(article);

    /* an untranslated table contains no class rows */
    if (strchr(spec, ':') != NULL)
    {
        char **rows = g_strsplit(spec, "\n", -1);

        for (guint idx = 0; rows[idx] != NULL; idx++)
        {
            char *sep = strchr(rows[idx], ':');

            /* skip malformed rows */
            if (sep == NULL || sep == rows[idx])
                continue;

            article_class cls = { NULL, { NULL }, { NULL } };
            cls.marker = g_strndup(rows[idx], sep - rows[idx]);

            /* the plural forms follow after a pipe symbol */
            char **numeri = g_strsplit(sep + 1, "|", 2);

            for (guint num = 0; numeri[num] != NULL; num++)
            {
                char **forms = g_strsplit(numeri[num], ",", GC_MAX);
                guint count = g_strv_length(forms);
                char **dest = (num == 0) ? cls.forms : cls.plural_forms;

                /* an empty specification means "no article" */
                if (count == 0)
                {
                    g_strfreev(forms);
                    continue;
                }

                for (guint gc = 0; gc < GC_MAX; gc++)
                {
                    /* fall back to the nearest preceding form for
                       missing or empty case forms */
                    guint use = MIN(gc, count - 1);
                    while (use > 0 && forms[use][0] == '\0')
                        use--;

                    dest[gc] = g_strdup(forms[use]);
                }

                g_strfreev(forms);
            }

            g_strfreev(numeri);
            g_array_append_val(table->classes, cls);
        }

        g_strfreev(rows);
    }

    table->loaded = true;

    return table;
}

/* find the article for a noun class, trying progressively shorter
   prefixes of the requested class marker */
static const char *lookup_article(article_t article, const char *marker,
                                  grammar_case gcase, gboolean plural)
{
    article_table *table = get_article_table(article);

    for (size_t len = strlen(marker); len > 0; len--)
    {
        for (guint idx = 0; idx < table->classes->len; idx++)
        {
            article_class *cls = &g_array_index(table->classes,
                                                article_class, idx);

            if (strlen(cls->marker) == len
                    && strncmp(cls->marker, marker, len) == 0)
            {
                const char *form = plural
                    ? cls->plural_forms[gcase] : cls->forms[gcase];

                /* a matching class without forms for the requested
                   number means "no article" (e.g. German indefinite
                   plural), not "unknown class" */
                return form ? form : "";
            }
        }
    }

    return NULL;
}

/*
 * Adjective endings, like the articles, are part of the message catalog.
 * The table consists of one line per article kind ("def", "indef",
 * "none"), each holding semicolon-separated noun class specifications:
 *
 *     <kind>:<class>=<ending>[,<ending>...][;<class>=...]
 *
 * with the comma-separated endings ordered by grammatical case like the
 * article forms. A tilde in a noun's case form is replaced with the
 * ending matching the requested article kind, noun class and case;
 * when the table defines no matching ending, the tilde is dropped.
 */

typedef struct ending_class
{
    char *marker;
    char *forms[GC_MAX];
    char *plural_forms[GC_MAX];
} ending_class;

/* per article kind: ART_NONE, ART_DEF, ART_INDEF */
static GArray *ending_tables[3] = { NULL };
static gboolean ending_tables_loaded = false;

static int ending_table_index(article_t article)
{
    switch (article)
    {
        case ART_DEF:   return 1;
        /* possessive pronouns decline like the indefinite article */
        case ART_INDEF:
        case ART_POSS:  return 2;
        default:        return 0;
    }
}

static int ending_table_index_by_name(const char *kind)
{
    if (strcmp(kind, "def") == 0)   return 1;
    if (strcmp(kind, "indef") == 0) return 2;
    if (strcmp(kind, "none") == 0)  return 0;
    return -1;
}

static void load_ending_tables(void)
{
    if (ending_tables_loaded)
        return;

    for (guint idx = 0; idx < 3; idx++)
        ending_tables[idx] = g_array_new(false, true, sizeof(ending_class));

    /* TRANSLATORS: The adjective ending table; see grammar.c for the
       format. Languages without declining adjectives leave this
       untranslated. German example:
       "def:m=e,en,en,en;f=e,e,en,en;n=e,e,en,en\n"
       "indef:m=er,en,en,en;f=e,e,en,en;n=es,es,en,en\n"
       "none:m=er,en,em,en;f=e,e,er,er;n=es,es,em,en" */
    const char *spec = C_("grammar", "adjective endings");

    if (strchr(spec, '=') != NULL)
    {
        char **rows = g_strsplit(spec, "\n", -1);

        for (guint idx = 0; rows[idx] != NULL; idx++)
        {
            char *sep = strchr(rows[idx], ':');
            if (sep == NULL || sep == rows[idx])
                continue;

            g_autofree char *kind = g_strndup(rows[idx], sep - rows[idx]);
            int tidx = ending_table_index_by_name(kind);
            if (tidx < 0)
                continue;

            char **specs = g_strsplit(sep + 1, ";", -1);

            for (guint cnum = 0; specs[cnum] != NULL; cnum++)
            {
                char *eq = strchr(specs[cnum], '=');
                if (eq == NULL || eq == specs[cnum])
                    continue;

                ending_class cls = { NULL, { NULL }, { NULL } };
                cls.marker = g_strndup(specs[cnum], eq - specs[cnum]);

                /* plural endings may follow after a pipe symbol */
                char **numeri = g_strsplit(eq + 1, "|", 2);

                for (guint num = 0; numeri[num] != NULL; num++)
                {
                    char **forms = g_strsplit(numeri[num], ",", GC_MAX);
                    guint count = g_strv_length(forms);
                    char **dest = (num == 0) ? cls.forms : cls.plural_forms;

                    if (count == 0)
                    {
                        g_strfreev(forms);
                        continue;
                    }

                    for (guint gc = 0; gc < GC_MAX; gc++)
                    {
                        /* fall back to the nearest preceding form for
                           missing or empty case forms */
                        guint use = MIN(gc, count - 1);
                        while (use > 0 && forms[use][0] == '\0')
                            use--;

                        dest[gc] = g_strdup(forms[use]);
                    }

                    g_strfreev(forms);
                }

                g_strfreev(numeri);
                g_array_append_val(ending_tables[tidx], cls);
            }

            g_strfreev(specs);
        }

        g_strfreev(rows);
    }

    ending_tables_loaded = true;
}

/* find the adjective ending for an article kind and noun class, trying
   progressively shorter prefixes of the requested class marker */
static const char *adjective_ending(article_t article, const char *marker,
                                    grammar_case gcase, gboolean plural)
{
    load_ending_tables();

    GArray *table = ending_tables[ending_table_index(article)];

    for (size_t len = strlen(marker); len > 0; len--)
    {
        for (guint idx = 0; idx < table->len; idx++)
        {
            ending_class *cls = &g_array_index(table, ending_class, idx);

            if (strlen(cls->marker) == len
                    && strncmp(cls->marker, marker, len) == 0)
            {
                const char *ending = plural
                    ? cls->plural_forms[gcase] : cls->forms[gcase];

                return ending ? ending : "";
            }
        }
    }

    /* no ending defined - the tilde is dropped */
    return "";
}

/* append a noun form, replacing adjective ending placeholders */
static void append_declined(GString *phrase, const char *form,
                            const char *ending)
{
    for (const char *pos = form; *pos != '\0'; pos++)
    {
        if (*pos == '~')
            g_string_append(phrase, ending);
        else
            g_string_append_c(phrase, *pos);
    }
}

/*
 * Ring of buffers for assembled phrases. This allows using the returned
 * phrases as arguments to printf-like functions - even multiple times in
 * a single call - without having to free them afterwards.
 */
#define PHRASE_BUFFERS 8
static GString *phrase_buffer(void)
{
    static GString *buffers[PHRASE_BUFFERS] = { NULL };
    static guint current = 0;

    current = (current + 1) % PHRASE_BUFFERS;

    if (buffers[current] == NULL)
        buffers[current] = g_string_new(NULL);
    else
        g_string_truncate(buffers[current], 0);

    return buffers[current];
}

/* capitalise the first letter of an UTF-8 string */
static void capitalise_phrase(GString *phrase)
{
    if (phrase->len == 0)
        return;

    gunichar first = g_utf8_get_char(phrase->str);
    gunichar upper = g_unichar_toupper(first);

    if (first == upper)
        return;

    char ustr[7] = { 0 };
    gint ulen = g_unichar_to_utf8(upper, ustr);
    g_string_erase(phrase, 0, g_utf8_next_char(phrase->str) - phrase->str);
    g_string_insert_len(phrase, 0, ustr, ulen);
}

/* extract the noun class marker: one to three lowercase letters,
   terminated by a colon; returns NULL if the noun carries none */
static char *noun_class(const char *noun)
{
    size_t len = 0;

    while (noun[len] >= 'a' && noun[len] <= 'z')
        len++;

    if (len == 0 || len > 3 || noun[len] != ':')
        return NULL;

    return g_strndup(noun, len);
}

/* append the noun with English article rules */
static void append_english(GString *phrase, article_t article,
                           const char *noun, gboolean plural)
{
    switch (article)
    {
    case ART_DEF:
        g_string_append(phrase, "the ");
        break;

    case ART_INDEF:
        /* plural nouns take no indefinite article */
        if (!plural)
            g_string_append(phrase,
                    strchr("aeiouAEIOU", noun[0]) ? "an " : "a ");
        break;

    case ART_POSS:
        g_string_append(phrase, "your ");
        break;

    default:
        break;
    }

    g_string_append(phrase, noun);
}

const char *noun_plural(const char *noun)
{
    if (noun_has_class(noun))
        return noun_phrase(noun, ART_NONE, GC_NOM, true, false);

    /* English fallback: append an "s" */
    GString *phrase = phrase_buffer();
    g_string_append(phrase, noun);
    g_string_append_c(phrase, 's');

    return phrase->str;
}

gboolean noun_has_class(const char *noun)
{
    g_autofree char *marker = noun_class(noun);
    return (marker != NULL);
}

const char *noun_phrase(const char *noun, article_t article,
                        grammar_case gcase, gboolean plural,
                        gboolean capitalise)
{
    g_assert(noun != NULL && gcase < GC_MAX);

    GString *phrase = phrase_buffer();

    g_autofree char *marker = noun_class(noun);

    if (marker == NULL)
    {
        /* no grammar metadata - use English rules and ignore the case */
        append_english(phrase, article, noun, plural);
    }
    else
    {
        /* split into singular and plural forms; skip marker and colon */
        char **numeri = g_strsplit(noun + strlen(marker) + 1, "|", 2);

        /* fall back to the singular forms if no plural forms are given */
        const char *fspec = (plural && numeri[1] != NULL)
            ? numeri[1] : numeri[0];

        /* split the case forms */
        char **forms = g_strsplit(fspec, ";", GC_MAX);
        guint count = g_strv_length(forms);

        /* fall back to the nearest preceding form for
           missing or empty oblique case forms */
        guint use = MIN((guint)gcase, count - 1);
        while (use > 0 && forms[use][0] == '\0')
            use--;

        /* resolve adjective ending placeholders in the selected form */
        GString *resolved = g_string_new(NULL);
        append_declined(resolved, forms[use],
                        adjective_ending(article, marker, gcase, plural));
        const char *form = resolved->str;

        const char *art = (article == ART_NONE)
            ? NULL : lookup_article(article, marker, gcase, plural);

        if (article != ART_NONE && art == NULL)
        {
            /* the language does not define articles for this class -
               apply English rules to the translated noun */
            append_english(phrase, article, form, plural);
        }
        else
        {
            if (art != NULL && art[0] != '\0')
            {
                g_string_append(phrase, art);

                /* articles ending in an apostrophe are joined with
                   the noun without a space (French "l'orc") */
                if (!g_str_has_suffix(art, "'"))
                    g_string_append_c(phrase, ' ');
            }

            g_string_append(phrase, form);
        }

        g_string_free(resolved, true);
        g_strfreev(forms);
        g_strfreev(numeri);
    }

    if (capitalise)
        capitalise_phrase(phrase);

    return phrase->str;
}
