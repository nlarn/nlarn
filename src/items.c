/*
 * items.h
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
#include <string.h>

#include "amulets.h"
#include "armour.h"
#include "container.h"
#include "display.h"
#include "food.h"
#include "game.h"
#include "gems.h"
#include "items.h"
#include "map.h"
#include "nlarn.h"
#include "player.h"
#include "potions.h"
#include "rings.h"
#include "scrolls.h"
#include "spells.h"
#include "utils.h"
#include "weapons.h"

static void item_typename_pluralize(item *it, char *description, int length);
static char *item_name_count(char *name, char *add_info, int singular, int definite, int length, int count);
static char *item_desc_get(item *it, int known);

const item_type_data item_data[IT_MAX] =
{
    /* item_t       name_sg      name_pl        IMG  desc_known      desc_unknown         max_id           op eq us st id */
    { IT_NONE,      "",          "",            ' ', "",             "",                  0,               0, 0, 0, 0, 0, },
    { IT_AMULET,    "amulet",    "amulets",     '"', "amulet of %s", "%s amulet",         AM_LARN,         0, 1, 0, 0, 1, },
    { IT_ARMOUR,    "armour",    "armour",      '[', "%s",           "%s",                AT_MAX,          1, 1, 0, 0, 1, },
    { IT_BOOK,      "book",      "books",       '+', "book of %s",   "%s book",           SP_MAX,          0, 0, 1, 1, 1, },
    { IT_CONTAINER, "container", "containers",  'C', "%s",           "%s",                CT_MAX,          0, 0, 0, 0, 0, },
    { IT_FOOD,      "food",      "foods",       '%', "%s",           "%s",                FT_MAX,          0, 0, 1, 1, 0, },
    { IT_GEM,       "gem",       "gems",        '*', "%s",           "%s gem",            GT_MAX,          1, 0, 0, 1, 0, },
    { IT_GOLD,      "coin",      "coins",       '$', "%s",           "%s",                0,               0, 0, 0, 1, 0, },
    { IT_POTION,    "potion",    "potions",     '!', "potion of %s", "%s potion",         PO_CURE_DIANTHR, 0, 0, 1, 1, 1, },
    { IT_RING,      "ring",      "rings",       '=', "ring of %s",   "%s ring",           RT_MAX,          1, 1, 0, 0, 1, },
    { IT_SCROLL,    "scroll",    "scrolls",     '?', "scroll of %s", "scroll labeled %s", ST_MAX,          0, 0, 1, 1, 1, },
    { IT_WEAPON,    "weapon",    "weapons",     '(', "%s",           "%s",                WT_MAX,          1, 1, 0, 0, 1, },
};

const item_material_data item_materials[IM_MAX] =
{
    /* type           name           adjective   colour */
    { IM_NONE,        "",            "",         DC_NONE      },
    { IM_PAPER,       "paper",       "papier",   DC_WHITE     },
    { IM_CLOTH,       "cloth",       "cloth",    DC_LIGHTGRAY },
    { IM_LEATHER,     "leather",     "leathern", DC_BROWN     },
    { IM_WOOD,        "wood",        "wooden",   DC_BROWN     },
    { IM_BONE,        "bone",        "osseous",  DC_DARKGRAY  },
    { IM_DRAGON_HIDE, "dragon hide", "scabby",   DC_NONE      }, /* ? */
    { IM_IRON,        "iron",        "irony",    DC_LIGHTGRAY },
    { IM_STEEL,       "steel",       "steely",   DC_WHITE     },
    { IM_COPPER,      "copper",      "cupreous", DC_BROWN     },
    { IM_SILVER,      "silver",      "silvery",  DC_LIGHTGRAY },
    { IM_GOLD,        "gold",        "golden",   DC_YELLOW    },
    { IM_PLATINUM,    "platinum",    "platinum", DC_WHITE     },
    { IM_MITHRIL,     "mitril",      "mithrial", DC_LIGHTGRAY },
    { IM_GLASS,       "glass",       "vitreous", DC_WHITE     },
    { IM_GEMSTONE,    "gemstone",    "gemstone", DC_NONE      }, /* ? */
};

/* functions */

item *item_new(item_t item_type, int item_id, int item_bonus)
{
    item *nitem;
    effect *eff = NULL;
    int loops = 0;

    assert(item_type > IT_NONE && item_type < IT_MAX);

    /* has to be zeroed or memcmp will fail */
    nitem = g_malloc0(sizeof(item));

    nitem->type = item_type;
    nitem->id = item_id;
    nitem->bonus = item_bonus;
    nitem->count = 1;

    /* set bonus_known on unoptimizable items to avoid stacking problems */
    if (!item_is_optimizable(item_type))
    {
         nitem->bonus_known = TRUE;
    }

    /* special item type specific attributes */
    switch (item_type)
    {
    case IT_AMULET:
        /* if the amulet has already been created try to create another one */
        while (nlarn->amulet_created[nitem->id] && (loops < item_max_id(IT_AMULET)))
        {
            nitem->id = rand_1n(item_max_id(IT_AMULET));
            loops++;
        }

        if (loops == item_max_id(IT_AMULET))
        {
            /* every amulet has been created -> return a ring instead */
            g_free(nitem);
            return item_new(IT_RING, rand_1n(item_max_id(IT_RING)), item_bonus);
        }

        nlarn->amulet_created[nitem->id] = TRUE;

        if (amulet_effect_type(nitem))
        {
            eff = effect_new(amulet_effect_type(nitem), 0);
            item_effect_add(nitem, eff);
        }

        break;

    case IT_GEM:
        /* ensure minimal size */
        if (nitem->bonus == 0)
            nitem->bonus = rand_1n(20);
        break;

    case IT_GOLD:
        nitem->count = item_id;
        break;

    case IT_POTION:
        /* prevent that the unique potion can be created twice */
        if ((item_id == PO_CURE_DIANTHR) && nlarn->cure_dianthr_created)
        {
            nitem->id = rand_1n(item_max_id(IT_POTION));
        }
        else if (item_id == PO_CURE_DIANTHR)
        {
            nlarn->cure_dianthr_created = TRUE;
        }
        break;

    case IT_RING:
        if (ring_effect_type(nitem))
        {
            eff = effect_new(ring_effect_type(nitem), 0);

            if (item_bonus)
            {
                eff->amount += item_bonus;
            }

            /* ring of extra regeneration is better than the average */
            if (item_id == RT_EXTRA_REGEN)
            {
                eff->amount *= 5;
            }

            item_effect_add(nitem, eff);
        }
        break;

    case IT_WEAPON:
        while (weapon_is_unique(nitem) && nlarn->weapon_created[nitem->id])
        {
            /* create another random weapon instead */
            nitem->id = rand_1n(WT_MAX);
        }

        if (weapon_is_unique(nitem))
        {
            /* mark unique weapon as created */
            nlarn->weapon_created[nitem->id] = TRUE;
        }

        /* special effects for Bessman's Hammer */
        if (nitem->id == WT_BESSMAN)
        {
            eff = effect_new(ET_INC_STR, 0);
            eff->amount = 10;
            item_effect_add(nitem, eff);

            eff = effect_new(ET_INC_DEX, 0);
            eff->amount = 10;
            item_effect_add(nitem, eff);

            eff = effect_new(ET_DEC_INT, 0);
            eff->amount = 10;
            item_effect_add(nitem, eff);
        }
        break;

    default:
        /* nop */
        break;
    }

    /* register item with game */
    nitem->oid = game_item_register(nlarn, nitem);

    return nitem;
}

item *item_new_random(item_t item_type)
{
    item *it;

    int item_id = 0;
    int item_bonus = 0;
    int min_id = 1, max_id = 0;
    int want_curse = TRUE; /* if the item has a chance to be cursed / blessed */

    assert(item_type > IT_NONE && item_type < IT_MAX);

    max_id = item_max_id(item_type);

    /* special settings for some item types */
    switch (item_type)
    {
    case IT_ARMOUR:
        item_bonus = rand_m_n(-3, 3);
        break;

    case IT_CONTAINER:
        max_id = CT_CHEST; /* only bags and caskets */
        want_curse = FALSE;
        break;

    case IT_FOOD:
    case IT_GEM:
        want_curse = FALSE;
        break;

    case IT_GOLD:
        /* ID is amount in case of gold */
        min_id = 50;
        max_id = 250;
        want_curse = FALSE;
        break;

    case IT_RING:
        item_bonus = rand_0n(3);
        break;

    case IT_WEAPON:
        item_bonus = rand_m_n(-3, 3);
        break;

    default:
        /* nop */
        break;
    }
    item_id = rand_m_n(min_id, max_id - 1);

    it = item_new(item_type, item_id, item_bonus);

    /* maybe the item is blessed or cursed */
    if (want_curse && chance(25))
    {
        if (chance(50))
        {
            item_bless(it);
        }
        else
        {
            item_curse(it);
        }
    }

    return it;
}

item *item_new_by_level(item_t item_type, int num_level)
{
    item *nitem;
    int id_min, id_max;
    int item_bonus = 0;
    float variance, id_base, divisor;

    assert (item_type > IT_NONE && item_type < IT_MAX && num_level < MAP_MAX);

    /* no amulets above map 8 */
    if ((item_type == IT_AMULET) && (num_level < 8))
        item_type = IT_RING;

    divisor = 1 / (float)(MAP_MAX - 1);

    switch (item_type)
    {
    case IT_ARMOUR:
    case IT_WEAPON:
        item_bonus = rand_m_n(-3, 3);
    case IT_BOOK:
        variance = 0.2;
        break;

    case IT_RING:
        item_bonus = rand_0n(3);
        variance = 0.5;
        break;

    default:
        /* no per-map randomnization */
        return item_new_random(item_type);
    }

    id_base = item_max_id(item_type) * (num_level * divisor);
    id_min = id_base - (item_max_id(item_type) * variance);
    id_max = id_base + (item_max_id(item_type) * variance);

    /* clean results */
    if (id_min < 1) id_min = 1;
    if (id_max < 1) id_max = 1;
    if (id_max > item_max_id(item_type)) id_max = item_max_id(item_type);

    /* create the item */
    nitem = item_new(item_type, rand_m_n(id_min, id_max), item_bonus);

    /* fine touch */
    /* maybe the item is blessed or cursed */
    if (chance(25))
    {
        if (chance(50))
        {
            item_bless(nitem);
        }
        else
        {
            item_curse(nitem);
        }
    }

    return nitem;
}

item *item_copy(item *original)
{
    item *nitem;
    guint idx = 0;

    assert(original != NULL);

    /* clone item */
    nitem = g_malloc0(sizeof(item));
    memcpy(nitem, original, sizeof(item));

    /* copy effects */
    nitem->effects = NULL;

    if (original->effects != NULL)
    {
        for (idx = 0; idx < original->effects->len; idx++)
        {
            effect *e = g_ptr_array_index(original->effects, idx);
            effect *ne = effect_copy(e);

            ne->item = nitem;
            item_effect_add(nitem, ne);
        }
    }

    /* reset inventory */
    nitem->content = NULL;

    /* regíster copy with game */
    nitem->oid = game_item_register(nlarn, nitem);

    return nitem;
}

item *item_split(item *original, guint32 count)
{
    item *nitem;

    assert(original != NULL && count < original->count);

    nitem = item_copy(original);

    nitem->count = count;
    original->count -= count;

    return nitem;
}

void item_destroy(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (it->effects)
    {
        effect *eff;

        while (it->effects->len)
        {
            gpointer effect_id = g_ptr_array_remove_index_fast(it->effects,
                                 it->effects->len - 1);

            if ((eff = game_effect_get(nlarn, effect_id)))
                effect_destroy(eff);
        }

        g_ptr_array_free(it->effects, TRUE);
    }

    if (it->content != NULL)
    {
        inv_destroy(it->content);
    }

    /* unregister item */
    game_item_unregister(nlarn, it->oid);

    g_free(it);
}

void item_serialize(gpointer oid, gpointer it, gpointer root)
{
    cJSON *ival;

    item *i = (item *)it;

    cJSON_AddItemToArray((cJSON *)root, ival = cJSON_CreateObject());

    cJSON_AddNumberToObject(ival, "oid", GPOINTER_TO_UINT(oid));
    cJSON_AddNumberToObject(ival, "type", i->type);
    cJSON_AddNumberToObject(ival, "id", i->id);
    cJSON_AddNumberToObject(ival, "bonus", i->bonus);
    cJSON_AddNumberToObject(ival, "count", i->count);

    if (i->blessed == 1) cJSON_AddTrueToObject(ival, "blessed");
    if (i->cursed == 1) cJSON_AddTrueToObject(ival, "cursed");
    if (i->blessed_known == 1) cJSON_AddTrueToObject(ival, "blessed_known");
    if (i->bonus_known == 1) cJSON_AddTrueToObject(ival, "bonus_known");

    if (i->corroded > 0) cJSON_AddNumberToObject(ival, "corroded", i->corroded);
    if (i->burnt > 0) cJSON_AddNumberToObject(ival, "burnt", i->burnt);
    if (i->rusty > 0) cJSON_AddNumberToObject(ival, "rusty", i->rusty);

    /* container content */
    if (inv_length(i->content) > 0)
    {
        cJSON_AddItemToObject(ival, "content", inv_serialize(i->content));
    }

    /* effects */
    if (i->effects)
    {
        cJSON_AddItemToObject(ival, "effects", effects_serialize(i->effects));
    }
}

item *item_deserialize(cJSON *iser, struct game *g)
{
    guint oid;
    item *it;
    cJSON *obj;

    it = g_malloc0(sizeof(item));

    /* must-have attributes */
    oid = cJSON_GetObjectItem(iser, "oid")->valueint;
    it->oid = GUINT_TO_POINTER(oid);

    it->type = cJSON_GetObjectItem(iser, "type")->valueint;
    it->id = cJSON_GetObjectItem(iser, "id")->valueint;
    it->bonus = cJSON_GetObjectItem(iser, "bonus")->valueint;
    it->count = cJSON_GetObjectItem(iser, "count")->valueint;

    /* optional attributes */
    obj = cJSON_GetObjectItem(iser, "blessed");
    if (obj != NULL) it->blessed = obj->valueint;

    obj = cJSON_GetObjectItem(iser, "cursed");
    if (obj != NULL) it->cursed = obj->valueint;

    obj = cJSON_GetObjectItem(iser, "blessed_known");
    if (obj != NULL) it->blessed_known = obj->valueint;

    obj = cJSON_GetObjectItem(iser, "bonus_known");
    if (obj != NULL) it->bonus_known = obj->valueint;

    obj = cJSON_GetObjectItem(iser, "corroded");
    if (obj != NULL) it->corroded = obj->valueint;

    obj = cJSON_GetObjectItem(iser, "burnt");
    if (obj != NULL) it->burnt = obj->valueint;

    obj = cJSON_GetObjectItem(iser, "rusty");
    if (obj != NULL) it->rusty = obj->valueint;

    /* container content */
    obj = cJSON_GetObjectItem(iser, "content");
    if (obj != NULL) it->content = inv_deserialize(obj);

    /* effects */
    obj = cJSON_GetObjectItem(iser, "effects");
    if (obj != NULL) it->effects = effects_deserialize(obj);

    /* add item to game */
    g_hash_table_insert(g->items, it->oid, it);

    /* increase max_id to match used ids */
    if (g->item_max_id < oid)
        g->item_max_id = oid;

    return it;
}

/**
 * Compare two items
 * @param a item1
 * @param b item2
 * @return TRUE if items are identical
 */
int item_compare(item *a, item *b)
{
    int tmp_count, result;
    gpointer oid_a, oid_b;

    if (a->type != b->type)
    {
        return FALSE;
    }
    else if (a->type == IT_GOLD)
    {
        return TRUE;
    }

    /* as count can be different for equal items, save count of b */
    tmp_count = b->count;
    b->count = a->count;

    /* store oids (never identical!) */
    oid_a = a->oid;
    oid_b = b->oid;

    a->oid = NULL;
    b->oid = NULL;

    result = (memcmp(a, b, sizeof(item)) == 0);

    b->count = tmp_count;

    /* restore oids */
    a->oid = oid_a;
    b->oid = oid_b;

    return result;
}

int item_sort(gconstpointer a, gconstpointer b, gpointer data)
{
    gint order;
    item *item_a = game_item_get(nlarn, *((gpointer**)a));
    item *item_b = game_item_get(nlarn, *((gpointer**)b));
    player *p = (player *)data;

    if (item_a->type == item_b->type)
    {
        /* both items are of identical type. compare their names. */
        order = g_ascii_strcasecmp(item_desc_get(item_a, player_item_known(p, item_a)),
                                   item_desc_get(item_b, player_item_known(p, item_b)));
    }
    else if (item_a->type < item_b->type)
        order = -1;
    else
        order = 1;

    return order;
}

char *item_describe(item *it, int known, int singular, int definite, char *str, int str_len)
{
    char desc[61] = "";
    char *temp = NULL;
    char *add_info = NULL;
    GPtrArray *add_info_elems = g_ptr_array_new();

    assert((it != NULL) && (it->type > IT_NONE) && (it->type < IT_MAX));

    /* collect additional information */
    if (it->blessed_known)
    {
        if (it->blessed)
            g_ptr_array_add(add_info_elems, "blessed");
        else if (it->cursed)
            g_ptr_array_add(add_info_elems, "cursed");
        else
            g_ptr_array_add(add_info_elems, "uncursed");
    }

    if (it->burnt == 1)
    {
        g_ptr_array_add(add_info_elems, "burnt");
    }
    if (it->burnt == 2)
    {
        g_ptr_array_add(add_info_elems, "very burnt");
    }

    if (it->corroded == 1)
    {
        g_ptr_array_add(add_info_elems, "corroded");
    }
    if (it->corroded == 2)
    {
        g_ptr_array_add(add_info_elems, "very corroded");
    }

    if (it->rusty == 1)
    {
        g_ptr_array_add(add_info_elems, "rusty");
    }
    if (it->rusty == 2)
    {
        g_ptr_array_add(add_info_elems, "very rusty");
    }

    if (add_info_elems->len > 0)
    {
        /* add sentinel */
        g_ptr_array_add(add_info_elems, NULL);
        add_info = g_strjoinv(", ", (char**)(add_info_elems->pdata));
    }

    switch (it->type)
    {
    case IT_ARMOUR:
        g_snprintf(desc, 60, item_desc_get(it, known));
        if (it->bonus_known)
        {
            g_snprintf(str, str_len, "%s %+d",
                       item_name_count(desc, add_info, singular, definite, 60,
                                       it->count), it->bonus);
        }
        else
        {
            g_snprintf(str, str_len, item_name_count(desc, add_info, singular,
                       definite, 60, it->count));
        }

        break;

    case IT_CONTAINER:
        g_snprintf(desc, 60, item_desc_get(it, known));
        item_name_count(desc, add_info, singular, definite, 60, it->count);
        g_snprintf(str, str_len, desc);
        break;

    case IT_FOOD:
        g_snprintf(desc, 60, item_desc_get(it, known));
        item_name_count(desc, add_info, singular, definite, 60, it->count);

        if ((it->count > 1) && !singular)
        {
            g_snprintf(str, str_len, "%ss", desc);
        }
        else
        {
            g_snprintf(str, str_len, desc);
        }

        break;

    case IT_GOLD:
        g_snprintf(str, str_len, "%d gold pieces", it->count);
        break;

    case IT_GEM:
        g_snprintf(desc, 60, item_desc_get(it, known));
        g_snprintf(str, str_len, "%d carats %s", gem_size(it), desc);

        item_name_count(str, add_info, singular, definite, str_len, it->count);
        break;

    case IT_WEAPON:
        if (weapon_is_unique(it))
        {
            char aip[41] = { 0 };
            if (it->bonus_known)
            {
                if (add_info != NULL)
                    g_snprintf(aip, 40, ", %s", add_info);

                g_snprintf(str, str_len, "%s%s (%+d%s)",
                           weapon_needs_article(it) ? "the " : "",
                           item_desc_get(it, known), it->bonus, aip);
            }
            else
            {
                if (add_info != NULL)
                    g_snprintf(aip, 40, " (%s)", add_info);

                g_snprintf(str, str_len, "%s%s%s",
                           weapon_needs_article(it) ? "the " : "",
                           item_desc_get(it, known), aip);
            }
        }
        else
        {
            g_snprintf(desc, 60, item_desc_get(it, known));
            if (it->bonus_known)
            {
                item_name_count(desc, add_info, singular, definite, 60, it->count);
                g_snprintf(str, str_len, "%s %+d", desc, it->bonus);
            }
            else
            {
                item_name_count(desc, add_info, singular, definite, 60, it->count);
                g_snprintf(str, str_len, desc);
            }
        }
        break;

    default:
        if (known)
        {
            g_snprintf(desc, 60, "%s", item_data[it->type].desc_known);
        }
        else
        {
            if ((it->type == IT_SCROLL) && (it->id == ST_BLANK))
            {
                g_snprintf(desc, 60, "unlabeled scroll");
            }
            else
            {
                g_snprintf(desc, 60, "%s", item_data[it->type].desc_unknown);
            }
        }

        if ((it->count > 1) && !singular)
        {
            item_typename_pluralize(it, desc, 60);
        }

        temp = g_strdup(desc);
        g_snprintf(desc, 60, temp, item_desc_get(it, known));
        g_free(temp);

        /* display bonus if it is a ring */
        if (it->bonus_known && it->type == IT_RING)
        {
            item_name_count(desc, add_info, singular, definite, 60, it->count);
            g_snprintf(str, str_len, "%s %+d", desc, it->bonus);
        }
        else
        {
            item_name_count(desc, add_info, singular, definite, 60, it->count);
            g_snprintf(str, str_len, desc);
        }

        break;
    }

    g_free(add_info);
    g_ptr_array_free(add_info_elems, TRUE);

    return(str);
}

item_material_t item_material(item *it)
{
    item_material_t material;

    assert (it != NULL);

    switch (it->type)
    {
    case IT_AMULET:
        material = amulet_material(it->id);
        break;

    case IT_ARMOUR:
        material = armour_material(it);
        break;

    case IT_BOOK:
        material = IM_PAPER;
        break;

    case IT_CONTAINER:
        material = container_material(it);
        break;

    case IT_FOOD:
        /* TODO: food material */
        material = IM_NONE;
        break;

    case IT_GEM:
        material = IM_GEMSTONE;
        break;

    case IT_GOLD:
        material = IM_GOLD;
        break;

    case IT_POTION:
        material = IM_GLASS;
        break;

    case IT_RING:
        material = ring_material(it->id);
        break;

    case IT_SCROLL:
        material = IM_PAPER;
        break;

    case IT_WEAPON:
        material =  weapon_material(it);
        break;

    default:
        material = IM_NONE;
    }

    return material;
}

guint item_price(item *it)
{
    guint price;

    assert (it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_AMULET:
        price = amulet_price(it);
        break;

    case IT_ARMOUR:
        price = armour_price(it);
        break;

    case IT_BOOK:
        price = book_price(it);
        break;

    case IT_CONTAINER:
        price = container_price(it);
        break;

    case IT_FOOD:
        price = food_price(it);
        break;

    case IT_GEM:
        price = gem_price(it);
        break;

    case IT_GOLD:
        price = 0;
        break;

    case IT_POTION:
        price = potion_price(it);
        break;

    case IT_RING:
        price = ring_price(it);
        break;

    case IT_SCROLL:
        price = scroll_price(it);
        break;

    case IT_WEAPON:
        price =  weapon_price(it);
        break;

    default:
        price = 0;
    }

    /* modify base prices by item's attributes */

    /* 20% price increase / decrease for every +/-1 bonus */
    if (it->bonus != 0) price = price * (1 + (0.2 * it->bonus));

    /* double price if blessed */
    if (it->blessed) price <<=1;

    /* half price if cursed */
    if (it->cursed) price >>=1;

    /* half price if corroded */
    if (it->corroded == 1) price >>=1;

    /* quarter price if very corroded */
    if (it->corroded == 2) price /= 4;

    /* half price if burnt */
    if (it->burnt == 1) price >>=1;

    /* quarter price if very burnt */
    if (it->burnt == 2) price /= 4;

    /* half price if rusty */
    if (it->rusty == 1) price >>=1;

    /* quarter price if very rusty */
    if (it->rusty == 2) price /= 4;

    return price;
}

/**
 * Calculate the weight of an given object
 * @param an item
 * @return weight in grams
 */
int item_weight(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_AMULET:
        return 150;
        break;

    case IT_ARMOUR:
        return armour_weight(it);
        break;

    case IT_BOOK:
        return book_weight(it);
        break;

    case IT_POTION:
        return 250;
        break;

    case IT_RING:
        return 10;
        break;

    case IT_SCROLL:
        return 100;
        break;

    case IT_CONTAINER:
        return container_weight(it) + inv_weight(it->content);
        break;

    case IT_FOOD:
        return food_weight(it);
        break;

    case IT_GOLD:
        /* Is this too heavy? is this too light?
           It should give the player a reason to use the bank. */
        return it->count * 4;
        break;

    case IT_GEM:
        return gem_weight(it);
        break;

    case IT_WEAPON:
        return weapon_weight(it);
        break;

    default:
        return 0;
        break;
    }
}

/**
 * Determine the colour of an given object
 * @param an item
 * @return the coulour
 */
int item_colour(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_AMULET:
    case IT_ARMOUR:
    case IT_RING:
    case IT_WEAPON:
        return item_materials[item_material(it)].colour;
        break;

    case IT_BOOK:
        return book_colour(it);
        break;

    case IT_POTION:
        return potion_colour(it->id);
        break;

    case IT_SCROLL:
        return DC_WHITE;
        break;

    case IT_CONTAINER:
        return DC_BROWN;
        break;

    case IT_FOOD:
        return DC_BROWN;
        break;

    case IT_GOLD:
        return DC_YELLOW;
        break;

    case IT_GEM:
        return gem_colour(it);
        break;

    default:
        return DC_BLACK;
        break;
    }
}

void item_effect_add(item *it, effect *e)
{
    assert (it != NULL && e != NULL);

    /* create list if not existant */
    if (!it->effects)
    {
        it->effects = g_ptr_array_new();
    }

    /* this effect is permanent */
    e->turns = 0;

    /* link item to effect */
    e->item = it->oid;

    /* add effect to list */
    effect_add(it->effects, e);
}

void item_effect_del(item *it, effect *e)
{
    assert (it != NULL && e != NULL);

    /* del effect from list */
    effect_del(it->effects, e);

    /*destroy effect */
    effect_destroy(e);

    /* delete array if no effect is left */
    if (!it->effects->len)
    {
        g_ptr_array_free(it->effects, TRUE);
        it->effects = NULL;
    }
}

int item_bless(item *it)
{
    assert (it != NULL && !it->cursed);

    if (it->blessed)
        return FALSE;

    it->blessed = TRUE;

    return TRUE;
}

int item_curse(item *it)
{
    assert (it != NULL && !it->blessed);

    if (it->cursed)
        return FALSE;

    it->cursed = TRUE;

    return TRUE;
}

int item_remove_blessing(item *it)
{
    assert (it != NULL && it->blessed);

    if (!it->blessed)
        return FALSE;

    it->blessed = FALSE;
    it->blessed_known = FALSE;

    return TRUE;
}

int item_remove_curse(item *it)
{
    assert (it != NULL && it->cursed);

    if (!it->cursed || it->blessed)
        return FALSE;

    it->cursed = FALSE;
    it->blessed_known = FALSE;

    return TRUE;
}

int item_enchant(item *it)
{
    guint pos;
    gpointer oid;
    effect *e;
    char desc[81] = { 0 };

    assert(it != NULL);

    it->bonus++;

    /* warn against overenchantment */
    if (it->bonus == 3)
    {
        /* hide bonus from description */
        gboolean bonus_known = it->bonus_known;
        it->bonus_known = FALSE;

        item_describe(it, player_item_known(nlarn->p, it),
                      (it->count == 1), TRUE, desc, 80);

        desc[0] = g_ascii_toupper(desc[0]);
        log_add_entry(nlarn->p->log, "%s vibrate%s strangely.",
                      desc, (it->count == 1) ? "s" : "");

        it->bonus_known = bonus_known;
    }

    /* item has been overenchanted */
    if (it->bonus > 3)
    {
        player_item_destroy(nlarn->p, it);
    }

    if ((it->type == IT_RING) && it->effects)
    {
        for (pos = 0; pos < it->effects->len; pos++)
        {
            oid = g_ptr_array_index(it->effects, pos);
            e = game_effect_get(nlarn, oid);

            e->amount++;
        }
    }

    return it->bonus;
}

int item_disenchant(item *it)
{
    guint pos;
    gpointer oid;
    effect *e;

    assert(it != NULL);

    it->bonus--;

    if (it->bonus < -3)
    {
        player_item_destroy(nlarn->p, it);
    }

    if ((it->type == IT_RING) && it->effects)
    {
        for (pos = 0; pos < it->effects->len; pos++)
        {
            oid = g_ptr_array_index(it->effects, pos);
            e = game_effect_get(nlarn, oid);

            e->amount--;
        }
    }

    return it->bonus;
}

int item_rust(item *it)
{
    assert(it != NULL);

    if (item_material(it) == IM_IRON)
    {
        if (it->rusty == 2)
        {
            /* it's been very rusty already -> destroy */
            return PI_DESTROYED;
        }
        else
        {
            it->rusty++;
            return PI_ENFORCED;
        }
    }
    else
    {
        /* item cannot rust. */
        return PI_NONE;
    }
}

int item_corrode(item *it)
{
    assert(it != NULL);

    if (item_material(it) == IM_IRON)
    {
        if (it->corroded == 2)
        {
            return PI_DESTROYED;
        }
        else
        {
            it->corroded++;
            return PI_ENFORCED;
        }
    }
    else
    {
        return PI_NONE;
    }
}

int item_burn(item *it)
{
    assert(it != NULL);

    if (item_material(it) <= IM_BONE)
    {
        if (it->burnt == 2)
        {
            return PI_DESTROYED;
        }
        else
        {
            it->burnt++;
            return PI_ENFORCED;
        }
    }
    else
    {
        return PI_NONE;
    }
}

int item_obtainable(item_t type, int id)
{
    int obtainable;

    switch (type)
    {
    case IT_ARMOUR:
    case IT_FOOD:
    case IT_RING:
        obtainable = TRUE;
        break;

    case IT_BOOK:
        obtainable = book_type_obtainable(id);
        break;

    case IT_POTION:
        obtainable = potion_type_obtainable(id);
        break;

    case IT_SCROLL:
        obtainable = scroll_type_obtainable(id);
        break;

    case IT_WEAPON:
        obtainable = weapon_type_obtainable(id);
        break;

    default:
        obtainable = FALSE;
    }

    return obtainable;
}

inventory *inv_new(gconstpointer owner)
{
    inventory *ninv;

    ninv = g_malloc0(sizeof(inventory));
    ninv->content = g_ptr_array_new();

    ninv->owner = owner;

    return ninv;
}

void inv_destroy(inventory *inv)
{
    assert(inv != NULL);

    while (inv_length(inv) > 0)
    {
        item *it = inv_get(inv, inv_length(inv) - 1);
        g_ptr_array_remove(inv->content, it->oid);
        item_destroy(it);
    }

    g_ptr_array_free(inv->content, TRUE);

    g_free(inv);
}

cJSON *inv_serialize(inventory *inv)
{
    int idx;
    cJSON *sinv = cJSON_CreateArray();

    for (idx = 0; idx < inv_length(inv); idx++)
    {
        item *it = inv_get(inv, idx);
        cJSON_AddItemToArray(sinv, cJSON_CreateNumber(GPOINTER_TO_UINT(it->oid)));
    }

    return sinv;
}

inventory *inv_deserialize(cJSON *iser)
{
    int idx;
    inventory *inv;

    inv = g_malloc0(sizeof(inventory));

    inv->content = g_ptr_array_new();

    for (idx = 0; idx < cJSON_GetArraySize(iser); idx++)
    {
        guint oid = cJSON_GetArrayItem(iser, idx)->valueint;
        g_ptr_array_add(inv->content, GUINT_TO_POINTER(oid));
    }

    return inv;
}

void inv_callbacks_set(inventory *inv, inv_callback_bool pre_add,
                       inv_callback_void post_add, inv_callback_bool pre_del,
                       inv_callback_void post_del)
{
    assert (inv != NULL);

    inv-> pre_add  = pre_add;
    inv-> post_add = post_add;
    inv-> pre_del  = pre_del;
    inv-> post_del = post_del;
}

int inv_add(inventory **inv, item *it)
{
    guint idx;

    assert(inv != NULL && it != NULL && it->oid != NULL);

    /* create inventory if necessary */
    if (!(*inv))
    {
        *inv = inv_new(NULL);
    }

    /* check if pre_add callback is set */
    if ((*inv)->pre_add)
    {
        /* call pre_add callback */
        if (!(*inv)->pre_add(*inv, it))
        {
            return FALSE;
        }
    }

    /* stack stackable items */
    if (item_is_stackable(it->type))
    {
        for (idx = 0; idx < inv_length(*inv); idx++)
        {
            item *i = inv_get(*inv, idx);
            if (item_compare(i, it))
            {
                /* just increase item count and release the original */
                i->count += it->count;
                item_destroy(it);

                return inv_length(*inv);
            }
        }
    }

    g_ptr_array_add((*inv)->content, it->oid);

    /* call post_add callback */
    if ((*inv)->post_add)
    {
        (*inv)->post_add(*inv, it);
    }

    return inv_length(*inv);
}

item *inv_get(inventory *inv, guint idx)
{
    gpointer oid = NULL;

    assert (inv != NULL && idx < inv->content->len);
    oid = g_ptr_array_index(inv->content, idx);

    return game_item_get(nlarn, oid);
}

item *inv_del(inventory **inv, guint idx)
{
    item *item;

    assert(*inv != NULL && (*inv)->content != NULL && idx < inv_length(*inv));

    item = inv_get(*inv, idx);

    if ((*inv)->pre_del)
    {
        if (!(*inv)->pre_del(*inv, item))
        {
            return NULL;
        }
    }

    g_ptr_array_remove_index((*inv)->content, idx);

    if ((*inv)->post_del)
    {
        (*inv)->post_del(*inv, item);
    }

    /* destroy inventory if empty and not owned by anybody */
    if (!inv_length(*inv) && !(*inv)->owner)
    {
        inv_destroy(*inv);
        *inv = NULL;
    }

    return item;
}

int inv_del_element(inventory **inv, item *it)
{
    assert(*inv != NULL && (*inv)->content != NULL && it != NULL);

    if ((*inv)->pre_del)
    {
        if (!(*inv)->pre_del(*inv, it))
        {
            return FALSE;
        }
    }

    g_ptr_array_remove((*inv)->content, it->oid);

    if ((*inv)->post_del)
    {
        (*inv)->post_del(*inv, it);
    }

    /* destroy inventory if empty and not owned by anybody */
    if (!inv_length(*inv) && !(*inv)->owner)
    {
        inv_destroy(*inv);
        *inv = NULL;
    }

    return TRUE;
}

int inv_del_oid(inventory **inv, gpointer oid)
{
    assert(*inv != NULL && (*inv)->content != NULL && oid != NULL);

    g_ptr_array_remove((*inv)->content, oid);

    /* destroy inventory if empty and not owned by anybody */
    if (!inv_length(*inv) && !(*inv)->owner)
    {
        inv_destroy(*inv);
        *inv = NULL;
    }

    return TRUE;
}

guint inv_length(inventory *inv)
{
    return (inv == NULL) ? 0 : inv->content->len;
}

void inv_sort(inventory *inv, GCompareDataFunc compare_func, gpointer user_data)
{
    assert(inv != NULL && inv->content != NULL);

    g_ptr_array_sort_with_data(inv->content, compare_func, user_data);
}

int inv_weight(inventory *inv)
{
    int sum = 0;
    guint idx;

    if (inv == NULL)
    {
        return 0;
    }

    /* add contents weight */
    for (idx = 0; idx < inv_length(inv); idx++)
    {
        sum += item_weight(inv_get(inv, idx));
    }

    return sum;
}

int inv_item_count(inventory *inv, item_t type, guint32 id)
{
    int count = 0;
    guint idx;

    for (idx = 0; idx < inv_length(inv); idx++)
    {
        item *i = inv_get(inv, idx);
        if (id)
        {
            if (i->type == type && i->id == id)
            {
                count++;
            }
        }
        else if (i->type == type)
        {
            count++;
        }
    }

    return count;
}

int inv_length_filtered(inventory *inv, int (*filter)(item *))
{
    int count = 0;
    guint pos;

    if (inv == NULL)
    {
        /* check for non-existant inventories */
        return 0;
    }

    /* return the inventory length if no filter has been set */
    if (!filter)
    {
        return inv_length(inv);
    }

    for (pos = 0; pos < inv_length(inv); pos++)
    {
        item *i = inv_get(inv, pos);

        if (filter(i))
        {
            count++;
        }
    }

    return count;
}

item *inv_get_filtered(inventory *inv, guint idx, int (*filter)(item *))
{
    guint num;
    guint curr = 0;

    /* return the inventory length if no filter has been set */
    if (!filter)
    {
        return inv_get(inv, idx);
    }

    for (num = 0; num < inv_length(inv); num++)
    {
        item *i = inv_get(inv, num);

        if (filter(i))
        {
            /* filter matches */
            if (curr == idx)
            {
                /* this is the requested item */
                return i;
            }
            else
            {
                curr++;
            }
        }
    }

    /* not found */
    return NULL;
}

int inv_filter_container(item *it)
{
    assert (it != NULL);

    return (it->type == IT_CONTAINER);
}

int inv_filter_not_container(item *it)
{
    assert (it != NULL);

    return (it->type != IT_CONTAINER);
}

int inv_filter_food(item *it)
{
    assert (it != NULL);

    return (it->type == IT_FOOD);
}

int inv_filter_gems(item *it)
{
    assert (it != NULL);

    return (it->type == IT_GEM);
}

int inv_filter_not_gold(item *it)
{
    assert (it != NULL);

    return (it->type != IT_GOLD);
}

int inv_filter_potions(item *it)
{
    assert (it != NULL);

    return (it->type == IT_POTION);
}

int inv_filter_readable_items(item *it)
{
    assert (it != NULL);

    return (it->type == IT_SCROLL) || (it->type == IT_BOOK);
}

static void item_typename_pluralize(item *it, char *description, int length)
{
    char *replaced;

    assert(it != NULL && description != NULL && length > 0);

    replaced = str_replace(description,
                           item_data[it->type].name_sg,
                           item_data[it->type].name_pl);

    g_snprintf(description, length, "%s", replaced);
}

static char *item_name_count(char *name, char *add_info, int singular, int definite, int length, int count)
{
    char *incoming;

    assert(name != NULL && length > 0 && count > 0);

    /* clone current string */
    incoming = g_strdup(name);

    if ((count == 1) || singular)
    {
        if (add_info != NULL)
        {
            g_snprintf(name, length, "%s %s %s",
                       (definite ? "the" : a_an(add_info)),
                       add_info, incoming);
        }
        else
        {
            g_snprintf(name, length, "%s %s",
                       (definite ? "the" : a_an(incoming)),
                       incoming);
        }
    }
    else
    {
        if (add_info != NULL)
        {
            if (definite)
                g_snprintf(name, length, "%s %s", add_info, incoming);
            else
                g_snprintf(name, length, "%s %s %s", int2str(count),
                           add_info, incoming);
        }
        else
        {
            g_snprintf(name, length, "%s %s", int2str(count), incoming);
        }
    }

    g_free(incoming);

    return name;
}

static char *item_desc_get(item *it, int known)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_AMULET:
        if (known)
            return amulet_name(it);
        else
            return item_material_adjective(item_material(it));
        break;

    case IT_ARMOUR:
        return armour_name(it);
        break;

    case IT_BOOK:
        if (known)
            return book_name(it);
        else
            return book_desc(it->id);
        break;

    case IT_CONTAINER:
        return container_name(it);
        break;

    case IT_FOOD:
        return food_name(it);
        break;

    case IT_POTION:
        if (known)
            return potion_name(it);
        else
            return potion_desc(it->id);
        break;

    case IT_RING:
        if (known)
            return ring_name(it);
        else
            return item_material_adjective(item_material(it));
        break;

    case IT_SCROLL:
        if (known)
            return scroll_name(it);
        else
            return scroll_desc(it->id);
        break;

    case IT_GEM:
        return (char *)gem_name(it);
        break;

    case IT_WEAPON:
        return weapon_name(it);
        break;

    default:
        return "";
    }
}
