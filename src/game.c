/*
 * game.c
 * Copyright (C) Joachim de Groot 2009 <jdegroot@web.de>
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

#include "nlarn.h"

static void game_move_monsters(game *g);
static void game_move_spheres(game *g);

static GList *game_scores_load(game *g);
static void game_scores_save(game *g, GList *scores);
static int game_score_compare(const void *scr_a, const void *scr_b);

static const char *mesgfile = "nlarn.msg";
static const char *helpfile = "nlarn.hlp";
static const char *mazefile = "maze";
static const char *fortunes = "fortune";
static const char *highscores = "highscores";

int game_save(game *g, char *filename)
{

    return EXIT_SUCCESS;
}

game *game_load(char *filename)
{

    return EXIT_SUCCESS;
}

game *game_new(int argc, char *argv[])
{
    game *g;
    int i;
    item_t it;

    /* these will be filled by the command line parser */
    static gint difficulty = 0;
    static gboolean wizard = FALSE;
    static char *name = NULL;
    static gboolean female = FALSE; /* default: male */
    static char *auto_pickup = NULL;

    /* objects needed to parse the command line */
    GError *error = NULL;
    GOptionContext *context;

    static GOptionEntry entries[] =
    {
        { "difficulty",  'd', 0, G_OPTION_ARG_INT,    &difficulty,  "Set difficulty",       NULL },
        { "wizard",      'w', 0, G_OPTION_ARG_NONE,   &wizard,      "Enable wizard mode",   NULL },
        { "name",        'n', 0, G_OPTION_ARG_STRING, &name,        "Set character's name", NULL },
        { "female",        0, 0, G_OPTION_ARG_NONE,   &female,      "Make a female character (default: male)", NULL },
        { "auto-pickup", 'a', 0, G_OPTION_ARG_STRING, &auto_pickup, "Item types to pick up automatically, e.g. '$*+'", NULL },
        { NULL }
    };

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    g_option_context_free(context);


    /* one game, please */
    g = g_malloc0(sizeof(game));

    /* determine paths and file names */
    g->basedir = g_path_get_dirname(argv[0]);

    /* TODO: check if files exist in this directory, otherwise try others */
    g->libdir = g_build_path(G_DIR_SEPARATOR_S, g->basedir, "lib", NULL);

    g->mesgfile = g_build_filename(g->libdir, mesgfile, NULL);
    g->helpfile = g_build_filename(g->libdir, helpfile, NULL);
    g->mazefile = g_build_filename(g->libdir, mazefile, NULL);
    g->fortunes = g_build_filename(g->libdir, fortunes, NULL);
    g->highscores = g_build_filename(g->libdir, highscores, NULL);

    /* set game parameters */
    game_difficulty(g) = difficulty;
    game_wizardmode(g) = wizard;

    /* generate player */
    g->p = player_new(g);

    if (name)
    {
        g->p->name = name;
    }
    else
    {
        /* get full name from system */
        g->p->name = (char *)g_get_real_name();
    }

    g->p->sex = !female;

    /* parse autopickup settings */
    if (auto_pickup)
    {
        for (i = 0; i < strlen(auto_pickup); i++)
        {
            for (it = IT_NONE; it < IT_MAX; it++)
            {
                if (auto_pickup[i] == item_image(it))
                {
                    g->p->settings.auto_pickup[it] = TRUE;
                }
            }
        }
    }

    /* allocate space for levels */
    for (i = 0; i < LEVEL_MAX; i++)
    {
        g->levels[i] = g_malloc0(sizeof (level));
        g->levels[i]->nlevel = i;
    }

    /* game time handling */
    g->gtime = 1;
    g->time_start = time(NULL);

    /* welcome message */
    log_add_entry(g->p->log, "Welcome to NLarn %d.%d.%d!",
                  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    log_set_time(g->p->log, g->gtime);

    /* randomize unidentified item descriptions */
    amulet_material_shuffle();
    book_desc_shuffle();
    potion_desc_shuffle();
    ring_material_shuffle();
    scroll_desc_shuffle();

    /* fill store with goods */
    building_dndstore_init();

    return g;
}

int game_destroy(game *g)
{
    int i;

    assert(g != NULL);

    /* everything must go */
    for (i = 0; i < LEVEL_MAX; i++)
    {
        level_destroy(g->levels[i]);
    }

    g_free(g->basedir);
    g_free(g->libdir);

    g_free(g->mesgfile);
    g_free(g->helpfile);
    g_free(g->mazefile);
    g_free(g->fortunes);
    g_free(g->highscores);

    player_destroy(g->p);
    g_free(g);

    return EXIT_SUCCESS;
}

void game_scores_destroy(GList *gs)
{
    GList *iterator;
    game_score_t *score;

    for (iterator = gs; iterator; iterator = iterator->next)
    {
        score = iterator->data;
        g_free(score->player_name);

        g_free(score);
    }

    g_list_free(gs);
}

game_score_t *game_score(game *g, player_cod cod, int cause)
{
    game_score_t *score;

    score = g_malloc0(sizeof(game_score_t));

    score->player_name = g_strdup(g->p->name);
    score->sex = g->p->sex;
    score->score = player_calc_score(g->p, (cod == PD_WON) ? TRUE : FALSE);
    score->moves = game_turn(g);
    score->cod = cod;
    score->cause = cause;
    score->hp = g->p->hp;
    score->hp_max = g->p->hp_max;
    score->level = g->p->level->nlevel;
    score->level_max = g->p->stats.max_level;
    score->dlevel = g->p->level->nlevel;
    score->dlevel_max = g->p->stats.deepest_level;
    score->difficulty = game_difficulty(g);
    score->time_start = g->time_start;
    score->time_end = time(0);

    return score;
}

GList *game_score_add(game *g, game_score_t *score)
{
    GList *gs, *el;
    game_score_t *oscore;

    assert (g != NULL && score != NULL);

    gs = game_scores_load(g);

    /* add new score */
    gs = g_list_append(gs, score);

    /* sort scoreboard entries */
    gs = g_list_sort(gs,  (GCompareFunc)game_score_compare);

    /* only interested in the last 99 scores */
    while (g_list_length(gs) > 99)
    {
        el = g_list_last(gs);

        oscore = el->data;

        g_free(oscore->player_name);
        g_free(oscore);

        gs = g_list_delete_link(gs, el);
    }

    /* save new scoreboard */
    game_scores_save(g, gs);

    return gs;
}

void game_spin_the_wheel(game *g, int times)
{
    int turn, monster_nr;
    monster *m;
    damage *dam;

    assert(g != NULL && times > 0);

    level_timer(g->p->level, times);

    for (turn = 0; turn < times; turn++)
    {
        player_regenerate(g->p);
        player_effects_expire(g->p, 1);

        /* check if player is stuck inside a wall without walk through wall */
        if ((level_tiletype_at(g->p->level, g->p->pos) == LT_WALL)
                && !player_effect(g->p, ET_WALL_WALK))
        {
            player_die(g->p, PD_STUCK, 0);
        }

        /* deal damage cause by level tiles to player */
        if ((dam = level_tile_damage(g->p->level, g->p->pos)))
        {
            player_damage_take(g->p, dam, PD_LEVEL,
                           level_tiletype_at(g->p->level, g->p->pos));
        }

        game_move_monsters(g);
        game_move_spheres(g);

        /* modify effects */
        for (monster_nr = 1; monster_nr <= g->p->level->mlist->len; monster_nr++)
        {
            m = g_ptr_array_index(g->p->level->mlist, monster_nr - 1);
            monster_effect_expire(m, g->p->log);
        }

        g->gtime++; /* count up the time  */
        log_set_time(g->p->log, g->gtime); /* adjust time for log entries */
    }

}

/**
 *  move all monsters on player's current level
 *
 *  @param the game
 *  @return Returns no value.
 */
static void game_move_monsters(game *g)
{
    level *l;
    /* handle to current monster */
    monster *m;
    int monster_nr;

    assert(g != NULL);

    /* make shortcut */
    l = g->p->level;

    for (monster_nr = 1; monster_nr <= l->mlist->len; monster_nr++)
    {
        m = g_ptr_array_index(l->mlist, monster_nr - 1);
        monster_move(m, g->p);
    } /* foreach monster */
}

static void game_move_spheres(game *g)
{
    level *l;
    sphere *s;
    int sphere_nr;

    assert(g != NULL);

    /* make shortcut */
    l = g->p->level;

    for (sphere_nr = 1; sphere_nr <= l->slist->len; sphere_nr++)
    {
        s = g_ptr_array_index(l->slist, sphere_nr - 1);
        sphere_move(s, l);
    }
}

static GList *game_scores_load(game *g)
{
    GList *gs = NULL;

    game_score_t *nscore;
    gint32 nlen;
    FILE *sb = NULL;

    /* open score file */
    sb = fopen(game_highscores(g), "r");

    if (sb == NULL)
    {
        return gs;
    }

    /* FIXME: error handling */

    while (!feof(sb) && !ferror(sb))
    {
        /* read length of player's name */
        fread(&nlen, sizeof(guint32), 1, sb);

        if (feof(sb))
            break;

        nscore = g_malloc(sizeof(game_score_t));

        /* alloc space for name */
        nscore->player_name = g_malloc(nlen + 1);

        /* read player's name */
        fread(nscore->player_name, sizeof(char), nlen, sb);

        /* null terminate player's name */
        nscore->player_name[nlen] = '\0';

        /* read the rest of the record */
        fread(&(nscore->sex), sizeof(game_score_t) - sizeof(char *), 1, sb);

        /* add record to array */
        gs = g_list_append(gs, nscore);
    }

    fclose(sb);

    return gs;
}

static void game_scores_save(game *g, GList *gs)
{
    int slen = 0;
    GList *iterator;
    game_score_t *score;

    FILE *sb = NULL;

    sb = fopen(game_highscores(g), "w+");

    if (sb == NULL)
    {
        /* do something! */
        return;
    }

    for (iterator = gs; iterator; iterator = iterator->next)
    {
        score = iterator->data;

        /* write length of player's name to file */
        slen = strlen(score->player_name);
        fwrite(&slen, sizeof(guint32), 1, sb);

        /* write player's name to file */
        fwrite(score->player_name, sizeof(char), slen, sb);

        /* write remaining records to file */
        fwrite(&(score->sex), sizeof(game_score_t) - sizeof(char *), 1, sb);
    }

    fclose(sb);
}

static int game_score_compare(const void *scr_a, const void *scr_b)
{
    game_score_t *a = (game_score_t *)scr_a;
    game_score_t *b = (game_score_t *)scr_b;

    if (a->score > b->score)
        return -1;

    if (b->score > a->score)
        return 1;

    return 0;
}
