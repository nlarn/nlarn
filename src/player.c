/*
 * player.c
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

#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "container.h"
#include "display.h"
#include "fov.h"
#include "game.h"
#include "nlarn.h"
#include "player.h"
#include "random.h"
#include "sobjects.h"

static const char aa1[] = "mighty evil master";
static const char aa2[] = "apprentice demi-god";
static const char aa3[] = "minor demi-god";
static const char aa4[] = "major demi-god";
static const char aa5[] = "minor deity";
static const char aa6[] = "major deity";
static const char aa7[] = "novice guardian";
static const char aa8[] = "apprentice guardian";

static const char *player_level_desc[] =
{
    "novice explorer",     "apprentice explorer", "practiced explorer", /* -3 */
    "expert explorer",     "novice adventurer",   "adventurer",         /* -6 */
    "apprentice conjurer", "conjurer",            "master conjurer",    /*  -9 */
    "apprentice mage",     "mage",                "experienced mage",   /* -12 */
    "master mage",         "apprentice warlord",  "novice warlord",     /* -15 */
    "expert warlord",      "master warlord",      "apprentice gorgon",  /* -18 */
    "gorgon",              "practiced gorgon",    "master gorgon",      /* -21 */
    "demi-gorgon",         "evil master",         "great evil master",  /* -24 */
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
    "earth guardian", "air guardian",  "fire guardian",    /* -96*/
    "water guardian", "time guardian", "ethereal guardian",/* -99*/
    "The Creator", /* 100*/
};

/*
    table of experience needed to be a certain level of player
    FIXME: this is crap. Use a formula instead an get rid of this...
 */
static const guint32 player_lvl_exp[] =
{
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

/* function declarations */

static guint player_item_pickup(player *p, inventory **inv, item *it, gboolean ask);
static inline void player_item_pickup_all(player *p, inventory **inv, item *it)
{
    player_item_pickup(p, inv, it, FALSE);
}

static inline void player_item_pickup_ask(player *p, inventory **inv, item *it)
{
    player_item_pickup(p, inv, it, TRUE);
}

static void player_sobject_memorize(player *p, sobject_t sobject, position pos);
static int player_sobjects_sort(gconstpointer a, gconstpointer b);
static cJSON *player_memory_serialize(player *p, position pos);
static void player_memory_deserialize(player *p, position pos, cJSON *mser);
static char *player_death_description(game_score_t *score, int verbose);
static char *player_equipment_list(player *p, gboolean decorate);
static char *player_create_obituary(player *p, game_score_t *score, GList *scores);
static void player_memorial_file_save(player *p, const char *text);
static int item_filter_equippable(item *it);
static int item_filter_dropable(item *it);
static int player_item_filter_multiple(player *p, item *it);

player *player_new()
{
    player *p;
    item *it;

    /* initialize player */
    p = g_malloc0(sizeof(player));

    p->strength     = 12;
    p->constitution = 12;
    p->intelligence = 12;
    p->dexterity    = 12;
    p->wisdom       = 12;

    // some base stats
    p->hp = p->hp_max = (p->constitution + 5);
    p->mp = p->mp_max = (p->intelligence + 5);

    p->level = p->stats.max_level = 1;

    /* set the player's default speed */
    p->speed = SPEED_NORMAL;

    p->known_spells = g_ptr_array_new_with_free_func(
            (GDestroyNotify)spell_destroy);
    p->effects = g_ptr_array_new();
    p->inventory = inv_new(p);

    inv_callbacks_set(p->inventory, &player_inv_pre_add, &player_inv_weight_recalc,
                      NULL, &player_inv_weight_recalc);

    it = item_new(IT_ARMOUR, AT_LEATHER);
    it->bonus = 1;
    player_item_identify(p, NULL, it);
    inv_add(&p->inventory, it);
    player_item_equip(p, NULL, it);

    it = item_new(IT_WEAPON, WT_DAGGER);
    player_item_identify(p, NULL, it);
    inv_add(&p->inventory, it);
    player_item_equip(p, NULL, it);

    /* some items are always known */
    p->identified_potions[PO_CURE_DIANTHR] = TRUE;
    p->identified_potions[PO_WATER] = TRUE;
    p->identified_scrolls[ST_BLANK] = TRUE;
    /* identify all non-unique armour items */
    for (armour_t at = 0; at < AT_MAX; at++)
        if (!armours[at].unique) p->identified_armour[at] = TRUE;

    /* initialize the field of vision */
    p->fv = fov_new();

    return p;
}

gboolean player_assign_bonus_stats(player *p, char *preset)
{
    int selection = 0;
    const char *text = "  a) Strong character\n"
                       "  b) Agile character\n"
                       "  c) Tough character\n"
                       "  d) Smart character\n"
                       "  e) Randomly pick one of the above\n"
                       "  f) Stats assigned randomly\n";

    const char preset_min = 'a';
    const char preset_max = 'f';

    if (preset != NULL && strlen(preset) > 0)
    {
        selection = preset[0];
        if (selection < preset_min || selection > preset_max)
            return FALSE;
    }

    while (selection < preset_min || selection > preset_max)
    {
        selection = display_show_message("Choose a character build", text, 0);
    }

    if (selection == 'e')
        selection = 'a' + rand_0n(4);

    // Allow choice between:
    // * strong Fighter (Str 20  Dex 15  Con 16  Int 12  Wis 12)
    // * agile Rogue    (Str 15  Dex 20  Con 14  Int 12  Wis 14)
    // * hardy Fighter  (Str 16  Dex 12  Con 20  Int 12  Wis 15)
    // * arcane scholar (Str 12  Dex 14  Con 12  Int 20  Wis 17)
    switch (selection)
    {
    case 'a': // strong Fighter
        p->strength     += 8;
        p->dexterity    += 3;
        p->constitution += 4;
        p->intelligence += 0;
        p->wisdom       += 0;
        break;
    case 'b': // Rogue-style character
        p->strength     += 3;
        p->dexterity    += 8;
        p->constitution += 2;
        p->intelligence += 0;
        p->wisdom       += 2;
        break;
    case 'c': // hardy Fighter
        p->strength     += 4;
        p->dexterity    += 0;
        p->constitution += 8;
        p->intelligence += 0;
        p->wisdom       += 3;
        break;
    case 'd': // arcane scholar
        p->strength     += 0;
        p->dexterity    += 2;
        p->constitution += 0;
        p->intelligence += 8;
        p->wisdom       += 5;
        break;
    case 'f': // random character
    {
        int bonus = 15;
        while (bonus-- > 0)
        {
            switch (rand_1n(6))
            {
            case 1:
                p->strength++;
                break;
            case 2:
                p->dexterity++;
                break;
            case 3:
                p->constitution++;
                break;
            case 4:
                p->intelligence++;
                break;
            case 5:
                p->wisdom++;
                break;
            }
        }
    }
    }

    p->stats.str_orig = p->strength;
    p->stats.con_orig = p->constitution;
    p->stats.int_orig = p->intelligence;
    p->stats.dex_orig = p->dexterity;
    p->stats.wis_orig = p->wisdom;

    // Recalculate hp and mp because they depend on Con and Int, respectively.
    p->hp = p->hp_max = (p->constitution + 5);
    p->mp = p->mp_max = (p->intelligence + 5);

    return TRUE;
}

void player_destroy(player *p)
{
    g_assert(p != NULL);

    /* release spells */
    g_ptr_array_free(p->known_spells, TRUE);

    /* release effects */
    for (guint idx = 0; idx < p->effects->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(p->effects, idx);
        effect *e = game_effect_get(nlarn, effect_id);

        if (e->item == NULL)
        {
            effect_destroy(e);
        }
    }

    g_ptr_array_free(p->effects, TRUE);

    /* free memorized stationary objects */
    if (p->sobjmem != NULL)
    {
        g_array_free(p->sobjmem, TRUE);
    }

    inv_destroy(p->inventory, FALSE);

    if (p->name)
    {
        g_free(p->name);
    }

    /* clean the FOV */
    fov_free(p->fv);

    g_free(p);
}

cJSON *player_serialize(player *p)
{
    cJSON *obj;
    cJSON *pser = cJSON_CreateObject();

    cJSON_AddStringToObject(pser, "name", p->name);
    cJSON_AddNumberToObject(pser, "sex", p->sex);

    cJSON_AddNumberToObject(pser, "strength", p->strength);
    cJSON_AddNumberToObject(pser, "intelligence", p->intelligence);
    cJSON_AddNumberToObject(pser, "wisdom", p->wisdom);
    cJSON_AddNumberToObject(pser, "constitution", p->constitution);
    cJSON_AddNumberToObject(pser, "dexterity", p->dexterity);

    cJSON_AddNumberToObject(pser, "hp", p->hp);
    cJSON_AddNumberToObject(pser, "hp_max", p->hp_max);
    cJSON_AddNumberToObject(pser, "mp", p->mp);
    cJSON_AddNumberToObject(pser, "mp_max", p->mp_max);
    cJSON_AddNumberToObject(pser, "regen_counter", p->regen_counter);

    cJSON_AddNumberToObject(pser, "bank_account", p->bank_account);
    cJSON_AddNumberToObject(pser, "bank_ieslvtb", p->bank_ieslvtb);
    cJSON_AddNumberToObject(pser, "outstanding_taxes", p->outstanding_taxes);

    cJSON_AddNumberToObject(pser, "experience", p->experience);
    cJSON_AddNumberToObject(pser, "level", p->level);

    cJSON_AddNumberToObject(pser, "speed", p->speed);
    cJSON_AddNumberToObject(pser, "movement", p->movement);

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

    if (p->eq_sweapon)
        cJSON_AddNumberToObject(pser, "eq_sweapon",
                                GPOINTER_TO_UINT(p->eq_sweapon->oid));

    if (p->eq_quiver)
        cJSON_AddNumberToObject(pser, "eq_quiver",
                                GPOINTER_TO_UINT(p->eq_quiver->oid));

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
                          cJSON_CreateIntArray((int*)p->identified_amulets, AM_MAX));

    cJSON_AddItemToObject(pser, "identified_armour",
                          cJSON_CreateIntArray((int*)p->identified_armour, AT_MAX));

    cJSON_AddItemToObject(pser, "identified_books",
                          cJSON_CreateIntArray((int*)p->identified_books, SP_MAX));

    cJSON_AddItemToObject(pser, "identified_potions",
                          cJSON_CreateIntArray((int*)p->identified_potions, PO_MAX));

    cJSON_AddItemToObject(pser, "identified_rings",
                          cJSON_CreateIntArray((int*)p->identified_rings, RT_MAX));

    cJSON_AddItemToObject(pser, "identified_scrolls",
                          cJSON_CreateIntArray((int*)p->identified_scrolls, ST_MAX));

    cJSON_AddItemToObject(pser, "courses_taken",
                          cJSON_CreateIntArray(p->school_courses_taken, SCHOOL_COURSE_COUNT));

    cJSON_AddNumberToObject(pser, "position", pos_val(p->pos));

    /* store last targeted monster */
    if (p->ptarget != NULL)
    {
        cJSON_AddNumberToObject(pser, "ptarget", GPOINTER_TO_UINT(p->ptarget));
    }

    position pos = pos_invalid;

    /* store players' memory of the map */
    cJSON_AddItemToObject(pser, "memory", obj = cJSON_CreateArray());

    for (Z(pos) = 0; Z(pos) < MAP_MAX; Z(pos)++)
    {
        cJSON *mm = cJSON_CreateArray();

        for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
            for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
                cJSON_AddItemToArray(mm, player_memory_serialize(p, pos));

        cJSON_AddItemToArray(obj, mm);
    }

    /* store remembered stationary objects */
    if (p->sobjmem != NULL)
    {
        obj = cJSON_CreateArray();
        cJSON_AddItemToObject(pser, "sobjmem", obj);

        for (guint idx = 0; idx < p->sobjmem->len; idx++)
        {
            player_sobject_memory *som;
            cJSON *soms = cJSON_CreateObject();

            som = &g_array_index(p->sobjmem, player_sobject_memory, idx);

            cJSON_AddNumberToObject(soms, "pos", pos_val(som->pos));
            cJSON_AddNumberToObject(soms, "sobject", som->sobject);

            cJSON_AddItemToArray(obj, soms);
        }
    }

    /* settings */
    cJSON_AddItemToObject(pser, "settings", obj = cJSON_CreateObject());
    cJSON_AddItemToObject(obj, "auto_pickup",
                          cJSON_CreateIntArray(p->settings.auto_pickup, IT_MAX));

    /* statistics */
    cJSON_AddItemToObject(pser, "stats", obj = cJSON_CreateObject());

    cJSON_AddNumberToObject(obj, "deepest_level", p->stats.deepest_level);
    cJSON_AddItemToObject(obj, "monsters_killed",
                          cJSON_CreateIntArray(p->stats.monsters_killed, MT_MAX));

    cJSON_AddNumberToObject(obj, "spells_cast", p->stats.spells_cast);
    cJSON_AddNumberToObject(obj, "potions_quaffed", p->stats.potions_quaffed);
    cJSON_AddNumberToObject(obj, "scrolls_read", p->stats.scrolls_read);
    cJSON_AddNumberToObject(obj, "books_read", p->stats.books_read);
    cJSON_AddNumberToObject(obj, "weapons_wasted", p->stats.weapons_wasted);
    cJSON_AddNumberToObject(obj, "life_protected", p->stats.life_protected);
    cJSON_AddNumberToObject(obj, "vandalism", p->stats.vandalism);
    cJSON_AddNumberToObject(obj, "items_bought", p->stats.items_bought);
    cJSON_AddNumberToObject(obj, "items_sold", p->stats.items_sold);
    cJSON_AddNumberToObject(obj, "gems_sold", p->stats.gems_sold);
    cJSON_AddNumberToObject(obj, "gold_found", p->stats.gold_found);
    cJSON_AddNumberToObject(obj, "gold_sold_items", p->stats.gold_sold_items);
    cJSON_AddNumberToObject(obj, "gold_sold_gems", p->stats.gold_sold_gems);
    cJSON_AddNumberToObject(obj, "gold_bank_interest", p->stats.gold_bank_interest);
    cJSON_AddNumberToObject(obj, "gold_spent_shop", p->stats.gold_spent_shop);
    cJSON_AddNumberToObject(obj, "gold_spent_id_repair", p->stats.gold_spent_id_repair);
    cJSON_AddNumberToObject(obj, "gold_spent_donation", p->stats.gold_spent_donation);
    cJSON_AddNumberToObject(obj, "gold_spent_college", p->stats.gold_spent_college);
    cJSON_AddNumberToObject(obj, "gold_spent_taxes", p->stats.gold_spent_taxes);

    cJSON_AddNumberToObject(obj, "max_level", p->stats.max_level);
    cJSON_AddNumberToObject(obj, "max_xp", p->stats.max_xp);

    cJSON_AddNumberToObject(obj, "str_orig", p->stats.str_orig);
    cJSON_AddNumberToObject(obj, "int_orig", p->stats.int_orig);
    cJSON_AddNumberToObject(obj, "wis_orig", p->stats.wis_orig);
    cJSON_AddNumberToObject(obj, "con_orig", p->stats.con_orig);
    cJSON_AddNumberToObject(obj, "dex_orig", p->stats.dex_orig);
    return pser;
}

player *player_deserialize(cJSON *pser)
{
    player *p;
    cJSON *obj, *elem;

    p = g_malloc0(sizeof(player));

    p->name = g_strdup(cJSON_GetObjectItem(pser, "name")->valuestring);
    p->sex = cJSON_GetObjectItem(pser, "sex")->valueint;

    p->strength = cJSON_GetObjectItem(pser, "strength")->valueint;
    p->intelligence = cJSON_GetObjectItem(pser, "intelligence")->valueint;
    p->wisdom = cJSON_GetObjectItem(pser, "wisdom")->valueint;
    p->constitution = cJSON_GetObjectItem(pser, "constitution")->valueint;
    p->dexterity = cJSON_GetObjectItem(pser, "dexterity")->valueint;

    p->hp = cJSON_GetObjectItem(pser, "hp")->valueint;
    p->hp_max = cJSON_GetObjectItem(pser, "hp_max")->valueint;
    p->mp = cJSON_GetObjectItem(pser, "mp")->valueint;
    p->mp_max = cJSON_GetObjectItem(pser, "mp_max")->valueint;
    p->regen_counter = cJSON_GetObjectItem(pser, "regen_counter")->valueint;

    p->bank_account = cJSON_GetObjectItem(pser, "bank_account")->valueint;
    p->bank_ieslvtb = cJSON_GetObjectItem(pser, "bank_ieslvtb")->valueint;
    p->outstanding_taxes = cJSON_GetObjectItem(pser, "outstanding_taxes")->valueint;

    p->experience = cJSON_GetObjectItem(pser, "experience")->valueint;
    p->level = cJSON_GetObjectItem(pser, "level")->valueint;

    p->speed = cJSON_GetObjectItem(pser, "speed")->valueint;
    p->movement = cJSON_GetObjectItem(pser, "movement")->valueint;

    /* known spells */
    obj = cJSON_GetObjectItem(pser, "known_spells");
    if (obj != NULL)
        p->known_spells = spells_deserialize(obj);
    else
        p->known_spells = g_ptr_array_new_with_free_func(
                (GDestroyNotify)spell_destroy);


    /* inventory */
    obj = cJSON_GetObjectItem(pser, "inventory");
    if (obj != NULL)
        p->inventory = inv_deserialize(obj);
    else
        p->inventory = inv_new(p);

    p->inventory->owner = p;
    inv_callbacks_set(p->inventory, &player_inv_pre_add, &player_inv_weight_recalc,
                      NULL, &player_inv_weight_recalc);


    /* effects */
    obj = cJSON_GetObjectItem(pser, "effects");
    if (obj != NULL)
        p->effects = effects_deserialize(obj);
    else
        p->effects = g_ptr_array_new();

    /* equipped items */
    obj = cJSON_GetObjectItem(pser, "eq_amulet");
    if (obj != NULL) p->eq_amulet = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_weapon");
    if (obj != NULL) p->eq_weapon = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_sweapon");
    if (obj != NULL) p->eq_sweapon = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_quiver");
    if (obj != NULL) p->eq_quiver = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_boots");
    if (obj != NULL) p->eq_boots = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_cloak");
    if (obj != NULL) p->eq_cloak = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_gloves");
    if (obj != NULL) p->eq_gloves = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_helmet");
    if (obj != NULL) p->eq_helmet = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_shield");
    if (obj != NULL) p->eq_shield = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_suit");
    if (obj != NULL) p->eq_suit = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_ring_l");
    if (obj != NULL) p->eq_ring_l = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    obj = cJSON_GetObjectItem(pser, "eq_ring_r");
    if (obj != NULL) p->eq_ring_r = game_item_get(nlarn, GUINT_TO_POINTER(obj->valueint));

    /* identified items */
    obj = cJSON_GetObjectItem(pser, "identified_amulets");
    for (int idx = 0; idx < AM_MAX; idx++)
        p->identified_amulets[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(pser, "identified_armour");
    for (int idx = 0; idx < AT_MAX; idx++)
        p->identified_armour[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(pser, "identified_books");
    for (int idx = 0; idx < SP_MAX; idx++)
        p->identified_books[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(pser, "identified_potions");
    for (int idx = 0; idx < PO_MAX; idx++)
        p->identified_potions[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(pser, "identified_rings");
    for (int idx = 0; idx < RT_MAX; idx++)
        p->identified_rings[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(pser, "identified_scrolls");
    for (int idx = 0; idx < ST_MAX; idx++)
        p->identified_scrolls[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    obj = cJSON_GetObjectItem(pser, "courses_taken");
    for (int idx = 0; idx < SCHOOL_COURSE_COUNT; idx++)
        p->school_courses_taken[idx] = cJSON_GetArrayItem(obj, idx)->valueint;

    pos_val(p->pos) = cJSON_GetObjectItem(pser, "position")->valueint;

    /* restore last targeted monster */
    if ((obj = cJSON_GetObjectItem(pser, "ptarget")) != NULL)
    {
        p->ptarget = GUINT_TO_POINTER(obj->valueint);
    }

    /* restore players' memory of the map */
    position pos = pos_invalid;
    obj = cJSON_GetObjectItem(pser, "memory");

    for (Z(pos) = 0; Z(pos) < MAP_MAX; Z(pos)++)
    {
        elem = cJSON_GetArrayItem(obj, Z(pos));

        for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
        {
            for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
            {
                cJSON *tile = cJSON_GetArrayItem(elem, X(pos) + (MAP_MAX_X * Y(pos)));
                player_memory_deserialize(p, pos, tile);
            }
        }
    }

    /* remembered stationary objects */
    obj = cJSON_GetObjectItem(pser, "sobjmem");
    if (obj != NULL)
    {
        int count = cJSON_GetArraySize(obj);

        p->sobjmem = g_array_sized_new(FALSE, FALSE, sizeof(player_sobject_memory), count);

        for (int idx = 0; idx < count; idx++)
        {
            player_sobject_memory som;
            cJSON *soms = cJSON_GetArrayItem(obj, idx);

            pos_val(som.pos) = cJSON_GetObjectItem(soms, "pos")->valueint;
            som.sobject = cJSON_GetObjectItem(soms, "sobject")->valueint;

            g_array_append_val(p->sobjmem, som);
        }
    }

    /* settings */
    obj = cJSON_GetObjectItem(pser, "settings");
    if (obj)
    {
        elem = cJSON_GetObjectItem(obj, "auto_pickup");
        for (int idx = IT_NONE; idx < IT_MAX; idx++)
            p->settings.auto_pickup[idx] = (gboolean) cJSON_GetArrayItem(elem, idx)->valueint;
    }

    /* statistics */
    obj = cJSON_GetObjectItem(pser, "stats");

    p->stats.deepest_level = cJSON_GetObjectItem(obj, "deepest_level")->valueint;

    elem = cJSON_GetObjectItem(obj, "monsters_killed");
    for (int idx = 0; idx < MT_MAX; idx++)
        p->stats.monsters_killed[idx] = cJSON_GetArrayItem(elem, idx)->valueint;

    p->stats.spells_cast = cJSON_GetObjectItem(obj, "spells_cast")->valueint;
    p->stats.potions_quaffed = cJSON_GetObjectItem(obj, "potions_quaffed")->valueint;
    p->stats.scrolls_read = cJSON_GetObjectItem(obj, "scrolls_read")->valueint;
    p->stats.books_read = cJSON_GetObjectItem(obj, "books_read")->valueint;
    p->stats.weapons_wasted = cJSON_GetObjectItem(obj, "weapons_wasted")->valueint;
    p->stats.life_protected = cJSON_GetObjectItem(obj, "life_protected")->valueint;
    p->stats.vandalism = cJSON_GetObjectItem(obj, "vandalism")->valueint;
    p->stats.items_bought = cJSON_GetObjectItem(obj, "items_bought")->valueint;
    p->stats.items_sold   = cJSON_GetObjectItem(obj, "items_sold")->valueint;
    p->stats.gems_sold    = cJSON_GetObjectItem(obj, "gems_sold")->valueint;
    p->stats.gold_found         = cJSON_GetObjectItem(obj, "gold_found")->valueint;
    p->stats.gold_sold_items    = cJSON_GetObjectItem(obj, "gold_sold_items")->valueint;
    p->stats.gold_sold_gems     = cJSON_GetObjectItem(obj, "gold_sold_gems")->valueint;
    p->stats.gold_bank_interest = cJSON_GetObjectItem(obj, "gold_bank_interest")->valueint;
    p->stats.gold_spent_shop      = cJSON_GetObjectItem(obj, "gold_spent_shop")->valueint;
    p->stats.gold_spent_id_repair = cJSON_GetObjectItem(obj, "gold_spent_id_repair")->valueint;
    p->stats.gold_spent_donation  = cJSON_GetObjectItem(obj, "gold_spent_donation")->valueint;
    p->stats.gold_spent_college   = cJSON_GetObjectItem(obj, "gold_spent_college")->valueint;
    p->stats.gold_spent_taxes     = cJSON_GetObjectItem(obj, "gold_spent_taxes")->valueint;

    p->stats.max_level = cJSON_GetObjectItem(obj, "max_level")->valueint;
    p->stats.max_xp = cJSON_GetObjectItem(obj, "max_xp")->valueint;

    p->stats.str_orig = cJSON_GetObjectItem(obj, "str_orig")->valueint;
    p->stats.int_orig = cJSON_GetObjectItem(obj, "int_orig")->valueint;
    p->stats.wis_orig = cJSON_GetObjectItem(obj, "wis_orig")->valueint;
    p->stats.con_orig = cJSON_GetObjectItem(obj, "con_orig")->valueint;
    p->stats.dex_orig = cJSON_GetObjectItem(obj, "dex_orig")->valueint;

    /* initialize the field of vision */
    p->fv = fov_new();

    return p;
}

gboolean player_make_move(player *p, int turns, gboolean interruptible, const char *desc, ...)
{
    int frequency; /* number of turns between occasions */
    int regen = 0; /* amount of regeneration */
    effect *e; /* temporary var for effect */
    guint idx = 0;
    char *question = NULL, *description = NULL;

    g_assert(p != NULL);

    /* do do nothing if there is nothing to */
    if (turns == 0) return FALSE;

    /* return if the game has not been entirely set up */
    if (nlarn->p == NULL) return TRUE;

    // Initialize attacked marker.
    p->attacked = FALSE;

    /* assemble message and append it to the buffer */
    if (desc != NULL)
    {
        va_list argp;

        va_start(argp, desc);
        description = g_strdup_vprintf(desc, argp);
        va_end(argp);

        question = g_strdup_printf("Do you want to continue %s?", description);
    }
    else
    {
        question = g_strdup("Do you want to continue?");
    }

    /* modifier for frequency */
    frequency = game_difficulty(nlarn) << 3;

    do
    {
        /* if the number of movement points exceeds 100 reduce number
           of turns and handle regeneration and some effects */
        if (p->movement >= SPEED_NORMAL)
        {
            /* reduce the player's movement points */
            p->movement -= SPEED_NORMAL;
        }

        /* player's extra moves have expired - finish a turn */
        if (p->movement < SPEED_NORMAL)
        {
            /* check for time stop */
            if ((e = player_effect_get(p, ET_TIMESTOP)))
            {
                /* time has been stopped - handle player's movement locally */
                p->movement += player_get_speed(p);

                /* expire only time stop */
                if (effect_expire(e) == -1)
                {
                    /* time stop has expired - remove it*/
                    player_effect_del(p, e);
                }
                else
                {
                    /* nothing else happens while the time is stopped */
                    turns--;
                    continue;
                }
            }

            /* move the rest of the world */
            game_spin_the_wheel(nlarn);

            /* expire temporary effects */
            idx = 0; // reset idx for proper expiration during multiturn events
            while (idx < p->effects->len)
            {
                gpointer effect_id = g_ptr_array_index(p->effects, idx);
                e = game_effect_get(nlarn, effect_id);

                /* trapped counter only reduces on movement */
                if (e->type != ET_TRAPPED && effect_expire(e) == -1)
                {
                    /* effect has expired */
                    player_effect_del(p, e);
                }
                else
                {
                    /* give a warning if critical effects are about to time out */
                    if (e->turns == 5)
                    {
                        gboolean interrupt_actions = TRUE;
                        if (e->type == ET_WALL_WALK)
                            log_add_entry(nlarn->log, "Your attunement to the walls is fading!");
                        else if (e->type == ET_LEVITATION)
                            log_add_entry(nlarn->log, "You are starting to drift towards the ground!");
                        else
                            interrupt_actions = FALSE;

                        if (interrupt_actions)
                            p->attacked = TRUE;
                    }
                    idx++;
                }
            }

            /* handle regeneration */
            if (p->regen_counter == 0)
            {
                p->regen_counter = 22 + frequency;

                if (p->hp < player_get_hp_max(p))
                {
                    regen = 1 + player_effect(p, ET_INC_HP_REGEN);

                    player_hp_gain(p, regen);
                }

                if (p->mp < player_get_mp_max(p))
                {
                    regen = 1 + player_effect(p, ET_INC_MP_REGEN);

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
                if ((game_turn(nlarn) - e->start)
                        % (15 - game_difficulty(nlarn)) == 0)
                {
                    damage *dam = damage_new(DAM_POISON, ATT_NONE, e->amount,
                                             DAMO_NONE, NULL);

                    player_damage_take(p, dam, PD_EFFECT, e->type);
                }
            }

            /* handle clumsiness */
            if ((player_effect_get(p, ET_CLUMSINESS)) != NULL)
            {
                if (chance(33) && p->eq_weapon)
                {
                    item *it = p->eq_weapon;
                    player_item_unequip(p, NULL, it, TRUE);
                    log_add_entry(nlarn->log, "You are unable to hold your weapon.");
                    player_item_drop(p, &p->inventory, it);
                }
            }

            /* handle itching */
            if ((player_effect_get(p, ET_ITCHING)) != NULL)
            {
                item **aslot;

                /* when the player is subject to itching, there is a chance
                   that (s)he takes off a piece of armour */
                if (chance(50) && (aslot = player_get_random_armour(p, FALSE)))
                {
                    /* deference the item at the selected armour slot */
                    item *armour = *aslot;

                    log_add_entry(nlarn->log, "The hysteria of itching forces you to remove your armour!");
                    player_item_unequip(p, &p->inventory, armour, TRUE);
                    player_item_drop(p, &p->inventory, armour);
                }
            }

            /* handle multi-turn actions */
            if (turns > 1)
            {
                /* repaint the screen and do a little pause when the action
                   continues, for longer episodes a shorter time. */
                if (!interruptible || p->attacked)
                {
                    display_paint_screen(p);
                    napms((turns > 10) ? 1 : 50);
                }

                /* offer to abort the action if the player is under attack */
                if (p->attacked && interruptible)
                {
                    if (!display_get_yesno(question, NULL, NULL, NULL))
                    {
                        /* user chose to abort the current action */
                        if (description != NULL)
                        {
                            /* log the event */
                            log_add_entry(nlarn->log, "You stop %s.", description);
                        }

                        /* clean up */
                        p->attacked = FALSE;
                        g_free(description);
                        g_free(question);

                        return FALSE;
                    }
                }
                // Don't reset if only 1 turn left, so the wait/run checks
                // can process it correctly.
                p->attacked = FALSE;
            }
        } else {
            /* otherwise just clean up monsters killed by the player
               during the extra turn */
            game_remove_dead_monsters(nlarn);
        }

        /* reduce number of turns */
        turns--;
    }
    while (turns > 0);

    g_free(description);
    g_free(question);

    /* successfully completed the action */
    return TRUE;
}

void player_die(player *p, player_cod cause_type, int cause)
{
    const char *message = NULL;
    const char *title = NULL;
    effect *ef = NULL;

    g_assert(p != NULL);

    /* check for life protection */
    if ((cause_type < PD_STUCK) && (ef = player_effect_get(p, ET_LIFE_PROTECTION)))
    {
        log_add_entry(nlarn->log, "You feel wiiieeeeerrrrrd all over!");

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
        p->stats.life_protected++;

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

    case PD_DROWNED:
        message = "You've drowned in deep water.";
        title = "Deep the water, short the breath...";
        break;

    case PD_MELTED:
        message = "You've melted.";
        title = "Aaaaaaaaaaaaa!";
        break;

    case PD_GENOCIDE:
        message = "You've genocided yourself!";
        title = "Oops?";
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

    log_add_entry(nlarn->log, message);

    /* resume game if wizard mode is enabled */
    if (game_wizardmode(nlarn) && (cause_type < PD_TOO_LATE))
    {
        log_add_entry(nlarn->log, "WIZARD MODE. You stay alive.");

        /* return to full power */
        p->hp = p->hp_max;
        p->mp = p->mp_max;

        /* return to level 1 if last level has been lost */
        if (p->level < 1) p->level = 1;

        /* clear some nasty effects */
        if ((ef = player_effect_get(p, ET_PARALYSIS)))
            player_effect_del(p, ef);
        if ((ef = player_effect_get(p, ET_CONFUSION)))
            player_effect_del(p, ef);
        if ((ef = player_effect_get(p, ET_BLINDNESS)))
            player_effect_del(p, ef);
        if ((ef = player_effect_get(p, ET_POISON)))
            player_effect_del(p, ef);

        if (player_get_con(p) <= 0)
        {
            if ((ef = player_effect_get(p, ET_DEC_CON)))
                player_effect_del(p, ef);
        }
        if (player_get_dex(p) <= 0)
        {
            if ((ef = player_effect_get(p, ET_DEC_DEX)))
                player_effect_del(p, ef);
        }
        if (player_get_int(p) <= 0)
        {
            if ((ef = player_effect_get(p, ET_DEC_INT)))
                player_effect_del(p, ef);
        }
        if (player_get_str(p) <= 0)
        {
            if ((ef = player_effect_get(p, ET_DEC_STR)))
                player_effect_del(p, ef);
        }
        if (player_get_wis(p) <= 0)
        {
            if ((ef = player_effect_get(p, ET_DEC_WIS)))
                player_effect_del(p, ef);
        }

        /* return to the game */
        return;
    }

    /* We really died! */

    /* do not show scores when in wizard mode */
    if (!game_wizardmode(nlarn))
    {
        /* redraw screen to make sure player can see the cause of his death */
        display_paint_screen(p);

        /* sleep a second */
        g_usleep(1000000);

        /* flush keyboard input buffer */
        flushinp();

        game_score_t *score = game_score(nlarn, cause_type, cause);
        GList *scores = game_score_add(nlarn, score);

        /* create a description of the player's achievements */
        gchar *text = player_create_obituary(p, score, scores);

        /* free the memory allocated for the scores*/
        game_scores_destroy(scores);

        display_show_message(title, text, 0);

        if (display_get_yesno("Do you want to save a memorial " \
                              "file for your character?", NULL, NULL, NULL))
        {
            player_memorial_file_save(p, text);
        }

        g_free(text);
    }

    game_delete_savefile();
    game_destroy(nlarn);

    exit(EXIT_SUCCESS);
}

guint64 player_calc_score(player *p, int won)
{
    guint64 score = 0;

    g_assert (p != NULL);

    /* money */
    score = player_get_gold(p) + p->bank_account - p->outstanding_taxes;

    /* value of equipment */
    for (guint idx = 0; idx < inv_length(p->inventory); idx++)
    {
        score += item_price(inv_get(p->inventory, idx));
    }

    /* value of stuff stored at home */
    for (guint idx = 0; idx < inv_length(nlarn->player_home); idx++)
    {
        score += item_price(inv_get(nlarn->player_home, idx));
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

gboolean player_movement_possible(player *p)
{
    /* no movement if overloaded */
    if (player_effect(p, ET_OVERSTRAINED))
    {
        log_add_entry(nlarn->log, "You cannot move as long you are overstrained.");
        return FALSE;
    }

    /* no movement if paralyzed */
    if (player_effect(p, ET_PARALYSIS))
    {
        log_add_entry(nlarn->log, "You can't move!");
        return FALSE;
    }

    /* no movement if trapped */
    if (player_effect(p, ET_TRAPPED))
    {
        effect *e = player_effect_get(p, ET_TRAPPED);
        if (effect_expire(e) == -1)
        {
            /* effect has expired */
            log_add_entry(nlarn->log, "You climb out of the pit!");
            player_effect_del(p, e);
        }
        else
            log_add_entry(nlarn->log, "You're trapped in a pit!");

        player_make_move(p, 1, FALSE, NULL);
        return FALSE;
    }

    return TRUE;
}

int player_move(player *p, direction dir, gboolean open_door)
{
    int times = 1;      /* how many time ticks it took */
    position target_p;  /* coordinates of target */
    map *pmap;          /* shortcut to player's current map */
    monster *target_m;  /* monster on target tile (if any) */
    sobject_t so;       /* stationary object on target tile (if any) */
    gboolean move_possible = FALSE;

    g_assert(p != NULL && dir > GD_NONE && dir < GD_MAX);

    if (!player_movement_possible(p))
        return 0;

    /* confusion: random movement */
    if (player_effect(p, ET_CONFUSION))
        dir = rand_1n(GD_MAX);

    /* determine target position of move */
    target_p = pos_move(p->pos, dir);

    /* exceeded map limits */
    if (!pos_valid(target_p))
        return FALSE;

    /* make a shortcut to the current map */
    pmap = game_map(nlarn, Z(p->pos));

    if (open_door && map_sobject_at(pmap, target_p) == LS_CLOSEDDOOR)
        return player_door_open(nlarn->p, dir);

    target_m = map_get_monster_at(pmap, target_p);

    if (target_m && monster_unknown(target_m))
    {
        /* reveal the mimic */
        log_add_entry(nlarn->log, "Wait! That is a %s!", monster_get_name(target_m));
        monster_unknown_set(target_m, FALSE);
        return times;
    }

    if (target_m && (monster_action(target_m) == MA_SERVE))
    {
        /* bump into a servant -> swap position */

        /* reposition the player here, otherwise monster_pos_set will fail */
        position old_pos = p->pos;
        p->pos = target_p;
        monster_pos_set(target_m, pmap, old_pos);

        move_possible = TRUE;

        log_add_entry(nlarn->log, "You swap places with the %s.",
                      monster_get_name(target_m));
    }
    else if (target_m)
    {
        /* attack - no movement */
        return player_attack(p, target_m);
    }

    /* check if the move is possible */
    if (map_pos_passable(pmap, target_p))
    {
        /* target tile is passable */
        move_possible = TRUE;
    }
    else if ((map_tiletype_at(pmap, target_p) == LT_WALL)
             && player_effect(p, ET_WALL_WALK))
    {
        /* target tile is a wall and player can walk trough walls */
        move_possible = TRUE;
    }
    else if (map_pos_transparent(pmap, target_p)
             && player_effect(p, ET_LEVITATION))
    {
        /* player is able to float above the obstacle */
        move_possible = TRUE;
    }

    if (!move_possible)
    {
        /* return if the move is not possible */

        /* bump into walls when blinded or confused */
        if (player_effect(p, ET_BLINDNESS) || player_effect(p, ET_CONFUSION))
        {
            player_memory_of(p, target_p).type = map_tiletype_at(pmap, target_p);
            const int tile = map_tiletype_at(pmap, target_p);
            log_add_entry(nlarn->log, "Ouch! You bump into %s!",
                          (tile == LT_DEEPWATER || tile == LT_LAVA)
                          ? "the railing" : mt_get_desc(tile));
            return times;
        }

        return FALSE;
    }

    /* reposition player */
    p->pos = target_p;

    /* trigger the trap */
    if (map_trap_at(pmap, target_p))
    {
        times += player_trap_trigger(p, map_trap_at(pmap, target_p), FALSE);
    }

    /* auto-pickup */
    if (map_ilist_at(pmap, p->pos))
        player_autopickup(p);

    /* mention stationary objects at this position */
    if ((so = map_sobject_at(pmap, p->pos)) && !player_effect(p, ET_BLINDNESS))
    {
        log_add_entry(nlarn->log, "You see %s here.", so_get_desc(so));
    }

    return times;
}

typedef struct min_max_damage
{
    int min_damage;
    int max_damage;
} min_max_damage;

static min_max_damage calc_min_max_damage(player *p, monster *m)
{
    /* without weapon cause a basic unarmed combat damage */
    int min_damage = 1;

    if (p->eq_weapon != NULL)
    {
        /* wielded weapon base damage */
        min_damage = weapon_damage(p->eq_weapon);

        // Blessed weapons do 50% bonus damage against demons and undead.
        if (p->eq_weapon->blessed
                && (monster_flags(m, MF_DEMON) || monster_flags(m, MF_UNDEAD)))
        {
            min_damage *= 3;
            min_damage /= 2;
        }
    }

    min_damage += player_effect(p, ET_INC_DAMAGE);
    min_damage -= player_effect(p, ET_SICKNESS);

    /* calculate maximum damage: strength bonus and difficulty malus */
    int max_damage = min_damage
                     + player_get_str(p) - 12
                     - game_difficulty(nlarn);

    /* ensure minimal damage */
    min_max_damage ret =
    {
        max(1, min_damage),
        max(max(1, min_damage), max_damage)
    };

    return ret;
}

static gboolean player_instakill_chance(player *p, monster *m)
{
    if (p->eq_weapon)
    {
        switch (p->eq_weapon->id)
        {
            /* Vorpal Blade */
        case WT_VORPALBLADE:
            if (monster_flags(m, MF_HEAD) && !monster_flags(m, MF_NOBEHEAD))
                return 5;
            break;

            /* Lance of Death */
        case WT_LANCEOFDEATH:
            /* the lance is pretty deadly for non-demons */
            if (!monster_flags(m, MF_DEMON))
                return 100;
            break;

            /* Slayer */
        case WT_SLAYER:
            if (monster_flags(m, MF_DEMON))
                return 100;
            break;

        default:
            break;
        }
    }
    return 0;
}

static int calc_real_damage(player *p, monster *m, int allow_chance)
{
    const int INSTANT_KILL = 10000;
    const min_max_damage mmd = calc_min_max_damage(p, m);
    int real_damage = rand_m_n(mmd.min_damage, mmd.max_damage + 1);

    /* *** SPECIAL WEAPONS *** */
    if (p->eq_weapon)
    {
        switch (p->eq_weapon->id)
        {
            /* Vorpal Blade */
        case WT_VORPALBLADE:
            if (allow_chance && chance(5) && monster_flags(m, MF_HEAD)
                    && !monster_flags(m, MF_NOBEHEAD))
            {
                log_add_entry(nlarn->log, "You behead the %s with your Vorpal Blade!",
                              monster_get_name(m));

                real_damage = INSTANT_KILL;
            }
            break;

            /* Lance of Death */
        case WT_LANCEOFDEATH:
            /* the lance is pretty deadly for non-demons */
            if (!monster_flags(m, MF_DEMON))
                real_damage = INSTANT_KILL;
            else
                real_damage = 300;
            break;

            /* Slayer */
        case WT_SLAYER:
            if (monster_flags(m, MF_DEMON))
                real_damage = INSTANT_KILL;
            break;

        default:
            /* triple damage if hitting a dragon and wearing an amulet of
               dragon slaying */
            if (monster_flags(m, MF_DRAGON)
                    && (p->eq_amulet && p->eq_amulet->id == AM_DRAGON_SLAYING))
            {
                real_damage *= 3;
            }
            break;
        }
    }

    return real_damage;
}

int player_attack(player *p, monster *m)
{
    /* disallow attacking other humans */
    if (monster_type(m) == MT_TOWN_PERSON)
    {
        log_add_entry(nlarn->log, "You bump into the %s.", monster_get_name(m));
        return 1;
    }

    if (chance(5) || chance(weapon_calc_to_hit(p, m, p->eq_weapon, NULL)))
    {
        damage *dam;
        effect *e;

        /* placed a hit */
        log_add_entry(nlarn->log, "You hit the %s.", monster_get_name(m));

        dam = damage_new(DAM_PHYSICAL, ATT_WEAPON, calc_real_damage(p, m, TRUE),
                         DAMO_PLAYER, p);

        /* weapon damage due to rust when hitting certain monsters */
        if (p->eq_weapon && monster_flags(m, MF_METALLIVORE))
        {
            p->eq_weapon = item_erode(&p->inventory, p->eq_weapon, IET_RUST, TRUE);

            /* count destroyed weapons */
            if (p->eq_weapon == NULL) p->stats.weapons_wasted += 1;
        }

        /* The weapon may break during usage */
        if (p->eq_weapon && chance(item_fragility(p->eq_weapon)))
        {
            log_add_entry(nlarn->log, "Your %s breaks!",
                          weapon_name(p->eq_weapon));

            item *weapon = p->eq_weapon;

            /* remove the weapon */
            player_item_unequip(p, NULL, p->eq_weapon, TRUE);
            inv_del_element(&p->inventory, weapon);
            item_destroy(weapon);
            p->stats.weapons_wasted += 1;
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

        /* inflict damage */
        if (!(m = monster_damage_take(m, dam)))
        {
            /* killed the monster */
            return 1;
        } else {
           /* store the monster for subsequent ranged attacks */
           p->ptarget = monster_oid(m);
        }

        /* Lance of Death has not killed */
        if (p->eq_weapon && (p->eq_weapon->id == WT_LANCEOFDEATH)
                && monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "Your lance of death tickles the %s!",
                          monster_get_name(m));
        }

        /* if the player is invisible and the monster does not have infravision,
           remember the position where the attack came from
         */
        if (player_effect(p, ET_INVISIBILITY) && !monster_flags(m, MF_INFRAVISION))
        {
            monster_update_player_pos(m, p->pos);
        }
    }
    else
    {
        /* missed */
        log_add_entry(nlarn->log, "You miss the %s.", monster_get_name(m));
    }

    return 1; /* i.e. turns used */
}

int player_map_enter(player *p, map *l, gboolean teleported)
{
    g_assert(p != NULL && l != NULL);

    /* store the last turn player has been on this map */
    game_map(nlarn, Z(p->pos))->visited = game_turn(nlarn);

    if (p->stats.deepest_level < l->nlevel)
    {
        p->stats.deepest_level = l->nlevel;
    }

    /* been teleported here or something like that, need a random spot */
    if (teleported)
    {
        effect *e;
        if ((e = player_effect_get(p, ET_TRAPPED)))
            player_effect_del(p, e);

        p->pos = map_find_space(l, LE_MONSTER, FALSE);
    }

    /* beginning of the game */
    else if ((l->nlevel == 0) && (game_turn(nlarn) == 1))
        p->pos = map_find_sobject(l, LS_HOME);

    /* took the elevator down */
    else if ((Z(p->pos) == 0) && (l->nlevel == (MAP_DMAX)))
        p->pos = map_find_sobject(l, LS_ELEVATORUP);

    /* took the elevator up */
    else if ((Z(p->pos) == (MAP_DMAX)) && (l->nlevel == 0))
        p->pos = map_find_sobject(l, LS_ELEVATORDOWN);

    /* climbing up */
    else if (Z(p->pos) > l->nlevel)
    {
        if (l->nlevel == 0)
            p->pos = map_find_sobject(l, LS_DNGN_ENTRANCE);
        else
            p->pos = map_find_sobject(l, LS_STAIRSDOWN);
    }
    /* climbing down */
    else if (l->nlevel > Z(p->pos))
    {
        if (l->nlevel == 1)
            p->pos = map_find_sobject(l, LS_DNGN_EXIT);
        else
            p->pos = map_find_sobject(l, LS_STAIRSUP);
    }

    if (l->nlevel == 0)
    {
        /* do not log this at the start of the game */
        if (nlarn->log->gtime > 1)
            log_add_entry(nlarn->log, "You return to town.");
    }
    else if (l->nlevel == 1 && Z(p->pos) == 0)
        log_add_entry(nlarn->log, "You enter the caverns of Larn.");

    /* remove monster that might be at player's position */
    if ((map_get_monster_at(l, p->pos)))
    {
        position mnpos = map_find_space(l, LE_MONSTER, FALSE);
        monster_pos_set(map_get_monster_at(l, p->pos),
                        game_map(nlarn, Z(p->pos)), mnpos);
    }

    /* recalculate FOV to make ensure correct display after entering a level */
    player_update_fov(p);

    /* call autopickup */
    player_autopickup(p);

    /* automatic save point */
    if (game_autosave(nlarn) && (game_turn(nlarn) > 1))
    {
        game_save(nlarn, NULL);
    }

    return TRUE;
}

item **player_get_random_armour(player *p, int enchantable)
{
    GPtrArray *equipped_armour;
    item **armour = NULL;

    g_assert (p != NULL);

    equipped_armour = g_ptr_array_new();

    /* add each equipped piece of armour to the pool to choose from */
    if (p->eq_boots)  g_ptr_array_add(equipped_armour, &p->eq_boots);
    if (p->eq_cloak)  g_ptr_array_add(equipped_armour, &p->eq_cloak);
    if (p->eq_gloves) g_ptr_array_add(equipped_armour, &p->eq_gloves);
    if (p->eq_helmet) g_ptr_array_add(equipped_armour, &p->eq_helmet);
    if (p->eq_shield) g_ptr_array_add(equipped_armour, &p->eq_shield);
    if (p->eq_suit)   g_ptr_array_add(equipped_armour, &p->eq_suit);

    if (equipped_armour->len > 0)
    {
        int tries = 10;
        do
        {
            armour = g_ptr_array_index(equipped_armour, rand_0n(equipped_armour->len));
        }
        while (enchantable && tries-- > 0 && (*armour)->bonus == 3);
    }

    g_ptr_array_free(equipped_armour, TRUE);

    return armour;
}

void player_pickup(player *p)
{
    g_assert(p != NULL);

    inventory **inv = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);

    if (inv_length(*inv) == 0)
    {
        log_add_entry(nlarn->log, "There is nothing here.");
    }
    else if (player_effect(p, ET_LEVITATION))
    {
        log_add_entry(nlarn->log, "You cannot reach the floor!");
        return;
    }
    else if (player_effect(p, ET_PARALYSIS))
    {
        log_add_entry(nlarn->log, "You can't move!");
        return;
    }
    else if (inv_length(*inv) == 1)
    {
        player_item_pickup(p, inv, inv_get(*inv, 0), TRUE);
    }
    else
    {
        /* define callback functions */
        GPtrArray *callbacks = g_ptr_array_new();

        display_inv_callback *callback = g_malloc0(sizeof(display_inv_callback));
        callback->description = "(,) get";
        callback->helpmsg = "Get the item. In case of an item stack, get the entire stack.";
        callback->key = ',';
        callback->inv = inv;
        callback->function = (display_inv_callback_func)&player_item_pickup_all;
        g_ptr_array_add(callbacks, callback);

        callback = g_malloc0(sizeof(display_inv_callback));
        callback->description = "(g)et partly";
        callback->helpmsg = "Get the item. In case of an item stack, choose how many items to pick up.";
        callback->key = 'g';
        callback->inv = inv;
        callback->checkfun = &player_item_filter_multiple;
        callback->function = (display_inv_callback_func)&player_item_pickup_ask;
        g_ptr_array_add(callbacks, callback);

        display_inventory("On the floor", p, inv, callbacks, FALSE,
                          TRUE, FALSE, NULL);

        /* clean up callbacks */
        display_inv_callbacks_clean(callbacks);
    }
}

void player_autopickup(player *p)
{
    inventory **floor;
    int other_items_count = 0;
    int other_item_id     = -1;
    gboolean did_pickup   = FALSE;

    g_assert (p != NULL && map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos));

    /* if the player is floating above the ground auto-pickup does not work.. */
    if (player_effect(p, ET_LEVITATION))
        return;

    floor = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);

    for (guint idx = 0; idx < inv_length(*floor); idx++)
    {
        item *i = inv_get(*floor, idx);

        if (p->settings.auto_pickup[i->type] || i->fired)
        {
            /* item type is set to be picked up */
            guint retval;
            guint count_orig = inv_length(*floor);

            /* try to pick up the item */
            retval = player_item_pickup(p, floor, i, FALSE);

            if (retval == 1)
                /* pickup has been cancelled by the player */
                return;

            if (count_orig != inv_length(*floor))
            {
                /* item has been picked up */
                /* go back one item as the following items lowered their number */
                idx--;
                did_pickup = TRUE;
            }
        }
        else
        {
            if (++other_items_count == 1)
                other_item_id = idx;
        }
    }

    if (did_pickup && other_items_count > 0)
    {
        if (other_items_count == 1)
        {
            item *it = inv_get(*floor, other_item_id);
            gchar *it_desc = item_describe(it, player_item_known(nlarn->p, it),
                                           FALSE, FALSE);

            log_add_entry(nlarn->log, "There %s %s here.",
                          is_are(it->count), it_desc);

            g_free(it_desc);
        }
        else
        {
            log_add_entry(nlarn->log, "There are %d more items here.",
                          other_items_count);
        }
    }
}

void player_autopickup_show(player *p)
{
    GString *msg;
    int count = 0;

    g_assert (p != NULL);

    msg = g_string_new(NULL);

    for (item_t it = IT_NONE; it < IT_MAX; it++)
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


    log_add_entry(nlarn->log, msg->str);

    g_string_free(msg, TRUE);
}

void player_level_gain(player *p, int count)
{
    const char *desc_orig, *desc_new;

    g_assert(p != NULL && count > 0);

    /* experience level 100 is the end of the career */
    if (p->level == 100)
        return;

    desc_orig = player_get_level_desc(p);

    p->level += count;

    desc_new = player_get_level_desc(p);

    if (g_strcmp0(desc_orig, desc_new) != 0)
    {
        log_add_entry(nlarn->log, "You gain experience and become %s %s!",
                      a_an(desc_new), desc_new);
    }
    else
    {
        log_add_entry(nlarn->log, "You gain experience!");
    }

    if (p->level > p->stats.max_level)
        p->stats.max_level = p->level;

    if (p->experience < player_lvl_exp[p->level - 1])
        /* artificially gained a level, need to adjust XP to match level */
        player_exp_gain(p, player_lvl_exp[p->level - 1] - p->experience);

    for (int i = 0; i < count; i++)
    {
        int base;

        /* increase HP max */
        base = (p->constitution - game_difficulty(nlarn)) >> 1;
        if (p->level < (guint)max(7 - game_difficulty(nlarn), 0))
            base += p->constitution >> 2;

        player_hp_max_gain(p, rand_1n(3) + rand_0n(max(base, 1)));

        /* increase MP max */
        base = (p->intelligence - game_difficulty(nlarn)) >> 1;
        if (p->level < (guint)max(7 - game_difficulty(nlarn), 0))
            base += p->intelligence >> 2;

        player_mp_max_gain(p, rand_1n(3) + rand_0n(max(base, 1)));
    }
}

void player_level_lose(player *p, int count)
{
    g_assert(p != NULL && count > 0);

    p->level -= count;
    log_add_entry(nlarn->log, "You return to experience level %d...", p->level);

    /* die if lost level 1 */
    if (p->level == 0) player_die(p, PD_LASTLEVEL, 0);

    if (p->experience > player_lvl_exp[p->level - 1])
        /* adjust XP to match level */
        player_exp_lose(p, p->experience - player_lvl_exp[p->level - 1]);

    for (int i = 0; i < count; i++)
    {
        int base;

        /* decrease HP max */
        base = (p->constitution - game_difficulty(nlarn)) >> 1;
        if (p->level < (guint)max(7 - game_difficulty(nlarn), 0))
            base += p->constitution >> 2;

        player_hp_max_lose(p, rand_1n(3) + rand_0n(max(base, 1)));

        /* decrease MP max */
        base = (p->intelligence - game_difficulty(nlarn)) >> 1;
        if (p->level < (guint)max(7 - game_difficulty(nlarn), 0))
            base += p->intelligence >> 2;

        player_mp_max_lose(p, rand_1n(3) + rand_0n(max(base, 1)));
    }
}

void player_exp_gain(player *p, int count)
{
    if (count <= 0)
        return;

    int numlevels = 0;

    g_assert(p != NULL);
    p->experience += count;

    if (p->stats.max_xp < p->experience)
        p->stats.max_xp = p->experience;

    while (player_lvl_exp[p->level + numlevels] <= p->experience)
        numlevels++;

    if (numlevels)
        player_level_gain(p, numlevels);
}

void player_exp_lose(player *p, guint count)
{
    int numlevels = 0;

    g_assert(p != NULL && count > 0);

    if (count > p->experience)
        p->experience = 0;
    else
        p->experience -= count;

    while ((player_lvl_exp[p->level - 1 - numlevels]) > p->experience)
        numlevels++;

    if (numlevels)
        player_level_lose(p, numlevels);
}


int player_hp_gain(player *p, int count)
{
    g_assert(p != NULL);

    p->hp += count;
    if (p->hp > player_get_hp_max(p))
        p->hp = player_get_hp_max(p);

    return p->hp;
}

int player_hp_lose(player *p, int count, player_cod cause_type, int cause)
{
    g_assert(p != NULL);

    p->hp -= count;

    if (p->hp < 1)
    {
        player_die(p, cause_type, cause);
    }

    return p->hp;
}

void player_damage_take(player *p, damage *dam, player_cod cause_type, int cause)
{
    effect *e = NULL;
    int hp_orig;
    guint effects_count;

    g_assert(p != NULL && dam != NULL);

    if (game_wizardmode(nlarn))
        log_add_entry(nlarn->log, damage_to_str(dam));

    if (dam->dam_origin.ot == DAMO_MONSTER)
    {
        monster *m = (monster *)dam->dam_origin.originator;

        /* amulet of power cancels demon attacks */
        if (monster_flags(m, MF_DEMON) && chance(75)
                && (p->eq_amulet && p->eq_amulet->id == AM_POWER))
        {
            log_add_entry(nlarn->log, "Your amulet cancels the %s's attack.",
                          monster_get_name(m));

            return;
        }
    }

    if (dam->attack == ATT_GAZE && player_effect_get(p, ET_BLINDNESS))
    {
        /* it is impossible to see a staring monster when blinded */
        return;
    }

    /* store player's hit points and the number
       of effects before calculating the damage */
    hp_orig = p->hp;
    effects_count = p->effects->len;

    /* check resistances */
    switch (dam->type)
    {
    case DAM_PHYSICAL:
        if (dam->amount > (gint)player_get_ac(p))
        {
            dam->amount -= player_get_ac(p);

            if (dam->amount >= 8 && dam->amount >= (gint)p->hp_max/4)
                log_add_entry(nlarn->log, "Ouch, that REALLY hurt!");
            else if (dam->amount >= (gint)p->hp_max/10)
                log_add_entry(nlarn->log, "Ouch!");

            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "Your armour protects you.");
        }
        break;

    case DAM_MAGICAL:
        if (dam->amount > (gint)(guint)player_effect(p, ET_RESIST_MAGIC))
        {
            dam->amount -= player_effect(p, ET_RESIST_MAGIC);

            if (dam->amount >= 8 && dam->amount >= (gint)p->hp_max/4)
                log_add_entry(nlarn->log, "Ouch, that REALLY hurt!");
            else if (dam->amount >= (gint)p->hp_max/10)
                log_add_entry(nlarn->log, "Ouch!");

            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "You resist.");
        }

        break;

    case DAM_FIRE:
        if (dam->amount > (gint)player_effect(p, ET_RESIST_FIRE))
        {
            dam->amount -= player_effect(p, ET_RESIST_FIRE);

            log_add_entry(nlarn->log, "You suffer burns.");
            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "The flames don't phase you.");
        }
        break;

    case DAM_COLD:
        if (dam->amount > (gint)player_effect(p, ET_RESIST_COLD))
        {
            dam->amount -= player_effect(p, ET_RESIST_COLD);

            log_add_entry(nlarn->log, "You suffer from frostbite.");
            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "It doesn't seem so cold.");
        }
        break;

    case DAM_ACID:
        if (dam->amount > 0)
        {
            log_add_entry(nlarn->log, "You are splashed with acid.");
            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "The acid doesn't affect you.");
        }
        break;

    case DAM_WATER:
        if (dam->amount > 0)
        {
            log_add_entry(nlarn->log, "You experience near-drowning.");
            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "The water doesn't affect you.");
        }
        break;

    case DAM_ELECTRICITY:
        /* double damage if levitating */
        if (player_effect(p, ET_LEVITATION))
            dam->amount *= 2;

        if (dam->amount > 0)
        {
            log_add_entry(nlarn->log, "Zapp!");
            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        else
        {
            log_add_entry(nlarn->log, "As you are grounded nothing happens.");
        }
        break;

    case DAM_POISON:
        /* check if the damage is not caused by the effect that is
           already attached to the player */
        if (cause_type != PD_EFFECT)
        {
            /* check resistance; prevent negative damage amount */
            dam->amount = max(0, dam->amount - rand_0n(player_get_con(p)));

            if (dam->amount > 0)
            {
                e = effect_new(ET_POISON);
                e->amount = dam->amount;
                player_effect_add(p, e);
            }
            else
            {
                log_add_entry(nlarn->log, "You resist the poison.");
            }
        }
        else
        {
            /* damage is caused by the effect of the poison effect () */
            log_add_entry(nlarn->log, "You feel poison running through your veins.");
            player_hp_lose(p, dam->amount, cause_type, cause);
        }
        break;

    case DAM_BLINDNESS:
        if (chance(dam->amount))
        {
            player_effect_add(p, effect_new(ET_BLINDNESS));
        }
        else if (player_effect_get(p, ET_BLINDNESS) == NULL)
        {
            log_add_entry(nlarn->log, "You are not blinded.");
        }

        break;

    case DAM_CONFUSION:
        /* check if the player succumbs to the monster's stare */
        if (chance(dam->amount - player_get_int(p)))
        {
            player_effect_add(p, effect_new(ET_CONFUSION));
        }
        else if (player_effect_get(p, ET_CONFUSION) == NULL)
        {
            log_add_entry(nlarn->log, "You are not confused.");
        }

        break;

    case DAM_PARALYSIS:
        /* check if the player succumbs to the monster's stare */
        if (chance(dam->amount - player_get_int(p)))
        {
            player_effect_add(p, effect_new(ET_PARALYSIS));
        }
        else if (player_effect_get(p, ET_PARALYSIS) == NULL)
        {
            log_add_entry(nlarn->log, "You avoid eye contact.");
        }

        break;

    case DAM_DEC_CON:
    case DAM_DEC_DEX:
    case DAM_DEC_INT:
    case DAM_DEC_STR:
    case DAM_DEC_WIS:
        if (!player_effect(p, ET_SUSTAINMENT)
            && chance(dam->amount -= player_get_con(p)))
        {
            effect_t et = (ET_DEC_CON + dam->type - DAM_DEC_CON);
            e = effect_new(et);
            /* the default number of turns is 1 */
            e->turns = dam->amount * 10;
            (void)player_effect_add(p, e);

            switch (dam->type)
            {
            case DAM_DEC_CON:
                if (player_get_con(p) < 1)
                    player_die(p, PD_EFFECT, ET_DEC_CON);
                break;

            case DAM_DEC_DEX:
                if (player_get_dex(p) < 1)
                    player_die(p, PD_EFFECT, ET_DEC_DEX);
                break;

            case DAM_DEC_INT:
                if (player_get_int(p) < 1)
                    player_die(p, PD_EFFECT, ET_DEC_INT);
                break;

            case DAM_DEC_STR:
                /* strength has been modified -> recalc burdened status */
                player_inv_weight_recalc(p->inventory, NULL);

                if (player_get_str(p) < 1)
                    player_die(p, PD_EFFECT, ET_DEC_STR);
                break;

            case DAM_DEC_WIS:
                if (player_get_wis(p) < 1)
                    player_die(p, PD_EFFECT, ET_DEC_WIS);
                break;

            default:
                break;
            }
        }
        else
        {
            log_add_entry(nlarn->log, "You are not affected.");
        }
        break;

    case DAM_DRAIN_LIFE:
        if (player_effect(p, ET_UNDEAD_PROTECTION)
                || !chance(dam->amount - player_get_wis(p)))
        {
            /* undead protection cancels drain life attacks */
            log_add_entry(nlarn->log, "You are not affected.");
        }
        else
        {
            log_add_entry(nlarn->log, "Your life energy is drained.");
            player_level_lose(p, 1);

            /* this is the only attack that can not be caught by the test below */
            p->attacked = TRUE;
        }
        break;

    default:
        /* the other damage types are not handled here */
        break;
    }

    g_free(dam);

    if (game_wizardmode(nlarn))
        log_add_entry(nlarn->log, "[applied: %d]", hp_orig - p->hp);

    /* check if an attack had an effect */
    if (p->hp < hp_orig || p->effects->len > effects_count)
    {
        /* set the attacked flag */
        p->attacked = TRUE;
    }
}

int player_hp_max_gain(player *p, int count)
{
    g_assert(p != NULL);

    p->hp_max += count;
    /* do _NOT_ increase hp */

    return p->hp_max;
}

int player_hp_max_lose(player *p, int count)
{
    g_assert(p != NULL);

    p->hp_max -= count;

    if (p->hp_max < 1)
        p->hp_max = 1;

    if (p->hp > (int)p->hp_max)
        p->hp = p->hp_max;

    return p->hp_max;
}

int player_mp_gain(player *p, int count)
{
    g_assert(p != NULL);

    p->mp += count;
    if (p->mp > player_get_mp_max(p))
        p->mp = player_get_mp_max(p);

    return p->mp;
}

int player_mp_lose(player *p, int count)
{
    g_assert(p != NULL);

    p->mp -= count;
    if (p->mp < 0)
        p->mp = 0;

    return p->mp;
}

int player_mp_max_gain(player *p, int count)
{
    g_assert(p != NULL);

    p->mp_max += count;
    /* do _NOT_ increase mp */

    return p->mp_max;
}

int player_mp_max_lose(player *p, int count)
{
    g_assert(p != NULL);

    p->mp_max -= count;

    if (p->mp_max < 1)
        p->mp_max = 1;

    if (p->mp > (int)p->mp_max)
        p->mp = p->mp_max;

    return p->mp_max;
}

effect *player_effect_add(player *p, effect *e)
{
    g_assert(p != NULL && e != NULL);

    /* one-time effects are handled here */
    if (e->turns == 1)
    {
        switch (e->type)
        {
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
            player_inv_weight_recalc(p->inventory, NULL);
            break;

        case ET_INC_WIS:
            p->wisdom += e->amount;
            break;

        case ET_INC_RND:
            while (e->amount-- > 0)
            {
                player_effect_add(p, effect_new(rand_m_n(ET_INC_CON,
                                                ET_INC_WIS)));
            }
            break;

        case ET_INC_HP_MAX:
            {
                float delta = (player_get_hp_max(p) / 100.0) * e->amount;
                int amount = max(1, (int)round(delta));
                p->hp_max += amount;
                player_hp_gain(p, amount);
            }
            break;

        case ET_INC_MP_MAX:
            {
                float delta = (player_get_mp_max(p) / 100.0) * e->amount;
                int amount = max(1, (int)round(delta));
                p->mp_max += amount;
                player_mp_gain(p, amount);
            }
            break;

        case ET_INC_LEVEL:
            player_level_gain(p, e->amount);
            break;

        case ET_INC_EXP:
            /* looks like a reasonable amount */
            player_exp_gain(p, rand_1n(player_lvl_exp[p->level - 1]
                                    - player_lvl_exp[p->level - 2]));
            break;

        case ET_INC_HP:
        case ET_MAX_HP:
            {
                /* also cure sickness, if present */
                effect *sickness;
                if ((sickness = player_effect_get(p, ET_SICKNESS)))
                {
                    player_effect_del(p, sickness);
                }
            }
            if (p->hp != (int)p->hp_max)
            {
                    guint amount = (p->hp_max * e->amount) / 100;
                    player_hp_gain(p, amount);
            }
            else if (e->item)
            {
                    /*
                     * Player's hp is at the max; increase max hp.
                     * (only when drinking a potion)
                     */
                    guint amount = e->amount;

                    effect_destroy(e);
                    e = effect_new(ET_INC_HP_MAX);
                    /* determine a reasonable percentage max_hp will increase,
                       i.e. 5% for the MAX_HP, 1% for INC_HP */
                    e->amount = amount / 20;
                    return player_effect_add(p, e);
            }
            break;

        case ET_INC_MP:
        case ET_MAX_MP:
            if (p->mp != (int)p->mp_max)
            {
                    guint amount = (p->mp_max * e->amount) / 100;
                    player_mp_gain(p, amount);
            }
            else if (e->item)
            {
                    /*
                     * Player's mp is at the max; increase max mp.
                     * (only when drinking a potion)
                     */
                    guint amount = e->amount;

                    effect_destroy(e);
                    e = effect_new(ET_INC_MP_MAX);
                    /* determine a reasonable percentage max_mp will increase,
                       i.e. 5% for the MAX_MP, 1% for INC_MP */
                    e->amount = amount / 20;
                    return player_effect_add(p, e);
            }
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
            player_inv_weight_recalc(p->inventory, NULL);
            break;

        case ET_DEC_WIS:
            p->wisdom -= e->amount;
            break;

        case ET_DEC_RND:
            player_effect_add(p, effect_new(rand_m_n(ET_DEC_CON, ET_DEC_WIS)));
            break;

        default:
            /* nop */
            break;
        }

        if (effect_get_amount(e) > 0 && effect_get_msg_start(e))
            log_add_entry(nlarn->log, "%s", effect_get_msg_start(e));
        else if (effect_get_amount(e) < 0 && effect_get_msg_stop(e))
            log_add_entry(nlarn->log, "%s", effect_get_msg_stop(e));

        effect_destroy(e);
        e = NULL;
    }
    else if (e->type == ET_SLEEP)
    {
        if (effect_get_msg_start(e))
            log_add_entry(nlarn->log, "%s", effect_get_msg_start(e));

        player_make_move(p, e->turns, FALSE, NULL);

        effect_destroy(e);
        e = NULL;
    }
    else
    {
        int str_orig = player_get_str(p);

        e = effect_add(p->effects, e);

        /* only log a message if the effect has really been added and
           actually has a value */
        if (e)
        {
            if (effect_get_amount(e) > 0 && effect_get_msg_start(e))
                log_add_entry(nlarn->log, "%s", effect_get_msg_start(e));
            else if (effect_get_amount(e) < 0 && effect_get_msg_stop(e))
                log_add_entry(nlarn->log, "%s", effect_get_msg_stop(e));
        }

        if (str_orig != player_get_str(p))
        {
            /* strength has been modified -> recalc burdened status */
            player_inv_weight_recalc(p->inventory, NULL);
        }
    }

    return e;
}

void player_effects_add(player *p, GPtrArray *effects)
{
    g_assert (p != NULL);

    /* if effects is NULL */
    if (!effects) return;

    for (guint idx = 0; idx < effects->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(effects, idx);
        effect *e = game_effect_get(nlarn, effect_id);
        player_effect_add(p, e);
    }
}

int player_effect_del(player *p, effect *e)
{
    int result, str_orig;

    g_assert(p != NULL && e != NULL && e->type > ET_NONE && e->type < ET_MAX);

    str_orig = player_get_str(p);

    if ((result = effect_del(p->effects, e)))
    {
        if (effect_get_amount(e) > 0 && effect_get_msg_stop(e))
            log_add_entry(nlarn->log, "%s", effect_get_msg_stop(e));
        else if (effect_get_amount(e) < 0 && effect_get_msg_start(e))
            log_add_entry(nlarn->log, "%s", effect_get_msg_start(e));

        if (str_orig != player_get_str(p))
        {
            /* strength has been modified -> recalc burdened status */
            player_inv_weight_recalc(p->inventory, NULL);
        }

        /* finally destroy the effect if its not bound to an item*/
        if (!e->item)
            effect_destroy(e);
    }

    return result;
}

void player_effects_del(player *p, GPtrArray *effects)
{
    g_assert (p != NULL);

    /* if effects is NULL */
    if (!effects) return;

    for (guint idx = 0; idx < effects->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(effects, idx);
        effect *e = game_effect_get(nlarn, effect_id);
        player_effect_del(p, e);
    }
}

effect *player_effect_get(player *p, effect_t et)
{
    g_assert(p != NULL && et > ET_NONE && et < ET_MAX);
    return effect_get(p->effects, et);
}

int player_effect(player *p, effect_t et)
{
    g_assert(p != NULL && et > ET_NONE && et < ET_MAX);
    return effect_query(p->effects, et);
}

char **player_effect_text(player *p)
{
    char **text = strv_new();

    for (guint pos = 0; pos < p->effects->len; pos++)
    {
        effect *e = game_effect_get(nlarn, g_ptr_array_index(p->effects, pos));

        if (effect_get_desc(e) != NULL)
        {
            strv_append_unique(&text, effect_get_desc(e));
        }
    }

    return text;
}

int player_inv_display(player *p)
{
    GPtrArray *callbacks;
    display_inv_callback *callback;

    g_assert(p != NULL);

    if (inv_length(p->inventory) == 0)
    {
        /* don't show empty inventory */
        log_add_entry(nlarn->log, "You do not carry anything.");
        return FALSE;
    }

    /* define callback functions */
    callbacks = g_ptr_array_new();

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(d)rop";
    callback->helpmsg = "Drop the selected item. If the item is a stack of multiple items, you will be prompted for the amount you want to drop.";
    callback->key = 'd';
    callback->inv = &p->inventory;
    callback->function = &player_item_drop;
    callback->checkfun = &player_item_is_dropable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(e)quip";
    callback->helpmsg = "Equip the selected item.";
    callback->key = 'e';
    callback->function = &player_item_equip;
    callback->checkfun = &player_item_is_equippable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(o)pen";
    callback->helpmsg = "Open the selected container.";
    callback->key = 'o';
    callback->function = &container_open;
    callback->checkfun = &player_item_is_container;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(s)tore";
    callback->helpmsg = "Put the item into a container you carry or one that is on the floor.";
    callback->key = 's';
    callback->function = &container_item_add;
    callback->checkfun = &player_item_can_be_added_to_container;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(u)nequip";
    callback->helpmsg = "Unequip the selected item.";
    callback->key = 'u';
    callback->function = &player_item_unequip_wrapper;
    callback->checkfun = &player_item_is_unequippable;
    g_ptr_array_add(callbacks, callback);

    /* unequip and use should never appear together */
    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(u)se";
    callback->helpmsg = "Use the selected item.";
    callback->key = 'u';
    callback->function = &player_item_use;
    callback->checkfun = &player_item_is_usable;
    g_ptr_array_add(callbacks, callback);

    callback = g_malloc0(sizeof(display_inv_callback));
    callback->description = "(n)ote";
    callback->helpmsg = "Add a note to the selected item or edit or delete an existing note.";
    callback->key = 'n';
    callback->function = &player_item_notes;
    g_ptr_array_add(callbacks, callback);

    /* display inventory */
    display_inventory("Inventory", p, &p->inventory, callbacks, FALSE,
                      TRUE, FALSE, NULL);

    /* clean up */
    display_inv_callbacks_clean(callbacks);

    return TRUE;
}

static char *player_print_weight(float weight)
{
    static char buf[21] = "";

    const char *unit = "g";
    if (weight > 1000)
    {
        weight = weight / 1000;
        unit = "kg";
    }

    g_snprintf(buf, 20, "%g%s", weight, unit);

    return buf;
}

char *player_can_carry(player *p)
{
    static char buf[21] = "";
    g_snprintf(buf, 20, "%s",
               player_print_weight(2000 * 1.3 * (float)player_get_str(p)));
    return buf;
}

char *player_inv_weight(player *p)
{
    static char buf[21] = "";
    g_snprintf(buf, 20, "%s",
               player_print_weight((float)inv_weight(p->inventory)));
    return buf;
}

int player_inv_pre_add(inventory *inv, item *it)
{
    player *p;
    int pack_weight;
    float can_carry;

    p = (player *)inv->owner;

    if (player_effect(p, ET_OVERSTRAINED))
    {
        log_add_entry(nlarn->log, "You are already overloaded!");
        return FALSE;
    }

    /* calculate inventory weight */
    pack_weight = inv_weight(inv);

    /* player can carry 2kg per str */
    can_carry = 2000 * (float)player_get_str(p);

    /* check if item weight can be carried */
    if ((pack_weight + item_weight(it)) > (int)(can_carry * 1.3))
    {
        /* get item description */
        gchar *buf = item_describe(it, player_item_known(p, it), FALSE, TRUE);
        /* capitalize first letter */
        buf[0] = g_ascii_toupper(buf[0]);

        log_add_entry(nlarn->log, "%s %s too heavy for you.", buf,
                      is_are(it->count));

        g_free(buf);
        return FALSE;
    }

    return TRUE;
}

void player_inv_weight_recalc(inventory *inv, item *it __attribute__((unused)))
{
    int pack_weight;
    float can_carry;
    effect *e = NULL;

    player *p;

    g_assert (inv != NULL);

    p = (player *)inv->owner;       /* make shortcut */
    pack_weight = inv_weight(inv);  /* calculate inventory weight */

    /* the player can carry 2kg per str */
    can_carry = 2000 * (float)player_get_str(p);

    if (pack_weight > (int)(can_carry * 1.3))
    {
        /* OVERSTRAINED  */
        if ((e = player_effect_get(p, ET_BURDENED)))
        {
            /* get rid of burden effect (mute log to avoid pointless message) */
            log_disable(nlarn->log);
            player_effect_del(p, e);
            log_enable(nlarn->log);
        }

        /* make overstrained */
        if (!player_effect(p, ET_OVERSTRAINED))
        {
            player_effect_add(p, effect_new(ET_OVERSTRAINED));
        }
    }
    else if (pack_weight < (int)(can_carry * 1.3) && (pack_weight > can_carry))
    {
        /* BURDENED */
        if ((e = player_effect_get(p, ET_OVERSTRAINED)))
        {
            /* get rid of overstrained effect */
            log_disable(nlarn->log);
            player_effect_del(p, e);
            log_enable(nlarn->log);
        }

        if (!player_effect(p, ET_BURDENED))
        {
            player_effect_add(p, effect_new(ET_BURDENED));
        }
    }
    else if (pack_weight < can_carry)
    {
        /* NOT burdened, NOT overstrained */
        if ((e = player_effect_get(p, ET_OVERSTRAINED)))
        {
            player_effect_del(p, e);
        }

        if ((e = player_effect_get(p, ET_BURDENED)))
        {
            player_effect_del(p, e);
        }
    }
}

void player_paperdoll(player *p)
{
    gchar *equipment = player_equipment_list(p, TRUE);

    if (strlen(equipment) > 0)
        display_show_message("Worn equipment", equipment, 0);
    else
        log_add_entry(nlarn->log, "You do not wear any equipment.");

    g_free(equipment);
}

void player_item_equip(player *p, inventory **inv __attribute__((unused)), item *it)
{
    g_assert(p != NULL && it != NULL);

    /* Check if the player is able to move. */
    if (!player_movement_possible(p))
        return;

    item **islot = NULL;  /* pointer to chosen item slot */
    int atime = 0;        /* time the desired action takes */
    gboolean known = player_item_known(p, it);
    gchar *desc = item_describe(it, known, FALSE, TRUE);

    /* the idea behind the time values: one turn to take one item off,
       one turn to get the item out of the pack */

    switch (it->type)
    {
    case IT_AMULET:
        if (p->eq_amulet == NULL)
        {
            if (!player_make_move(p, 2, TRUE, "putting %s on", desc))
            {
                /* interrupted */
                g_free(desc);
                return;
            }

            p->eq_amulet = it;
            log_add_entry(nlarn->log, "You put %s on.", desc);
            p->identified_amulets[it->id] = TRUE;
        }
        break;

    case IT_AMMO:
        if (p->eq_quiver == NULL)
        {
            if (!player_make_move(p, 2, TRUE, "putting %s into the quiver", desc))
            {
                /* interrupted */
                g_free(desc);
                return;
            }

            p->eq_quiver = it;
            log_add_entry(nlarn->log, "You put %s into your quiver.", desc);
        }
        break;

    case IT_ARMOUR:
        switch (armour_class(it))
        {
        case AC_BOOTS:
            islot = &(p->eq_boots);
            atime = 3;
            break;

        case AC_CLOAK:
            islot = &(p->eq_cloak);
            atime = 2;
            break;

        case AC_GLOVES:
            islot = &(p->eq_gloves);
            atime = 3;
            break;

        case AC_HELMET:
            islot = &(p->eq_helmet);
            atime = 2;
            break;

        case AC_SHIELD:
            islot = &(p->eq_shield);
            atime = 2;
            break;

        case AC_SUIT:
            islot = &(p->eq_suit);
            atime = it->id + 1;
            break;

        case AC_MAX:
            /* shouldn't happen */
            break;
        }

        if ((islot != NULL) && (*islot == NULL))
        {
            if (!player_make_move(p, atime, TRUE, "wearing %s", desc))
            {
                /* interrupted */
                g_free(desc);
                return;
            }

            /* identify the armour while wearing */
            p->identified_armour[it->id] = TRUE;
            /* the armour's bonus is revealed when putting it on */
            it->bonus_known = TRUE;

            /* Refresh the armour's description before logging. */
            g_free(desc);
            desc = item_describe(it, known, TRUE, FALSE);
            log_add_entry(nlarn->log, "You are now wearing %s.", desc);

            /* put the piece of armour in the equipment slot */
            *islot = it;
        }
        break;

    case IT_RING:
        /* determine item slot */
        if (p->eq_ring_l == NULL)
            islot = &(p->eq_ring_l);
        else if (p->eq_ring_r == NULL)
            islot = &(p->eq_ring_r);

        if (islot != NULL)
        {
            if (!player_make_move(p, 2, TRUE, "putting %s on", desc))
            {
                /* interrupted */
                g_free(desc);
                return;
            }

            log_add_entry(nlarn->log, "You put %s on.", desc);
            *islot = it;
            p->identified_rings[it->id] = TRUE;

            if (ring_bonus_is_obs(it))
                it->bonus_known = TRUE;
        }
        break;

    case IT_WEAPON:
        if (p->eq_weapon != NULL && p->eq_sweapon == NULL)
        {
            /* make the primary weapon the secondary one */
            weapon_swap(p);
        }

        if (p->eq_weapon == NULL)
        {
            if (!player_make_move(p, 2 + weapon_is_twohanded(it),
                                  TRUE, "wielding %s", desc))
            {
                /* action aborted */
                g_free(desc);
                return;
            }

            p->eq_weapon = it;
            log_add_entry(nlarn->log, "You now wield %s.", desc);
        }
        break;

    default:
        /* nop */
        break;
    }

    /* free memory allocated at the beginning */
    g_free(desc);

    if (it->cursed)
    {
        /* generate a new description with definite article */
        desc = item_describe(it, known, TRUE, TRUE);

        /* capitalize first letter */
        desc[0] = g_ascii_toupper(desc[0]);
        log_add_entry(nlarn->log, it->type == IT_WEAPON
                ? "%s welds itself into your hand."
                : "%s feels uncomfortably cold!", desc);
        it->blessed_known = TRUE;
        g_free(desc);
    }

    player_effects_add(p, it->effects);

    if (known != player_item_known(p, it))
    {
        /* The player identified the item by using it. */
        desc = item_describe(it, player_item_known(p, it), FALSE, FALSE);
        log_add_entry(nlarn->log, "It seems that this is %s.", desc);
        g_free(desc);
    }
}

void player_item_unequip_wrapper(player *p, inventory **inv, item *it)
{
    player_item_unequip(p, inv, it, FALSE);
}

void player_item_unequip(player *p,
                         inventory **inv __attribute__((unused)),
                         item *it,
                         gboolean forced)
{
    g_assert(p != NULL && it != NULL);

    /* Check if the player is able to move. */
    if (!player_movement_possible(p))
        return;

   /* the idea behind the time values: one turn to take one item off,
      one turn to get the item out of the pack */

    /* item description */
    gchar *desc = item_describe(it, player_item_known(p, it), FALSE, TRUE);

    switch (it->type)
    {
    case IT_AMULET:
        if (p->eq_amulet == it)
        {
            if (forced || !it->cursed)
            {
                if (!forced)
                {
                    if (!player_make_move(p, 2, TRUE, "removing %s", desc))
                    {
                        /* interrupted */
                        g_free(desc);
                        return;
                    }

                    log_add_entry(nlarn->log, "You remove %s.", desc);
                }

                if (p->eq_amulet)
                {
                    player_effects_del(p, p->eq_amulet->effects);
                    p->eq_amulet = NULL;
                }
            }
            else
            {
                log_add_entry(nlarn->log, "You can not remove %s.%s", desc,
                              it->blessed_known ? "" : " It appears to be cursed.");

                it->blessed_known = TRUE;
            }
        }
        break;

    case IT_AMMO:
        if (p->eq_quiver == it)
        {
            if (!forced)
            {
                if (!player_make_move(p, 2, TRUE, "taking %s out of the quiver", desc))
                {
                    /* interrupted */
                    g_free(desc);
                    return;
                }

                log_add_entry(nlarn->log, "You take %s out your quiver.", desc);
            }

            p->eq_quiver = NULL;
        }
        break;

        /* take off armour */
    case IT_ARMOUR:
    {
        int atime = 0;
        item **aslot = NULL;  /* pointer to armour slot */

        switch (armour_class(it))
        {
        case AC_BOOTS:
            aslot = &(p->eq_boots);
            atime = 3;
            break;

        case AC_CLOAK:
            aslot = &(p->eq_cloak);
            atime = 2;
            break;

        case AC_GLOVES:
            aslot = &(p->eq_gloves);
            atime = 3;
            break;

        case AC_HELMET:
            aslot = &(p->eq_helmet);
            atime = 2;
            break;

        case AC_SHIELD:
            aslot = &(p->eq_shield);
            atime = 2;
            break;

        case AC_SUIT:
            aslot = &(p->eq_suit);
            /* the better the armour, the longer it takes to get out of it */
            atime = (p->eq_suit)->type + 1;
            break;
        default:
            break;
        }

        if ((aslot != NULL) && (*aslot == it))
        {
            if (forced || !it->cursed)
            {
                if (!forced)
                {
                    if (!player_make_move(p, atime, TRUE, "taking %s off", desc))
                    {
                        /* interrupted */
                        g_free(desc);
                        return;
                    }

                    log_add_entry(nlarn->log, "You finish taking off %s.", desc);
                }
                if (*aslot)
                {
                    player_effects_del(p, (*aslot)->effects);
                    *aslot = NULL;
                }
            }
            else
            {
                log_add_entry(nlarn->log, "You can't take of %s.%s", desc,
                              it->blessed_known ? "" : " It appears to be cursed.");

                it->blessed_known = TRUE;
            }
        }
    }
    break;

    case IT_RING:
    {
        item **rslot = NULL;  /* pointer to ring slot */

        /* determine ring slot */
        if (it == p->eq_ring_l)
            rslot = &(p->eq_ring_l);
        else if (it == p->eq_ring_r)
            rslot = &(p->eq_ring_r);

        if (rslot != NULL)
        {
            if (forced || !it->cursed)
            {
                if (!forced)
                {
                    if (!player_make_move(p, 2, TRUE, "removing %s", desc))
                    {
                        /* interrupted */
                        g_free(desc);
                        return;
                    }

                    log_add_entry(nlarn->log, "You remove %s.", desc);
                }

                if (*rslot)
                {
                    player_effects_del(p, (*rslot)->effects);
                    *rslot = NULL;
                }
            }
            else
            {
                log_add_entry(nlarn->log, "You can not remove %s.%s", desc,
                              it->blessed_known ? "" : " It appears to be cursed.");

                it->blessed_known = TRUE;
            }
        }
    }
    break;

    case IT_WEAPON:
        {
            item **wslot = NULL;

            if (p->eq_weapon && it == p->eq_weapon) {
                wslot = &(p->eq_weapon);
            } else if (p->eq_sweapon && it == p->eq_sweapon) {
                wslot = &(p->eq_sweapon);
            }

            if (wslot && (forced || !(*wslot)->cursed))
            {
                if (!forced)
                {
                    if (!player_make_move(p, 2 + weapon_is_twohanded(*wslot),
                                          TRUE, "putting %s away", desc))
                    {
                        /* interrupted */
                        g_free(desc);
                        return;
                    }

                    log_add_entry(nlarn->log, "You put away %s.", desc);
                }

                player_effects_del(p, (*wslot)->effects);
                *wslot = NULL;
            }
            else if (wslot != NULL)
            {
                log_add_entry(nlarn->log, "You can't put away %s. " \
                              "It's welded into your hands.", desc);
            }
        }
        break;

    default:
        break;
    }

    g_free(desc);
}

/* silly filter to get containers */
int player_item_is_container(player *p __attribute__((unused)), item *it)
{
    g_assert(it != NULL && it->type < IT_MAX);

    return (it->type == IT_CONTAINER);
}

/* silly filter to check if item can be put into a container */
int player_item_can_be_added_to_container(player *p, item *it)
{
    g_assert(p != NULL && it != NULL && it->type < IT_MAX);

    if (it->type == IT_CONTAINER)
    {
        return FALSE;
    }

    if (player_item_is_equipped(p, it))
    {
        return FALSE;
    }

    /* check player's inventory for containers */
    for (guint idx = 0; idx < inv_length(p->inventory); idx++)
    {
        item *i = inv_get(p->inventory, idx);
        if (i->type == IT_CONTAINER)
        {
            return TRUE;
        }
    }

    /* no match till now, check floor for containers */
    inventory **floor = map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos);

    for (guint idx = 0; idx < inv_length(*floor); idx++)
    {
        item *i = inv_get(*floor, idx);
        if (i->type == IT_CONTAINER)
        {
            return TRUE;
        }
    }

    /* nope */
    return FALSE;
}

int player_item_filter_unequippable(item* it)
{
    return player_item_is_unequippable(nlarn->p, it);
}

int player_item_is_equipped(player *p, item *it)
{
    g_assert(p != NULL && it != NULL);

    if (!item_is_equippable(it->type))
        return FALSE;

    if (it == p->eq_amulet)
        return TRUE;

    if (it == p->eq_boots)
        return TRUE;

    if (it == p->eq_cloak)
        return TRUE;

    if (it == p->eq_gloves)
        return TRUE;

    if (it == p->eq_helmet)
        return TRUE;

    if (it == p->eq_ring_l)
        return TRUE;

    if (it == p->eq_ring_r)
        return TRUE;

    if (it == p->eq_shield)
        return TRUE;

    if (it == p->eq_suit)
        return TRUE;

    if (it == p->eq_weapon)
        return TRUE;

    if (it == p->eq_quiver)
        return TRUE;

    if (it == p->eq_sweapon)
        return TRUE;

    return FALSE;
}

int player_item_is_equippable(player *p, item *it)
{
    g_assert(p != NULL && it != NULL);

    if (!item_is_equippable(it->type))
        return FALSE;

    if (player_item_is_equipped(p, it))
        return FALSE;

    switch(it->type)
    {
        /* amulets */
        case IT_AMULET:
            if (p->eq_amulet) return FALSE;
            break;

        /* ammo */
        case IT_AMMO:
            if (p->eq_quiver) return FALSE;
            break;

        /* armour */
        case IT_ARMOUR:
            switch (armour_class(it))
            {
                case AC_BOOTS:
                    if (p->eq_boots) return FALSE;
                    break;

                case AC_CLOAK:
                    if (p->eq_cloak) return FALSE;
                    break;

                case AC_GLOVES:
                    if (p->eq_gloves) return FALSE;
                    break;

                case AC_HELMET:
                    if (p->eq_helmet) return FALSE;
                    break;

                case AC_SHIELD:
                    if (p->eq_shield) return FALSE;

                    /* shield / two-handed weapon combination */
                    if (p->eq_weapon && weapon_is_twohanded(p->eq_weapon))
                        return FALSE;
                    break;

                case AC_SUIT:
                    if (p->eq_cloak) return FALSE;
                    if (p->eq_suit) return FALSE;
                    break;
                default:
                    g_assert(0);
                    break;
            }
            break;

        /* rings */
        case IT_RING:
            /* wearing gloves */
            if (p->eq_gloves) return FALSE;

            /* already wearing two rings */
            if (p->eq_ring_l && p->eq_ring_r) return FALSE;
            break;

        /* weapons */
        case IT_WEAPON:
            /* primary and secondary weapon slot used */
            if (p->eq_weapon && p->eq_sweapon) return FALSE;

            /* primary weapon is cursed */
            if (p->eq_weapon && p->eq_weapon->cursed) return FALSE;

            /* two-handed weapon / shield combinations */
            if (weapon_is_twohanded(it) && (p->eq_shield)) return FALSE;
            break;
        default:
            return FALSE;
        }

    return TRUE;
}

int player_item_is_unequippable(player *p, item *it)
{
    assert(it);

    if (!player_item_is_equipped(p, it)) return FALSE;
    if (it == p->eq_suit && p->eq_cloak) return FALSE;
    if (it->type == IT_RING && p->eq_gloves) return FALSE;

    return TRUE;
}

int player_item_is_usable(player *p __attribute__((unused)), item *it)
{
    g_assert(it != NULL);
    return item_is_usable(it->type);
}

int player_item_is_dropable(player *p, item *it)
{
    g_assert(p != NULL && it != NULL);
    return !player_item_is_equipped(p, it);
}

int player_item_is_damaged(player *p __attribute__((unused)), item *it)
{
    g_assert(it != NULL);

    if (it->corroded) return TRUE;
    if (it->burnt) return TRUE;
    if (it->rusty) return TRUE;

    return FALSE;
}

int player_item_is_affordable(player *p, item *it)
{
    g_assert(p != NULL && it != NULL);

    return ((item_price(it) <= player_get_gold(p))
            || (item_price(it) <= p->bank_account));
}

int player_item_is_sellable(player *p, item *it)
{
    g_assert(p != NULL && it != NULL);

    return (!player_item_is_equipped(p, it));
}

int player_item_is_identifiable(player *p, item *it)
{
    return (!player_item_identified(p, it));
}

/* determine if item type is known */
int player_item_known(player *p, item *it)
{
    g_assert(p != NULL && it != NULL && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_AMULET:
        return p->identified_amulets[it->id];
        break;

    case IT_ARMOUR:
        return !armour_unique(it) || p->identified_armour[it->id];
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

    g_assert(p != NULL && it != NULL);

    /* some items are always identified */
    if (!item_is_identifyable(it->type))
        return TRUE;

    known = player_item_known(p, it);

    if (!it->blessed_known)
        known = FALSE;

    if (item_is_optimizable(it->type) && !it->bonus_known)
        known = FALSE;

    return known;
}

int player_item_not_equipped(item *it)
{
    return !player_item_is_equipped(nlarn->p, it);
}

char *player_item_identified_list(player *p)
{
    GString *list;
    item *it; /* fake pretend item */

    item_t type_ids[] =
    {
        IT_AMULET,
        IT_ARMOUR,
        IT_BOOK,
        IT_POTION,
        IT_RING,
        IT_SCROLL,
        IT_NONE,
    };

    g_assert (p != NULL);

    list = g_string_new(NULL);
    it = item_new(type_ids[1], 1);
    it->bonus_known = FALSE;

    for (guint idx = 0; type_ids[idx] != IT_NONE; idx++)
    {
        guint count = 0;
        GString *sublist = g_string_new(NULL);

        /* item category header */
        char *heading = g_strdup(item_name_pl(type_ids[idx]));
        heading[0] = g_ascii_toupper(heading[0]);

        /* no linefeed before first category */
        if (idx > 0) g_string_append_c(sublist, '\n');
        g_string_append_printf(sublist, "`yellow`%s`end`\n", heading);

        g_free(heading);

        it->type = type_ids[idx];

        for (guint id = 0; id < item_max_id(type_ids[idx]); id++)
        {
            it->id = id;

            /* no non-unique armour in the list */
            if (it->type == IT_ARMOUR && !armour_unique(it))
                continue;

            if (player_item_known(p, it))
            {
                gchar *desc_unid = item_describe(it, FALSE, TRUE, FALSE);
                gchar *desc_id = item_describe(it, TRUE, TRUE, FALSE);

                g_string_append_printf(sublist, " `lightgreen`%33s`end` - %s \n", desc_unid, desc_id);

                g_free(desc_id);
                g_free(desc_unid);
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

void player_item_identify(player *p, inventory **inv __attribute__((unused)), item *it)
{
    g_assert(p != NULL && it != NULL);

    switch (it->type)
    {
    case IT_AMULET:
        p->identified_amulets[it->id] = TRUE;
        break;

    case IT_ARMOUR:
        p->identified_armour[it->id] = TRUE;
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

    it->blessed_known = TRUE;
    it->bonus_known = TRUE;
}

void player_item_use(player *p, inventory **inv __attribute__((unused)), item *it)
{
    item_usage_result result;

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* hide windows */
    display_windows_hide();

    /* Check if the player is able to move. */
    if (!player_movement_possible(p))
        return;

    switch (it->type)
    {
        /* read book */
    case IT_BOOK:
        result = book_read(p, it);
        break;

        /* drink potion */
    case IT_POTION:
        result = potion_quaff(p, it);
        break;

        /* read scroll */
    case IT_SCROLL:
        result = scroll_read(p, it);
        break;

    default:
        /* NOP */
        return;
        break;
    }

    if (result.identified)
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

    if (result.used_up)
    {
        if (it->count > 1)
        {
            it->count--;
        }
        else
        {
            inv_del_element(&p->inventory, it);
            item_destroy(it);
        }
    }

    /* show windows */
    display_windows_show();
}

void player_item_destroy(player *p, item *it)
{
    gchar *desc = item_describe(it, player_item_known(p, it), FALSE, TRUE);
    desc[0] = g_ascii_toupper(desc[0]);

    if (player_item_is_equipped(p, it))
    {
        player_item_unequip(p, &p->inventory, it, TRUE);
    }

    log_add_entry(nlarn->log, "%s %s destroyed!", desc, is_are(it->count));

    int count = 0;
    if (it->content)
    {
        count = container_move_content(p, &it->content,
                map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos));
    }

    inv_del_element(&p->inventory, it);
    item_destroy(it);

    if (count)
    {
        log_add_entry(nlarn->log, "%s's content%s spill%s onto the floor.",
                      desc,
                      (count == 1) ? "" : "s",
                      (count == 1) ? "s" : "");
    }

    g_free(desc);
}

void player_item_drop(player *p, inventory **inv, item *it)
{
    gchar *buf;
    guint count = 0;
    gboolean split = FALSE;

    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* Don't use player_movement_possible() here as this would take
       the possibility to drop stuff when overstrained. */
    if (player_effect(p, ET_PARALYSIS))
    {
        log_add_entry(nlarn->log, "You can't move!");
        return;
    }

    if (player_item_is_equipped(p, it))
        return;

    if (it->count > 1)
    {
        /* use the item type plural name except for ammunition */
        buf = g_strdup_printf("Drop how many %s%s?",
                              (it->type == IT_AMMO ? ammo_name(it) : item_name_pl(it->type)),
                              (it->type == IT_AMMO ? "s" : ""));

        count = display_get_count(buf, it->count);
        g_free(buf);

        if (!count)
            return;
    }

    if (count && count < it->count)
    {
        /* split the item if only a part of it is to be dropped;
           otherwise the entire quantity gets dropped */
        it = item_split(it, count);

        /* remember the fact */
        split = TRUE;
    }

    /* show the message before dropping the item, as dropping might remove
       the burdened / overloaded effect. If the pre_del callback fails,
       it has to display a message which explains why it has failed. */
    buf = item_describe(it, player_item_known(p, it), FALSE, FALSE);
    log_add_entry(nlarn->log, "You drop %s.", buf);

    /* if the action is aborted or if the callback failed return without dropping the item */
    if (!player_make_move(p, 2, TRUE, "dropping %s", buf)
            || !inv_del_element(inv, it))
    {
        /* if the item has been split return it */
        if (split)
        {
            inv_add(&p->inventory, it);
        }

        g_free(buf);
        return;
    }

    g_free(buf);

    if (it->type == IT_GOLD)
    {
        p->stats.gold_found -= it->count;
    }

    inv_add(map_ilist_at(game_map(nlarn, Z(p->pos)), p->pos), it);

    /* reveal if item is cursed or blessed when dropping it on an altar */
    sobject_t ms = map_sobject_at(game_map(nlarn, Z(p->pos)), p->pos);

    if (ms == LS_ALTAR
            && (!player_effect(p, ET_BLINDNESS) || game_wizardmode(nlarn))
            && item_is_blessable(it->type))
    {
        if (it->cursed || it->blessed)
        {
            buf = item_describe(it, player_item_known(p, it), FALSE, TRUE);
            buf[0] = g_ascii_toupper(buf[0]);

            log_add_entry(nlarn->log, "%s %s surrounded by a %s halo.",
                          buf, is_are(it->count),
                          it->cursed ? "black" : "white");

            g_free(buf);
        }

        it->blessed_known = TRUE;
    }

    return;
}

void player_item_notes(player *p, inventory **inv __attribute__((unused)), item *it)
{
    gchar *desc = item_describe(it, player_item_known(p, it), FALSE, TRUE);
    gchar *caption = g_strdup_printf("Add your description for %s (delete with ESC)", desc);
    g_free(desc);

    /* get the new note */
    gchar *temp = display_get_string(caption, it->notes, 60);

    /* free the old note before adding the new note to the item */
    g_free(it->notes);
    it->notes = temp;

    g_free(caption);
}

void player_read(player *p)
{
    g_assert (p != NULL);

    if (inv_length_filtered(p->inventory, item_filter_legible) > 0)
    {
        item *it = display_inventory("Choose an item to read", p,
                &p->inventory, NULL, FALSE, FALSE, FALSE, item_filter_legible);

        if (it)
        {
            player_item_use(p, NULL, it);
        }
    }
    else
    {
        log_add_entry(nlarn->log, "You have nothing to read.");
    }
}

void player_quaff(player *p)
{
    g_assert (p != NULL);

    if (inv_length_filtered(p->inventory, item_filter_potions) > 0)
    {
        item *it = display_inventory("Choose an item to drink", p,
                &p->inventory, NULL, FALSE, FALSE, FALSE, item_filter_potions);

        if (it)
        {
            player_item_use(p, NULL, it);
        }
    }
    else
    {
        log_add_entry(nlarn->log, "You have nothing to drink.");
    }
}


void player_equip(player *p)
{
    g_assert (p != NULL);

    if (inv_length_filtered(p->inventory, item_filter_equippable) > 0)
    {
        item *it = display_inventory("Choose an item to equip", p,
                &p->inventory, NULL, FALSE, FALSE, FALSE, item_filter_equippable);

        if (it)
        {
            player_item_equip(p, NULL, it);
        }
    }
    else
    {
        log_add_entry(nlarn->log, "You have nothing you could equip.");
    }
}

void player_take_off(player *p)
{
    g_assert (p != NULL);

    if (inv_length_filtered(p->inventory, player_item_filter_unequippable) > 0)
    {
        item *it = display_inventory("Choose an item to take off", p,
                &p->inventory, NULL, FALSE, FALSE, FALSE,
                player_item_filter_unequippable);

        if (it)
            player_item_unequip(p, NULL, it, FALSE);
    }
    else
    {
        log_add_entry(nlarn->log, "You have nothing you could take off.");
    }
}

void player_drop(player *p)
{
    g_assert (p != NULL);

    if (inv_length_filtered(p->inventory, item_filter_dropable) > 0)
    {
        item *it = display_inventory("Choose an item to drop", p, &p->inventory,
                               NULL, FALSE, FALSE, FALSE, item_filter_dropable);

        if (it)
            player_item_drop(p, &p->inventory, it);
    }
    else
    {
        log_add_entry(nlarn->log, "You have nothing you could drop.");
    }
}

guint player_get_ac(player *p)
{
    int ac = 0;
    g_assert(p != NULL);

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

int player_get_hp_max(player *p)
{
    g_assert(p != NULL);
    return p->hp_max;
}

int player_get_mp_max(player *p)
{
    g_assert(p != NULL);
    return p->mp_max;
}

int player_get_str(player *p)
{
    g_assert(p != NULL);
    return p->strength
           + player_effect(p, ET_INC_STR)
           - player_effect(p, ET_DEC_STR)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_int(player *p)
{
    g_assert(p != NULL);
    return p->intelligence
           + player_effect(p, ET_INC_INT)
           - player_effect(p, ET_DEC_INT)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_wis(player *p)
{
    g_assert(p != NULL);
    return p->wisdom
           + player_effect(p, ET_INC_WIS)
           - player_effect(p, ET_DEC_WIS)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_con(player *p)
{
    g_assert(p != NULL);
    return p->constitution
           + player_effect(p, ET_INC_CON)
           - player_effect(p, ET_DEC_CON)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_dex(player *p)
{
    g_assert(p != NULL);
    return p->dexterity
           + player_effect(p, ET_INC_DEX)
           - player_effect(p, ET_DEC_DEX)
           + player_effect(p, ET_HEROISM)
           - player_effect(p, ET_DIZZINESS);
}

int player_get_speed(player *p)
{
    g_assert(p != NULL);
    return p->speed
           + player_effect(p, ET_SPEED)
           - player_effect(p, ET_SLOWNESS)
           - player_effect(p, ET_BURDENED);
}

guint player_get_gold(player *p)
{
    guint gold = 0;

    g_assert(p != NULL);

    /* gold stacks, thus there can only be one item in the inventory */
    if (inv_length_filtered(p->inventory, item_filter_gold))
    {
        item *i = inv_get_filtered(p->inventory, 0, item_filter_gold);
        gold += i->count;
    }

    /* check content of all containers in player's inventory */
    for (guint idx = 0;
         idx < inv_length_filtered(p->inventory, item_filter_container);
         idx++)
    {
        item *i = inv_get_filtered(p->inventory, idx, item_filter_container);

        if (inv_length_filtered(i->content, item_filter_gold))
        {
            item *it = inv_get_filtered(i->content, 0, item_filter_gold);
            gold += it->count;
        }
    }

    return gold;
}

void player_remove_gold(player *p, guint amount)
{
    g_assert(p != NULL);

    /* gold stacks, thus there can only be one item in the inventory */
    if (inv_length_filtered(p->inventory, item_filter_gold))
    {
        item *i = inv_get_filtered(p->inventory, 0, item_filter_gold);

        if (amount >= i->count)
        {
            amount -= i->count;
            inv_del_element(&p->inventory, i);
        }
        else
        {
            i->count -= amount;
            goto done;
        }
    }

    for (guint idx = 0;
         idx < inv_length_filtered(p->inventory, item_filter_container);
         idx++)
    {
        item *i = inv_get_filtered(p->inventory, idx, item_filter_container);

        if (inv_length_filtered(i->content, item_filter_gold))
        {
            i = inv_get_filtered(i->content, 0, item_filter_gold);

            if (amount >= i->count)
            {
                amount -= i->count;
                inv_del_element(&p->inventory, i);
            }
            else
            {
                i->count -= amount;
                goto done;
            }
        }
    }

    /* yes, I _do_ _know_ that goto is evil.. */
done:
    /* force recalculation of inventory weight */
    player_inv_weight_recalc(p->inventory, NULL);
}

const char *player_get_level_desc(player *p)
{
    g_assert(p != NULL);
    return player_level_desc[p->level - 1];
}

void player_search(player *p)
{
    g_assert (p != NULL);

    map *m = game_map(nlarn, Z(p->pos));

    /* look for traps on adjacent tiles */
    for (direction dir = GD_NONE + 1; dir < GD_MAX; dir++)
    {
        position pos = pos_move(p->pos, dir);

        /* skip invalid positions */
        if (!pos_valid(pos)) continue;

        /* skip unpassable positions unless the player is blind */
        if (!map_pos_passable(m, pos) && !player_effect(p, ET_BLINDNESS))
            continue;

        /* consume one turn per tile */
        player_make_move(p, 1, FALSE, NULL);

        /* stop searching when the player is under attack */
        if (p->attacked || player_adjacent_monster(p, TRUE)) return;

        /* examine the surrounding area if blind */
        if (player_effect(p, ET_BLINDNESS))
        {
            /* examine tile types */
            player_memory_of(p, pos).type = map_tile_at(m, pos)->type;

            /* examine stationary objects */
            player_memory_of(p, pos).sobject = map_tile_at(m, pos)->sobject;
        }

        /* search for traps */
        trap_t tt = map_trap_at(m, pos);

        if ((tt != TT_NONE) && (tt != player_memory_of(p, pos).trap))
        {
            /* found an unknown trap - determine the chance that
             * the player can find it */
            int prop = (trap_chance(tt) / 2) + player_get_int(p);

            if (chance(prop))
            {
                /* discovered the trap */
                log_add_entry(nlarn->log, "You find a %s!",
                        trap_description(tt));
                 player_memory_of(p, pos).trap = tt;
            }
        }

        /* search for traps on containers */
        inventory **ti = map_ilist_at(m, pos);
        if (ti != NULL)
        {
            for (guint idx = 0;
                    idx < inv_length_filtered(*ti, item_filter_container);
                    idx++)
            {
                item *c = inv_get_filtered(*ti, idx, item_filter_container);

                /* the chance that the player discovers the trap is 3 * int */
                if (c->cursed && !c->blessed_known && chance(player_get_int(p) * 3))
                {
                    gchar *idesc = item_describe(c, FALSE, TRUE, TRUE);
                    /* the container is cursed */
                    c->blessed_known = TRUE;
                    log_add_entry(nlarn->log, "You discover a trap on %s!",
                            idesc);

                    g_free(idesc);
                }
            }
        }
    }
}

void player_list_sobjmem(player *p)
{
    GString *sobjlist = g_string_new(NULL);

    if (p->sobjmem == NULL)
    {
        g_string_append(sobjlist, "You have not discovered any landmarks yet.");

    }
    else
    {
        int prevmap = -1;

        /* sort the array of memorized landmarks */
        g_array_sort(p->sobjmem, player_sobjects_sort);

        /* assemble a list of landmarks per map */
        for (guint idx = 0; idx < p->sobjmem->len; idx++)
        {
            player_sobject_memory *som;
            som = &g_array_index(p->sobjmem, player_sobject_memory, idx);

            g_string_append_printf(sobjlist, "%-4s %s (%d, %d)\n",
                                   (Z(som->pos) > prevmap) ? map_names[Z(som->pos)] : "",
                                   so_get_desc(som->sobject),
                                   Y(som->pos), X(som->pos));

            if (Z(som->pos) > prevmap) prevmap = Z(som->pos);
        }
    }

    display_show_message("Discovered landmarks", sobjlist->str, 5);
    g_string_free(sobjlist, TRUE);
}

void player_update_fov(player *p)
{
    int radius;
    position pos = p->pos;
    map *pmap;

    int range = (Z(p->pos) == 0 ? 15 : 6);

    /* calculate range */
    if (player_effect(nlarn->p, ET_BLINDNESS))
        radius = 0;
    else
    {
        radius = range + player_effect(nlarn->p, ET_AWARENESS);
        if (player_effect(nlarn->p, ET_TRAPPED))
            radius -= 4;
    }

    /* get current map */
    pmap = game_map(nlarn, Z(p->pos));

    /* determine if the player has infravision */
    gboolean infravision = player_effect(p, ET_INFRAVISION);

    /* if player is enlightened, use a circular area around the player */
    if (player_effect(p, ET_ENLIGHTENMENT))
    {
        /* reset FOV manually */
        fov_reset(p->fv);

        area *enlight = area_new_circle(p->pos,
                player_effect(p, ET_ENLIGHTENMENT), FALSE);

        /* set visible field according to returned area */
        for (int y = 0; y < enlight->size_y; y++)
        {
            for (int x = 0; x < enlight->size_x; x++)
            {
                X(pos) = x + enlight->start_x;
                Y(pos) = y + enlight->start_y;

                if (pos_valid(pos) && area_point_get(enlight, x, y))
                {
                    /* The position if enlightened.
                       Now determine if the position has a direct visible connection
                       to the players position. This is required to instruct fov_set()
                       if a monster at the position shall be added to the list of the
                       visible monsters. */
                    gboolean mchk = ((pos_distance(p->pos, pos) <= 7)
                                    && map_pos_is_visible(pmap, p->pos, pos));

                    fov_set(p->fv, pos, TRUE, infravision, mchk);
                }
            }
        }

        area_destroy(enlight);
    }
    else
    {
        /* otherwise use the fov algorithm */
        fov_calculate(p->fv, pmap, p->pos, radius, infravision);
    }

    /* update visible fields in player's memory */
    for (Y(pos) = 0; Y(pos) < MAP_MAX_Y; Y(pos)++)
    {
        for (X(pos) = 0; X(pos) < MAP_MAX_X; X(pos)++)
        {
            if (fov_get(p->fv, pos))
            {
                monster *m = map_get_monster_at(pmap, pos);
                inventory **inv = map_ilist_at(pmap, pos);

                player_memory_of(p,pos).type = map_tiletype_at(pmap, pos);
                player_memory_of(p,pos).sobject = map_sobject_at(pmap, pos);

                /* remember certain stationary objects */
                switch (map_sobject_at(pmap, pos))
                {
                case LS_ALTAR:
                case LS_BANK2:
                case LS_FOUNTAIN:
                case LS_MIRROR:
                case LS_THRONE:
                case LS_THRONE2:
                case LS_STATUE:
                    player_sobject_memorize(p, map_sobject_at(pmap, pos), pos);
                    break;

                default:
                    player_sobject_forget(p, pos);
                    break;
                }

                if (m && monster_flags(m, MF_MIMIC) && monster_unknown(m))
                {
                    /* remember the undiscovered mimic as an item */
                    item *it = get_mimic_item(m);
                    if (it != NULL)
                    {
                        player_memory_of(p,pos).item = it->type;
                        player_memory_of(p,pos).item_colour = item_colour(it);
                    }
                }
                else if (inv_length(*inv) > 0)
                {
                    item *it;

                    /* memorize the most interesting item on the tile */
                    if (inv_length_filtered(*inv, item_filter_gems) > 0)
                    {
                        /* there's a gem in the stack */
                        it = inv_get_filtered(*inv, 0, item_filter_gems);
                    }
                    else if (inv_length_filtered(*inv, item_filter_gold) > 0)
                    {
                        /* there is gold in the stack */
                        it = inv_get_filtered(*inv, 0, item_filter_gold);
                    }
                    else
                    {
                        /* memorize the topmost item on the stack */
                        it = inv_get(*inv, inv_length(*inv) - 1);
                    }

                    player_memory_of(p,pos).item = it->type;
                    player_memory_of(p,pos).item_colour = item_colour(it);
                }
                else
                {
                    /* no item at that position */
                    player_memory_of(p,pos).item = IT_NONE;
                    player_memory_of(p,pos).item_colour = 0;
                }
            }
        }
    }
}

static guint player_item_pickup(player *p, inventory **inv, item *it, gboolean ask)
{
    g_assert(p != NULL && it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    gchar *buf;
    gpointer oid = it->oid;
    guint gold_amount = 0;

    if (ask && (it->count > 1))
    {
        /* use the item type plural name except for ammunition */
        buf = g_strdup_printf("Pick up how many %s%s?",
                              (it->type == IT_AMMO ? ammo_name(it) : item_name_pl(it->type)),
                              (it->type == IT_AMMO ? "s" : ""));

        guint count = display_get_count(buf, it->count);
        g_free(buf);

        if (count == 0)
            return 0;

        if (count < it->count)
        {
            it = item_split(it, count);
            /* set oid to NULL to prevent that the original is remove
               from the originating inventory */
            oid = NULL;
        }
    }

    /* Log the attempt to pick up the item */
    buf = item_describe(it, player_item_known(p, it), FALSE, FALSE);
    log_add_entry(nlarn->log, "You pick up %s.", buf);

    if (it->type == IT_GOLD)
    {
        /* record the amount of gold as the item might be destroyed */
        gold_amount = it->count;
    }

    /* Reset the fired flag. This has to be done before adding the item to the
       inventory as otherwise the item comparison would fail.
       If picking up fails, the item will not be picked up automatically again. */
    it->fired = FALSE;

    /* one turn to pick item up, one to stuff it into the pack */
    if (!player_make_move(p, 2, TRUE, "picking up %s", buf))
    {
        /* Adding the item to the player's inventory has failed.
           If the item has been split, return it to the originating inventory */
        if (oid == NULL) inv_add(inv, it);
        g_free(buf);
        return 1;
    }

    g_free(buf);
    if (!inv_add(&p->inventory, it))
    {
        /* Adding the item to the player's inventory has failed.
           If the item has been split, return it to the originating inventory */
        if (oid == NULL) inv_add(inv, it);

        return 2;
    }

    if (gold_amount > 0)
    {
        /* if the player has tried to pick up gold and succeeded in doing so,
           add the amount picked up to the statistics */
        p->stats.gold_found += gold_amount;
    }

    /* remove the item from the originating inventory if it has not been split */
    if (oid != NULL)
        inv_del_oid(inv, oid);

    return 0;
}

static void player_sobject_memorize(player *p, sobject_t sobject, position pos)
{
    player_sobject_memory nsom;

    if (p->sobjmem == NULL)
    {
        p->sobjmem = g_array_new(FALSE, FALSE, sizeof(player_sobject_memory));
    }

    /* check if the sobject has already been memorized */
    for (guint idx = 0; idx < p->sobjmem->len; idx++)
    {
        player_sobject_memory *som;
        som = &g_array_index(p->sobjmem, player_sobject_memory, idx);

        /* memory for this position exists */
        if (pos_identical(som->pos, pos))
        {
            /* update remembered sobject */
            som->sobject = sobject;

            /* the work is done */
            return;
        }
    }

    /* add a new memory entry */
    nsom.pos = pos;
    nsom.sobject = sobject;

    g_array_append_val(p->sobjmem, nsom);
}

void player_sobject_forget(player *p, position pos)
{
    /* just return if nothing has been memorized yet */
    if (p->sobjmem == NULL)
    {
        return;
    }

    for (guint idx = 0; idx < p->sobjmem->len; idx++)
    {
        player_sobject_memory *som = &g_array_index(p->sobjmem,
                player_sobject_memory, idx);

        /* remove existing entries for this position */
        if (pos_identical(som->pos, pos))
        {
            g_array_remove_index(p->sobjmem, idx);
            break;
        }
    }

    /* free the sobject memory if no entry remains */
    if (p->sobjmem->len == 0)
    {
        g_array_free(p->sobjmem, TRUE);
        p->sobjmem = NULL;
    }
}

gboolean player_adjacent_monster(player *p, gboolean ignore_harmless)
{
    gboolean monster_visible = FALSE;

    /* get the list of all visible monsters */
    GList *mlist, *miter;
    miter = mlist = fov_get_visible_monsters(p->fv);

    /* no visible monsters? */
    if (mlist == NULL)
        return monster_visible;

    /* got a list of monsters, check if any are dangerous */
    do
    {
        monster *m = (monster *)miter->data;

        // Ignore the town inhabitants.
        if (monster_type(m) == MT_TOWN_PERSON)
            continue;

        /* ignore servants */
        if (monster_action(m) == MA_SERVE)
            continue;

        /* Only ignore floating eye if already paralysed. */
        if (ignore_harmless
            && (monster_type(m) == MT_FLOATING_EYE)
            && player_effect_get(p, ET_PARALYSIS))
            continue;

        /* Ignore adjacent umber hulk if already confused. */
        if (ignore_harmless
            && (monster_type(m) == MT_UMBER_HULK)
            && player_effect_get(p, ET_CONFUSION))
            continue;

        /* when reaching this point, the monster is a threat */
        monster_visible = TRUE;
        break;
    }
    while ((miter = miter->next) != NULL);

    /* free the list returned by fov_get_visible_monsters() */
    g_list_free(mlist);

    return monster_visible;
}

static int player_sobjects_sort(gconstpointer a, gconstpointer b)
{
    player_sobject_memory *som_a = (player_sobject_memory *)a;
    player_sobject_memory *som_b = (player_sobject_memory *)b;

    if (Z(som_a->pos) == Z(som_b->pos))
    {
        if (som_a->sobject == som_b->sobject)
            return 0;
        if (som_a->sobject < som_b->sobject)
            return -1;
        else
            return 0;
    }
    else if (Z(som_a->pos) < Z(som_b->pos))
        return -1;
    else
        return 1;
}

static cJSON *player_memory_serialize(player *p, position pos)
{
    cJSON *mser;

    mser = cJSON_CreateObject();
    if (player_memory_of(p, pos).type > LT_NONE)
        cJSON_AddNumberToObject(mser, "type",
                                player_memory_of(p, pos).type);

    if (player_memory_of(p, pos).sobject > LS_NONE)
        cJSON_AddNumberToObject(mser, "sobject",
                                player_memory_of(p, pos).sobject);

    if (player_memory_of(p, pos).item > IT_NONE)
        cJSON_AddNumberToObject(mser, "item",
                                player_memory_of(p, pos).item);

    if (player_memory_of(p, pos).item_colour > 0)
        cJSON_AddNumberToObject(mser, "item_colour",
                                player_memory_of(p, pos).item_colour);

    if (player_memory_of(p, pos).trap > TT_NONE)
        cJSON_AddNumberToObject(mser, "trap",
                                player_memory_of(p, pos).trap);

    return mser;
}

static void player_memory_deserialize(player *p, position pos, cJSON *mser)
{
    cJSON *obj;

    obj = cJSON_GetObjectItem(mser, "type");
    if (obj != NULL)
        player_memory_of(p, pos).type = obj->valueint;

    obj = cJSON_GetObjectItem(mser, "sobject");
    if (obj != NULL)
        player_memory_of(p, pos).sobject = obj->valueint;

    obj = cJSON_GetObjectItem(mser, "item");
    if (obj != NULL)
        player_memory_of(p, pos).item = obj->valueint;

    obj = cJSON_GetObjectItem(mser, "item_colour");
    if (obj != NULL)
        player_memory_of(p, pos).item_colour = obj->valueint;

    obj = cJSON_GetObjectItem(mser, "trap");
    if (obj != NULL)
        player_memory_of(p, pos).trap = obj->valueint;
}

static char *player_death_description(game_score_t *score, int verbose)
{
    const char *desc;
    GString *text;

    g_assert(score != NULL);

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

    case PD_GENOCIDE:
        desc = "genocided";
        break;

    case PD_SPELL:
        if (score->cause < SP_MAX)
            desc = "blasted";
        else
            desc = "got killed";
        break;

    default:
        desc = "killed";
    }

    text = g_string_new_len(NULL, 200);

    g_string_append_printf(text, "%s (%c), %s", score->player_name,
                           (score->sex == PS_MALE) ? 'm' : 'f', desc);


    if (score->cod == PD_GENOCIDE)
    {
        g_string_append_printf(text, " %sself",
                               (score->sex == PS_MALE) ? "him" : "her");
    }

    if (verbose)
    {
        g_string_append_printf(text, " on level %s", map_names[score->dlevel]);

        if (score->dlevel_max > score->dlevel)
        {
            g_string_append_printf(text, " (max. %s)", map_names[score->dlevel_max]);
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
        switch (score->cause)
        {
        case ET_DEC_STR:
            g_string_append(text, " by enfeeblement.");
            break;

        case ET_DEC_DEX:
            g_string_append(text, " by clumsiness.");
            break;

        case ET_POISON:
            g_string_append(text, " by poison.");
            break;
        }
        break;

    case PD_LASTLEVEL:
        g_string_append_printf(text,". %s left %s body.",
                               (score->sex == PS_MALE) ? "He" : "She",
                               (score->sex == PS_MALE) ? "his" : "her");
        break;

    case PD_MONSTER:
        /* TODO: regard monster's invisibility */
        /* TODO: while sleeping / doing sth. */
        g_string_append_printf(text, " by %s %s.",
                               a_an(monster_type_name(score->cause)),
                               monster_type_name(score->cause));
        break;

    case PD_SPHERE:
        g_string_append(text, " by a sphere of destruction.");
        break;

    case PD_TRAP:
        g_string_append_printf(text, " by %s%s %s.",
                               score->cause == TT_TRAPDOOR ? "falling through " : "",
                               a_an(trap_description(score->cause)),
                               trap_description(score->cause));
        break;

    case PD_MAP:
        g_string_append_printf(text, " by %s.", mt_get_desc(score->cause));
        break;

    case PD_SPELL:
        /* player spell */
        g_string_append_printf(text, " %s away with the spell \"%s\".",
                               (score->sex == PS_MALE) ? "himself" : "herself",
                               spell_name_by_id(score->cause));
        break;

    case PD_CURSE:
        g_string_append_printf(text, " by a cursed %s.",
                               item_name_sg(score->cause));
        break;

    case PD_SOBJECT:
        switch (score->cause)
        {
        case LS_FOUNTAIN:
            g_string_append(text, " by toxic water from a fountain.");
            break;
        default:
            g_string_append(text, " by falling down a staircase.");
            break;
        }
        break;

    default:
        /* no further description */
        g_string_append_c(text, '.');
        break;
    }

    if (verbose)
    {
        g_string_append_printf(text, " %s has scored %" G_GINT64_FORMAT
                               " points, with the difficulty set to %d.",
                               (score->sex == PS_MALE) ? "He" : "She",
                               score->score, score->difficulty);
    }

    return g_string_free(text, FALSE);
}

void calc_fighting_stats(player *p)
{
    g_assert(p != NULL);

    gchar *desc = NULL;
    GString *text;

    position pos = map_find_space_in(game_map(nlarn, Z(p->pos)),
                                     rect_new_sized(p->pos, 2), LE_MONSTER, FALSE);

    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "Couldn't create a monster.");
        return;
    }

    text = g_string_new("");

    if (p->eq_weapon)
    {
        p->eq_weapon->blessed_known = TRUE;
        p->eq_weapon->bonus_known   = TRUE;

        desc = item_describe(p->eq_weapon, TRUE, TRUE, FALSE);
    }

    const int damage_modifier = player_effect(p, ET_INC_DAMAGE)
                                - player_effect(p, ET_SICKNESS);

    g_string_append_printf(text, "\nPlayer stats\n"
                           "------------\n"
                           "  wielded weapon : %s\n"
                           "  weapon class   : %d\n"
                           "  accuracy       : %d\n"
                           "  experience     : %d\n"
                           "  strength       : %d\n"
                           "  dexterity      : %d\n"
                           "  damage modifier: %d\n"
                           "  speed          : %d\n"
                           "  difficulty     : %d\n\n",
                           (p->eq_weapon ? desc : "none"),
                           (p->eq_weapon ? weapon_damage(p->eq_weapon) : 0),
                           (p->eq_weapon ? weapon_acc(p->eq_weapon) : 0),
                           p->level,
                           player_get_str(p),
                           player_get_dex(p),
                           damage_modifier,
                           player_get_speed(p),
                           game_difficulty(nlarn));

    g_string_append_printf(text, "Monsters\n"
                           "--------\n\n");

    gboolean mention_instakill = FALSE;

    for (guint32 idx = MT_NONE + 1; idx < MT_MAX_GENERATED; idx++)
    {
        monster *m;
        if (!(m = monster_new(idx, pos)))
        {
            g_string_append_printf(text, "Monster %s could not be created.\n\n",
                                   monster_name(m));
            continue;
        }
        int to_hit = weapon_calc_to_hit(p, m, p->eq_weapon, NULL);
        to_hit += ((100 - to_hit) * 5)/100;

        const int instakill_chance = player_instakill_chance(p, m);

        g_string_append_printf(
            text,
            "%s (ac: %d, max hp: %d, speed: %d)\n"
            "     to-hit chance: %d%%\n"
            "  instakill chance: %d%%\n",
            monster_name(m),
            monster_ac(m),
            monster_type_hp_max(monster_type(m)),
            monster_speed(m),
            to_hit, instakill_chance
        );

        if (instakill_chance < 100)
        {
            if (monster_flags(m, MF_REGENERATE))
            {
                g_string_append_printf(text, "      regeneration: every %d turns\n",
                                       10 - game_difficulty(nlarn));
            }

            const min_max_damage mmd = calc_min_max_damage(p, m);
            double avg_dam = 0;
            int tries = 100;
            while (tries-- > 0)
                avg_dam += calc_real_damage(p, m, FALSE);

            avg_dam /= 100;

            int hits_needed = (monster_type_hp_max(monster_type(m)) / avg_dam);
            if (((int) (avg_dam * 10)) % 10)
                hits_needed++;

            g_string_append_printf(
                text,
                "       min. damage: %d hp\n"
                "       max. damage: %d hp\n"
                "       avg. damage: %.2f hp\n"
                "  avg. hits needed: %d%s\n\n",
                mmd.min_damage, mmd.max_damage,
                avg_dam, hits_needed,
                hits_needed > 1 && instakill_chance > 0 ? " [*]" : ""
            );

            if (!mention_instakill && instakill_chance > 0)
                mention_instakill = TRUE;
        }
        else
            g_string_append_printf(text, "\n");

        monster_destroy(m);
    }

    if (mention_instakill)
        g_string_append_printf(text, "*) ignoring instakills\n");

    display_show_message("Fighting statistics", text->str, 0);

    if (display_get_yesno("Do you want to save the calculations?",NULL, NULL, NULL))
    {
        char *filename, *proposal;
        GError *error = NULL;

        proposal = g_strconcat(p->name, ".stat", NULL);
        filename = display_get_string("Enter filename: ", proposal, 40);

        if (filename != NULL)
        {
            /* file name has been provided. Try to save file */
            if (!g_file_set_contents(filename, text->str, -1, &error))
            {
                display_show_message("Error", error->message, 0);
                g_error_free(error);
            }

            g_free(proposal);
        }

        g_free(filename);
    }

    g_free(desc);
    g_string_free(text, TRUE);
}

static char *player_equipment_list(player *p, gboolean decorate)
{
    int idx = 0;
    GString *el = g_string_new(NULL);

    struct
    {
        item *slot;
        const char *desc;
    } slots[] =
    {
        { p->eq_weapon,  "Main weapon:" },
        { p->eq_sweapon, "Sec. weapon:" },
        { p->eq_quiver,  "In quiver:" },
        { p->eq_boots,   "Boots:" },
        { p->eq_cloak,   "Cloak:" },
        { p->eq_gloves,  "Gloves:" },
        { p->eq_helmet,  "Helmet:" },
        { p->eq_shield,  "Shield:" },
        { p->eq_suit,    "Body armour:" },
        { p->eq_ring_l,  "Left ring:" },
        { p->eq_ring_r,  "Right ring:" },
        { NULL, NULL },
    };

    while (slots[idx].desc != NULL)
    {
        /* skip empty item slots */
        if (slots[idx].slot == NULL)
        {
            idx++;
            continue;
        }

        char *desc = item_describe(slots[idx].slot, player_item_known(p,
                    slots[idx].slot), FALSE, FALSE);

        g_string_append_printf(el, "%s%-12s%s %s\n",
                decorate ? "`white`" : "", slots[idx].desc,
                decorate ? "`end`" : "", desc);

        g_free(desc);
        idx++;
    }

    return g_string_free(el, FALSE);
}

static char *player_create_obituary(player *p, game_score_t *score, GList *scores)
{
    const char *pronoun = (p->sex == PS_MALE) ? "He" : "She";
    GList *iterator;

    /* the obituary */
    GString *text;

    /* determine position of score in the score list */
    guint rank = g_list_index(scores, score);

    gchar *tmp = player_death_description(score, TRUE);
    text = g_string_new(tmp);
    g_free(tmp);

    /* assemble surrounding scores list */
    g_string_append(text, "\n\n");

    /* get entry three entries up of current/top score in list */
    iterator = g_list_nth(scores, max(rank - 3, 0));

    /* display up to 7 entries */
    for (int nrec = max(rank - 3, 0);
         iterator && (nrec < (max(rank, 0) + 4));
         iterator = iterator->next, nrec++)
    {
        gchar *desc;

        game_score_t *cscore = (game_score_t *)iterator->data;

        desc = player_death_description(cscore, FALSE);
        g_string_append_printf(text, "  %c%2d) %7" G_GINT64_FORMAT " %s\n",
                               (cscore == score) ? '*' : ' ',
                               nrec + 1, cscore->score, desc);

        g_string_append_printf(text, "               [exp. level %d, dungeon lvl. %s, %d/%d hp, difficulty %d]\n",
                               cscore->level, map_names[cscore->dlevel],
                               cscore->hp, cscore->hp_max, cscore->difficulty);
        g_free(desc);
    }

    /* some statistics */
    g_string_append_printf(text, "\n%s %s after searching for the potion for %d mobul%s. ",
                           pronoun, score->cod < PD_TOO_LATE ? "died" : "returned",
                           gtime2mobuls(nlarn->gtime), plural(gtime2mobuls(nlarn->gtime)));

    g_string_append_printf(text, "%s cast %s spell%s, ", pronoun,
                           int2str(p->stats.spells_cast),
                           plural(p->stats.spells_cast));
    g_string_append_printf(text, "quaffed %s potion%s, ",
                           int2str(p->stats.potions_quaffed),
                           plural(p->stats.potions_quaffed));
    g_string_append_printf(text, "and read %s book%s ",
                           int2str(p->stats.books_read),
                           plural(p->stats.books_read));
    g_string_append_printf(text, "and %s scroll%s. ",
                           int2str(p->stats.scrolls_read),
                           plural(p->stats.scrolls_read));

    if (p->stats.weapons_wasted > 0)
    {
        g_string_append_printf(text, "\n%s wasted %s weapon%s in combat. ",
                               pronoun, int2str(p->stats.weapons_wasted),
                               plural(p->stats.weapons_wasted));
    }

    if (p->stats.vandalism > 0)
    {
        g_string_append_printf(text, "\n%s committed %s act%s of vandalism. ",
                               pronoun, int2str(p->stats.vandalism),
                               plural(p->stats.vandalism));
    }

    if (p->stats.life_protected > 0)
    {
        g_string_append_printf(text, "\n%s life was protected %s. ",
                               (p->sex == PS_MALE) ? "His" : "Her",
                               int2time_str(p->stats.life_protected));
    }

    g_string_append_printf(text, "\n%s had %s gp on %s bank account "
                           "when %s %s.",
                           pronoun, int2str(p->bank_account),
                           (p->sex == PS_MALE) ? "his" : "her",
                           (p->sex == PS_MALE) ? "he"  : "she",
                           score->cod < PD_TOO_LATE ? "died"
                           : "returned home");

    g_string_append_printf(text, "\n%s found %d gold in the dungeon, "
                           "sold %s gem%s for %d and %s non-gem "
                           "item%s for %d gold, and earned %d gold "
                           "as bank interest.",
                           pronoun, p->stats.gold_found,
                           int2str(p->stats.gems_sold),
                           plural(p->stats.gems_sold),
                           p->stats.gold_sold_gems,
                           int2str(p->stats.items_sold),
                           plural(p->stats.items_sold),
                           p->stats.gold_sold_items,
                           p->stats.gold_bank_interest);

    g_string_append_printf(text, "\n%s bought %s item%s for %d gold, spent "
                           "%d on item identification or repair, "
                           "donated %d gold to charitable causes, and "
                           "invested %d gold in personal education.",
                           pronoun,
                           int2str(p->stats.items_bought),
                           plural(p->stats.items_bought),
                           p->stats.gold_spent_shop,
                           p->stats.gold_spent_id_repair,
                           p->stats.gold_spent_donation,
                           p->stats.gold_spent_college);

    if (p->outstanding_taxes)
        g_string_append_printf(text, " %s owed the tax office %d gold%s",
                               pronoun, p->outstanding_taxes,
                               p->stats.gold_spent_taxes ? "" : ".");

    if (p->stats.gold_spent_taxes)
        g_string_append_printf(text, " %s paid %d gold taxes.",
                               p->outstanding_taxes ? "and" : pronoun,
                               p->stats.gold_spent_taxes);

    /* append map of current level if the player is not in the town */
    if (Z(p->pos) > 0)
    {
        g_string_append(text, "\n\n-- The current level ------------------\n\n");
        tmp = map_dump(game_map(nlarn, Z(p->pos)), p->pos);
        g_string_append(text, tmp);
        g_free(tmp);
    }

    /* player's attributes */
    g_string_append(text, "\n\n-- Attributes -------------------------\n\n");
    g_string_append_printf(text, "Strength:     %d (%+2d)\n",
                           p->strength, p->strength - p->stats.str_orig);
    g_string_append_printf(text, "Intelligence: %d (%+2d)\n",
                           p->intelligence, p->intelligence - p->stats.int_orig);
    g_string_append_printf(text, "Wisdom:       %d (%+2d)\n",
                           p->wisdom, p->wisdom - p->stats.wis_orig);
    g_string_append_printf(text, "Constitution: %d (%+2d)\n",
                           p->constitution, p->constitution - p->stats.con_orig);
    g_string_append_printf(text, "Dexterity:    %d (%+2d)\n",
                           p->dexterity, p->dexterity - p->stats.dex_orig);

    /* effects */
    char **effect_desc = player_effect_text(p);

    if (*effect_desc)
    {
        g_string_append(text, "\n\n-- Effects ----------------------------\n\n");

        for (guint pos = 0; effect_desc[pos]; pos++)
        {
            g_string_append_printf(text, "%s\n", effect_desc[pos]);
        }
    }

    g_strfreev(effect_desc);

    /* append list of known spells */
    if (p->known_spells->len > 0)
    {
        g_string_append(text, "\n\n-- Known Spells -----------------------\n\n");

        for (guint pos = 0; pos < p->known_spells->len; pos++)
        {
            spell *s = (spell *)g_ptr_array_index(p->known_spells, pos);
            tmp = str_capitalize(g_strdup(spell_name(s)));

            g_string_append_printf(text, "%-24s (lvl. %2d): %3d\n",
                                   tmp, s->knowledge, s->used);

            g_free(tmp);
        }
    }

    /* identify entire inventory */
    for (guint pos = 0; pos < inv_length(p->inventory); pos++)
    {
        item *it = inv_get(p->inventory, pos);
        it->blessed_known = TRUE;
        it->bonus_known = TRUE;
    }

    /* equipped items */
    gchar *el = player_equipment_list(p, FALSE);
    guint equipment_count = 0;

    if (strlen(el) > 0)
    {
        g_string_append(text, "\n\n-- Equipment --------------------------\n\n");
        g_string_append(text, el);

        for (guint idx = 0; idx < inv_length(p->inventory); idx++)
        {
            item *it = inv_get(p->inventory, idx);
            if (player_item_is_equipped(p, it))
                equipment_count++;
        }
    }
    g_free(el);

    /* inventory */
    if (equipment_count < inv_length(p->inventory))
    {
        g_string_append(text, "\n\n-- Items in pack ----------------------\n\n");
        for (guint pos = 0; pos < inv_length(p->inventory); pos++)
        {
            item *it = inv_get(p->inventory, pos);
            if (!player_item_is_equipped(p, it))
            {
                gchar *it_desc = item_describe(it, TRUE, FALSE, FALSE);
                g_string_append_printf(text, "%s\n", it_desc);
                g_free(it_desc);
            }
        }
    }

    /* list monsters killed */
    guint body_count = 0;
    g_string_append(text, "\n\n-- Creatures vanquished ---------------\n\n");

    for (guint mnum = MT_NONE + 1; mnum < MT_MAX; mnum++)
    {
        if (p->stats.monsters_killed[mnum] > 0)
        {
            guint mcount = p->stats.monsters_killed[mnum];
            tmp = str_capitalize(g_strdup(monster_type_plural_name(mnum,
                                          mcount)));

            g_string_append_printf(text, "%3d %s\n", mcount, tmp);

            g_free(tmp);
            body_count += mcount;
        }
    }
    g_string_append_printf(text, "\n%3d total\n", body_count);

    /* genocided monsters */
    gboolean printed_headline = FALSE;
    for (guint mnum = MT_NONE + 1; mnum < MT_MAX; mnum++)
    {
        if (!monster_is_genocided(mnum))
            continue;

        if (!printed_headline)
        {
                g_string_append(text, "\n\n-- Genocided creatures ---------------\n\n");
        }

        tmp = str_capitalize(g_strdup(monster_type_plural_name(mnum, 2)));
        g_string_append_printf(text, "%s\n", tmp);
    }

     /* messages */
    g_string_append(text, "\n\n-- Last messages ----------------------\n\n");
    for (guint pos = log_length(nlarn->log) - min(10, log_length(nlarn->log));
         pos < log_length(nlarn->log); pos++)
    {
        message_log_entry *entry = log_get_entry(nlarn->log, pos);
        g_string_append_printf(text, "%s\n", entry->message);
    }
    /* print uncommitted messages */
    if (nlarn->log->buffer->len > 0)
    {
        g_string_append_printf(text, "%s\n", nlarn->log->buffer->str);
    }

    return (g_string_free(text, FALSE));
}

static void player_memorial_file_save(player *p, const char *text)
{
    char *proposal = NULL;
    char *filename;
    gboolean done = FALSE;

    while (!done)
    {
        GError *error = NULL;

        if (proposal == NULL)
        {
            /* When starting, propose a file name. Use the previously
               entered file name when trying to find a new name for an
               existing file. */
            proposal = g_strconcat(p->name, ".txt", NULL);
        }

        filename = display_get_string("Enter filename: ", proposal, 40);
        g_free(proposal);
        proposal = NULL;

        if (filename == NULL)
        {
            /* user pressed ESC, thus display_get_string() returned NULL */
            done = TRUE;
        } else {
            /* file name has been provided, try to save file */
            char *fullname = g_build_path(G_DIR_SEPARATOR_S,
#ifdef G_OS_WIN32
                    g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS),
#else
                    g_get_home_dir(),
#endif
                    filename, NULL);

            if (g_file_test(fullname, G_FILE_TEST_IS_SYMLINK))
            {
                display_show_message("Error", "File is a symlink. I won't " \
                        "overwrite those...", 0);
                g_free(fullname);
                continue;
            }

            if (g_file_test(fullname, G_FILE_TEST_IS_DIR))
            {
                display_show_message("Error", "There is a directory with the " \
                        "name you gave. Thus I can't write the file.", 0);
                g_free(fullname);
                continue;
            }

            if (g_file_test(fullname, G_FILE_TEST_IS_REGULAR)
                    && !display_get_yesno("File exists!\n"
                        "Do you want to overwrite it?", NULL, NULL, NULL))
            {
                g_free(fullname);
                proposal = filename;
                continue;
            }

            /* wrap the text and insert line feed for the platform */
            char *wtext = str_prepare_for_saving(text);

            if (g_file_set_contents(fullname, wtext, -1, &error))
            {
                /* successfully saved the memorial file */
                done = TRUE;
            }
            else
            {
                display_show_message("Error", error->message, 0);
                g_error_free(error);
            }

            g_free(wtext);

            g_free(filename);
            g_free(fullname);
        }
    }
}

static int item_filter_equippable(item *it)
{
    return player_item_is_equippable(nlarn->p, it);
}

static int item_filter_dropable(item *it)
{
    return player_item_is_dropable(nlarn->p, it);
}

static int player_item_filter_multiple(player *p __attribute__((unused)), item *it)
{
    return (it->count > 1);
}
