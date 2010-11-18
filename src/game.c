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
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <glib/gstdio.h>

#ifdef __unix
#include <sys/stat.h>
#endif

#include "cJSON.h"
#include "defines.h"
#include "display.h"
#include "game.h"
#include "lua_wrappers.h"
#include "nlarn.h"
#include "player.h"
#include "spheres.h"
#include "utils.h"

static void game_new();
static gboolean game_load(gchar *filename);
static void game_init_lua(game *g);

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
    log_add_entry(nlarn->log, "Welcome %sto NLarn %d.%d.%d%s!",
                  newgame ? "" : "back ", VERSION_MAJOR, VERSION_MINOR,
                  VERSION_PATCH, SVNID);
    log_add_entry(nlarn->log, "For a list of commands, press '?'.");
}

void game_init(int argc, char *argv[])
{
    const char *default_lib_dir = "/usr/share/games/nlarn";

    int idx;
    item_t it;

    /* these will be filled by the command line parser */
    static gint difficulty = 0;
    static gboolean wizard = FALSE;
    static gboolean no_autosave = FALSE;
    static char *name = NULL;
    static char *gender = NULL;
    static char *auto_pickup = NULL;
    static char *savefile = NULL;
    static char *stats = NULL;

    /* allocate space for game structure */
    nlarn = g_malloc0(sizeof(game));


    /* determine paths and file names */
    /* base directory for a local install */
    nlarn->basedir = g_path_get_dirname(argv[0]);

    /* try to use the directory below the binary's location first */
    nlarn->libdir = g_build_path(G_DIR_SEPARATOR_S, nlarn->basedir, "lib", NULL);

    if (!g_file_test(nlarn->libdir, G_FILE_TEST_IS_DIR))
    {
        /* local lib directory could not be found,
           try the system wide directory */
        if (g_file_test(default_lib_dir, G_FILE_TEST_IS_DIR))
        {
            /* system-wide data directory exists */
            /* string has to be dup'd as it is feed in the end */
            nlarn->libdir = g_strdup((char *)default_lib_dir);
        }
        else
        {
            g_printerr("Could not find game library directory.\n");
            g_printerr("Please reinstall the game.\n");

            exit(EXIT_FAILURE);
        }
    }

    nlarn->mesgfile = g_build_filename(nlarn->libdir, mesgfile, NULL);
    nlarn->helpfile = g_build_filename(nlarn->libdir, helpfile, NULL);
    nlarn->mazefile = g_build_filename(nlarn->libdir, mazefile, NULL);
    nlarn->fortunes = g_build_filename(nlarn->libdir, fortunes, NULL);
    nlarn->highscores = g_build_filename(nlarn->libdir, highscores, NULL);

    /* initialize the lua interpreter */
    game_init_lua(nlarn);


    /* ini file handling */
    GKeyFile *ini_file = g_key_file_new();
    GError *error = NULL;

    /* determine location of the configuration file */
    gchar *filename = g_build_path(G_DIR_SEPARATOR_S, game_userdir(),
                                   "nlarn.ini", NULL);

    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
    {
        /* ini file has not been found in user config directory */
        g_free(filename);

        /* try to find it in the binarys directory */
        filename = g_build_path(G_DIR_SEPARATOR_S, nlarn->basedir, "nlarn.ini", NULL);
    }

    /* try to load settings from the configuration file */
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

        no_autosave = g_key_file_get_boolean(ini_file, "nlarn", "no-autosave", &error);
        g_clear_error(&error);

        name = g_key_file_get_string(ini_file, "nlarn", "name", &error);
        g_clear_error(&error);

        gender = g_key_file_get_string(ini_file, "nlarn", "gender", &error);
        g_clear_error(&error);

        auto_pickup = g_key_file_get_string(ini_file, "nlarn", "auto-pickup", &error);
        g_clear_error(&error);

        stats = g_key_file_get_string(ini_file, "nlarn", "stats", &error);
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
        { "name",        'n', 0, G_OPTION_ARG_STRING, &name,        "Set character's name", NULL },
        { "gender",      'g', 0, G_OPTION_ARG_STRING, &gender,      "Set character's gender (m/f)", NULL },
        { "stats",       's', 0, G_OPTION_ARG_STRING, &stats,       "Set character's stats (a-f)", NULL },
        { "auto-pickup", 'a', 0, G_OPTION_ARG_STRING, &auto_pickup, "Item types to pick up automatically, e.g. '$*+'", NULL },
        { "difficulty",  'd', 0, G_OPTION_ARG_INT,    &difficulty,  "Set difficulty",       NULL },
        { "no-autosave", 'N', 0, G_OPTION_ARG_NONE,   &no_autosave, "Disable autosave",   NULL },
        { "wizard",      'w', 0, G_OPTION_ARG_NONE,   &wizard,      "Enable wizard mode",   NULL },
#ifdef DEBUG
        { "savefile",    'f', 0, G_OPTION_ARG_FILENAME, &savefile,  "Save file to restore", NULL },
#endif
        { NULL }
    };

    GOptionContext *context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_printerr("option parsing failed: %s\n", error->message);

        exit (EXIT_FAILURE);
    }
    g_option_context_free(context);

    /* initialise the display - must not happen before this point
       otherwise displaying the command line help fails */
    display_init();

    /* set autosave setting (default: TRUE) */
    game_autosave(nlarn) = !no_autosave;

    if (!game_load(savefile))
    {
        /* set game parameters */
        game_difficulty(nlarn) = difficulty;
        game_wizardmode(nlarn) = wizard;

        /* restoring a save game failed - start a new game. */
        game_new();

        /* put the player into the town */
        player_map_enter(nlarn->p, game_map(nlarn, 0), FALSE);

        /* give player knowledge of the town */
        scroll_mapping(nlarn->p, NULL);

        if (name)
        {
            nlarn->p->name = name;
        }

        if (gender)
        {
            gender[0] = g_ascii_tolower(gender[0]);

            switch (gender[0])
            {
            case 'm':
                nlarn->p->sex = PS_MALE;
                break;

            case 'f':
                nlarn->p->sex = PS_FEMALE;
                break;

            default:
                nlarn->p->sex = PS_NONE;
                break;
            }
        }

        if (stats)
        {
            stats[0] = g_ascii_tolower(stats[0]);
            nlarn-> player_stats_set = player_assign_bonus_stats(nlarn->p, stats);
        }


        if (wizard)
        {
            log_add_entry(nlarn->log, "Wizard mode has been activated.");
        }

        /* parse autopickup settings */
        if (auto_pickup && (nlarn->p != NULL))
        {
            for (idx = 0; idx < strlen(auto_pickup); idx++)
            {
                for (it = IT_NONE; it < IT_MAX; it++)
                {
                    if (auto_pickup[idx] == item_glyph(it))
                    {
                        nlarn->p->settings.auto_pickup[it] = TRUE;
                    }
                }
            }
        } /* end autopickup */

    } /* end new game only settings */
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
    log_destroy(g->log);

    if (g->store_stock)
        inv_destroy(g->store_stock, FALSE);

    if (g->monastery_stock)
        inv_destroy(g->monastery_stock, FALSE);

    g_hash_table_destroy(g->items);
    g_hash_table_destroy(g->effects);
    g_hash_table_destroy(g->monsters);
    g_ptr_array_free(g->dead_monsters, TRUE);

    g_ptr_array_foreach(g->spheres, (GFunc)sphere_destroy, g);
    g_ptr_array_free(g->spheres, TRUE);

    /* terminate the Lua interpreter */
    lua_close(g->L);

    g_free(g);

    return EXIT_SUCCESS;
}

const gchar *game_userdir()
{
    static gchar *userdir = NULL;

    if (userdir == NULL)
    {
#ifdef WIN32
        userdir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(),
                               "nlarn", NULL);
#else
        userdir = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(),
                               ".nlarn", NULL);
#endif
    }

    return userdir;
}

int game_save(game *g, const char *filename)
{
    int idx, err;
    struct cJSON *save, *obj;
    char *fullname = NULL;
    display_window *win = NULL;

    assert(g != NULL);

    /* if the display has been initialised, show a popup message */
    if (display_available())
        win = display_popup(2, 2, 0, NULL, "Saving....");

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
                          cJSON_CreateIntArray(g->book_desc_mapping, SP_MAX_BOOK - 1));

    cJSON_AddItemToObject(save, "monster_genocided",
                          cJSON_CreateIntArray(g->monster_genocided, MT_MAX));

    if (g->wizard) cJSON_AddTrueToObject(save, "wizard");
    if (g->fullvis) cJSON_AddTrueToObject(save, "fullvis");

    /* store stock */
    if (inv_length(g->store_stock) > 0)
    {
        cJSON_AddItemToObject(save, "store_stock", inv_serialize(g->store_stock));
    }

    /* monastery stock */
    if (inv_length(g->monastery_stock) > 0)
    {
        cJSON_AddItemToObject(save, "monastery_stock", inv_serialize(g->monastery_stock));
    }

    /* storage at player's home */
    if (inv_length(g->player_home) > 0)
    {
        cJSON_AddItemToObject(save, "player_home", inv_serialize(g->player_home));
    }

    /* log */
    cJSON_AddItemToObject(save, "log", log_serialize(g->log));

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
    if (!g_file_test(game_userdir(), G_FILE_TEST_IS_DIR))
    {
        /* directory is missing -> create it */
        int ret = g_mkdir(game_userdir(), 0755);

        if (ret == -1)
        {
            /* creating the directory failed */
            log_add_entry(g->log, "Failed to create directory %s.", game_userdir());
            free(sg);

            return FALSE;
        }
    }

    /* assemble save file name */
    fullname = g_build_path(G_DIR_SEPARATOR_S, game_userdir(), filename ? filename : "nlarn.sav", NULL);

    /* open save file for writing */
    gzFile file = gzopen(fullname, "wb");

    if (file == NULL)
    {
        log_add_entry(g->log, "Error opening save file \"%s\".", fullname);
        free(sg);
        return FALSE;
    }

    if (gzputs(file, sg) != strlen(sg))
    {
        log_add_entry(g->log, "Error writing save file \"%s\": %s",
                      fullname, gzerror(file, &err));

        free(sg);
        return FALSE;
    }

    /* free the memory acquired by g_strdup_printf */
    g_free(fullname);

    free(sg);
    gzclose(file);

    /* if a popup message has been opened, destroy it here */
    if (win != NULL)
        display_window_destroy(win);

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

    /* check if the player is on a deep water tile without levitation */
    if ((map_tiletype_at(map, g->p->pos) == LT_DEEPWATER)
            && !player_effect(g->p, ET_LEVITATION))
    {
        player_die(g->p, PD_DROWNED, 0);
    }

    /* check if the player is on a lava tile without levitation */
    if ((map_tiletype_at(map, g->p->pos) == LT_LAVA)
            && !player_effect(g->p, ET_LEVITATION))
    {
        player_die(g->p, PD_MELTED, 0);
    }

    /* deal damage cause by map tiles to player */
    damage *dam = map_tile_damage(map, g->p->pos,
                                  player_effect(g->p, ET_LEVITATION));

    if (dam != NULL)
        player_damage_take(g->p, dam, PD_MAP, map_tiletype_at(map, g->p->pos));

    /* move all monsters */
    g_hash_table_foreach(g->monsters, (GHFunc)monster_move, g);

    /* destroy all monsters that have been killed during this turn */
    game_remove_dead_monsters(g);

    /* move all spheres */
    g_ptr_array_foreach(g->spheres, (GFunc)sphere_move, g);

    g->gtime++; /* count up the time  */
    log_set_time(g->log, g->gtime); /* adjust time for log entries */
}

void game_remove_dead_monsters(game *g)
{
    assert (g != NULL);

    monster *m;
    while (g->dead_monsters->len > 0)
    {
        m = g_ptr_array_index(g->dead_monsters, g->dead_monsters->len - 1);

        monster_destroy(m);
        g_ptr_array_remove_index(g->dead_monsters, g->dead_monsters->len - 1);
    }
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

static void game_new()
{
    size_t idx;

    /* initialize object hashes (here as they will be needed by player_new) */
    nlarn->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->monsters = g_hash_table_new(&g_direct_hash, &g_direct_equal);

    /* initialize the array to store monsters that died during the turn */
    nlarn->dead_monsters = g_ptr_array_new();

    nlarn->spheres = g_ptr_array_new();

    /* generate player */
    nlarn->p = player_new();

    /* randomize unidentified item descriptions */
    game_items_shuffle(nlarn);

    /* fill the store */
    building_dndstore_init();

    /* initialize the monastery */
    building_monastery_init();

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

    /* start a new diary */
    nlarn->log = log_new();

    /* welcome message */
    print_welcome_message(TRUE);

    log_set_time(nlarn->log, nlarn->gtime);
}

static gboolean game_load(gchar *filename)
{
    int size, idx;
    cJSON *save, *obj;
    char *fullname = NULL;
    display_window *win = NULL;

    /* size of the buffer we allocate to store the uncompressed file content */
    const int bufsize = 1024 * 1024 * 3;

    /* assemble save file name; if no filename has been supplied, default
       to "nlarn.sav" */
    fullname = g_build_path(G_DIR_SEPARATOR_S, game_userdir(),
                            filename ? filename : "nlarn.sav", NULL);

    /* try to open save file */
    gzFile file = gzopen(fullname, "rb");

    if (file == NULL)
    {
        /* failed to open save game file */
        g_free(fullname);

        return FALSE;
    }

    /* if the display has been initialised, show a popup message */
    if (display_available())
        win = display_popup(2, 2, 0, NULL, "Loading....");

    /* temporary buffer to store uncompressed save file content */
    char *sgbuf = g_malloc0(bufsize);

    if (!gzread(file, sgbuf, bufsize))
    {
        /* Reading the file failed. Terminate the game with an error message */
        display_shutdown();
        g_printerr("Failed to restore save file \"%s\".\n", fullname);
        g_free(fullname);

        exit(EXIT_FAILURE);
    }

    /* close save file */
    gzclose(file);

    /* parse save file */
    save = cJSON_Parse(sgbuf);

    /* throw away the buffer */
    g_free(sgbuf);

    /* check for save file incompatibility */
    gboolean compatible_version = FALSE;
    if (cJSON_GetObjectItem(save, "nlarn_version"))
    {
        nlarn->version = cJSON_GetObjectItem(save, "nlarn_version")->valueint;

        if (nlarn->version == SAVEFILE_VERSION)
            compatible_version = TRUE;
    }

    /* handle incompatible save file */
    if (!compatible_version)
    {
        /* free the memory allocated by loading the save file */
        cJSON_Delete(save);

        /* if a popup message has been opened, destroy it here */
        if (win != NULL)
            display_window_destroy(win);

        /* offer to delete the incompatible save game */
        if (display_get_yesno("Saved game could not be loaded. " \
                              "Delete and start new game?",
                              NULL, NULL))
        {
            /* delete save file */
            g_unlink(fullname);
            g_free(fullname);
        }
        else
        {
            display_shutdown();
            g_printerr("Save file \"%s\" is not compatible to current version.\n", fullname);
            g_free(fullname);

            exit(EXIT_FAILURE);
        }

        return FALSE;
    }

    /* restore saved game */
    nlarn->time_start = cJSON_GetObjectItem(save, "time_start")->valueint;
    nlarn->gtime = cJSON_GetObjectItem(save, "gtime")->valueint;
    nlarn->difficulty = cJSON_GetObjectItem(save, "difficulty")->valueint;

    if (cJSON_GetObjectItem(save, "wizard"))
        nlarn->wizard = TRUE;

    if (cJSON_GetObjectItem(save, "fullvis"))
        nlarn->fullvis = TRUE;

    obj = cJSON_GetObjectItem(save, "amulet_created");
    size = cJSON_GetArraySize(obj);
    assert(size == AM_MAX);
    for (idx = 0; idx < size; idx++)
        nlarn->amulet_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "weapon_created");
    size = cJSON_GetArraySize(obj);
    assert(size == WT_MAX);
    for (idx = 0; idx < size; idx++)
        nlarn->weapon_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    if (cJSON_GetObjectItem(save, "cure_dianthr_created"))
        nlarn->cure_dianthr_created = TRUE;


    obj = cJSON_GetObjectItem(save, "amulet_material_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == AM_MAX - 1);
    for (idx = 0; idx < size; idx++)
        nlarn->amulet_material_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "potion_desc_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == PO_MAX - 1);
    for (idx = 0; idx < size; idx++)
        nlarn->potion_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "ring_material_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == RT_MAX - 1);
    for (idx = 0; idx < size; idx++)
        nlarn->ring_material_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "scroll_desc_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == ST_MAX - 1);
    for (idx = 0; idx < size; idx++)
        nlarn->scroll_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "book_desc_mapping");
    size = cJSON_GetArraySize(obj);
    assert(size == SP_MAX_BOOK - 1);
    for (idx = 0; idx < size; idx++)
        nlarn->book_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "monster_genocided");
    size = cJSON_GetArraySize(obj);
    assert(size == MT_MAX);
    for (idx = 0; idx < size; idx++)
        nlarn->monster_genocided[idx] = cJSON_GetArrayItem(obj, idx)->valueint;


    /* restore effects (have to come first) */
    nlarn->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "effects");

    for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        effect_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);


    /* restore items */
    nlarn->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "items");
    for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        item_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);


    /* restore maps */
    obj = cJSON_GetObjectItem(save, "maps");
    size = cJSON_GetArraySize(obj);
    assert(size == MAP_MAX);
    for (idx = 0; idx < size; idx++)
        nlarn->maps[idx] = map_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);


    /* restore dnd store stock */
    obj = cJSON_GetObjectItem(save, "store_stock");
    if (obj != NULL) nlarn->store_stock = inv_deserialize(obj);

    /* restore monastery stock */
    obj = cJSON_GetObjectItem(save, "monastery_stock");
    if (obj != NULL) nlarn->monastery_stock = inv_deserialize(obj);

    /* restore storage of player's home */
    obj = cJSON_GetObjectItem(save, "player_home");
    if (obj != NULL) nlarn->player_home = inv_deserialize(obj);

    /* restore log */
    nlarn->log = log_deserialize(cJSON_GetObjectItem(save, "log"));


    /* restore player */
    nlarn->p = player_deserialize(cJSON_GetObjectItem(save, "player"));


    /* restore monsters */
    nlarn->monsters = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "monsters");

    for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        monster_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);

    /* initialize the array to store monsters that died during the turn */
    nlarn->dead_monsters = g_ptr_array_new();


    /* restore spheres */
    nlarn->spheres = g_ptr_array_new();
    obj = cJSON_GetObjectItem(save, "spheres");

    if ((obj = cJSON_GetObjectItem(save, "spheres")))
    {
        for (idx = 0; idx < cJSON_GetArraySize(obj); idx++)
            sphere_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);
    }

    /* free parsed save game */
    cJSON_Delete(save);

    /* set log turn number to current game turn number */
    log_set_time(nlarn->log, nlarn->gtime);

    /* welcome message */
    print_welcome_message(FALSE);

    /* free memory used by the full file name */
    g_free(fullname);

    /* refresh FOV */
    player_update_fov(nlarn->p);

    /* no need to define the player's stats */
    nlarn->player_stats_set = TRUE;

    /* if a popup message has been opened, destroy it here */
    if (win != NULL)
        display_window_destroy(win);

    return TRUE;
}

static void game_init_lua(game *g)
{
    assert (g != NULL);

    /* initialize Lua interpreter */
    nlarn->L = luaL_newstate();

    /* open Lua libraries */
    luaL_openlibs(nlarn->L);

    /* register all required functions */
    wrap_display(g->L);
    wrap_effects(g->L);
    wrap_utils(g->L);
    wrap_monsters(g->L);
}

static void game_items_shuffle(game *g)
{
    shuffle(g->amulet_material_mapping, AM_MAX - 1, 0);
    shuffle(g->potion_desc_mapping, PO_MAX - 1, 1);
    shuffle(g->ring_material_mapping, RT_MAX - 1, 0);
    shuffle(g->scroll_desc_mapping, ST_MAX - 1, 1);
    shuffle(g->book_desc_mapping, SP_MAX_BOOK - 1, 0);
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

void game_delete_savefile()
{
    char *fullname = NULL;

    /* assemble save file name */
    fullname = g_build_path(G_DIR_SEPARATOR_S, game_userdir(),
                            "nlarn.sav", NULL);

    gzFile file = gzopen(fullname, "rb");
    if (file != NULL)
    {
        gzclose(file);
        /* actually delete the file */
        g_unlink(fullname);
    }
    g_free(fullname);
}
