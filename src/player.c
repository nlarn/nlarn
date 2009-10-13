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

#include <assert.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "container.h"
#include "display.h"
#include "food.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"

static const char aa1[] = "mighty evil master";
static const char aa2[] = "apprentice demi-god";
static const char aa3[] = "minor demi-god";
static const char aa4[] = "major demi-god";
static const char aa5[] = "minor deity";
static const char aa6[] = "major deity";
static const char aa7[] = "novice guardian";
static const char aa8[] = "apprentice guardian";
static const char aa9[] = "The Creator";

static const char *player_level_desc[] =
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
static const guint32 player_lvl_exp[] =
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

static int player_trap_trigger(player *p, trap_t trap);
static void player_calculate_octant(player *p, int row, float start, float end, int radius, int xx, int xy, int yx, int yy);

static cJSON *player_memory_serialize(player *p, position pos);
static char *player_death_description(game_score_t *score, int verbose);

player *player_new()
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

    p->level = p->stats.max_level = 1;

    p->known_spells = g_ptr_array_new();
    p->effects = g_ptr_array_new();
    p->inventory = inv_new(p);

    inv_callbacks_set(p->inventory, &player_inv_pre_add, &player_inv_weight_recalc,
                      NULL, &player_inv_weight_recalc);

    it = item_new(IT_ARMOUR, AT_LEATHER, 1);
    inv_add(&p->inventory, it);
    player_item_equip(p, NULL, it);

    it = item_new(IT_WEAPON, WT_DAGGER, 0);
    it->bonus_known = TRUE;
    inv_add(&p->inventory, it);
    player_item_equip(p, NULL, it);

    /* start a new diary */
    p->log = log_new();

    /* potion of cure dianthroritis is always known */
    p->identified_potions[PO_CURE_DIANTHR] = TRUE;

    return p;
}

void player_destroy(player *p)
{
    guint idx;
    effect *eff;

    assert(p != NULL);

    /* release spells */
    while ((p->known_spells)->len > 0)
        spell_destroy(g_ptr_array_remove_index_fast(p->known_spells,
                      (p->known_spells)->len - 1));

    g_ptr_array_free(p->known_spells, TRUE);

    /* release effects */
    for (idx = 0; idx < p->effects->len; idx++)
    {
        eff = g_ptr_array_index(p->effects, idx);

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

cJSON *player_serialize(player *p)
{
    cJSON *obj;
    cJSON *pser = cJSON_CreateObject();

    cJSON_AddStringToObject(pser, "name", p->name);
    cJSON_AddStringToObject(pser, "sex", p->sex ? "male" : "female");

    cJSON_AddNumberToObject(pser, "strength", p->strength);
    cJSON_AddNumberToObject(pser, "intelligence", p->intelligence);
    cJSON_AddNumberToObject(pser, "wisdom", p->wisdom);
    cJSON_AddNumberToObject(pser, "constitution", p->constitution);
    cJSON_AddNumberToObject(pser, "dexterity", p->dexterity);
    cJSON_AddNumberToObject(pser, "charisma", p->charisma);

    cJSON_AddNumberToObject(pser, "hp", p->hp);
    cJSON_AddNumberToObject(pser, "hp_max", p->hp_max);
    cJSON_AddNumberToObject(pser, "mp", p->mp);
    cJSON_AddNumberToObject(pser, "mp_max", p->mp_max);
    cJSON_AddNumberToObject(pser, "regen_counter", p->regen_counter);

    cJSON_AddNumberToObject(pser, "bank_account", p->bank_account);
    cJSON_AddNumberToObject(pser, "outstanding_taxes", p->outstanding_taxes);
    cJSON_AddNumberToObject(pser, "interest_lasttime", p->interest_lasttime);

    cJSON_AddNumberToObject(pser, "experience", p->experience);
    cJSON_AddNumberToObject(pser, "level", p->level);

    /* known spells */
    if (p->known_spells->len > 0)
    {
        cJSON_AddItemToObject(pser, "known_spells",
                              spells_serialize(p->known_spells));
    }
    /* inventory */
    if (inv_length(p->inventory) > 0)
    {
        cJSON_AddItemToObject(pser, "inventory", inv_serialize(p->inventory));
    }

    /* effects */
    if (p->effects->len > 0)
    {
        cJSON_AddItemToObject(pser, "effects", effects_serialize(p->effects));
    }

    /* equipped items */
    if (p->eq_amulet)
        cJSON_AddNumberToObject(pser, "eq_amulet",
                                GPOINTER_TO_UINT(p->eq_amulet->oid));

    if (p->eq_weapon)
        cJSON_AddNumberToObject(pser, "eq_weapon",
                                GPOINTER_TO_UINT(p->eq_weapon->oid));

    if (p->eq_boots)
        cJSON_AddNumberToObject(pser, "eq_boots",
                                GPOINTER_TO_UINT(p->eq_boots->oid));

    if (p->eq_cloak)
        cJSON_AddNumberToObject(pser, "eq_cloak",
                                GPOINTER_TO_UINT(p->eq_cloak->oid));

    if (p->eq_gloves)
        cJSON_AddNumberToObject(pser, "eq_gloves",
                                GPOINTER_TO_UINT(p->eq_gloves->oid));

    if (p->eq_helmet)
        cJSON_AddNumberToObject(pser, "eq_helmet",
                                GPOINTER_TO_UINT(p->eq_helmet->oid));

    if (p->eq_shield)
        cJSON_AddNumberToObject(pser, "eq_shield",
                                GPOINTER_TO_UINT(p->eq_shield->oid));

    if (p->eq_suit)
        cJSON_AddNumberToObject(pser, "eq_suit",
                                GPOINTER_TO_UINT(p->eq_suit->oid));

    if (p->eq_ring_l)
        cJSON_AddNumberToObject(pser, "eq_ring_l",
                                GPOINTER_TO_UINT(p->eq_ring_l->oid));

    if (p->eq_ring_r)
        cJSON_AddNumberToObject(pser, "eq_ring_r",
                                GPOINTER_TO_UINT(p->eq_ring_r->oid));

    /* identified items */
    cJSON_AddItemToObject(pser, "identified_amulets",
                          cJSON_CreateIntArray(p->identified_amulets, AM_MAX));

    cJSON_AddItemToObject(pser, "identified_books",
                          cJSON_CreateIntArray(p->identified_books, SP_MAX));

    cJSON_AddItemToObject(pser, "identified_potions",
                          cJSON_CreateIntArray(p->identified_potions, PO_MAX));

    cJSON_AddItemToObject(pser, "identified_rings",
                          cJSON_CreateIntArray(p->identified_rings, RT_MAX));

    cJSON_AddItemToObject(pser, "identified_scrolls",
                          cJSON_CreateIntArray(p->identified_scrolls, ST_MAX));

    cJSON_AddItemToObject(pser, "courses_taken",
                          cJSON_CreateIntArray(p->school_courses_taken, SCHOOL_COURSE_COUNT));

    cJSON_AddItemToObject(pser, "position", pos_serialize(p->pos));

    position pos;

    /* store players' memory of the map */
    cJSON_AddItemToObject(pser, "memory", obj = cJSON_CreateArray());

    for (pos.z = 0; pos.z < MAP_MAX; pos.z++)
    {
        cJSON *mm = cJSON_CreateArray();

        for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
            for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
                cJSON_AddItemToArray(mm, player_memory_serialize(p, pos));

        cJSON_AddItemToArray(obj, mm);
    }

    /* statistics */
    cJSON_AddItemToObject(pser, "stats", obj = cJSON_CreateObject());

    cJSON_AddNumberToObject(obj, "moves_made", p->stats.moves_made);
    cJSON_AddNumberToObject(obj, "deepest_level", p->stats.deepest_level);
    cJSON_AddItemToObject(obj, "monsters_killed",
                          cJSON_CreateIntArray(p->stats.monsters_killed, MT_MAX) );
    cJSON_AddNumberToObject(obj, "spells_cast", p->stats.spells_cast);
    cJSON_AddNumberToObject(obj, "potions_quaffed", p->stats.potions_quaffed);
    cJSON_AddNumberToObject(obj, "scrolls_read", p->stats.scrolls_read);
    cJSON_AddNumberToObject(obj, "gold_collected", p->stats.gold_collected);
    cJSON_AddNumberToObject(obj, "gold_spent", p->stats.gold_spent);
    cJSON_AddNumberToObject(obj, "times_prayed", p->stats.times_prayed);
    cJSON_AddNumberToObject(obj, "max_level", p->stats.max_level);
    cJSON_AddNumberToObject(obj, "max_xp", p->stats.max_xp);



    return pser;
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
    frequency = game_difficulty(nlarn) << 3;

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
        if ((game_turn(nlarn) - e->start) % (22 - frequency) == 0)
        {
            damage *dam = damage_new(DAM_POISON, e->amount, NULL);

            player_damage_take(p, dam, PD_EFFECT, e->type);
        }
    }

    /* handle clumsiness */
    if ((e = player_effect_get(p, ET_CLUMSINESS)))
    {
        if (chance(33) && p->eq_weapon)
        {
            item *it = p->eq_weapon;

            log_disable(p->log);
            player_item_unequip(p, NULL, it);
            log_enable(p->log);

            log_add_entry(p->log, effect_get_msg_start(e));
            player_item_drop(p, &p->inventory, it);
        }
    }

    /* handle itching */
    if ((e = player_effect_get(p, ET_ITCHING)))
    {
        item *it;

        if (chance(50) && (it = player_random_armour(p)))
        {
            log_disable(p->log);
            player_item_unequip(p, NULL, it);
            log_enable(p->log);

            log_add_entry(p->log, effect_get_msg_start(e));
            player_item_drop(p, &p->inventory, it);
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

    char *tmp;

    int count, num;

    effect *ef;

    assert(p != NULL);

    /* check for life protection */
    if ((cause_type < PD_STUCK) && (ef = player_effect_get(p, ET_LIFE_PROTECTION)))
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
            player_level_gain(p, 1);
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

    case PD_STUCK:
        message = "You are trapped in solid rock.";
        title = "Ouch!";
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
    if (game_wizardmode(nlarn) && (cause_type < PD_TOO_LATE))
    {
        log_add_entry(p->log, "WIZARD MODE. You stay alive.");

        p->hp = p->hp_max;
        /* return to level 1 if last level has been lost */
        if (p->level < 1) player_level_gain(p, 1);

        /* return to the game */
        return;
    }

    /* do not show scores when in wizardmode */
    if (!game_wizardmode(nlarn))
    {
        /* redraw screen to make sure player can see the cause of his death */
        display_paint_screen(p);

        score = game_score(nlarn, cause_type, cause);
        scores = game_score_add(nlarn, score);

        tmp = player_death_description(score, TRUE);
        text = g_string_new(tmp);
        g_free(tmp);

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
            gchar *desc;

            cscore = (game_score_t *)iterator->data;

            desc = player_death_description(cscore, FALSE);
            g_string_append_printf(text, "%s%2d) %7" G_GINT64_FORMAT " %s [lvl. %d, %d hp]\n",
                                   (cscore == score) ? "*" : " ", num + count,
                                   cscore->score,
                                   desc,
                                   score->dlevel, cscore->hp);

            g_free(desc);
        }

        /* append map of current level */
        g_string_append(text, "\n\nThis is the map of the current level:\n\n");
        tmp = map_dump(game_map(nlarn, p->pos.z));
        g_string_append(text, tmp);
        g_free(tmp);

        /* list monsters killed */
        guint mnum, body_count = 0;
        g_string_append(text, "\n\nCreatures vanquished:\n\n");

        for (mnum = MT_NONE + 1; mnum < MT_MAX; mnum++)
        {
            if (p->stats.monsters_killed[mnum] > 0)
            {
                g_string_append_printf(text, "%3d %s%s\n",
                                       p->stats.monsters_killed[mnum],
                                       monster_name_by_type(mnum),
                                       (p->stats.monsters_killed[mnum] > 1) ? "s" : "");

                body_count += p->stats.monsters_killed[mnum];
            }
        }
        g_string_append_printf(text, "\n%3d total\n\n", body_count);

        display_show_message(title, text->str);

        /* repaint screen */
        display_paint_screen(p);

        if (display_get_yesno("Do you want to save a memorial " \
                              "file for your character?", NULL, NULL))
        {
            char *filename, *proposal;
            GError *error = NULL;

            /* repaint screen onece again */
            display_paint_screen(p);

            proposal = g_strconcat(p->name, ".txt", NULL);
            filename = display_get_string("Enter filename: ", proposal, 40);

            if (filename != NULL)
            {
                /* file name has been provided. try to save file */
                if (!g_file_set_contents(filename, text->str, -1, &error))
                {
                    display_show_message("Error", error->message);
                    g_error_free(error);
                }

                g_free(proposal);
            }

            g_free(filename);
        }

        g_string_free(text, TRUE);
        game_scores_destroy(scores);
    }

    display_shutdown();
    game_destroy(nlarn);

    exit(EXIT_SUCCESS);
}

gint64 player_calc_score(player *p, int won)
{
    gint64 score = 0;
    guint idx;

    assert (p != NULL);

    /* money */
    score = player_get_gold(p) + p->bank_account - p->outstanding_taxes;

    /* value of equipment */
    for (idx = 0; idx < inv_length(p->inventory); idx++)
    {
        score += item_price(inv_get(p->inventory, idx));
    }

    /* experience */
    score += p->experience;

    /* give points for remaining time if game has been won */
    if (won)
    {
        score += game_remaining_turns(nlarn) * (game_difficulty(nlarn) + 1);
    }

    return score;
}

int player_move(player *p, direction dir)
{
    int times = 1;      /* how many time ticks it took */
    position target_p;  /* coordinates of target */
    map *map;           /* shortcut to player's current map */
    monster *target_m;  /* monster on target tile (if any) */

    assert(p != NULL && dir > GD_NONE && dir < GD_MAX);

    /* no movement if overloaded */
    if (player_effect(p, ET_OVERSTRAINED))
    {
        log_add_entry(p->log, "You cannot move as long you are overstrained.");
        return 0;
    }

    /* no movement if paralyzed */
    if (player_effect(p, ET_PARALYSIS))
    {
        log_add_entry(p->log, "You can't move!");
        return 0;
    }

    /* confusion: random movement */
    if (player_effect(p, ET_CONFUSION))
    {
        dir = rand_1n(GD_MAX);
    }

    /* determine target position of move */
    target_p = pos_move(p->pos, dir);

    /* exceeded map limits */
    if (!pos_valid(target_p))
        return FALSE;

    /* make a shortcut to the current map */
    map = game_map(nlarn, p->pos.z);

    /* impassable */
    if (!map_pos_passable(map, target_p))
    {
        /* if it is not a wall, it is definitely not passable */
        if ((!map_tile_at(map, target_p) == LT_WALL))
            return FALSE;

        /* bump into wall when blind */
        if (player_effect(p, ET_BLINDNESS))
        {
            player_memory_of(p, target_p).type = map_tiletype_at(map, target_p);
            log_add_entry(p->log, "Ouch!");
            return times;
        }

        /* return if player cannot walk through walls */
        if (!player_effect(p, ET_WALL_WALK))
            return FALSE;
    }

    target_m = map_get_monster_at(map, target_p);

    if (target_m && monster_unknown(target_m))
    {
        /* reveal the mimic */
        log_add_entry(p->log, "Wait! That is a %s!", monster_name(target_m));
        monster_unknown_set(target_m, FALSE);
        return times;
    }

    /* attack - no movement */
    if (target_m)
    {
        return player_attack(p, target_m);
    }

    /* reposition player */
    p->pos = target_p;

    /* trigger the trap */
    if (map_trap_at(map, target_p))
    {
        times += player_trap_trigger(p, map_trap_at(map, target_p));
    }

    /* auto-pickup */
    if (map_ilist_at(map, p->pos))
    {
        player_autopickup(p);
    }

    return times;
}

int player_attack(player *p, monster *m)
{
    int prop;
    int amount;
    damage *dam;
    int roll;           /* the dice */
    effect *e;

    prop = p->level
           + player_get_dex(p)
           + (p->eq_weapon ? (weapon_wc(p->eq_weapon) / 4) : 0)
           + monster_ac(m) /* FIXME: I don't want those pointless D&D rules */
           - 12
           - game_difficulty(nlarn);

    roll = rand_1n(21);
    if ((roll <= prop) || (roll == 1))
    {
        /* placed a hit */
        log_add_entry(p->log, "You hit the %s.", monster_name(m));

        amount = player_get_str(p)
                 + player_get_wc(p)
                 - 12
                 - game_difficulty(nlarn);

        dam = damage_new(DAM_PHYSICAL, rand_1n(amount + 1), p);

        /* weapon damage due to rust when hitting certain monsters */
        if (p->eq_weapon && monster_is_metallivore(m))
        {
            /* impact of perishing */
            int pi = item_rust(p->eq_weapon);

            if (pi == PI_ENFORCED)
            {
                log_add_entry(p->log, "Your weapon is dulled by the %s.", monster_name(m));
            }
            else if (pi == PI_DESTROYED)
            {
                /* weapon has been destroyed */
                log_add_entry(p->log, "Your %s disintegrates!", weapon_name(p->eq_weapon));

                /* delete the weapon from the inventory */
                inv_del_element(&p->inventory, p->eq_weapon);

                /* destroy it and remove the reference to it */
                item_destroy(p->eq_weapon);
                p->eq_weapon = NULL;
            }
        }

        /* hitting a monster breaks stealth condition */
        if ((e = player_effect_get(p, ET_STEALTH)))
        {
            player_effect_del(p, e);
        }

        /* hitting a monster breaks hold monster spell */
        if ((e = monster_effect_get(m, ET_HOLD_MONSTER)))
        {
            monster_effect_del(m, e);
        }

        /* triple damage if hitting a dragon and wearing an amulet of dragon slaying */
        if ((monster_type(m) >= MT_WHITE_DRAGON && monster_type(m) <= MT_RED_DRAGON)
                && (p->eq_amulet && p->eq_amulet->id == AM_DRAGON_SLAYING))
        {
            dam->amount *= 3;
        }


        /* *** SPECIAL WEAPONS *** */
        if (p->eq_weapon)
        {
            /* Vorpal Blade */
            if ((p->eq_weapon->id == WT_VORPALBLADE)
                    && chance(5)
                    && monster_has_head(m)
                    && monster_is_beheadable(m))
            {
                log_add_entry(p->log, "You behead the %s with your Vorpal Blade!",
                              monster_name(m));

                dam->amount = monster_hp(m) + monster_ac(m);
            }

            /* Lance of Death */
            if ((p->eq_weapon->id == WT_LANCEOFDEATH))
            {
                /* the lance is pretty deadly for non-demons */
                if (monster_type(m) < MT_DEMONLORD_I)
                    dam->amount = 10000;
                else
                    dam->amount = 300;
            }

            /* Slayer */
            if (p->eq_weapon->id == WT_SLAYER)
            {
                if (monster_type(m) >= MT_DEMONLORD_I)
                    dam->amount = 10000;
            }
        }


        /* inflict damage */
        if (!(m = monster_damage_take(m, dam)))
        {
            /* killed the monster */
            return 1;
        }

        /* Lance of Death has not killed */
        if (p->eq_weapon && (p->eq_weapon->id == WT_LANCEOFDEATH))
        {
            log_add_entry(p->log, "Your lance of death tickles the %s!", monster_name(m));
        }

        /* if the player is invisible and the monster does not have infravision,
           remember the position where the attack came from
         */
        if (player_effect(p, ET_INVISIBILITY) && !monster_has_infravision(m))
        {
            monster_update_player_pos(m, p->pos);
        }
    }
    else
    {
        /* missed */
        log_add_entry(p->log, "You miss the %s.", monster_name(m));
    }

    return 1; /* i.e. turns used */
}

/**
 * function to enter a map.  This routine must be called anytime the
 * player changes levels.
 * Existing levels will get a few more monsters.
 *
 * @param player entering map
 * @param entered map
 * @param has to be TRUE if the player didn't enter the map regularly
 * @return TRUE
 */
int player_map_enter(player *p, map *l, gboolean teleported)
{
    int count;
    position pos;
    monster *m;

    assert(p != NULL && l != NULL);

    /* store the last turn player has been on this map */
    game_map(nlarn, p->pos.z)->visited = game_turn(nlarn);

    if (p->stats.deepest_level < l->nlevel)
    {
        p->stats.deepest_level = l->nlevel;
    }

    else
    {
        /* call map timer */
        /* count might be negative if time has been modified (time warp) */
        count = abs(game_turn(nlarn) - l->visited);
        map_timer(l, min(count, G_MAXUINT8));

        /* add some new monsters */
        if (l->nlevel > 0)
        {
            map_fill_with_life(l);
        }
    }

    /* been teleported here or something like that, need a random spot */
    if (teleported)
        pos = map_find_space(l, LE_MONSTER);

    /* beginning of the game */
    else if ((l->nlevel == 0) && (game_turn(nlarn) == 1))
        pos = map_find_stationary(l, LS_HOME);

    /* took the elevator up */
    else if ((p->pos.z == 0) && (l->nlevel == (MAP_MAX - 1)))
        pos = map_find_stationary(l, LS_ELEVATORUP);

    /* took the elevator down */
    else if ((p->pos.z == (MAP_MAX - 1)) && (l->nlevel == 0))
        pos = map_find_stationary(l, LS_ELEVATORDOWN);

    /* climbing up */
    else if (p->pos.z > l->nlevel)
    {
        if (l->nlevel == 0)
            pos = map_find_stationary(l, LS_ENTRANCE);
        else
            pos = map_find_stationary(l, LS_STAIRSDOWN);
    }
    /* climbing down */
    else if (l->nlevel > p->pos.z)
    {
        if (l->nlevel == 1)
            pos = map_find_stationary(l, LS_ENTRANCE);
        else
            pos = map_find_stationary(l, LS_STAIRSUP);
    }

    p->pos = pos;

    /* remove monster that might be at player's positon */
    if ((m = map_get_monster_at(l, p->pos)))
    {
        monster_pos_set(m, game_map(nlarn, p->pos.z), map_find_space(l, LE_MONSTER));
    }

    if (l->nlevel == 0)
    {
        /* do not log this at the start of the game */
        if (p->log->gtime > 1)
            log_add_entry(p->log, "You return to town.");
    }
    else if (l->nlevel == 1 && p->pos.z == 0)
        log_add_entry(p->log, "You enter the caverns of Larn.");

    else if (p->pos.z < l->nlevel)
        log_add_entry(p->log, "You descend to level %d.", l->nlevel);

    else
        log_add_entry(p->log, "You ascend to level %d.", l->nlevel);

    /* recalculate FOV to make ensure correct display after entering a level */
    player_update_fov(p, (player_effect(p, ET_BLINDNESS) ? 0 : 6 + player_effect(p, ET_AWARENESS)));

    return TRUE;
}

item *player_random_armour(player *p)
{
    GPtrArray *armours;
    item *armour = NULL;

    assert (p != NULL);

    armours = g_ptr_array_new();

    /* add each equipped piece of armour to the pool to choose from */
    if (p->eq_boots)  g_ptr_array_add(armours, p->eq_boots);
    if (p->eq_cloak)  g_ptr_array_add(armours, p->eq_cloak);
    if (p->eq_gloves) g_ptr_array_add(armours, p->eq_gloves);
    if (p->eq_helmet) g_ptr_array_add(armours, p->eq_helmet);
    if (p->eq_shield) g_ptr_array_add(armours, p->eq_shield);
    if (p->eq_suit)   g_ptr_array_add(armours, p->eq_suit);

    if (armours->len > 0)
    {
        armour = g_ptr_array_index(armours, rand_0n(armours->len));
    }

    g_ptr_array_free(armours, TRUE);

    return armour;
}

int player_examine(player *p, position pos)
{
    map_tile *tile;
    item *it;
    char item_desc[81];
    guint idx;
    char *desc = NULL, *tmp, *where;

    assert(p != NULL);

    if (pos_identical(pos, p->pos))
        where = "here";
    else
        where = "there";

    tile = map_tile_at(game_map(nlarn, p->pos.z), pos);

    /* describe the level tile */
    desc = g_strdup(lt_get_desc(tile->type));
    desc[0] = g_ascii_toupper(desc[0]);

    log_add_entry(p->log, "%s.", desc, where);
    g_free(desc);

    /* name monster */
    if (tile->monster != NULL)
    {
        log_add_entry(p->log, "A%s %s.", a_an(monster_name(tile->monster)),
                      monster_name(tile->monster));
    }

    /* add message if target tile contains a stationary item */
    if (tile->stationary > LS_NONE)
    {
        log_add_entry(p->log, "You see %s %s.", ls_get_desc(tile->stationary), where);
    }

    /* add message if target tile contains a known trap */
    if (player_memory_of(p, pos).trap)
    {
        log_add_entry(p->log, "There is a%s %s %s.", a_an(trap_description(tile->trap)),
                      trap_description(tile->trap), where);
    }

    /* add message if target tile contains items */
    if (tile->ilist != NULL && inv_length(tile->ilist) > 0)
    {
        if (inv_length(tile->ilist) > 3)
        {
            log_add_entry(p->log, "There are multiple items %s.", where);
        }
        else
        {
            for (idx = 0; idx < inv_length(tile->ilist); idx++)
            {
                it = inv_get(tile->ilist, idx);
                item_describe(it, player_item_known(p, it),
                              FALSE, FALSE, item_desc, 80);

                if (idx > 0)
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

            log_add_entry(p->log, "You see %s %s.", desc, where);
            g_free(desc);
        }
    }

    return TRUE;
}

int player_pickup(player *p)
{
    inventory **inv;
    int time = 0;

    GPtrArray *callbacks;
    display_inv_callback *callback;

    assert(p != NULL);

    inv = map_ilist_at(game_map(nlarn, p->pos.z), p->pos);

    if (inv_length(*inv) == 0)
    {
        log_add_entry(p->log, "There is nothing here.");
        return 0;
    }
    else if (inv_length(*inv) == 1)
    {
        return player_item_pickup(p, inv, inv_get(*inv, 0));
    }
    else
    {
        /* define callback functions */
        callbacks = g_ptr_array_new();

        callback = g_malloc0(sizeof(display_inv_callback));
        callback->description = "(g)et";
        callback->key = 'g';
        callback->inv = inv;
        callback->function = &player_item_pickup;
        g_ptr_array_add(callbacks, callback);

        display_inventory("On the floor", p, inv, callbacks, FALSE, NULL);

        /* clean up callbacks */
        display_inv_callbacks_clean(callbacks);
    }

    return time;
}

void player_autopickup(player *p)
{
    guint idx;
    item *i;
    inventory **floor;

    assert (p != NULL && map_ilist_at(game_map(nlarn, p->pos.z), p->pos));

    floor = map_ilist_at(game_map(nlarn, p->pos.z), p->pos);

    for (idx = 0; idx < inv_length(*floor); idx++)
    {
        i = inv_get(*floor, idx);

        if (p->settings.auto_pickup[i->type])
        {
            /* try to pick up the item */
            if (player_item_pickup(p, floor, i))
            {
                /* item has been picked up */
                /* go back one item as the following items lowered their number */
                idx--;
            }
        }
    }
}

void player_autopickup_show(player *p)
{
    GString *msg;
    int count = 0;
    item_t it;

    assert (p != NULL);

    msg = g_string_new(NULL);

    for (it = IT_NONE; it < IT_MAX; it++)
    {
        if (p->settings.auto_pickup[it])
        {
            if (count)
                g_string_append(msg, ", ");

            g_string_append(msg, item_name_pl(it));
            count++;
        }
    }

    if (!count)
        g_string_append(msg, "Auto-pickup is not enabled.");
    else
    {
        g_string_prepend(msg, "Auto-pickup is enabled for ");
        g_string_append(msg, ".");
    }


    log_add_entry(p->log, msg->str);

    g_string_free(msg, TRUE);
}

void player_level_gain(player *p, int count)
{
    int base;
    int i;

    assert(p != NULL && count > 0);

    p->level += count;
    log_add_entry(p->log, "You gain experience and become a %s!", player_level_desc[p->level]);

    if (p->level > p->stats.max_level)
        p->stats.max_level = p->level;

    if (p->experience < player_lvl_exp[p->level])
        /* artificially gained a level, need to adjust XP to match level */
        player_exp_gain(p, player_lvl_exp[p->level] - p->experience);

    for (i = 0; i < count; i++)
    {
        /* increase HP max */
        base = (p->constitution - game_difficulty(nlarn)) >> 1;
        if (p->level < max(7 - game_difficulty(nlarn), 0))
            base += p->constitution >> 2;

        player_hp_max_gain(p, rand_1n(3) + rand_0n(max(base, 1)));

        /* increase MP max */
        base = (p->intelligence - game_difficulty(nlarn)) >> 1;
        if (p->level < max(7 - game_difficulty(nlarn), 0))
            base += p->intelligence >> 2;

        player_mp_max_gain(p, rand_1n(3) + rand_0n(max(base, 1)));
    }
}

void player_level_lose(player *p, int count)
{
    int base;
    int i;

    assert(p != NULL && count > 0);

    p->level -= count;
    log_add_entry(p->log, "You return to experience level %d...", p->level);

    if (p->experience > player_lvl_exp[p->level])
        /* adjust XP to match level */
        player_exp_lose(p, p->experience - player_lvl_exp[p->level]);

    for (i = 0; i < count; i++)
    {
        /* decrease HP max */
        base = (p->constitution - game_difficulty(nlarn)) >> 1;
        if (p->level < max(7 - game_difficulty(nlarn), 0))
            base += p->constitution >> 2;

        player_hp_max_lose(p, rand_1n(3) + rand_0n(max(base, 1)));

        /* decrease MP max */
        base = (p->intelligence - game_difficulty(nlarn)) >> 1;
        if (p->level < max(7 - game_difficulty(nlarn), 0))
            base += p->intelligence >> 2;

        player_mp_max_lose(p, rand_1n(3) + rand_0n(max(base, 1)));
    }

    /* die if lost level 1 */
    if (p->level == 0)
        player_die(p, PD_LASTLEVEL, 0);
}

void player_exp_gain(player *p, int count)
{
    int numlevels = 0;

    assert(p != NULL && count > 0);
    p->experience += count;

    if (p->stats.max_xp < p->experience)
        p->stats.max_xp = p->experience;

    while (player_lvl_exp[p->level + 1 + numlevels] <= p->experience)
        numlevels++;

    if (numlevels)
        player_level_gain(p, numlevels);
}

void player_exp_lose(player *p, int count)
{
    int numlevels = 0;

    assert(p != NULL && count > 0);
    p->experience -= count;

    while ((player_lvl_exp[p->level - numlevels]) > p->experience)
        numlevels++;

    if (numlevels)
        player_level_lose(p, numlevels);
}


int player_hp_gain(player *p, int count)
{
    assert(p != NULL);

    p->hp += count;
    if (p->hp > player_get_hp_max(p))
        p->hp = player_get_hp_max(p);

    return p->hp;
}

void player_damage_take(player *p, damage *dam, player_cod cause_type, int cause)
{
    monster *m = NULL;
    effect *e = NULL;

    const damage_msg damage_msgs[] =
    {
        { NULL, NULL, },
        { "Ouch!", "Your armour protects you.", }, /* DAM_PHYSICAL */
        { NULL, "You resist.", }, /* DAM_MAGICAL */
        { "You suffer burns.", "The flames don't phase you.", }, /* DAM_FIRE */
        { "You suffer from frostbite.", "It doesn't seem so cold.", }, /* DAM_COLD */
        { NULL, NULL, }, /* DAM_ACID */
        { "You experience near-drowning.", "The water doesn't affect you.", }, /* DAM_WATER */
        { NULL, NULL, }, /* DAM_ELECTRICITY */
        /* effect start messages are covered by player_effect_add
         * only need to notify if the player resisted */
        { "You feel poison running through your veins.", "You resist the poison.", }, /* DAM_POISON */
        { NULL, "You are not blinded.", }, /* DAM_BLINDNESS */
        { NULL, "You are not confused.", }, /* DAM_CONFUSION */
        { NULL, "", }, /* DAM_PARALYSIS */
        { NULL, "", }, /* DAM_DEC_STR */
        { NULL, "", }, /* DAM_DEC_DEX */
        { "Your life energy is drained.", "You are not affected.", }, /* DAM_DRAIN_LIFE */
    };

    assert(p != NULL && dam != NULL);

    if (dam->originator)
    {
        m = (monster *)dam->originator;

        /* half damage if player is protected against spirits */
        if (player_effect(p, ET_SPIRIT_PROTECTION) && monster_is_spirit(m))
        {
            if (dam->type == DAM_PHYSICAL)
            {
                /* half physical damage */
                dam->amount >>= 1;
            }
            else
            {
                /* cancel special attacks */
                dam->amount = 0;
            }
        }

        /* amulet of power cancels demon attacks */
        if ((monster_type(m) >= MT_DEMONLORD_I) && chance(75)
                && (p->eq_amulet && p->eq_amulet->id == AM_POWER))
        {
            log_add_entry(p->log, "Your amulet cancels the %s's attack.",
                          monster_name(m));

            return;
        }
    }

    /* check resistances */
    switch (dam->type)
    {
    case DAM_PHYSICAL:
        dam->amount -= player_get_ac(p);
        break;

    case DAM_MAGICAL:
        dam->amount -= player_effect(p, ET_RESIST_MAGIC);
        break;

    case DAM_FIRE:
        dam->amount -= player_effect(p, ET_RESIST_FIRE);
        break;

    case DAM_COLD:
        dam->amount -= player_effect(p, ET_RESIST_COLD);
        break;

    case DAM_ACID:
        break;

    case DAM_WATER:
        break;

    case DAM_ELECTRICITY:
        break;

    case DAM_POISON:
        break;

    case DAM_BLINDNESS:
        break;

    case DAM_CONFUSION:
        break;

    case DAM_PARALYSIS:
        break;

    case DAM_DEC_STR:
        break;

    case DAM_DEC_DEX:
        break;

    case DAM_DRAIN_LIFE:
        if (player_effect(p, ET_UNDEAD_PROTECTION))
            dam->amount = 0;
        break;

    default:
        /* well, well */
        break;
    }

    /* prevent adding to HP if damage went below zero due to resistances */
    dam->amount = max(0, dam->amount);

    if (dam->amount)
    {
        /* add effects */
        switch (dam->type)
        {
        case DAM_POISON:
            if (!(e = player_effect_get(p, ET_POISON)))
            {
                e = effect_new(ET_POISON, game_turn(nlarn));
                e->amount = dam->amount;
                player_effect_add(p, e);
            }
            else if (cause_type != PD_EFFECT)
            {
                e->amount += dam->amount;
            }
            break;

        case DAM_BLINDNESS:
            /* it is not possible to become more blind */
            if (!(e = player_effect_get(p, ET_BLINDNESS)))
            {
                e = effect_new(ET_BLINDNESS, game_turn(nlarn));
                player_effect_add(p, e);
            }
            break;

        case DAM_CONFUSION:
            if (!(e = player_effect_get(p, ET_CONFUSION)))
            {
                e = effect_new(ET_CONFUSION, game_turn(nlarn));
                player_effect_add(p, e);
            }
            break;

        case DAM_PARALYSIS:
            if (!(e = player_effect_get(p, ET_PARALYSIS)))
            {
                e = effect_new(ET_PARALYSIS, game_turn(nlarn));
                player_effect_add(p, e);
            }
            break;

        case DAM_DEC_STR:
            e = effect_new(ET_DEC_STR, game_turn(nlarn));
            e->turns = dam->amount * 50;
            player_effect_add(p, e);
            break;

        case DAM_DEC_DEX:
            e = effect_new(ET_DEC_DEX, game_turn(nlarn));
            e->turns = dam->amount * 50;
            player_effect_add(p, e);
            break;

        case DAM_DRAIN_LIFE:
            player_level_lose(p, dam->amount);
            break;

        default:
            /* pfffft. */
            break;
        }

        /* taken damage */
        p->hp -= dam->amount;

        /* notify player */
        if (damage_msgs[dam->type].msg_affected)
        {
            log_add_entry(p->log, damage_msgs[dam->type].msg_affected);
        }
    }
    else
    {
        /* not affected - notifiy player */
        if ((dam->type <= DAM_DRAIN_LIFE) && damage_msgs[dam->type].msg_unaffected)
            log_add_entry(p->log, damage_msgs[dam->type].msg_unaffected);
    }

    g_free(dam);

    if (p->hp < 1)
    {
        player_die(p, cause_type, cause);
    }

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

    if (p->hp > (int)p->hp_max)
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

    if (p->mp > (int)p->mp_max)
        p->mp = p->mp_max;

    return p->mp_max;
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
            player_effect_add(p, effect_new(rand_m_n(ET_INC_CHA, ET_INC_WIS), game_turn(nlarn)));
            break;

        case ET_INC_HP_MAX:
            p->hp_max += ((player_get_hp_max(p) / 100) * e->amount);
            break;

        case ET_INC_MP_MAX:
            p->mp_max += ((player_get_mp_max(p) / 100) * e->amount);
            break;

        case ET_INC_LEVEL:
            player_level_gain(p, e->amount);
            break;

        case ET_INC_EXP:
            /* looks like a reasonable amount */
            player_exp_gain(p, rand_1n(player_lvl_exp[p->level] - player_lvl_exp[p->level - 1]));
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
            player_effect_add(p, effect_new(rand_m_n(ET_DEC_CHA, ET_DEC_WIS), game_turn(nlarn)));
            break;

        case ET_DEC_HP_MAX:
            p->hp_max -= ((player_get_hp_max(p) / 100) * e->amount);
            break;

        case ET_DEC_MP_MAX:
            p->mp_max -= ((player_get_mp_max(p) / 100) * e->amount);
            break;

        case ET_DEC_LEVEL:
            player_level_lose(p, e->amount);
            break;

        case ET_DEC_EXP:
            /* looks like a reasonable amount */
            player_exp_lose(p, rand_1n(player_lvl_exp[p->level] - player_lvl_exp[p->level - 1]));
            break;

        default:
            /* nop */
            break;
        }
        effect_destroy(e);
    }
    else if (e->type == ET_SLEEP)
    {
        game_spin_the_wheel(nlarn, e->turns);
        effect_destroy(e);
    }
    else
    {
        effect_add(p->effects, e);
    }
}

void player_effects_add(player *p, GPtrArray *effects)
{
    guint idx;
    effect *e;

    assert (p != NULL);

    /* if effects is NULL */
    if (!effects) return;

    for (idx = 0; idx < effects->len; idx++)
    {
        e = g_ptr_array_index(effects, idx);
        g_ptr_array_add(p->effects, e);

        if (effect_get_msg_start(e))
        {
            log_add_entry(p->log, effect_get_msg_start(e));
        }
    }
}

int player_effect_del(player *p, effect *e)
{
    assert(p != NULL && e != NULL && e->type > ET_NONE && e->type < ET_MAX);

    if (effect_get_msg_stop(e))
        log_add_entry(p->log, effect_get_msg_stop(e));

    return effect_del(p->effects, e);
}

void player_effects_del(player *p, GPtrArray *effects)
{
    guint idx;
    effect *e;

    assert (p != NULL);

    /* if effects is NULL */
    if (!effects) return;

    for (idx = 0; idx < effects->len; idx++)
    {
        e = g_ptr_array_index(effects, idx);
        g_ptr_array_remove_fast(p->effects, e);

        if (effect_get_msg_stop(e))
        {
            log_add_entry(p->log, effect_get_msg_stop(e));
        }
    }
}

effect *player_effect_get(player *p, int effect_id)
{
    assert(p != NULL && effect_id > ET_NONE && effect_id < ET_MAX);
    return effect_get(p->effects, effect_id);
}

int player_effect(player *p, int effect_type)
{
    assert(p != NULL && effect_type > ET_NONE && effect_type < ET_MAX);
    return effect_query(p->effects, effect_type);
}

void player_effects_expire(player *p, int turns)
{
    guint idx = 0;
    effect *e;

    assert(p != NULL);

    while (idx < p->effects->len)
    {
        e = g_ptr_array_index(p->effects, idx);

        if (effect_expire(e, turns) == -1)
        {
            /* effect has expired */
            player_effect_del(p, e);
            effect_destroy(e);
        }
        else
        {
            idx++;
        }
    }
}

GPtrArray *player_effect_text(player *p)
{
    GPtrArray *text;

    text = g_ptr_array_new();

    if (player_effect(p, ET_DIZZINESS))    g_ptr_array_add(text, "dizzy");
    if (player_effect(p, ET_SICKNESS))     g_ptr_array_add(text, "sick");
    if (player_effect(p, ET_BLINDNESS))    g_ptr_array_add(text, "blind");
    if (player_effect(p, ET_CONFUSION))    g_ptr_array_add(text, "confused");
    if (player_effect(p, ET_PARALYSIS))    g_ptr_array_add(text, "paralyzed");
    if (player_effect(p, ET_POISON))       g_ptr_array_add(text, "poisoned");
    if (player_effect(p, ET_SLOWNESS))     g_ptr_array_add(text, "slow");
    if (player_effect(p, ET_BURDENED))     g_ptr_array_add(text, "burdened");
    if (player_effect(p, ET_OVERSTRAINED)) g_ptr_array_add(text, "overload");

    return text;
}

int player_inv_display(player *p)
{
    GPtrArray *callbacks;
    display_inv_callback *callback;

    assert(p != NULL);

    if (inv_length(p->inventory) == 0)
    {
        /* don't show empty inventory */
        log_add_entry(p->log, "You do not carry anything.");
        return FALSE;
    }

    /* define callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(d)rop";
    callback->key = 'd';
    callback->inv = &p->inventory;
    callback->function = &player_item_drop;
    callback->checkfun = &player_item_is_dropable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(e)quip";
    callback->key = 'e';
    callback->function = &player_item_equip;
    callback->checkfun = &player_item_is_equippable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(o)pen";
    callback->key = 'o';
    callback->function = &container_open;
    callback->checkfun = &player_item_is_container;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(p)ut into sth.";
    callback->key = 'p';
    callback->function = &container_item_add;
    callback->checkfun = &player_item_can_be_added_to_container;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(u)nequip";
    callback->key = 'u';
    callback->function = &player_item_unequip;
    callback->checkfun = &player_item_is_equipped;
    g_ptr_array_add(callbacks, callback);

    /* unequip and use should never appear together */
    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(u)se";
    callback->key = 'u';
    callback->function = &player_item_use;
    callback->checkfun = &player_item_is_usable;
    g_ptr_array_add(callbacks, callback);

    display_inventory("Inventory", p, &p->inventory, callbacks, FALSE, NULL);

    /* clean up */
    display_inv_callbacks_clean(callbacks);

    return TRUE;
}

char *player_inv_weight(player *p)
{
    float weight;
    char *unit = "g";
    static char buf[21] = "";

    assert (p != NULL);

    weight = (float)inv_weight(p->inventory);
    if (weight > 1000)
    {
        weight = weight / 1000;
        unit = "kg";
    }

    g_snprintf(buf, 20, "%g%s", weight, unit);

    return buf;
}

int player_inv_pre_add(inventory *inv, item *item)
{
    player *p;
    char buf[61];

    int pack_weight;
    float can_carry;

    p = (player *)inv->owner;


    if (player_effect(p, ET_OVERSTRAINED))
    {
        log_add_entry(p->log, "You are already overloaded!");
        return FALSE;
    }

    /* calculate inventory weight */
    pack_weight = inv_weight(inv);

    /* player can carry 2kg per str */
    can_carry = 2000 * (float)player_get_str(p);

    /* check if item weight can be carried */
    if ((pack_weight + item_weight(item)) > (int)(can_carry * 1.3))
    {
        /* get item description */
        item_describe(item, player_item_identified(p, item), (item->count == 1), TRUE, buf, 60);
        /* capitalize first letter */
        buf[0] = g_ascii_toupper(buf[0]);

        log_add_entry(p->log, "%s is too heavy for you.", buf);
        return FALSE;
    }

    return TRUE;
}

void player_inv_weight_recalc(inventory *inv, item *item)
{
    int pack_weight;
    float can_carry;
    effect *e = NULL;

    player *p;

    assert (inv != NULL);

    /* make shortcut */
    p = (player *)inv->owner;

    /* don't need that parameter */
    item = NULL;

    /* calculate inventory weight */
    pack_weight = inv_weight(inv);

    /* player can carry 2kg per str */
    can_carry = 2000 * (float)player_get_str(p);


    if (pack_weight > (int)(can_carry * 1.3))
    {
        /* OVERSTRAINED - if burdened get rid of this */
        if ((e = player_effect_get(p, ET_BURDENED)))
        {
            player_effect_del(p, e);
        }

        /* make overstrained */
        if (!player_effect(p, ET_OVERSTRAINED))
        {
            player_effect_add(p, effect_new(ET_OVERSTRAINED, game_turn(nlarn)));
        }
    }
    else if (pack_weight < (int)(can_carry * 1.3) && (pack_weight > can_carry))
    {
        /* BURDENED - if overstrained get rid of this */
        if ((e = player_effect_get(p, ET_OVERSTRAINED)))
        {
            player_effect_del(p, e);
        }

        if (!player_effect(p, ET_BURDENED))
        {
            player_effect_add(p, effect_new(ET_BURDENED, game_turn(nlarn)));
        }
    }
    else if (pack_weight < can_carry)
    {
        /* NOT burdened, NOT overstrained */
        if ((e = player_effect_get(p, ET_OVERSTRAINED)))
        {
            player_effect_del(p, e);
        }

        if (player_effect(p, ET_BURDENED))
        {
            player_effect_del(p, player_effect_get(p, ET_BURDENED));
        }
    }
}

/* level is needed to make function signature match display_inventory requirements */
int player_item_equip(player *p, inventory **inv, item *it)
{
    item **islot = NULL;  /* pointer to chosen item slot */
    int time = 0;         /* time the desired action takes */
    char description[61];

    assert(p != NULL && it != NULL);

    /* don't need that parameter */
    inv = NULL;

    /* the idea behind the time values: one turn to take one item off,
       one turn to get the item out of the pack */

    switch (it->type)
    {
    case IT_AMULET:
        if (p->eq_amulet == NULL)
        {
            p->eq_amulet = it;

            item_describe(it, player_item_known(p, it), TRUE, TRUE, description, 60);
            log_add_entry(p->log, "You put %s on.", description);
            p->identified_amulets[it->id] = TRUE;
            time = 2;
        }
        break;

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

            item_describe(it, player_item_known(p, it), TRUE, FALSE, description, 60);
            log_add_entry(p->log, "You are now wearing %s.", description);

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
            item_describe(it, player_item_known(p, it), TRUE, TRUE, description, 60);
            log_add_entry(p->log, "You put %s on.", description);

            *islot = it;

            if (ring_is_observable(it))
            {
                p->identified_rings[it->id] = TRUE;
            }

            if (ring_bonus_is_obs(it))
            {
                it->bonus_known = TRUE;
            }

            time = 2;
        }
        break;

    case IT_WEAPON:
        if (p->eq_weapon == NULL)
        {
            item_describe(it, player_item_known(p, it), TRUE, TRUE, description, 60);

            p->eq_weapon = it;
            log_add_entry(p->log, "You now wield %s.", description);

            time = 2 + weapon_is_twohanded(p->eq_weapon);

            if (it->cursed)
            {
                /* capitalize first letter */
                description[0] = g_ascii_toupper(description[0]);
                log_add_entry(p->log, "%s welds itself into your hand.", description);
                it->curse_known = TRUE;
            }
        }
        break;

    default:
        /* nop */
        break;
    }

    player_effects_add(p, it->effects);

    return time;
}

int player_item_unequip(player *p, inventory **inv, item *it)
{
    int equipment_type;
    item **aslot = NULL;  /* pointer to armour slot */
    item **rslot = NULL;  /* pointer to ring slot */

    int time = 0;         /* time elapsed */
    char desc[61];        /* item description */

    assert(p != NULL && it != NULL);

    /* don't need that parameter */
    inv = NULL;

    equipment_type = player_item_is_equipped(p, it);

    /* the idea behind the time values: one turn to take one item off,
       one turn to get the item out of the pack */

    switch (equipment_type)
    {
    case PE_AMULET:
        if (p->eq_amulet != NULL)
        {
            item_describe(it, player_item_known(p, it), TRUE, TRUE, desc, 60);

            if (!it->cursed)
            {
                log_add_entry(p->log, "You remove %s.", desc);

                player_effects_del(p, p->eq_amulet->effects);
                p->eq_amulet = NULL;

                time = 2;
            }
            else
            {
                log_add_entry(p->log, "You can not remove %s.%s", desc,
                              it->curse_known ? "" : " It appears to be cursed.");

                it->curse_known = TRUE;
            }
        }
        break;

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
            item_describe(it, player_item_known(p, it), TRUE, TRUE, desc, 60);

            if (!p->eq_weapon->cursed)
            {
                log_add_entry(p->log, "You you put away %s.", desc);

                time = 2 + weapon_is_twohanded(p->eq_weapon);
                player_effects_del(p, p->eq_weapon->effects);

                p->eq_weapon = NULL;
            }
            else
            {
                log_add_entry(p->log, "You can't put away %s. " \
                              "It's welded into your hands.", desc);
            }
        }
        break;
    }

    /* take off armour */
    if ((aslot != NULL) && (*aslot != NULL))
    {
        item_describe(it, player_item_known(p, it), TRUE, TRUE, desc, 60);

        if (!it->cursed)
        {
            log_add_entry(p->log, "You finish taking off %s.", desc);

            player_effects_del(p, (*aslot)->effects);

            *aslot = NULL;
        }
        else
        {
            log_add_entry(p->log, "You can't take of %s.%s", desc,
                          it->curse_known ? "" : " It appears to be cursed.");

            it->curse_known = TRUE;
        }
    }

    if ((rslot != NULL) && (*rslot != NULL))
    {
        item_describe(it, player_item_known(p, it), TRUE, TRUE, desc, 60);

        if (!it->cursed)
        {
            log_add_entry(p->log, "You remove %s.", desc);

            player_effects_del(p, (*rslot)->effects);

            *rslot = NULL;
            time = 2;
        }
        else
        {
            log_add_entry(p->log, "You can not remove %s.%s", desc,
                          it->curse_known ? "" : " It appears to be cursed.");

            it->curse_known = TRUE;
        }
    }

    return time;
}

/* silly filter to get containers */
int player_item_is_container(player *p, item *it)
{
    assert(p != NULL && it != NULL && it->type < IT_MAX);

    return (it->type == IT_CONTAINER);
}

/* silly filter to check if item can be put into a container */
int player_item_can_be_added_to_container(player *p, item *it)
{
    guint idx;
    item *i;

    assert(p != NULL && it != NULL && it->type < IT_MAX);

    if (it->type == IT_CONTAINER)
    {
        return FALSE;
    }

    if (player_item_is_equipped(p, it))
    {
        return FALSE;
    }

    /* check player's inventory for containers */
    for (idx = 0; idx < inv_length(p->inventory); idx++)
    {
        i = inv_get(p->inventory, idx);
        if (i->type == IT_CONTAINER)
        {
            return TRUE;
        }
    }

    /* no match till now, check floor for containers */
    inventory **floor = map_ilist_at(game_map(nlarn, p->pos.z), p->pos);

    for (idx = 0; idx < inv_length(*floor); idx++)
    {
        i = inv_get(*floor, idx);
        if (i->type == IT_CONTAINER)
        {
            return TRUE;
        }
    }

    /* nope */
    return FALSE;
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

    if (it == p->eq_amulet)
        return PE_AMULET;

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

    /* amulets */
    if ((it->type == IT_AMULET) && (p->eq_amulet))
        return FALSE;

    /* armour */
    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_BOOTS)
            && (p->eq_boots))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_CLOAK)
            && (p->eq_cloak))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_GLOVES)
            && (p->eq_gloves))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_HELMET)
            && (p->eq_helmet))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_SHIELD)
            && (p->eq_shield))
        return FALSE;

    /* shield / two-handed weapon combination */
    if (it->type == IT_ARMOUR
            && (armour_category(it) == AC_SHIELD)
            && (p->eq_weapon)
            && weapon_is_twohanded(p->eq_weapon))
        return FALSE;

    if ((it->type == IT_ARMOUR)
            && (armour_category(it) == AC_SUIT)
            && (p->eq_suit))
        return FALSE;

    /* rings */
    if ((it->type == IT_RING) && (p->eq_ring_l) && (p->eq_ring_r))
        return FALSE;

    /* weapons */
    if ((it->type == IT_WEAPON) && (p->eq_weapon))
        return FALSE;

    /* twohanded weapon / shield combinations */
    if ((it->type == IT_WEAPON) && weapon_is_twohanded(it)&& (p->eq_shield))
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

    return ((item_price(it) <= player_get_gold(p))
            || (item_price(it) <= p->bank_account));
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

/* determine if item type is known */
int player_item_known(player *p, item *it)
{
    assert(p != NULL && it != NULL && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_AMULET:
        return p->identified_amulets[it->id];
        break;

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

/* determine if a concrete item is fully identified */
int player_item_identified(player *p, item *it)
{
    gboolean known = FALSE;

    assert(p != NULL && it != NULL);

    known = player_item_known(p, it);

    if (it->cursed && !it->curse_known)
        known = FALSE;

    if (it->blessed && !it->blessed_known)
        known = FALSE;

    if ((it->type == IT_ARMOUR || it->type == IT_RING || it->type == IT_WEAPON)
            && !it->bonus_known)
        known = FALSE;

    return known;
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
        IT_AMULET,
        IT_BOOK,
        IT_POTION,
        IT_RING,
        IT_SCROLL,
    };

    assert (p != NULL);

    list = g_string_new(NULL);
    it = item_new(type_ids[0], 1, 0);

    for (type = 0; type < 5; type++)
    {
        count = 0;
        sublist = g_string_new(NULL);

        /* item category header */
        heading = g_strdup(item_name_pl(type_ids[type]));
        heading[0] = g_ascii_toupper(heading[0]);

        /* no linefeed before first category */
        g_string_append_printf(sublist, (type == 0) ? "%s\n" : "\n%s\n",
                               heading);

        g_free(heading);

        it->type = type_ids[type];

        for (id = 1; id < item_max_id(type_ids[type]); id++)
        {
            it->id = id;
            if (player_item_known(p, it))
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

    if (list->len > 0)
    {
        /* append trailing newline */
        g_string_append_c(list, '\n');
        return g_string_free(list, FALSE);
    }
    else
    {
        g_string_free(list, TRUE);
        return NULL;
    }
}

void player_item_identify(player *p, inventory **inv, item *it)
{
    assert(p != NULL && it != NULL);

    /* don't need that parameter */
    inv = NULL;

    switch (it->type)
    {
    case IT_AMULET:
        p->identified_amulets[it->id] = TRUE;
        break;

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

int player_item_use(player *p, inventory **inv, item *it)
{
    char description[61];
    int item_used_up = TRUE;
    int item_identified = FALSE;
    int time = 0; /* number of turns this action took */

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* don't need that parameter */
    inv = NULL;

    if (player_effect(p, ET_BLINDNESS) && (it->type == IT_BOOK || it->type == IT_SCROLL))
    {
        log_add_entry(p->log, "As you are blind you can't read %s.",
                      item_describe(it, player_item_known(p, it),
                                    TRUE, TRUE, description, 60));
        return time;
    }

    switch (it->type)
    {

        /* read book */
    case IT_BOOK:
        item_describe(it, player_item_known(p, it),
                      TRUE, TRUE, description, 60);

        log_add_entry(p->log, "You read %s.", description);

        /* cursed spellbooks have nasty effects */
        if (it->cursed)
        {
            log_add_entry(p->log, "There was something wrong with this book! " \
                          "It crumbles to dust.");

            player_mp_lose(p, rand_0n(p->mp));
        }
        else
        {
            switch (spell_learn(p, it->id))
            {
            case 0:
                log_add_entry(p->log, "You cannot understand the content of this book.");
                item_used_up = FALSE;
                break;

            case 1:
                /* learnt spell */
                log_add_entry(p->log, "You master the spell \"%s\".",
                              book_name(it));

                item_identified = TRUE;
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
        item_describe(it, player_item_known(p, it),
                      TRUE, FALSE, description, 60);

        log_add_entry(p->log, "You eat %s.", description);

        if (it->id == FT_FORTUNE_COOKIE)
        {
            log_add_entry(p->log,
                          "It has a piece of paper inside. It reads: \"%s\"",
                          food_get_fortune(game_fortunes(nlarn)));
        }

        time = 2;

        break;


        /* drink potion */
    case IT_POTION:
        item_describe(it, player_item_known(p, it),
                      TRUE, FALSE, description, 60);

        log_add_entry(p->log, "You drink %s.", description);

        if (it->cursed)
        {
            damage *dam = damage_new(DAM_POISON, rand_1n(p->hp), NULL);

            log_add_entry(p->log, "The Potion is foul!");

            log_add_entry(p->log, "You spit gore!");
            player_damage_take(p, dam, PD_CURSE, it->type);
        }
        else
        {

            switch (it->id)
            {
            case PO_AMNESIA:
                item_identified = potion_amnesia(p, it);
                break;

            case PO_OBJ_DETECT:
            case PO_TRE_DETECT:
                item_identified = potion_detect_item(p, it);
                break;

            case PO_WATER:
                log_add_entry(p->log, "This tastes like water..");
                item_identified = TRUE;
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
        item_describe(it, player_item_known(p, it),
                      TRUE, FALSE, description, 60);

        log_add_entry(p->log, "You read %s.", description);

        if (it->cursed)
        {
            damage *dam = damage_new(DAM_FIRE, rand_1n(p->hp), NULL);

            log_add_entry(p->log, "The Scroll explodes!");

            log_add_entry(p->log, "You are hurt by the explosion!");
            player_damage_take(p, dam, PD_CURSE, it->type);
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
                item_identified = TRUE;
                log_add_entry(p->log, "This scroll is blank.");
                break;

            case ST_CREATE_MONSTER:
                item_identified = spell_create_monster(p);
                break;

            case ST_CREATE_ARTIFACT:
                item_identified = scroll_create_artefact(p, it);
                break;

            case ST_TIMEWARP:
                item_identified = scroll_timewarp(p, it);
                break;

            case ST_TELEPORT:
                item_identified = scroll_teleport(p, it);
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
                spell_vaporize_rock(p);
                item_identified = TRUE;
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
        item_describe(it, player_item_known(p, it),
                      TRUE, FALSE, description, 40);

        log_add_entry(p->log,
                      "I have absolutely no idea how to use %s.",
                      description);

        item_used_up = FALSE;
    }

    if (item_identified)
    {
        /* identify the item type, not the entire item */
        switch (it->type)
        {
        case IT_BOOK:
            p->identified_books[it->id] = TRUE;
            break;

        case IT_POTION:
            p->identified_potions[it->id] = TRUE;
            break;

        case IT_SCROLL:
            p->identified_scrolls[it->id] = TRUE;
            break;

        default:
            /* NOP */
            break;
        }
    }

    if (item_used_up)
    {
        if (it->count > 1)
        {
            it->count--;
        }
        else
        {
            inv_del_element(&p->inventory, it);
        }
    }

    return time;
}

int player_item_drop(player *p, inventory **inv, item *it)
{
    char desc[61];
    guint count = 0;

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (player_item_is_equipped(p, it))
        return FALSE;

    if (it->count > 1)
    {
        g_snprintf(desc, 60, "Drop how many %s?", item_name_pl(it->type));

        count = display_get_count(desc, it->count);

        if (!count)
        {
            return 0;
        }
    }

    if (count && count < it->count)
    {
        /* split the item if only a part of it is to be dropped */
        it = item_split(it, count);
        /* otherwise the entire quantity gets dropped */
    }

    if (!inv_del_element(inv, it))
    {
        /* if the callback failed, dropping has failed */
        return FALSE;
    }

    inv_add(map_ilist_at(game_map(nlarn, p->pos.z), p->pos), it);
    item_describe(it, player_item_known(p, it), FALSE, FALSE, desc, 60);

    log_add_entry(p->log, "You drop %s.", desc);

    return TRUE;
}

int player_item_pickup(player *p, inventory **inv, item *it)
{
    char desc[61];
    guint count = 0;

    assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if ((it->count > 1) && (it->type != IT_GOLD))
    {
        g_snprintf(desc, 60, "Pick up how many %s?", item_name_pl(it->type));

        count = display_get_count(desc, it->count);

        if (count == 0)
        {
            return 0;
        }

        if (count < it->count)
        {
            it = item_split(it, count);
        }
    }

    /* prepare item description for logging later.
     * this has to come before adding the item as it can already be freed at
     * this point (stackable items)
     */
    item_describe(it, player_item_known(p, it), FALSE, FALSE, desc, 60);

    if (inv_add(&p->inventory, it))
    {
        log_add_entry(p->log, "You pick up %s.", desc);
    }
    else
    {
        return FALSE;
    }

    inv_del_element(inv, it);

    /* one turn to pick item up, one to stuff it into the pack */
    return 2;
}

int player_altar_desecrate(player *p)
{
    effect *e = NULL;
    map *current;

    assert (p != NULL);

    current = game_map(nlarn, p->pos.z);

    if (map_stationary_at(current, p->pos) != LS_ALTAR)
    {
        log_add_entry(p->log, "I see no altar to desecrate here.");
        return FALSE;
    }

    if (chance(60))
    {
        /* try to find a space for the monster near the altar */
        position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                          LE_MONSTER);

        if (pos_valid(mpos))
        {
            /* create a monster - should be very dangerous */
            monster_new(MT_RED_DRAGON, mpos);
        }

        e = effect_new(ET_AGGRAVATE_MONSTER, game_turn(nlarn));
        e->turns = 2500;
        player_effect_add(p, e);
    }
    else if (chance(30))
    {
        /* destroy altar */
        log_add_entry(p->log, "The altar crumbles into a pile of dust before your eyes.");
        map_stationary_set(current, p->pos, LS_NONE);
    }
    else
    {
        log_add_entry(p->log, "Nothing happens.");
    }

    return TRUE;
}

int player_altar_pray(player *p)
{
    int donation = 0;
    int player_gold, tithe;
    effect *e = NULL;
    item *armour = NULL;
    map *current;

    assert (p != NULL);

    current = game_map(nlarn, p->pos.z);

    if (map_stationary_at(current, p->pos) != LS_ALTAR)
    {
        log_add_entry(p->log, "I see no altar to pray at here.");
        return FALSE;
    }

    player_gold = player_get_gold(p);
    donation = display_get_count("How much do you donate?", player_gold);

    if (donation == 0)
    {
        if (chance(75))
        {
            log_add_entry(p->log, "Nothing happens.");
        }
        else if (chance(4) && (armour = player_random_armour(p)))
        {
            /* enchant armour */
            log_add_entry(p->log, "You feel your %s vibrate for a moment.",
                          armour_name(armour));

            item_enchant(armour);
        }
        else if (chance(4) && p->eq_weapon)
        {
            /* enchant weapon */
            log_add_entry(p->log, "You feel your %s vibrate for a moment.",
                          weapon_name(p->eq_weapon));

            item_enchant(p->eq_weapon);
        }
        else
        {
            position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                              LE_MONSTER);

            if (pos_valid(mpos))
            {
                monster_new_by_level(mpos);
            }
        }
    }
    else if (player_gold >= donation)
    {
        tithe = player_gold / 10 ;
        player_set_gold(p, player_gold - donation);

        /* if player gave less than 10% of players gold, make a monster */
        if (donation < tithe || donation < rand_0n(50))
        {
            /* create a monster, it should be very dangerous */
            position mpos = map_find_space_in(current, rect_new_sized(p->pos, 1),
                                              LE_MONSTER);

            if (pos_valid(mpos))
            {
                monster_new_by_level(mpos);
            }

            e = effect_new(ET_AGGRAVATE_MONSTER, game_turn(nlarn));
            e->turns = 200;
            player_effect_add(p, e);

            return 1;
        }

        if (chance(50))
        {
            log_add_entry(p->log, "You have been heard!");

            e = effect_new(ET_PROTECTION, game_turn(nlarn));
            e->turns = 500;
            player_effect_add(p, e);

            e = effect_new(ET_UNDEAD_PROTECTION, game_turn(nlarn));
            player_effect_add(p, e);
        }

        if (chance(4) && (armour = player_random_armour(p)))
        {
            /* enchant weapon */
            log_add_entry(p->log, "You feel your %s vibrate for a moment.",
                          armour_name(armour));

            item_enchant(armour);
        }

        if (chance(4) && p->eq_weapon)
        {
            /* enchant weapon */
            log_add_entry(p->log, "You feel your %s vibrate for a moment.",
                          weapon_name(p->eq_weapon));

            item_enchant(p->eq_weapon);
        }

        log_add_entry(p->log, "Thank You.");
    }

    return 1;
}

int player_building_enter(player *p)
{
    int moves_count = 0;

    switch (map_stationary_at(game_map(nlarn, p->pos.z), p->pos))
    {
    case LS_BANK:
    case LS_BANK2:
        moves_count = building_bank(p);
        break;

    case LS_DNDSTORE:
        moves_count = building_dndstore(p);
        break;

    case LS_HOME:
        moves_count = building_home(p);
        break;

    case LS_LRS:
        moves_count = building_lrs(p);
        break;

    case LS_SCHOOL:
        moves_count = building_school(p);
        break;

    case LS_TRADEPOST:
        moves_count = building_tradepost(p);
        break;

    default:
        log_add_entry(p->log, "There is nothing to enter here.");
    }

    return moves_count;
}

int player_door_close(player *p)
{
    /* position used to interact with stationaries */
    position pos;

    /* possible directions of actions */
    int *dirs;

    /* direction of action */
    direction dir = GD_NONE;

    /* a counter and another one */
    int count, num;

    /* a monster */
    monster *m;

    /* the current map */
    map *map = game_map(nlarn, p->pos.z);

    dirs = map_get_surrounding(map, p->pos, LS_OPENDOOR);

    for (count = 0, num = 1; num < GD_MAX; num++)
    {
        if (dirs[num])
        {
            count++;
            dir = num;
        }
    }

    if (count > 1)
    {
        dir = display_get_direction("Close which door?", dirs);
    }
    /* dir has been set in the for loop above if count == 1 */
    else if (count == 0)
    {
        dir = GD_NONE;
    }

    g_free(dirs);

    /* select random direction if player is confused */
    if (player_effect(p, ET_CONFUSION))
    {
        dir = rand_0n(GD_MAX);
    }

    if (dir)
    {
        pos = pos_move(p->pos, dir);
        if (pos_valid(pos) && (map_stationary_at(map, pos) == LS_OPENDOOR))
        {

            /* check if player is standing in the door */
            if (pos_identical(pos, p->pos))
            {
                log_add_entry(p->log, "Please step out of the doorway.");
                return 0;
            }

            /* check for monster in the doorway */
            m = map_get_monster_at(map, pos);

            if (m)
            {
                log_add_entry(p->log,
                              "You cannot close the door. The %s is in the way.",
                              monster_name(m));
                return 0;
            }

            /* check for items in the doorway */
            if (*map_ilist_at(map, pos))
            {
                log_add_entry(p->log,
                              "You cannot close the door. There is something in the way.");
                return 0;
            }

            map_stationary_set(map, pos, LS_CLOSEDDOOR);
            log_add_entry(p->log, "You close the door.");
        }
        else
        {
            log_add_entry(p->log, "Huh?");
            return 0;
        }
    }
    else
    {
        log_add_entry(p->log, "Which door are you talking about?");
        return 0;
    }

    return 1;
}

int player_door_open(player *p)
{
    /* position used to interact with stationaries */
    position pos;

    /* possible directions of actions */
    int *dirs;

    /* direction of action */
    int dir = GD_NONE;

    /* a counter and another one */
    int count, num;

    /* the current map */
    map *map = game_map(nlarn, p->pos.z);

    dirs = map_get_surrounding(map, p->pos, LS_CLOSEDDOOR);

    for (count = 0, num = 1; num < GD_MAX; num++)
    {
        if (dirs[num])
        {
            count++;
            dir = num;
        }
    }

    if (count > 1)
    {
        dir = display_get_direction("Open which door?", dirs);
    }
    /* dir has been set in the for loop above if count == 1 */
    else if (count == 0)
    {
        dir = GD_NONE;
    }

    g_free(dirs);

    /* select random direction if player is confused */
    if (player_effect(p, ET_CONFUSION))
    {
        dir = rand_1n(GD_MAX);
    }

    if (dir)
    {
        pos = pos_move(p->pos, dir);

        if (pos_valid(pos) && (map_stationary_at(map, pos) == LS_CLOSEDDOOR))
        {
            map_stationary_set(map, pos, LS_OPENDOOR);
            log_add_entry(p->log, "You open the door.");
        }
        else
        {
            log_add_entry(p->log, "Huh?");
            return 0;
        }
    }
    else
    {
        log_add_entry(p->log, "Which door are you talking about?");
        return 0;
    }

    return 1;
}

int player_fountain_drink(player *p)
{
    effect *e = NULL;

    int fntchange = 0;
    int amount = 0;
    int et = ET_NONE;
    map *map = game_map(nlarn, p->pos.z);

    assert (p != NULL);

    if (map_stationary_at(map, p->pos) == LS_DEADFOUNTAIN)
    {
        log_add_entry(p->log, "There is no water to drink.");
        return 0;
    }

    if (map_stationary_at(map, p->pos) != LS_FOUNTAIN)
    {
        log_add_entry(p->log, "I see no fountain to drink from here.");
        return 0;
    }

    if (chance(7))
    {
        e = effect_new(ET_SICKNESS, game_turn(nlarn));
        player_effect_add(p, e);
    }
    else if (chance(13))
    {
        /* see invisible */
        e = effect_new(ET_INFRAVISION, game_turn(nlarn));
        player_effect_add(p, e);
    }
    else if (chance(45))
    {
        log_add_entry(p->log, "Nothing seems to have happened.");
    }
    else if (chance(15))
    {
        /* positive effect from list below */
        fntchange = 1;
    }
    else
    {
        /* negative effect from list below */
        fntchange = -1;
    }

    if (fntchange != 0)
    {
        switch (rand_1n(9))
        {
        case 1:
            if (fntchange > 0)
                et = ET_INC_STR;
            else
                et = ET_DEC_STR;
            break;

        case 2:
            if (fntchange > 0)
                et = ET_INC_INT;
            else
                et = ET_DEC_INT;
            break;

        case 3:
            if (fntchange > 0)
                et = ET_INC_WIS;
            else
                et = ET_DEC_WIS;
            break;

        case 4:
            if (fntchange > 0)
                et = ET_INC_CON;
            else
                et = ET_DEC_CON;
            break;

        case 5:
            if (fntchange > 0)
                et = ET_INC_DEX;
            else
                et = ET_DEC_DEX;
            break;

        case 6:
            if (fntchange > 0)
                et = ET_INC_CHA;
            else
                et = ET_DEC_CHA;
            break;

        case 7:
            amount = rand_1n(p->pos.z + 1);
            if (fntchange > 0)
            {
                log_add_entry(p->log, "You gain %d hit point%s",
                              amount, plural(amount));

                player_hp_gain(p, amount);
            }
            else
            {
                log_add_entry(p->log, "You lose %d hit point%s!",
                              amount, plural(amount));

                damage *dam = damage_new(DAM_NONE, amount, NULL);
                player_damage_take(p, dam, PD_STATIONARY, LS_FOUNTAIN);
            }

            break;

        case 8:
            amount = rand_1n(p->pos.z + 1);
            if (fntchange > 0)
            {
                log_add_entry(p->log, "You just gained %d mana point%s.",
                              amount, plural(amount));

                player_mp_gain(p, amount);
            }
            else
            {
                log_add_entry(p->log, "You just lost %d mana point%s.",
                              amount, plural(amount));

                player_mp_lose(p, amount);
            }
            break;

        case 9:
            amount = 5 * rand_1n((p->pos.z + 1) * (p->pos.z + 1));

            if (fntchange > 0)
            {
                log_add_entry(p->log, "You just gained experience.");
                player_exp_gain(p, amount);
            }
            else
            {
                log_add_entry(p->log, "You just lost experience.");
                player_exp_lose(p, amount);
            }
            break;
        }

        /* the rng stated that it wants the players attributes changed */
        if (et)
        {
            e = effect_new(et, game_turn(nlarn));
            player_effect_add(p, e);
        }
    }

    if (chance(25))
    {
        log_add_entry(p->log, "The fountains bubbling slowly quiets.");
        map_stationary_set(map, p->pos, LS_DEADFOUNTAIN);
    }

    return 1;
}

int player_fountain_wash(player *p)
{
    map *map = game_map(nlarn, p->pos.z);

    assert (p != NULL);

    if (map_stationary_at(map, p->pos) == LS_DEADFOUNTAIN)
    {
        log_add_entry(p->log, "There is no water to wash in.");
        return 0;
    }

    if (map_stationary_at(map, p->pos) != LS_FOUNTAIN)
    {
        log_add_entry(p->log, "I see no fountain to wash at here!");
        return 0;
    }

    if (chance(10))
    {
        log_add_entry(p->log, "Oh no! The water was foul!");

        damage *dam = damage_new(DAM_POISON,
                                 rand_1n((p->pos.z << 2) + 2), NULL);

        player_damage_take(p, dam, PD_STATIONARY, LS_FOUNTAIN);
    }
    else if (chance(30))
    {
        effect *e = NULL;
        if ((e = player_effect_get(p, ET_ITCHING)))
        {
            log_add_entry(p->log, "You got the dirt off!");
            player_effect_del(p, e);
        }
    }
    else if (chance(30))
    {
        log_add_entry(p->log, "This water seems to be hard water! " \
                      "The dirt didn't come off!");
    }
    else if (chance(35))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(map, rect_new_sized(p->pos, 1),
                                          LE_MONSTER);

        if (pos_valid(mpos))
        {
            /* make water lord */
            monster_new(MT_WATER_LORD, mpos);
        }
    }
    else
    {
        log_add_entry(p->log, "Nothing seems to have happened.");
    }

    return 1;
}

int player_stairs_down(player *p)
{
    map *nlevel = NULL;

    switch (map_stationary_at(game_map(nlarn, p->pos.z), p->pos))
    {
    case LS_STAIRSDOWN:
        nlevel = game_map(nlarn, p->pos.z + 1);;
        break;

    case LS_ELEVATORDOWN:
        /* first vulcano map */
        nlevel = game_map(nlarn, MAP_MAX - 1);
        break;

    case LS_ENTRANCE:
        if (p->pos.z == 0)
            nlevel = game_map(nlarn, 1);
        else
            log_add_entry(p->log, "Climb up to return to town.");
        break;

    default:
        return FALSE;
        break;
    }

    /* if told to switch level, do so */
    if (nlevel != NULL)
    {
        return player_map_enter(p, nlevel, FALSE);
    }

    return 0;
}

int player_stairs_up(player *p)
{
    map *nlevel = NULL;

    switch (map_stationary_at(game_map(nlarn, p->pos.z), p->pos))
    {
    case LS_STAIRSUP:
        nlevel = game_map(nlarn, p->pos.z - 1);
        break;

    case LS_ELEVATORUP:
        /* return to town */
        nlevel = game_map(nlarn, 0);
        break;

    case LS_ENTRANCE:
        if (p->pos.z == 1)
            nlevel = game_map(nlarn, 0);
        else
            log_add_entry(p->log, "Climb down to enter the dungeon.");

        break;
    default:
        log_add_entry(p->log, "I see no stairway up here.");
    }

    /* if told to switch level, do so */
    if (nlevel != NULL)
    {
        return player_map_enter(p, nlevel, FALSE);
    }

    return 0;
}

int player_throne_pillage(player *p)
{
    int i;
    int count = 0; /* gems created */

    /* current map */
    map *map = game_map(nlarn, p->pos.z);

    assert (p != NULL);

    if (map_stationary_at(map, p->pos) == LS_DEADTHRONE)
    {
        log_add_entry(p->log, "There are no gems left on this throne.");
        return 0;
    }

    if ((map_stationary_at(map, p->pos) != LS_THRONE)
            && (map_stationary_at(map, p->pos) != LS_THRONE2))
    {
        log_add_entry(p->log, "I see no throne here to remove gems from.");
        return 0;
    }

    if (chance(25))
    {
        for (i = 0; i < rand_1n(4); i++)
        {
            /* gems pop off the throne */
            inv_add(map_ilist_at(map, p->pos), item_new_random(IT_GEM));
            count++;
        }

        log_add_entry(p->log, "You manage to pry off %s gem%s.",
                      count > 1 ? "some" : "a", plural(count));

        map_stationary_set(map, p->pos, LS_DEADTHRONE);
    }
    else if (chance(40))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(map, rect_new_sized(p->pos, 1),
                                          LE_MONSTER);

        if (pos_valid(mpos))
        {
            /* make gnome king */
            monster_new(MT_GNOME_KING, mpos);

            /* next time there will be no gnome king */
            map_stationary_set(map, p->pos, LS_THRONE2);
        }
    }
    else
    {
        log_add_entry(p->log, "Nothing happens.");
    }

    return 1 + count;
}

int player_throne_sit(player *p)
{
    map *map = game_map(nlarn, p->pos.z);
    map_stationary_t st = map_stationary_at(map, p->pos);

    assert (p != NULL);

    if ((st != LS_THRONE) && (st != LS_THRONE2) && (st != LS_DEADTHRONE))
    {
        log_add_entry(p->log, "I see no throne to sit on here.");
        return 0;
    }

    if (chance(30) && (st == LS_THRONE))
    {
        /* try to find a space for the monster near the player */
        position mpos = map_find_space_in(map, rect_new_sized(p->pos, 1),
                                          LE_MONSTER);

        if (pos_valid(mpos))
        {
            /* make a gnome king */
            monster_new(MT_GNOME_KING, mpos);

            /* next time there will be no gnome king */
            map_stationary_set(map, p->pos, LS_THRONE2);
        }
    }
    else if (chance(35))
    {
        log_add_entry(p->log, "Zaaaappp! You've been teleported!");
        p->pos = map_find_space(map, LE_MONSTER);
    }
    else
    {
        log_add_entry(p->log, "Nothing happens.");
    }

    return 1;
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
    wc -= player_effect(p, ET_SICKNESS);

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

guint player_get_gold(player *p)
{
    guint idx;
    item *i;

    assert(p != NULL);

    for (idx = 0; idx < inv_length(p->inventory); idx++)
    {
        i = inv_get(p->inventory, idx);
        if (i->type == IT_GOLD)
            return i->count;
    }

    return 0;
}

guint player_set_gold(player *p, guint amount)
{
    guint idx;
    item *i;

    assert(p != NULL);

    for (idx = 0; idx < inv_length(p->inventory); idx++)
    {
        i = inv_get(p->inventory, idx);
        if (i->type == IT_GOLD)
        {
            if (!amount)
            {
                inv_del_element(&p->inventory, i);
            }
            else
            {
                i->count = amount;
            }

            /* force recalculation of inventory weight */
            player_inv_weight_recalc(p->inventory, NULL);

            return !amount ? amount : i->count;
        }
    }

    /* no gold found -> generate new gold */
    i = item_new(IT_GOLD, amount, 0);
    inv_add(&p->inventory, i);

    return i->count;
}

char *player_get_level_desc(player *p)
{
    assert(p != NULL);
    return (char *)player_level_desc[p->level];
}

/**
 * Player stepped on a trap
 *
 * @param the player
 * @param the trap
 * @return time this move took
 */
static int player_trap_trigger(player *p, trap_t trap)
{
    /* additional time of turn, if any */
    int time = 0;

    /* chance to trigger the trap on target tile */
    int possibility;

    possibility = trap_chance(trap);

    if (player_memory_of(p, p->pos).trap == trap)
        /* if player knows the trap a 5% chance remains */
        possibility = 5;

    if (chance(possibility))
    {
        log_add_entry(p->log, trap_p_message(trap));

        /* refresh player's knowlege of trap */
        player_memory_of(p, p->pos).trap = trap;

        switch (trap)
        {
        case TT_TRAPDOOR:
            time += player_map_enter(p, game_map(nlarn, p->pos.z + 1), TRUE);
            break;

        case TT_TELEPORT:
            p->pos = map_find_space(game_map(nlarn, p->pos.z), LE_MONSTER);
            break;

        default:
            if (trap_damage(trap))
            {
                damage *dam = damage_new(DAM_PHYSICAL,
                                         rand_1n(trap_damage(trap)) + p->pos.z,
                                         NULL);

                player_damage_take(p, dam, PD_TRAP, trap);
            }

            /* if there is an effect on the trap add it to player's effects. */
            if (trap_effect(trap) && chance(trap_effect_chance(trap)))
            {
                /* display message if there is one */
                if (trap_e_message(trap))
                {
                    log_add_entry(p->log, trap_e_message(trap));
                }

                player_effect_add(p, effect_new(trap_effect(trap), game_turn(nlarn)));
            }
        }

    }
    else if (player_memory_of(p, p->pos).trap == trap)
    {
        log_add_entry(p->log, "You evade the %s.", trap_description(trap));
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
    map *map;

    item *it;
    monster *m;

    area *enlight;

    const int mult[4][8] =
    {
        { 1,  0,  0, -1, -1,  0,  0,  1 },
        { 0,  1, -1,  0,  0, -1,  1,  0 },
        { 0,  1,  1,  0,  0, -1, -1,  0 },
        { 1,  0,  0,  1, -1,  0,  0, -1 }
    };

    /* reset FOV */
    memset(&(p->fov), 0, MAP_SIZE * sizeof(int));

    /* get current map */
    map = game_map(nlarn, p->pos.z);

    /* set level correctly */
    pos.z = p->pos.z;

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
    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            if (player_pos_visible(p, pos))
            {
                m = map_get_monster_at(map, pos);

                player_memory_of(p,pos).type = map_tiletype_at(map, pos);
                player_memory_of(p,pos).stationary = map_stationary_at(map, pos);

                if (map_ilist_at(map, pos)
                        && inv_length(*map_ilist_at(map, pos)))
                {
                    it = inv_get(*map_ilist_at(map, pos),
                                 inv_length(*map_ilist_at(map, pos)) - 1);

                    player_memory_of(p,pos).item = it->type;
                }
                else if (m && monster_type(m) == MT_MIMIC && monster_unknown(m))
                {
                    /* show the mimic as an item */
                    player_memory_of(p,pos).item = monster_item_type(m);
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
            if ((X < 0) || (X >= MAP_MAX_X))
                continue;

            if ((Y < 0) || (Y >= MAP_MAX_Y))
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
                    if (!map_pos_transparent(game_map(nlarn, p->pos.z), pos_new(X,Y, p->pos.z)))
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
                    if (!map_pos_transparent(game_map(nlarn, p->pos.z), pos_new(X, Y, p->pos.z)) && (j < radius))
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

static cJSON *player_memory_serialize(player *p, position pos)
{
    cJSON *mser;

    mser = cJSON_CreateObject();

    if (player_memory_of(p, pos).type > LT_NONE)
        cJSON_AddNumberToObject(mser, "type",
                                player_memory_of(p, pos).type);

    if (player_memory_of(p, pos).stationary > LS_NONE)
        cJSON_AddNumberToObject(mser, "stationary",
                                player_memory_of(p, pos).stationary);

    if (player_memory_of(p, pos).item > IT_NONE)
        cJSON_AddNumberToObject(mser, "item",
                                player_memory_of(p, pos).item);

    if (player_memory_of(p, pos).trap > TT_NONE)
        cJSON_AddNumberToObject(mser, "trap",
                                player_memory_of(p, pos).trap);

    return mser;
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

    case PD_STUCK:
        desc = "got stuck in solid rock";
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
                               a_an(monster_name_by_type(score->cause)),
                               monster_name_by_type(score->cause));
        break;

    case PD_SPHERE:
        g_string_append(text, " by a sphere of destruction.");
        break;

    case PD_TRAP:
        g_string_append_printf(text, " by a %s.", trap_description(score->cause));
        break;

    case PD_MAP:
        g_string_append_printf(text, " by %s.", lt_get_desc(score->cause));
        break;

    case PD_SPELL:
        g_string_append_printf(text, " %s with the spell \"%s\".",
                               score->sex ? "himself" : "herself",
                               spell_name_by_id(score->cause));
        break;

    case PD_CURSE:
        g_string_append_printf(text, " by a cursed %s.",
                               item_name_sg(score->cause));
        break;

    case PD_STATIONARY:
        /* currently only the fountain can cause death */
        g_string_append(text, " by toxic water from a fountain.");
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
