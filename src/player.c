/*
 * player.c
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

static const char aa1[] = "mighty evil master";
static const char aa2[] = "apprentice demi-god";
static const char aa3[] = "minor demi-god";
static const char aa4[] = "major demi-god";
static const char aa5[] = "minor deity";
static const char aa6[] = "major deity";
static const char aa7[] = "novice guardian";
static const char aa8[] = "apprentice guardian";
static const char aa9[] = "The Creator";

static const char *player_lvl_desc[] =
{
    "novice explorer",		"apprentice explorer",	"practiced explorer",	/* -3 */
    "expert explorer",		"novice adventurer",	"adventurer",			/* -6 */
    "apprentice conjurer",	"conjurer",				"master conjurer",		/*  -9 */
    "apprentice mage",		"mage", 				"experienced mage",		/* -12 */
    "master mage",			"apprentice warlord", 	"novice warlord",		/* -15 */
    "expert warlord",		"master warlord", 		"apprentice gorgon",	/* -18 */
    "gorgon", 				"practiced gorgon", 	"master gorgon",		/* -21 */
    "demi-gorgon", 			"evil master", 			"great evil master",	/* -24 */
    aa1, aa1, aa1, /* -27*/
    aa1, aa1, aa1, /* -30*/
    aa1, aa1, aa1, /* -33*/
    aa1, aa1, aa1, /* -36*/
    aa1, aa1, aa1, /* -39*/
    aa2, aa2, aa2, /* -42*/
    aa2, aa2, aa2, /* -45*/
    aa2, aa2, aa2, /* -48*/
    aa3, aa3, aa3, /* -51*/
    aa3, aa3, aa3, /* -54*/
    aa3, aa3, aa3, /* -57*/
    aa4, aa4, aa4, /* -60*/
    aa4, aa4, aa4, /* -63*/
    aa4, aa4, aa4, /* -66*/
    aa5, aa5, aa5, /* -69*/
    aa5, aa5, aa5, /* -72*/
    aa5, aa5, aa5, /* -75*/
    aa6, aa6, aa6, /* -78*/
    aa6, aa6, aa6, /* -81*/
    aa6, aa6, aa6, /* -84*/
    aa7, aa7, aa7, /* -87*/
    aa8, aa8, aa8, /* -90*/
    aa8, aa8, aa8, /* -93*/
    "earth guardian", "air guardian",	"fire guardian",	/* -96*/
    "water guardian", "time guardian",	"ethereal guardian",/* -99*/
    aa9       , aa9       , aa9       ,/* -102*/
};

/*
    table of experience needed to be a certain level of player
    FIXME: this is crap. Use a formula instead an get rid of this...
 */
static const long player_lvl_exp[] =
{
    0,      /* there is no level 0 */
    0, 10, 20, 40, 80, 160, 320, 640, 1280, 2560, 5120,                                        /*  1-11 */
    10240, 20480, 40960, 100000, 200000, 400000, 700000, 1000000,                              /* 12-19 */
    2000000, 3000000, 4000000, 5000000, 6000000, 8000000, 10000000,                            /* 20-26 */
    12000000, 14000000, 16000000, 18000000, 20000000, 22000000, 24000000, 26000000, 28000000,  /* 27-35 */
    30000000, 32000000, 34000000, 36000000, 38000000, 40000000, 42000000, 44000000, 46000000,  /* 36-44 */
    48000000, 50000000, 52000000, 54000000, 56000000, 58000000, 60000000, 62000000, 64000000,  /* 45-53 */
    66000000, 68000000, 70000000, 72000000, 74000000, 76000000, 78000000, 80000000, 82000000,  /* 54-62 */
    84000000, 86000000, 88000000, 90000000, 92000000, 94000000, 96000000, 98000000, 100000000, /* 63-71 */
    105000000, 110000000, 115000000, 120000000, 125000000, 130000000, 135000000, 140000000,    /* 72-79 */
    145000000, 150000000, 155000000, 160000000, 165000000, 170000000, 175000000, 180000000,    /* 80-87 */
    185000000, 190000000, 195000000, 200000000, 210000000, 220000000, 230000000, 240000000,    /* 88-95 */
    250000000, 260000000, 270000000, 280000000, 290000000, 300000000                           /* 96-101*/
};

static int player_level_leave(player *p);

static int player_trigger_trap(player *p, trap_t trap);
static void player_calculate_octant(player *p, int row, float start, float end, int radius, int xx, int xy, int yx, int yy);
static void player_magic_alter_reality(player *p);
static int player_magic_create_monster(player *p);
static void player_magic_create_sphere(player *p);
static void player_magic_genocide_monster(player *p);
static void player_magic_make_wall(player *p);
static void player_magic_vaporize_rock(player *p);

static char *player_death_description(game_score_t *score, int verbose);

player *player_new(struct game *game)
{
    player *p;
    item *it;

    /* initialize player */
    p = g_malloc0(sizeof(player));

    p->strength = 12 + rand_0n(6);
    p->charisma = 12 + rand_0n(6);
    p->dexterity = 12 + rand_0n(6);
    p->wisdom = 12 + rand_0n(6);
    p->intelligence = 12 + rand_0n(6);
    p->constitution = 12 + rand_0n(6);

    p->hp = p->hp_max = (p->constitution + rand_0n(10));
    p->mp = p->mp_max = (p->intelligence + rand_0n(10));

    p->lvl = p->stats.max_level = 1;

    p->known_spells = g_ptr_array_new();
    p->effects = g_ptr_array_new();
    p->inventory = inv_new();

    it = item_new(IT_ARMOUR, AT_LEATHER, 1);
    inv_add(p->inventory, it);
    player_item_equip(p, it);

    it = item_new(IT_WEAPON, WT_DAGGER, 0);
    it->bonus_known = TRUE;
    inv_add(p->inventory, it);
    player_item_equip(p, it);

    /* start a new diary */
    p->log = log_new();

    /* potion of cure dianthroritis is always known */
    p->identified_potions[PO_CURE_DIANTHR] = TRUE;

    /* set pointer back to game */
    p->game = game;

    return p;
}

void player_destroy(player *p)
{
    int i;
    effect *eff;

    assert(p != NULL);

    /* release spells */
    while ((p->known_spells)->len > 0)
        spell_destroy(g_ptr_array_remove_index_fast(p->known_spells,
                      (p->known_spells)->len - 1));

    g_ptr_array_free(p->known_spells, TRUE);

    /* release effects */
    for (i = 1; i <= p->effects->len; i++)
    {
        eff = g_ptr_array_index(p->effects, i - 1);

        if (!eff->item)
        {
            effect_destroy(eff);
        }
    }

    g_ptr_array_free(p->effects, TRUE);

    inv_destroy(p->inventory);
    log_delete(p->log);

    if (p->name)
    {
        g_free(p->name);
    }

    g_free(p);
}

int player_regenerate(player *p)
{
    /* number of turns between occasions */
    int frequency;

    /* amount of regeneration */
    int regen = 0;

    /* temporary var for effect */
    effect *e;

    assert(p != NULL);

    /* modifier for frequency */
    frequency = game_difficulty(p->game) << 3;

    /* handle regeneration */
    if (p->regen_counter == 0)
    {
        p->regen_counter = 22 + frequency;

        if (p->hp < player_get_hp_max(p))
        {
            regen = 1
                    + player_effect(p, ET_INC_HP_REGEN)
                    - player_effect(p, ET_DEC_HP_REGEN);

            player_hp_gain(p, regen);
        }

        if (p->mp < player_get_mp_max(p))
        {
            regen = 1
                    + player_effect(p, ET_INC_MP_REGEN)
                    - player_effect(p, ET_DEC_MP_REGEN);

            player_mp_gain(p, regen);
        }
    }
    else
    {
        p->regen_counter--;
    }

    /* handle poison */
    if ((e = player_effect_get(p, ET_POISON)))
    {
        if ((game_turn(p->game) - e->start) % (22 - frequency) == 0)
        {
            player_hp_lose(p, e->amount, PD_EFFECT, e->type);
        }
    }

    return TRUE;
}

/**
 * Kill the player
 *
 * @param the player
 * @param the cause, e.g. PD_TRAP
 * @param the id of the specific cause, e.g. TT_DART
 *
 */
void player_die(player *p, player_cod cause_type, int cause)
{
    GString *text;
    game_score_t *score, *cscore;
    GList *scores, *iterator;

    char *message = NULL;
    char *title = NULL;

    int count, num;

    effect *ef;

    assert(p != NULL);

    /* check for life protection */
    if ((cause_type < PD_TOO_LATE) && (ef = player_effect_get(p, ET_LIFE_PROTECTION)))
    {
        log_add_entry(p->log, "You feel wiiieeeeerrrrrd all over!");

        if (ef->amount > 1)
        {
            ef->amount--;
        }
        else
        {
            player_effect_del(p, ef);
        }

        if (cause_type == PD_LASTLEVEL)
        {
            /* revert effects of drain level */
            player_lvl_gain(p, 1);
        }

        p->hp = p->hp_max;

        return;
    }

    switch (cause_type)
    {
    case PD_LASTLEVEL:
        message = "You fade to gray...";
        title = "In Memoriam";
        break;

    case PD_TOO_LATE:
        message = "You returned home too late!";
        title = "The End";
        break;

    case PD_WON:
        message = "You saved your daughter!";
        title = "Congratulations! You won!";
        break;

    case PD_LOST:
        message = "You didn't manage to save your daughter.";
        title = "You lost";
        break;

    case PD_QUIT:
        message = "You quit.";
        title = "The End";
        break;

    default:
        message = "You die...";
        title = "R. I. P.";
    }

    log_add_entry(p->log, message);


    /* resume game if wizard mode is enabled */
    if (game_wizardmode(p->game) && (cause_type != PD_QUIT))
    {
        log_add_entry(p->log, "WIZARD MODE. You stay alive.");

        p->hp = p->hp_max;
        /* return to level 1 if last level has been lost */
        if (p->lvl < 1) player_lvl_gain(p, 1);

        /* return to the game */
        return;
    }

    /* redraw screen to make sure player can see the cause of his death */
    display_paint_screen(p);

    score = game_score(p->game, cause_type, cause);
    scores = game_score_add(p->game, score);

    text = g_string_new(player_death_description(score, TRUE));

    /* assemble surrounding scores list */
    g_string_append(text, "\n\n");

    /* get position of current score in list */
    iterator = g_list_find(scores, score);

    /* jump up three entries */
    count = 1;
    while (count < 4)
    {
        if (iterator->prev == NULL)
        {
            break;
        }

        iterator = iterator->prev;
        count++;
    }

    /* determine position of first element of iterator */
    num = g_list_position(scores, iterator);

    /* display up to 7 entries */
    for (count = 1; iterator && (count < 8);
            iterator = iterator->next, count++)
    {
        cscore = (game_score_t *)iterator->data;
        g_string_append_printf(text, "%s%2d) %7" G_GINT64_FORMAT " %s [lvl. %d, %d hp]\n",
                               (cscore == score) ? "*" : " ", num + count,
                               cscore->score,
                               player_death_description(cscore, FALSE),
                               score->dlevel, cscore->hp);
    }


    display_show_message(title, text->str);
    g_string_free(text, TRUE);

    game_scores_destroy(scores);

    display_shutdown();
    game_destroy(p->game);

    exit(EXIT_SUCCESS);
}

gint64 player_calc_score(player *p, int won)
{
    gint64 score = 0;
    int i;

    assert (p != NULL);

    /* money */
    score = player_get_gold(p) + p->bank_account - p->outstanding_taxes;

    /* value of equipment */
    for (i = 1; i <= inv_length(p->inventory); i++)
    {
        score += item_price(inv_get(p->inventory, i - 1));
    }

    /* experience */
    score += p->experience;

    /* give points for remaining time if game has been won */
    if (won)
    {
        score += game_remaining_turns(p->game);
    }

    return score;
}

int player_move(player *p, int direction)
{
    int times = 1;         /* how many time ticks it took */
    position target_p;     /* coordinates of target */
    level_tile *target_t;  /* shortcut for target map tile */
    monster *target_m;     /* monster on target tile (if any) */

    assert(p != NULL && direction > GD_NONE && direction < GD_MAX);

    /* confusion: random movement */
    if (player_effect(p, ET_CONFUSION))
    {
        direction = rand_1n(GD_MAX - 1);
    }

    target_p = pos_move(p->pos, direction);

    /* make a shortcut to the target tile */
    target_t = level_tile_at(p->level, target_p);

    /* exceeded level limits */
    if (!pos_valid(target_p))
        return FALSE;

    /* impassable */
    if (!level_pos_passable(p->level, target_p))
        return FALSE;

    /* attack - no movement */
    if ((target_m = level_get_monster_at(p->level, target_p)))
        return player_attack(p, target_m);

    /* reposition player */
    player_position(p, target_p);

    /* trigger the trap */
    if (target_t->trap)
    {
        times += player_trigger_trap(p, target_t->trap);
    } /* end trap */


    if (p->settings.auto_pickup
            && (target_t->ilist && inv_length(target_t->ilist)))
    {
        /* pick up all items on floor */
        while (inv_length(target_t->ilist))
        {
            /* add time needed to pickup items to turn time */
            times += player_item_pickup(p, inv_del(target_t->ilist,
                                                   inv_length(target_t->ilist) - 1));
        }

        /* destroy unused inventory */
        if (!target_t->ilist->len)
        {
            inv_destroy(target_t->ilist);
            target_t->ilist = NULL;
        }
    } /* auto-pickup */

    return times;
}

int player_attack(player *p, monster *m)
{
    int prop;
    int damage;
    int roll;           /* the dice */
    effect *e;
    item *w;			/* shortcut to player's weapon */
    int pi;				/* impact of perishing */

    w = (p->eq_weapon == NULL) ? NULL : p->eq_weapon;

    prop = p->lvl
           + player_get_dex(p)
           + monster_get_ac(m)
           - 12
           - game_difficulty(p->game);

    if (w != NULL)
        /* player wields weapon */
        prop += (weapon_wc(w) / 4);

    roll = rand_1n(20);
    if ((roll < prop) || (roll == 1))
    {
        /* placed a hit */
        log_add_entry(p->log, "You hit the %s.", monster_get_name(m));

        damage = player_get_str(p)
                 + player_get_wc(p)
                 - 12
                 - game_difficulty(p->game);

        damage = rand_1n(damage);

        /* weapon damage due to rust */
        if ((w != NULL)
                && (m->type == MT_RUST_MONSTER
                    || m->type == MT_DISENCHANTRESS
                    || m->type == MT_GELATINOUSCUBE)
           )
        {
            pi = item_rust(w);
            if (pi == PI_ENFORCED)
            {
                log_add_entry(p->log, "Your weapon is dulled by the %s.", monster_get_name(m));
            }
            else if (pi == PI_DESTROYED)
            {
                /* weapon has been destroyed */
                log_add_entry(p->log, "Your %s disintegrates!", weapon_name(w));

                /* delete the weapon from the inventory */
                inv_del_element(p->inventory, w);

                /* destroy it and remove any reference to it */
                item_destroy(w);
                p->eq_weapon = w = NULL;
            }
        }

        /* FIXME: make sure hitting monst breaks stealth condition */

        if ((e = monster_effect_get(m, ET_HOLD_MONSTER)))
        {
            /* hitting a monster breaks hold monster spell */
            monster_effect_del(m, e);
        }

        /* FIXME: if a dragon and orb(s) of dragon slaying: damage *= 3	 */

        /* Deal with Vorpal Blade */
        if ((w != NULL)
                && (w->type == WT_VORPALBLADE)
                && chance(5)
                && monster_has_head(m)
                && monster_is_beheadable(m))
        {
            log_add_entry(p->log, "You behead the %s with your Vorpal Blade!", monster_get_name(m));
            damage = m->hp;
        }

        if ((m->type >= MT_DEMONLORD_I) && (w != NULL))
        {
            if (w->type == WT_LANCEOFDEATH)
                damage = 300;
            if (w->type == WT_SLAYER)
                damage = 10000;
        }

        /* inflict damage */
        m->hp -= damage;

        if ((w != NULL)
                && (m->type >= MT_DEMONLORD_I)
                && (w->type == WT_LANCEOFDEATH)
                && (m->hp > 0))
        {
            log_add_entry(p->log, "Your lance of death tickles the %s!", monster_get_name(m));
        }

        if (m->type == MT_METAMORPH)
        {
            if ((m->hp < 25) && (m->hp > 0))
            {
                m->type = MT_BRONCE_DRAGON + rand_0n(9);
                log_add_entry(p->log, "The metamorph turns into a %s!",
                              monster_get_name(m));
            }
        }

        /* if the player is invisible and the monster does not have infravision,
           remember the position where the attack came from
         */
        if (player_effect(p, ET_INVISIBILITY) && !monster_has_infravision(m))
        {
            monster_update_player_pos(m, p->pos);
        }


        if (m->hp < 1)
        {
            /* killed the monster */
            player_monster_kill(p, m);
        }
    }
    else
    {
        /* missed */
        log_add_entry(p->log, "You miss the %s.", monster_get_name(m));
    }

    return 1; /* i.e. turns used */
}

int player_position(player *p, position target)
{
    assert(p != NULL);

    if (pos_valid(target) && level_pos_passable(p->level, target))
    {
        p->pos = target;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * function to enter a level.  This routine must be called anytime the
 * player changes levels.  If that level is unknown it will be created.
 * A new set of monsters will be created for a new level, and existing
 * levels will get a few more monsters.
 *
 * Note that it is here we remove genocided monsters from the present level.
 *
 * @param player entering level
 * @param entered level
 * @param previous level
 * @return TRUE
 */
int player_level_enter(player *p, level *l)
{
    int count;
    position pos;

    assert(p != NULL && l != NULL);

    if (p->level)
    {
        player_level_leave(p);
    }

    if (!(l->visited))
    {
        level_new(l,
                  game_difficulty(p->game),
                  game_mazefile(p->game));

        if (p->stats.deepest_level < l->nlevel)
        {
            p->stats.deepest_level = l->nlevel;
        }
    }
    else
    {
        log_disable(p->log);

        /* expire timed effects */
        /* count might be negative if time has been modified (time warp) */
        count = abs(game_turn(p->game) - l->visited);
        level_expire_timer(l, min(count, G_MAXUINT8));

        monsters_genocide(l);

        log_enable(p->log);

        /* add some new monsters */
        if (l->nlevel > 0)
        {
            level_fill_with_live(l);
        }
    }

    /* position player */
    if ((l->nlevel == 0) && (p->level == NULL))
        /* GAME STARTS */
        pos = level_find_stationary(l, LS_HOME);

    else if ((p->level->nlevel == 0) && (l->nlevel == (LEVEL_MAX - 1)))
        /* took the elevator up */
        pos = level_find_stationary(l, LS_ELEVATORUP);

    else if ((p->level->nlevel == (LEVEL_MAX - 1)) && (l->nlevel == 0))
        /* took the elevator down */
        pos = level_find_stationary(l, LS_ELEVATORDOWN);

    else if (p->level->nlevel > l->nlevel)
    {
        /* climbing up */
        if (l->nlevel == 0)
            pos = level_find_stationary(l, LS_ENTRANCE);
        else
            pos = level_find_stationary(l, LS_STAIRSDOWN);
    }
    else if (l->nlevel > p->level->nlevel)
    {
        /* climbing down */
        if (l->nlevel == 1)
            pos = level_find_stationary(l, LS_ENTRANCE);
        else
            pos = level_find_stationary(l, LS_STAIRSUP);
    }

    if (!pos_valid(pos))
    {
        /* been teleported here or something like that, need a random spot */
        pos = level_find_space(l, LE_MONSTER);
    }

    p->pos = pos;

    if (l->nlevel == 0)
    {
        /* do not log this at the start of the game */
        if (p->log->gtime > 1)
            log_add_entry(p->log, "You return to town.");
    }
    else if (l->nlevel == 1 && p->level->nlevel == 0)
        log_add_entry(p->log, "You enter the caverns of Larn.");

    else if (p->level->nlevel < l->nlevel)
        log_add_entry(p->log, "You descend to level %d.", l->nlevel);

    else
        log_add_entry(p->log, "You ascend to level %d.", l->nlevel);


    /* move player to new level */
    p->level = l;

    /* give player knowledge of level 0 (town) */
    if (l->nlevel == 0)
        scroll_mapping(p, NULL);

    /* recalculate FOV to make ensure correct display after entering a level */
    player_update_fov(p, (player_effect(p, ET_BLINDNESS) ? 0 : 6 + player_effect(p, ET_AWARENESS)));

    return TRUE;
}

int player_teleport(player *p)
{
    int nlevel;
    assert(p != NULL);

    if (p->level->nlevel == 0)
        nlevel = 0;
    else if (p->level->nlevel < LEVEL_DMAX)
        nlevel = rand_0n(LEVEL_DMAX);
    else
        nlevel = rand_m_n(LEVEL_DMAX, LEVEL_MAX);

    if (nlevel != p->level->nlevel)
    {
        player_level_enter(p, p->game->levels[nlevel]);

        p->pos = level_find_space(p->level, LE_MONSTER);

        return TRUE;
    }

    return FALSE;
}

void player_monster_kill(player *p, monster *m)
{
    player_exp_gain(p, monster_get_exp(m));
    p->stats.monsters_killed[m->type] += 1;

    monster_die(m, p->log);
}

int player_examine(player *p, position pos)
{
    level_tile *tile;
    item *it;
    char item_desc[81];
    int i;
    char *desc = NULL, *tmp;

    assert(p != NULL);

    tile = level_tile_at(p->level, pos);

    /* add message if target tile contains a stationary item */
    if (tile->stationary > LS_NONE)
    {
        log_add_entry(p->log, ls_get_desc(tile->stationary));
    }

    /* add message if target tile contains a known trap */
    if (player_memory_of(p, pos).trap)
    {
        log_add_entry(p->log, "There is a %s here.", trap_description(tile->trap));
    }

    /* add message if target tile contains items */
    if (tile->ilist != NULL && inv_length(tile->ilist) > 0)
    {
        if (inv_length(tile->ilist) > 3)
        {
            log_add_entry(p->log, "There are multiple items here.");
        }
        else
        {
            for (i = 1; i <= inv_length(tile->ilist); i++)
            {
                it = inv_get(tile->ilist, i - 1);
                item_describe(it,
                              player_item_identified(p, it),
                              FALSE, FALSE,
                              item_desc, 80);

                if (i > 1)
                {
                    tmp = g_strjoin(" and ", desc, item_desc, NULL);
                    g_free(desc);
                    desc = tmp;
                }
                else
                {
                    desc = g_malloc(strlen(item_desc) + 1);
                    strcpy(desc, item_desc);
                }
            }
            log_add_entry(p->log, "You see %s here.", desc);
            g_free(desc);
        }
    }

    return TRUE;
}

int player_pickup(player *p)
{
    inventory **inv;
    item *it;
    char item_desc[81];
    int time;

    GPtrArray *callbacks;
    display_inv_callback *callback;
    int cb;

    assert(p != NULL);

    inv = &(level_ilist_at(p->level, p->pos));

    if ((*inv == NULL) || (inv_length(*inv) == 0))
    {
        log_add_entry(p->log, "There is nothing here.");

        time = 0;
    }
    else if (inv_length(*inv) == 1)
    {

        it = inv_del(*inv, 0);

        log_add_entry(p->log,
                      "You pick up %s.",
                      item_describe(it,
                                    player_item_identified(p, it),
                                    FALSE, FALSE,
                                    item_desc, 80));

        inv_add(p->inventory, it);

        time = 2;
    }
    else
    {
        /* define callback functions */
        callbacks = g_ptr_array_new();

        callback = g_malloc(sizeof(display_inv_callback));
        callback->description = "(g)et";
        callback->key = 'g';
        callback->function = &player_item_pickup;
        callback->checkfun = NULL;
        callback->active = FALSE;
        g_ptr_array_add(callbacks, callback);

        display_inventory("On the floor", p, *inv, callbacks, FALSE, NULL);

        /* clean up callbacks */
        for (cb = 1; cb <= callbacks->len; cb++)
            g_free(g_ptr_array_index(callbacks, cb - 1));

        g_ptr_array_free(callbacks, TRUE);

        time = 0;
    }

    /* clean up floor - destroy unused inventory */
    if ((*inv != NULL) && !(*inv)->len)
    {
        inv_destroy(*inv);
        *inv = NULL;
    }

    return time;
}

void player_lvl_gain(player *p, int count)
{
    int base;
    int i;

    assert(p != NULL && count > 0);

    p->lvl += count;
    log_add_entry(p->log, "You gain experience and become a %s!", player_lvl_desc[p->lvl]);

    if (p->lvl > p->stats.max_level)
        p->stats.max_level = p->lvl;

    if (p->experience < player_lvl_exp[p->lvl])
        /* artificially gained a level, need to adjust XP to match level */
        player_exp_gain(p, player_lvl_exp[p->lvl] - p->experience);

    for (i = 0; i < count; i++)
    {
        /* increase HP max */
        base = (p->constitution - game_difficulty(p->game)) >> 1;
        if (p->lvl < (7 - game_difficulty(p->game)))
            base += p->constitution >> 2;

        player_hp_max_gain(p, rand_1n(3) + rand_0n(max(base, 1)));

        /* increase MP max */
        base = (p->intelligence - game_difficulty(p->game)) >> 1;
        if (p->lvl < (7 - game_difficulty(p->game)))
            base += p->intelligence >> 2;

        player_mp_max_gain(p, rand_1n(3) + rand_0n(max(base, 1)));
    }
}

void player_lvl_lose(player *p, int count)
{
    int base;
    int i;

    assert(p != NULL && count > 0);

    p->lvl -= count;
    log_add_entry(p->log, "You return to experience level %d...", p->lvl);

    if (p->experience > player_lvl_exp[p->lvl])
        /* adjust XP to match level */
        player_exp_lose(p, p->experience - player_lvl_exp[p->lvl]);

    for (i = 0; i < count; i++)
    {
        /* decrease HP max */
        base = (p->constitution - game_difficulty(p->game)) >> 1;
        if (p->lvl < (7 - game_difficulty(p->game)))
            base += p->constitution >> 2;

        player_hp_max_lose(p, rand_1n(3) + rand_0n(max(base, 1)));

        /* decrease MP max */
        base = (p->intelligence - game_difficulty(p->game)) >> 1;
        if (p->lvl < (7 - game_difficulty(p->game)))
            base += p->intelligence >> 2;

        player_mp_max_lose(p, rand_1n(3) + rand_0n(max(base, 1)));
    }

    /* die if lost level 1 */
    if (p->lvl == 0)
        player_die(p, PD_LASTLEVEL, 0);
}

void player_exp_gain(player *p, int count)
{
    int numlevels = 0;

    assert(p != NULL && count > 0);
    p->experience += count;

    if (p->stats.max_xp < p->experience)
        p->stats.max_xp = p->experience;

    while (player_lvl_exp[p->lvl + 1 + numlevels] <= p->experience)
        numlevels++;

    if (numlevels)
        player_lvl_gain(p, numlevels);
}

void player_exp_lose(player *p, int count)
{
    int numlevels = 0;

    assert(p != NULL && count > 0);
    p->experience -= count;

    while ((player_lvl_exp[p->lvl - numlevels]) > p->experience)
        numlevels++;

    if (numlevels)
        player_lvl_lose(p, numlevels);
}


int player_hp_gain(player *p, int count)
{
    assert(p != NULL);

    p->hp += count;
    if (p->hp > player_get_hp_max(p))
        p->hp = player_get_hp_max(p);

    return p->hp;
}

void player_hp_lose(player *p, int count, player_cod cause_type, int cause)
{
    assert(p != NULL);
    p->hp -= count;

    if (p->hp < 1)
        player_die(p, cause_type, cause);
}

int player_hp_max_gain(player *p, int count)
{
    assert(p != NULL);

    p->hp_max += count;
    /* do _NOT_ increase hp */

    return p->hp_max;
}

int player_hp_max_lose(player *p, int count)
{
    assert(p != NULL);

    p->hp_max -= count;

    if (p->hp_max < 1)
        p->hp_max = 1;

    if (p->hp > p->hp_max)
        p->hp = p->hp_max;

    return p->hp_max;
}

int player_mp_gain(player *p, int count)
{
    assert(p != NULL);

    p->mp += count;
    if (p->mp > player_get_mp_max(p))
        p->mp = player_get_mp_max(p);

    return p->mp;
}

int player_mp_lose(player *p, int count)
{
    assert(p != NULL);

    p->mp -= count;
    if (p->mp < 0)
        p->mp = 0;

    return p->mp;
}

int player_mp_max_gain(player *p, int count)
{
    assert(p != NULL);

    p->mp_max += count;
    /* do _NOT_ increase mp */

    return p->mp_max;
}

int player_mp_max_lose(player *p, int count)
{
    assert(p != NULL);

    p->mp_max -= count;

    if (p->mp_max < 1)
        p->mp_max = 1;

    if (p->mp > p->mp_max)
        p->mp = p->mp_max;

    return p->mp_max;
}

/**
 * Select a spell to cast and cast it
 * @param the player
 * @return number of turns elapsed
 */
int player_spell_cast(player *p)
{
    int turns = 0;
    char buffer[61];

    spell *spell;
    effect *eff;
    position pos;
    monster *monster = NULL;

    GPtrArray *mlist;
    int i;

    int amount = 0;
    int radius = 0;
    level_tile_t type = LT_NONE;
    area *range = NULL;

    if (player_effect(p, ET_CONFUSION))
    {
        log_add_entry(p->log, "You can't aim your magic!");
        return turns;
    }

    spell = display_spell_select("Select a spell to cast", p, NULL);

    /* ESC pressed */
    if (!spell)
    {
        return turns;
    }

    /* insufficient mana */
    if (p->mp < spell_level(spell))
    {
        log_add_entry(p->log,
                      "You lack the power to cast %s.",
                      spell_name(spell));

        return turns;
    }

    log_add_entry(p->log,
                  "You cast %s.",
                  spell_name(spell));

    /* time usage */
    turns = 1;

    /* charge mana */
    player_mp_lose(p, spell_level(spell));

    switch (spell_type(spell))
    {
        /* spells that cause an effect on the player */
    case SC_PLAYER:

        assert(spell_effect(spell) > ET_NONE);

        eff = effect_new(spell_effect(spell), game_turn(p->game));

        /* make effects that are permanent by default non-permanent */
        /* unless it is the spell of healing, which does work this way */
        if ((eff->turns == 1) && (eff->type != ET_INC_HP))
        {
            eff->turns = 100;
        }

        player_effect_add(p, eff);

        break; /* SC_PLAYER */

        /* spells that cause an effect on a monster */
    case SC_POINT:

        g_snprintf(buffer, 60, "Select a target for %s.", spell_name(spell));
        pos = display_get_position(p, buffer, FALSE, FALSE);
        monster = level_get_monster_at(p->level, pos);

        if (!monster)
        {
            log_add_entry(p->log, "Which monster are you talking about?");
            break;
        }

        switch (spell->id)
        {

            /* dehydration */
        case SP_DRY:
            if (monster_hp_lose(monster, 100 + p->lvl))
            {
                log_add_entry(p->log,
                              spell_msg_succ(spell),
                              monster_get_name(monster));

                player_monster_kill(p, monster);
            }
            break; /* SP_DRY */

            /* drain life */
        case SP_DRL:
            amount = min(p->hp - 1, p->hp_max / 2);

            if (monster_hp_lose(monster, amount))
                player_monster_kill(p, monster);

            player_hp_lose(p, amount, PD_SPELL, SP_DRL);

            break; /* SP_DRL */

            /* finger of death */
        case SP_FGR:
            if (chance(1))
            {
                player_die(p, PD_SPELL, SP_FGR);
            }

            if (player_get_wis(p) > rand_m_n(10,20))
            {
                if (monster_hp_lose(monster, 2000))
                {
                    log_add_entry(p->log,
                                  spell_msg_succ(spell),
                                  monster_get_name(monster));

                    player_monster_kill(p, monster);
                }
            }
            else
            {
                log_add_entry(p->log, "It didn't work.");
            }

            break; /* SP_FGR */

            /* polymorph */
        case SP_PLY:
            do
            {
                monster->type = rand_1n(MT_MAX);
            }
            while (monster_is_genocided(monster->type));
            monster->hp = monster_get_hp_max(monster);

            break;

            /* teleport */
        case SP_TEL:
            log_add_entry(p->log, "The %s disappears.", monster_get_name(monster));
            monster->pos = level_find_space(p->level, LE_MONSTER);

            break; /* SP_TEL */

        default:
            /* spell has an effect, add that to the monster */
            assert(spell_effect(spell) != ET_NONE);

            if (spell_msg_succ(spell))
                log_add_entry(p->log, spell_msg_succ(spell),
                              monster_get_name(monster));

            eff = effect_new(spell_effect(spell), game_turn(p->game));
            if (!eff->amount) eff->amount = p->intelligence;

            /* show message if monster is visible */
            if (monster->m_visible
                    && effect_get_msg_m_start(eff)
                    && !monster_effect(monster, eff->type))
            {
                log_add_entry(p->log,
                              effect_get_msg_m_start(eff),
                              monster_get_name(monster));
            }

            /* has to come in the end as eff might be destroyed */
            monster_effect_add(monster, eff);

            break;

        } /* SC_POINT */

        break;
    case SC_RAY:    /* creates a ray */

        g_snprintf(buffer, 60, "Select a target for the %s.", spell_name(spell));
        pos = display_get_position(p, buffer, TRUE, TRUE);

        if (pos_valid(pos) && (monster = level_get_monster_at(p->level, pos)))
        {
            switch (spell->id)
            {
            case SP_MLE:
                amount = rand_1n(((p->lvl + 1) << 1)) + p->lvl + 3;
                break;

            case SP_SSP:
                amount = rand_1n(10) + 15 + p->lvl;
                break;

            case SP_CLD:
                amount = rand_1n(25) + 20 + p->lvl;
                break;

            case SP_LIT:
                amount = rand_1n(25) + 20 + (p->lvl << 1);
                break;
            }

            log_add_entry(p->log, spell_msg_succ(spell), monster_get_name(monster));

            if (monster_hp_lose(monster, amount))
                player_monster_kill(p, monster);
        }
        else
        {
            log_add_entry(p->log, "Aborted.");
        }

        break; /* SC_RAY */

    case SC_FLOOD: /* effect pours like water */

        g_snprintf(buffer, 60, "Where do you want to place the %s?", spell_name(spell));
        pos = display_get_position(p, buffer, FALSE, TRUE);

        if (pos_valid(pos))
        {
            switch (spell->id)
            {
            case SP_CKL:
                radius = 3;
                type = LT_CLOUD;
                amount = 10 + p->lvl;
                break;

            case SP_FLO:
                radius = 4;
                type = LT_WATER;
                amount = 25 + p->lvl;
                break;

            case SP_MFI:
                radius = 4;
                type = LT_FIRE;
                amount = 15 + p->lvl;
                break;
            }

            range = area_new_circle_flooded(pos, radius,
                                            level_get_obstacles(p->level, pos, radius));

            level_set_tiletype(p->level, range, type, amount);
            area_destroy(range);
        }
        else
        {
            log_add_entry(p->log, "Trust me, you don't.");
        }

        break; /* SC_FLOOD */

    case SC_BLAST: /* effect occurs like an explosion */

        g_snprintf(buffer, 60, "Point to the center of the %s.", spell_name(spell));
        pos = display_get_position(p, buffer, FALSE, TRUE);

        /* currently only fireball */
        amount = 25 + p->lvl + rand_0n(25 + p->lvl);

        mlist = level_get_monsters_in(p->level, rect_new_sized(pos, 1));

        for (i = 1; i <= mlist->len; i++)
        {
            monster = g_ptr_array_index(mlist, i - 1);

            log_add_entry(p->log,
                          spell_msg_succ(spell),
                          monster_get_name(monster));

            if (monster_hp_lose(monster, amount))
            {
                player_monster_kill(p, monster);
            }
        }

        if (pos_in_rect(p->pos, rect_new_sized(pos, 1)))
        {
            /* player has been hit by the blast as well */
            log_add_entry(p->log, "The fireball hits you.");

            player_hp_lose(p, amount - player_effect(p, ET_FIRE_RESISTANCE),
                           PD_SPELL, SP_BAL);
        }

        g_ptr_array_free(mlist, FALSE);

        break; /* SC_BLAST */

    case SC_OTHER:  /* unclassified */

        switch (spell->id)
        {
            /* cure blindness */
        case SP_CBL:
            if ((eff = player_effect_get(p, ET_BLINDNESS)))
                player_effect_del(p, eff);
            else
                log_add_entry(p->log, "You weren't even blinded!");
            break;

            /* create monster */
        case SP_CRE:
            player_magic_create_monster(p);
            break;

            /* time stop */
        case SP_STP:
            /* TODO: implement (ticket 39) */
            break;

            /* vaporize rock */
        case SP_VPR:
            player_magic_vaporize_rock(p);
            break;

            /* make wall */
        case SP_MKW:
            player_magic_make_wall(p);
            break;

            /* sphere of annihilation */
        case SP_SPH:
            player_magic_create_sphere(p);
            break;

            /* genocide */
        case SP_GEN:
            player_magic_genocide_monster(p);
            break;

            /* summon daemon */
        case SP_SUM:
            /* TODO: implement (ticket 55) */
            break;

            /* alter realitiy */
        case SP_ALT:
            player_magic_alter_reality(p);
            break;

        case SP_PER:
            /* TODO: implement (ticket 69) */
            break;
        }

        break;

    case SC_NONE:
    case SC_MAX:
        log_add_entry(p->log, "internal Error in %s:%d.", __FILE__, __LINE__);
        break;
    }

    return turns;
}

/**
 * Try to add a spell to the list of known spells
 *
 * @param the player
 * @param id of spell to learn
 * @return FALSE if learning the spell failed, otherwise level of knowledge
 */
int player_spell_learn(player *p, int spell_type)
{
    spell *s;
    int i;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    if (!player_spell_known(p, spell_type))
    {
        s = spell_new(spell_type);
        /* FIXME: this is very dowdy */
        s->learnt = p->log->gtime;

        /* TODO: add a check for intelligence */
        if (spell_level(s) > p->lvl)
        {
            /* spell is beyond the players scope */
            spell_destroy(s);
            return FALSE;
        }

        g_ptr_array_add(p->known_spells, s);
        return s->knowledge;
    }
    else
    {
        /* spell already known, improve knowledge */
        for (i = 1; i <= p->known_spells->len; i++)
        {
            /* search spell */
            s = g_ptr_array_index(p->known_spells, i - 1);

            if (s->id == spell_type)
            {
                /* found it */
                s->knowledge++;
                return s->knowledge;
            }
        }
    }

    /* should not reach this point, but who knows.. */
    return FALSE;
}

/**
 * Remove a spell from the list of known spells
 *
 * @param the player
 * @param the id of the spell to forget
 * @return TRUE if the spell could be found and removed, othrwise FALSE
 */
int player_spell_forget(player *p, int spell_type)
{
    spell *s;
    int i;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    for (i = 1; i <= p->known_spells->len; i++);
    {
        s = g_ptr_array_index(p->known_spells, i - 1);
        if (s->id == spell_type)
        {
            g_ptr_array_remove_index_fast(p->known_spells, i);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if a spell is known to the player
 * @param the player
 * @param id of the spell in question
 * @return FALSE if unknown, otherwise level of knowledge of that spell
 */
int player_spell_known(player *p, int spell_type)
{
    spell *s;
    int i;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    for (i = 1; i <= p->known_spells->len; i++)
    {
        s = g_ptr_array_index(p->known_spells, i - 1);
        if (s->id == spell_type)
        {
            return s->knowledge;
        }
    }

    return FALSE;
}

int player_inv_display(player *p)
{
    GPtrArray *callbacks;
    display_inv_callback *callback;
    int cb;

    assert(p!= NULL);

    if (inv_length(p->inventory) == 0)
    {
        /* don't show empty inventory */
        log_add_entry(p->log, "You do not carry anything.");
        return FALSE;
    }

    /* define callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(d)rop";
    callback->key = 'd';
    callback->function = &player_item_drop;
    callback->checkfun = &player_item_is_dropable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(e)quip";
    callback->key = 'e';
    callback->function = &player_item_equip;
    callback->checkfun = &player_item_is_equippable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(u)nequip";
    callback->key = 'u';
    callback->function = &player_item_unequip;
    callback->checkfun = &player_item_is_equipped;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    /* unequip and use should never appear together */
    callback = g_malloc(sizeof(display_inv_callback));
    callback->description = "(u)se";
    callback->key = 'u';
    callback->function = &player_item_use;
    callback->checkfun = &player_item_is_usable;
    callback->active = FALSE;
    g_ptr_array_add(callbacks, callback);

    display_inventory("Inventory", p, p->inventory, callbacks, FALSE, NULL);

    /* clean up */
    for (cb = 1; cb <= callbacks->len; cb++)
        g_free(g_ptr_array_index(callbacks, cb - 1));

    g_ptr_array_free(callbacks, TRUE);

    return TRUE;
}

/* level is needed to make function signature match display_inventory requirements */
int player_item_equip(player *p, item *it)
{
    item **islot = NULL;  /* pointer to chosen item slot */
    int time = 0;         /* time the desired action takes */
    char description[61];

    assert(p != NULL && it != NULL);

    /* the idea behind the time values: one turn to take one item off,
       one turn to get the item out of the pack */

    switch (it->type)
    {
    case IT_ARMOUR:
        switch (armour_category(it))
        {
        case AC_BOOTS:
            islot = &(p->eq_boots);
            time = 3;
            break;

        case AC_CLOAK:
            islot = &(p->eq_cloak);
            time = 2;
            break;

        case AC_GLOVES:
            islot = &(p->eq_gloves);
            time = 3;
            break;

        case AC_HELMET:
            islot = &(p->eq_helmet);
            time = 2;
            break;

        case AC_SHIELD:
            islot = &(p->eq_shield);
            time = 2;
            break;

        case AC_SUIT:
            islot = &(p->eq_suit);
            time = it->id + 1;
            break;

        case AC_NONE:
        case AC_MAX:
            /* shouldn't happen */
            break;
        }

        if (*islot == NULL)
        {
            it->bonus_known = TRUE;

            log_add_entry(p->log, "You are now wearing %s.",
                          item_describe(it,
                                        player_item_identified(p, it),
                                        TRUE, FALSE,
                                        description, 60));

            *islot = it;
        }
        break;

    case IT_RING:
        if (p->eq_ring_l == NULL)
            islot = &(p->eq_ring_l);
        else if (p->eq_ring_r == NULL)
            islot = &(p->eq_ring_r);

        if (islot != NULL)
        {
            log_add_entry(p->log, "You put %s on.",
                          item_describe(it,
                                        player_item_identified(p, it),
                                        TRUE, TRUE,
                                        description, 60));

            *islot = it;

            if (ring_is_observable(it))
            {
                player_item_identify(p, it);
                it->bonus_known = TRUE;
            }

            time = 2;
        }
        break;

    case IT_WEAPON:
        if (p->eq_weapon == NULL)
        {
            p->eq_weapon = it;
            log_add_entry(p->log, "You now wield %s.",
                          item_describe(it,
                                        player_item_identified(p, it),
                                        TRUE, TRUE,
                                        description, 60));

            time = 2 + weapon_is_twohanded(p->eq_weapon);

            if (it->cursed)
            {
                log_add_entry(p->log, "The %s welds itself into your hand.",
                              item_describe(it,
                                            player_item_identified(p, it),
                                            TRUE, TRUE,
                                            description, 60));

                it->curse_known = TRUE;
            }
        }
        break;

    default:
        /* nop */
        break;
    }

    if (it->effect)
    {
        player_effect_add(p, it->effect);
    }

    return time;
}

int player_item_unequip(player *p, item *it)
{
    int equipment_type;
    item **aslot = NULL;  /* handle to armour slot */
    item **rslot = NULL;  /* handle to ring slot */

    int time = 0;         /* time elapsed */
    char desc[61];        /* item description */

    assert(p != NULL && it != NULL);

    equipment_type = player_item_is_equipped(p, it);

    /* the idea behind the time values: one turn to take one item off,
       one turn to get the item out of the pack */

    switch (equipment_type)
    {
    case PE_BOOTS:
        aslot = &(p->eq_boots);
        time = 3;
        break;

    case PE_CLOAK:
        aslot = &(p->eq_cloak);
        time = 2;
        break;

    case PE_GLOVES:
        aslot = &(p->eq_gloves);
        time = 3;
        break;

    case PE_HELMET:
        aslot = &(p->eq_helmet);
        time = 2;
        break;

    case PE_SHIELD:
        aslot = &(p->eq_shield);
        time = 2;
        break;

    case PE_SUIT:
        aslot = &(p->eq_suit);
        /* the better the armour, the longer it takes to get out of it */
        time = (p->eq_suit)->type + 1;
        break;

    case PE_RING_L:
        rslot = &(p->eq_ring_l);
        break;

    case PE_RING_R:
        rslot = &(p->eq_ring_r);
        break;

    case PE_WEAPON:
        if (p->eq_weapon != NULL)
        {
            if (!p->eq_weapon->cursed)
            {
                log_add_entry(p->log, "You you put away %s.",
                              item_describe(it, player_item_identified(p, it),
                                            TRUE, TRUE,
                                            desc, 60));

                time = 2 + weapon_is_twohanded(p->eq_weapon);
                p->eq_weapon = NULL;
            }
            else
            {
                log_add_entry(p->log, "You can't put away the %s. " \
                              "It's welded into your hands.",
                              item_describe(it, player_item_identified(p, it),
                                            TRUE, TRUE,
                                            desc, 60));
            }
        }
        break;
    }

    /* take off armour */
    if ((aslot != NULL) && (*aslot != NULL))
    {
        if (!it->cursed)
        {
            log_add_entry(p->log, "You finish taking off %s.",
                          item_describe(it, player_item_identified(p, it),
                                        TRUE, TRUE,
                                        desc, 60));
            *aslot = NULL;
        }
        else
        {
            log_add_entry(p->log, "You can't take of the %s.%s",
                          item_describe(it, player_item_identified(p, it),
                                        TRUE, TRUE,
                                        desc, 60),
                          it->curse_known ? "" : " It appears to be cursed.");
            it->curse_known = TRUE;
        }
    }

    if ((rslot != NULL) && (*rslot != NULL))
    {
        if (!it->cursed)
        {
            log_add_entry(p->log, "You remove %s.",
                          item_describe(it, player_item_identified(p, it),
                                        TRUE, TRUE,
                                        desc, 60));

            player_effect_del(p, (*rslot)->effect);
            *rslot = NULL;
            time = 2;
        }
        else
        {
            log_add_entry(p->log, "You can not remove the %s.%s",
                          item_describe(it, player_item_identified(p, it),
                                        TRUE, TRUE,
                                        desc, 60),
                          it->curse_known ? "" : " It appears to be cursed.");

            it->curse_known = TRUE;

        }
    }

    return time;
}

/**
 * @param the player
 * @param the item
 * @return place where item is equipped
 */
int player_item_is_equipped(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    if (!item_is_equippable(it->type))
        return PE_NONE;

    if (it == p->eq_boots)
        return PE_BOOTS;

    if (it == p->eq_cloak)
        return PE_CLOAK;

    if (it == p->eq_gloves)
        return PE_GLOVES;

    if (it == p->eq_helmet)
        return PE_HELMET;

    if (it == p->eq_ring_l)
        return PE_RING_L;

    if (it == p->eq_ring_r)
        return PE_RING_R;

    if (it == p->eq_shield)
        return PE_SHIELD;

    if (it == p->eq_suit)
        return PE_SUIT;

    if (it == p->eq_weapon)
        return PE_WEAPON;

    return PE_NONE;
}

int player_item_is_equippable(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    if (!item_is_equippable(it->type))
        return FALSE;

    if (player_item_is_equipped(p, it))
        return FALSE;

    /* armour */
    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_BOOTS)
            && (p->eq_boots != NULL))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_CLOAK)
            && (p->eq_cloak != NULL))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_GLOVES)
            && (p->eq_gloves != NULL))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_HELMET)
            && (p->eq_helmet != NULL))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_SHIELD)
            && (p->eq_shield != NULL))
        return FALSE;

    /* shield / two-handed weapon combination */
    if (it->type == IT_ARMOUR
            && (armour_category(it) == AC_SHIELD)
            && (p->eq_weapon != NULL)
            && weapon_is_twohanded(p->eq_weapon))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_SUIT)
            && (p->eq_suit != NULL))
        return FALSE;

    /* rings */
    if ((it->type == IT_RING)
            && (p->eq_ring_l != NULL)
            && (p->eq_ring_r != NULL))
        return FALSE;

    /* weapons */
    if ((it->type == IT_WEAPON)
            && (p->eq_weapon != NULL))
        return FALSE;

    /* twohanded weapon / shield combinations */
    if ((it->type == IT_WEAPON)
            && weapon_is_twohanded(it)
            && (p->eq_shield != NULL))
        return FALSE;

    return TRUE;
}

int player_item_is_usable(player *p, item *it)
{
    assert(p != NULL && it != NULL);
    return item_is_usable(it->type);
}

int player_item_is_dropable(player *p, item *it)
{
    assert(p != NULL && it != NULL);
    return !player_item_is_equipped(p, it);
}

int player_item_is_damaged(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    if (it->corroded) return TRUE;
    if (it->burnt) return TRUE;
    if (it->rusty) return TRUE;

    return FALSE;
}

int player_item_is_affordable(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    return (item_price(it) <= player_get_gold(p));
}

int player_item_is_sellable(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    return (!player_item_is_equipped(p, it));
}

int player_item_is_identifiable(player *p, item *it)
{
    return (!player_item_identified(p, it));
}

int player_item_identified(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    switch (it->type)
    {
    case IT_BOOK:
        return p->identified_books[it->id];
        break;

    case IT_POTION:
        return p->identified_potions[it->id];
        break;

    case IT_RING:
        return p->identified_rings[it->id];
        break;

    case IT_SCROLL:
        return p->identified_scrolls[it->id];
        break;

    default:
        return TRUE;
    }
}

char *player_item_identified_list(player *p)
{
    GString *list, *sublist;
    char buf_unidentified[81], buf_identified[81];
    item *it; /* fake pretend item */
    char *heading;

    int type, id, count;

    item_t type_ids[] =
    {
        IT_BOOK,
        IT_POTION,
        IT_RING,
        IT_SCROLL,
    };

    assert (p != NULL);

    list = g_string_new(NULL);
    it = item_new(IT_BOOK, 1, 0);

    for (type = 0; type < 4; type++)
    {
        count = 0;
        sublist = g_string_new(NULL);

        /* item category header */
        heading = g_strdup(item_get_name_pl(type_ids[type]));
        heading[0] = g_ascii_toupper(heading[0]);

        /* no linefeed before first category */
        g_string_append_printf(sublist, (type == 0) ? "%s\n" : "\n%s\n",
                               heading);

        g_free(heading);

        it->type = type_ids[type];

        for (id = 1; id < item_get_max_id(type_ids[type]); id++)
        {
            it->id = id;
            if (player_item_identified(p, it))
            {

                item_describe(it, FALSE, TRUE, FALSE, buf_unidentified, 80);
                item_describe(it, TRUE, TRUE, FALSE, buf_identified, 80);
                g_string_append_printf(sublist, " %33s - %s \n",
                                       buf_unidentified, buf_identified);
                count++;
            }
        }

        if (count)
        {
            g_string_append(list, sublist->str);
        }

        g_string_free(sublist, TRUE);
    }
    item_destroy(it);

    return g_string_free(list, FALSE);
}

void player_item_identify(player *p, item *it)
{
    assert(p != NULL && it != NULL);

    switch (it->type)
    {
    case IT_BOOK:
        p->identified_books[it->id] = TRUE;
        break;

    case IT_POTION:
        p->identified_potions[it->id] = TRUE;
        break;

    case IT_RING:
        p->identified_rings[it->id] = TRUE;
        break;

    case IT_SCROLL:
        p->identified_scrolls[it->id] = TRUE;
        break;

    default:
        /* NOP */
        break;
    }

    if (it->cursed)
        it->curse_known = TRUE;

    if (it->blessed)
        it->blessed_known = TRUE;

    it->bonus_known = TRUE;
}

int player_item_use(player *p, item *it)
{
    char description[61];
    int item_used_up = TRUE;
    int item_identified = TRUE;
    int damage = 0; /* damage take by cursed items */
    int time = 0; /* number of turns this action took */

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (player_effect(p, ET_BLINDNESS) && (it->type == IT_BOOK || it->type == IT_SCROLL))
    {
        log_add_entry(p->log, "As you are blind you can't read %s.",
                      item_describe(it, player_item_identified(p, it),
                                    TRUE, TRUE, description, 60));
        return time;
    }

    switch (it->type)
    {

        /* read book */
    case IT_BOOK:
        item_describe(it, player_item_identified(p, it),
                      TRUE, TRUE, description, 60);

        log_add_entry(p->log, "You read %s.", description);

        /* cursed spellbooks have nasty effects */
        if (it->cursed)
        {
            log_add_entry(p->log, "There was something wrong with this book!");
            player_mp_lose(p, rand_0n(p->mp));
        }
        else
        {
            item_used_up = FALSE;
            switch (player_spell_learn(p, it->id))
            {
            case 0:
                log_add_entry(p->log, "You cannot understand the content of this book.");
                item_identified = FALSE;
                break;

            case 1:
                /* learnt spell */
                log_add_entry(p->log,
                              "You master the spell \"%s\".",
                              book_name(it));

                break;

            default:
                /* improved knowledge of spell */
                log_add_entry(p->log,
                              "You improved your knowledge of the spell %s.",
                              book_name(it));
                break;
            }

            /* five percent chance to increase intelligence */
            if (chance(5))
            {
                log_add_entry(p->log, "Reading makes you ingenious.");
                p->intelligence++;
            }
        }
        time = 2 + spell_level_by_id(it->id);

        break;


    case IT_CONTAINER:
        break;


        /* eat food */
    case IT_FOOD:
        item_describe(it, player_item_identified(p, it),
                      TRUE, FALSE, description, 60);

        log_add_entry(p->log, "You eat %s.", description);

        if (it->id == FT_FORTUNE_COOKIE)
        {
            log_add_entry(p->log,
                          "It has a piece of paper inside. It reads: \"%s\"",
                          food_get_fortune(game_fortunes(p->game)));
        }

        time = 2;

        break;


        /* drink potion */
    case IT_POTION:
        item_describe(it, player_item_identified(p, it),
                      TRUE, FALSE, description, 60);

        log_add_entry(p->log, "You drink %s.", description);

        if (it->cursed)
        {
            log_add_entry(p->log, "The Potion is foul!");
            if ((damage = rand_0n(p->hp)))
            {
                log_add_entry(p->log, "You vomit blood!");
                player_hp_lose(p, it->type, PD_CURSE, damage);
            }
        }
        else
        {

            switch (it->id)
            {
            case PO_OBJ_DETECT:
            case PO_TRE_DETECT:
                item_identified = potion_detect_item(p, it);
                break;

            case PO_WATER:
                log_add_entry(p->log, "This tastes like water..");
                break;

            case PO_CURE_DIANTHR:
                log_add_entry(p->log, "You really want to keep the potion for your daughter.");
                item_used_up = FALSE;
                break;

            default:
                item_identified = potion_with_effect(p, it);
                break;
            }
        }
        time = 2;
        break;


        /* read scroll */
    case IT_SCROLL:
        item_describe(it, player_item_identified(p, it),
                      TRUE, FALSE, description, 60);

        log_add_entry(p->log, "You read %s.", description);

        if (it->cursed)
        {
            log_add_entry(p->log, "The Scroll explodes!");
            if ((damage = rand_0n(p->hp)))
            {
                log_add_entry(p->log, "You are hurt by the explosion!");
                player_hp_lose(p, damage, PD_CURSE, it->type);
            }
        }
        else
        {
            switch (it->id)
            {
            case ST_ENCH_ARMOUR:
                item_identified = scroll_enchant_armour(p, it);
                break;

            case ST_ENCH_WEAPON:
                item_identified = scroll_enchant_weapon(p, it);
                break;

            case ST_BLANK:
                item_used_up = FALSE;
                log_add_entry(p->log, "This scroll is blank.");
                break;

            case ST_CREATE_MONSTER:
                item_identified = player_magic_create_monster(p);
                break;

            case ST_CREATE_ARTIFACT:
                item_identified = scroll_create_artefact(p, it);
                break;

            case ST_TIMEWARP:
                item_identified = scroll_timewarp(p, it);
                break;

            case ST_TELEPORT:
                item_identified = player_teleport(p);
                break;

            case ST_HEAL_MONSTER:
                item_identified = scroll_heal_monster(p, it);
                break;

            case ST_MAPPING:
                log_add_entry(p->log, "There is a map on the scroll!");
                item_identified = scroll_mapping(p, it);
                break;

            case ST_GEM_PERFECTION:
                item_identified = scroll_gem_perfection(p, it);
                break;

            case ST_SPELL_EXTENSION:
                item_identified = scroll_spell_extension(p, it);
                break;

            case ST_IDENTIFY:
                item_identified = scroll_identify(p, it);
                break;

            case ST_REMOVE_CURSE:
                item_identified = scroll_remove_curse(p, it);
                break;

            case ST_ANNIHILATION:
                item_identified = scroll_annihilate(p, it);
                break;

            case ST_PULVERIZATION:
                player_magic_vaporize_rock(p);
                break;

            default:
                item_identified = scroll_with_effect(p, it);
                break;
            }

            if (!item_identified)
            {
                log_add_entry(p->log, "Nothing happens.");
            }
        }

        time = 2;
        break;

    default:
        item_describe(it, player_item_identified(p, it),
                      TRUE, FALSE, description, 40);

        log_add_entry(p->log,
                      "I have absolutely no idea how to use %s.",
                      description);

        item_used_up = FALSE;
    }

    if (item_identified)
    {
        player_item_identify(p, it);
    }

    if (item_used_up)
    {
        if (it->count > 1)
        {
            it->count--;
        }
        else
        {
            inv_del_element(p->inventory, it);
        }
    }

    return time;
}

int player_item_drop(player *p, item *it)
{
    char desc[61];
    int count;

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (player_item_is_equipped(p, it))
        return FALSE;

    if (it->count > 1)
    {
        g_snprintf(desc, 60, "Drop how many %s?", item_get_name_pl(it->type));

        count = display_get_count(desc, it->count);

        if (count < it->count)
            it = item_split(it, count);
    }

    inv_del_element(p->inventory, it);

    if (level_ilist_at(p->level, p->pos) == NULL)
        level_ilist_at(p->level, p->pos) = inv_new();

    inv_add(level_ilist_at(p->level, p->pos), it);
    log_add_entry(p->log,
                  "You drop %s.",
                  item_describe(it,
                                player_item_identified(p, it),
                                FALSE, FALSE,
                                desc, 60));

    return TRUE;
}

int player_item_pickup(player *p, item *it)
{
    char desc[61];
    int count;

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (it->count > 1 && it->type != IT_GOLD)
    {
        g_snprintf(desc, 60, "Pick up how many %s?", item_get_name_pl(it->type));

        count = display_get_count(desc, it->count);

        if (count < it->count)
            it = item_split(it, count);
    }

    inv_del_element(level_ilist_at(p->level, p->pos), it);

    log_add_entry(p->log,
                  "You pick up %s.",
                  item_describe(it,
                                player_item_identified(p, it),
                                FALSE, FALSE,
                                desc, 60));

    /* this has to come after the logging as it can be freed at this point
       ->stackable item */
    inv_add(p->inventory, it);

    /* one turn to pick item up, one to stuff it into the pack */
    return 2;
}

int player_item_buy(player *p, item *it)
{
    int price;
    int count = 0;
    int player_gold;
    char text[81];
    char name[41];

    item *it_clone;

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    player_gold = player_get_gold(p);
    price = item_price(it);

    if (it->count > 1)
    {
        item_describe(it, player_item_identified(p, it), FALSE, FALSE, name, 40);
        g_snprintf(text, 80, "How many %s do you want to buy?", name);

        /* get count */
        count = display_get_count(text, it->count);
        price *= count;

        if (player_gold < price)
        {
            display_paint_screen(p);

            it_clone = item_clone(it);
            it_clone->count = count;

            item_describe(it, player_item_identified(p, it), FALSE, TRUE, name, 40);
            g_snprintf(text, 80, "You cannot afford the %d gold for %s.",
                       price, name);

            item_destroy(it_clone);

            display_show_message(NULL, text);

            return FALSE;
        }
    }
    else
    {
        item_describe(it, player_item_identified(p, it), TRUE, TRUE, name, 40);
        g_snprintf(text, 80, "Do you want to buy %s for %d gold?",
                   name, price);

        if (!display_get_yesno(text, NULL, NULL))
        {
            return FALSE;
        }
    }

    player_set_gold(p, player_gold - price);

    if ((it->count > 1) && (count < it->count))
    {
        /* split the item */
        inv_add(p->inventory, item_split(it, count));
    }
    else
    {
        inv_add(p->inventory, item_clone(it));
        it->count = 0;
    }

    return TRUE;
}

int player_item_sell(player *p, item *it)
{
    int price;
    int count = 0;
    char question[81];
    char name[41];

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    item_describe(it, player_item_identified(p, it), FALSE, FALSE, name, 40);

    price = item_price(it);

    /* modify price if player sells stuff at the trading post */
    if (level_stationary_at(p->level, p->pos) == LS_TRADEPOST)
    {
        if (!player_item_is_damaged(p, it))
        {
            /* good items: 20% of value */
            price /= 5;
        }
        else
        {
            /* damaged items: 10% of value */
            price /= 10;
        }
    }

    if (price < 1)
        price = 1;

    if (it->count > 1)
    {
        item_describe(it, player_item_identified(p, it), FALSE, FALSE, name, 40);
        g_snprintf(question, 80, "How many %s do you want to sell for %d gold?",
                   name, price);

        /* get count */
        count = display_get_count(question, it->count);
        price *= count;
    }
    else
    {
        item_describe(it, player_item_identified(p, it), TRUE, TRUE, name, 40);
        g_snprintf(question, 80, "Do you want to sell %s for %d gold?",
                   name, price);

        if (!display_get_yesno(question, NULL, NULL))
        {
            return FALSE;
        }
    }

    player_set_gold(p, player_get_gold(p) + price);

    if ((it->count > 1) && (count < it->count))
    {
        it->count -= count;
    }
    else
    {
        inv_del_element(p->inventory, it);
    }

    return TRUE;
}

int player_item_shop_identify(player *p, item *it)
{
    int player_gold;
    int price;
    char name[41];
    char message[81];

    const char title[] = "Identify item";

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    player_gold = player_get_gold(p);
    price = 50 << game_difficulty(p->game);

    item_describe(it, player_item_identified(p, it), TRUE, TRUE, name, 40);

    if (price <= player_gold)
    {
        g_snprintf(message, 80, "Pay %d gold to identify the %s?", price, name);

        if (display_get_yesno(message, NULL, NULL))
        {
            player_item_identify(p, it);
            player_set_gold(p, player_gold - price);

            return TRUE;
        }
    }
    else
    {
        g_snprintf(message, 80, "Identifying the %s costs %d gold.", name, price);
        display_show_message((char *)title, message);
    }

    return FALSE;
}

int player_item_shop_repair(player *p, item *it)
{
    int damages = 0;
    int player_gold;
    int price;
    char name[41];
    char message[81];

    const char title[] = "Repair item";

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* determine how much the item is damaged */
    damages += it->burnt;
    damages += it->corroded;
    damages += it->rusty;

    player_gold = player_get_gold(p);
    price = (50 << game_difficulty(p->game)) * damages;

    item_describe(it, player_item_identified(p, it), TRUE, TRUE, name, 40);

    if (price <= player_gold)
    {
        g_snprintf(message, 80, "Pay %d gold to repair the %s?", price, name);

        if (display_get_yesno(message, NULL, NULL))
        {
            it->burnt = 0;
            it->corroded = 0;
            it->rusty = 0;

            player_set_gold(p, player_gold - price);

            return TRUE;
        }
    }
    else
    {
        g_snprintf(message, 80, "Repairing the %s costs %d gold.", name, price);
        display_show_message((char *)title, message);
    }

    return FALSE;
}

int player_effect(player *p, int effect_type)
{
    assert(p != NULL && effect_type > ET_NONE && effect_type < ET_MAX);
    return effect_query(p->effects, effect_type);
}

void player_effect_add(player *p, effect *e)
{

    assert(p != NULL && e != NULL);

    if (effect_get_msg_start(e))
        log_add_entry(p->log, "%s", effect_get_msg_start(e));

    /* one-time effects are handled here */
    if (e->turns == 1)
    {
        switch (e->type)
        {
        case ET_INC_CHA:
            p->charisma += e->amount;
            break;

        case ET_INC_CON:
            p->constitution += e->amount;
            break;

        case ET_INC_DEX:
            p->dexterity += e->amount;
            break;

        case ET_INC_INT:
            p->intelligence += e->amount;
            break;

        case ET_INC_STR:
            p->strength += e->amount;
            break;

        case ET_INC_WIS:
            p->wisdom += e->amount;
            break;

        case ET_INC_RND:
            player_effect_add(p, effect_new(rand_m_n(ET_INC_CHA, ET_INC_WIS), game_turn(p->game)));
            break;

        case ET_INC_HP_MAX:
            p->hp_max += ((player_get_hp_max(p) / 100) * e->amount);
            break;

        case ET_INC_MP_MAX:
            p->mp_max += ((player_get_mp_max(p) / 100) * e->amount);
            break;

        case ET_INC_LEVEL:
            player_lvl_gain(p, e->amount);
            break;

        case ET_INC_EXP:
            /* looks like a reasonable amount */
            player_exp_gain(p, rand_1n(player_lvl_exp[p->lvl] - player_lvl_exp[p->lvl - 1]));
            break;

        case ET_FIRE_RESISTANCE:
            p->fire_resistance += e->amount;
            break;

        case ET_COLD_RESISTANCE:
            p->cold_resistance += e->amount;
            break;

        case ET_MAGIC_RESISTANCE:
            p->magic_resistance += e->amount;
            break;

        case ET_INC_HP:
            player_hp_gain(p, (int)(((float)p->hp_max / 100) * e->amount));
            break;

        case ET_MAX_HP:
            player_hp_gain(p, player_get_hp_max(p));
            break;

        case ET_INC_MP:
            player_mp_gain(p, (int)(((float)p->mp_max / 100) * e->amount));
            break;

        case ET_MAX_MP:
            player_mp_gain(p, player_get_mp_max(p));
            break;

        case ET_DEC_CHA:
            p->charisma -= e->amount;
            break;

        case ET_DEC_CON:
            p->constitution -= e->amount;
            break;

        case ET_DEC_DEX:
            p->dexterity -= e->amount;
            break;

        case ET_DEC_INT:
            p->intelligence -= e->amount;
            break;

        case ET_DEC_STR:
            p->strength -= e->amount;
            break;

        case ET_DEC_WIS:
            p->wisdom -= e->amount;
            break;

        case ET_DEC_RND:
            player_effect_add(p, effect_new(rand_m_n(ET_DEC_CHA, ET_DEC_WIS), game_turn(p->game)));
            break;

        case ET_DEC_HP_MAX:
            p->hp_max -= ((player_get_hp_max(p) / 100) * e->amount);
            break;

        case ET_DEC_MP_MAX:
            p->mp_max -= ((player_get_mp_max(p) / 100) * e->amount);
            break;

        case ET_DEC_LEVEL:
            player_lvl_lose(p, e->amount);
            break;

        case ET_DEC_EXP:
            /* looks like a reasonable amount */
            player_exp_lose(p, rand_1n(player_lvl_exp[p->lvl] - player_lvl_exp[p->lvl - 1]));
            break;
        }
        effect_destroy(e);
    }
    else if (e->type == ET_SLEEP)
    {
        game_spin_the_wheel(p->game, e->turns);
        effect_destroy(e);
    }
    else
    {
        effect_add(p->effects, e);
    }
}

int player_effect_del(player *p, effect *e)
{
    assert(p != NULL && e != NULL && e->type > ET_NONE && e->type < ET_MAX);

    if (effect_get_msg_stop(e))
        log_add_entry(p->log, "%s", effect_get_msg_stop(e));

    return effect_del(p->effects, e);
}

effect *player_effect_get(player *p, int effect_id)
{
    assert(p != NULL && effect_id > ET_NONE && effect_id < ET_MAX);
    return effect_get(p->effects, effect_id);
}

void player_effects_expire(player *p, int turns)
{
    int i = 1;
    effect *e;

    assert(p != NULL);

    while (i <= p->effects->len)
    {
        e = g_ptr_array_index(p->effects, i - 1);

        if (effect_expire(e, turns) == -1)
        {
            /* effect has expired */
            player_effect_del(p, e);
            effect_destroy(e);
        }
        else
        {
            i ++;
        }
    }
}

int player_get_ac(player *p)
{
    int ac = 0;
    assert(p != NULL);

    if (p->eq_boots != NULL)
        ac += armour_ac(p->eq_boots);

    if (p->eq_cloak != NULL)
        ac += armour_ac(p->eq_cloak);

    if (p->eq_gloves != NULL)
        ac += armour_ac(p->eq_gloves);

    if (p->eq_helmet != NULL)
        ac += armour_ac(p->eq_helmet);

    if (p->eq_shield != NULL)
        ac += armour_ac(p->eq_shield);

    if (p->eq_suit != NULL)
        ac += armour_ac(p->eq_suit);

    ac += player_effect(p, ET_PROTECTION);
    ac += player_effect(p, ET_DIVINE_PROTECTION);
    ac += player_effect(p, ET_INVULNERABILITY);

    return ac;
}

int player_get_wc(player *p)
{
    int wc = 0;
    assert(p != NULL);

    if (p->eq_weapon != NULL)
        wc += weapon_wc(p->eq_weapon);

    wc += player_effect(p, ET_INC_DAMAGE);
    wc -= player_effect(p, ET_DEC_DAMAGE);

    /* minimal damage */
    if (wc < 1)
        wc = 1;

    return wc;
}

int player_get_hp_max(player *p)
{
    assert(p != NULL);
    return p->hp_max
           + player_effect(p, ET_INC_HP_MAX)
           - player_effect(p, ET_DEC_HP_MAX);
}

int player_get_mp_max(player *p)
{
    assert(p != NULL);
    return p->mp_max
           + player_effect(p, ET_INC_MP_MAX)
           - player_effect(p, ET_DEC_MP_MAX);
}

int player_get_str(player *p)
{
    assert(p != NULL);
    return p->strength
           + player_effect(p, ET_INC_STR)
           - player_effect(p, ET_DEC_STR)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_int(player *p)
{
    assert(p != NULL);
    return p->intelligence
           + player_effect(p, ET_INC_INT)
           - player_effect(p, ET_DEC_INT)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_wis(player *p)
{
    assert(p != NULL);
    return p->wisdom
           + player_effect(p, ET_INC_WIS)
           - player_effect(p, ET_DEC_WIS)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_con(player *p)
{
    assert(p != NULL);
    return p->constitution
           + player_effect(p, ET_INC_CON)
           - player_effect(p, ET_DEC_CON)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_dex(player *p)
{
    assert(p != NULL);
    return p->dexterity
           + player_effect(p, ET_INC_DEX)
           - player_effect(p, ET_DEC_DEX)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_cha(player *p)
{
    assert(p != NULL);
    return p->charisma
           + player_effect(p, ET_INC_CHA)
           - player_effect(p, ET_DEC_CHA)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_gold(player *p)
{
    int pos;
    item *i;

    assert(p != NULL);

    for (pos = 1; pos <= inv_length(p->inventory); pos++)
    {
        i = inv_get(p->inventory, pos - 1);
        if (i->type == IT_GOLD)
            return i->count;
    }

    return 0;
}

int player_set_gold(player *p, int amount)
{
    int pos;
    item *i;

    assert(p != NULL);

    for (pos = 1; pos <= inv_length(p->inventory); pos++)
    {
        i = inv_get(p->inventory, pos - 1);
        if (i->type == IT_GOLD)
        {
            i->count = amount;
            return i->count;
        }
    }

    /* no gold found -> generate new gold */
    i = item_new(IT_GOLD, amount, 0);
    inv_add(p->inventory, i);

    return i->count;
}

char *player_get_lvl_desc(player *p)
{
    assert(p != NULL);
    return (char *)player_lvl_desc[p->lvl];
}

static int player_level_leave(player *p)
{
    assert(p != NULL);

    /* store the last turn player has been on this level */
    p->level->visited = game_turn(p->game);

    return TRUE;
}

/**
 * Player stepped on a trap
 *
 * @param the player
 * @param the trap
 * @return time this move took
 */
static int player_trigger_trap(player *p, trap_t trap)
{
    /* additional time of turn, if any */
    int time = 0;

    /* chance to trigger the trap on target tile */
    int possibility;

    possibility = trap_chance(trap);

    if (player_memory_of(p, p->pos).trap == trap)
        /* if player knows the trap a little chance remains */
        possibility = 5;

    if (chance(possibility))
    {
        log_add_entry(p->log, trap_p_message(trap));

        /* refresh player's knowlege of trap */
        player_memory_of(p, p->pos).trap = trap;

        switch (trap)
        {
        case TT_TRAPDOOR:
            time += player_level_enter(p, (p->game)->levels[p->level->nlevel + 1]);
            /* fall through to TT_TELEPORT to find a new space */

        case TT_TELEPORT:
            p->pos = level_find_space(p->level, LE_MONSTER);
            break;

        default:
            if (trap_damage(trap))
                player_hp_lose(p, rand_1n(trap_damage(trap)) + p->level->nlevel,
                               PD_TRAP, trap);

            /* if there is an effect on the trap add it to player's effects. */
            if (trap_effect(trap) && chance(trap_effect_chance(trap)))
            {
                /* display message if there is one */
                if (trap_e_message(trap))
                {
                    log_add_entry(p->log, trap_e_message(trap));
                }

                player_effect_add(p, effect_new(trap_effect(trap), game_turn(p->game)));
            }

        }

    }
    else if (player_memory_of(p, p->pos).trap == trap)
    {
        log_add_entry(p->log,
                      "You evade the %s.",
                      trap_description(trap));
    }

    return time;
}

/* this and the following function have been
 * ported from python to c using the example at
 * http://roguebasin.roguelikedevelopment.org/index.php?title=Python_shadowcasting_implementation
 */
void player_update_fov(player *p, int radius)
{
    int octant;
    position pos;
    item *it;

    area *enlight;

    const int mult[4][8] =
    {
        { 1,  0,  0, -1, -1,  0,  0,  1 },
        { 0,  1, -1,  0,  0, -1,  1,  0 },
        { 0,  1,  1,  0,  0, -1, -1,  0 },
        { 1,  0,  0,  1, -1,  0,  0, -1 }
    };

    /* reset FOV */
    memset(&(p->fov), 0, LEVEL_SIZE * sizeof(int));

    /* if player is enlightened, use a circular area around the player
     * otherwise fov algorithm
     */

    if (player_effect(p, ET_ENLIGHTENMENT))
    {
        int x, y;

        enlight = area_new_circle(p->pos, player_effect(p, ET_ENLIGHTENMENT));

        /* set visible field according to returned area */
        for (y = 0; y <  enlight->size_y; y++)
        {
            for (x = 0; x < enlight->size_x; x++)
            {
                pos.x = x + enlight->start_x;
                pos.y = y + enlight->start_y;

                if (pos_valid(pos) && area_point_get(enlight, x, y))
                    player_pos_visible(p,pos) = TRUE;
            }
        }

        area_destroy(enlight);
    }
    else
    {
        /* determine which fields are visible */
        for (octant = 0; octant < 8; octant++)
        {
            player_calculate_octant(p, 1, 1.0, 0.0, radius,
                                    mult[0][octant], mult[1][octant],
                                    mult[2][octant], mult[3][octant]);

        }
        p->fov[p->pos.y][p->pos.x] = TRUE;
    }

    /* update visible fields in player's memory */
    for (pos.y = 0; pos.y < LEVEL_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < LEVEL_MAX_X; pos.x++)
        {
            if (player_pos_visible(p, pos))
            {
                player_memory_of(p,pos).type = level_tiletype_at(p->level, pos);
                player_memory_of(p,pos).stationary = level_stationary_at(p->level, pos);

                if (level_ilist_at(p->level, pos)
                        && inv_length(level_ilist_at(p->level, pos)))
                {
                    it = inv_get(level_ilist_at(p->level, pos), inv_length(level_ilist_at(p->level, pos)) - 1);
                    player_memory_of(p,pos).item = it->type;
                }
                else
                {
                    player_memory_of(p,pos).item = IT_NONE;
                }
            }
        }
    }
}

static void player_calculate_octant(player *p, int row, float start,
                                    float end, int radius, int xx,
                                    int xy, int yx, int yy)
{
    int radius_squared;
    int j;
    int dx, dy;
    int X, Y;
    int blocked;
    float l_slope, r_slope;
    float new_start = 0;

    if (start < end)
        return;

    radius_squared = radius * radius;

    for (j  = row; j <= radius + 1; j++)
    {
        dx = -j - 1;
        dy = -j;

        blocked = FALSE;

        while (dx <= 0)
        {
            dx += 1;

            /* Translate the dx, dy coordinates into map coordinates: */
            X = p->pos.x + dx * xx + dy * xy;
            Y = p->pos.y + dx * yx + dy * yy;

            /* check if coordinated are within bounds */
            if ((X < 0) || (X >= LEVEL_MAX_X))
                continue;

            if ((Y < 0) || (Y >= LEVEL_MAX_Y))
                continue;

            /* l_slope and r_slope store the slopes of the left and right
             * extremities of the square we're considering: */
            l_slope = (dx - 0.5) / (dy + 0.5);
            r_slope = (dx + 0.5) / (dy - 0.5);

            if (start < r_slope)
            {
                continue;
            }
            else if (end > l_slope)
            {
                break;
            }
            else
            {
                /* Our light beam is touching this square; light it */
                if ((dx * dx + dy * dy) < radius_squared)
                {
                    p->fov[Y][X] = TRUE;
                }

                if (blocked)
                    /* we're scanning a row of blocked squares */
                    if (!level_pos_transparent(p->level, pos_new(X,Y)))
                    {
                        new_start = r_slope;
                        continue;
                    }
                    else
                    {
                        blocked = FALSE;
                        start = new_start;
                    }
                else
                {
                    if (!level_pos_transparent(p->level, pos_new(X,Y)) && (j < radius))
                        /* This is a blocking square, start a child scan */
                        blocked = TRUE;

                    player_calculate_octant(p, j+1, start, l_slope, radius, xx, xy, yx, yy);
                    new_start = r_slope;
                }
            }
        }

        /* Row is scanned; do next row unless last square was blocked */
        if (blocked)
        {
            break;
        }
    }
}

static void player_magic_alter_reality(player *p)
{
    level *nlevel, *olevel;

    olevel = p->level;

    /* create new level */
    nlevel = g_malloc0(sizeof (level));
    nlevel->nlevel = olevel->nlevel;

    level_new(nlevel,
              game_difficulty(p->game),
              game_mazefile(p->game));

    /* make new level active */
    p->game->levels[p->level->nlevel] = nlevel;
    p->level = nlevel;

    /* reposition player (if needed) */
    if (!level_pos_passable(nlevel, p->pos))
    {
        p->pos = level_find_space(nlevel, LE_MONSTER);
    }

    /* destroy old level */
    level_destroy(olevel);
}

static int player_magic_create_monster(player *p)
{
    monster *m;

    position pos;

    /* this spell doesn't work in town */
    if (p->level->nlevel == 0)
    {
        log_add_entry(p->log, "Nothing happens.");
        return FALSE;
    }

    /* try to find a space for the monster near the player */
    pos = level_find_space_in(p->level,
                              rect_new_sized(p->pos, 2),
                              LE_MONSTER);

    if (pos_valid(pos))
    {
        m = monster_new_by_level(p->level);
        monster_move(m, pos);

        return TRUE;
    }
    else
    {
        log_add_entry(p->log, "You feel failure.");
        return FALSE;
    }
}

static void player_magic_create_sphere(player *p)
{
    position pos;

    assert(p != NULL);

    pos = display_get_position(p, "Where do you want to place the sphere?",
                               FALSE, TRUE);

    if (pos_valid(pos))
    {
        g_ptr_array_add(p->level->slist, sphere_new(pos, p, p->lvl * 10));
    }
    else
    {
        log_add_entry(p->log, "Huh?");
    }
}

static void player_magic_genocide_monster(player *p)
{
    char in;
    int id;

    assert(p != NULL);

    log_add_entry(p->log, "Whih monster do you want to genocide (type letter)?");
    display_paint_screen(p);

    in = getch();

    for (id = 1; id < MT_MAX; id++)
    {
        if (monster_get_image_by_type(id) == in)
        {
            if (!monster_is_genocided(id))
            {
                monster_genocide(id);
                log_add_entry(p->log,
                              "Wiped out all %ss",
                              monster_get_name_by_type(id));

                monsters_genocide(p->level);
            }

            return;
        }
    }

    log_add_entry(p->log, "No such monster.");
}

static void player_magic_make_wall(player *p)
{
    position pos;

    pos = display_get_position(p, "Select a position where you want to place a wall.", FALSE, TRUE);

    if (pos_identical(pos, p->pos))
    {
        log_add_entry(p->log, "You are actually standing there.");
        return;
    }
    else if (!pos_valid(pos))
    {
        log_add_entry(p->log, "No wall today.");
        return;
    }

    if (level_tiletype_at(p->level, pos) != LT_WALL)
    {
        level_tiletype_at(p->level, pos) = LT_WALL;

        /* destroy all items at that position */
        if (level_ilist_at(p->level, pos))
        {
            inv_destroy(level_ilist_at(p->level, pos));
            level_ilist_at(p->level, pos) = NULL;
        }

        log_add_entry(p->log, "You have created a wall.");
    }
    else
    {
        log_add_entry(p->log, "There was a wall already..");
    }
}

static void player_magic_vaporize_rock(player *p)
{
    position pos;
    monster *m = NULL;
    char *desc = NULL;

    pos = display_get_position(p, "What do you want to vaporize?", FALSE, FALSE);

    if (!pos_valid(pos))
    {
        log_add_entry(p->log, "So you chose not to vaprize anything.");
        return;
    }

    if (level_tiletype_at(p->level, pos) == LT_WALL)
    {
        level_tiletype_at(p->level, pos) = LT_FLOOR;
        return;
    }

    if ((m = level_get_monster_at(p->level, pos)) && (m->type == MT_XORN))
    {
        /* xorns take damage from vpr */
        if (monster_hp_lose(m, divert(200, 10)))
        {
            player_monster_kill(p, m);
        }
    }

    switch (level_stationary_at(p->level, pos))
    {
    case LS_ALTAR:
        m = monster_new(MT_DAEMON_PRINCE, p->level);
        desc = "altar";
        break;

    case LS_FOUNTAIN:
        m = monster_new(MT_WATER_LORD, p->level);
        desc = "fountain";
        break;

    case LS_STATUE:
        if (game_difficulty(p->game) < 3)
        {
            if (!level_ilist_at(p->level, pos))
                level_ilist_at(p->level, pos) = inv_new();

            inv_add(level_ilist_at(p->level, pos),
                    item_new(IT_BOOK, rand_1n(SP_MAX - 1), 0));
        }

        desc = "statue";
        break;

    case LS_THRONE:
    case LS_THRONE2:
        m = monster_new(MT_GNOME_KING, p->level);
        desc = "throne";
        break;

    case LS_DEADFOUNTAIN:
    case LS_DEADTHRONE:
        level_stationary_at(p->level, pos) = LS_NONE;
        break;

    default:
        log_add_entry(p->log, "Somehow that did not work.");
        /* NOP */
    }

    if (desc)
    {
        log_add_entry(p->log, "You destroy the %s.", desc);
        level_stationary_at(p->level, pos) = LS_NONE;
    }

    /* created a monster - position it correctly */
    if (m)
    {
        monster_move(m, pos);
    }
}

static char *player_death_description(game_score_t *score, int verbose)
{
    char *desc;
    GString *text;

    assert(score != NULL);

    switch (score->cod)
    {
    case PD_LASTLEVEL:
        desc = "passed away";
        break;

    case PD_TOO_LATE:
        desc = "returned with the potion too late";
        break;

    case PD_WON:
        desc = "returned in time with the cure";
        break;

    case PD_LOST:
        desc = "could not find the potion in time";
        break;

    case PD_QUIT:
        desc = "quit the game";
        break;

    default:
        desc = "killed";
    }

    text = g_string_new_len(NULL, 200);

    g_string_append_printf(text, "%s (%c), %s",
                           score->player_name, score->sex ? 'm' : 'f', desc);

    if (verbose)
    {
        g_string_append_printf(text, " on level %d", score->dlevel);

        if (score->dlevel_max > score->dlevel)
        {
            g_string_append_printf(text, " (max. %d)", score->dlevel_max);
        }

        if (score->cod < PD_TOO_LATE)
        {
            g_string_append_printf(text, " with %d and a maximum of %d hp",
                                   score->hp, score->hp_max);
        }
    }

    switch (score->cod)
    {
    case PD_EFFECT:
        /* currently only poison can lead to death */
        g_string_append(text, " by poison.");
        break;

    case PD_LASTLEVEL:
        g_string_append_printf(text,". %s left %s body.",
                               score->sex ? "He" : "She",
                               score->sex ? "his" : "her");
        break;

    case PD_MONSTER:
        /* TODO: regard monster's invisibility */
        /* TODO: while sleeping / doing sth. */
        g_string_append_printf(text, " by a%s %s.",
                               a_an(monster_get_name_by_type(score->cause)),
                               monster_get_name_by_type(score->cause));
        break;

    case PD_SPHERE:
        g_string_append(text, " by a sphere of destruction.");
        break;

    case PD_TRAP:
        g_string_append_printf(text, " by a %s.", trap_description(score->cause));
        break;

    case PD_LEVEL:
        g_string_append_printf(text, " by %s.", lt_get_desc(score->cause));
        break;

    case PD_SPELL:
        g_string_append_printf(text, " %s with the spell \"%s\".",
                               score->sex ? "himself" : "herself",
                               spell_name_by_id(score->cause));
        break;

    case PD_CURSE:
        g_string_append_printf(text, " by a cursed %s.",
                               item_get_name_sg(score->cause));
        break;

    default:
        /* no further description */
        g_string_append(text, ".");
        break;
    }

    if (verbose)
    {
        g_string_append_printf(text, " %s has scored %" G_GINT64_FORMAT " points.",
                               score->sex ? "He" : "She", score->score);
    }

    return g_string_free(text, FALSE);
}
