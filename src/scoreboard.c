/*
 * scoreboard.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#if (defined __unix) || (defined __unix__) || (defined __APPLE__)
# include <unistd.h>
# include <sys/file.h>
#endif

#include <glib/gi18n.h>

#include "extdefs.h"
#include "scoreboard.h"
#include "cJSON.h"

#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
/* file descriptor for the scoreboard file when running setgid */
int scoreboard_fd = -1;

void scoreboard_close_fd()
{
    close(scoreboard_fd);
}

static gzFile scoreboard_open_cloned_fd(const char* mode)
{
    /*
     * We'll need the file desciptor for loading saving the scores in this
     * and later games in the same session, so duplicate it
     */
    int fd = dup(scoreboard_fd);

    /*
     * Lock the scoreboard file while updating the scoreboard.
     * Wait until another process that holds the lock releases it again.
     */
    if (flock(fd, LOCK_EX) == -1)
    {
        perror("Could not lock the scoreboard file");
    }

    /*
     * Reposition the file offset to the start, otherwise we couldn't load
     * there any data after reading or writing the file. gzrewind() didn't
     * work after writing the file, thus we ensure that the position is as
     * expected here.
     */
     lseek(fd, 0, SEEK_SET);

    return gzdopen(fd, mode);
}
#endif

/* scoreboard version
 *
 * version 1: player_cod (and, where meaningful, the "cause" of death)
 *            were stored as raw numeric enum values, so adding a new
 *            cause anywhere but the very end of the enum could shift
 *            older entries onto the wrong description.
 * version 2: player_sex, player_cod and the "cause" of death (when it
 *            names a constant of another enum) are stored as their
 *            symbolic constant names instead. Version 1 scoreboards
 *            are still read (see scores_load()) and transparently
 *            rewritten in version 2 format. */
static const gint sb_ver = 2;

/* maximum number of entries kept in the persisted scoreboard */
static const guint sb_entries = 100;

static void scores_save(GList *gs);

/* "cause" is polymorphic: its meaning depends on cod. Where it names a
   constant of another X-macro'd enum, dispatch to that enum's string
   conversion; everywhere else cause is unused (always 0) and is simply
   stored as a number. */
static const char *score_cause_string(player_cod cod, guint cause)
{
    switch (cod)
    {
    case PD_EFFECT:  return effect_t_string(cause);
    case PD_MONSTER: return monster_t_string(cause);
    case PD_TRAP:    return trap_t_string(cause);
    case PD_MAP:     return map_tile_t_string(cause);
    case PD_SPELL:   return spell_id_string(cause);
    case PD_CURSE:   return item_t_string(cause);
    case PD_SOBJECT: return sobject_t_string(cause);
    default:         return NULL;
    }
}

static guint score_cause_value(player_cod cod, const char *str)
{
    switch (cod)
    {
    case PD_EFFECT:  return effect_t_value(str);
    case PD_MONSTER: return monster_t_value(str);
    case PD_TRAP:    return trap_t_value(str);
    case PD_MAP:     return map_tile_t_value(str);
    case PD_SPELL:   return spell_id_value(str);
    case PD_CURSE:   return item_t_value(str);
    case PD_SOBJECT: return sobject_t_value(str);
    default:         return 0;
    }
}

GList *scores_load()
{
    /* linked list of all scores */
    GList *gs = NULL;

    /* read the scoreboard file into memory */
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    gzFile file = scoreboard_open_cloned_fd("rb");
#else
    gzFile file = gzopen(nlarn_highscores, "rb");
#endif

    if (file == NULL)
    {
        return gs;
    }

    /* size of buffer to store uncompressed scoreboard content */
    const gint bufsize = 8192;

    /* allocate buffer space */
    gchar *scores = g_malloc(bufsize);

    /* count of buffer allocations */
    gint bufcount = 1;

    /* read the scoreboard file
     * append subsequent blocks at the end of the previously read block */
    while(gzread(file, scores + ((bufcount - 1) * bufsize), bufsize) == bufsize)
    {
        /* it seems the buffer space was insufficient -> increase it */
        bufcount += 1;
        scores = g_realloc(scores, (bufsize * bufcount));
    }

    /* close save file */
    gzclose(file);


    /* parse the scores */
    cJSON *pscores;
    if ((pscores = cJSON_Parse(scores)) == NULL)
    {
        /* empty file, no entries */
        return gs;
    }

    /* version of scoreboard file */
    gint version = cJSON_GetObjectItem(pscores, "version")->valueint;

    /* point to the first entry of the scores array */
    cJSON *s_entry = cJSON_GetObjectItem(pscores, "scores")->child;

    while (s_entry != NULL)
    {
        /* create new score record */
        score_t *nscore = g_malloc(sizeof(score_t));

        /* add record to array */
        gs = g_list_append(gs, nscore);

        /* fill score record fields with data */
        nscore->player_name = g_strdup(cJSON_GetObjectItem(s_entry, "player_name")->valuestring);
        nscore->score      = cJSON_GetObjectItem(s_entry, "score")->valueint;
        nscore->moves      = cJSON_GetObjectItem(s_entry, "moves")->valueint;
        nscore->hp         = cJSON_GetObjectItem(s_entry, "hp")->valueint;
        nscore->hp_max     = cJSON_GetObjectItem(s_entry, "hp_max")->valueint;
        nscore->level      = cJSON_GetObjectItem(s_entry, "level")->valueint;
        nscore->level_max  = cJSON_GetObjectItem(s_entry, "level_max")->valueint;
        nscore->dlevel     = cJSON_GetObjectItem(s_entry, "dlevel")->valueint;
        nscore->dlevel_max = cJSON_GetObjectItem(s_entry, "dlevel_max")->valueint;
        nscore->difficulty = cJSON_GetObjectItem(s_entry, "difficulty")->valueint;
        nscore->time_start = cJSON_GetObjectItem(s_entry, "time_start")->valueint;
        nscore->time_end   = cJSON_GetObjectItem(s_entry, "time_end")->valueint;

        if (version < 2)
        {
            /* version 1: sex, cod and cause are raw numeric enum values */
            nscore->sex   = cJSON_GetObjectItem(s_entry, "sex")->valueint;
            nscore->cod   = cJSON_GetObjectItem(s_entry, "cod")->valueint;
            nscore->cause = cJSON_GetObjectItem(s_entry, "cause")->valueint;
        }
        else
        {
            /* version 2+: sex and cod are symbolic constant names, and
               so is cause wherever it names a constant of another enum */
            nscore->sex = player_sex_value(
                    cJSON_GetObjectItem(s_entry, "sex")->valuestring);
            nscore->cod = player_cod_value(
                    cJSON_GetObjectItem(s_entry, "cod")->valuestring);

            cJSON *cause_val = cJSON_GetObjectItem(s_entry, "cause");
            nscore->cause = cJSON_IsString(cause_val)
                    ? score_cause_value(nscore->cod, cause_val->valuestring)
                    : (guint)cause_val->valueint;
        }

        s_entry = s_entry->next;
    }

    /* free memory  */
    cJSON_Delete(pscores);

    /* free the memory allocated for gzread */
    g_free(scores);

    return gs;
}

static void scores_save(GList *gs)
{
    /* serialize the scores */
    cJSON *sf = cJSON_CreateObject();
    cJSON_AddNumberToObject(sf, "version", sb_ver);
    cJSON *scores = cJSON_CreateArray();
    cJSON_AddItemToObject(sf, "scores", scores);

    for (GList *iterator = gs; iterator; iterator = iterator->next)
    {
        score_t *score = iterator->data;

        /* create new object to store a single scoreboard entry */
        cJSON *sc = cJSON_CreateObject();
        cJSON_AddItemToArray(scores, sc);

        /* add all scoreboard entry values */
        cJSON_AddStringToObject(sc, "player_name", score->player_name);
        cJSON_AddStringToObject(sc, "sex", player_sex_string(score->sex));
        cJSON_AddNumberToObject(sc, "score", score->score);
        cJSON_AddNumberToObject(sc, "moves", score->moves);
        cJSON_AddStringToObject(sc, "cod", player_cod_string(score->cod));

        const char *cause_str = score_cause_string(score->cod, score->cause);
        if (cause_str)
            cJSON_AddStringToObject(sc, "cause", cause_str);
        else
            cJSON_AddNumberToObject(sc, "cause", score->cause);

        cJSON_AddNumberToObject(sc, "hp", score->hp);
        cJSON_AddNumberToObject(sc, "hp_max", score->hp_max);
        cJSON_AddNumberToObject(sc, "level", score->level);
        cJSON_AddNumberToObject(sc, "level_max", score->level_max);
        cJSON_AddNumberToObject(sc, "dlevel", score->dlevel);
        cJSON_AddNumberToObject(sc, "dlevel_max", score->dlevel_max);
        cJSON_AddNumberToObject(sc, "difficulty", score->difficulty);
        cJSON_AddNumberToObject(sc, "time_start", score->time_start);
        cJSON_AddNumberToObject(sc, "time_end", score->time_end);
    }

    /* export the cJSON structure to a string */
    char *uscores = cJSON_PrintUnformatted(sf);
    cJSON_Delete(sf);

    /* open the file for writing */
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    gzFile sb = scoreboard_open_cloned_fd("wb");
#else
    gzFile sb = gzopen(nlarn_highscores, "wb");
#endif

    if (sb == NULL)
    {
        /* opening the file failed */
        if (nlarn) log_add_entry(nlarn->log, _("Error opening scoreboard file."));
        free(uscores);
        return;
    }

    /* write to file */
    if (gzputs(sb, uscores) != (int)strlen(uscores))
    {
        /* handle error */
        int err;

        if (nlarn) log_add_entry(nlarn->log, _("Error writing scoreboard file: %s"),
                      gzerror(sb, &err));

        free(uscores);
        return;
    }

    /* Close the file. This action unlocks the scoreboard file again. */
    gzclose(sb);

    /* return memory */
    g_free(uscores);
}

static int score_compare(const void *scr_a, const void *scr_b)
{
    score_t *a = (score_t *)scr_a;
    score_t *b = (score_t *)scr_b;

    if (a->score > b->score)
        return -1;

    if (b->score > a->score)
        return 1;

    return 0;
}

score_t *score_new(game *g, player_cod cod, int cause)
{
    score_t *score = g_malloc0(sizeof(score_t));

    score->player_name = g_strdup(g->p->name);
    score->sex = g->p->sex;
    score->score = player_calc_score(g->p, (cod == PD_WON) ? true : false);
    score->moves = game_turn(g);
    score->cod = cod;
    score->cause = cause;
    score->hp = g->p->hp;
    score->hp_max = g->p->hp_max;
    score->level = g->p->level;
    score->level_max = g->p->stats.max_level;
    score->dlevel = Z(g->p->pos);
    score->dlevel_max = g->p->stats.deepest_level;
    score->difficulty = game_difficulty(g);
    score->time_start = g->time_start;
    score->time_end = time(0);

    return score;
}

GList *score_add(score_t *score)
{
    g_assert (score != NULL);

    GList *gs = scores_load();

    /* add new score */
    gs = g_list_append(gs, score);

    /* sort scoreboard entries */
    gs = g_list_sort(gs, (GCompareFunc)score_compare);

    /* Persist only the top sb_entries entries, using a
       shallow copy so the full list - which may contain one entry more
       than that, namely the score just added if it did not make the
       cut - is still returned unchanged. The caller needs its score_t
       to remain valid regardless, and keeps freeing every entry of the
       returned list itself. */
    GList *persisted = g_list_copy(gs);
    if (g_list_length(persisted) > sb_entries)
    {
        GList *cutoff = g_list_nth(persisted, sb_entries);
        cutoff->prev->next = NULL;
        cutoff->prev = NULL;
        g_list_free(cutoff);
    }

    scores_save(persisted);
    g_list_free(persisted);

    return gs;
}

char *score_death_description(score_t *score, int verbose)
{
    const char *desc;

    g_assert(score != NULL);

    switch (score->cod)
    {
    case PD_LASTLEVEL:
        desc = _("passed away");
        break;

    case PD_STUCK:
        desc = _("got stuck in solid rock");
        break;

    case PD_TOO_LATE:
        desc = _("returned with the potion too late");
        break;

    case PD_WON:
        desc = _("returned in time with the cure");
        break;

    case PD_LOST:
        desc = _("could not find the potion in time");
        break;

    case PD_QUIT:
        desc = _("quit the game");
        break;

    case PD_GENOCIDE:
        desc = _("genocided");
        break;

    case PD_SPELL:
        if (score->cause < SP_MAX)
            desc = _("blasted");
        else
            desc = _("got killed");
        break;

    default:
        desc = _("killed");
    }

    GString *text = g_string_new_len(NULL, 200);

    g_string_append_printf(text, "%s (%c), %s", score->player_name,
                           (score->sex == PS_MALE) ? 'm' : 'f', desc);


    if (score->cod == PD_GENOCIDE)
    {
        g_string_append(text, (score->sex == PS_MALE)
                               ? _(" himself") : _(" herself"));
    }

    if (verbose)
    {
        g_string_append_printf(text, _(" on level %s"), map_names[score->dlevel]);

        if (score->dlevel_max > score->dlevel)
        {
            g_string_append_printf(text, _(" (max. %s)"), map_names[score->dlevel_max]);
        }

        if (player_cod_is_death(score->cod))
        {
            g_string_append_printf(text, _(" with %d and a maximum of %d hp"),
                                   score->hp, score->hp_max);
        }
    }

    switch (score->cod)
    {
    case PD_EFFECT:
        switch (score->cause)
        {
        case ET_DEC_STR:
            g_string_append(text, _(" by enfeeblement."));
            break;

        case ET_DEC_DEX:
            g_string_append(text, _(" by clumsiness."));
            break;

        case ET_POISON:
            g_string_append(text, _(" by poison."));
            break;

        /* no other effects can cause death at the moment */
        default:
            break;
        }
        break;

    case PD_LASTLEVEL:
        g_string_append(text, (score->sex == PS_MALE)
                               ? _(". He left his body.")
                               : _(". She left her body."));
        break;

    case PD_MONSTER:
        /* TODO: regard monster's invisibility */
        /* TODO: while sleeping / doing sth. */
        g_string_append_printf(text, _(" by %s."),
                               monster_type_name_art(score->cause,
                                                     ART_INDEF, GC_ACC));
        break;

    case PD_SPHERE:
        g_string_append(text, _(" by a sphere of destruction."));
        break;

    case PD_TRAP:
        g_string_append_printf(text, score->cause == TT_TRAPDOOR
                                   ? _(" by falling through %s.")
                                   : _(" by %s."),
                               noun_phrase(trap_description_raw(score->cause),
                                           ART_INDEF, GC_ACC, false, false));
        break;

    case PD_MAP:
        g_string_append_printf(text, _(" by %s."),
                noun_phrase(mt_get_desc_raw(score->cause), ART_NONE,
                            GC_ACC, false, false));
        break;

    case PD_SPELL:
        /* player spell */
        g_string_append_printf(text, (score->sex == PS_MALE)
                                   ? _(" himself away with the spell \"%s\".")
                                   : _(" herself away with the spell \"%s\"."),
                               spell_name_by_id(score->cause));
        break;

    case PD_CURSE:
        g_string_append_printf(text, _(" by %s."),
                noun_phrase_adj(item_name_sg_raw(score->cause),
                                C_("item status", "cursed"),
                                ART_INDEF, GC_ACC, false, false));
        break;

    case PD_SOBJECT:
        switch (score->cause)
        {
        case LS_FOUNTAIN:
            g_string_append(text, _(" by toxic water from a fountain."));
            break;
        default:
            g_string_append(text, _(" by falling down a staircase."));
            break;
        }
        break;

    case PD_RICOCHET:
        g_string_append_printf(text, (score->sex == PS_MALE)
                               ? _(" by his own ricocheting shot.")
                               : _(" by her own ricocheting shot."));
        break;

    default:
        /* no further description */
        g_string_append_c(text, '.');
        break;
    }

    if (verbose)
    {
        g_autofree char *points = g_strdup_printf("%" G_GUINT64_FORMAT,
                                                   score->score);
        g_string_append_printf(text, (score->sex == PS_MALE)
                                   ? _(" He has scored %s points, "
                                       "with the difficulty set to %d.")
                                   : _(" She has scored %s points, "
                                       "with the difficulty set to %d."),
                               points, score->difficulty);
    }

    return g_string_free(text, false);
}

static void scores_append_entry(GString *text, score_t *cscore, int rank1based,
                                bool highlight)
{
    gchar *desc = score_death_description(cscore, false);
    g_string_append_printf(text, "%s%2d) %7" G_GUINT64_FORMAT " %s\n",
                           highlight ? "`EMPH`" : "",
                           rank1based, cscore->score, desc);

    char *dungeon_desc = ""; /* empty for the town */
    if (cscore->dlevel > 10)
        dungeon_desc = _("volcano lvl. ");
    else if (cscore->dlevel > 0)
        dungeon_desc = _("caverns lvl. ");

    g_string_append_printf(text, _("            [exp. level %d, %s%s, %d/%d hp, difficulty %d]%s\n"),
                           cscore->level, dungeon_desc, map_names[cscore->dlevel],
                           cscore->hp, cscore->hp_max, cscore->difficulty,
                           highlight ? "`end`" : "");
    g_free(desc);
}

char *scores_to_string(GList *scores, score_t *score)
{
    /* no scoreboard entries? */
    if (!scores) return NULL;

    GString *text = g_string_new(NULL);

    guint rank = 0;
    GList *iterator = scores;

    /* show scores surrounding a specific score? */
    if (score)
    {
        /* determine position of score in the score list */
        rank = g_list_index(scores, score);

        /* the scoreboard only keeps the top entries; anything beyond
           that was never saved. Show the last few entries for context -
           i.e. what this score would have needed to beat - followed by
           a note that it did not make the cut. */
        if (rank >= sb_entries)
        {
            const int last_n = 5;
            int start = max(sb_entries - last_n, 0);
            GList *last_iter = g_list_nth(scores, start);

            for (guint nrec = start; last_iter && nrec < sb_entries;
                 last_iter = last_iter->next, nrec++)
            {
                scores_append_entry(text, (score_t *)last_iter->data, nrec + 1, false);
            }

            g_string_append_c(text, '\n');

            g_autofree char *points = g_strdup_printf("%" G_GUINT64_FORMAT,
                                                       score->score);
            g_string_append_printf(text, _("With a score of %s, this "
                    "performance did not make it into the top %d of the "
                    "NLarn Hall of Fame."), points, sb_entries);

            return g_string_free(text, false);
        }

        /* get entry three entries up of current/top score in list */
        iterator = g_list_nth(scores, max(rank - 3, 0));
    }

    /* display up to 7 surrounding entries or all when score wasn't specified */
    for (int nrec = max(rank - 3, 0);
         iterator && (score ? (nrec < (max(rank, 0) + 4)) : true);
         iterator = iterator->next, nrec++)
    {
        score_t *cscore = (score_t *)iterator->data;
        scores_append_entry(text, cscore, nrec + 1, cscore == score);
    }

    return g_string_free(text, false);
}

void scores_destroy(GList *gs)
{
    for (GList *iterator = gs; iterator; iterator = iterator->next)
    {
        score_t *score = iterator->data;
        g_free(score->player_name);
        g_free(score);
    }

    g_list_free(gs);
}
