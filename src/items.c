/*
 * items.h
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
#include <string.h>

#include "amulets.h"
#include "armour.h"
#include "container.h"
#include "food.h"
#include "gems.h"
#include "items.h"
#include "level.h"
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
    /* item_t       name_sg      name_pl        IMG  desc_known      desc_unknown         max_id           eq us st */
    { IT_NONE,      "",          "",            ' ', "",             "",                  0,               0, 0, 0, },
    { IT_AMULET,    "amulet",    "amulets",     '"', "amulet of %s", "%s amulet",         AM_LARN,         1, 0, 0, },
    { IT_ARMOUR,    "armour",    "armour",      '[', "%s",           "%s",                AT_MAX,          1, 0, 0, },
    { IT_BOOK,      "book",      "books",       '+', "book of %s",   "%s book",           SP_MAX,          0, 1, 1, },
    { IT_CONTAINER, "container", "containers",  'C', "%s",           "%s",                CT_MAX,          0, 0, 0, },
    { IT_FOOD,      "food",      "foods",       '%', "%s",           "%s",                FT_MAX,          0, 1, 1, },
    { IT_GEM,       "gem",       "gems",        '*', "%s",           "%s gem",            GT_MAX,          0, 0, 1, },
    { IT_GOLD,      "coin",      "coins",       '$', "%s",           "%s",                0,               0, 0, 1, },
    { IT_POTION,    "potion",    "potions",     '!', "potion of %s", "%s potion",         PO_CURE_DIANTHR, 0, 1, 1, },
    { IT_RING,      "ring",      "rings",       '=', "ring of %s",   "%s ring",           RT_MAX,          1, 0, 0, },
    { IT_SCROLL,    "scroll",    "scrolls",     '?', "scroll of %s", "scroll labeled %s", ST_MAX,          0, 1, 1, },
    { IT_WEAPON,    "weapon",    "weapons",     '(', "%s",           "%s",                WT_MAX,          1, 0, 0, },
};

const item_material_data item_materials[IM_MAX] =
{
    /* type           name           adjective */
    { IM_NONE,        "",            "",          },
    { IM_PAPER,       "paper",       "papier",    },
    { IM_CLOTH,       "cloth",       "cloth",     },
    { IM_LEATHER,     "leather",     "leathern",  },
    { IM_WOOD,        "wood",        "wooden",    },
    { IM_BONE,        "bone",        "osseous",   },
    { IM_DRAGON_HIDE, "dragon hide", "scabby",    }, /* ? */
    { IM_IRON,        "iron",        "irony",     },
    { IM_STEEL,       "steel",       "steely",    },
    { IM_COPPER,      "copper",      "cupreous",  },
    { IM_SILVER,      "silver",      "silvery",   },
    { IM_GOLD,        "gold",        "golden",    },
    { IM_PLATINUM,    "platinum",    "platinum",  },
    { IM_MITHRIL,     "mitril",      "mithrial",  },
    { IM_GLASS,       "glass",       "vitreous",  },
    { IM_GEMSTONE,    "gemstone",    "gemstone",  }, /* ? */
};

/* static vars */

static int amulet_created[AM_MAX] = { 1, 0 };
static int weapon_created[WT_MAX] = { 1, 0 };

/* the potion of cure dianthroritis is a unique item */
static int cure_dianthr_created = FALSE;


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

    /* special item type specific attributes */
    switch (item_type)
    {
    case IT_AMULET:
        /* if the amulet has already been created try to create another one */
        while (amulet_created[nitem->id] && (loops < item_max_id(IT_AMULET)))
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

        amulet_created[nitem->id] = TRUE;

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
        if ((item_id == PO_CURE_DIANTHR) && cure_dianthr_created)
        {
            nitem->id = rand_1n(PO_SEE_INVISIBLE);
        }
        else if (item_id == PO_CURE_DIANTHR)
        {
            cure_dianthr_created = TRUE;
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
        while (weapon_is_unique(nitem) && weapon_created[nitem->id])
        {
            /* create another random weapon instead */
            nitem->id = rand_1n(WT_MAX);
        }

        if (weapon_is_unique(nitem))
        {
            /* mark unique weapon as created */
            weapon_created[nitem->id] = TRUE;
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

    assert (item_type > IT_NONE && item_type < IT_MAX && num_level < LEVEL_MAX);

    /* no amulets above level 8 */
    if ((item_type == IT_AMULET) && (num_level < 8))
        item_type = IT_RING;

    divisor = 1 / (float)(LEVEL_MAX - 1);

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
        /* no per-level randomnization */
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

item *item_clone(item *original)
{
    item *nitem;

    assert(original != NULL);

    /* clone item */
    nitem = g_malloc0(sizeof(item));
    memcpy(nitem, original, sizeof(item));

    return nitem;
}

item *item_split(item *original, guint32 count)
{
    item *nitem;

    assert(original != NULL && count < original->count);

    nitem = item_clone(original);

    nitem->count = count;
    original->count -= count;

    return nitem;
}

void item_destroy(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (it->effects)
    {
        while (it->effects->len)
        {
            effect_destroy(g_ptr_array_remove_index_fast(it->effects,
                           it->effects->len - 1));
        }

        g_ptr_array_free(it->effects, TRUE);
    }

    if (it->content != NULL)
    {
        inv_destroy(it->content);
    }

    g_free(it);
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

    result = (memcmp(a, b, sizeof(item)) == 0);

    b->count = tmp_count;

    return result;
}

int item_sort(gconstpointer a, gconstpointer b, gpointer data)
{
    gint order;
    item *item_a = *((item**)a);
    item *item_b = *((item**)b);
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

    assert((it != NULL) && (it->type > IT_NONE) && (it->type < IT_MAX));

    if (it->blessed && it->blessed_known)
    {
        add_info = "blessed";
    }
    else if (it->cursed && it->curse_known)
    {
        add_info = "cursed";
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
        g_snprintf(str, str_len, "%d carats %s",
                   gem_size(it), desc);

        item_name_count(str, add_info, singular, definite, str_len, it->count);
        break;

    case IT_WEAPON:
        if (weapon_is_unique(it))
        {
            g_snprintf(str, str_len, "%s%s",
                       (it->id == WT_BESSMAN || it->id == WT_SLAYER) ? "" : "the ",
                       item_desc_get(it, known));
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

void item_effect_add(item *i, effect *e)
{
    assert (i != NULL && e != NULL);

    /* create list if not existant */
    if (!i->effects)
    {
        i->effects = g_ptr_array_new();
    }

    /* this effect is permanent */
    e->turns = 0;

    /* link item to effect */
    e->item = i;

    /* add effect to list */
    effect_add(i->effects, e);
}

void item_effect_del(item *i, effect *e)
{
    assert (i != NULL && e != NULL);

    /* del effect from list */
    effect_del(i->effects, e);

    /*destroy effect */
    effect_destroy(e);

    /* delete array if no effect is left */
    if (!i->effects->len)
    {
        g_ptr_array_free(i->effects, TRUE);
        i->effects = NULL;
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

    if (it->blessed)
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
    it->curse_known = FALSE;

    return TRUE;
}

int item_enchant(item *it)
{
    guint pos;
    effect *e;

    assert(it != NULL);

    it->bonus++;

    if ((it->type == IT_RING) && it->effects)
    {
        for (pos = 0; pos < it->effects->len; pos++)
        {
            e = g_ptr_array_index(it->effects, pos);
            e->amount++;
        }
    }

    return it->bonus;
}

int item_disenchant(item *it)
{
    guint pos;
    effect *e;

    assert(it != NULL);

    it->bonus--;

    if ((it->type == IT_RING) && it->effects)
    {
        for (pos = 0; pos < it->effects->len; pos++)
        {
            e = g_ptr_array_index(it->effects, pos);
            e->amount--;
        }
    }

    return it->bonus;
}

int item_rust(item *it)
{
    assert(it != NULL);

    if ((item_material(it) == IM_IRON) || (item_material(it) == IM_STEEL))
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
        item_destroy(inv_del(&inv, inv_length(inv) - 1));
    }

    /* check if inventory has not yet been destroyed by inv_del */
    if (inv != NULL)
    {
        g_ptr_array_free(inv->content, TRUE);
        g_free(inv);
    }
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

int inv_add(inventory **inv, item *new_item)
{
    guint pos;
    item *i;

    assert(inv != NULL && item_new != NULL);

    /* create inventory if necessary */
    if (!(*inv))
    {
        *inv = inv_new(NULL);
    }

    /* call pre_add callback */
    if ((*inv)->pre_add)
    {
        if (!(*inv)->pre_add(*inv, new_item)) return FALSE;
    }

    if (item_is_stackable(new_item->type))
    {
        for (pos = 0; pos < inv_length(*inv); pos++)
        {
            i = (item *)inv_get(*inv, pos);
            if (item_compare(i, new_item))
            {
                /* just increase item count and release the original */
                i->count += new_item->count;
                item_destroy(new_item);

                return inv_length(*inv);
            }
        }
    }

    g_ptr_array_add((*inv)->content, new_item);

    /* call post_add callback */
    if ((*inv)->post_add)
    {
        (*inv)->post_add(*inv, new_item);
    }

    return inv_length(*inv);
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

    g_ptr_array_remove_index_fast((*inv)->content, idx);

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

int inv_del_element(inventory **inv, item *item)
{
    assert(*inv != NULL && (*inv)->content != NULL && item != NULL);

    if ((*inv)->pre_del)
    {
        if (!(*inv)->pre_del(*inv, item))
        {
            return FALSE;
        }
    }

    g_ptr_array_remove((*inv)->content, item);

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

    return TRUE;
}

void inv_sort(inventory *inv,GCompareDataFunc compare_func, gpointer user_data)
{
    assert(inv != NULL && inv->content != NULL);

    g_ptr_array_sort_with_data(inv->content, compare_func, user_data);
}

int inv_weight(inventory *inv)
{
    int sum = 0;
    guint idx;

    assert(inv != NULL);

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
    item *i;

    for (idx = 0; idx < inv_length(inv); idx++)
    {
        i = inv_get(inv, idx);
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
    item *i;

    /* return the inventory length if no filter has been set */
    if (!filter)
    {
        return inv_length(inv);
    }

    for (pos = 0; pos < inv_length(inv); pos++)
    {
        i = inv_get(inv, pos);

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
    item *i;

    /* return the inventory length if no filter has been set */
    if (!filter)
    {
        return inv_get(inv, idx);
    }

    for (num = 0; num < inv_length(inv); num++)
    {
        i = inv_get(inv, num);

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

    return (it->type == IT_GEM) || (it->type == IT_BOOK);
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
    const char *item_count_desc[] = { "two", "three", "four", "five",
                                      "six", "seven", "eight", "nine", "ten",
                                      "eleven", "twelve", "thirteen", "fourteen",
                                      "fivteen", "sixteen", "seventeen", "eighteen",
                                      "nineteen", "twenty"
                                    };
    char *incoming;

    assert(name != NULL && length > 0 && count > 0);

    /* clone current string */
    incoming = g_strdup(name);

    if ((count == 1) || singular)
    {
        if (definite)
        {
            if (add_info)
                g_snprintf(name, length, "the %s %s", add_info, incoming);
            else
                g_snprintf(name, length, "the %s", incoming);
        }
        else
        {
            if (add_info)
                g_snprintf(name, length, "a%s %s %s", a_an(incoming),
                           add_info, incoming);
            else
                g_snprintf(name, length, "a%s %s", a_an(incoming), incoming);
        }

        g_free(incoming);

        return name;
    }
    else if ((count > 1) && (count <= 20 ))
    {
        if (add_info)
        {
            if (definite)
                g_snprintf(name, length, "%s %s", add_info, incoming);
            else
                g_snprintf(name, length, "%s %s %s", item_count_desc[count - 2],
                           add_info, incoming);
        }
        else
        {
            g_snprintf(name, length, "%s %s", item_count_desc[count - 2], incoming);
        }
    }
    else
    {
        if (add_info)
            g_snprintf(name, length, "%d %s %s", count, add_info, incoming);
        else
            g_snprintf(name, length, "%d %s", count, incoming);
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
        return gem_name(it);
        break;

    case IT_WEAPON:
        return weapon_name(it);
        break;

    default:
        return "";
    }
}
