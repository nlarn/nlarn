/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * rings.h
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

#ifndef __RINGS_H_
#define __RINGS_H_

enum ring_types
{
    RT_NONE,
    RT_EXTRA_REGEN,
    RT_REGENERATION,
    RT_PROTECTION,
    RT_ENERGY,
    RT_DEXTERITY,
    RT_STRENGTH,
    RT_CLEVERNESS,
    RT_INC_DAMAGE,
    RT_MAX
};

typedef struct ring_data
{
    int id;
    char *name;
    int	effect_type; 	/* effect causes by this ring */
    int price;
    unsigned
        observable: 1;  /* can be identified by using */
} ring_data;

typedef struct ring
{
    int type;
    int bonus;
    effect *effect;				/* storage for effect */
    unsigned
        blessed: 1,
        cursed: 1;
} ring;

/* function definitions */

void ring_material_shuffle();
ring *ring_new(int ring_type, int bonus);
void ring_destroy(ring *r);

inline char *ring_get_name(ring *r);
inline char *ring_get_desc(ring *r);
inline int ring_get_effect(ring *r);
inline item_material_t ring_get_material(ring *r);
inline int ring_get_price(ring *r);
inline int ring_is_observable(ring *r);

int ring_bless(ring *r);
int ring_curse(ring *r);

#endif
