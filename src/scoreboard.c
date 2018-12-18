/*
 * scoreboard.c
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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
# include <sys/file.h>
#endif

#include "nlarn.h"
#include "scoreboard.h"
#include "cJSON.h"

#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
/* file descriptor for the scoreboard file when running setgid */
int scoreboard_fd = -1;
#endif

/* scoreboard version */
static const gint sb_ver = 1;

static GList *scores_load()
{
    /* linked list of all scores */
    GList *gs = NULL;

    /* read the scoreboard file into memory */
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    /* we'll need the file desciptor for saving, too, so duplicate it */
    int fd = dup(scoreboard_fd);

    /*
     * Lock the scoreboard file while updating the scoreboard.
     * Wait until another process that holds the lock releases it again.
     */
    if (flock(fd, LOCK_EX) == -1)
    {
        perror("Could not lock the scoreboard file");
    }

    gzFile file = gzdopen(fd, "rb");
#else
    gzFile file = gzopen(nlarn->highscores, "rb");
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

#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    /* reposition to the start otherwise writing would append */
    gzrewind(file);
#endif
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
    gzFile sb = gzdopen(scoreboard_fd, "wb");
#else
    gzFile sb = gzopen(nlarn->highscores, "wb");
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

    /*
     * Close file.
     * As this was the last reference to that file, this action
     * unlocks the scoreboard file again.
     */
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
    score->score = player_calc_score(g->p, (cod == PD_WON) ? TRUE : FALSE);
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
