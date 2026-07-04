/*
 * grammar.h
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

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <glib.h>

/* The grammatical case of a noun phrase. English does not distinguish
   cases, but translations may render different words for each. */
typedef enum grammar_case
{
    GC_NOM, /* nominative - the noun is the subject of the sentence */
    GC_ACC, /* accusative - the noun is the direct object */
    GC_DAT, /* dative     - the noun is the indirect object */
    GC_GEN, /* genitive   - the noun is a possessor */
    GC_MAX
} grammar_case;

/* The article used to render a noun phrase. */
typedef enum article_t
{
    ART_NONE,  /* no article: "orc" */
    ART_DEF,   /* definite article: "the orc" */
    ART_INDEF, /* indefinite article: "an orc" */
} article_t;

/**
 * @brief Build a noun phrase from a translated noun.
 *
 * Translated nouns may carry grammar metadata: an optional leading
 * noun class marker (usually the noun's gender), and optional
 * additional forms for the oblique cases, separated by semicolons:
 *
 *     [<class>:]<nominative>[;<accusative>[;<dative>[;<genitive>]]]
 *
 * The class marker consists of one to three lowercase letters; its
 * meaning is defined by the language's article tables, which are
 * themselves part of the message catalog (see grammar.c). Missing or
 * empty case forms fall back to the nearest preceding form.
 *
 * Examples (German): "m:Ork" (der/den/dem Ork), "m:Drache;Drachen"
 * (der Drache, den/dem Drachen), "n:schwebende Auge;schwebende
 * Auge;schwebenden Auge" (das schwebende Auge, dem schwebenden Auge).
 *
 * A tilde within a form marks the ending of a declinable adjective; it
 * is replaced with the appropriate ending from the language's
 * adjective ending table (see grammar.c), which depends on the
 * article, the noun class and the case: "m:türkis~ Trank" renders as
 * "ein türkiser Trank", "der türkise Trank" and "dem türkisen Trank".
 *
 * Nouns without grammar metadata (in particular untranslated English
 * nouns) are rendered with English rules: the requested case is
 * ignored, the definite article is "the" and the indefinite article
 * is chosen by the initial letter of the noun.
 *
 * The returned string is owned by a small ring of internal buffers;
 * it remains valid until after a few subsequent calls and thus can be
 * used multiple times in the argument list of a printf-like function
 * without the need to free it.
 *
 * @param noun The translated noun, possibly carrying grammar metadata.
 * @param article The article to prepend.
 * @param gcase The grammatical case to render.
 * @param capitalise Capitalise the first letter (for sentence starts).
 * @return The assembled noun phrase.
 */
const char *noun_phrase(const char *noun, article_t article,
                        grammar_case gcase, gboolean capitalise);

#endif
