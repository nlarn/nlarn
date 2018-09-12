/*
 * game.h
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

#ifndef __GAME_H_
#define __GAME_H_

#include <lua.h>
#include "inventory.h"
#include "items.h"
#include "map.h"
#include "player.h"
#include "spheres.h"

#define TIMELIMIT 30000 /* maximum number of moves before the game is called */

/* internal counter for save file compatibility */
#define SAVEFILE_VERSION    23

/* the world as we know it */
typedef struct game
{
    lua_State *L;               /* the lua engine */
    player *p;                  /* the player */
    map *maps[MAP_MAX];         /* the dungeon */
    guint8 version;             /* save compatibility value */
    guint64 time_start;         /* start time */
    guint32 gtime;              /* turn count */
    guint8 difficulty;          /* game difficulty */
    message_log *log;           /* game message log */

    gchar *basedir;
    gchar *libdir;
    gchar *mesgfile;
    gchar *helpfile;
    gchar *mazefile;
    gchar *fortunes;
    gchar *highscores;

    /* stock of the dnd store */
    inventory *store_stock;
    /* stock of the monastery */
    inventory *monastery_stock;

    /* storage of player's home */
    inventory *player_home;

    /* item / monster status */
    int amulet_created[AM_MAX];
    int armour_created[AT_MAX];
    int weapon_created[WT_MAX];
    int monster_genocided[MT_MAX];

    /* Item obfuscation mappings */
    int amulet_material_mapping[AM_MAX];
    int potion_desc_mapping[PO_MAX];
    int ring_material_mapping[RT_MAX];
    int scroll_desc_mapping[ST_MAX];
    int book_desc_mapping[SP_MAX];

    /* these are the item ids assigned to new objects of the latter types */

    guint item_max_id;
    guint effect_max_id;
    guint monster_max_id;

    /* every object of the types item, effect and monster will be registered
       in these hashed when created and unregistered when destroyed. */

    GHashTable *items;
    GHashTable *effects;
    GHashTable *monsters;

    /* Monsters that died during a turn have to be added to this array
       to allow destroying them after all monsters have been moved.
       The functions used to iterate over the GHashTable *monsters above
       do not allow to modify the hash table while iterating over it,
       giving the most nasty effects when doing so.
     */
    GPtrArray *dead_monsters;

    /* spheres do not need to be referenced, thus a pointer array is sufficient */
    GPtrArray *spheres;

    /* flags */
    guint32
        player_stats_set: 1, /* the player's stats have been assigned */
        cure_dianthr_created: 1, /* the potion of cure dianthroritis is a unique item */
        wizard: 1, /* wizard mode */
        fullvis: 1, /* show entire map in wizard mode */
        autosave: 1; /* save the game when entering a new map */
} game;

typedef struct _game_score
{
    char *player_name;
    gint8 sex;
    guint64 score;
    guint32 moves;
    player_cod cod;
    gint32 cause;
    gint32 hp;
    guint32 hp_max;
    gint32 level;
    gint32 level_max;
    gint32 dlevel;
    gint32 dlevel_max;
    gint32 difficulty;
    gint64 time_start;
    gint64 time_end;
} game_score_t;


/* function declarations */

/**
 * @brief Initialise the game. This function will try to restore a saved game;
 *        if it fails it will start a new game.
 *
 * @param count of command line arguments
 * @param command line arguments
 */
void game_init(int argc, char *argv[]);

game *game_destroy(game *g);

/**
 * @brief Function to return the user-specific directory.
 */
const gchar *game_userdir();

/**
 * @brief Save a game.
 * @param The game to save
 * @param The name of the file to be saved. Defaults to "nlarn.sav",
 *        if a NULL has been supplied.
 */
int game_save(game *g, const char *filename);

game_score_t *game_score(game *g, player_cod cod, int cause);
GList *game_score_add(game *g, game_score_t *score);
void game_scores_destroy(GList *gs);

map *game_map(game *g, guint nmap);
void game_spin_the_wheel(game *g);
void game_remove_dead_monsters(game *g);

/* functions to store game data */
gpointer game_inventory_register(game *g, inventory *inv);
void game_inventory_unregister(game *g, gpointer inv);
inventory *game_inventory_get(game *g, gpointer id);

gpointer game_item_register(game *g, item *it);
void game_item_unregister(game *g, gpointer it);
item *game_item_get(game *g, gpointer id);

gpointer game_effect_register(game *g, effect *e);
void game_effect_unregister(game *g, gpointer e);
effect *game_effect_get(game *g, gpointer id);

gpointer game_monster_register(game *g, monster *m);
void game_monster_unregister(game *g, gpointer m);
monster *game_monster_get(game *g, gpointer id);

void game_delete_savefile();

/* macros */

#define game_difficulty(g) ((g)->difficulty)
#define game_wizardmode(g) ((g)->wizard)
#define game_fullvis(g)    ((g)->fullvis)
#define game_autosave(g)   ((g)->autosave)

#define game_turn(g)            ((g)->gtime)
#define game_remaining_turns(g) (((g)->gtime > TIMELIMIT) ? 0 : TIMELIMIT - (g)->gtime)

#define game_dir(g) ((g)->basedir)
#define game_lib(g) ((g)->libdir)

#define game_mesgfile(g) ((g)->mesgfile)
#define game_helpfile(g) ((g)->helpfile)
#define game_mazefile(g) ((g)->mazefile)
#define game_fortunes(g) ((g)->fortunes)

/* gtime <> mobuls conversion */
#define gtime2mobuls(gtime)  ((abs(((int)gtime)) + 99) / 100)
#define mobuls2gtime(mobuls) ((int)(mobuls) * 100)

#endif
