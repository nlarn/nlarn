/*
 * spells.c
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

#include <assert.h>
#include <string.h>

#include "display.h"
#include "nlarn.h"
#include "spells.h"
#include "spheres.h"

const spell_data spells[SP_MAX] =
{
    {
        SP_NONE, NULL, NULL,
        SC_NONE, DAM_NONE, ET_NONE,
        NULL, NULL, NULL,
        0, 0
    },
    {
        SP_PRO, "pro","protection",
        SC_PLAYER, DAM_NONE, ET_PROTECTION,
        "Generates a protection field.",
        NULL, NULL,
        1, 260
    },
    {
        SP_MLE, "mle", "magic missile",
        SC_RAY, DAM_MAGICAL, ET_NONE,
        "Creates and hurls a magic missile equivalent to a + 1 magic arrow.",
        "Your missiles hit the %s.",
        "Your missile bounces off the %s.",
        1, 320
    },
    {
        SP_DEX, "dex", "dexterity",
        SC_PLAYER, DAM_NONE, ET_INC_DEX,
        "Improves the casters dexterity.",
        NULL, NULL,
        1, 260
    },
    {
        SP_SLE, "sle", "sleep",
        SC_POINT, DAM_NONE, ET_SLEEP,
        "Causes some monsters to go to sleep.",
        NULL,
        "The %s doesn't sleep.",
        1, 260
    },
    {
        SP_CHM, "chm", "charm monster",
        SC_PLAYER, DAM_NONE, ET_CHARM_MONSTER,
        "Some monsters may be awed at your magnificence.",
        NULL, NULL,
        1, 260
    },
    {
        SP_SSP, "ssp", "sonic spear",
        SC_RAY, DAM_PHYSICAL, ET_NONE,
        "Causes your hands to emit a screeching sound toward what they point.",
        "The sound damages the %s.",
        "The %s can't hear the noise.",
        1, 300
    },
    {
        SP_STR, "str", "strength",
        SC_PLAYER, DAM_NONE, ET_INC_STR,
        "Increase the casters strength for a short term.",
        NULL, NULL,
        2, 460
    },
    {
        SP_CPO, "cpo", "cure poison",
        SC_OTHER, DAM_NONE, ET_NONE,
        "The caster is cured from poison.",
        NULL, NULL,
        2, 460
    },
    {
        SP_HEL, "hel", "healing",
        SC_PLAYER, DAM_NONE, ET_INC_HP,
        "Restores some HP to the caster.",
        NULL, NULL,
        2, 400
    },
    {
        SP_CBL, "cbl", "cure blindness",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Restores sight to one so unfortunate as to be blinded.",
        NULL, NULL,
        2, 400
    },
    {
        SP_CRE, "cre", "create monster",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Creates a monster near the caster appropriate for the location.",
        NULL, NULL,
        2, 400
    },
    {
        SP_PHA, "pha", "phantasmal forces",
        SC_POINT, DAM_NONE, ET_SCARE_MONSTER,
        "Creates illusions, and if believed, monsters flee.",
        "The %s believed!",
        "The %s didn't believe the illusions!",
        2, 600
    },
    {
        SP_INV, "inv", "invisibility",
        SC_PLAYER, DAM_NONE, ET_INVISIBILITY,
        "The caster becomes invisible.",
        NULL, NULL,
        2, 600
    },
    {
        SP_BAL, "bal", "fireball",
        SC_BLAST, DAM_FIRE, ET_NONE,
        "Makes a ball of fire that burns on what it hits.",
        "The fireball hits the %s.",
        NULL,
        3, 1200
    },
    {
        SP_CLD, "cld", "cone of cold",
        SC_RAY, DAM_COLD, ET_NONE,
        "Sends forth a cone of cold which freezes what it touches.",
        "The cone of cold strikes the %s.",
        "The %s loves the cold!",
        3, 1200
    },
    {
        SP_PLY, "ply", "polymorph",
        SC_POINT, DAM_NONE, ET_NONE,
        "You can find out what this does for yourself.",
        NULL,
        "The %s resists.",
        3, 950
    },
    {
        SP_CAN, "can", "cancellation",
        SC_PLAYER, DAM_NONE, ET_CANCELLATION,
        "Negates the ability of a monster to use his special abilities.",
        NULL, NULL,
        3, 950
    },
    {
        SP_HAS, "has", "haste self",
        SC_PLAYER, DAM_NONE, ET_SPEED,
        "Speeds up the casters movements.",
        NULL, NULL,
        3, 950
    },
    {
        SP_CKL, "ckl", "killing cloud",
        SC_FLOOD, DAM_ACID, ET_NONE,
        "Creates a fog of poisonous gas which kills all that is within it.",
        "The %s gasps for air.",
        "The %s loves the water!",
        3, 1200
    },
    {
        SP_VPR, "vpr", "vaporize rock",
        SC_OTHER, DAM_NONE, ET_NONE,
        "This changes rock to air.",
        NULL, NULL,
        3, 950
    },
    {
        SP_DRY, "dry", "dehydration",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "Dries up water in the immediate vicinity.",
        "The %s shrivels up.",
        "The %s isn't affected.",
        4, 1600
    },
    {
        SP_LIT, "lit", "lightning",
        SC_RAY, DAM_ELECTRICITY, ET_NONE,
        "You finger will emit a lightning bolt when this spell is cast.",
        "A lightning bolt hits the %s.",
        "The %s loves fire and lightning!",
        4, 1600
    },
    {
        SP_DRL, "drl", "drain life",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "Subtracts hit points from both you and a monster.",
        NULL, NULL,
        4, 1400
    },
    {
        SP_GLO, "glo", "invulnerability",
        SC_PLAYER, DAM_NONE, ET_INVULNERABILITY,
        "This globe helps to protect the player from physical attack.",
        NULL, NULL,
        4, 1400
    },
    {
        SP_FLO, "flo", "flood",
        SC_FLOOD, DAM_WATER, ET_NONE,
        "This creates an avalanche of H2O to flood the immediate chamber.",
        "The %s struggles for air in your flood!",
        NULL,
        4, 1600
    },
    {
        SP_FGR, "fgr", "finger of death",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "This is a holy spell and calls upon your god to back you up.",
        "The %s's heart stopped.",
        "The %s isn't affected.",
        4, 1600
    },
    {
        SP_SCA, "sca", "scare monster",
        SC_POINT, DAM_NONE, ET_SCARE_MONSTER,
        "Terrifies the monster so that hopefully he wont hit the magic user.",
        NULL, NULL,
        5, 2000
    },
    {
        SP_HLD, "hld", "hold monster",
        SC_POINT, DAM_NONE, ET_HOLD_MONSTER,
        "The monster is frozen in his tracks if this is successful.",
        NULL, NULL,
        5, 2000
    },
    {
        SP_STP, "stp", "time stop",
        SC_OTHER, DAM_NONE, ET_TIMESTOP,
        "All movement in the caverns ceases for a limited duration.",
        NULL, NULL,
        5, 2500
    },
    {
        SP_TEL, "tel", "teleport away",
        SC_POINT, DAM_NONE, ET_NONE,
        "Moves a particular monster around in the dungeon.",
        NULL, NULL,
        5, 2000
    },
    {
        SP_MFI, "mfi", "magic fire",
        SC_FLOOD, DAM_FIRE, ET_NONE,
        "This causes a curtain of fire to appear all around you.",
        "The %s cringes from the flame.",
        NULL,
        5, 2500
    },
    {
        SP_MKW, "mkw", "make wall",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Makes a wall in the specified place.",
        NULL, NULL,
        6, 3000
    },
    {
        SP_SPH, "sph", "sphere of annihilation",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Anything caught in this sphere is instantly killed.",
        NULL, NULL,
        6, 3500
    },
    {
        SP_GEN,  "gen", "genocide",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Eliminates a species of monster from the caverns.",
        NULL, NULL,
        6, 3800
    },
    {
        SP_SUM, "sum", "summon demon",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Summons a demon who hopefully helps you out.",
        NULL, NULL,
        6, 3500
    },
    {
        SP_WTW, "wtw", "walk through walls",
        SC_PLAYER, DAM_NONE, ET_WALL_WALK,
        "Allows the caster to walk through walls for a short period of time.",
        NULL, NULL,
        6, 3800
    },
    {
        SP_ALT, "alt", "alter reality",
        SC_OTHER, DAM_NONE, ET_NONE,
        "God only knows what this will do.",
        NULL,
        "Polinneaus won't let you mess with his dungeon!",
        6, 3800
    },
};

static const char *book_descriptions[SP_MAX - 1] =
{
    "parchment-bound",
    "thick", /* 1 */
    "dusty",
    "leather-bound",
    "heavy", /* 4 */
    "ancient",
    "buckram",
    "gilded",
    "embossed",
    "old",
    "thin", /* 10 */
    "light", /* 11 */
    "large", /* 12 */
    "vellum",
    "tan",
    "papyrus",
    "linen",
    "musty",
    "faded",
    "antique",
    "worn out",
    "tattered",
    "aged",
    "ornate",
    "inconspicuous",
    "awe-inspiring",
    "stained",
    "mottled",
    "plaid",
    "wax-lined",
    "bamboo",
    "clasped",
    "ragged",
    "dull",
    "canvas",
    "well-thumbed",
    "chambray",
};


spell *spell_new(int id)
{
    spell *nspell;

    assert(id > SP_NONE && id < SP_MAX);

    nspell = g_malloc0(sizeof(spell));
    nspell->id = id;
    nspell->knowledge = 1;

    return nspell;
}

void spell_destroy(spell *s)
{
    assert(s != NULL);
    g_free(s);
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

/**
 * Select a spell to cast and cast it
 * @param the player
 * @return number of turns elapsed
 */
int spell_cast(player *p)
{
    int turns = 0;
    int mp_usage = 0;
    gboolean well_done = FALSE;

    spell *spell;

    if (player_effect(p, ET_CONFUSION))
    {
        log_add_entry(p->log, "You can't aim your magic!");
        return turns;
    }

    spell = display_spell_select("Select a spell to cast", p);

    /* ESC pressed */
    if (!spell)
    {
        return turns;
    }

    /* insufficient mana */
    if (p->mp < spell_level(spell))
    {
        log_add_entry(p->log, "You lack the power to cast %s.",
                      spell_name(spell));

        return turns;
    }

    log_add_entry(p->log, "You cast %s.", spell_name(spell));

    /* time usage */
    turns = 1;

    /* bad luck */
    if (chance(5) || rand_1n(18) > player_get_int(p))
    {
        log_add_entry(p->log, "It didn't work!");
        player_mp_lose(p, spell_level(spell));

        return turns;
    }

    switch (spell_type(spell))
    {
        /* spells that cause an effect on the player */
    case SC_PLAYER:
        mp_usage = spell_type_player(spell, p);
        break;

        /* spells that cause an effect on a monster */
    case SC_POINT:
        mp_usage = spell_type_point(spell, p);
        break;

        /* creates a ray */
    case SC_RAY:
        mp_usage = spell_type_ray(spell, p);
        break;

        /* effect pours like water */
    case SC_FLOOD:
        mp_usage = spell_type_flood(spell, p);
        break;

        /* effect occurs like an explosion */
    case SC_BLAST:
        mp_usage = spell_type_blast(spell, p);
        break;

    case SC_OTHER:  /* unclassified */

        switch (spell->id)
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

            /* time stop */
        case SP_STP:
            /* TODO: implement (ticket 39) */
            break;

            /* vaporize rock */
        case SP_VPR:
            well_done = spell_vaporize_rock(p);
            break;

            /* make wall */
        case SP_MKW:
            well_done = spell_make_wall(p);
            break;

            /* sphere of annihilation */
        case SP_SPH:
            well_done = spell_create_sphere(spell, p);
            break;

            /* genocide */
        case SP_GEN:
            well_done = spell_genocide_monster(p);
            break;

            /* summon daemon */
        case SP_SUM:
            /* TODO: implement (ticket 55) */
            break;

            /* alter realitiy */
        case SP_ALT:
            well_done = spell_alter_reality(p);
            break;
        }

        /* spell has been casted successfully, set mp usage accordingly */
        if (well_done)
        {
            mp_usage = spell_level(spell);
        }

        break;

    case SC_NONE:
    case SC_MAX:
        log_add_entry(p->log, "internal Error in %s:%d.", __FILE__, __LINE__);
        break;
    }

    if (mp_usage > 0)
    {
        /* charge mana */
        player_mp_lose(p, mp_usage);
        /* statistics */
        p->stats.spells_cast++;
    }

    return turns;
}

/**
 * Try to add a spell to the list of known spells
 *
 * @param the player
 * @param id of spell to learn
 * @return FALSE if learning the spell failed, otherwise level of knowledge
 */
int spell_learn(player *p, guint spell_type)
{
    spell *s;
    guint idx;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    if (!spell_known(p, spell_type))
    {
        s = spell_new(spell_type);
        s->learnt = game_turn(nlarn);

        /* TODO: add a check for intelligence */
        if (spell_level(s) > (int)p->lvl)
        {
            /* spell is beyond the players scope */
            spell_destroy(s);
            return FALSE;
        }

        g_ptr_array_add(p->known_spells, s);
        return s->knowledge;
    }
    else
    {
        /* spell already known, improve knowledge */
        for (idx = 0; idx < p->known_spells->len; idx++)
        {
            /* search spell */
            s = (spell *)g_ptr_array_index(p->known_spells, idx);

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

/**
 * Remove a spell from the list of known spells
 *
 * @param the player
 * @param the id of the spell to forget
 * @return TRUE if the spell could be found and removed, othrwise FALSE
 */
int spell_forget(player *p, guint spell_type)
{
    spell *s;
    guint idx;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    for (idx = 0; idx < p->known_spells->len; idx++);
    {
        s = g_ptr_array_index(p->known_spells, idx);
        if (s->id == spell_type)
        {
            g_ptr_array_remove_index_fast(p->known_spells, idx);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if a spell is known to the player
 * @param the player
 * @param id of the spell in question
 * @return FALSE if unknown, otherwise level of knowledge of that spell
 */
int spell_known(player *p, guint spell_type)
{
    spell *s;
    guint idx;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    for (idx = 0; idx < p->known_spells->len; idx++)
    {
        s = g_ptr_array_index(p->known_spells, idx);
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

    assert(s != NULL && p != NULL && (spell_type(s) == SC_PLAYER));

    e = effect_new(spell_effect(s), game_turn(nlarn));

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

    if (e->type == ET_WALL_WALK)
    {
        e->turns = 6 + rand_1n(10);
    }

    player_effect_add(p, e);

    return (spell_level(s));
}

int spell_type_point(spell *s, struct player *p)
{
    monster *monster = NULL;
    position pos;
    effect *e;
    char buffer[61];
    int amount = 0;

    assert(s != NULL && p != NULL && (spell_type(s) == SC_POINT));

    g_snprintf(buffer, 60, "Select a target for %s.", spell_name(s));

    pos = display_get_position(p, buffer, FALSE, FALSE);

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(p->log, "Aborted.");
        return FALSE;
    }

    monster = map_get_monster_at(p->map, pos);

    if (!monster)
    {
        log_add_entry(p->log, "Which monster are you talking about?");
        return FALSE;
    }

    switch (s->id)
    {
        /* dehydration */
    case SP_DRY:
        amount = (100 * s->knowledge) + p->lvl;
        monster_damage_take(monster, damage_new(DAM_MAGICAL, amount, p));
        break; /* SP_DRY */

        /* drain life */
    case SP_DRL:
        amount = min(p->hp - 1, (int)p->hp_max / 2);

        monster_damage_take(monster, damage_new(DAM_MAGICAL, amount, p));
        player_damage_take(p, damage_new(DAM_MAGICAL, amount, NULL), PD_SPELL, SP_DRL);

        break; /* SP_DRL */

        /* finger of death */
    case SP_FGR:
        if (chance(1))
        {
            player_die(p, PD_SPELL, SP_FGR);
        }

        if ((player_get_wis(p) + s->knowledge) > rand_m_n(10,20))
        {
            monster_damage_take(monster, damage_new(DAM_MAGICAL, 2000, p));
        }
        else
        {
            log_add_entry(p->log, "It didn't work.");
        }

        break; /* SP_FGR */

        /* polymorph */
    case SP_PLY:
        monster_polymorph(monster);
        break;

        /* teleport */
    case SP_TEL:
        log_add_entry(p->log, "The %s disappears.",
                      monster_name(monster));

        monster_set_pos(monster, p->map, map_find_space(p->map, LE_MONSTER));

        break; /* SP_TEL */

    default:
        /* spell has an effect, add that to the monster */
        assert(spell_effect(s) != ET_NONE);

        if (spell_msg_succ(s))
        {
            log_add_entry(p->log, spell_msg_succ(s),
                          monster_name(monster));
        }

        e = effect_new(spell_effect(s), game_turn(nlarn));

        if (!e->amount)
        {
            e->amount = p->intelligence;
        }

        e->amount *= s->knowledge;

        /* show message if monster is visible */
        if (monster_in_sight(monster) && effect_get_msg_m_start(e)
                && !monster_effect(monster, e->type))
        {
            log_add_entry(p->log, effect_get_msg_m_start(e),
                          monster_name(monster));
        }

        /* has to come in the end as e might be destroyed */
        monster_effect_add(monster, e);

        break;
    }

    return spell_level(s);
}

int spell_type_ray(spell *s, struct player *p)
{
    monster *monster = NULL;
    position pos;
    char buffer[61];
    int amount = 0;

    assert(s != NULL && p != NULL && (spell_type(s) == SC_RAY));

    g_snprintf(buffer, 60, "Select a target for the %s.", spell_name(s));
    pos = display_get_position(p, buffer, TRUE, TRUE);

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(p->log, "Aborted.");
        return FALSE;
    }

    if (!(monster = map_get_monster_at(p->map, pos)))
    {
        log_add_entry(p->log, "Which monster are you talking about?");
        return FALSE;
    }

    switch (s->id)
    {
    case SP_MLE:
        amount = rand_1n(((p->lvl + 1) << s->knowledge)) + p->lvl + 3;
        break;

    case SP_SSP:
        amount = rand_1n(10) + (15 * s->knowledge) + p->lvl;
        break;

    case SP_CLD:
        amount = rand_1n(25) + (20 * s->knowledge) + p->lvl;
        break;

    case SP_LIT:
        amount = rand_1n(25) + (20 * s->knowledge) + (p->lvl << 1);
        break;
    }

    if (map_stationary_at(p->map, pos) == LS_MIRROR)
    {
        log_add_entry(p->log, "The mirror reflects your spell! The %s hits you!",
                      spell_name(s));

        player_damage_take(p, damage_new(spell_damage(s), amount, NULL), PD_SPELL, s->id);
    }

    log_add_entry(p->log, spell_msg_succ(s), monster_name(monster));

    /* FIXME: get all monsters in the ray and affect them all */
    monster_damage_take(monster, damage_new(spell_damage(s), amount, p));

    return spell_level(s);
}

int spell_type_flood(spell *s, struct player *p)
{
    position pos;
    area *range = NULL;
    map_tile_t type = LT_NONE;
    int radius = 0;
    int amount = 0;
    char buffer[81];

    assert(s != NULL && p != NULL && (spell_type(s) == SC_FLOOD));

    g_snprintf(buffer, 60, "Where do you want to place the %s?", spell_name(s));
    pos = display_get_position(p, buffer, FALSE, TRUE);

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(p->log, "Aborted.");
        return FALSE;
    }

    switch (s->id)
    {
    case SP_CKL:
        radius = 3;
        type = LT_CLOUD;
        amount = (10 * s->knowledge) + p->lvl;
        break;

    case SP_FLO:
        radius = 4;
        type = LT_WATER;
        amount = (25 * s->knowledge) + p->lvl;
        break;

    case SP_MFI:
        radius = 4;
        type = LT_FIRE;
        amount = (15 * s->knowledge) + p->lvl;
        break;
    }

    range = area_new_circle_flooded(pos, radius,
                                    map_get_obstacles(p->map, pos, radius));

    map_set_tiletype(p->map, range, type, amount);
    area_destroy(range);

    return spell_level(s);
}

int spell_type_blast(spell *s, struct player *p)
{
    GPtrArray *mlist;
    monster *monster = NULL;
    position pos;
    char buffer[61];
    int amount;
    guint idx;
    damage *dam;

    assert(s != NULL && p != NULL && (spell_type(s) == SC_BLAST));

    g_snprintf(buffer, 60, "Point to the center of the %s.", spell_name(s));
    pos = display_get_position(p, buffer, FALSE, TRUE);

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(p->log, "Aborted.");
        return FALSE;
    }

    /* currently only fireball */
    amount = (25 * s->knowledge) + p->lvl + rand_0n(25 + p->lvl);

    mlist = map_get_monsters_in(p->map, rect_new_sized(pos, 1));

    for (idx = 0; idx < mlist->len; idx++)
    {
        monster = g_ptr_array_index(mlist, idx);
        dam = damage_new(DAM_FIRE, amount, p);

        monster_damage_take(monster, dam);
    }

    if (pos_in_rect(p->pos, rect_new_sized(pos, 1)))
    {
        /* player has been hit by the blast as well */
        log_add_entry(p->log, "The fireball hits you.");

        dam = damage_new(DAM_FIRE, amount, NULL);
        player_damage_take(p, dam, PD_SPELL, SP_BAL);
    }

    g_ptr_array_free(mlist, FALSE);

    return TRUE;
}

gboolean spell_alter_reality(player *p)
{
    map *nlevel, *olevel;

    olevel = p->map;

    /* create new map */
    nlevel = map_new(olevel->nlevel, game_mazefile(nlarn));

    /* make new map active */
    nlarn->maps[p->map->nlevel] = nlevel;
    p->map = nlevel;

    /* reposition player (if needed) */
    if (!map_pos_passable(nlevel, p->pos))
    {
        p->pos = map_find_space(nlevel, LE_MONSTER);
    }

    /* destroy old map */
    map_destroy(olevel);

    return TRUE;
}

gboolean spell_create_monster(struct player *p)
{
    monster *m;

    position pos;

    /* this spell doesn't work in town */
    if (p->map->nlevel == 0)
    {
        log_add_entry(p->log, "Nothing happens.");
        return FALSE;
    }

    /* try to find a space for the monster near the player */
    pos = map_find_space_in(game_map(nlarn, p->pos.z),
                              rect_new_sized(p->pos, 2),
                              LE_MONSTER);

    if (pos_valid(pos))
    {
        m = monster_new_by_level(p->pos.z);
        monster_set_pos(m, game_map(nlarn, p->pos.z), pos);

        return TRUE;
    }
    else
    {
        log_add_entry(p->log, "You feel failure.");
        return FALSE;
    }
}

gboolean spell_create_sphere(spell *s, struct player *p)
{
    position pos;
    sphere *sphere;

    assert(p != NULL);

    pos = display_get_position(p, "Where do you want to place the sphere?",
                               FALSE, TRUE);

    if (pos_valid(pos))
    {
        sphere = sphere_new(pos, p, p->lvl * 10 * s->knowledge);
        g_ptr_array_add(p->map->slist, sphere);

        return TRUE;
    }
    else
    {
        log_add_entry(p->log, "Huh?");

        return FALSE;
    }
}

gboolean spell_cure_poison(struct player *p)
{
    effect *eff = NULL;

    assert(p != NULL);

    if ((eff = player_effect_get(p, ET_POISON)))
    {
        player_effect_del(p, eff);
        return TRUE;
    }
    else
    {
        log_add_entry(p->log, "You weren't even poisoned!");
        return FALSE;
    }
}

gboolean spell_cure_blindness(struct player *p)
{
    effect *eff = NULL;

    assert(p != NULL);

    if ((eff = player_effect_get(p, ET_BLINDNESS)))
    {
        player_effect_del(p, eff);
        return TRUE;
    }
    else
    {
        log_add_entry(p->log, "You weren't even blinded!");
        return FALSE;
    }
}

gboolean spell_genocide_monster(player *p)
{
    char *in;
    int id;

    assert(p != NULL);

    display_paint_screen(p);
    in = display_get_string("Which monster do you want to genocide (type letter)?", NULL, 1);

    if (!in)
    {
        log_add_entry(p->log, "You chose not to genocide any monster.");
        return FALSE;
    }

    for (id = 1; id < MT_MAX; id++)
    {
        if (monster_image_by_type(id) == in[0])
        {
            if (!monster_is_genocided(id))
            {
                monster_genocide(id);
                log_add_entry(p->log, "Wiped out all %ss.",
                              monster_name_by_type(id));

                monsters_genocide(p->map);

                g_free(in);

                return TRUE;
            }
        }
    }

    g_free(in);

    log_add_entry(p->log, "No such monster.");
    return FALSE;
}

gboolean spell_make_wall(player *p)
{
    position pos;

    pos = display_get_position(p, "Select a position where you want to place a wall.", FALSE, TRUE);

    if (pos_identical(pos, p->pos))
    {
        log_add_entry(p->log, "You are actually standing there.");
        return FALSE;
    }
    else if (!pos_valid(pos))
    {
        log_add_entry(p->log, "No wall today.");
        return FALSE;
    }

    if (map_tiletype_at(p->map, pos) != LT_WALL)
    {
        map_tile *tile = map_tile_at(p->map, pos);

        tile->type = tile->base_type = LT_WALL;

        /* destroy all items at that position */
        if (tile->ilist != NULL)
        {
            inv_destroy(tile->ilist);
            tile->ilist = NULL;
        }

        log_add_entry(p->log, "You have created a wall.");

        return TRUE;
    }
    else
    {
        log_add_entry(p->log, "There was a wall already..");
        return FALSE;
    }
}

gboolean spell_vaporize_rock(player *p)
{
    position pos;
    monster *m = NULL;
    char *desc = NULL;
    map_tile *tile;

    pos = display_get_position(p, "What do you want to vaporize?", FALSE, FALSE);

    if (!pos_valid(pos))
    {
        log_add_entry(p->log, "So you chose not to vaprize anything.");
        return FALSE;
    }

    tile = map_tile_at(p->map, pos);

    if (tile->type == LT_WALL)
    {
        tile->type = LT_FLOOR;
    }

    if ((m = map_get_monster_at(p->map, pos)) && (monster_type(m) == MT_XORN))
    {
        /* xorns take damage from vpr */
        monster_damage_take(m, damage_new(DAM_PHYSICAL, divert(200, 10), p));
    }

    switch (tile->stationary)
    {
    case LS_ALTAR:
        m = monster_new(MT_DAEMON_PRINCE);
        desc = "altar";
        break;

    case LS_FOUNTAIN:
        m = monster_new(MT_WATER_LORD);
        desc = "fountain";
        break;

    case LS_STATUE:
        if (game_difficulty(nlarn) < 3)
        {
            inv_add(&tile->ilist,
                    item_new(IT_BOOK, rand_1n(item_max_id(IT_BOOK)), 0));
        }

        desc = "statue";
        break;

    case LS_THRONE:
    case LS_THRONE2:
        m = monster_new(MT_GNOME_KING);
        desc = "throne";
        break;

    case LS_DEADFOUNTAIN:
    case LS_DEADTHRONE:
        tile->stationary = LS_NONE;
        break;

    default:
        log_add_entry(p->log, "Somehow that did not work.");
        /* NOP */
    }

    if (desc)
    {
        log_add_entry(p->log, "You destroy the %s.", desc);
        tile->stationary = LS_NONE;
    }

    /* created a monster - position it correctly */
    if (m)
    {
        monster_level_enter(m, p->map);
        monster_set_pos(m, game_map(nlarn, p->pos.z), pos);
    }

    return TRUE;
}


char *book_desc(int book_id)
{
    assert(book_id > SP_NONE && book_id < SP_MAX);
    return (char *)book_descriptions[nlarn->book_desc_mapping[book_id - 1]];
}

int book_weight(item *book)
{
    assert (book != NULL && book->type == IT_BOOK);

    switch (nlarn->book_desc_mapping[book->id - 1])
    {
    case 1: /* thick */
    case 4: /* heavy */
    case 12: /* large */
        return 1200;
        break;

    case 10: /* thin */
    case 11: /* light */
        return 400;
        break;

    default:
        return 800;
    }
}
