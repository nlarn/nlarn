/*
 * effects.c
 * Copyright (C) 2009-2011, 2012 Joachim de Groot <jdegroot@web.de>
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

#include "cJSON.h"
#include "effects.h"
#include "game.h"
#include "nlarn.h"
#include "random.h"

static const effect_data effects[ET_MAX] =
{
    /*
        name "name" duration amount desc
        msg_start
        msg_stop
        msg_start_monster
        msg_stop_monster
        var_duration var_amount inc_duration inc_amount
    */

    {
        ET_NONE, "ET_NONE", 0, 0, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_CON, "ET_INC_CON", 1, 1, NULL,
        "You have a greater intestinal constitution!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_INC_DEX, "ET_INC_DEX", 1, 1, NULL,
        "You feel skilful!",
        "Your dextrousness returns to normal.",
        NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_INC_INT, "ET_INC_INT", 1, 1, NULL,
        "You feel clever!",
        "You are not so smart anymore.",
        NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_INC_STR, "ET_INC_STR", 1, 1, NULL,
        "Your muscles are stronger!",
        "Your strength returns to normal.",
        NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_INC_WIS, "ET_INC_WIS", 1, 1, NULL,
        "You feel more self-confident!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_INC_RND, "ET_INC_RND", 1, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_DAMAGE, "ET_INC_DAMAGE", 100, 10, NULL,
        NULL, NULL, NULL, NULL,
        TRUE, TRUE, FALSE, TRUE
    },

    {
        ET_INC_HP_MAX, "ET_INC_HP_MAX", 1, 5 /* % */, NULL,
        "You feel healthy!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_MP_MAX, "ET_INC_MP_MAX", 1, 5 /* % */, NULL,
        "You feel energetic!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_HP_REGEN, "ET_INC_HP_REGEN", 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_MP_REGEN, "ET_INC_MP_REGEN", 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_LEVEL, "ET_INC_LEVEL", 1, 1, NULL,
        "You feel much more skilful!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_EXP, "ET_INC_EXP", 1, 0, NULL,
        "You feel experienced.",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_RESIST_FIRE, "ET_RESIST_FIRE", 0, 25, NULL,
        "You feel a chill run up your spine!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_RESIST_COLD, "ET_RESIST_COLD", 0, 25, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_RESIST_MAGIC, "ET_RESIST_MAGIC", 0, 25, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_PROTECTION, "ET_PROTECTION", 250, 3, "protected",
        "You feel protected!", "Your protection wanes.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_STEALTH, "ET_STEALTH", 250, TRUE, "stealthy",
        "You start to move stealthily.",
        "You're not stealthy anymore.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_AWARENESS, "ET_AWARENESS", 250, 3, "aware",
        "You become aware of your surroundings.",
        "You are no longer aware of your surroundings.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_SPEED, "ET_SPEED", 250, 25, "fast",
        "You are suddenly moving much faster.",
        "You feel yourself slow down.",
        "The %s seems to move much faster.",
        "The %s suddenly slows down.",
        TRUE, FALSE, FALSE, TRUE
    },

    {
        ET_HEROISM, "ET_HEROISM", 250, 5, NULL,
        "WOW!!! You feel Super-fantastic!!!",
        "You return to normal. How sad!",
        "The %s looks more perilous!",
        "The %s looks less perilous.",
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INVISIBILITY, "ET_INVISIBILITY", 250, TRUE, "invisible",
        "Suddenly you can't see yourself!",
        "You are no longer invisible.",
        "The %s disappears.",
        NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_INVULNERABILITY, "ET_INVULNERABILITY", 250, 10, "invulnerable",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_INFRAVISION, "ET_INFRAVISION", 250, TRUE, "infravision",
        "Your vision sharpens.",
        "Your vision returns to normal.",
        "The %s seems more observant.",
        "The %s seems less observant.",
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_ENLIGHTENMENT, "ET_ENLIGHTENMENT", 250, 8, "enlightened",
        "You have been granted enlightenment!",
        "You are no longer enlightened.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_REFLECTION, "ET_REFLECTION", 400, TRUE, "reflection",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_DETECT_MONSTER, "ET_DETECT_MONSTER", 10, TRUE, NULL,
        "You sense the presence of monsters.",
        NULL, NULL, NULL,
        FALSE, FALSE, TRUE, FALSE
    },

    {
        ET_HOLD_MONSTER, "ET_HOLD_MONSTER", 30, TRUE, "held",
        NULL, NULL,
        "The %s seems to freeze.",
        "The %s can move again.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_SCARED, "ET_SCARED", 250, TRUE, "scared",
        NULL, NULL,
        "The %s is very afraid.",
        "The %s is no longer scared.",
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_CHARM_MONSTER, "ET_CHARM_MONSTER", 250, TRUE, "charmed",
        NULL, NULL,
        "The %s is awestruck at your magnificence!",
        "The %s is no longer impressed.",
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_INC_HP, "ET_INC_HP", 1, 20 /* % */, NULL,
        "You feel better.",
        NULL,
        "The %s looks better.",
        NULL,
        FALSE, TRUE, FALSE, FALSE
    },

    {
        ET_MAX_HP, "ET_MAX_HP", 1, 100 /* % */, NULL,
        "You are completely healed.",
        NULL,
        "The %s looks completely healed.",
        NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_MP, "ET_INC_MP", 1, 20 /* % */, NULL,
        "Magical energies course through your body.",
        NULL,
        "The %s seems to regain energy.",
        NULL,
        FALSE, TRUE, FALSE, FALSE
    },

    {
        ET_MAX_MP, "ET_MAX_MP", 1, 100 /* % */, NULL,
        "You feel much more powerful.",
        NULL,
        "The %s looks much more powerful.",
        NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_CANCELLATION, "ET_CANCELLATION", 250, TRUE, "cancellation",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_UNDEAD_PROTECTION, "ET_UNDEAD_PROTECTION", 400, TRUE, "undead protection",
        "You feel safe in the dark.",
        NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_SPIRIT_PROTECTION, "ET_SPIRIT_PROTECTION", 400, TRUE, "spirit protection",
        "You feel a protecting force.",
        NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_LIFE_PROTECTION, "ET_LIFE_PROTECTION", 2500, TRUE, "life protection",
        "You've never felt so safe.",
        "You feel less safe than before.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_NOTHEFT, "ET_NOTHEFT", 400, TRUE, "theft protection",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_SUSTAINMENT, "ET_SUSTAINMENT", 400, TRUE, "sustainment",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_TIMESTOP, "ET_TIMESTOP", 20, TRUE, "time stop",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_WALL_WALK, "ET_WALL_WALK", 20, TRUE, "wall-walk",
        "You can now walk through walls.",
        "You can no longer walk through walls.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_LEVITATION, "ET_LEVITATION", 20, TRUE, "levitation",
        "You start to float in the air!",
        "You gently sink to the ground.",
        "The %s starts to float in the air!",
        "The %s gently sinks to the ground.",
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_DEC_CON, "ET_DEC_CON", 1, 1, NULL,
        "You feel incapacitated.",
        "You feel tougher.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_DEX, "ET_DEC_DEX", 1, 1, NULL,
        "You feel clumsy.",
        "Your dexterousness returns.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_INT, "ET_DEC_INT", 1, 1, NULL,
        "You feel imbecile.",
        "Your intelligence returns.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_STR, "ET_DEC_STR", 1, 1, NULL,
        "You are weaker.",
        "You regain your strength.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_WIS, "ET_DEC_WIS", 1, 1, NULL,
        "You feel ignorant.",
        "You feel less ignorant.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_RND, "ET_DEC_RND", 1, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_AGGRAVATE_MONSTER, "ET_AGGRAVATE_MONSTER", 500, TRUE, "aggravating",
        "You sense rising anger.",
        NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_SLEEP, "ET_SLEEP", 25, TRUE, "sleeping",
        "You fall asleep.",
        "You wake up.",
        "The %s falls asleep.",
        "The %s wakes up.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_DIZZINESS, "ET_DIZZINESS", 250, 5, "dizzy",
        "You're dizzy and weak!",
        "You're no longer dizzy.",
        "The %s looks dizzy and weak.",
        "The %s no longer looks dizzy.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_SICKNESS, "ET_SICKNESS", 250, 10, "sick",
        "You feel a sickness coming on.",
        "You now feel better.",
        NULL, NULL,
        TRUE, TRUE, TRUE, TRUE
    },

    {
        ET_BLINDNESS, "ET_BLINDNESS", 250, TRUE, "blind",
        "You can't see anything!",
        "The blindness lifts.",
        "The %s seems to be blinded.",
        "Looks like the %s can see again.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_CLUMSINESS, "ET_CLUMSINESS", 250, TRUE, "clumsy",
        "You begin to lose hand to eye coordination!",
        "You're less awkward now.",
        NULL, NULL,
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_ITCHING, "ET_ITCHING", 100, TRUE, "itching",
        "You feel an irritation spread over your skin!",
        "The irritation subsides.",
        NULL, NULL,
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_CONFUSION, "ET_CONFUSION", 25, TRUE, "confused",
        "You are confused.",
        "You regain your senses.",
        "The %s looks confused.",
        "The %s seems to have regained it's senses.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_PARALYSIS, "ET_PARALYSIS", 25, TRUE, "paralysed",
        "You are paralysed.",
        "You can move again.",
        NULL, NULL,
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_POISON, "ET_POISON", 300, 1, "poisoned",
        NULL, /* message is shown in player_damage_take */
        "You are cured.",
        "The %s looks poisoned.",
        "The %s looks cured.",
        TRUE, FALSE, TRUE, TRUE
    },

    {
        ET_AMNESIA, "ET_AMNESIA", 1, 0, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_SLOWNESS, "ET_SLOWNESS", 250, 25, "slow",
        "You feel yourself slow down.",
        "You are moving faster again.",
        "The %s slows down.",
        "The %s seems to move much faster.",
        TRUE, FALSE, FALSE, TRUE
    },

    {
        ET_BURDENED, "ET_BURDENED", 0, 25, "burdened",
        "You are burdened.",
        "You are no longer burdened.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_OVERSTRAINED, "ET_OVERSTRAINED", 0, TRUE, "overload",
        "You are overloaded!",
        "You are no longer overloaded.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_TRAPPED, "ET_TRAPPED", 10, TRUE, "trapped",
        NULL,
        "You are no longer trapped!",
        NULL,
        "The %s climbs out of the pit!",
        TRUE, FALSE, TRUE, FALSE
    },
};

effect *effect_new(effect_t type)
{
    effect *ne;

    g_assert(type > ET_NONE && type < ET_MAX);

    ne = g_malloc0(sizeof(effect));
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
    effect *ne;

    g_assert(e != NULL);

    ne = g_malloc(sizeof(effect));
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
    cJSON_AddNumberToObject(eval,"type", e->type);
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
    effect *e;
    guint oid;
    cJSON *itm;

    e = g_malloc0(sizeof(effect));

    oid = cJSON_GetObjectItem(eser, "oid")->valueint;
    e->oid =  GUINT_TO_POINTER(oid);

    e->type = cJSON_GetObjectItem(eser, "type")->valueint;
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

gboolean effect_type_inc_duration(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].inc_duration;
}

gboolean effect_type_inc_amount(effect_t type)
{
    g_assert(type < ET_MAX);
    return effects[type].inc_amount;
}

const char *effect_get_desc(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].desc;
}

const char *effect_get_msg_start(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start;
}

const char *effect_get_msg_stop(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop;
}

const char *effect_get_msg_m_start(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start_monster;
}

const char *effect_get_msg_m_stop(effect *e)
{
    g_assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop_monster;
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
        gboolean modified_existing = FALSE;

        /* if the effect's duration can be extended, reset it */
        if (effects[e->type].inc_duration)
        {
            e->turns = max(e->turns, ne->turns);
            modified_existing = TRUE;
        }

        /* if the effect's amount can be extended, do so */
        if (effects[e->type].inc_amount)
        {
            e->amount += ne->amount;
            modified_existing = TRUE;
        }

        effect_destroy(ne);

        if (modified_existing == TRUE)
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
