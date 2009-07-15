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

    /* these will be filled by the command line parser */
    static gint difficulty = 0;
    static gboolean wizard = FALSE;
    static char *name = NULL;
    static gboolean female = FALSE; /* default: male */
    static gboolean auto_pickup = FALSE;

    /* objects needed to parse the command line */
    GError *error = NULL;
    GOptionContext *context;

    static GOptionEntry entries[] =
    {
        { "difficulty", 'd', 0, G_OPTION_ARG_INT, &difficulty, "Set difficulty", NULL },
        { "wizard", 'w', 0, G_OPTION_ARG_NONE, &wizard, "Enable wizard mode", NULL },
        { "name", 'n', 0, G_OPTION_ARG_STRING, &name, "Set character's name", NULL },
        { "female", 0, 0, G_OPTION_ARG_NONE, &female, "Make a female character (default: male)", NULL },
        { "auto-pickup", 0, 0, G_OPTION_ARG_NONE, &auto_pickup, "Automatically pick up items", NULL },
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
    g->p->settings.auto_pickup = auto_pickup;

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
    book_desc_shuffle();
    potion_desc_shuffle();
    ring_material_shuffle();
    scroll_desc_shuffle();

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
    int damage;

    assert(g != NULL && times > 0);

    level_expire_timer(g->p->level, times);

    for (turn = 0; turn < times; turn++)
    {
        player_regenerate(g->p);
        player_effects_expire(g->p, 1);

        /* deal damage cause by level tiles to player */
        if ((damage = level_tile_damage(g->p->level, g->p->pos)))
        {
            player_hp_lose(g->p, damage, PD_LEVEL,
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

    /* player's current position */
    position p_pos;

    /* monster's new position / temporary position */
    position m_npos, m_npos_tmp;

    /* number of tries to find m_npos */
    int tries;

    /* distance between player and m_npos_tmp */
    int dist;

    /* path to player */
    level_path *path = NULL;

    level_path_element *el = NULL;
    int monster_nr;

    assert(g != NULL);

    /* make shortcuts */
    l = g->p->level;
    p_pos = g->p->pos;

    for (monster_nr = 1; monster_nr <= l->mlist->len; monster_nr++)
    {
        m = g_ptr_array_index(l->mlist, monster_nr - 1);

        /* determine if the monster can see the player */
        if (pos_distance(m->pos, p_pos) > 7)
        {
            m->p_visible = FALSE;
        }
        else if (player_effect(g->p, ET_INVISIBILITY) && !monster_has_infravision(m))
        {
            m->p_visible = FALSE;
        }
        else if (monster_effect(m, ET_BLINDNESS))
        {
            /* monster is blinded */
            m->p_visible = FALSE;
        }
        else if (player_effect(g->p, ET_STEALTH) > monster_get_int(m))
        {
            /* player is stealther than monster is smart */
            m->p_visible = FALSE;
        }
        else
        {
            /* determine if player's position is visible from monster's position */
            m->p_visible = level_pos_is_visible(l, m->pos, p_pos);
        }


        if (m->p_visible)
        {
            /* update monster's knowledge of player's position */
            monster_update_player_pos(m, p_pos);
        }

        /*
         * regenerate / inflict poison upon monster.
         * as monster might be killed during the process we need
         * to exit the loop in this case
         */
        if (!monster_regenerate(m, game_turn(g), game_difficulty(g), g->p->log))
        {
            monster_die(m, g->p->log);
            continue;
        }

        /* deal damage caused by floor effects */
        if (monster_hp_lose(m, level_tile_damage(g->p->level, m->pos)))
        {
            monster_die(m, g->p->log);
            continue;
        }


        /* update monsters action */
        if (monster_update_action(m) && m->m_visible)
        {
            /* the monster has chosen a new action and the player
               can see the new action, so let's describe it */

            if (m->action == MA_ATTACK)
            {
                /* TODO: certain monster types will make a sound when attacking the player */
                /*
                log_add_entry(g->p->log,
                              "The %s has spotted you and heads towards you!",
                              monster_get_name(m));
                 */
            }

            else if (m->action == MA_FLEE)
            {
                log_add_entry(g->p->log,
                              "The %s turns to flee!",
                              monster_get_name(m));
            }
        }


        /* determine monster's next move */
        m_npos = m->pos;

        switch (m->action)
        {
        case MA_FLEE:
            dist = 0;

            for (tries = 1; tries < GD_MAX; tries++)
            {
                /* try all fields surrounding the monster if the
                 * distance between monster & player is greater */
                if (tries == GD_CURR)
                    continue;

                m_npos_tmp = pos_move(m->pos, tries);

                if (pos_valid(m_npos_tmp)
                        && lt_is_passable(level_tiletype_at(l,m_npos_tmp))
                        && !level_is_monster_at(l, m_npos_tmp)
                        && (pos_distance(p_pos, m_npos_tmp) > dist))
                {
                    /* distance is bigger than current distance */
                    m_npos = m_npos_tmp;
                    dist = pos_distance(m->player_pos, m_npos_tmp);
                }
            }

            break; /* end MA_FLEE */

        case MA_REMAIN:
            /* Sgt. Stan Still - do nothing */
            break;

        case MA_WANDER:
            tries = 0;

            do
            {
                m_npos = pos_move(m->pos, rand_1n(GD_MAX));
                tries++;
            }
            while ((!pos_valid(m_npos)
                    || !lt_is_passable(level_tiletype_at(l,m_npos))
                    || level_is_monster_at(l, m_npos))
                    && (tries < GD_MAX));

            /* new position has not been found, reset to current position */
            if (tries == GD_MAX)
                m_npos = m->pos;

            break; /* end MA_WANDER */

        case MA_ATTACK:
            if (pos_adjacent(m->pos, m->player_pos) && (m->lastseen == 1))
            {
                /* monster is standing next to player */
                monster_player_attack(m, g->p);
            }
            else
            {
                path = level_find_path(g->p->level, m->pos, m->player_pos);

                if (path && !g_queue_is_empty(path->path))
                {
                    el = g_queue_pop_head(path->path);
                    m_npos = el->pos;
                }
                else
                {
                    /* no path found. stop following player */
                    m->lastseen = 0;
                }

                /* cleanup */
                if (path)
                {
                    level_path_destroy(path);
                }
            }
            break; /* end MA_ATTACK */

        case MA_NONE:
        case MA_MAX:
            /* possibly a bug */
            break;
        }


        /* can the player see the new position? */
        m->m_visible = player_pos_visible(g->p, m_npos);

        /* *** if new position has been found - move the monster *** */
        if (!pos_identical(m_npos, m->pos))
        {
            if (pos_identical(p_pos, m_npos))
            {
                /* bump into invisible player */
                monster_update_player_pos(m, p_pos);
                m_npos = m->pos;

                log_add_entry(g->p->log, "The %s bumped into you.", monster_get_name(m));
            }

            /* check for door */
            if ((level_stationary_at(l,m_npos) == LS_CLOSEDDOOR)
                    && monster_has_hands(m)
                    && monster_get_int(m) > 3) /* lock out zombies */
            {
                /* open the door */
                level_stationary_at(l,m_npos) = LS_OPENDOOR;

                /* notify the player if the door is visible */
                if (m->m_visible)
                {
                    log_add_entry(g->p->log,
                                  "The %s opens the door.",
                                  monster_get_name(m));
                }
            }

            /* move towards player; check for monsters */
            else if (!level_is_monster_at(l, m_npos)
                     && ls_is_passable(level_stationary_at(l, m_npos))
                    )
            {
                monster_move(m, m_npos);

                /* check for traps */
                if (level_trap_at(l, m->pos))
                {
                    m = monster_trigger_trap(m, g->p);
                    if (m == NULL) continue; /* trap killed the monster */
                }

            } /* end new position */
        } /* end monster repositioning */

        monster_pickup_items(m, g->p->log);

        /* increment count of turns since when player was last seen */
        if (m->lastseen) m->lastseen++;

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
