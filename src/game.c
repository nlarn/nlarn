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

#include <assert.h>
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "spheres.h"
#include "utils.h"

static void game_move_monsters(game *g);
static void game_move_spheres(game *g);

static void game_items_shuffle(game *g);

static GList *game_scores_load(game *g);
static void game_scores_save(game *g, GList *scores);
static int game_score_compare(const void *scr_a, const void *scr_b);

static const char *mesgfile = "nlarn.msg";
static const char *helpfile = "nlarn.hlp";
static const char *mazefile = "maze";
static const char *fortunes = "fortune";
static const char *highscores = "highscores";

void game_new(int argc, char *argv[])
{
    const char *default_lib_dir = "/usr/share/games/nlarn";

    size_t idx;
    item_t it;

    /* these will be filled by the command line parser */
    static gint difficulty = 0;
    static gboolean wizard = FALSE;
    static char *name = NULL;
    static gboolean female = FALSE; /* default: male */
    static char *auto_pickup = NULL;

    /* one game, please */
    nlarn = g_malloc0(sizeof(game));

    /* base directory for a local install */
    nlarn->basedir = g_path_get_dirname(argv[0]);

    /* ini file handling */
    GKeyFile *ini_file = g_key_file_new();
    GError *error = NULL;

    /* This is the path used when the game is installed system wide */
    gchar *filename = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(),
                                   "nlarn.ini", NULL);

    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
    {
        /* ini file has not been found in user config directory */
        g_free(filename);
        filename = g_build_path(G_DIR_SEPARATOR_S, nlarn->basedir, "nlarn.ini", NULL);
    }

    g_key_file_load_from_file(ini_file, filename, G_KEY_FILE_NONE, &error);
    g_free(filename);

    if (!error)
    {
        /* ini file has been found, get values */
        /* clear error after each attempt as values need not to be defined */
        difficulty = g_key_file_get_integer(ini_file, "nlarn", "difficulty", &error);
        g_clear_error(&error);

        wizard = g_key_file_get_boolean(ini_file, "nlarn", "wizard", &error);
        g_clear_error(&error);

        name = g_key_file_get_string(ini_file, "nlarn", "name", &error);
        g_clear_error(&error);

        female = g_key_file_get_boolean(ini_file, "nlarn", "female", &error);
        g_clear_error(&error);

        auto_pickup = g_key_file_get_string(ini_file, "nlarn", "auto-pickup", &error);
        g_clear_error(&error);
    }
    else
    {
        /* file not found. never mind but clean up the mess */
        g_clear_error(&error);
    }

    /* cleanup */
    g_key_file_free(ini_file);

    /* parse the command line */
    static GOptionEntry entries[] =
    {
        { "difficulty",  'd', 0, G_OPTION_ARG_INT,    &difficulty,  "Set difficulty",       NULL },
        { "wizard",      'w', 0, G_OPTION_ARG_NONE,   &wizard,      "Enable wizard mode",   NULL },
        { "name",        'n', 0, G_OPTION_ARG_STRING, &name,        "Set character's name", NULL },
        { "female",        0, 0, G_OPTION_ARG_NONE,   &female,      "Make a female character (default: male)", NULL },
        { "auto-pickup", 'a', 0, G_OPTION_ARG_STRING, &auto_pickup, "Item types to pick up automatically, e.g. '$*+'", NULL },
        { NULL }
    };

    GOptionContext *context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }
    g_option_context_free(context);


    /* determine paths and file names */
    if (g_file_test(default_lib_dir, G_FILE_TEST_IS_DIR))
    {
        /* system-wide data directory exists */
        /* string has to be dup'd as it is feed in the end */
        nlarn->libdir = g_strdup((char *)default_lib_dir);
    }
    else
    {
        /* try to use installation directory */
        nlarn->libdir = g_build_path(G_DIR_SEPARATOR_S, nlarn->basedir, "lib", NULL);
    }

    nlarn->mesgfile = g_build_filename(nlarn->libdir, mesgfile, NULL);
    nlarn->helpfile = g_build_filename(nlarn->libdir, helpfile, NULL);
    nlarn->mazefile = g_build_filename(nlarn->libdir, mazefile, NULL);
    nlarn->fortunes = g_build_filename(nlarn->libdir, fortunes, NULL);
    nlarn->highscores = g_build_filename(nlarn->libdir, highscores, NULL);

    /* set game parameters */
    game_difficulty(nlarn) = difficulty;
    game_wizardmode(nlarn) = wizard;

    /* initialize object hashes (here as they will be needed by player_new) */
    nlarn->inventories = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->monsters = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->spheres = g_hash_table_new(&g_direct_hash, &g_direct_equal);

    /* generate player */
    nlarn->p = player_new();

    if (name)
    {
        nlarn->p->name = name;
    }
    else
    {
        /* get full name from system */
        nlarn->p->name = (char *)g_get_real_name();
    }

    nlarn->p->sex = !female;

    /* parse autopickup settings */
    if (auto_pickup)
    {
        for (idx = 0; idx < strlen(auto_pickup); idx++)
        {
            for (it = IT_NONE; it < IT_MAX; it++)
            {
                if (auto_pickup[idx] == item_image(it))
                {
                    nlarn->p->settings.auto_pickup[it] = TRUE;
                }
            }
        }
    }

    /* allocate space for levels */
    for (idx = 0; idx < MAP_MAX; idx++)
    {
        nlarn->maps[idx] = map_new(idx, nlarn->mazefile);
    }

    /* game time handling */
    nlarn->gtime = 1;
    nlarn->time_start = time(NULL);

    /* welcome message */
    log_add_entry(nlarn->p->log, "Welcome to NLarn %d.%d.%d!",
                  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    if (wizard)
        log_add_entry(nlarn->p->log, "Wizard mode has been activated.");

    log_set_time(nlarn->p->log, nlarn->gtime);

    /* randomize unidentified item descriptions */
    game_items_shuffle(nlarn);

    /* fill the store */
    building_dndstore_init();
}

int game_destroy(game *g)
{
    int i;

    assert(g != NULL);

    /* everything must go */
    for (i = 0; i < MAP_MAX; i++)
    {
        map_destroy(g->maps[i]);
    }

    g_free(g->basedir);
    g_free(g->libdir);

    g_free(g->mesgfile);
    g_free(g->helpfile);
    g_free(g->mazefile);
    g_free(g->fortunes);
    g_free(g->highscores);

    player_destroy(g->p);

    g_hash_table_destroy(g->inventories);
    g_hash_table_destroy(g->items);
    g_hash_table_destroy(g->effects);
    g_hash_table_destroy(g->monsters);
    g_hash_table_destroy(g->spheres);

    g_free(g);

    return EXIT_SUCCESS;
}

int game_save(game *g, char *filename)
{
    int fd;

    game_save_head_t *head = g_malloc0(sizeof(game_save_head_t));

    assert(g != NULL && filename != NULL);

    head->version_major = VERSION_MAJOR;
    head->version_minor = VERSION_MINOR;
    head->version_patch = VERSION_PATCH;

/* mingw does not define the latter */
#ifdef S_IRGRP
    fd = creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
    fd = creat(filename, S_IRUSR | S_IWUSR);
#endif
    if (fd == -1)
    {
        g_free(head);
        return FALSE;
    }

    /* before starting to write jump ahead to leave space for the header */
    lseek(fd, sizeof(game_save_head_t), SEEK_SET);

    /* return to the beginning of the file */
    lseek(fd, 0, SEEK_SET);
    /* and write the file header */
    write(fd, head, sizeof(game_save_head_t));

    close(fd);
    g_free(head);

    return TRUE;
}

game *game_load(char *filename)
{
    assert(filename != NULL);

    return EXIT_SUCCESS;
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
    score->level = g->p->lvl;
    score->level_max = g->p->stats.max_level;
    score->dlevel = g->p->pos.z;
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

map *game_map(game *g, guint nmap)
{
    assert (g != NULL && nmap >= 0 && nmap < MAP_MAX);

    return g->maps[nmap];
}

void game_spin_the_wheel(game *g, guint times)
{
    guint turn;
    damage *dam;

    assert(g != NULL && times > 0);

    map_timer(game_map(nlarn,g->p->pos.z), times);

    for (turn = 0; turn < times; turn++)
    {
        player_regenerate(g->p);
        player_effects_expire(g->p, 1);

        /* check if player is stuck inside a wall without walk through wall */
        if ((map_tiletype_at(game_map(nlarn,g->p->pos.z), g->p->pos) == LT_WALL)
                && !player_effect(g->p, ET_WALL_WALK))
        {
            player_die(g->p, PD_STUCK, 0);
        }

        /* deal damage cause by map tiles to player */
        if ((dam = map_tile_damage(game_map(nlarn,g->p->pos.z), g->p->pos)))
        {
            player_damage_take(g->p, dam, PD_LEVEL,
                               map_tiletype_at(game_map(nlarn,g->p->pos.z), g->p->pos));
        }

        game_move_monsters(g);
        game_move_spheres(g);

        g->gtime++; /* count up the time  */
        log_set_time(g->p->log, g->gtime); /* adjust time for log entries */
    }
}

void game_inventory_register(game *g, inventory *inv)
{
    assert (g != NULL && inv != NULL);

    g_hash_table_insert(g->inventories, inv, inv);
}

void game_inventory_unregister(game *g, inventory *inv)
{
    assert (g != NULL && inv != NULL);

    g_hash_table_remove(g->inventories, inv);
}

void game_item_register(game *g, item *it)
{
    assert (g != NULL && it != NULL);

    g_hash_table_insert(g->items, it, it);
}

void game_item_unregister(game *g, item *it)
{
    assert (g != NULL && it != NULL);

    g_hash_table_remove(g->items, it);
}

void game_effect_register(game *g, effect *e)
{
    assert (g != NULL && e != NULL);

    g_hash_table_insert(g->effects, e, e);
}

void game_effect_unregister(game *g, effect *e)
{
    assert (g != NULL && e != NULL);

    g_hash_table_remove(g->effects, e);
}

void game_monster_register(game *g, monster *m)
{
    assert (g != NULL && m != NULL);

    g_hash_table_insert(g->monsters, m, m);
}

void game_monster_unregister(game *g, monster *m)
{
    assert (g != NULL && m != NULL);

    g_hash_table_remove(g->monsters, m);
}

void game_sphere_register(game *g, sphere *s)
{
    assert (g != NULL && s != NULL);

    g_hash_table_insert(g->spheres, s, s);
}

void game_sphere_unregister(game *g, sphere *s)
{
    assert (g != NULL && s != NULL);

    g_hash_table_remove(g->spheres, s);
}

/**
 *  move all monsters on player's current map
 *
 *  @param the game
 *  @return Returns no value.
 */
static void game_move_monsters(game *g)
{
    map *l;

    /* list of monsters */
    GList *monsters;

    /* handle to current monster */
    monster *m;


    assert(g != NULL);

    /* make shortcut */
    l = game_map(nlarn,g->p->pos.z);

    monsters = g_hash_table_get_values(g->monsters);

    do
    {
        m = (monster *)monsters->data;

        position mpos = monster_pos(m);

        /* modify effects */
        monster_effect_expire(m, g->p->log);

        if (mpos.z == g->p->pos.z)
        {
            monster_move(m, g->p);
        }
    }
    while ((monsters = monsters->next));

    g_list_free(monsters);
}

static void game_move_spheres(game *g)
{
    map *l;
    sphere *s;
    guint idx;

    assert(g != NULL);

    /* make shortcut */
    l = game_map(nlarn,g->p->pos.z);

    for (idx = 0; idx < l->slist->len; idx++)
    {
        s = g_ptr_array_index(l->slist, idx);
        sphere_move(s, l);
    }
}

static void game_items_shuffle(game *g)
{
    shuffle(g->amulet_material_mapping, AM_MAX - 1, 0);
    shuffle(g->potion_desc_mapping, PO_MAX - 1, 1);
    shuffle(g->ring_material_mapping, RT_MAX - 1, 0);
    shuffle(g->scroll_desc_mapping, ST_MAX - 1, 1);
    shuffle(g->book_desc_mapping, SP_MAX - 1, 0);
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
