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
static void game_monster_attack_player(monster *m, player *p);
static monster *game_monster_trigger_trap(game *g, monster *m, trap_t trap);
static void game_move_spheres(game *g);

static const char *mesgfile = "nlarn.msg";
static const char *helpfile = "nlarn.hlp";
static const char *mazefile = "maze";
static const char *fortunes = "fortune";

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
    g->p->auto_pickup = auto_pickup;

    /* allocate space for levels */
    for (i = 0; i < LEVEL_MAX; i++)
    {
        g->levels[i] = g_malloc0(sizeof (level));
        g->levels[i]->nlevel = i;
    }

    /* game time handling */
    g->gtime = 0;
    g->time_start = time(NULL);

    /* welcome message */
    log_add_entry(g->p->log, "Welcome to NLarn %d.%d.%d!", MAJOR_VERSION, MINOR_VERSION, PATCH_LEVEL);

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

    player_destroy(g->p);
    g_free(g);

    return EXIT_SUCCESS;
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

        /* increment count of turns since when player was last seen */
        if (m->lastseen)
        {
            m->lastseen++;
        }

        /* determine if the monster can see the player */
        if (player_effect(g->p, ET_INVISIBILITY) && !monster_has_infravision(m))
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
            level_monster_die(g->p->level, m, g->p->log);
            continue;
        }

        /* deal damage caused by floor effects */
        if (monster_hp_lose(m, level_tile_damage(g->p->level, m->pos)))
        {
            level_monster_die(g->p->level, m, g->p->log);
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

            if (pos_identical(p_pos, m_npos))
            {
                /* bump into invisible player */
                monster_update_player_pos(m, p_pos);
                m_npos = m->pos;
            }

            break; /* end MA_WANDER */

        case MA_ATTACK:
            if (pos_adjacent(m->pos, m->player_pos))
            {
                /* monster is standing next to player */
                game_monster_attack_player(m, g->p);
            }
            else
            {
                path = level_find_path(g->p->level, m->pos, m->player_pos);

                if (path && !g_queue_is_empty(path->path))
                {
                    el = g_queue_pop_head(path->path);
                    m_npos = el->pos;
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
                m->pos = m_npos;

                /* check for traps */
                if (level_trap_at(l, m->pos))
                {
                    m = game_monster_trigger_trap(g, m, level_trap_at(l, m->pos));
                }

            } /* end new position */
        } /* end monster repositioning */

        /* monster survived to this point */
        if (m)
        {
            monster_pickup_items(m, level_ilist_at(l, m->pos), g->p->log);
        }

    } /* foreach monster */
}

static void game_monster_attack_player(monster *m, player *p)
{
    int dam;
    int spc_dam = 0; /* damage from special attack */
    char *message = NULL; /* message for special attack */
    position pos; /* new position for teleport away */
    item *it; /* destroyed / robbed item */
    int pi; /* impact of perishing */

    if (player_effect(p, ET_SPIRIT_PROTECTION) && monster_is_spirit(m))
        return;

    if (player_effect(p, ET_UNDEAD_PROTECTION) && monster_is_undead(m))
        return;

    /* the player is invisible and the monster bashes into thin air */
    if (!pos_identical(m->player_pos, p->pos))
    {
        if (!level_is_monster_at(p->level, p->pos) && m->m_visible)
        {
            log_add_entry(p->log, "The %s bashes into thin air.", monster_get_name(m));
        }

        m->lastseen = 0;
        return;
    }

    if (player_effect(p, ET_INVISIBILITY) && chance(65))
    {
        log_add_entry(p->log,
                      "The %s misses wildly.",
                      monster_get_name(m));
        return;
    }

    if (player_effect(p, ET_CHARM_MONSTER)
            && (rand_m_n(5, 30) * monster_get_level(m) - player_get_cha(p) < 30))
    {
        log_add_entry(p->log,
                      "The %s is awestruck at your magnificence!",
                      monster_get_name(m));
        return;
    }

    dam = monster_get_dam(m);
    if (dam > 1)
    {
        dam += rand_1n(dam);
        dam += monster_get_level(m);
        dam += game_difficulty(p->game);
    }

    if ((dam > player_get_ac(p)) || (rand_1n(20) == 1))
    {
        dam -= player_get_ac(p);

        if (dam > 0)
        {
            player_hp_lose(p, dam, PD_MONSTER, m->type);
            log_add_entry(p->log,
                          "The %s hits you.",
                          monster_get_name(m));

            /* special attacks */
            switch (m->type)
            {
            case MT_RUST_MONSTER:
                /* FIXME: pick random armour type */
                if (p->eq_suit != NULL)
                {
                    pi = item_rust(p->eq_suit);
                    if (pi == PI_ENFORCED)
                    {
                        log_add_entry(p->log, "Your %s is dulled by the %s.",
                                      armour_name(p->eq_suit),
                                      monster_get_name(m));
                    }
                    else if (pi == PI_DESTROYED)
                    {
                        /* armour has been destroyed */
                        log_add_entry(p->log, "Your %s disintegrates!",
                                      armour_name(p->eq_suit));

                        it = inv_find_object(p->inventory, p->eq_suit);

                        log_disable(p->log);
                        player_item_unequip(p, it);
                        log_enable(p->log);

                        inv_del_element(p->inventory, it);
                    }
                }
                break;

            case MT_HELLHOUND:
            case MT_RED_DRAGON:
            case MT_BRONCE_DRAGON:
            case MT_SILVER_DRAGON:
                spc_dam = ((m->type == MT_HELLHOUND) ? rand_m_n(8, 23) : rand_m_n(20, 45))
                          - player_get_ac(p)
                          - player_effect(p, ET_FIRE_RESISTANCE);

                if (spc_dam > 0)
                    message = "The %s breathes fire at you!";
                else
                    message = "The %s's flame doesn't phase you.";

                break;

            case MT_CENTIPEDE:
            case MT_GIANT_ANT:
                if (player_get_str(p) > 3)
                    player_effect_add(p, effect_new(ET_DEC_STR, game_turn(p->game)));

                message = "The %s stung you!";

                break;

            case MT_WHITE_DRAGON:
                spc_dam = rand_1n(15)
                          + 18
                          - player_get_ac(p)
                          - player_effect(p, ET_COLD_RESISTANCE);

                if (spc_dam > 0)
                    message = "The %s blasts you with his cold breath.";
                else
                    message = "The %s's breath doesn't seem so cold.";

                break;

            case MT_VAMPIRE:
            case MT_WRAITH:
                message = "The %s drains you of your life energy!";
                player_lvl_lose(p, 1);
                break;

            case MT_WATER_LORD:
                message = "The %s got you with a gusher!";
                spc_dam = rand_1n(15)
                          + 25
                          - player_get_ac(p);
                break;

            case MT_LEPRECHAUN:
                /* FIXME: if player has a device of no theft abort this */
                /* FIXME: stolen gold should go to the leprechaun's inventory */
                if (player_get_gold(p))
                {
                    if (player_get_gold(p) > 32767)
                        player_set_gold(p, player_get_gold(p) >> 1);
                    else
                        player_set_gold(p, rand_1n(1 + (player_get_gold(p) >> 1)));

                    if (player_get_gold(p) < 0)
                        player_set_gold(p, 0);

                    message = "The %s picks your pocket. Your purse feels lighter";
                    /* TODO: add gold to leprechaun's inventory */
                }
                else
                {
                    message = "The %s couldn't find any gold to steal";
                }

                /* teleport away */
                pos = level_find_space(p->level, LE_MONSTER);
                m->pos = pos;

                break;

            case MT_DISENCHANTRESS:
                /* destroy random item */
                it = inv_get(p->inventory, rand_1n(inv_length(p->inventory)));
                if (it->type != IT_SCROLL && it->type != IT_POTION)
                {
                    message = "The %s hits you - you feel a sense of loss.";
                    if (player_item_is_equipped(p, it))
                        player_item_unequip(p, it);

                    inv_del_element(p->inventory, it);
                    item_destroy(it);
                }
                break;

            case MT_ICE_LIZARD:
            case MT_GREEN_DRAGON:
                message = "The %s hit you with his barbed tail.";
                spc_dam = rand_1n(25)
                          - player_get_ac(p);

                break;

            case MT_UMBER_HULK:
                player_effect_add(p, effect_new(ET_CONFUSION, game_turn(p->game)));
                break;

            case MT_SPIRIT_NAGA:
                /* FIXME: random effect */
                break;

            case MT_PLATINUM_DRAGON:
                message = "The %s flattens you with his psionics!";
                spc_dam = rand_1n(15)
                          + 30
                          - player_get_ac(p);
                break;

            case MT_NYMPH:
                /* FIXME: if player has a device of no theft abort this */
                if (inv_length(p->inventory))
                {
                    it = inv_get(p->inventory, rand_1n(inv_length(p->inventory)));
                    if (player_item_is_equipped(p, it))
                    {
                        log_disable(p->log);
                        player_item_unequip(p, it);
                        log_enable(p->log);
                    }

                    inv_del_element(p->inventory, it);
                    inv_add(m->inventory, it);

                    message = "The %s picks your pocket.";
                }
                else
                {
                    message = "The %s couldn't find anything to steal";
                }

                /* teleport away */
                pos = level_find_space(p->level, LE_MONSTER);
                m->pos = pos;

                break;

            case MT_BUGBEAR:
            case MT_OSEQUIP:
                spc_dam = ((m->type == MT_BUGBEAR) ? rand_m_n(5, 15) : rand_m_n(10, 25))
                          - player_get_ac(p);
                message = "The %s bit you!";
                spc_dam = rand_1n(15) + 10 - player_get_ac(p);
                break;

            default:
                break;
            } /* special attacks */

            if (spc_dam > 0)
                player_hp_lose(p, spc_dam, PD_MONSTER, m->type);

            if (message != NULL)
                log_add_entry(p->log, message, monster_get_name(m));
        }
        else
        {
            log_add_entry(p->log,
                          "The %s hit you but your armour protects you.",
                          monster_get_name(m));
        }
    }
    else
    {

        log_add_entry(p->log,
                      "The %s missed.",
                      monster_get_name(m));
    }

}

static monster *game_monster_trigger_trap(game *g, monster *m, trap_t trap)
{
    /* original position of the monster */
    position pos;

    /* effect monster might have gained during the move */
    effect *eff = NULL;

    /* return if the monster has not triggered the trap */
    if (!chance(trap_chance(trap)))
    {
        return m;
    }

    /* flying monsters are only affected by sleeping gas traps */
    if (monster_can_fly(m) && (trap != TT_SLEEPGAS))
    {
        return m;
    }

    pos = m->pos;

    /* monster triggered the trap */
    switch (trap)
    {
    case TT_TRAPDOOR:
        /* just remove the monster */
        g_ptr_array_remove_fast(g->p->level->mlist, m);

        break;

    case TT_TELEPORT:
        m->pos = level_find_space(g->p->level, LE_MONSTER);

        break;

    default:
        /* if there is an effect on the trap add it to the
         * monster's list of effects. */
        if (trap_effect(trap))
        {
            eff = effect_new(trap_effect(trap), game_turn(g));
            monster_effect_add(m, eff);
        }

    } /* switch (trap) */

    if (m->m_visible)
    {
        log_add_entry(g->p->log,
                      trap_m_message(trap),
                      monster_get_name(m));

        if (eff)
            log_add_entry(g->p->log,
                          effect_get_msg_m_start(eff),
                          monster_get_name(m));

        /* set player's knowlege of trap */
        player_memory_of(g->p, pos).trap = trap;
    }

    /* has the monster survived the trap? */
    if (trap_damage(trap) && monster_hp_lose(m, rand_1n(trap_damage(trap))))
    {
        level_monster_die(g->p->level, m, g->p->log);
        m = NULL;
    }

    /* destroy the monster if it has left the level */
    if (trap == TT_TRAPDOOR)
        monster_destroy(m);


    return m;
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
