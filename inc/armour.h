/*
 * armour.h
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

#ifndef __ARMOUR_H_
#define __ARMOUR_H_

#include "effects.h"
#include "items.h"
#include "utils.h"

typedef enum _armour_class
{
    AC_BOOTS,
    AC_CLOAK,
    AC_GLOVES,
    AC_HELMET,
    AC_SHIELD,
    AC_SUIT,
    AC_MAX
} armour_class;


#define ARMOUR_TYPE_ENUM(ARMOUR_TYPE) \
    ARMOUR_TYPE(AT_CLOAK,) \
    ARMOUR_TYPE(AT_LGLOVES,) \
    ARMOUR_TYPE(AT_LBOOTS,) \
    ARMOUR_TYPE(AT_LHELMET,) \
    ARMOUR_TYPE(AT_LEATHER,) \
    ARMOUR_TYPE(AT_WSHIELD,) \
    ARMOUR_TYPE(AT_SLEATHER,) \
    ARMOUR_TYPE(AT_RINGMAIL,) \
    ARMOUR_TYPE(AT_LSHIELD,) \
    ARMOUR_TYPE(AT_CHAINHOOD,) \
    ARMOUR_TYPE(AT_CHAINMAIL,) \
    ARMOUR_TYPE(AT_SPLINTMAIL,) \
    ARMOUR_TYPE(AT_PHELMET,) \
    ARMOUR_TYPE(AT_PBOOTS,) \
    ARMOUR_TYPE(AT_SPEEDBOOTS,) \
    ARMOUR_TYPE(AT_PLATEMAIL,) \
    ARMOUR_TYPE(AT_SSHIELD,) \
    ARMOUR_TYPE(AT_SPLATEMAIL,) \
    ARMOUR_TYPE(AT_INVISCLOAK,) \
    ARMOUR_TYPE(AT_ELVENCHAIN,) \
    ARMOUR_TYPE(AT_MAX,)

DECLARE_ENUM(armour_t, ARMOUR_TYPE_ENUM)

typedef struct _armour_data
{
    armour_t id;
    const char *name;
    int ac;
    armour_class category;
    item_material_t material;
    effect_t et;        /* for uniques: effect on the item */
    int weight;         /* used to determine inventory weight */
    int price;          /* base price in the shops */
    armour_t disguise;  /* item used for description until armour type is identified */
    bool
        obtainable: 1,  /* available in the shop? */
        unique: 1;      /* generated only once */
} armour_data;

/* external vars */

extern const armour_data armours[AT_MAX];

/* inline functions */
static inline effect_t armour_effect(const item *armour)
{
    g_assert(armour->id < AT_MAX);
    return armours[armour->id].et;
}

static inline armour_t armour_disguise(const item *armour)
{
    g_assert(armour->id < AT_MAX);
    return armours[armour->id].disguise;
}

static inline gboolean armour_unique(const item *armour)
{
    g_assert(armour->id < AT_MAX);
    return armours[armour->id].unique;
}

static inline guint armour_base_ac(const item *armour)
{
    g_assert(armour->id < AT_MAX);
    return armours[armour->id].ac;
}

static inline guint armour_ac(const item *armour)
{
    const guint ac = armour_base_ac(armour)
             + item_condition_bonus(armour);

    return (guint)max(ac, 0);
}

/* macros */

#define armour_name(armour)     (armours[(armour)->id].name)
#define armour_class(armour)    (armours[(armour)->id].category)
#define armour_material(armour) (armours[(armour)->id].material)
#define armour_weight(armour)   (armours[(armour)->id].weight)
#define armour_price(armour)    (armours[(armour)->id].price)

#endif
