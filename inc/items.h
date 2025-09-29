/*
 * items.h
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

#ifndef ITEM_H
#define ITEM_H

#include <glib.h>

#include "colours.h"
#include "effects.h"
#include "enumFactory.h"

#define ITEM_TYPE_ENUM(ITEM_TYPE) \
    ITEM_TYPE(IT_NONE,) \
    ITEM_TYPE(IT_AMULET,)    /* amulet, defined in amulets.h */ \
    ITEM_TYPE(IT_AMMO,)      /* ammunition, defined in weapons.h */ \
    ITEM_TYPE(IT_ARMOUR,)    /* armour, defined in armour.h */ \
    ITEM_TYPE(IT_BOOK,)      /* book, defined in spells.h */ \
    ITEM_TYPE(IT_CONTAINER,) /* container, defined in container.h */ \
    ITEM_TYPE(IT_GEM,)       /* gem, defined in gems.h */ \
    ITEM_TYPE(IT_GOLD,)      /* just gold. defined nowhere as type and count are sufficient. */ \
    ITEM_TYPE(IT_POTION,)    /* potion, defined in potions.h */ \
    ITEM_TYPE(IT_RING,)      /* ring, defined in rings.h */ \
    ITEM_TYPE(IT_SCROLL,)    /* magic_scroll, defined in scrolls.h */ \
    ITEM_TYPE(IT_WEAPON,)    /* weapon, defined in weapons.h */ \
    ITEM_TYPE(IT_MAX,)       /* ~ item type count */ \
    ITEM_TYPE(IT_ALL,)       /* for special uses: all item types */

DECLARE_ENUM(item_t, ITEM_TYPE_ENUM)

/* inspired by Nethack's objclass.h */
typedef enum item_material_t {
    IM_PAPER,
    IM_CLOTH,
    IM_LEATHER,
    IM_WOOD,
    IM_BONE,
    /* materials up to here can burn */
    IM_DRAGON_HIDE,     /* not leather! */
    IM_LEAD,
    IM_IRON,            /* Fe, can rust/corrode */
    IM_STEEL,           /* stainless steel */
    IM_COPPER,          /* Cu - includes brass */
    IM_SILVER,
    IM_GOLD,            /* Au */
    IM_PLATINUM,        /* Pt */
    IM_MITHRIL,
    IM_GLASS,
    IM_STONE,
    IM_GEMSTONE,
    IM_MAX              /* ~ item material count */
} item_material_t;

typedef struct item_material_data {
    item_material_t type;
    const char *name;
    const char *adjective;
    colour fg;
    guint fragility;
} item_material_data;

typedef struct item_usage_result
{
    int identified;
    int used_up;
} item_usage_result;

typedef enum item_erosion_t {
    IET_NONE,
    IET_BURN,
    IET_CORRODE,
    IET_RUST,
    IET_MAX
} item_erosion_type;

struct game;
struct _inventory;

typedef struct _item {
    gpointer oid;           /* item's game object id */
    item_t type;            /* element type */
    guint32 id;             /* item id, type specific */
    gint32 bonus;
    guint32 count;          /* for stackable items */
    GPtrArray *effects;     /* storage for effects */
    struct _inventory *content;     /* for containers */
    char *notes;            /* storage for player's notes about the item */
    guint32
        blessed: 1,
        cursed: 1,
        corroded: 2,        /* 0: no; 1: yes; 2: very */
        burnt: 2,           /* 0: no; 1: yes; 2: very */
        rusty: 2,           /* 0: no; 1: yes; 2: very */
        blessed_known: 1,   /* player known if item is cursed / blessed */
        bonus_known: 1,     /* player knows the bonus */
        fired: 1,           /* player has fired the item */
        picked_up: 1;       /* picked up by monster */
} item;

typedef struct item_type_data {
    item_t id;
    const char *name_sg;
    const char *name_pl;
    const char glyph;
    guint max_id;
    bool
        optimizable: 1,     /* item type can have a bonus */
        blessable: 1,       /* item type can be blessed / cursed */
        corrodible: 1,      /* item type can corrode */
        equippable: 1,
        usable: 1,
        stackable: 1,
        identifyable: 1,
        desirable: 1;       /* items of type shall be generated as monster loot */
} item_type_data;

/* function definitions */

item *item_new(item_t item_type, int item_id);
item *item_new_random(item_t item_type, gboolean finetouch);
item *item_new_by_level(item_t item_type, int num_level);
item *item_new_finetouch(item *it);
item *item_copy(item *original);
item *item_split(item *original, guint32 count);
void item_destroy(item *it);

void item_serialize(gpointer oid, gpointer it, gpointer root);
item *item_deserialize(cJSON *iser, struct game *g);

/**
 * Compare two items.
 *
 * @param a item1
 * @param b item2
 * @return true if items are identical
 */
int item_compare(item *a, item *b);

int item_sort(gconstpointer a, gconstpointer b, gpointer data, gboolean force_id);

/**
 * Describe an item.
 *
 * @param it the item
 * @param known true if the item is known to the player.
 * @param singular true if the item count shall be ignored and the description
 *        for a single item of a stack shall be returned.
 * @param definite true if the description shall be prepended by the definite
 *        article.
 * @return a newly allocated string that should be disposed with g_free().
 */
gchar *item_describe(item *it, gboolean known, gboolean singular, gboolean definite);

item_material_t item_material(item *it);
guint item_base_price(item *it);
guint item_price(item *it);

/**
 * Calculate the weight of the given item.
 *
 * @param it an item
 * @return weight in grams
 */
int item_weight(item *it);

/**
 * Determine the colour of the given object.
 *
 * @param it an item
 * @return the coulour
 */
colour item_colour(item *it);


/*
 * @brief Determine the chance if an item breaks when exposed to force.
 *
 * @param it an item
 * @return an integer between 0 and 100.
 */
guint item_fragility(item *it);

void item_effect_add(item *it, effect *e);

int item_bless(item *it);
int item_curse(item *it);
int item_remove_curse(item *it);

item *item_enchant(item *it);
item *item_disenchant(item *it);

/**
 * Erode an item.
 *
 * @param  inv the inventory the item is in (may be null for new items)
 * @param  it the item to erode
 * @param  iet the type of erosion which affects the item
 * @param  visible true if the player can see the item
 * @return the item, NULL if the item has been destroyed
 *
 */
item *item_erode(struct _inventory **inv, item *it, item_erosion_type iet, gboolean visible);

int item_obtainable(item_t type, int id);

/**
 * @brief Describe an item thoroughly.
 *
 * @param it An item.
 * @param known if the item is known
 * @param shop show the item price
 * @return A newly allocated string that must be freed in the calling function.
 */
char *item_detailed_description(item *it, gboolean known, gboolean shop);

/* external vars */
extern const item_type_data item_data[IT_MAX];
extern const item_material_data item_materials[IM_MAX];

static inline int item_condition_bonus(const item *it)
{
    g_assert(it->type < IT_MAX);

    /* sum item bonus or malus and the general condition */
    int bonus = it->bonus;
    bonus -= it->rusty;
    bonus -= it->burnt;
    bonus -= it->corroded;

    return bonus;
}

/* item macros */
#define item_glyph(type)          (item_data[(type)].glyph)
#define item_name_sg(type)        (item_data[(type)].name_sg)
#define item_name_pl(type)        (item_data[(type)].name_pl)
#define item_max_id(type)         (item_data[(type)].max_id)
#define item_is_optimizable(type)     (item_data[(type)].optimizable)
#define item_is_blessable(type)       (item_data[(type)].blessable)
#define item_is_corrodible(type)      (item_data[(type)].corrodible)
#define item_is_equippable(type)      (item_data[(type)].equippable)
#define item_is_usable(type)          (item_data[(type)].usable)
#define item_is_stackable(type)       (item_data[(type)].stackable)
#define item_is_identifyable(type)    (item_data[(type)].identifyable)
#define item_is_desirable(type)       (item_data[(type)].desirable)
#define item_material_name(type)      (item_materials[(type)].name)
#define item_material_adjective(type) (item_materials[(type)].adjective)

/* item filters */

int item_filter_container(item *it);
int item_filter_gems(item *it);
int item_filter_gold(item *it);
int item_filter_not_gold(item *it);
int item_filter_potions(item *it);
int item_filter_legible(item *it);
int item_filter_unid(item *it);
int item_filter_cursed(item *it);
int item_filter_cursed_or_unknown(item *it);
int item_filter_nonblessed(item *it);

static inline int item_filter_weapon(item *it)
{
    return (IT_WEAPON == it->type);
}

/**
 * @brief Item filter function for the potion of cure dianthroritis.
 * @param it a pointer to an item
 * @return true if the supplied item is the potion of cure dianthroritis
 */
int item_filter_pcd(item *it);

/**
 * @brief Item filter function for blank scrolls.
 * @param it a pointer to an item
 * @return true if the supplied item is a blank scroll
 */
int item_filter_blank_scroll(item *it);

/**
 * @brief Check if an item is unique.
 *
 * @param it A pointer to an item.
 * @return true if the item is unique.
 */
gboolean item_is_unique(item *it);

#endif
