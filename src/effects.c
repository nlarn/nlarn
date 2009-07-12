/*
 * effects.c
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

static const effect_data effects[ET_MAX] =
{
    /*
        type
        duration
        value
        msg_start
        msg_stop
        msg_start_monster
        msg_stop_monster
    */

    {
        ET_NONE,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_CHA,
        1,
        1,
        "You feel charismatic!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_CON,
        1,
        1,
        "You have a greater intestinal constitude!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_DEX,
        1,
        1,
        "You feel skilfully!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_INT,
        1,
        1,
        "You feel clever!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_STR,
        1,
        1,
        "Your muscles feel stronger!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_WIS,
        1,
        1,
        "You feel more self-confident!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_RND,
        1,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_DAMAGE,
        100,
        10,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_HP_MAX,
        1,
        5, /* percent */
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_MP_MAX,
        1,
        5, /* percent */
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_HP_REGEN,
        0,
        1,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_MP_REGEN,
        0,
        1,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_LEVEL,
        1,
        1,
        "You feel much more skillful!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_EXP,
        1,
        0,
        "You feel experienced.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_FIRE_RESISTANCE,
        1,
        10,
        "You feel a chill run up your spine!",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_COLD_RESISTANCE,
        1,
        10,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_MAGIC_RESISTANCE,
        1,
        10,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_PROTECTION,
        250,
        3,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_STEALTH,
        500,
        0,
        "You start to move stealthy.",
        "You do no longer move stealthy.",
        NULL,
        NULL,
    },

    {
        ET_AWARENESS,
        1800,
        3,
        "You become aware of your surroundings.",
        "You are no longer aware of your surroundings.",
        NULL,
        NULL,
    },

    {
        ET_SPEED,
        250,
        0,
        "You are suddenly moving much faster.",
        "You feel yourself slow down.",
        "The %s seems to move much faster.",
        "The %s suddenly slows down.",
    },

    {
        ET_HEROISM,
        250,
        5,
        "WOW!!! You feel Super-fantastic!!!",
        "You return to normal. How sad!",
        NULL,
        NULL,
    },

    {
        ET_INVISIBILITY,
        250,
        0,
        "Suddenly you can't see yourself!",
        "You are no longer invisible.",
        "The %s disappears.",
        NULL,
    },

    {
        ET_INVULNERABILITY,
        200,
        10,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INFRAVISION,
        250,
        0,
        "You feel your vision sharpen.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_ENLIGHTENMENT,
        250,
        12,
        "You have been granted enlightenment!",
        "You are no longer enlightened.",
        NULL,
        NULL,
    },

    {
        ET_DETECT_MONSTER,
        10,
        0,
        "You sense the presence of monsters.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_HOLD_MONSTER,
        30,
        0,
        NULL,
        NULL,
        "The %s seems to freeze.",
        "The %s can move again.",
    },

    {
        ET_SCARE_MONSTER,
        250,
        0,
        NULL,
        NULL,
        "The %s is very afraid.",
        "The %s is no longer scared.",
    },

    {
        ET_CHARM_MONSTER,
        250,
        0,
        NULL,
        NULL,
        "The %s is awestruck at your magnificence!",
        "The %s is no longer impressed.",
    },

    {
        ET_AGGRAVATE_MONSTER,
        800,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_HASTE_MONSTER,
        250,
        0,
        "You feel nervous.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_INC_HP,
        1,
        20, /* percent */
        "You feel better.",
        NULL,
        "The %s looks better.",
        NULL,
    },

    {
        ET_MAX_HP,
        1,
        0,
        "You feel completely healed.",
        NULL,
        "The %s looks completely healed.",
        NULL,
    },

    {
        ET_INC_MP,
        1,
        20, /* percent */
        "Magical energies course through your body.",
        NULL,
        "The %s seems to regain energy.",
        NULL,
    },

    {
        ET_MAX_MP,
        1,
        0,
        "You feel much more poweful.",
        NULL,
        "The %s looks much more powerful.",
        NULL,
    },

    {
        ET_CANCELLATION,
        250,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DIVINE_PROTECTION,
        250,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_UNDEAD_PROTECTION,
        400,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_SPIRIT_PROTECTION,
        400,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_TIMESTOP,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_WALL_WALK,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_LIFE_PROTECTION,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_NOTHEFT,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_CHA,
        1,
        1,
        "You feel rejected.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_CON,
        1,
        1,
        "You feel incapacitated.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_DEX,
        1,
        1,
        "You feel clumsy.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_INT,
        1,
        1,
        "You feel imbecile.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_STR,
        1,
        1,
        "You feel weaker.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_WIS,
        1,
        1,
        "You feel ignorant.",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_RND,
        1,
        1,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_DAMAGE,
        250,
        10,
        "You can't focus.",
        "You feel more focused.",
        NULL,
        NULL,
    },

    {
        ET_DEC_HP_MAX,
        1,
        5, /* percent */
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_MP_MAX,
        1,
        5, /* percent */
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_HP_REGEN,
        0,
        1,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_MP_REGEN,
        0,
        1,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_LEVEL,
        1,
        1,
        "You stagger for a moment...",
        NULL,
        NULL,
        NULL,
    },

    {
        ET_DEC_EXP,
        1,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_SLEEP,
        25,
        0,
        "You fall asleep.",
        "You wake up.",
        "The %s falls asleep.",
        "The %s wakes up.",
    },

    {
        ET_DIZZINESS,
        250,
        5,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_BLINDNESS,
        250,
        0,
        "You can't see anything!",
        "You feel your vision return to normal.",
        "The %s seems to be blinded.",
        "Looks like the %s can see again.",
    },

    {
        ET_CLUMSINESS,
        25,
        0,
        "You are unable to hold your weapon.",
        "You now feel less awkward.",
        NULL,
        NULL,
    },

    {
        ET_ITCHING,
        25,
        0,
        "The hysteria of itching forces you to remove your armour!",
        "You feel the irritation subside.",
        NULL,
        NULL,
    },

    {
        ET_CONFUSION,
        25,
        0,
        "You feel confused.",
        "You regain your senses.",
        "The %s looks confused.",
        "The %s seems to have regained it's senses.",
    },

    {
        ET_LAUGHTER,
        25,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_POISON,
        300,
        1,
        "You feel a sickness engulf you.",
        "You feel cured.",
        "The %s looks poisoned.",
        "The %s looks cured.",
    },

    {
        ET_AMNESIA,
        1,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {
        ET_SLOWNESS,
        250,
        0,
        "You feel yourself slow down.",
        "You are moving faster again.",
        "The %s slows down.",
        "The %s seems to move much faster.",
    },

};

effect *effect_new(int type, time_t now)
{
    effect *ne;

    assert(type > ET_NONE && type < ET_MAX);

    ne = g_malloc(sizeof(effect));
    ne->type = type;
    ne->start = now;
    ne->turns = (effects[type].duration <= 1) ? effects[type].duration : divert(effects[type].duration, 10);
    ne->amount = effects[type].value;

    return ne;
}

void effect_destroy(effect *e)
{
    assert(e != NULL);
    g_free(e);
}

char *effect_get_msg_start(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start;
}

char *effect_get_msg_stop(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop;
}

char *effect_get_msg_m_start(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start_monster;
}

char *effect_get_msg_m_stop(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop_monster;
}

int effect_get_amount(effect *e)
{
    if (e != NULL)
        return e->amount;
    else
        return 0;
}

void effect_add(GPtrArray *ea, effect *ne)
{
    effect *e;

    assert(ea != NULL && ne != NULL);

    /* check for duplicates */
    /* leave permanent effects alone */
    if ((e = effect_get(ea, ne->type)) && (e->turns > 1))
    {
        /* either add to effect's lifetime or power */
        if (effect_get_amount(ne) && chance(50))
        {
            e->amount += ne->amount;
        }
        else
        {
            e->turns += ne->turns;
        }

        effect_destroy(ne);
    }
    else
    {
        g_ptr_array_add(ea, ne);
    }
}

int effect_del(GPtrArray *a, effect *e)
{
    assert(a != NULL && e != NULL);
    return g_ptr_array_remove_fast(a,e);
}

effect *effect_get(GPtrArray *a, int effect_type)
{
    int pos;
    effect *e;

    assert(a != NULL && effect_type > ET_NONE && effect_type < ET_MAX);
    for (pos = 1; pos <=a->len; pos++)
    {
        e = g_ptr_array_index(a, pos - 1);
        if (e->type == effect_type)
            return e;
    }

    return NULL;
}


int effect_query(GPtrArray *a, int effect_type)
{
    int i;
    int value = 0;
    effect *e;

    assert(a != NULL && effect_type > ET_NONE && effect_type < ET_MAX);

    for (i = 1; i <= a->len; i++)
    {
        e = (effect *)g_ptr_array_index(a, i - 1);
        if (e->type == effect_type)
        {
            if (e->amount)
                value += e->amount;
            else
                return TRUE;
        }
    }

    return value;
}

/**
 * Count down the number of turns remaining for an effect.
 *
 * @param an active_effect
 * @return turns remaining. Expired effects return -1, permantent effects 0
 */
int effect_expire(effect *ae, int turns)
{
    assert(ae != NULL);

    if (ae->turns > 1)
    {
        ae->turns -= turns;
    }
    else if (ae->turns != 0)
    {
        ae->turns = -1;
    }

    return ae->turns;
}
