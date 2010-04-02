/*
 * game.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <glib/gstdio.h>

#ifdef __unix
#include <sys/stat.h>
#endif

#include "cJSON.h"
#include "defines.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "spheres.h"
#include "utils.h"

static void game_initialize_settings(game *g, int argc, char *argv[], gboolean new_game);

static void game_monsters_move(game *g);

static void game_items_shuffle(game *g);

static GList *game_scores_load(game *g);
static void game_scores_save(game *g, GList *scores);
static int game_score_compare(const void *scr_a, const void *scr_b);

static const char *mesgfile = "nlarn.msg";
static const char *helpfile = "nlarn.hlp";
static const char *mazefile = "maze";
static const char *fortunes = "fortune";
static const char *highscores = "highscores";

static void print_welcome_message(gboolean newgame)
{
    log_add_entry(nlarn->p->log, "Welcome %sto NLarn %d.%d.%d!",
                  newgame ? "" : "back ",
                  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    log_add_entry(nlarn->p->log, "For a list of commands, press '?'.");
}

void game_new(int argc, char *argv[])
{
    size_t idx;

    /* one game, please */
    nlarn = g_malloc0(sizeof(game));

    /* initialize object hashes (here as they will be needed by player_new) */
    nlarn->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->monsters = g_hash_table_new(&g_direct_hash, &g_direct_equal);

    nlarn->spheres = g_ptr_array_new();

    /* generate player */
    nlarn->p = player_new();

    /* initialize game settings */
    game_initialize_settings(nlarn, argc, argv, TRUE);

    /* randomize unidentified item descriptions */
    game_items_shuffle(nlarn);

    /* fill the store */
    building_dndstore_init();

    /* generate levels */
    for (idx = 0; idx < MAP_MAX; idx++)
    {
        /* if map_new fails, it returns NULL.
           loop while no map has been generated */
        do
        {
            nlarn->maps[idx] = map_new(idx, nlarn->mazefile);
        }
        while (nlarn->maps[idx] == NULL);
    }

    /* game time handling */
    nlarn->gtime = 1;
    nlarn->time_start = time(NULL);
    nlarn->version = SAVEFILE_VERSION;

    /* welcome message */
    print_welcome_message(TRUE);

    log_set_time(nlarn->p->log, nlarn->gtime);
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

    g_free(g->userdir);

    g_free(g->basedir);
    g_free(g->libdir);

    g_free(g->mesgfile);
    g_free(g->helpfile);
    g_free(g->mazefile);
    g_free(g->fortunes);
    g_free(g->highscores);

    player_destroy(g->p);

    inv_destroy(g->store_stock, FALSE);

    g_hash_table_destroy(g->items);
    g_hash_table_destroy(g->effects);
    g_hash_table_destroy(g->monsters);

    g_ptr_array_foreach(g->spheres, (GFunc)sphere_destroy, g);
    g_ptr_array_free(g->spheres, TRUE);

    g_free(g);

    return EXIT_SUCCESS;
}

gchar *game_userdir()
{
#ifdef WIN32
    return g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(),
                        "nlarn", NULL);
#else
    return g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(),
                        ".nlarn", NULL);
#endif
}

int game_save(game *g, const char *filename)
{
    int idx, err;
    struct cJSON *save, *obj;

    assert(g != NULL && filename != NULL);

    save = cJSON_CreateObject();

    cJSON_AddNumberToObject(save, "nlarn_version", g->version);
    cJSON_AddNumberToObject(save, "time_start", g->time_start);
    cJSON_AddNumberToObject(save, "gtime", g->gtime);
    cJSON_AddNumberToObject(save, "difficulty", g->difficulty);

    /* maps */
    cJSON_AddItemToObject(save, "maps", obj = cJSON_CreateArray());
    for (idx = 0; idx < MAP_MAX; idx++)
    {
        cJSON_AddItemToArray(obj, map_serialize(g->maps[idx]));
    }

    cJSON_AddItemToObject(save, "amulet_created",
                          cJSON_CreateIntArray(g->amulet_created, AM_MAX));

    cJSON_AddItemToObject(save, "weapon_created",
                          cJSON_CreateIntArray(g->weapon_created, WT_MAX));

    if (g->cure_dianthr_created) cJSON_AddTrueToObject(save, "cure_dianthr_created");

    cJSON_AddItemToObject(save, "amulet_material_mapping",
                          cJSON_CreateIntArray(g->amulet_material_mapping, AM_MAX - 1));

    cJSON_AddItemToObject(save, "potion_desc_mapping",
                          cJSON_CreateIntArray(g->potion_desc_mapping, PO_MAX - 1));

    cJSON_AddItemToObject(save, "ring_material_mapping",
                          cJSON_CreateIntArray(g->ring_material_mapping, RT_MAX - 1));

    cJSON_AddItemToObject(save, "scroll_desc_mapping",
                          cJSON_CreateIntArray(g->scroll_desc_mapping, ST_MAX - 1));

    cJSON_AddItemToObject(save, "book_desc_mapping",
                          cJSON_CreateIntArray(g->book_desc_mapping, SP_MAX - 1));

    cJSON_AddItemToObject(save, "monster_genocided",
                          cJSON_CreateIntArray(g->monster_genocided, MT_MAX));

    if (g->wizard) cJSON_AddTrueToObject(save, "wizard");

    /* store stock */
    if (inv_length(g->store_stock) > 0)
    {
        cJSON_AddItemToObject(save, "store_stock", inv_serialize(g->store_stock));
    }

    /* add player */
    cJSON_AddItemToObject(save, "player",  player_serialize(g->p));

    /* add items */
    cJSON_AddItemToObject(save, "items", obj = cJSON_CreateArray());
    g_hash_table_foreach(g->items, item_serialize, obj);

    /* add effects */
    cJSON_AddItemToObject(save, "effects", obj = cJSON_CreateArray());
    g_hash_table_foreach(g->effects, (GHFunc)effect_serialize, obj);

    /* add monsters */
    cJSON_AddItemToObject(save, "monsters", obj = cJSON_CreateArray());
    g_hash_table_foreach(g->monsters, (GHFunc)monster_serialize, obj);

    /* add spheres */
    if (g->spheres->len > 0)
    {
        cJSON_AddItemToObject(save, "spheres", obj = cJSON_CreateArray());
        g_ptr_array_foreach(g->spheres, (GFunc)sphere_serialize, obj);
    }

    /* print save game into a string */
    char *sg = cJSON_Print(save);

    /* free memory claimed by json structures */
    cJSON_Delete(save);

    /* verify that user directory exists */
    if (!g_file_test(g->userdir, G_FILE_TEST_IS_DIR))
    {
        /* directory is missing -> create it */
        int ret = g_mkdir(g->userdir, 0755);

        if (ret == -1)
        {
            /* creating the directory failed */
            log_add_entry(g->p->log, "Failed to create directory %s.", g->userdir);
            free(sg);

            return FALSE;
        }
    }

    /* open save file for writing */
    gzFile file = gzopen(filename, "wb");

    if (file == NULL)
    {
        log_add_entry(g->p->log, "Error opening save file.");
        free(sg);
        return FALSE;
    }

    if (gzputs(file, sg) != strlen(sg))
    {
        log_add_entry(g->p->log, "Error writing save file: %s",
                      gzerror(file, &err));

        free(sg);
        return FALSE;
    }

    free(sg);
    gzclose(file);

    return TRUE;
}

gboolean game_load(const char *filename, int argc, char *argv[])
{
    int size, idx;
    game *g;
    cJSON *save, *obj;

    const int bufsize = 1024 * 1024 * 3;

    assert(filename != NULL);

    /* open save file */
    gzFile file = gzopen(filename, "rb");

    if (file == NULL)
    {
        /* failed to open save game file */
        return FALSE;
    }

    /* temporary buffer to store uncompressed save file content */
    char *sgbuf = g_malloc0(bufsize);

    if (!gzread(file, sgbuf, bufsize))
    {
        g_printerr("Failed to restore save file.");
        exit(EXIT_FAILURE);
    }

    /* close save file */
    gzclose(file);

    /* parse save file */
    save = cJSON_Parse(sgbuf);

    /* throw away the buffer */
    g_free(sgbuf);

    /* allocate space for game structure */
    nlarn = g = g_malloc0(sizeof(game));

    /* check for save file incompatibility */
    gboolean compatible_version = FALSE;
    if (cJSON_GetObjectItem(save, "nlarn_version"))
    {
        g->version = cJSON_GetObjectItem(save, "nlarn_version")->valueint;

        if (g->version == SAVEFILE_VERSION)
            compatible_version = TRUE;
    }
    if (!compatible_version)
    {
        cJSON_Delete(save);
        return FALSE;
    }

    /* restore saved game */
    g->time_start = cJSON_GetObjectItem(save, "time_start")->valueint;
    g->gtime = cJSON_GetObjectItem(save, "gtime")->valueint;
    g->difficulty = cJSON_GetObjectItem(save, "difficulty")->valueint;

    if (cJSON_GetObjectItem(save, "wizard"))
        g->wizard = TRUE;

    obj = cJSON_GetObjectItem(save, "amulet_created");
    size = cJSON_GetArraySize(obj);
    assert(size = AM_MAX);
    for (idx = 0; idx < size; idx++)
        g->amulet_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "weapon_created");
    size = cJSON_GetArraySize(obj);
    assert(size = WT_MAX);
    for (idx = 0; idx < size; idx++)
        g->weapon_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    if (cJSON_GetObjectItem(save, "cure_dianthr_created"))
        g->cure_dianthr_created = TRUE;


    obj = cJSON_GetObjectItem(save, "amulet_material_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size = AM_MAX - 1);
    for (idx = 0; idx < size; idx++)
        g->amulet_material_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "potion_desc_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size = PO_MAX - 1);
    for (idx = 0; idx < size; idx++)
        g->potion_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "ring_material_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size = RT_MAX - 1);
    for (idx = 0; idx < size; idx++)
        g->ring_material_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "scroll_desc_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == ST_MAX - 1);
    for (idx = 0; idx < size; idx++)
        g->scroll_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "book_desc_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == SP_MAX - 1);
    for (idx = 0; idx < size; idx++)
        g->book_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "monster_genocided");
    size = cJSON_GetArraySize(obj);
    assert(size == MT_MAX);
    for (idx = 0; idx < size; idx++)
        g->monster_genocided[idx] = cJSON_GetArrayItem(obj, idx)->valueint;


    /* restore effects (have to come first) */
    g->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "effects");

    for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        effect_deserialize(cJSON_GetArrayItem(obj, idx), g);


    /* restore items */
    g->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "items");
    for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        item_deserialize(cJSON_GetArrayItem(obj, idx), g);


    /* restore maps */
    obj = cJSON_GetObjectItem(save, "maps");
    size = cJSON_GetArraySize(obj);
    assert(size == MAP_MAX);
    for (idx = 0; idx < size; idx++)
        g->maps[idx] = map_deserialize(cJSON_GetArrayItem(obj, idx), g);


    /* store dnd store stock */
    obj = cJSON_GetObjectItem(save, "store_stock");
    if (obj != NULL) g->store_stock = inv_deserialize(obj);


    /* restore player */
    g->p = player_deserialize(cJSON_GetObjectItem(save, "player"));


    /* restore monsters */
    g->monsters = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "monsters");

    for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        monster_deserialize(cJSON_GetArrayItem(obj, idx), g);


    /* restore spheres */
    g->spheres = g_ptr_array_new();
    obj = cJSON_GetObjectItem(save, "spheres");

    if ((obj = cJSON_GetObjectItem(save, "spheres")))
    {
        for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
            sphere_deserialize(cJSON_GetArrayItem(obj, idx), g);
    }

    /* free parsed save game */
    cJSON_Delete(save);

    /* initialize settings */
    game_initialize_settings(g, argc, argv, FALSE);

    log_set_time(nlarn->p->log, g->gtime);

    /* welcome message */
    print_welcome_message(FALSE);

    return TRUE;
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
    score->level = g->p->level;
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
    GList *gs;

    assert (g != NULL && score != NULL);

    gs = game_scores_load(g);

    /* add new score */
    gs = g_list_append(gs, score);

    /* sort scoreboard entries */
    gs = g_list_sort(gs, (GCompareFunc)game_score_compare);

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

void game_spin_the_wheel(game *g)
{
    int nmap;
    map *map;

    assert(g != NULL);

    /* add the player's speed to the player's movement points */
    nlarn->p->movement += player_get_speed(nlarn->p);

    /* per-map actions */
    for (nmap = 0; nmap < MAP_MAX; nmap++)
    {
        map = game_map(g, nmap);

        /* call map timers */
        map_timer(map);

        /* spawn some monsters every now and then */
        if (nmap > 0 && (g->gtime % (100 + nmap)) == 0)
        {
            map_fill_with_life(map);
        }
    }

    map = game_map(nlarn, g->p->pos.z);

    /* check if player is stuck inside a wall without walk through wall */
    if ((map_tiletype_at(map, g->p->pos) == LT_WALL)
            && !player_effect(g->p, ET_WALL_WALK))
    {
        player_die(g->p, PD_STUCK, 0);
    }

    /* deal damage cause by map tiles to player */
    damage *dam  = map_tile_damage(map, g->p->pos);
    if (dam != NULL)
    {
        player_damage_take(g->p, dam, PD_MAP, map_tiletype_at(map, g->p->pos));
    }

    game_monsters_move(g);
    g_ptr_array_foreach(g->spheres, (GFunc)sphere_move, g);

    g->gtime++; /* count up the time  */
    log_set_time(g->p->log, g->gtime); /* adjust time for log entries */
}

gpointer game_item_register(game *g, item *it)
{
    assert (g != NULL && it != NULL);

    gpointer nkey = GUINT_TO_POINTER(++g->item_max_id);
    g_hash_table_insert(g->items, nkey, it);

    return nkey;
}

void game_item_unregister(game *g, gpointer it)
{
    assert (g != NULL && it != NULL);

    g_hash_table_remove(g->items, it);
}

item *game_item_get(game *g, gpointer id)
{
    assert(g != NULL && id != NULL);

    return (item *)g_hash_table_lookup(g->items, id);
}

gpointer game_effect_register(game *g, effect *e)
{
    assert (g != NULL && e != NULL);

    gpointer nkey = GUINT_TO_POINTER(++g->effect_max_id);
    g_hash_table_insert(g->effects, nkey, e);

    return nkey;
}

void game_effect_unregister(game *g, gpointer e)
{
    assert (g != NULL && e != NULL);

    g_hash_table_remove(g->effects, e);
}

effect *game_effect_get(game *g, gpointer id)
{
    assert(g != NULL && id != NULL);
    return (effect *)g_hash_table_lookup(g->effects, id);
}

gpointer game_monster_register(game *g, monster *m)
{
    assert (g != NULL && m != NULL);

    gpointer nkey = GUINT_TO_POINTER(++g->monster_max_id);
    g_hash_table_insert(g->monsters, nkey, m);

    return nkey;
}

void game_monster_unregister(game *g, gpointer m)
{
    assert (g != NULL && m != NULL);

    g_hash_table_remove(g->monsters, m);
}

monster *game_monster_get(game *g, gpointer id)
{
    assert(g != NULL && id != NULL);
    return (monster *)g_hash_table_lookup(g->monsters, id);
}

static void game_initialize_settings(game *g, int argc, char *argv[], gboolean new_game)
{
    const char *default_lib_dir = "/usr/share/games/nlarn";

    int idx;
    item_t it;

    /* these will be filled by the command line parser */
    static gint difficulty = 0;
    static gboolean wizard = FALSE;
    static char *name = NULL;
    static char *sex = NULL;
    static char *auto_pickup = NULL;

    /* define per-user dir */
    g->userdir = game_userdir();

    /* base directory for a local install */
    g->basedir = g_path_get_dirname(argv[0]);

    /* ini file handling */
    GKeyFile *ini_file = g_key_file_new();
    GError *error = NULL;

    /* this is the default path */
    gchar *filename = g_build_path(G_DIR_SEPARATOR_S, g->userdir,
                                   "nlarn.ini", NULL);

    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
    {
        /* ini file has not been found in user config directory */
        g_free(filename);

        /* try to find it in the binarys directory */
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

        sex = g_key_file_get_string(ini_file, "nlarn", "sex", &error);
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
        { "sex",         's', 0, G_OPTION_ARG_STRING, &sex,         "Set character's sex (m/f)", NULL },
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
    /* try to use the directory below the binary's location first */
    g->libdir = g_build_path(G_DIR_SEPARATOR_S, g->basedir, "lib", NULL);

    if (!g_file_test(g->libdir, G_FILE_TEST_IS_DIR))
    {
        /* local lib directory could not be found,
           try the system wide directory */
        if (g_file_test(default_lib_dir, G_FILE_TEST_IS_DIR))
        {
            /* system-wide data directory exists */
            /* string has to be dup'd as it is feed in the end */
            g->libdir = g_strdup((char *)default_lib_dir);
        }
        else
        {
            g_printerr("Could not find game library directory.\n");
            g_printerr("Please reinstall the game.\n");
            exit(EXIT_FAILURE);
        }
    }

    g->mesgfile = g_build_filename(g->libdir, mesgfile, NULL);
    g->helpfile = g_build_filename(g->libdir, helpfile, NULL);
    g->mazefile = g_build_filename(g->libdir, mazefile, NULL);
    g->fortunes = g_build_filename(g->libdir, fortunes, NULL);
    g->highscores = g_build_filename(g->libdir, highscores, NULL);

    if (new_game)
    {
        /* not restoring a save game */
        /* set game parameters */
        game_difficulty(g) = difficulty;
        game_wizardmode(g) = wizard;

        if (name)
        {
            g->p->name = name;
        }

        if (sex)
        {
            sex[0] = g_ascii_tolower(sex[0]);

            switch(sex[0])
            {
            case 'm':
                g->p->sex = PS_MALE;
                break;

            case 'f':
                g->p->sex = PS_FEMALE;
                break;

            default:
                g->p->sex = PS_NONE;
                break;
            }
        }

        if (wizard)
        {
            log_add_entry(g->p->log, "Wizard mode has been activated.");
        }
    } /* end new game only settings */

    /* parse autopickup settings */
    if (auto_pickup)
    {
        for (idx = 0; idx < strlen(auto_pickup); idx++)
        {
            for (it = IT_NONE; it < IT_MAX; it++)
            {
                if (auto_pickup[idx] == item_image(it))
                {
                    g->p->settings.auto_pickup[it] = TRUE;
                }
            }
        }
    } /* end autopickup */
}

/**
 *  move all monsters in the game
 *
 *  @param the game
 *  @return Returns no value.
 */
static void game_monsters_move(game *g)
{
    GList *monsters;    /* list of monsters */
    GList *iter;        /* iterator for monsters list */
    monster *m;         /* handle to current monster */

    assert(g != NULL);

    monsters = g_hash_table_get_values(g->monsters);

    for (iter = monsters; iter != NULL; iter = iter->next)
    {
        m = (monster *)iter->data;
        position mpos = monster_pos(m);

        /* modify effects */
        monster_effects_expire(m);

        /* regenerate / inflict poison upon monster. */
        if (!monster_regenerate(m, g->gtime, g->difficulty, g->p->log))
        {
            monster_die(m, NULL);
            continue;
        }

        /* damage caused by map effects */
        damage *dam = map_tile_damage(monster_map(m), monster_pos(m));

        /* deal damage caused by floor effects */
        if ((dam != NULL) && !(m = monster_damage_take(m, dam)))
        {
            continue;
        }

        /* move the monsters only if they are on the same
           level as the player or an adjacent level */
        if (mpos.z == g->p->pos.z
                || (mpos.z == g->p->pos.z - 1)
                || (mpos.z == g->p->pos.z + 1))
        {
            monster_move(m, g->p);
        }
    }

    g_list_free(monsters);
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

#ifdef __unix
    /* set file permissions */
    chmod(game_highscores(g), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
#endif
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
