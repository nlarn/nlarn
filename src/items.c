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

static size_t item_get_size(int item_type);
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

const item_material_data item_material[IM_MAX] =
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

item *item_create_from_object(item_t item_type, void *object)
{
    item *nitem;

    assert(item_type > IT_NONE && item_type < IT_MAX && object != NULL);

    nitem = (item *)g_malloc(sizeof(item));

    nitem->type = item_type;
    nitem->item = object;
    nitem->count = 1;

    return(nitem);
}

item *item_new(item_t item_type, int item_id, int item_bonus)
{
    item *nitem;
    gpointer object = NULL;

    assert(item_type > IT_NONE && item_type < IT_MAX);

    nitem = g_malloc(sizeof(item));

    nitem->type = item_type;
    nitem->count = 1;

    switch (item_type)
    {
    case IT_ARMOUR:
        object = armour_new(item_id, item_bonus);
        break;

    case IT_BOOK:
        object = book_new(item_id);
        break;

    case IT_CONTAINER:
        object = container_new(item_id);
        break;

    case IT_FOOD:
        object = food_new(item_id);
        break;

    case IT_GEM:
        object = gem_new(item_id, item_bonus);
        break;

    case IT_GOLD:
        object = NULL;
        nitem->count = item_id;
        break;

    case IT_POTION:
        object = potion_new(item_id);
        break;

    case IT_RING:
        object = ring_new(item_id, item_bonus);
        break;

    case IT_SCROLL:
        object = scroll_new(item_id);
        break;

    case IT_WEAPON:
        object = weapon_new(item_id, item_bonus);
        break;

    default:
        /* nop */
        break;
    }

    nitem->item = object;

    return nitem;
}

item *item_clone(item *original)
{
    item *nitem;

    assert(original != NULL);

    /* clone item */
    nitem = g_malloc0(sizeof(item));

    nitem->type = original->type;
    nitem->count = original->count;

    /* clone content */
    nitem->item = g_malloc(item_get_size(original->type));

    memcpy(nitem->item,
           original->item,
           item_get_size(original->type));

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

    if (it->item != NULL)
    {
        switch (it->type)
        {
        case IT_ARMOUR:
            armour_destroy((armour *)it->item);
            break;

        case IT_BOOK:
            book_destroy((book *)it->item);
            break;

        case IT_CONTAINER:
            container_destroy((container *)it->item);
            break;

        case IT_FOOD:
            food_destroy((food *)it->item);
			break;

        case IT_GEM:
            gem_destroy((gem *)it->item);
            break;

        case IT_POTION:
            potion_destroy((potion *)it->item);
            break;

        case IT_RING:
            ring_destroy((ring *)it->item);
            break;

        case IT_SCROLL:
            scroll_destroy((magic_scroll *)it->item);
            break;

        case IT_WEAPON:
            weapon_destroy((weapon *)it->item);
            break;

        default:
            if (it->item != NULL)
                g_free(it->item);
        }
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
    if (a->type != b->type)
        return FALSE;
    else if (a->type == IT_GOLD)
        return TRUE;

    return(memcmp(a->item, b->item, item_get_size(a->type)) == 0);
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
                 ((armour *)it->item)->ac_bonus);

        break;

    case IT_BOOK:
    case IT_POTION:
    case IT_RING:
    case IT_SCROLL:
        if (known)
            snprintf(desc, 60, "%s", item_data[it->type].desc_known);
        else
            if ((it->type == IT_SCROLL)
                    && ((magic_scroll *)it->item)->type == ST_BLANK)
                snprintf(desc, 60, "unlabeled scroll");
            else
                snprintf(desc, 60, "%s", item_data[it->type].desc_unknown);

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
                 gem_get_size((gem *)it->item),
                 desc);

        item_name_count(str, singular, definite, str_len, it->count);
        break;

    case IT_WEAPON:
        if (weapon_is_uniqe((weapon *)it->item))
            snprintf(str, str_len, "the %s",
                     item_desc_get(it, known));
        else
        {
            snprintf(desc, 60, "%s", item_desc_get(it, known));
            item_name_count(desc, singular, definite, 60, it->count);
            snprintf(str, str_len, "%s %+d",
                     desc, ((weapon *)it->item)->wc_bonus);
        }
        break;

    default:
        /* nop */
        break;
    }

    return(str);
}

/**
 * Calculate the weight of an given object
 * @param an item
 * @return weight in grams
 */
int item_get_weight(item *it)
{
    assert(it != NULL && it->type > IT_NONE && it->type < IT_MAX);

    /* FIXME: correct weight */
    switch (it->type)
    {
    case IT_ARMOUR:
        return armour_get_weight((armour *)it->item);
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
        return container_get_weight((container *)it->item);
        break;

    case IT_FOOD:
        /* the fortune cookie is light */
        return 10;
        break;

    case IT_GOLD:
        /* FIXME: is this too heavy? is this too light?
           It should give the player a reason to use the bank.
         */
        return it->count * 4;
        break;

    case IT_GEM:
        return gem_get_weight((gem *)it->item);
        break;

    case IT_WEAPON:
        return weapon_get_weight((weapon *)it->item);
        break;

    default:
        return 0;
        break;
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

item *inv_find_object(inventory *inv, void *object)
{
    item *i;
    int pos;

    assert(inv != NULL && object != NULL);

    for (pos = 1; pos <= inv->len; pos++)
    {
        i = inv_get(inv, pos - 1);
        if (i->item == object)
        {
            return i;
        }
    }

    /* not found.. */
    return NULL;
}

static size_t item_get_size(int item_type)
{

    assert(item_type > IT_NONE && item_type < IT_MAX);

    switch (item_type)
    {
    case IT_ARMOUR:
        return sizeof(armour);
        break;

    case IT_BOOK:
        return sizeof(book);
        break;

    case IT_CONTAINER:
        return sizeof(container);
        break;

    case IT_FOOD:
        return sizeof(food);
        break;

    case IT_GEM:
        return sizeof(gem);
        break;

    case IT_GOLD:
        return 0;
        break;

    case IT_POTION:
        return sizeof(potion);
        break;

    case IT_RING:
        return sizeof(ring);
        break;

    case IT_SCROLL:
        return sizeof(magic_scroll);
        break;

    case IT_WEAPON:
        return sizeof(weapon);
        break;

    default:
        return 0;
    }
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
        return armour_get_name((armour *)it->item);
        break;

    case IT_BOOK:
        if (known)
            return book_get_name((book *)it->item);
        else
            return book_get_desc((book *)it->item);
        break;

    case IT_CONTAINER:
        return container_get_name((container *)it->item);
        break;

    case IT_FOOD:
        return food_get_name((food *)it->item);
        break;

    case IT_POTION:
        if (known)
            return potion_get_name((potion *)it->item);
        else
            return potion_get_desc((potion *)it->item);
        break;

    case IT_RING:
        if (known)
            return ring_get_name((ring *)it->item);
        else
            return item_material_adjective(ring_get_material((ring *)it->item));
        break;

    case IT_SCROLL:
        if (known)
            return scroll_get_name((magic_scroll *)it->item);
        else
            return scroll_get_desc((magic_scroll *)it->item);
        break;

    case IT_GEM:
        /* TODO: handle known / unknown */
        return gem_get_name((gem *)it->item);
        break;

    case IT_WEAPON:
        return weapon_get_name((weapon *)it->item);
        break;

    default:
        return "";
    }
}
