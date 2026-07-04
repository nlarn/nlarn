/*
 * effects.c
 * Copyright (C) 2009-2026 Joachim de Groot <jdegroot@web.de>
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
#include <string.h>

#include <glib/gi18n.h>

#include "cJSON.h"
#include "effects.h"
#include "game.h"
#include "extdefs.h"
#include "random.h"

DEFINE_ENUM(effect_t, EFFECT_TYPE_ENUM)

static const effect_data effects[ET_MAX] =
{
    /*
        name N_("name") duration amount desc
        msg_start
        msg_stop
        msg_start_monster
        msg_stop_monster
        var_duration var_amount inc_duration inc_amount
    */

    {
        ET_NONE, "ET_NONE", 0, 0, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_CON, "ET_INC_CON", 1, 1, NULL,
        N_("You have a greater intestinal constitution!"),
        NULL, NULL, NULL,
        false, false, false, true
    },

    {
        ET_INC_DEX, "ET_INC_DEX", 1, 1, NULL,
        N_("You feel skilful!"),
        N_("Your dextrousness returns to normal."),
        NULL, NULL,
        false, false, false, true
    },

    {
        ET_INC_INT, "ET_INC_INT", 1, 1, NULL,
        N_("You feel clever!"),
        N_("You are not so smart anymore."),
        NULL, NULL,
        false, false, false, true
    },

    {
        ET_INC_STR, "ET_INC_STR", 1, 1, NULL,
        N_("Your muscles are stronger!"),
        N_("Your strength returns to normal."),
        NULL, NULL,
        false, false, false, true
    },

    {
        ET_INC_WIS, "ET_INC_WIS", 1, 1, NULL,
        N_("You feel more self-confident!"),
        NULL, NULL, NULL,
        false, false, false, true
    },

    {
        ET_INC_RND, "ET_INC_RND", 1, 1, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_DAMAGE, "ET_INC_DAMAGE", 100, 10, NULL,
        NULL, NULL, NULL, NULL,
        true, true, false, true
    },

    {
        ET_INC_HP_MAX, "ET_INC_HP_MAX", 1, 5 /* % */, NULL,
        N_("You feel healthy!"),
        NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_MP_MAX, "ET_INC_MP_MAX", 1, 5 /* % */, NULL,
        N_("You feel energetic!"),
        NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_HP_REGEN, "ET_INC_HP_REGEN", 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_MP_REGEN, "ET_INC_MP_REGEN", 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_LEVEL, "ET_INC_LEVEL", 1, 1, NULL,
        N_("You feel much more skilful!"),
        NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_INC_EXP, "ET_INC_EXP", 1, 0, NULL,
        N_("You feel experienced."),
        NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_RESIST_FIRE, "ET_RESIST_FIRE", 0, 25, NULL,
        N_("You feel a chill run up your spine!"),
        NULL, NULL, NULL,
        false, false, false, true
    },

    {
        ET_RESIST_COLD, "ET_RESIST_COLD", 0, 25, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, true
    },

    {
        ET_RESIST_MAGIC, "ET_RESIST_MAGIC", 0, 25, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, true
    },

    {
        ET_PROTECTION, "ET_PROTECTION", 250, 3, N_("protected"),
        N_("You feel protected!"), N_("Your protection wanes."),
        NULL, NULL,
        true, false, true, false
    },

    {
        ET_STEALTH, "ET_STEALTH", 250, true, N_("stealthy"),
        N_("You start to move stealthily."),
        N_("You're not stealthy anymore."),
        NULL, NULL,
        true, false, true, false
    },

    {
        ET_AWARENESS, "ET_AWARENESS", 250, 3, N_("aware"),
        N_("You become aware of your surroundings."),
        N_("You are no longer aware of your surroundings."),
        NULL, NULL,
        true, false, true, false
    },

    {
        ET_SPEED, "ET_SPEED", 250, 25, N_("fast"),
        N_("You are suddenly moving much faster."),
        N_("You feel yourself slow down."),
        N_("%s seems to move much faster."),
        N_("%s suddenly slows down."),
        true, false, false, true
    },

    {
        ET_HEROISM, "ET_HEROISM", 250, 5, NULL,
        N_("WOW!!! You feel Super-fantastic!!!"),
        N_("You return to normal. How sad!"),
        N_("%s looks more perilous!"),
        N_("%s looks less perilous."),
        false, false, false, false
    },

    {
        ET_INVISIBILITY, "ET_INVISIBILITY", 250, true, N_("invisible"),
        N_("Suddenly you can't see yourself!"),
        N_("You are no longer invisible."),
        N_("%s disappears."),
        NULL,
        true, false, true, false
    },

    {
        ET_INVULNERABILITY, "ET_INVULNERABILITY", 250, 10, N_("invulnerable"),
        NULL, NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_INFRAVISION, "ET_INFRAVISION", 250, true, N_("infravision"),
        N_("Your vision sharpens."),
        N_("Your vision returns to normal."),
        N_("%s seems more observant."),
        N_("%s seems less observant."),
        true, false, true, false
    },

    {
        ET_ENLIGHTENMENT, "ET_ENLIGHTENMENT", 250, 8, N_("enlightened"),
        N_("You have been granted enlightenment!"),
        N_("You are no longer enlightened."),
        NULL, NULL,
        true, false, true, false
    },

    {
        ET_REFLECTION, "ET_REFLECTION", 400, true, N_("reflection"),
        NULL, NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_DETECT_MONSTER, "ET_DETECT_MONSTER", 10, true, NULL,
        N_("You sense the presence of monsters."),
        NULL, NULL, NULL,
        false, false, true, false
    },

    {
        ET_HOLD_MONSTER, "ET_HOLD_MONSTER", 30, true, N_("held"),
        NULL, NULL,
        N_("%s seems to freeze."),
        N_("%s can move again."),
        true, false, false, false
    },

    {
        ET_SCARED, "ET_SCARED", 250, true, N_("scared"),
        NULL, NULL,
        N_("%s is very afraid."),
        N_("%s is no longer scared."),
        true, false, true, false
    },

    {
        ET_CHARM_MONSTER, "ET_CHARM_MONSTER", 50, true, N_("charmed"),
        NULL, NULL,
        N_("%s is awestruck at your magnificence!"),
        N_("%s is no longer impressed."),
        true, false, true, false
    },

    {
        ET_INC_HP, "ET_INC_HP", 1, 20 /* % */, NULL,
        N_("You feel better."),
        NULL,
        N_("%s looks better."),
        NULL,
        false, true, false, false
    },

    {
        ET_MAX_HP, "ET_MAX_HP", 1, 100 /* % */, NULL,
        N_("You are completely healed."),
        NULL,
        N_("%s looks completely healed."),
        NULL,
        false, false, false, false
    },

    {
        ET_INC_MP, "ET_INC_MP", 1, 20 /* % */, NULL,
        N_("Magical energies course through your body."),
        NULL,
        N_("%s seems to regain energy."),
        NULL,
        false, true, false, false
    },

    {
        ET_MAX_MP, "ET_MAX_MP", 1, 100 /* % */, NULL,
        N_("You feel much more powerful."),
        NULL,
        N_("%s looks much more powerful."),
        NULL,
        false, false, false, false
    },

    {
        ET_CANCELLATION, "ET_CANCELLATION", 250, true, N_("cancellation"),
        NULL, NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_UNDEAD_PROTECTION, "ET_UNDEAD_PROTECTION", 400, true, N_("undead protection"),
        N_("You feel safe in the dark."),
        NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_SPIRIT_PROTECTION, "ET_SPIRIT_PROTECTION", 400, true, N_("spirit protection"),
        N_("You feel a protecting force."),
        NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_LIFE_PROTECTION, "ET_LIFE_PROTECTION", 2500, true, N_("life protection"),
        N_("You've never felt so safe."),
        N_("You feel less safe than before."),
        NULL, NULL,
        false, false, false, false
    },

    {
        ET_NOTHEFT, "ET_NOTHEFT", 400, true, N_("theft protection"),
        NULL, NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_SUSTAINMENT, "ET_SUSTAINMENT", 400, true, N_("sustainment"),
        NULL, NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_TIMESTOP, "ET_TIMESTOP", 20, true, N_("time stop"),
        NULL, NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_WALL_WALK, "ET_WALL_WALK", 20, true, N_("wall-walk"),
        N_("You can now walk through walls."),
        N_("You can no longer walk through walls."),
        NULL, NULL,
        true, false, true, false
    },

    {
        ET_LEVITATION, "ET_LEVITATION", 20, true, N_("levitation"),
        N_("You start to float in the air!"),
        N_("You gently sink to the ground."),
        N_("%s starts to float in the air!"),
        N_("%s gently sinks to the ground."),
        true, false, true, false
    },

    {
        ET_DEC_CON, "ET_DEC_CON", 1, 1, NULL,
        N_("You feel incapacitated."),
        N_("You feel tougher."),
        NULL, NULL,
        false, false, true, true
    },

    {
        ET_DEC_DEX, "ET_DEC_DEX", 1, 1, NULL,
        N_("You feel clumsy."),
        N_("Your dexterousness returns."),
        NULL, NULL,
        false, false, true, true
    },

    {
        ET_DEC_INT, "ET_DEC_INT", 1, 1, NULL,
        N_("You feel imbecile."),
        N_("Your intelligence returns."),
        NULL, NULL,
        false, false, true, true
    },

    {
        ET_DEC_STR, "ET_DEC_STR", 1, 1, NULL,
        N_("You are weaker."),
        N_("You regain your strength."),
        NULL, NULL,
        false, false, true, true
    },

    {
        ET_DEC_WIS, "ET_DEC_WIS", 1, 1, NULL,
        N_("You feel ignorant."),
        N_("You feel less ignorant."),
        NULL, NULL,
        false, false, true, true
    },

    {
        ET_DEC_RND, "ET_DEC_RND", 1, 1, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_AGGRAVATE_MONSTER, "ET_AGGRAVATE_MONSTER", 500, true, N_("aggravating"),
        N_("You sense rising anger."),
        NULL, NULL, NULL,
        true, false, true, false
    },

    {
        ET_SLEEP, "ET_SLEEP", 25, true, N_("sleeping"),
        N_("You fall asleep."),
        N_("You wake up."),
        N_("%s falls asleep."),
        N_("%s wakes up."),
        true, false, false, false
    },

    {
        ET_DIZZINESS, "ET_DIZZINESS", 250, 5, N_("dizzy"),
        N_("You're dizzy and weak!"),
        N_("You're no longer dizzy."),
        N_("%s looks dizzy and weak."),
        N_("%s no longer looks dizzy."),
        true, false, false, false
    },

    {
        ET_SICKNESS, "ET_SICKNESS", 250, 10, N_("sick"),
        N_("You feel a sickness coming on."),
        N_("You now feel better."),
        NULL, NULL,
        true, true, true, true
    },

    {
        ET_BLINDNESS, "ET_BLINDNESS", 250, true, N_("blind"),
        N_("You can't see anything!"),
        N_("The blindness lifts."),
        N_("%s seems to be blinded."),
        N_("Looks like %s can see again."),
        true, false, false, false
    },

    {
        ET_CLUMSINESS, "ET_CLUMSINESS", 250, true, N_("clumsy"),
        N_("You begin to lose hand to eye coordination!"),
        N_("You're less awkward now."),
        NULL, NULL,
        true, false, false, false
    },

    {
        ET_ITCHING, "ET_ITCHING", 100, true, N_("itching"),
        N_("You feel an irritation spread over your skin!"),
        N_("The irritation subsides."),
        NULL, NULL,
        true, false, false, false
    },

    {
        ET_CONFUSION, "ET_CONFUSION", 25, true, N_("confused"),
        N_("You are confused."),
        N_("You regain your senses."),
        N_("%s looks confused."),
        N_("%s seems to have regained it's senses."),
        true, false, false, false
    },

    {
        ET_PARALYSIS, "ET_PARALYSIS", 25, true, N_("paralysed"),
        N_("You are paralysed."),
        N_("You can move again."),
        NULL, NULL,
        true, false, false, false
    },

    {
        ET_POISON, "ET_POISON", 300, 1, N_("poisoned"),
        NULL, /* message is shown in player_damage_take */
        N_("You are cured."),
        N_("%s looks poisoned."),
        N_("%s looks cured."),
        true, false, true, true
    },

    {
        ET_AMNESIA, "ET_AMNESIA", 1, 0, NULL,
        NULL, NULL, NULL, NULL,
        false, false, false, false
    },

    {
        ET_SLOWNESS, "ET_SLOWNESS", 250, 25, N_("slow"),
        N_("You feel yourself slow down."),
        N_("You are moving faster again."),
        N_("%s slows down."),
        N_("%s seems to move much faster."),
        true, false, false, true
    },

    {
        ET_BURDENED, "ET_BURDENED", 0, 25, N_("burdened"),
        N_("You are burdened."),
        N_("You are no longer burdened."),
        NULL, NULL,
        false, false, false, false
    },

    {
        ET_OVERSTRAINED, "ET_OVERSTRAINED", 0, true, N_("overload"),
        N_("You are overloaded!"),
        N_("You are no longer overloaded."),
        NULL, NULL,
        false, false, false, false
    },

    {
        ET_TRAPPED, "ET_TRAPPED", 10, true, N_("trapped"),
        NULL,
        N_("You are no longer trapped!"),
        NULL,
        N_("%s climbs out of the pit!"),
        true, false, true, false
    },
};

effect *effect_new(effect_t type)
{
    g_assert(type > ET_NONE && type < ET_MAX);

    effect *ne = g_malloc0(sizeof(effect));
    ne->type = type;
    ne->start = game_turn(nlarn);

    /* determine effect duration */
    if (effects[type].var_duration)
    {
        ne->turns = divert(effects[type].duration, 10);
    }
    else
    {
        ne->turns = effects[type].duration;
    }

    /* determine effect amount */
    if (effects[type].var_amount)
    {
        ne->amount = divert(effects[type].amount, 10);
    }
    else
    {
        ne->amount = effects[type].amount;
    }

    /* register effect */
    ne->oid = game_effect_register(nlarn, ne);

    return ne;
}

effect *effect_copy(effect *e)
{
    g_assert(e != NULL);

    effect *ne = g_malloc(sizeof(effect));
    memcpy(ne, e, sizeof(effect));

    /* register copy with game */
    ne->oid = game_effect_register(nlarn, ne);

    return ne;
}

void effect_destroy(effect *e)
{
    g_assert(e != NULL);

    /* unregister effect */
    game_effect_unregister(nlarn, e->oid);

    g_free(e);
}

void effect_serialize(gpointer oid, effect *e, cJSON *root)
{
    cJSON *eval;

    cJSON_AddItemToArray(root, eval = cJSON_CreateObject());

    cJSON_AddNumberToObject(eval,"oid", GPOINTER_TO_UINT(oid));
    cJSON_AddStringToObject(eval,"type", effect_t_string(e->type));
    cJSON_AddNumberToObject(eval,"start", e->start);
    cJSON_AddNumberToObject(eval,"turns", e->turns);
    cJSON_AddNumberToObject(eval,"amount", e->amount);

    if (e->item)
    {
        cJSON_AddNumberToObject(eval,"item", GPOINTER_TO_UINT(e->item));
    }
}

effect *effect_deserialize(cJSON *eser, game *g)
{
    cJSON *itm;

    effect *e = g_malloc0(sizeof(effect));

    guint oid = cJSON_GetObjectItem(eser, "oid")->valueint;
    e->oid =  GUINT_TO_POINTER(oid);

    e->type = effect_t_value(cJSON_GetObjectItem(eser, "type")->valuestring);
    e->start = cJSON_GetObjectItem(eser, "start")->valueint;
    e->turns = cJSON_GetObjectItem(eser, "turns")->valueint;
    e->amount = cJSON_GetObjectItem(eser, "amount")->valueint;

    if ((itm = cJSON_GetObjectItem(eser, "item")))
    {
        e->item = GUINT_TO_POINTER(itm->valueint);
    }

    /* add effect to game */
    g_hash_table_insert(g->effects, e->oid, e);

    /* increase max_id to match used ids */
    if (g->effect_max_id < oid)
    {
        g->effect_max_id = oid;
    }

    return e;
}

cJSON *effects_serialize(GPtrArray *effs)
{
    cJSON *eser = cJSON_CreateArray();

    for (guint idx = 0; idx < effs->len; idx++)
    {
        gpointer eff_oid = g_ptr_array_index(effs, idx);
        cJSON_AddItemToArray(eser, cJSON_CreateNumber(GPOINTER_TO_UINT(eff_oid)));
    }

    return eser;
}

GPtrArray *effects_deserialize(cJSON *eser)
{
    GPtrArray *effs = g_ptr_array_new();

    for (int idx = 0; idx < cJSON_GetArraySize(eser); idx++)
    {
        cJSON *effser = cJSON_GetArrayItem(eser, idx);
        guint oid = effser->valueint;
        g_ptr_array_add(effs, GUINT_TO_POINTER(oid));
    }

    return effs;
}

const char *effect_type_name(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].name;
}

int effect_type_amount(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].amount;
}

guint effect_type_duration(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].duration;
}

bool effect_type_inc_duration(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].inc_duration;
}

bool effect_type_inc_amount(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].inc_amount;
}

const char *effect_get_desc(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].desc ? _(effects[e->type].desc) : NULL;
}

const char *effect_get_msg_start(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start ? _(effects[e->type].msg_start) : NULL;
}

const char *effect_get_msg_stop(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop ? _(effects[e->type].msg_stop) : NULL;
}

const char *effect_get_msg_m_start(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start_monster ? _(effects[e->type].msg_start_monster) : NULL;
}

const char *effect_get_msg_m_stop(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop_monster ? _(effects[e->type].msg_stop_monster) : NULL;
}

int effect_get_amount(effect *e)
{
    g_assert (e != NULL);
    return e->amount;
}

effect *effect_add(GPtrArray *ea, effect *ne)
{
    effect *e;

    g_assert(ea != NULL && ne != NULL);

    /* check for existing effects unless the effect belongs to an item */
    if (!ne->item && (e = effect_get(ea, ne->type)))
    {
        bool modified_existing = false;

        /* if the effect's duration can be extended, reset it */
        if (effects[e->type].inc_duration)
        {
            e->turns = max(e->turns, ne->turns);
            modified_existing = true;
        }

        /* if the effect's amount can be extended, do so */
        if (effects[e->type].inc_amount)
        {
            e->amount += ne->amount;
            modified_existing = true;
        }

        effect_destroy(ne);

        if (modified_existing == true)
        {
            return e;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        g_ptr_array_add(ea, ne->oid);
        return ne;
    }
}

int effect_del(GPtrArray *ea, effect *e)
{
    g_assert(ea != NULL && e != NULL);
    return g_ptr_array_remove_fast(ea, e->oid);
}

effect *effect_get(GPtrArray *ea, effect_t type)
{
    g_assert(ea != NULL && type > ET_NONE && type < ET_MAX);

    for (guint idx = 0; idx < ea->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(ea, idx);
        effect *e = game_effect_get(nlarn, effect_id);

        /* do not return effects caused by items */
        if ((e->type == type) && (e->item == NULL))
        {
            return e;
        }
    }

    return NULL;
}

int effect_query(GPtrArray *ea, effect_t type)
{
    int amount = 0;

    g_assert(ea != NULL && type > ET_NONE && type < ET_MAX);

    for (guint idx = 0; idx < ea->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(ea, idx);
        effect *e = game_effect_get(nlarn, effect_id);

        if (e->type == type) amount += e->amount;
    }

    return amount;
}

int effect_expire(effect *e)
{
    g_assert(e != NULL);

    if (e->turns > 1)
    {
        e->turns--;
    }
    else if (e->turns != 0)
    {
        e->turns = -1;
    }

    return e->turns;
}
