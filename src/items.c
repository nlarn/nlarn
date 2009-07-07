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

#include "nlarn.h"

static void item_typename_pluralize(item *it, char *description, int length);
static char *item_name_count(char *name, int singular, int definite, int length, int count);
static char *item_desc_get(item *it, int known);

const item_type_data item_data[IT_MAX] =
{
    /* item_t       name_sg      name_pl    IMG  desc_known      desc_unknown         eq us st */
    { IT_NONE,      "",          "",        ' ', "",             "",                  0, 0, 0, },
    { IT_ARMOUR,    "armour",    "",        '[', "%s",           "%s",                1, 0, 0, },
    { IT_BOOK,      "book",      "books",   '+', "book of %s",   "%s book",           0, 1, 1, },
    { IT_CONTAINER, "container", "",        'C', "%s",           "%s",                0, 0, 0, },
    { IT_FOOD,      "food",      "foods",   '%', "%s",           "%s",                0, 1, 1, },
    { IT_GEM,       "gem",       "gems",    '*', "%s",           "%s gem",            0, 0, 1, },
    { IT_GOLD,      "coin",      "coins",   '$', "%s",           "%s",                0, 0, 1, },
    { IT_POTION,    "potion",    "potions", '!', "potion of %s", "%s potion",         0, 1, 1, },
    { IT_RING,      "ring",      "rings",   '=', "ring of %s",   "%s ring",           1, 0, 0, },
    { IT_SCROLL,    "scroll",    "scrolls", '?', "scroll of %s", "scroll labeled %s", 0, 1, 1, },
    { IT_WEAPON,    "weapon",    "",        '(', "%s",           "%s",                1, 0, 0, },
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

static int weapon_created[WT_MAX] = { 1, 0 };

/* the potion of cure dianthroritis is a unique item */
static int cure_dianthr_created = FALSE;


/* functions */

item *item_new(item_t item_type, int item_id, int item_bonus)
{
    item *nitem;
    effect *eff = NULL;

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
    case IT_CONTAINER:
        nitem->content = inv_new();
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
            /* this effect is permanent */
            eff->turns = 0;

            if (item_bonus)
                eff->amount += item_bonus;

            /* ring of extra regeneration is better than the average */
            if (item_id == RT_EXTRA_REGEN)
            {
                eff->amount *= 5;
            }
        }
        break;

    case IT_WEAPON:
        while (weapon_is_unique(nitem) && weapon_created[nitem->id])
        {
            /* create another random weapon instead */
            nitem->id = rand_1n(WT_MAX - 1);
        }

        if (weapon_is_unique(nitem))
        {
            /* mark unique weapon as created */
            weapon_created[nitem->id] = TRUE;
        }
        break;

    default:
        /* nop */
        break;
    }

    /* add effect to item if one has been created */
    if (eff)
    {
        nitem->effect = eff;
        eff->item = nitem;
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

item *item_split(item *original, int count)
{
    item *nitem;

    assert(original != NULL && count < original->count);

    nitem = item_clone(original);

    nitem->count = count;
    original->count -= count;

    return nitem;
}

item *item_create_random(item_t item_type)
{
    /* FIXME: use propabilities for each object type */
    int item_id = 0;
    int item_bonus = 0;
    int min_id = 0, max_id = 0;

    assert(item_type > IT_NONE && item_type < IT_MAX);

    /* select item_type */
    switch (item_type)
    {
    case IT_ARMOUR:
        min_id = AT_NONE;
        max_id = AT_MAX;
        item_bonus = rand_m_n(-4, 4);
        break;

    case IT_BOOK:
        min_id = SP_NONE;	/* Books derive from spells, therefore use spell IDs */
        max_id = SP_MAX;
        break;

    case IT_CONTAINER:
        min_id = CT_NONE;
        max_id = CT_CHEST; /* only bags and caskets */
        break;

    case IT_FOOD:
        min_id = FT_NONE;
        max_id = FT_MAX;
        break;

    case IT_GEM:
        min_id = GT_NONE;
        max_id = GT_MAX;
        break;

    case IT_GOLD:
        /* ID is amount in case of gold */
        min_id = 50;
        max_id = 250;
        break;

    case IT_POTION:
        min_id = PO_NONE;
        max_id = PO_CURE_DIANTHR;   /* prevent random potions of cure */
        break;

    case IT_RING:
        min_id = RT_NONE;
        max_id = RT_MAX;
        break;

    case IT_SCROLL:
        min_id = ST_NONE;
        max_id = ST_MAX;
        break;

    case IT_WEAPON:
        min_id = WT_NONE;
        max_id = WT_MAX;
        item_bonus = rand_m_n(-4, 4);
        break;

    default:
        /* nop */
        break;
    }
    item_id = rand_m_n(min_id + 1, max_id - 1);

    return item_new(item_type, item_id, item_bonus);
}

item *item_create_by_level(item_t item_type, int num_level)
{
    /* FIXME: really implement this */
    return(item_create_random(item_type));
}

void item_destroy(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    if (it->effect != NULL)
    {
        effect_destroy(it->effect);
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

int item_sort(gconstpointer a, gconstpointer b)
{
    gint order;
    item *item_a = *((item**)a);
    item *item_b = *((item**)b);

    if (item_a->type == item_b->type)
    {
        /* both items are of identical type. compare their names. */
        /* FIXME: need identified / unidentified status here */
        order = strcasecmp(item_desc_get(item_a, TRUE), item_desc_get(item_b, TRUE));
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
    char *temp;

    assert((it != NULL) && (it->type > IT_NONE) && (it->type < IT_MAX));

    switch (it->type)
    {
    case IT_ARMOUR:
        snprintf(desc, 60, "%s", item_desc_get(it, known));
        snprintf(str, str_len, "%s %+d",
                 item_name_count(desc, singular, definite, 60, it->count),
                 it->bonus);

        break;

    case IT_BOOK:
    case IT_POTION:
    case IT_RING:
    case IT_SCROLL:
        if (known)
            snprintf(desc, 60, "%s", item_data[it->type].desc_known);
        else
            if ((it->type == IT_SCROLL) && (it->id == ST_BLANK))
            {
                snprintf(desc, 60, "unlabeled scroll");
            }
            else
            {
                snprintf(desc, 60, "%s", item_data[it->type].desc_unknown);
            }

        if ((it->count > 1) && !singular)
            item_typename_pluralize(it, desc, 60);

        temp = g_strdup(desc);
        snprintf(desc, 60, temp, item_desc_get(it, known));
        g_free(temp);

        item_name_count(desc, singular, definite, 60, it->count);

        strncpy(str, desc, str_len);
        break;

    case IT_CONTAINER:
        snprintf(desc, 60, "%s", item_desc_get(it, known));
        item_name_count(desc, singular, definite, 60, it->count);
        snprintf(str, str_len, "%s", desc);
        break;

    case IT_FOOD:
        snprintf(desc, 60, "%s", item_desc_get(it, known));
        item_name_count(desc, singular, definite, 60, it->count);

        if ((it->count > 1) && !singular)
            snprintf(str, str_len, "%ss", desc);
        else
            snprintf(str, str_len, "%s", desc);

        break;

    case IT_GOLD:
        snprintf(str, str_len, "%d gold pieces", it->count);
        break;

    case IT_GEM:
        snprintf(desc, 60, "%s", item_desc_get(it, known));
        snprintf(str, str_len, "%d carats %s",
                 gem_size(it),
                 desc);

        item_name_count(str, singular, definite, str_len, it->count);
        break;

    case IT_WEAPON:
        if (weapon_is_unique(it))
            snprintf(str, str_len, "the %s",
                     item_desc_get(it, known));
        else
        {
            snprintf(desc, 60, "%s", item_desc_get(it, known));
            item_name_count(desc, singular, definite, 60, it->count);
            snprintf(str, str_len, "%s %+d",
                     desc, it->bonus);
        }
        break;

    default:
        /* nop */
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

int item_price(item *it)
{
    int price;

    assert (it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
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

    /* double price if blessed */
    if (it->blessed) price <<=1;

    /* half price if cursed */
    if (it->cursed) price >>=1;

    /* half price if corroded */
    if (it->corroded == 1) price >>=1;

    /* quarter price if very corroded */
    if (it->corroded == 1) price /= 4;

    /* half price if burnt */
    if (it->burnt == 1) price >>=1;

    /* quarter price if very burnt */
    if (it->burnt == 1) price /= 4;

    /* half price if rusty */
    if (it->rusty == 1) price >>=1;

    /* quarter price if very rusty */
    if (it->rusty == 1) price /= 4;

    if (price < 0) price = 0;

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

    /* FIXME: correct weight */
    switch (it->type)
    {
    case IT_ARMOUR:
        return armour_weight(it);
        break;

    case IT_BOOK:
        /* heavy tome */
        return 1500;
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
        /* FIXME: is this too heavy? is this too light?
           It should give the player a reason to use the bank.
         */
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
 * return the distinct effect cause by an item or NULL
 * @param an item
 * @return NULL or effect
 */
effect *item_effect(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
    case IT_ARMOUR:
        return NULL;
        break;

    case IT_RING:
        return ring_effect(it);
        break;

    case IT_WEAPON:
        return NULL;
        break;

    default:
        return NULL;
        break;
    }
}

int item_bless(item *it)
{
    if (it->blessed)
        return FALSE;

    it->blessed = 1;
    it->cursed = 0;

    return TRUE;
}

int item_curse(item *it)
{
    if (it->cursed)
        return FALSE;

    it->blessed = 0;
    it->cursed = 1;

    return TRUE;
}

int item_enchant(item *it)
{
    assert(it != NULL);

    it->bonus++;

    if (it->effect)
    {
        it->effect->amount++;
    }

    return it->bonus;
}

int item_disenchant(item *it)
{
    assert(it != NULL);

    it->bonus--;

    if (it->effect)
    {
        it->effect->amount--;
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

inventory *inv_new()
{
    return (inventory *)g_ptr_array_new();
}

int inv_add(inventory *inv, item *new_item)
{
    int pos;
    item *i;

    assert(inv != NULL && item_new != NULL);

    if (item_is_stackable(new_item->type))
    {
        for (pos = 1; pos <= inv->len; pos++)
        {
            i = (item *)g_ptr_array_index(inv, pos - 1);
            if (item_compare(i, new_item))
            {
                /* just increase item count and release the original */
                i->count += new_item->count;
                item_destroy(new_item);

                return inv->len;
            }
        }
    }

    g_ptr_array_add(inv, new_item);

    return inv->len;
}

void inv_destroy(inventory *inv)
{
    assert(inv != NULL);

    while (inv->len > 0)
        item_destroy(g_ptr_array_remove_index_fast(inv, inv->len - 1));

    g_ptr_array_free(inv, TRUE);
    inv = NULL;
}

/**
 * clean unused items from an inventory
 *
 */
int inv_clean(inventory *inv)
{
    item *it;
    int pos;
    int count = 0;

    assert(inv != NULL);

    for (pos = 1; pos <= inv_length(inv); pos++)
    {
        it = g_ptr_array_index(inv, pos - 1);
        if (it->count == 0)
        {
            g_ptr_array_remove_index(inv, pos - 1);
        }
    }

    return count;
}

int inv_weight(inventory *inv)
{
    int sum = 0;
    int pos;

    assert(inv != NULL);

    /* add contents weight */
    for (pos = 1; pos <= inv_length(inv); pos++)
    {
        sum += item_weight(inv_get(inv, pos - 1));
    }

    return sum;
}

int inv_item_count(inventory *inv, item_t type, int id)
{
    int count = 0;
    int pos;
    item *i;

    for (pos = 1; pos <= inv_length(inv); pos++)
    {
        i = inv_get(inv, pos - 1);
        if (id)
        {
            if (i->type == type && i->id == id)
                count++;
        }
        else if (i->type == type)
            count++;
    }

    return count;
}

int inv_length_filtered(inventory *inv, int (*filter)(item *))
{
    int count = 0;
    int pos;
    item *i;

    /* return the inventory length if no filter has been set */
    if (!filter)
    {
        return inv_length(inv);
    }

    for (pos = 1; pos <= inv_length(inv); pos++)
    {
        i = inv_get(inv, pos - 1);

        if (filter(i))
        {
            count++;
        }
    }

    return count;
}

item *inv_get_filtered(inventory *inv, int pos, int (*filter)(item *))
{
    int num;
    int curr = 0;
    item *i;

    /* return the inventory length if no filter has been set */
    if (!filter)
    {
        return inv_get(inv, pos);
    }

    for (num = 1; num <= inv_length(inv); num++)
    {
        i = inv_get(inv, num - 1);

        if (filter(i))
        {
            /* filter matches */
            if (curr == pos)
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

static void item_typename_pluralize(item *it, char *description, int length)
{
    char *replaced;

    assert(it != NULL && description != NULL && length > 0);

    replaced = str_replace(description,
                           item_data[it->type].name_sg,
                           item_data[it->type].name_pl);

    snprintf(description, length, "%s", replaced);
}

static char *item_name_count(char *name, int singular, int definite, int length, int count)
{
    const char *item_count_desc[] = { "two", "three", "four", "five",
                                      "six", "seven", "eight", "nine", "ten",
                                      "eleven", "twelve", "thirteen", "fourteen",
                                      "fivteen", "sixteen", "seventeen", "eighteen",
                                      "nineteen", "twenty"
                                    };
    const char vowels[] = "aeioAEIO";
    char *incoming;

    assert(name != NULL && length > 0 && count > 0);

    /* clone current string */
    incoming = g_strdup(name);

    if ((count == 1) || singular)
    {
        if (definite)
        {
            snprintf(name, length, "the %s", incoming);
        }
        else
        {
            if (strchr(vowels, incoming[0]))
            {
                snprintf(name, length, "an %s", incoming);
            }
            else
            {
                snprintf(name, length, "a %s", incoming);
            }
        }

        g_free(incoming);

        return name;
    }
    else if ((count > 1) && (count <= 20 ))
    {
        snprintf(name, length, "%s %s", item_count_desc[count - 2], incoming);
    }
    else
    {
        snprintf(name, length, "%d %s", count, incoming);
    }

    g_free(incoming);

    return name;
}

static char *item_desc_get(item *it, int known)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    switch (it->type)
    {
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
