/*
 * game.h
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

#ifndef __GAME_H_
#define __GAME_H_

#include "items.h"
#include "map.h"
#include "player.h"
#include "spheres.h"

#define TIMELIMIT 30000 /* maximum number of moves before the game is called */

/* the world as we know it */
typedef struct game
{
    player *p;                  /* the player */
    map *maps[MAP_MAX];         /* the dungeon */
    guint64 time_start;         /* start time */
    guint32 gtime;              /* turn count */
    guint8 difficulty;          /* game difficulty */

    gchar *basedir;
    gchar *libdir;
    gchar *mesgfile;
    gchar *helpfile;
    gchar *mazefile;
    gchar *fortunes;
    gchar *highscores;

    /* stock of the dnd store */
    /* TODO: make sure these items are freed on terminating the game */
    inventory *store_stock;

    /* item / monster status */
    int amulet_created[AM_MAX];
    int weapon_created[WT_MAX];
    int monster_genocided[MT_MAX];

    int amulet_material_mapping[AM_MAX - 1];
    int potion_desc_mapping[PO_MAX - 1];
    int ring_material_mapping[RT_MAX - 1];
    int scroll_desc_mapping[ST_MAX - 1];
    int book_desc_mapping[SP_MAX - 1];

    /* these are the item ids assigned to new objects of the latter types */

    guint inventory_max_id;
    guint item_max_id;
    guint effect_max_id;
    guint monster_max_id;

    /* every object of the types inventory, item and effect will be registered
       in these hashed when created and unregistered when destroyed. */

    GHashTable *inventories;
    GHashTable *items;
    GHashTable *effects;
    GHashTable *monsters;

    /* spheres do not need to be referenced, thus a pointer array is sufficient */
    GPtrArray *spheres;

    /* flags */
    guint32
        cure_dianthr_created: 1, /* the potion of cure dianthroritis is a unique item */
        wizard: 1; /* wizard mode */
} game;

typedef struct _game_score
{
    char *player_name;
    gint8 sex;
    gint64 score;
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

typedef struct _game_save_head
{
    guint32
        version_major: 8,
        version_minor: 8,
        version_patch: 8;
    gint32 inventory_offset;
    gint32 item_offset;
    gint32 effect_offset;
    gint32 monster_offset;
    gint32 spheres_offset;
} game_save_head_t;

/* function declarations */

void game_new(int argc, char *argv[]);
int game_destroy(game *g);

int game_save(game *g, char *filename);
game *game_load(char *filename);

game_score_t *game_score(game *g, player_cod cod, int cause);
GList *game_score_add(game *g, game_score_t *score);
void game_scores_destroy(GList *gs);

map *game_map(game *g, guint nmap);
void game_spin_the_wheel(game *g, guint times);

/* functions to store game data */
gpointer game_inventory_register(game *g, inventory *inv);
void game_inventory_unregister(game *g, gpointer inv);
inventory *game_inventory_get(game *g, gpointer id);

gpointer game_item_register(game *g, item *it);
void game_item_unregister(game *g, gpointer it);
inventory *game_inventory_get(game *g, gpointer id);

gpointer game_effect_register(game *g, effect *e);
void game_effect_unregister(game *g, gpointer e);
effect *game_effect_get(game *g, gpointer id);

gpointer game_monster_register(game *g, monster *m);
void game_monster_unregister(game *g, gpointer m);
monster *game_monster_get(game *g, gpointer id);

/* macros */

#define game_difficulty(g) ((g)->difficulty)
#define game_wizardmode(g) ((g)->wizard)

#define game_turn(g)            ((g)->gtime)
#define game_remaining_turns(g) (TIMELIMIT - (g)->gtime)

#define game_dir(g) ((g)->basedir)
#define game_lib(g) ((g)->libdir)

#define game_mesgfile(g) ((g)->mesgfile)
#define game_helpfile(g) ((g)->helpfile)
#define game_mazefile(g) ((g)->mazefile)
#define game_fortunes(g) ((g)->fortunes)
#define game_highscores(g) ((g)->highscores)

/* gtime <> mobuls conversion */
#define gtime2mobuls(gtime)  ((abs((int)(gtime)) + 99) / 100)
#define mobuls2gtime(mobuls) ((int)(mobuls) * 100)

#endif
