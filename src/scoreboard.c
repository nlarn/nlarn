/*
 * scoreboard.c
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

/* scoreboard version */
static const gint sb_ver = 1;

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

    /* parsed scoreboard; scoreboard entry */
    cJSON *pscores, *s_entry;

    /* parse the scores */
    if ((pscores = cJSON_Parse(scores)) == NULL)
    {
        /* empty file, no entries */
        return gs;
    }

    /* version of scoreboard file */
    gint version = cJSON_GetObjectItem(pscores, "version")->valueint;

    if (version < sb_ver)
    {
        /* TODO: when there are multiple versions, handle old versions here */
    }

    /* point to the first entry of the scores array */
    s_entry = cJSON_GetObjectItem(pscores, "scores")->child;

    while (s_entry != NULL)
    {
        /* create new score record */
        score_t *nscore = g_malloc(sizeof(score_t));

        /* add record to array */
        gs = g_list_append(gs, nscore);

        /* fill score record fields with data */
        nscore->player_name = g_strdup(cJSON_GetObjectItem(s_entry, "player_name")->valuestring);
        nscore->sex        = cJSON_GetObjectItem(s_entry, "sex")->valueint;
        nscore->score      = cJSON_GetObjectItem(s_entry, "score")->valueint;
        nscore->moves      = cJSON_GetObjectItem(s_entry, "moves")->valueint;
        nscore->cod        = cJSON_GetObjectItem(s_entry, "cod")->valueint;
        nscore->cause      = cJSON_GetObjectItem(s_entry, "cause")->valueint;
        nscore->hp         = cJSON_GetObjectItem(s_entry, "hp")->valueint;
        nscore->hp_max     = cJSON_GetObjectItem(s_entry, "hp_max")->valueint;
        nscore->level      = cJSON_GetObjectItem(s_entry, "level")->valueint;
        nscore->level_max  = cJSON_GetObjectItem(s_entry, "level_max")->valueint;
        nscore->dlevel     = cJSON_GetObjectItem(s_entry, "dlevel")->valueint;
        nscore->dlevel_max = cJSON_GetObjectItem(s_entry, "dlevel_max")->valueint;
        nscore->difficulty = cJSON_GetObjectItem(s_entry, "difficulty")->valueint;
        nscore->time_start = cJSON_GetObjectItem(s_entry, "time_start")->valueint;
        nscore->time_end   = cJSON_GetObjectItem(s_entry, "time_end")->valueint;

        s_entry = s_entry->next;
    }

    /* free memory  */
    cJSON_Delete(pscores);

    /* free the memory allocated for gzread */
    g_free(scores);

    return gs;
}

static void scores_save(game *g, GList *gs)
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
        cJSON_AddNumberToObject(sc, "sex", score->sex);
        cJSON_AddNumberToObject(sc, "score", score->score);
        cJSON_AddNumberToObject(sc, "moves", score->moves);
        cJSON_AddNumberToObject(sc, "cod", score->cod);
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
        log_add_entry(g->log, "Error opening scoreboard file.");
        free(uscores);
        return;
    }

    /* write to file */
    if (gzputs(sb, uscores) != (int)strlen(uscores))
    {
        /* handle error */
        int err;

        log_add_entry(g->log, "Error writing scoreboard file: %s",
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

GList *score_add(game *g, score_t *score)
{
    g_assert (g != NULL && score != NULL);

    GList *gs = scores_load();

    /* add new score */
    gs = g_list_append(gs, score);

    /* sort scoreboard entries */
    gs = g_list_sort(gs, (GCompareFunc)score_compare);

    /* save new scoreboard */
    scores_save(g, gs);

    return gs;
}

char *score_death_description(score_t *score, int verbose)
{
    const char *desc;
    GString *text;

    g_assert(score != NULL);

    switch (score->cod)
    {
    case PD_LASTLEVEL:
        desc = "passed away";
        break;

    case PD_STUCK:
        desc = "got stuck in solid rock";
        break;

    case PD_TOO_LATE:
        desc = "returned with the potion too late";
        break;

    case PD_WON:
        desc = "returned in time with the cure";
        break;

    case PD_LOST:
        desc = "could not find the potion in time";
        break;

    case PD_QUIT:
        desc = "quit the game";
        break;

    case PD_GENOCIDE:
        desc = "genocided";
        break;

    case PD_SPELL:
        if (score->cause < SP_MAX)
            desc = "blasted";
        else
            desc = "got killed";
        break;

    default:
        desc = "killed";
    }

    text = g_string_new_len(NULL, 200);

    g_string_append_printf(text, "%s (%c), %s", score->player_name,
                           (score->sex == PS_MALE) ? 'm' : 'f', desc);


    if (score->cod == PD_GENOCIDE)
    {
        g_string_append_printf(text, " %sself",
                               (score->sex == PS_MALE) ? "him" : "her");
    }

    if (verbose)
    {
        g_string_append_printf(text, " on level %s", map_names[score->dlevel]);

        if (score->dlevel_max > score->dlevel)
        {
            g_string_append_printf(text, " (max. %s)", map_names[score->dlevel_max]);
        }

        if (score->cod < PD_TOO_LATE)
        {
            g_string_append_printf(text, " with %d and a maximum of %d hp",
                                   score->hp, score->hp_max);
        }
    }

    switch (score->cod)
    {
    case PD_EFFECT:
        switch (score->cause)
        {
        case ET_DEC_STR:
            g_string_append(text, " by enfeeblement.");
            break;

        case ET_DEC_DEX:
            g_string_append(text, " by clumsiness.");
            break;

        case ET_POISON:
            g_string_append(text, " by poison.");
            break;
        }
        break;

    case PD_LASTLEVEL:
        g_string_append_printf(text,". %s left %s body.",
                               (score->sex == PS_MALE) ? "He" : "She",
                               (score->sex == PS_MALE) ? "his" : "her");
        break;

    case PD_MONSTER:
        /* TODO: regard monster's invisibility */
        /* TODO: while sleeping / doing sth. */
        g_string_append_printf(text, " by %s %s.",
                               a_an(monster_type_name(score->cause)),
                               monster_type_name(score->cause));
        break;

    case PD_SPHERE:
        g_string_append(text, " by a sphere of destruction.");
        break;

    case PD_TRAP:
        g_string_append_printf(text, " by %s%s %s.",
                               score->cause == TT_TRAPDOOR ? "falling through " : "",
                               a_an(trap_description(score->cause)),
                               trap_description(score->cause));
        break;

    case PD_MAP:
        g_string_append_printf(text, " by %s.", mt_get_desc(score->cause));
        break;

    case PD_SPELL:
        /* player spell */
        g_string_append_printf(text, " %s away with the spell \"%s\".",
                               (score->sex == PS_MALE) ? "himself" : "herself",
                               spell_name_by_id(score->cause));
        break;

    case PD_CURSE:
        g_string_append_printf(text, " by a cursed %s.",
                               item_name_sg(score->cause));
        break;

    case PD_SOBJECT:
        switch (score->cause)
        {
        case LS_FOUNTAIN:
            g_string_append(text, " by toxic water from a fountain.");
            break;
        default:
            g_string_append(text, " by falling down a staircase.");
            break;
        }
        break;

    default:
        /* no further description */
        g_string_append_c(text, '.');
        break;
    }

    if (verbose)
    {
        g_string_append_printf(text, " %s has scored %" G_GINT64_FORMAT
                               " points, with the difficulty set to %d.",
                               (score->sex == PS_MALE) ? "He" : "She",
                               score->score, score->difficulty);
    }

    return g_string_free(text, false);
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

        /* get entry three entries up of current/top score in list */
       iterator = g_list_nth(scores, max(rank - 3, 0));
    }

    /* display up to 7 surronding entries or all when score wasn't specified */
    for (int nrec = max(rank - 3, 0);
         iterator && (score ? (nrec < (max(rank, 0) + 4)) : true);
         iterator = iterator->next, nrec++)
    {
        gchar *desc;

        score_t *cscore = (score_t *)iterator->data;

        desc = score_death_description(cscore, false);
        g_string_append_printf(text, "  %c%2d) %7" G_GINT64_FORMAT " %s\n",
                               (cscore == score) ? '*' : ' ',
                               nrec + 1, cscore->score, desc);

        g_string_append_printf(text, "               [exp. level %d, caverns lvl. %s, %d/%d hp, difficulty %d]\n",
                               cscore->level, map_names[cscore->dlevel],
                               cscore->hp, cscore->hp_max, cscore->difficulty);
        g_free(desc);
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
