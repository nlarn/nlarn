/*
 * effects.c
 * Copyright (C) 2009, 2010 Joachim de Groot <jdegroot@web.de>
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

#include "cJSON.h"
#include "effects.h"
#include "game.h"
#include "nlarn.h"
#include "utils.h"

static const effect_data effects[ET_MAX] =
{
    /*
        type duration amount
        msg_start
        msg_stop
        msg_start_monster
        msg_stop_monster
        var_duration var_amount inc_duration inc_amount
    */

    {
        ET_NONE, 0, 0, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_CHA, 1, 1, NULL,
        "You feel charismatic!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_CON, 1, 1, NULL,
        "You have a greater intestinal constitude!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_DEX, 1, 1, NULL,
        "You feel skilful!",
        "Your dextrousness returns to normal.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_INT, 1, 1, NULL,
        "You feel clever!",
        "You are not so smart anymore.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_STR, 1, 1, NULL,
        "Your muscles are stronger!",
        "Your strength returns to normal.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_WIS, 1, 1, NULL,
        "You feel more self-confident!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_RND, 1, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_DAMAGE, 100, 10, NULL,
        NULL, NULL, NULL, NULL,
        TRUE, TRUE, FALSE, TRUE
    },

    {
        ET_INC_HP_MAX, 1, 5, NULL, /* percent */
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_MP_MAX, 1, 5, NULL, /* percent */
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_HP_REGEN, 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_MP_REGEN, 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_LEVEL, 1, 1, NULL,
        "You feel much more skilful!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_EXP, 1, 0, NULL,
        "You feel experienced.",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_RESIST_FIRE, 0, 25, NULL,
        "You feel a chill run up your spine!",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_RESIST_COLD, 0, 25, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_RESIST_MAGIC, 0, 25, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, TRUE
    },

    {
        ET_PROTECTION, 250, 3, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_STEALTH, 250, TRUE, "stealthy",
        "You start to move stealthily.",
        "You're not stealthy anymore.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_AWARENESS, 250, 3, "aware",
        "You become aware of your surroundings.",
        "You are no longer aware of your surroundings.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_SPEED, 250, 25, "fast",
        "You are suddenly moving much faster.",
        "You feel yourself slow down.",
        "The %s seems to move much faster.",
        "The %s suddenly slows down.",
        TRUE, FALSE, FALSE, TRUE
    },

    {
        ET_HEROISM, 250, 5, NULL,
        "WOW!!! You feel Super-fantastic!!!",
        "You return to normal. How sad!",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INVISIBILITY, 250, TRUE, "invisible",
        "Suddenly you can't see yourself!",
        "You are no longer invisible.",
        "The %s disappears.",
        NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_INVULNERABILITY, 250, 10, NULL,
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_INFRAVISION, 250, TRUE, "infravision",
        "Your vision sharpens.",
        "Your vision returns to normal.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_ENLIGHTENMENT, 250, 12, "enlightened",
        "You have been granted enlightenment!",
        "You are no longer enlightened.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_DETECT_MONSTER, 10, TRUE, NULL,
        "You sense the presence of monsters.",
        NULL, NULL, NULL,
        FALSE, FALSE, TRUE, FALSE
    },

    {
        ET_HOLD_MONSTER, 30, TRUE, NULL,
        NULL, NULL,
        "The %s seems to freeze.",
        "The %s can move again.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_SCARE_MONSTER, 250, TRUE, NULL,
        NULL, NULL,
        "The %s is very afraid.",
        "The %s is no longer scared.",
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_CHARM_MONSTER, 250, TRUE, NULL,
        NULL, NULL,
        "The %s is awestruck at your magnificence!",
        "The %s is no longer impressed.",
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_AGGRAVATE_MONSTER, 500, TRUE, "aggravating",
        "You sense rising anger.",
        NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_INC_HP, 1, 20, NULL, /* percent */
        "You feel better.",
        NULL,
        "The %s looks better.",
        NULL,
        FALSE, TRUE, FALSE, FALSE
    },

    {
        ET_MAX_HP, 1, 0, NULL,
        "You are completely healed.",
        NULL,
        "The %s looks completely healed.",
        NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_INC_MP, 1, 20, NULL, /* percent */
        "Magical energies course through your body.",
        NULL,
        "The %s seems to regain energy.",
        NULL,
        FALSE, TRUE, FALSE, FALSE
    },

    {
        ET_MAX_MP, 1, 0, NULL,
        "You feel much more poweful.",
        NULL,
        "The %s looks much more powerful.",
        NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_CANCELLATION, 250, TRUE, "cancellation",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_UNDEAD_PROTECTION, 400, TRUE, "undead protection",
        "You feel safe in the dark.",
        NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_SPIRIT_PROTECTION, 400, TRUE, "spirit protection",
        "You feel a protecting force.",
        NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_LIFE_PROTECTION, 0, TRUE, "life protection",
        "You never felt so safe.",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_NOTHEFT, 400, TRUE, "theft protection",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_TIMESTOP, 20, TRUE, "time stop",
        NULL, NULL, NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_WALL_WALK, 20, TRUE, "wall-walk",
        "You can now walk through walls",
        "You can no longer walk through walls.",
        NULL, NULL,
        TRUE, FALSE, TRUE, FALSE
    },

    {
        ET_DEC_CHA, 1, 1, NULL,
        "You feel rejected.",
        NULL, NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_CON, 1, 1, NULL,
        "You feel incapacitated.",
        NULL, NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_DEX, 1, 1, NULL,
        "You feel clumsy.",
        "Your dexterousness returns.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_INT, 1, 1, NULL,
        "You feel imbecile.",
        NULL, NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_STR, 1, 1, NULL,
        "You are weaker.",
        "You regain your strength.",
        NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_WIS, 1, 1, NULL,
        "You feel ignorant.",
        NULL, NULL, NULL,
        FALSE, FALSE, TRUE, TRUE
    },

    {
        ET_DEC_RND, 1, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_DEC_HP_MAX, 1, 5, NULL, /* percent */
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_DEC_MP_MAX, 1, 5, NULL, /* percent */
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_DEC_HP_REGEN, 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_DEC_MP_REGEN, 0, 1, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_DEC_LEVEL, 1, 1, NULL,
        "You stagger for a moment...",
        NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_DEC_EXP, 1, 0, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_SLEEP, 25, TRUE, "sleeping",
        "You fall asleep.",
        "You wake up.",
        "The %s falls asleep.",
        "The %s wakes up.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_DIZZINESS, 250, 5, "dizzy",
        "You're dizzy and weak!",
        "You're no longer dizzy.",
        "The %s looks dizzy and weak.",
        "The %s no longer looks dizzy.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_SICKNESS, 250, 10, "sick",
        "You feel a sickness coming on.",
        "You now feel better.",
        NULL, NULL,
        TRUE, TRUE, TRUE, TRUE
    },

    {
        ET_BLINDNESS, 250, TRUE, "blind",
        "You can't see anything!",
        "The blindness lifts.",
        "The %s seems to be blinded.",
        "Looks like the %s can see again.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_CLUMSINESS, 250, TRUE, "clumsy",
        "You are unable to hold your weapon.",
        "You're less awkward now.",
        NULL, NULL,
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_ITCHING, 100, TRUE, "itching",
        "The hysteria of itching forces you to remove your armour!",
        "The irritation subsides.",
        NULL, NULL,
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_CONFUSION, 25, TRUE, "confused",
        "You are confused.",
        "You regain your senses.",
        "The %s looks confused.",
        "The %s seems to have regained it's senses.",
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_PARALYSIS, 25, TRUE, "paralyzed",
        "You are paralyzed.",
        "You can move again.",
        NULL, NULL,
        TRUE, FALSE, FALSE, FALSE
    },

    {
        ET_POISON, 300, 1, "poisoned",
        NULL, /* message is shown in player_damage_take */
        "You are cured.",
        "The %s looks poisoned.",
        "The %s looks cured.",
        TRUE, FALSE, TRUE, TRUE
    },

    {
        ET_AMNESIA, 1, 0, NULL,
        NULL, NULL, NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_SLOWNESS, 250, 25, "slow",
        "You feel yourself slow down.",
        "You are moving faster again.",
        "The %s slows down.",
        "The %s seems to move much faster.",
        TRUE, FALSE, FALSE, TRUE
    },

    {
        ET_BURDENED, 0, TRUE, "burdened",
        "You are burdened.",
        "You are no longer burdened.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },

    {
        ET_OVERSTRAINED, 0, TRUE, "overload",
        "You are overloaded!",
        "You are no longer overloaded.",
        NULL, NULL,
        FALSE, FALSE, FALSE, FALSE
    },
};

effect *effect_new(effect_type type)
{
    effect *ne;

    assert(type > ET_NONE && type < ET_MAX);

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
        ne->turns = effects[type].duration ;
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

    assert(e != NULL);

    ne = g_malloc(sizeof(effect));
    memcpy(ne, e, sizeof(effect));

    /* register copy with game */
    ne->oid = game_effect_register(nlarn, ne);

    return ne;
}

void effect_destroy(effect *e)
{
    assert(e != NULL);

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
    cJSON *item;

    e = g_malloc0(sizeof(effect));

    oid = cJSON_GetObjectItem(eser, "oid")->valueint;
    e->oid =  GUINT_TO_POINTER(oid);

    e->type = cJSON_GetObjectItem(eser, "type")->valueint;
    e->start = cJSON_GetObjectItem(eser, "start")->valueint;
    e->turns = cJSON_GetObjectItem(eser, "turns")->valueint;
    e->amount = cJSON_GetObjectItem(eser, "amount")->valueint;

    if ((item = cJSON_GetObjectItem(eser, "item")))
    {
        e->item = GUINT_TO_POINTER(item->valueint);
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

cJSON *effects_serialize(GPtrArray *effects)
{
    int idx;
    cJSON *eser = cJSON_CreateArray();

    for (idx = 0; idx < effects->len; idx++)
    {
        gpointer eff_oid = g_ptr_array_index(effects, idx);
        cJSON_AddItemToArray(eser, cJSON_CreateNumber(GPOINTER_TO_UINT(eff_oid)));
    }

    return eser;
}

GPtrArray *effects_deserialize(cJSON *eser)
{
    int idx;
    GPtrArray *effects;

    effects = g_ptr_array_new();

    for (idx = 0; idx < cJSON_GetArraySize(eser); idx++)
    {
        cJSON *effser = cJSON_GetArrayItem(eser, idx);
        guint oid = effser->valueint;
        g_ptr_array_add(effects, GUINT_TO_POINTER(oid));
    }

    return effects;
}

const char *effect_get_desc(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].desc;
}

const char *effect_get_msg_start(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start;
}

const char *effect_get_msg_stop(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop;
}

const char *effect_get_msg_m_start(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_start_monster;
}

const char *effect_get_msg_m_stop(effect *e)
{
    assert(e != NULL && e->type > ET_NONE && e->type < ET_MAX);
    return effects[e->type].msg_stop_monster;
}

int effect_get_amount(effect *e)
{
    assert (e != NULL);
    return e->amount;
}

effect *effect_add(GPtrArray *ea, effect *ne)
{
    effect *e;

    assert(ea != NULL && ne != NULL);

    /* check for existing effects - leave permanent effects alone */
    if ((e = effect_get(ea, ne->type)) && (e->turns > 0))
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
    assert(ea != NULL && e != NULL);
    return g_ptr_array_remove_fast(ea, e->oid);
}

effect *effect_get(GPtrArray *ea, effect_type type)
{
    guint idx;

    assert(ea != NULL && type > ET_NONE && type < ET_MAX);

    for (idx = 0; idx < ea->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(ea, idx);
        effect *e = game_effect_get(nlarn, effect_id);

        if (e->type == type)
        {
            return e;
        }
    }

    return NULL;
}

int effect_query(GPtrArray *ea, effect_type type)
{
    guint idx;
    int amount = 0;

    assert(ea != NULL && type > ET_NONE && type < ET_MAX);

    for (idx = 0; idx < ea->len; idx++)
    {
        gpointer effect_id = g_ptr_array_index(ea, idx);
        effect *e = game_effect_get(nlarn, effect_id);

        if (e->type == type) amount += e->amount;
    }

    return amount;
}

int effect_expire(effect *e)
{
    assert(e != NULL);

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
