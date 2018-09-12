/*
 * spells.c
 * Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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

#include "defines.h"
#include "display.h"
#include "map.h"
#include "nlarn.h"
#include "random.h"
#include "sobjects.h"
#include "spells.h"
#include "spheres.h"

const spell_data spells[SP_MAX] =
{
    {
        SP_PRO, "pro","protection",
        SC_PLAYER, DAM_NONE, ET_PROTECTION,
        "Generates a protection field.",
        NULL, NULL,
        DC_NONE, 1, 260, TRUE
    },
    {
        SP_MLE, "mle", "magic missile",
        SC_RAY, DAM_MAGICAL, ET_NONE,
        "Creates and hurls a missile magic of magical energy at a target.",
        "The missile hits the %s.",
        "The missile bounces off the %s.",
        DC_NONE, 1, 320, TRUE
    },
    {
        SP_DEX, "dex", "dexterity",
        SC_PLAYER, DAM_NONE, ET_INC_DEX,
        "Improves the caster's dexterity.",
        NULL, NULL,
        DC_NONE, 1, 260, FALSE
    },
    {
        SP_SLE, "sle", "sleep",
        SC_POINT, DAM_NONE, ET_SLEEP,
        "Causes some monsters to go to sleep.",
        NULL,
        "The %s doesn't sleep.",
        DC_NONE, 1, 260, TRUE
    },
    {
        SP_CHM, "chm", "charm monster",
        SC_POINT, DAM_NONE, ET_CHARM_MONSTER,
        "Some monsters may be awed at your magnificence.",
        NULL, NULL,
        DC_NONE, 1, 260, FALSE
    },
    {
        SP_SSP, "ssp", "sonic spear",
        SC_RAY, DAM_PHYSICAL, ET_NONE,
        "Causes your hands to emit a screeching sound toward what they point.",
        "The sound damages the %s.",
        "The %s can't hear the noise.",
        DC_LIGHTCYAN, 2, 480, FALSE
    },
    {
        SP_STR, "str", "strength",
        SC_PLAYER, DAM_NONE, ET_INC_STR,
        "Increase the caster's strength for a short term.",
        NULL, NULL,
        DC_NONE, 2, 460, FALSE
    },
    {
        SP_CPO, "cpo", "cure poison",
        SC_OTHER, DAM_NONE, ET_NONE,
        "The caster is cured from poison.",
        NULL, NULL,
        DC_NONE, 2, 460, TRUE
    },
    {
        SP_HEL, "hel", "healing",
        SC_PLAYER, DAM_NONE, ET_INC_HP,
        "Restores some HP to the caster.",
        NULL, NULL,
        DC_NONE, 2, 500, TRUE
    },
    {
        SP_CBL, "cbl", "cure blindness",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Restores sight to one so unfortunate as to be blinded.",
        NULL, NULL,
        DC_NONE, 2, 400, TRUE
    },
    {
        SP_CRE, "cre", "create monster",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Creates a monster near the caster appropriate for the location.",
        NULL, NULL,
        DC_NONE, 2, 400, FALSE
    },
    {
        SP_PHA, "pha", "phantasmal forces",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Creates illusions, and if believed, the monster flees.",
        "The %s believed!",
        "The %s didn't believe the illusions!",
        DC_NONE, 2, 600, FALSE
    },
    {
        SP_INV, "inv", "invisibility",
        SC_PLAYER, DAM_NONE, ET_INVISIBILITY,
        "The caster becomes invisible.",
        NULL, NULL,
        DC_NONE, 2, 600, FALSE
    },
    {
        SP_BAL, "bal", "fireball",
        SC_BLAST, DAM_FIRE, ET_NONE,
        "Makes a ball of fire that burns on what it hits.",
        "The fireball hits the %s.",
        NULL,
        DC_LIGHTRED, 3, 1200, FALSE
    },
    {
        SP_CLD, "cld", "cone of cold",
        SC_RAY, DAM_COLD, ET_NONE,
        "Sends forth a cone of cold which freezes what it touches.",
        "The cone of cold strikes the %s.",
        NULL,
        DC_WHITE, 3, 1200, FALSE
    },
    {
        SP_PLY, "ply", "polymorph",
        SC_POINT, DAM_NONE, ET_NONE,
        "You can find out what this does for yourself.",
        NULL,
        "The %s resists.",
        DC_NONE, 3, 950, FALSE
    },
    {
        SP_CAN, "can", "cancellation",
        SC_PLAYER, DAM_NONE, ET_CANCELLATION,
        "Protects the caster against spheres of annihilation.",
        NULL, NULL,
        DC_NONE, 3, 950, FALSE
    },
    {
        SP_HAS, "has", "haste self",
        SC_PLAYER, DAM_NONE, ET_SPEED,
        "Speeds up the caster's movements.",
        NULL, NULL,
        DC_NONE, 3, 950, FALSE
    },
    {
        SP_CKL, "ckl", "killing cloud",
        SC_FLOOD, DAM_ACID, ET_NONE,
        "Creates a fog of poisonous gas which kills all that is within it.",
        "The %s gasps for air.",
        NULL,
        DC_NONE, 3, 1200, FALSE
    },
    {
        SP_VPR, "vpr", "vaporize rock",
        SC_OTHER, DAM_NONE, ET_NONE,
        "This changes rock to air.",
        NULL, NULL,
        DC_NONE, 3, 950, FALSE
    },
    {
        SP_DRY, "dry", "dehydration",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "Dries up water in the immediate vicinity.",
        "The %s shrivels up.",
        "The %s isn't affected.",
        DC_NONE, 4, 1600, FALSE
    },
    {
        SP_LIT, "lit", "lightning",
        SC_RAY, DAM_ELECTRICITY, ET_NONE,
        "Your finger will emit a lightning bolt when this spell is cast.",
        "A lightning bolt hits the %s.",
        "The %s loves fire and lightning!",
        DC_YELLOW, 4, 1600, FALSE
    },
    {
        SP_DRL, "drl", "drain life",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "Subtracts hit points from both you and a monster.",
        NULL, NULL,
        DC_NONE, 4, 1400, FALSE
    },
    {
        SP_GLO, "glo", "invulnerability",
        SC_PLAYER, DAM_NONE, ET_INVULNERABILITY,
        "This globe helps to protect the player from physical attack.",
        NULL, NULL,
        DC_NONE, 4, 1400, FALSE
    },
    {
        SP_FLO, "flo", "flood",
        SC_FLOOD, DAM_WATER, ET_NONE,
        "This creates an avalanche of H2O to flood the immediate chamber.",
        "The %s struggles for air in the flood!",
        "The %s loves the water!",
        DC_NONE, 4, 1600, FALSE
    },
    {
        SP_FGR, "fgr", "finger of death",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "This is a holy spell and calls upon your god to back you up.",
        "The %s's heart stopped.",
        "The %s isn't affected.",
        DC_NONE, 4, 1600, FALSE
    },
    {
        SP_SCA, "sca", "scare monsters",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Terrifies nearby monsters so that hopefully they flee the magic user.",
        NULL, NULL,
        DC_NONE, 5, 2000, FALSE
    },
    {
        SP_HLD, "hld", "hold monster",
        SC_POINT, DAM_NONE, ET_HOLD_MONSTER,
        "The monster is frozen in his tracks if this is successful.",
        NULL, NULL,
        DC_NONE, 5, 2000, FALSE
    },
    {
        SP_STP, "stp", "time stop",
        SC_PLAYER, DAM_NONE, ET_TIMESTOP,
        "All movement in the caverns ceases for a limited duration.",
        NULL, NULL,
        DC_NONE, 5, 2500, FALSE
    },
    {
        SP_TEL, "tel", "teleport away",
        SC_POINT, DAM_NONE, ET_NONE,
        "Moves a particular monster around in the dungeon.",
        NULL, NULL,
        DC_NONE, 5, 2000, FALSE
    },
    {
        SP_MFI, "mfi", "magic fire",
        SC_FLOOD, DAM_FIRE, ET_NONE,
        "This causes a curtain of fire to appear all around you.",
        "The %s cringes from the flame.",
        NULL,
        DC_NONE, 5, 2500, FALSE
    },
    {
        SP_MKW, "mkw", "make wall",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Makes a wall in the specified place.",
        NULL, NULL,
        DC_NONE, 6, 3000, FALSE
    },
    {
        SP_SPH, "sph", "sphere of annihilation",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Anything caught in this sphere is instantly killed.",
        NULL, NULL,
        DC_NONE, 6, 3500, FALSE
    },
    {
        SP_SUM, "sum", "summon demon",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Summons a demon who hopefully helps you out.",
        NULL, NULL,
        DC_NONE, 6, 3500, FALSE
    },
    {
        SP_WTW, "wtw", "walk through walls",
        SC_PLAYER, DAM_NONE, ET_WALL_WALK,
        "Allows the caster to walk through walls for a short period of time.",
        NULL, NULL,
        DC_NONE, 6, 3800, FALSE
    },
    {
        SP_ALT, "alt", "alter reality",
        SC_OTHER, DAM_NONE, ET_NONE,
        "God only knows what this will do.",
        NULL,
        "Polinneaus won't let you mess with his dungeon!",
        DC_NONE, 6, 3800, FALSE
    },
};

struct book_obfuscation_s
{
    const char* desc;
    const int weight;
    const int colour;
}
book_obfuscation[SP_MAX] =
{
    { "parchment-bound", 800, DC_BROWN,     },
    { "thick",          1200, DC_RED,       },
    { "dusty",           800, DC_LIGHTGRAY, },
    { "leather-bound",   800, DC_BROWN,     },
    { "heavy",          1200, DC_GREEN,     },
    { "ancient",         800, DC_DARKGRAY,  },
    { "buckram",         800, DC_LIGHTGRAY, },
    { "gilded",          800, DC_YELLOW,    },
    { "embossed",        800, DC_BLUE,      },
    { "old",             800, DC_LIGHTGRAY, },
    { "thin",            400, DC_GREEN,     },
    { "light",           400, DC_WHITE,     },
    { "large",          1200, DC_BLUE,      },
    { "vellum",          800, DC_BROWN,     },
    { "tan",             800, DC_BROWN,     },
    { "papyrus",         800, DC_BROWN,     },
    { "linen",           800, DC_WHITE,     },
    { "musty",           800, DC_GREEN,     },
    { "faded",           800, DC_DARKGRAY,  },
    { "antique",         800, DC_DARKGRAY,  },
    { "worn out",        800, DC_DARKGRAY,  },
    { "tattered",        800, DC_LIGHTGRAY, },
    { "aged",            800, DC_DARKGRAY,  },
    { "ornate",          800, DC_BLUE,      },
    { "inconspicuous",   800, DC_LIGHTGRAY, },
    { "awe-inspiring",   800, DC_WHITE,     },
    { "stained",         800, DC_BROWN,     },
    { "mottled",         800, DC_RED,       },
    { "plaid",           800, DC_RED,       },
    { "wax-lined",       800, DC_BROWN,     },
    { "bamboo",          800, DC_YELLOW,    },
    { "clasped",         800, DC_YELLOW,    },
    { "well-thumbed",    800, DC_BLUE,      },
    { "ragged",          800, DC_LIGHTGRAY, },
    { "dull",            800, DC_DARKGRAY,  },
    { "canvas",          800, DC_YELLOW,    },
/*
    reserve descriptions for unimplemented spells:
    chambray
*/
};

/* the last cast spell */
static spell *last_spell = NULL;

/* local functions */
static int spell_cast(player *p, spell *s);
static void spell_print_success_message(spell *s, monster *m);
static void spell_print_failure_message(spell *s, monster *m);
static int count_adjacent_water_squares(position pos);
static int try_drying_ground(position pos);

/* simple wrapper for spell_area_pos_hit() */
static gboolean spell_traj_pos_hit(const GList *traj,
        const damage_originator *damo,
        gpointer data1, gpointer data2);

static gboolean spell_area_pos_hit(position pos,
        const damage_originator *damo,
        gpointer data1, gpointer data2);

spell *spell_new(spell_id id)
{
    spell *nspell;

    g_assert(id < SP_MAX);

    nspell = g_malloc0(sizeof(spell));
    nspell->id = id;
    nspell->knowledge = 1;

    return nspell;
}

void spell_destroy(spell *s)
{
    g_assert(s != NULL);
    g_free(s);
}

cJSON *spell_serialize(spell *s)
{
    cJSON *sser = cJSON_CreateObject();

    cJSON_AddNumberToObject(sser, "id", s->id);
    cJSON_AddNumberToObject(sser, "knowledge", s->knowledge);
    cJSON_AddNumberToObject(sser, "used", s->used);

    return sser;
}

spell *spell_deserialize(cJSON *sser)
{
    spell *s = g_malloc0(sizeof(spell));

    s->id = cJSON_GetObjectItem(sser, "id")->valueint;
    s->knowledge = cJSON_GetObjectItem(sser, "knowledge")->valueint;
    s->used = cJSON_GetObjectItem(sser, "used")->valueint;

    return s;
}

cJSON *spells_serialize(GPtrArray *sparr)
{
    cJSON *sser = cJSON_CreateArray();

    for (guint idx = 0; idx < sparr->len; idx++)
    {
        spell *s = g_ptr_array_index(sparr, idx);
        cJSON_AddItemToArray(sser, spell_serialize(s));
    }

    return sser;
}

GPtrArray *spells_deserialize(cJSON *sser)
{
    GPtrArray *n_spells = g_ptr_array_new_with_free_func(
            (GDestroyNotify)spell_destroy);

    for (int idx = 0; idx < cJSON_GetArraySize(sser); idx++)
    {
        spell *s = spell_deserialize(cJSON_GetArrayItem(sser, idx));
        g_ptr_array_add(n_spells, s);
    }

    return n_spells;
}

int spell_sort(gconstpointer a, gconstpointer b)
{
    gint order;
    spell *spell_a = *((spell**)a);
    spell *spell_b = *((spell**)b);

    if (spell_a->id == spell_b->id)
        order = 0;
    else
        order = strcmp(spell_code(spell_a), spell_code(spell_b));

    return order;
}

/* Knowledge of a spell and intelligence make casting easier. */
static int spell_success_value(player *p, spell *sp)
{
    g_assert(p != NULL && sp != NULL);

    if (player_get_int(p) < (3 * spell_level(sp)))
        return 0;

    return (player_get_int(p) - 2 * (spell_level(sp) - sp->knowledge));
}


int spell_cast_new(struct player *p)
{
    /* check if the player knows any spell */
    if (!p->known_spells || !p->known_spells->len)
    {
        log_add_entry(nlarn->log, "You don't know any spells.");
        return 0;
    }

    /* spell casting is impossible when confused */
    if (player_effect(p, ET_CONFUSION))
    {
        log_add_entry(nlarn->log, "You can't aim your magic!");
        return 0;
    }

    /* show spell selection dialogue */
    last_spell = display_spell_select("Select a spell to cast", p);

    /* player aborted spell selection by pressing ESC */
    if (!last_spell)
        return 0;

    return spell_cast(p, last_spell);
}

int spell_cast_previous(struct player *p)
{
    /* spell casting is impossible when confused */
    if (player_effect(p, ET_CONFUSION))
    {
        log_add_entry(nlarn->log, "You can't aim your magic!");
        return 0;
    }

    /* not casted any spell before */
    if (!last_spell)
    {
        return spell_cast_new(p);
    }

    return spell_cast(p, last_spell);
}

int spell_learn(player *p, spell_id spell_type)
{
    g_assert(p != NULL && spell_type < SP_MAX);

    if (!spell_known(p, spell_type))
    {
        /* Check if the player's intelligence is sufficient to learn the spell */
        if ((spells[spell_type].level * 3) > (int)player_get_int(p))
            /* spell is beyond the players scope */
            return FALSE;

        /* Check if the player's level is spell sufficient to learn the spell */
        if (spells[spell_type].level > (int)p->level)
            /* spell is beyond the players scope */
            return FALSE;

        spell *s = spell_new(spell_type);
        g_ptr_array_add(p->known_spells, s);
        return s->knowledge;
    }
    else
    {
        /* spell already known, improve knowledge */
        for (guint idx = 0; idx < p->known_spells->len; idx++)
        {
            /* search spell */
            spell *s = (spell *)g_ptr_array_index(p->known_spells, idx);

            if (s->id == spell_type)
            {
                /* found it */
                s->knowledge++;
                return s->knowledge;
            }
        }
    }

    /* should not reach this point, but who knows.. */
    return FALSE;
}

int spell_known(player *p, spell_id spell_type)
{
    g_assert(p != NULL && spell_type < SP_MAX);

    for (guint idx = 0; idx < p->known_spells->len; idx++)
    {
        spell *s = g_ptr_array_index(p->known_spells, idx);
        if (s->id == spell_type)
        {
            return s->knowledge;
        }
    }

    return FALSE;
}

int spell_type_player(spell *s, struct player *p)
{
    effect *e = NULL;

    g_assert(s != NULL && p != NULL && (spell_type(s) == SC_PLAYER));

    /* check if the player is already affected by the effect */
    if ((e = player_effect_get(p, spell_effect(s))))
    {
        /* player has cast this spell before */
        if (effect_type_inc_amount(e->type))
        {
            /* The effect amount can be incremented.
             * Increase the amount of the effect up to the base
             * effect value * spell knowledge value. */
            if (e->amount < (effect_type_amount(e->type) * (int)s->knowledge))
            {
                e->amount += effect_type_amount(e->type);
                log_add_entry(nlarn->log, "You have extended the power of %s.",
                        spell_name(s));

                /* force recalculation of burdened
                   status if extending strength */
                if (e->type == ET_INC_STR)
                {
                    player_inv_weight_recalc(p->inventory, NULL);
                }
            }
            else
            {
                /* maximum reached -> indicate failure */
                log_add_entry(nlarn->log, "You have already extended the "
                        "power of %s to the extent of your knowledge.",
                        spell_name(s));

                return FALSE;
            }
        }
        else if (effect_type_inc_duration(e->type))
        {
            /* The duration of this effect can be incremented.
             * Increase the duration of the effect up to the base
             * effect duration * spell knowledge value. */
            if (e->turns + effect_type_duration(e->type)
                < (effect_type_duration(e->type) * s->knowledge))
            {
                e->turns += effect_type_duration(e->type);
                log_add_entry(nlarn->log, "You have extended the duration "
                        "of %s.", spell_name(s));
            }
            else
            {
                /* maximum reached -> indicate failure */
                log_add_entry(nlarn->log, "You have already extended the "
                        "duration of %s to the extent of your knowledge.",
                        spell_name(s));

                return FALSE;
            }
        }

        return TRUE;
    }

    e = effect_new(spell_effect(s));

    /* make effects that are permanent by default non-permanent */
    /* unless it is the spell of healing, which does work this way */
    if ((e->turns == 1) && (e->type != ET_INC_HP))
    {
        e->turns = 100 * s->knowledge;
    }

    if (e->type == ET_INC_HP)
    {
        e->amount *= s->knowledge;
    }

    player_effect_add(p, e);

    return TRUE;
}

int spell_type_point(spell *s, struct player *p)
{
    monster *m = NULL;
    position pos;
    effect *e;
    char buffer[61];
    int amount = 0;

    g_assert(s != NULL && p != NULL && (spell_type(s) == SC_POINT));

    g_snprintf(buffer, 60, "Select a target for %s.", spell_name(s));

    /* Allow non-visible positions if the player is blinded. */
    pos = display_get_position(p, buffer, FALSE, FALSE, 0, FALSE,
            !player_effect(p, ET_BLINDNESS));

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    if (pos_identical(pos, p->pos))
    {
        log_add_entry(nlarn->log, "This spell only works on monsters.");
        return FALSE;
    }

    /* When the player is blinded, check if the position can be reached.
       As it is possible to target any position, it might be a position
       the player could not target under normal circumstances. */
    if (player_effect(p, ET_BLINDNESS) &&
            !map_pos_is_visible(game_map(nlarn, Z(p->pos)), p->pos, pos))
    {
        /* be sure to waste the MPs and give no hints. */
        return TRUE;
    }

    m = map_get_monster_at(game_map(nlarn, Z(p->pos)), pos);

    if (!m)
    {
        if (s->id == SP_DRY)
            return try_drying_ground(pos);

        if (!player_effect(p, ET_BLINDNESS))
        {
            log_add_entry(nlarn->log, "The is no monster there.");
            return FALSE;
        }
        else
        {
            /* The spell didn't do anything, but as the player is blinded
               assume it was targeted at the position intentionally probing
               for targets. */
            return TRUE;
        }
    }

    switch (s->id)
    {
        /* dehydration */
    case SP_DRY:
        amount = (100 * s->knowledge) + p->level;
        spell_print_success_message(s, m);
        monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_MAGIC, amount,
                                                DAMO_PLAYER, p));
        break; /* SP_DRY */

        /* drain life */
    case SP_DRL:
        amount = min(p->hp - 1, (int)p->hp_max / 2);

        spell_print_success_message(s, m);
        monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_MAGIC, amount,
                                                DAMO_PLAYER, p));

        player_damage_take(p, damage_new(DAM_MAGICAL, ATT_MAGIC, amount,
                                         DAMO_PLAYER, NULL), PD_SPELL, SP_DRL);

        break; /* SP_DRL */

        /* finger of death */
    case SP_FGR:
    {
        // Lower chances of working against undead and demons.
        const int roll = (monster_flags(m, MF_UNDEAD) ? 40 :
                          monster_flags(m, MF_DEMON)  ? 30 : 20);

        if ((player_get_wis(p) + s->knowledge) > (guint)rand_m_n(10, roll))
        {
            spell_print_success_message(s, m);
            monster_damage_take(m, damage_new(DAM_MAGICAL, ATT_MAGIC, 2000,
                                                    DAMO_PLAYER, p));
        }
        else
            spell_print_failure_message(s, m);
        break; /* SP_FGR */
    }

        /* polymorph */
    case SP_PLY:
        if (chance(5 * (monster_level(m) - 2 * s->knowledge)))
        {
            /* It didn't work */
            spell_print_failure_message(s, m);
        }
        else
        {
            monster_polymorph(m);
        }
        break;

        /* teleport */
    case SP_TEL:
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s disappears.",
                          monster_name(m));
        }

        map *mmap = game_map(nlarn, Z(monster_pos(m)));
        monster_pos_set(m, mmap, map_find_space(mmap, LE_MONSTER, FALSE));
        break; /* SP_TEL */

    default:
        /* spell has an effect, add that to the monster */
        g_assert(spell_effect(s) != ET_NONE);

        e = effect_new(spell_effect(s));

        if (!e->amount)
        {
            e->amount = p->intelligence;
        }

        e->amount *= s->knowledge;
        e = monster_effect_add(m, e);

        if (e)
            spell_print_success_message(s, m);
        else
            spell_print_failure_message(s, m);

        break;
    }

    return TRUE;
}

int spell_type_ray(spell *s, struct player *p)
{
    g_assert(s != NULL && p != NULL && (spell_type(s) == SC_RAY));

    char buffer[61];

    g_snprintf(buffer, 60, "Select a target for the %s.", spell_name(s));
    /* Allow non-visible positions if the player is blinded. */
    position target = display_get_position(p, buffer, TRUE, FALSE, 0, FALSE,
                                           !player_effect(p, ET_BLINDNESS));

    /* player pressed ESC */
    if (!pos_valid(target))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    if (pos_identical(target, p->pos))
    {
        log_add_entry(nlarn->log, "Why would you want to do that?");
        return FALSE;
    }

    damage_originator damo = { DAMO_PLAYER, p };
    damage *dam = damage_new(spells[s->id].damage_type, ATT_MAGIC, 0,
                             damo.ot, damo.originator);

    /* determine amount of damage */
    switch (s->id)
    {
    case SP_MLE:
        dam->amount = (1 + rand_1n(5)) * s->knowledge + p->level;
        break;

    case SP_SSP:
        dam->amount = (2 + rand_1n(10)) * s->knowledge + p->level;
        break;

    case SP_CLD:
        dam->amount = (3 + rand_1n(15)) * s->knowledge + p->level;
        break;

    case SP_LIT:
        dam->amount = (4 + rand_1n(20)) * s->knowledge + p->level;
        break;
    default:
        /* this shouldn't happen */
        break;
    }

    /* throw a ray to the selected target */
    map_trajectory(p->pos, target, &damo, spell_traj_pos_hit,
                   s, dam, TRUE, '*', spell_colour(s), TRUE);

    /* The callback functions give a copy of the damage to the specific
       functions, thus the original has to be destroyed here. */
    damage_free(dam);

    return TRUE;
}

int spell_type_flood(spell *s, struct player *p)
{
    position pos;
    char buffer[81];
    int radius = 0;
    int amount = 0;
    map_tile_t type = LT_NONE;

    g_assert(s != NULL && p != NULL && (spell_type(s) == SC_FLOOD));

    g_snprintf(buffer, 60, "Where do you want to place the %s?", spell_name(s));
    pos = display_get_position(p, buffer, FALSE, FALSE, 0, FALSE, TRUE);

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    switch (s->id)
    {
    case SP_CKL:
        radius = 3;
        type = LT_CLOUD;
        amount = (10 * s->knowledge) + p->level;
        break;

    case SP_FLO:
        radius = 4;
        type = LT_WATER;
        amount = (25 * s->knowledge) + p->level;
        break;

    case SP_MFI:
        radius = 4;
        type = LT_FIRE;
        amount = (15 * s->knowledge) + p->level;
        break;

    default:
        /* this shouldn't happen */
        break;
    }

    area *obstacles = map_get_obstacles(game_map(nlarn, Z(pos)), pos, radius, FALSE);
    area *range = area_new_circle_flooded(pos, radius, obstacles);

    if (area_pos_get(range, p->pos)
            && !display_get_yesno("The spell is going to hit you. " \
                                  "Cast anyway?", NULL, NULL, NULL))
    {
        log_add_entry(nlarn->log, "Aborted.");
        area_destroy(range);
        return FALSE;
    }

    map_set_tiletype(game_map(nlarn, Z(pos)), range, type, amount);
    area_destroy(range);

    return TRUE;
}

int spell_type_blast(spell *s, struct player *p)
{
    g_assert(s != NULL && p != NULL && (spell_type(s) == SC_BLAST));

    area *ball;
    position pos;
    char buffer[61];
    int amount = 0;
    int radius = 0;
    damage_originator damo = { DAMO_PLAYER, p };
    map *cmap = game_map(nlarn, Z(p->pos));

    switch (s->id)
    {
        /* currently there is only the fireball */
    case SP_BAL:
    default:
        radius = 2;
        amount = (3 + rand_1n(15)) * s->knowledge + p->level;
        break;
    }

    g_snprintf(buffer, 60, "Point to the center of the %s.", spell_name(s));
    /* Allow non-visible positions if the player is blinded. */
    pos = display_get_position(p, buffer, FALSE, TRUE, radius, FALSE,
            !player_effect(p, ET_BLINDNESS));

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    /* get the affected area to determine if the player would be hit */
    ball = area_new_circle_flooded(pos, radius, map_get_obstacles(cmap, pos,
                radius, TRUE));

    gboolean player_affected = area_pos_get(ball, p->pos);
    area_destroy(ball);

    if (player_affected
        && !display_get_yesno("The spell is going to hit you. Cast anyway?", NULL, NULL, NULL))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    damage *dam = damage_new(spells[s->id].damage_type, ATT_MAGIC,
                             amount, DAMO_PLAYER, p);
    area_blast(pos, radius, &damo, spell_area_pos_hit, s, dam, '*', spell_colour(s));

    /* destroy the damage as the callbacks deliver a copy */
    damage_free(dam);

    return TRUE;
}

gboolean spell_alter_reality(player *p)
{
    map *nlevel;
    position pos = { { 0, 0, Z(p->pos) } };

    if (Z(p->pos) == 0)
        return FALSE;

    /* reset the player's memory of the current map */
    memset(&player_memory_of(p, pos), 0,
           MAP_MAX_Y * MAP_MAX_X * sizeof(player_tile_memory));

    map_destroy(game_map(nlarn, Z(p->pos)));

    /* create new map */
    nlevel = nlarn->maps[Z(p->pos)] = map_new(Z(p->pos), game_mazefile(nlarn));

    /* reposition player (if needed) */
    if (!map_pos_passable(nlevel, p->pos))
    {
        p->pos = map_find_space(nlevel, LE_MONSTER, FALSE);
    }

    return TRUE;
}

gboolean spell_create_monster(struct player *p)
{
    position mpos;

    /* this spell doesn't work in town */
    if (Z(p->pos) == 0)
    {
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }

    /* try to find a space for the monster near the player */
    mpos = map_find_space_in(game_map(nlarn, Z(p->pos)),
                             rect_new_sized(p->pos, 2), LE_MONSTER, FALSE);

    if (pos_valid(mpos))
    {
        monster_new_by_level(mpos);
        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "You feel failure.");
        return FALSE;
    }
}

gboolean spell_create_sphere(spell *s, struct player *p)
{
    g_assert(p != NULL);

    position pos = display_get_new_position(p, p->pos,
            "Where do you want to place the sphere?",
            FALSE, FALSE, FALSE, 0, TRUE, TRUE);

    if (pos_valid(pos))
    {
        sphere *sph = sphere_new(pos, p, p->level * 10 * s->knowledge);
        g_ptr_array_add(nlarn->spheres, sph);

        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "Huh?");

        return FALSE;
    }
}

gboolean spell_cure_poison(struct player *p)
{
    effect *eff = NULL;

    g_assert(p != NULL);

    if ((eff = player_effect_get(p, ET_POISON)))
    {
        player_effect_del(p, eff);
        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "You weren't even poisoned!");
        return FALSE;
    }
}

gboolean spell_cure_blindness(struct player *p)
{
    effect *eff = NULL;

    g_assert(p != NULL);

    if ((eff = player_effect_get(p, ET_BLINDNESS)))
    {
        player_effect_del(p, eff);
        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "You weren't even blinded!");
        return FALSE;
    }
}

gboolean spell_phantasmal_forces(spell *s, struct player *p)
{
    position mpos;
    monster *m = NULL;

    mpos = display_get_position(p, "Choose a target for phantasmal forces.",
                                FALSE, FALSE, 0, TRUE, TRUE);

    if (!pos_valid(mpos))
    {
        return FALSE;
    }

    m = map_get_monster_at(game_map(nlarn, Z(mpos)), mpos);

    if (m == NULL)
    {
        return FALSE;
    }

    if ((player_get_int(p) + s->knowledge) > monster_int(m))
    {
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, spell_msg_succ(s), monster_name(m));
        }

        monster_effect_add(m, effect_new(ET_SCARED));
        return TRUE;
    }
    else
    {
        if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, spell_msg_fail(s), monster_name(m));
        }
        return FALSE;
    }
}

gboolean spell_scare_monsters(spell *s, struct player *p)
{
    monster *m;
    int count = 0;
    position pos = pos_invalid;
    map *cmap = game_map(nlarn, Z(p->pos));
    Z(pos) = Z(p->pos);

    /* the radius of this spell is determined by the player's level of
       spell knowledge */
    area *a = area_new_circle(p->pos, 1 + s->knowledge, FALSE);

    for (int y = a->start_y; y < a->start_y + a->size_y; y++)
    {
        Y(pos) = y;

        for (int x = a->start_x; x < a->start_x + a->size_x; x++)
        {
            X(pos) = x;

            if (!pos_valid(pos))
            {
                /* possibly reached the level boundary */
                continue;
            }

            m = map_get_monster_at(cmap, pos);

            /* no monster at position? */
            if (m == NULL) continue;

            /* there is a monster, check if it is affected */
            if (player_get_int(p) > (int)monster_int(m))
            {
                monster_effect_add(m, effect_new(ET_SCARED));
                count++;
            }
        }
    }

    /* free allocated memory */
    area_destroy(a);

    return (count > 0);
}

gboolean spell_summon_demon(spell *s, struct player *p)
{
    monster *demon;
    position pos;

    /* find a place near the player for the demon servant */
    pos = map_find_space_in(game_map(nlarn, Z(p->pos)),
                            rect_new_sized(p->pos, 2),
                            LE_MONSTER, FALSE);

    if (!pos_valid(pos))
        return FALSE;

    /* generate a demon */
    demon = monster_new(min(MT_DEMONLORD_I + (s->knowledge - 1),
                            MT_DEMONLORD_VII), pos);

    /* turn the demon into a servant */
    monster_update_action(demon, MA_SERVE);

    return TRUE;
}

gboolean spell_make_wall(player *p)
{
    position pos;

    pos = display_get_new_position(p, p->pos,
                                   "Select a position where you want to place a wall.",
                                   FALSE, FALSE, FALSE, 0, FALSE, TRUE);

    if (pos_identical(pos, p->pos))
    {
        log_add_entry(nlarn->log, "You are actually standing there.");
        return FALSE;
    }
    else if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "No wall today.");
        return FALSE;
    }

    map *pmap = game_map(nlarn, Z(p->pos));
    if (map_tiletype_at(pmap, pos) != LT_WALL)
    {
        map_tile *tile = map_tile_at(pmap, pos);

        /* destroy all items at that position */
        if (tile->ilist != NULL)
        {
            inv_destroy(tile->ilist, TRUE);
            tile->ilist = NULL;
        }

        sobject_destroy_at(p, pmap, pos);

        log_add_entry(nlarn->log, "You have created a wall.");

        tile->type = tile->base_type = LT_WALL;

        monster *m;
        if ((m = map_get_monster_at(pmap, pos)))
        {
            if (monster_type(m) != MT_XORN)
            {
                if (monster_in_sight(m))
                {
                    /* briefly display the new monster before it dies */
                    display_paint_screen(nlarn->p);
                    g_usleep(250000);

                    log_add_entry(nlarn->log, "The %s is trapped in the wall!",
                                  monster_get_name(m));
                }

                monster_die(m, nlarn->p);
            }
        }

        return TRUE;
    }
    else
    {
        log_add_entry(nlarn->log, "There was a wall already..");
        return FALSE;
    }
}

gboolean spell_vaporize_rock(player *p)
{
    monster *m;
    position pos;
    map *pmap = game_map(nlarn, Z(p->pos));

    pos = display_get_new_position(p, p->pos,
                                   "What do you want to vaporize?",
                                   FALSE, FALSE, FALSE, 0, FALSE, TRUE);

    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "So you chose not to vaporize anything.");
        return FALSE;
    }

    if (map_tiletype_at(pmap, pos) == LT_WALL)
    {
        map_tiletype_set(pmap, pos, LT_FLOOR);
        p->stats.vandalism++;
    }

    if ((m = map_get_monster_at(pmap, pos)))
    {
        /* xorns take damage from vpr */
        if (monster_type(m) == MT_XORN)
        {
            monster_damage_take(m, damage_new(DAM_PHYSICAL, ATT_NONE,
                                              divert(200, 10), DAMO_PLAYER, p));
        }
        else if (monster_in_sight(m))
        {
            log_add_entry(nlarn->log, "The %s can't be vaporized.",
                          monster_get_name(m));
        }
    }

    sobject_destroy_at(p, pmap, pos);

    return TRUE;
}


char *book_desc(spell_id book_id)
{
    g_assert(book_id < SP_MAX);
    return (char *)book_obfuscation[nlarn->book_desc_mapping[book_id]].desc;
}

int book_weight(item *book)
{
    g_assert (book != NULL && book->type == IT_BOOK);
    return book_obfuscation[nlarn->book_desc_mapping[book->id]].weight;
}

int book_colour(item *book)
{
    g_assert (book != NULL && book->type == IT_BOOK);
    return book_obfuscation[nlarn->book_desc_mapping[book->id]].colour;
}

item_usage_result book_read(struct player *p, item *book)
{
    item_usage_result result = { FALSE, FALSE };
    gchar *desc = item_describe(book, player_item_known(p, book),
                                TRUE, TRUE);

    if (player_effect(p, ET_BLINDNESS))
    {
        log_add_entry(nlarn->log, "As you are blind you can't read %s.",
                      desc);

        g_free(desc);
        return result;
    }

    if (book->cursed && book->blessed_known)
    {
        log_add_entry(nlarn->log, "You'd rather not read this cursed book.");
        g_free(desc);
        return result;
    }

    log_add_entry(nlarn->log, "You start reading %s.", desc);

    /*
     * Try to complete reading the book.
     * Reading a book takes ten turns per spell level.
     */
    if (!player_make_move(p, 10 * spell_level_by_id(book->id),
                          TRUE, "reading %s", desc))
    {
        /* the action has been aborted */
        g_free(desc);
        return result;
    }

    g_free(desc);

    /* the book has successfully been read - increase number of books read */
    p->stats.books_read++;

    /* cursed spell books have nasty effects */
    if (book->cursed)
    {
        log_add_entry(nlarn->log, "There was something wrong with this book! " \
                      "It crumbles to dust.");

        player_mp_lose(p, rand_0n(p->mp));
        result.used_up = TRUE;
    }
    else
    {
        switch (spell_learn(p, book->id))
        {
        case 0:
            log_add_entry(nlarn->log, "You cannot understand the content of this book.");
            // Bad pun.
            if (strcmp(book_desc(book->id), "dull") == 0)
                log_add_entry(nlarn->log, "It seems really boring, though.");
            break;

        case 1:
            /* learnt spell */
            log_add_entry(nlarn->log, "You master the spell %s.", book_name(book));

            result.used_up = TRUE;
            result.identified = TRUE;
            break;

        default:
            /* improved knowledge of spell */
            log_add_entry(nlarn->log, "You improved your knowledge of the spell %s.",
                          book_name(book));

            result.used_up = TRUE;
            result.identified = TRUE;
            break;
        }

        /* five percent chance to increase intelligence */
        if (result.used_up && chance(2))
        {
            log_add_entry(nlarn->log, "Reading makes you ingenious.");
            p->intelligence++;
        }
    }

    return result;
}

static int spell_cast(player *p, spell *s)
{
    int turns = 0;
    gboolean well_done = FALSE;

    /* insufficient mana */
    if (p->mp < spell_level(s))
    {
        log_add_entry(nlarn->log, "You lack the power to cast %s.",
                      spell_name(s));

        return 0;
    }
    else if (spell_success_value(p, s) < 1)
    {
        log_add_entry(nlarn->log, "This spell is too difficult for you.");
        return 0;
    }

    log_add_entry(nlarn->log, "You cast %s.", spell_name(s));

    /* time usage */
    turns = 1;

    /* bad luck, low intelligence */
    if (chance(1) || spell_success_value(p, s) < (int)rand_1n(16))
    {
        log_add_entry(nlarn->log, "It didn't work!");
        player_mp_lose(p, spell_level(s));

        return turns;
    }

    switch (spell_type(s))
    {
        /* spells that cause an effect on the player */
    case SC_PLAYER:
        well_done = spell_type_player(s, p);
        break;

        /* spells that cause an effect on a monster */
    case SC_POINT:
        well_done = spell_type_point(s, p);
        break;

        /* creates a ray */
    case SC_RAY:
        well_done = spell_type_ray(s, p);
        break;

        /* effect pours like water */
    case SC_FLOOD:
        well_done = spell_type_flood(s, p);
        break;

        /* effect occurs like an explosion */
    case SC_BLAST:
        well_done = spell_type_blast(s, p);
        break;

    case SC_OTHER:  /* unclassified */

        switch (s->id)
        {
            /* cure poison */
        case SP_CPO:
            well_done = spell_cure_poison(p);
            break;

            /* cure blindness */
        case SP_CBL:
            well_done = spell_cure_blindness(p);
            break;

            /* create monster */
        case SP_CRE:
            well_done = spell_create_monster(p);
            break;

            /* phantasmal forces */
        case SP_PHA:
            well_done = spell_phantasmal_forces(s, p);
            break;

            /* vaporize rock */
        case SP_VPR:
            well_done = spell_vaporize_rock(p);
            break;

            /* scare monsters */
        case SP_SCA:
            well_done = spell_scare_monsters(s, p);
            break;

            /* make wall */
        case SP_MKW:
            well_done = spell_make_wall(p);
            break;

            /* sphere of annihilation */
        case SP_SPH:
            well_done = spell_create_sphere(s, p);
            break;

            /* summon demon */
        case SP_SUM:
            well_done = spell_summon_demon(s, p);
            break;

            /* alter reality */
        case SP_ALT:
            well_done = spell_alter_reality(p);
            if (!well_done)
                log_add_entry(nlarn->log, spell_msg_fail_by_id(s->id));
            break;

        default:
            /* this shouldn't happen */
            break;
        }
        break;

    case SC_MAX:
        log_add_entry(nlarn->log, "Internal Error in %s:%d.", __FILE__, __LINE__);
        break;
    }

    if (!well_done)
        return 0;

    if (well_done)
    {
        /* spell has been cast successfully, set mp usage accordingly */
        player_mp_lose(p, spell_level(s));

        /* increase number of spells cast */
        p->stats.spells_cast++;

        /* increase usage counter for this specific spell */
        s->used++;
    }

    return turns;
}

static void spell_print_success_message(spell *s, monster *m)
{
    g_assert(s != NULL && m != NULL);

    /* invisible monster -> no message */
    if (!monster_in_sight(m))
    {
        log_add_entry(nlarn->log, "You think you've hit something.");
        return;
    }

    /* no message defined */
    if (spell_msg_succ(s) == NULL)
        return;

    log_add_entry(nlarn->log, spell_msg_succ(s), monster_get_name(m));
}

static void spell_print_failure_message(spell *s, monster *m)
{
    g_assert(s != NULL && m != NULL);

    /* invisible monster -> no message */
    if (!monster_in_sight(m))
        return;

    /* no message defined */
    if (spell_msg_fail(s) == NULL)
        return;

    log_add_entry(nlarn->log, spell_msg_fail(s), monster_get_name(m));
}

static int count_adjacent_water_squares(position pos)
{
    position p = pos_invalid;
    Z(p) = Z(pos);

    int count = 0;
    for (X(p) = X(pos) - 1; X(p) <= X(pos) + 1; X(p)++)
        for (Y(p) = Y(pos) - 1; Y(p) <= Y(pos) + 1; Y(p)++)
        {
            if (!pos_valid(p))
                continue;

            if (pos_identical(p, pos))
                continue;

            const map_tile *tile = map_tile_at(game_map(nlarn, Z(pos)), p);
            if (tile->type == LT_WATER || tile->type == LT_DEEPWATER)
                count++;
        }

    return count;
}

static int try_drying_ground(position pos)
{
    map_tile *tile = map_tile_at(game_map(nlarn, Z(pos)), pos);
    if (tile->type == LT_DEEPWATER)
    {
        /* success chance depends on number of adjacent water squares */
        const int adj_water = count_adjacent_water_squares(pos);
        if ((int)rand_1n(9) <= adj_water)
        {
            log_add_entry(nlarn->log, "Nothing happens.");
            return FALSE;
        }

        tile->type = LT_WATER;
        log_add_entry(nlarn->log, "The water is more shallow now.");
        return TRUE;
    }
    else if (tile->type == LT_WATER)
    {
        /* success chance depends on number of adjacent water squares */
        const int adj_water = count_adjacent_water_squares(pos);
        if ((int)rand_1n(9) <= adj_water)
        {
            log_add_entry(nlarn->log, "Nothing happens.");
            return FALSE;
        }

        if (tile->base_type == LT_NONE)
            tile->type = LT_DIRT;
        else
            tile->type = tile->base_type;

        if (tile->timer)
            tile->timer = 0;

        log_add_entry(nlarn->log, "The water evaporates!");
        return TRUE;
    }
    return FALSE;
}

static gboolean spell_traj_pos_hit(const GList *traj,
        const damage_originator *damo,
        gpointer data1, gpointer data2)
{
    position pos;
    pos_val(pos) = GPOINTER_TO_UINT(traj->data);

    return spell_area_pos_hit(pos, damo, data1, data2);
}

static gboolean spell_area_pos_hit(position pos,
        const damage_originator *damo,
        gpointer data1, gpointer data2)
{
    spell *sp = (spell *)data1;
    damage *dam = (damage *)data2;
    map *cmap = game_map(nlarn, Z(pos));
    sobject_t mst = map_sobject_at(cmap, pos);
    monster *m = map_get_monster_at(cmap, pos);
    item_erosion_type iet;
    gboolean terminated = FALSE;

    /* determine if the spell causes item erosion */
    switch (sp->id)
    {
    case SP_BAL:
        iet = IET_BURN;
        break;

    default:
        iet = IET_NONE;
        break;
    }

    /* The spell hit a sobject. */
    if (mst > LS_NONE)
    {
        if (mst == LS_MIRROR && fov_get(nlarn->p->fv, pos))
        {
            /* reflection is handled in map_trajectory, but we need
               to generate a message here if the mirror is visible */
            log_add_entry(nlarn->log, "The mirror reflects the %s!",spell_name(sp));
            return terminated;
        }

        if (mst == LS_STATUE
            && (sp->id == SP_BAL || sp->id == SP_LIT)
            && (game_difficulty(nlarn) <= 2))
        {
        /* fireball and lightning destroy statues up to diff. level 2 */
            sobject_destroy_at(damo->originator, cmap, pos);
            terminated = TRUE;
        }

        if (mst == LS_CLOSEDDOOR && (spell_level(sp) > 2))
        {
            /* Blast the door away */
            sobject_destroy_at(damo->originator, cmap, pos);
            terminated = TRUE;
        }
    }

    /* The spell hit a monster */
    if (m != NULL)
    {
        spell_print_success_message(sp, m);

        /* erode the monster's inventory */
        if (iet > IET_NONE)
            inv_erode(monster_inv(m), iet, FALSE, NULL);

        monster_damage_take(m, damage_copy(dam));

        /*
         * If the monster is at least of human size, the spell stops at
         * the monster, otherwise it passes and may hit other monsters
         */
        if (monster_size(m) >= ESIZE_MEDIUM)
            terminated = TRUE;
    }

    /* The spell hit the player */
    if (pos_identical(nlarn->p->pos, pos))
    {
        if (player_effect(nlarn->p, ET_REFLECTION))
        {
            /* The player reflects the spell. Actual handling of the reflection
               is done in map_trajectory, just give a message here. */
            log_add_entry(nlarn->log, "Your amulet reflects the %s!", spell_name(sp));
        }
        else
        {
            int evasion = nlarn->p->level/(2+game_difficulty(nlarn)/2)
                          + player_get_dex(nlarn->p)
                          - 10
                          - game_difficulty(nlarn);

            // Automatic hit if paralysed or overstrained.
            if (player_effect(nlarn->p, ET_PARALYSIS)
                || player_effect(nlarn->p, ET_OVERSTRAINED))
                evasion = 0;
            else
            {
                if (player_effect(nlarn->p, ET_BLINDNESS))
                    evasion /= 4;
                if (player_effect(nlarn->p, ET_CONFUSION))
                    evasion /= 2;
                if (player_effect(nlarn->p, ET_BURDENED))
                    evasion /= 2;
            }

            if (evasion >= (int)rand_1n(21))
            {
                if (!player_effect(nlarn->p, ET_BLINDNESS))
                {
                    log_add_entry(nlarn->log, "The %s whizzes by you!", spell_name(sp));
                }

                /* missed */
                terminated = FALSE;
            }
            else
            {
                log_add_entry(nlarn->log, "The %s hits you!", spell_name(sp));
                player_damage_take(nlarn->p, damage_copy(dam), PD_SPELL, sp->id);

                /* erode the player's inventory */
                if (iet > IET_NONE)
                {
                    /*
                     * Filter equipped and exposed items, e.g.
                     * a body armour will not be affected by erosion
                     * when the player wears a cloak over it.
                     */
                    inv_erode(&(nlarn->p->inventory), iet, TRUE,
                            player_item_filter_unequippable);
                }

                /* hit */
                terminated = TRUE;
            }
        } /* The spell wasn't reflected */
    } /* The spell hit the player's position */

    if (iet > IET_NONE && map_ilist_at(cmap, pos))
    {
        /* there are items at the given map position, erode them */
        inv_erode(map_ilist_at(cmap, pos), iet,
                fov_get(nlarn->p->fv, pos), NULL);
    }

    return terminated;
}
