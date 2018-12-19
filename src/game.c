/*
 * game.c
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

/* setres[gu]id() from unistd.h are only defined when this is set
   *before* including it. Another header in the list seems to do so
   before we include it here.*/
#ifdef __linux__
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
#endif

#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <glib/gstdio.h>
#include <sys/param.h>

#if (defined __unix) || (defined __unix__) || (defined __APPLE__)
# include <unistd.h>
# include <sys/file.h>
# include <sys/stat.h>
#endif

#ifdef WIN32
# include <io.h>
# include <sys/locking.h>
#endif

#include "cJSON.h"
#include "config.h"
#include "display.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "scoreboard.h"
#include "spheres.h"
#include "random.h"

static void game_new();
static gboolean game_load();
static void game_items_shuffle(game *g);

static const char *default_lib_dir = "/usr/share/nlarn";
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
static const char *default_var_dir = "/var/games/nlarn";
#endif
static const char *mesgfile = "nlarn.msg";
static const char *helpfile = "nlarn.hlp";
static const char *mazefile = "maze";
static const char *fortunes = "fortune";
static const char *highscores = "highscores";
static const char *config_file = "nlarn.ini";
static const char *save_file = "nlarn.sav";

/* file descriptor for locking the savegame file */
static int sgfd = 0;

static void print_welcome_message(gboolean newgame)
{
    log_add_entry(nlarn->log, "Welcome %sto NLarn %d.%d.%d%s!",
                  newgame ? "" : "back ", VERSION_MAJOR, VERSION_MINOR,
                  VERSION_PATCH, GITREV);
    log_add_entry(nlarn->log, "For a list of commands, press '?'.");
}

static int try_locking_savegame_file(FILE *sg)
{
    /*
     * get a copy of the file descriptor for locking - gzclose would close
     * it and thus unlock the file.
     */
    int fd = dup(fileno(sg));

    /* Try to obtain the lock on the save file to avoid reading it twice */
#if (defined __unix) || (defined __unix__) || (defined __APPLE__)
    if (flock(fd, LOCK_EX | LOCK_NB) == -1)
#elif (defined(WIN32))
    if (_locking(fd, LK_NBLCK, 0xffffffff) == -1)
#endif
    {
        /* could not obtain the lock */
        GString *desc = g_string_new("NLarn cannot be started.\n\n"
                                     "Could not lock the savegame file:\n");
        g_string_append(desc, strerror(errno));
        display_show_message("Error", desc->str, 0);
        g_string_free(desc, TRUE);
        display_shutdown();
        exit(EXIT_FAILURE);
    }

    return fd;
}

/* the game settings */
static struct game_config config = {};

void game_init(int argc, char *argv[])
{
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    gid_t realgid;
    uid_t realuid;

    /* assemble the scoreboard filename */
    gchar *scoreboard_filename = g_build_path(G_DIR_SEPARATOR_S, default_var_dir,
                                              highscores, NULL);

    /* Open the scoreboard file. */
    if ((scoreboard_fd = open(scoreboard_filename, O_RDWR)) == -1)
    {
        perror("Could not open scoreboard file");
        exit(EXIT_FAILURE);
    }

    /* Figure out who we really are. */
    realgid = getgid();
    realuid = getuid();

    /* This is where we drop our setuid/setgid privileges. */
#ifdef __linux__
    if (setresgid(-1, realgid, realgid) != 0) {
#else
    if (setregid(-1, realgid) != 0) {
#endif
        perror("Could not drop setgid privileges");
        exit(EXIT_FAILURE);
    }

#ifdef __linux__
    if (setresuid(-1, realuid, realuid) != 0) {
#else
    if (setreuid(-1, realuid) != 0) {
#endif
        perror("Could not drop setuid privileges");
        exit(EXIT_FAILURE);
    }
#endif

    /* allocate space for game structure */
    nlarn = g_malloc0(sizeof(game));

    /* determine paths and file names */
    /* base directory for a local install */
    nlarn->basedir = g_path_get_dirname(argv[0]);

    /* try to use the directory below the binary's location first */
    nlarn->libdir = g_build_path(G_DIR_SEPARATOR_S, nlarn->basedir, "lib", NULL);

    if (!g_file_test(nlarn->libdir, G_FILE_TEST_IS_DIR))
    {
        /* local lib directory could not be found, try the system wide directory. */
#ifdef __APPLE__
        char *rellibdir = g_build_path(G_DIR_SEPARATOR_S, nlarn->basedir,
                                       "../Resources", NULL);
#endif
        if (g_file_test(default_lib_dir, G_FILE_TEST_IS_DIR))
        {
            /* system-wide data directory exists */
            /* string has to be dup'd as it is feed in the end */
            nlarn->libdir = g_strdup((char *)default_lib_dir);
        }
#ifdef __APPLE__
        else if (g_file_test(rellibdir, G_FILE_TEST_IS_DIR))
        {
            /* program seems to be installed relocatable */
            nlarn->libdir = g_strdup(rellibdir);
        }
#endif
        else
        {
            g_printerr("Could not find game library directory.\n\n"
                       "Paths I've tried:\n"
                       " * %s\n"
#ifdef __APPLE__
                       " * %s\n"
#endif
                       " * %s\n\n"
                       "Please reinstall the game.\n",
                       nlarn->libdir,
#ifdef __APPLE__
                       rellibdir,
#endif
                       default_lib_dir);

            g_free(nlarn->libdir);
#ifdef __APPLE__
            g_free(rellibdir);
#endif
            exit(EXIT_FAILURE);
        }
    }

    nlarn->mesgfile = g_build_filename(nlarn->libdir, mesgfile, NULL);
    nlarn->helpfile = g_build_filename(nlarn->libdir, helpfile, NULL);
    nlarn->mazefile = g_build_filename(nlarn->libdir, mazefile, NULL);
    nlarn->fortunes = g_build_filename(nlarn->libdir, fortunes, NULL);
#if ((defined (__unix) || defined (__unix__)) && defined (SETGID))
    nlarn->highscores = scoreboard_filename;
#else
    nlarn->highscores = g_build_filename(nlarn->libdir, highscores, NULL);
#endif

    /* parse the command line options */
    parse_commandline(argc, argv, &config);

    /* show version information */
    if (config.show_version) {
        g_printf("NLarn version %d.%d.%d%s, built on %s.\n\n",
                VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, GITREV,
                __DATE__);
        g_printf("Game base directory:\t%s\n", nlarn->basedir);
        g_printf("Game lib directory:\t%s\n", nlarn->libdir);
        g_printf("Game savefile version:\t%d\n", SAVEFILE_VERSION);

        exit(EXIT_SUCCESS);
    }

    /* show highscores */
    if (config.show_scores) {
        GList *scores = scores_load();
        g_autofree char *s = scores_to_string(scores, NULL);

        g_printf("NLarn Hall of Fame\n==================\n\n%s", s);
        scores_destroy(scores);

        exit(EXIT_SUCCESS);
    }

    /* verify that user directory exists */
    if (!g_file_test(game_userdir(), G_FILE_TEST_IS_DIR))
    {
        /* directory is missing -> create it */
        int ret = g_mkdir(game_userdir(), 0755);

        if (ret == -1)
        {
            /* creating the directory failed */
            g_printerr("Failed to create directory %s.", game_userdir());
            exit(EXIT_FAILURE);
        }
    }

    /* try loading settings from the default configuration file */
    nlarn->inifile = g_build_path(G_DIR_SEPARATOR_S, game_userdir(),
            config_file, NULL);

    /* write a default configuration file, if none exists */
    if (!g_file_test(nlarn->inifile, G_FILE_TEST_IS_REGULAR))
    {
        write_ini_file(nlarn->inifile, NULL);
    }

    /* try to load settings from the configuration file */
    parse_ini_file(nlarn->inifile, &config);

#ifdef SDLPDCURSES
    /* If a font size was defined, export it to the environment
     * before initialising PDCurses. */
    if (config.font_size)
    {
        gchar size[4];
        g_snprintf(size, 3, "%d", config.font_size);
        g_setenv("PDC_FONT_SIZE", size, TRUE);
    }
#endif
    /* initialise the display - must not happen before this point
       otherwise displaying the command line help fails */
    display_init();

    /* call display_shutdown when terminating the game */
    atexit(display_shutdown);

    /* set autosave setting (default: TRUE) */
    game_autosave(nlarn) = !config.no_autosave;

    /* assemble the save file name */
    nlarn->savefile = g_build_path(G_DIR_SEPARATOR_S, game_userdir(),
            save_file, NULL);

    if (!game_load())
    {
        /* set game parameters */
        game_difficulty(nlarn) = config.difficulty;
        game_wizardmode(nlarn) = config.wizard;

        /* restoring a save game failed - start a new game. */
        game_new();

        /* put the player into the town */
        player_map_enter(nlarn->p, game_map(nlarn, 0), FALSE);

        /* give player knowledge of the town */
        scroll_mapping(nlarn->p, NULL);

        if (config.name && strlen(config.name) > 0)
        {
            nlarn->p->name = config.name;
        }

        if (config.gender)
        {
            nlarn->p->sex = parse_gender(config.gender[0]);
        }

        if (config.stats && strlen(config.stats) > 0)
        {
            config.stats[0] = g_ascii_tolower(config.stats[0]);
            nlarn->player_stats_set = player_assign_bonus_stats(
                    nlarn->p, config.stats[0]);
        }


        if (config.wizard)
        {
            log_add_entry(nlarn->log, "Wizard mode has been activated.");
        }

    } /* end new game only settings */

    /* parse auto pick-up settings */
    if (config.auto_pickup && (nlarn->p != NULL))
    {
        parse_autopickup_settings(config.auto_pickup,
                nlarn->p->settings.auto_pickup);

         g_free(config.auto_pickup);
    }
}

game *game_destroy(game *g)
{
    g_assert(g != NULL);

    /* everything must go */
    g_free(g->basedir);
    g_free(g->libdir);

    g_free(g->mesgfile);
    g_free(g->helpfile);
    g_free(g->mazefile);
    g_free(g->fortunes);
    g_free(g->highscores);
    g_free(g->inifile);
    g_free(g->savefile);

    for (int i = 0; i < MAP_MAX; i++)
    {
        if (g->maps[i] == NULL)
        {
            /* killed early during game initialisation */
            g_free(g);
            return NULL;
        }
        map_destroy(g->maps[i]);
    }

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
    g_free(g);

    return NULL;
}

const gchar *game_userdir()
{
    static gchar *userdir = NULL;

    if (userdir == NULL)
    {
        if (config.userdir)
        {
            if (!g_file_test(config.userdir, G_FILE_TEST_IS_DIR))
            {
                g_printerr("Supplied user directory \"%s\" does not exist.",
                        config.userdir);

                exit(EXIT_FAILURE);
            }
            else
            {
                userdir = g_strdup(config.userdir);
            }

        }
        else
        {
#ifdef WIN32
            userdir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(),
                    "nlarn", NULL);
#else
            userdir = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(),
                    ".nlarn", NULL);
#endif
        }
    }

    return userdir;
}

int game_save(game *g)
{
    int err;
    struct cJSON *save, *obj;
    display_window *win = NULL;

    g_assert(g != NULL);

    /* if the display has been initialised, show a pop-up message */
    if (display_available())
        win = display_popup(2, 2, 0, NULL, "Saving....", 0);

    save = cJSON_CreateObject();

    cJSON_AddNumberToObject(save, "nlarn_version", g->version);
    cJSON_AddNumberToObject(save, "time_start", g->time_start);
    cJSON_AddNumberToObject(save, "gtime", g->gtime);
    cJSON_AddNumberToObject(save, "difficulty", g->difficulty);
    cJSON_AddItemToObject(save, "rng_state", rand_serialize());

    /* maps */
    cJSON_AddItemToObject(save, "maps", obj = cJSON_CreateArray());
    for (int idx = 0; idx < MAP_MAX; idx++)
    {
        cJSON_AddItemToArray(obj, map_serialize(g->maps[idx]));
    }

    cJSON_AddItemToObject(save, "amulet_created",
                          cJSON_CreateIntArray(g->amulet_created, AM_MAX));

    cJSON_AddItemToObject(save, "armour_created",
                          cJSON_CreateIntArray(g->armour_created, AT_MAX));

    cJSON_AddItemToObject(save, "weapon_created",
                          cJSON_CreateIntArray(g->weapon_created, WT_MAX));

    if (g->cure_dianthr_created) cJSON_AddTrueToObject(save, "cure_dianthr_created");

    cJSON_AddItemToObject(save, "amulet_material_mapping",
                          cJSON_CreateIntArray(g->amulet_material_mapping, AM_MAX));

    cJSON_AddItemToObject(save, "potion_desc_mapping",
                          cJSON_CreateIntArray(g->potion_desc_mapping, PO_MAX));

    cJSON_AddItemToObject(save, "ring_material_mapping",
                          cJSON_CreateIntArray(g->ring_material_mapping, RT_MAX));

    cJSON_AddItemToObject(save, "scroll_desc_mapping",
                          cJSON_CreateIntArray(g->scroll_desc_mapping, ST_MAX));

    cJSON_AddItemToObject(save, "book_desc_mapping",
                          cJSON_CreateIntArray(g->book_desc_mapping, SP_MAX));

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

    /* free memory claimed by JSON structures */
    cJSON_Delete(save);

    /* open save file for writing */
    FILE* fhandle;
    if (sgfd)
    {
        /*
         * File is already opened.
         * We need to open a duplicate of the file descriptor as gzclose
         * would close the file descriptor we keep to ensure the lock
         * on the file is kept.
         */
        fhandle = fdopen(dup(sgfd), "w");
        /* Position at beginning of file, otherwise zlib would append */
        rewind(fhandle);
    }
    else
    {
        /* File need to be opened for the first time */
        fhandle = fopen(g->savefile, "wb");
    }

    if (fhandle == NULL)
    {
        log_add_entry(g->log, "Error opening save file \"%s\".", g->savefile);
        free(sg);
        return FALSE;
    }

    if (!sgfd)
    {
        /* first time save, try locking the file */
        sgfd = try_locking_savegame_file(fhandle);
    }

    gzFile file = gzdopen(fileno(fhandle), "wb");
    if (gzputs(file, sg) != (int)strlen(sg))
    {
        log_add_entry(g->log, "Error writing save file \"%s\": %s",
                g->savefile, gzerror(file, &err));

        free(sg);
        return FALSE;
    }

    free(sg);
    gzclose(file);

    /* if a pop-up message has been opened, destroy it here */
    if (win != NULL)
        display_window_destroy(win);

    return TRUE;
}

map *game_map(game *g, guint nmap)
{
    g_assert (g != NULL && nmap < MAP_MAX);

    return g->maps[nmap];
}

void game_spin_the_wheel(game *g)
{
    map *amap;

    g_assert(g != NULL);

    /* add the player's speed to the player's movement points */
    nlarn->p->movement += player_get_speed(nlarn->p);

    /* per-map actions */
    for (int nmap = 0; nmap < MAP_MAX; nmap++)
    {
        amap = game_map(g, nmap);

        /* call map timers */
        map_timer(amap);

        /* spawn some monsters every now and then */
        if (g->gtime % (100 + nmap) == 0)
        {
            map_fill_with_life(amap);
        }
    }

    amap = game_map(nlarn, Z(g->p->pos));

    /* check if player is stuck inside a wall without walk through wall */
    if ((map_tiletype_at(amap, g->p->pos) == LT_WALL)
            && !player_effect(g->p, ET_WALL_WALK))
    {
        player_die(g->p, PD_STUCK, 0);
    }

    /* check if the player is on a deep water tile without levitation */
    if ((map_tiletype_at(amap, g->p->pos) == LT_DEEPWATER)
            && !player_effect(g->p, ET_LEVITATION))
    {
        player_die(g->p, PD_DROWNED, 0);
    }

    /* check if the player is on a lava tile without levitation */
    if ((map_tiletype_at(amap, g->p->pos) == LT_LAVA)
            && !player_effect(g->p, ET_LEVITATION))
    {
        player_die(g->p, PD_MELTED, 0);
    }

    /* deal damage cause by map tiles to player */
    damage *dam = map_tile_damage(amap, g->p->pos,
                                  player_effect(g->p, ET_LEVITATION));

    if (dam != NULL)
        player_damage_take(g->p, dam, PD_MAP, map_tiletype_at(amap, g->p->pos));

    /* move all monsters */
    g_hash_table_foreach(g->monsters, (GHFunc)monster_move, g);

    /* destroy all monsters that have been killed during this turn */
    game_remove_dead_monsters(g);

    /* move all spheres */
    g_ptr_array_foreach(g->spheres, (GFunc)sphere_move, g);

    /* calculate bank interest */
    building_bank_calc_interest(g);

    g->gtime++; /* count up the time  */
    log_set_time(g->log, g->gtime); /* adjust time for log entries */
}

void game_remove_dead_monsters(game *g)
{
    g_assert (g != NULL);

    while (g->dead_monsters->len > 0)
    {
        g_ptr_array_remove_index(g->dead_monsters, g->dead_monsters->len - 1);
    }
}

gpointer game_item_register(game *g, item *it)
{
    g_assert (g != NULL && it != NULL);

    gpointer nkey = GUINT_TO_POINTER(++g->item_max_id);
    g_hash_table_insert(g->items, nkey, it);

    return nkey;
}

void game_item_unregister(game *g, gpointer it)
{
    g_assert (g != NULL && it != NULL);

    g_hash_table_remove(g->items, it);
}

item *game_item_get(game *g, gpointer id)
{
    g_assert(g != NULL && id != NULL);

    return (item *)g_hash_table_lookup(g->items, id);
}

gpointer game_effect_register(game *g, effect *e)
{
    g_assert (g != NULL && e != NULL);

    gpointer nkey = GUINT_TO_POINTER(++g->effect_max_id);
    g_hash_table_insert(g->effects, nkey, e);

    return nkey;
}

void game_effect_unregister(game *g, gpointer e)
{
    g_assert (g != NULL && e != NULL);

    g_hash_table_remove(g->effects, e);
}

effect *game_effect_get(game *g, gpointer id)
{
    g_assert(g != NULL && id != NULL);
    return (effect *)g_hash_table_lookup(g->effects, id);
}

gpointer game_monster_register(game *g, monster *m)
{
    g_assert (g != NULL && m != NULL);

    gpointer nkey = GUINT_TO_POINTER(++g->monster_max_id);
    g_hash_table_insert(g->monsters, nkey, m);

    return nkey;
}

void game_monster_unregister(game *g, gpointer m)
{
    g_assert (g != NULL && m != NULL);

    g_hash_table_remove(g->monsters, m);
}

monster *game_monster_get(game *g, gpointer id)
{
    g_assert(g != NULL && id != NULL);
    return (monster *)g_hash_table_lookup(g->monsters, id);
}

static void game_new()
{
    /* initialize object hashes (here as they will be needed by player_new) */
    nlarn->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    nlarn->monsters = g_hash_table_new(&g_direct_hash, &g_direct_equal);

    /* initialize the array to store monsters that died during the turn */
    nlarn->dead_monsters = g_ptr_array_new_with_free_func(
            (GDestroyNotify)monster_destroy);

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
    for (size_t idx = 0; idx < MAP_MAX; idx++)
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

static gboolean game_load()
{
    int size;
    cJSON *save, *obj;
    display_window *win = NULL;

    /* size of the buffer we allocate to store the uncompressed file content */
    const int bufsize = 1024 * 1024 * 3;

    /* try to open save file */
    FILE* file = fopen(nlarn->savefile, "rb+");

    if (file == NULL)
    {
        /* failed to open save game file */
        return FALSE;
    }

    /*
     * When not on Windows, lock the save file as long the process is
     * alive. This ensures no two instances of the game can be started
     * from the user's saved game.
     */
    sgfd = try_locking_savegame_file(file);

    /* open the file with zlib */
    gzFile sg = gzdopen(fileno(file), "rb");

    /* if the display has been initialised, show a pop-up message */
    if (display_available())
        win = display_popup(2, 2, 0, NULL, "Loading....", 0);

    /* temporary buffer to store uncompressed save file content */
    char *sgbuf = g_malloc0(bufsize);

    if (!gzread(sg, sgbuf, bufsize))
    {
        /* Reading the file failed. Terminate the game with an error message */
        display_shutdown();
        g_printerr("Failed to restore save file \"%s\".\n", nlarn->savefile);

        exit(EXIT_FAILURE);
    }

    /* close save file */
    gzclose(sg);

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

        /* if a pop-up message has been opened, destroy it here */
        if (win != NULL)
            display_window_destroy(win);

        /* offer to delete the incompatible save game */
        if (display_get_yesno("Saved game could not be loaded. "
                    "Delete and start new game?", NULL, NULL, NULL))
        {
            /* delete save file */
            g_unlink(nlarn->savefile);
        }
        else
        {
            display_shutdown();
            g_printerr("Save file \"%s\" is not compatible to current version.\n",
                    nlarn->savefile);

            exit(EXIT_FAILURE);
        }

        return FALSE;
    }

    /* restore saved game */
    nlarn->time_start = cJSON_GetObjectItem(save, "time_start")->valueint;
    nlarn->gtime = cJSON_GetObjectItem(save, "gtime")->valueint;
    nlarn->difficulty = cJSON_GetObjectItem(save, "difficulty")->valueint;
    rand_deserialize(cJSON_GetObjectItem(save, "rng_state"));

    if (cJSON_GetObjectItem(save, "wizard"))
        nlarn->wizard = TRUE;

    if (cJSON_GetObjectItem(save, "fullvis"))
        nlarn->fullvis = TRUE;

    obj = cJSON_GetObjectItem(save, "amulet_created");
    size = cJSON_GetArraySize(obj);
    g_assert(size == AM_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->amulet_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "armour_created");
    size = cJSON_GetArraySize(obj);
    g_assert(size == AT_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->armour_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "weapon_created");
    size = cJSON_GetArraySize(obj);
    g_assert(size == WT_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->weapon_created[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    if (cJSON_GetObjectItem(save, "cure_dianthr_created"))
        nlarn->cure_dianthr_created = TRUE;


    obj = cJSON_GetObjectItem(save, "amulet_material_mapping");
    size = cJSON_GetArraySize(obj);
    g_assert(size == AM_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->amulet_material_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "potion_desc_mapping");
    size = cJSON_GetArraySize(obj);
    g_assert(size == PO_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->potion_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "ring_material_mapping");
    size = cJSON_GetArraySize(obj);
    g_assert(size == RT_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->ring_material_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "scroll_desc_mapping");
    size = cJSON_GetArraySize(obj);
    g_assert(size == ST_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->scroll_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "book_desc_mapping");
    size = cJSON_GetArraySize(obj);
    g_assert(size == SP_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->book_desc_mapping[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(save, "monster_genocided");
    size = cJSON_GetArraySize(obj);
    g_assert(size == MT_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->monster_genocided[idx] = cJSON_GetArrayItem(obj, idx)->valueint;


    /* restore effects (have to come first) */
    nlarn->effects = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "effects");

    for (int idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        effect_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);


    /* restore items */
    nlarn->items = g_hash_table_new(&g_direct_hash, &g_direct_equal);
    obj = cJSON_GetObjectItem(save, "items");
    for (int idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        item_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);


    /* restore maps */
    obj = cJSON_GetObjectItem(save, "maps");
    size = cJSON_GetArraySize(obj);
    g_assert(size == MAP_MAX);
    for (int idx = 0; idx < size; idx++)
        nlarn->maps[idx] = map_deserialize(cJSON_GetArrayItem(obj, idx));


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

    for (int idx = 0; idx < cJSON_GetArraySize(obj); idx++)
        monster_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);

    /* initialize the array to store monsters that died during the turn */
    nlarn->dead_monsters = g_ptr_array_new_with_free_func(
            (GDestroyNotify)monster_destroy);


    /* restore spheres */
    nlarn->spheres = g_ptr_array_new();

    if ((obj = cJSON_GetObjectItem(save, "spheres")))
    {
        for (int idx = 0; idx < cJSON_GetArraySize(obj); idx++)
            sphere_deserialize(cJSON_GetArrayItem(obj, idx), nlarn);
    }

    /* free parsed save game */
    cJSON_Delete(save);

    /* set log turn number to current game turn number */
    log_set_time(nlarn->log, nlarn->gtime);

    /* welcome message */
    print_welcome_message(FALSE);

    /* refresh FOV */
    player_update_fov(nlarn->p);

    /* no need to define the player's stats */
    nlarn->player_stats_set = TRUE;

    /* if a pop-up message has been opened, destroy it here */
    if (win != NULL)
        display_window_destroy(win);

    return TRUE;
}

static void game_items_shuffle(game *g)
{
    shuffle(g->amulet_material_mapping, AM_MAX, 0);
    shuffle(g->potion_desc_mapping, PO_MAX, 1);
    shuffle(g->ring_material_mapping, RT_MAX, 0);
    shuffle(g->scroll_desc_mapping, ST_MAX, 1);
    shuffle(g->book_desc_mapping, SP_MAX, 0);
}

void game_delete_savefile()
{
    if (sgfd == 0)
    {
        /* no savegame present */
        return;
    }

    /* close and thus unlock the savegame file descriptor;
     * Windows won't let us delete the file otherwise */
    close(sgfd);
    sgfd = 0;

    /* assemble save file name */
    char *fullname = g_build_path(G_DIR_SEPARATOR_S, game_userdir(),
            save_file, NULL);

    /* actually delete the file */
    g_unlink(fullname);
    g_free(fullname);
}
