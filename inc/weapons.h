/*
 * weapons.h
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

#ifndef __WEAPONS_H_
#define __WEAPONS_H_

#include "items.h"

typedef enum _ammo_class
{
    AMMO_SLING,
    AMMO_BOW,
    AMMO_CROSSBOW,
    AMMO_MAX
} ammo_class;

typedef enum _ammo_t
{
    AMT_STONE,
    AMT_SBULLET,
    AMT_ARROW,
    AMT_BOLT,
    AMT_MAX
} ammo_t;

typedef struct _ammo_data
{
    ammo_t type;
    const char *name;
    ammo_class ac;
    int damage;
    int accuracy;
    int material;
    int weight;
    int price;
    unsigned
    obtainable: 1;
} ammo_data;

typedef enum _weapon_class
{
    WC_MELEE,   /* melee weapon */
    WC_RANGED,  /* ranged weapon */
    WC_MAX
} weapon_class;

typedef enum _weapon_t
{
    WT_ODAGGER,
    WT_DAGGER,
    WT_SLING,
    WT_OSHORTSWORD,
    WT_SHORTSWORD,
    WT_ESHORTSWORD,
    WT_OSPEAR,
    WT_SPEAR,
    WT_ESPEAR,
    WT_BOW,
    WT_CLUB,
    WT_MACE,
    WT_FLAIL,
    WT_BATTLEAXE,
    WT_CROSSBOW,
    WT_LONGSWORD,
    WT_ELONGSWORD,
    WT_2SWORD,
    WT_SWORDSLASHING,
    WT_LANCEOFDEATH,
    WT_VORPALBLADE,
    WT_SLAYER,
    WT_SUNSWORD,
    WT_BESSMAN,
    WT_MAX
} weapon_t;

typedef struct _weapon_data
{
    weapon_t type;
    const char *name;
    const char *short_name;
    weapon_class wc;    /* weapon class */
    ammo_class ammo;    /* required class of ammunition for ranged weapons */
    int damage;         /* weapon's base damage */
    int accuracy;       /* weapon accuracy */
    int material;       /* material type from item_materials */
    int weight;         /* used to determine inventory weight and if item can be thrown */
    int price;          /* base price in the shops */
    unsigned
        twohanded: 1,   /* needs two hands */
        unique: 1,      /* unique */
        article: 1,     /* needs an article in the description */
        obtainable: 1,  /* available in the shop */
        throwable: 1;   /* can be thrown */
} weapon_data;

/* forward declarations */
struct player;
struct _monster;

/* external vars */
extern const ammo_data ammos[AMT_MAX];
extern const char *ammo_class_name[AMMO_MAX];
extern const weapon_data weapons[WT_MAX];

/* functions */
int  weapon_fire(struct player *p);
void weapon_swap(struct player *p);

/*
 * @brief Return a shortened description of a given weapon
 *
 * @param a weapon
 * @return a newly allocated string that must be g_free()'d
 */
char *weapon_shortdesc(item *weapon, guint available_space);

/*
 * Retuns the percentual chance that the given weapon type
 * can instantly kill the given monster type.
 */
int weapon_instakill_chance(weapon_t wt, monster_t mt);

static inline int ammo_base_damage(item *ammo)
{
    g_assert(ammo->id < WT_MAX);
    return ammos[ammo->id].damage;
}

static inline int ammo_damage(item *ammo)
{
    int dmg = ammo_base_damage(ammo)
              + item_condition_bonus(ammo);

    /* even the worst ammo will not heal monsters */
    return max(0, dmg);
}

static inline int ammo_base_accuracy(item *ammo)
{
    g_assert(ammo->id < WT_MAX);
    return ammos[ammo->id].accuracy;
}

static inline int ammo_accuracy(item *ammo)
{
    return ammo_base_accuracy(ammo) + item_condition_bonus(ammo);
}

static inline int weapon_base_damage(item *weapon)
{
    g_assert(weapon->id < WT_MAX);
    return weapons[weapon->id].damage;
}

static inline int weapon_damage(item *weapon)
{
    int dmg = weapon_base_damage(weapon)
              + item_condition_bonus(weapon);

    /* even the worst weapon will not heal monsters */
    return max(1, dmg);
}

static inline int weapon_base_acc(item *weapon)
{
    g_assert(weapon->id < WT_MAX);
    return weapons[weapon->id].accuracy;
}

static inline int weapon_acc(item *weapon)
{
    return weapon_base_acc(weapon) + item_condition_bonus(weapon);
}

/* macros */
#define ammo_name(itm)          (ammos[(itm)->id].name)
#define ammo_class(itm)         (ammos[(itm)->id].ac)
#define ammo_material(itm)      (ammos[(itm)->id].material)
#define ammo_weight(itm)        (ammos[(itm)->id].weight)
#define ammo_price(itm)         (ammos[(itm)->id].price)
#define ammo_t_obtainable(atm)  (ammos[atm].obtainable)

#define weapon_type_obtainable(id)   (weapons[id].obtainable)
#define weapon_name(weapon)          (weapons[(weapon)->id].name)
#define weapon_short_name(weapon)    (weapons[(weapon)->id].short_name)
#define weapon_class(weapon)         (weapons[(weapon)->id].wc)
#define weapon_ammo(weapon)          (weapons[(weapon)->id].ammo)
#define weapon_material(weapon)      (weapons[(weapon)->id].material)
#define weapon_weight(weapon)        (weapons[(weapon)->id].weight)
#define weapon_price(weapon)         (weapons[(weapon)->id].price)
#define weapon_is_twohanded(weapon)  (weapons[(weapon)->id].twohanded)
#define weapon_is_ranged(weapon)     (weapons[(weapon)->id].wc == WC_RANGED)
#define weapon_is_unique(weapon)     (weapons[(weapon)->id].unique)
#define weapon_is_throwable(weapon)  (weapons[(weapon)->id].throwable)
#define weapon_needs_article(weapon) (weapons[(weapon)->id].article)

#endif
