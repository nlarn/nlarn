/*
 * spells.c
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
#include <unistd.h>

#include "display.h"
#include "map.h"
#include "nlarn.h"
#include "spells.h"
#include "spheres.h"

const spell_data spells[SP_MAX] =
{
    {
        SP_NONE, NULL, NULL,
        SC_NONE, DAM_NONE, ET_NONE,
        NULL, NULL, NULL,
        0, 0, FALSE
    },
    {
        SP_PRO, "pro","protection",
        SC_PLAYER, DAM_NONE, ET_PROTECTION,
        "Generates a protection field.",
        NULL, NULL,
        1, 260, TRUE
    },
    {
        SP_MLE, "mle", "magic missile",
        SC_RAY, DAM_MAGICAL, ET_NONE,
        "Creates and hurls a magic missile equivalent to a + 1 magic arrow.",
        "Your missile hits the %s.",
        "Your missile bounces off the %s.",
        1, 320, TRUE
    },
    {
        SP_DEX, "dex", "dexterity",
        SC_PLAYER, DAM_NONE, ET_INC_DEX,
        "Improves the caster's dexterity.",
        NULL, NULL,
        1, 260, FALSE
    },
    {
        SP_SLE, "sle", "sleep",
        SC_POINT, DAM_NONE, ET_SLEEP,
        "Causes some monsters to go to sleep.",
        NULL,
        "The %s doesn't sleep.",
        1, 260, TRUE
    },
    {
        SP_CHM, "chm", "charm monster",
        SC_POINT, DAM_NONE, ET_CHARM_MONSTER,
        "Some monsters may be awed at your magnificence.",
        NULL, NULL,
        1, 260, FALSE
    },
    {
        SP_SSP, "ssp", "sonic spear",
        SC_RAY, DAM_PHYSICAL, ET_NONE,
        "Causes your hands to emit a screeching sound toward what they point.",
        "The sound damages the %s.",
        "The %s can't hear the noise.",
        1, 300, TRUE
    },
    {
        SP_STR, "str", "strength",
        SC_PLAYER, DAM_NONE, ET_INC_STR,
        "Increase the caster's strength for a short term.",
        NULL, NULL,
        2, 460, FALSE
    },
    {
        SP_CPO, "cpo", "cure poison",
        SC_OTHER, DAM_NONE, ET_NONE,
        "The caster is cured from poison.",
        NULL, NULL,
        2, 460, TRUE
    },
    {
        SP_HEL, "hel", "healing",
        SC_PLAYER, DAM_NONE, ET_INC_HP,
        "Restores some HP to the caster.",
        NULL, NULL,
        2, 400, TRUE
    },
    {
        SP_CBL, "cbl", "cure blindness",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Restores sight to one so unfortunate as to be blinded.",
        NULL, NULL,
        2, 400, TRUE
    },
    {
        SP_CRE, "cre", "create monster",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Creates a monster near the caster appropriate for the location.",
        NULL, NULL,
        2, 400, FALSE
    },
    {
        SP_PHA, "pha", "phantasmal forces",
        SC_POINT, DAM_NONE, ET_SCARE_MONSTER,
        "Creates illusions, and if believed, monsters flee.",
        "The %s believed!",
        "The %s didn't believe the illusions!",
        2, 600, FALSE
    },
    {
        SP_INV, "inv", "invisibility",
        SC_PLAYER, DAM_NONE, ET_INVISIBILITY,
        "The caster becomes invisible.",
        NULL, NULL,
        2, 600, TRUE
    },
    {
        SP_BAL, "bal", "fireball",
        SC_BLAST, DAM_FIRE, ET_NONE,
        "Makes a ball of fire that burns on what it hits.",
        "The fireball hits the %s.",
        NULL,
        3, 1200, TRUE
    },
    {
        SP_CLD, "cld", "cone of cold",
        SC_RAY, DAM_COLD, ET_NONE,
        "Sends forth a cone of cold which freezes what it touches.",
        "The cone of cold strikes the %s.",
        "The %s loves the cold!",
        3, 1200, TRUE
    },
    {
        SP_PLY, "ply", "polymorph",
        SC_POINT, DAM_NONE, ET_NONE,
        "You can find out what this does for yourself.",
        NULL,
        "The %s resists.",
        3, 950, FALSE
    },
    {
        SP_CAN, "can", "cancellation",
        SC_PLAYER, DAM_NONE, ET_CANCELLATION,
        "Negates the ability of a monster to use his special abilities.",
        NULL, NULL,
        3, 950, FALSE
    },
    {
        SP_HAS, "has", "haste self",
        SC_PLAYER, DAM_NONE, ET_SPEED,
        "Speeds up the caster's movements.",
        NULL, NULL,
        3, 950, FALSE
    },
    {
        SP_CKL, "ckl", "killing cloud",
        SC_FLOOD, DAM_ACID, ET_NONE,
        "Creates a fog of poisonous gas which kills all that is within it.",
        "The %s gasps for air.",
        NULL,
        3, 1200, FALSE
    },
    {
        SP_VPR, "vpr", "vaporize rock",
        SC_OTHER, DAM_NONE, ET_NONE,
        "This changes rock to air.",
        NULL, NULL,
        3, 950, FALSE
    },
    {
        SP_DRY, "dry", "dehydration",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "Dries up water in the immediate vicinity.",
        "The %s shrivels up.",
        "The %s isn't affected.",
        4, 1600, FALSE
    },
    {
        SP_LIT, "lit", "lightning",
        SC_RAY, DAM_ELECTRICITY, ET_NONE,
        "You finger will emit a lightning bolt when this spell is cast.",
        "A lightning bolt hits the %s.",
        "The %s loves fire and lightning!",
        4, 1600, TRUE
    },
    {
        SP_DRL, "drl", "drain life",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "Subtracts hit points from both you and a monster.",
        NULL, NULL,
        4, 1400, FALSE
    },
    {
        SP_GLO, "glo", "invulnerability",
        SC_PLAYER, DAM_NONE, ET_INVULNERABILITY,
        "This globe helps to protect the player from physical attack.",
        NULL, NULL,
        4, 1400, TRUE
    },
    {
        SP_FLO, "flo", "flood",
        SC_FLOOD, DAM_WATER, ET_NONE,
        "This creates an avalanche of H2O to flood the immediate chamber.",
        "The %s struggles for air in your flood!",
        "The %s loves the water!",
        4, 1600, FALSE
    },
    {
        SP_FGR, "fgr", "finger of death",
        SC_POINT, DAM_PHYSICAL, ET_NONE,
        "This is a holy spell and calls upon your god to back you up.",
        "The %s's heart stopped.",
        "The %s isn't affected.",
        4, 1600, FALSE
    },
    {
        SP_SCA, "sca", "scare monster",
        SC_POINT, DAM_NONE, ET_SCARE_MONSTER,
        "Terrifies the monster so that hopefully he wont hit the magic user.",
        NULL, NULL,
        5, 2000, FALSE
    },
    {
        SP_HLD, "hld", "hold monster",
        SC_POINT, DAM_NONE, ET_HOLD_MONSTER,
        "The monster is frozen in his tracks if this is successful.",
        NULL, NULL,
        5, 2000, FALSE
    },
    {
        SP_STP, "stp", "time stop",
        SC_PLAYER, DAM_NONE, ET_TIMESTOP,
        "All movement in the caverns ceases for a limited duration.",
        NULL, NULL,
        5, 2500, FALSE
    },
    {
        SP_TEL, "tel", "teleport away",
        SC_POINT, DAM_NONE, ET_NONE,
        "Moves a particular monster around in the dungeon.",
        NULL, NULL,
        5, 2000, FALSE
    },
    {
        SP_MFI, "mfi", "magic fire",
        SC_FLOOD, DAM_FIRE, ET_NONE,
        "This causes a curtain of fire to appear all around you.",
        "The %s cringes from the flame.",
        NULL,
        5, 2500, FALSE
    },
    {
        SP_MKW, "mkw", "make wall",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Makes a wall in the specified place.",
        NULL, NULL,
        6, 3000, FALSE
    },
    {
        SP_SPH, "sph", "sphere of annihilation",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Anything caught in this sphere is instantly killed.",
        NULL, NULL,
        6, 3500, FALSE
    },
/*
    {
        SP_SUM, "sum", "summon demon",
        SC_OTHER, DAM_NONE, ET_NONE,
        "Summons a demon who hopefully helps you out.",
        NULL, NULL,
        6, 3500, FALSE
    },
*/
    {
        SP_WTW, "wtw", "walk through walls",
        SC_PLAYER, DAM_NONE, ET_WALL_WALK,
        "Allows the caster to walk through walls for a short period of time.",
        NULL, NULL,
        6, 3800, FALSE
    },
    {
        SP_ALT, "alt", "alter reality",
        SC_OTHER, DAM_NONE, ET_NONE,
        "God only knows what this will do.",
        NULL,
        "Polinneaus won't let you mess with his dungeon!",
        6, 3800, FALSE
    },
};

struct book_obfuscation_s
{
    const char* desc;
    const int weight;
    const int colour;
}
book_obfuscation[SP_MAX - 1] =
{
    { "parchment-bound", 800, DC_BROWN,     },
    { "thick",          1200, DC_LIGHTGRAY, },
    { "dusty",           800, DC_LIGHTGRAY, },
    { "leather-bound",   800, DC_BROWN,     },
    { "heavy",          1200, DC_LIGHTGRAY, },
    { "ancient",         800, DC_DARKGRAY,  },
    { "buckram",         800, DC_LIGHTGRAY, },
    { "gilded",          800, DC_YELLOW,    },
    { "embossed",        800, DC_LIGHTGRAY, },
    { "old",             800, DC_LIGHTGRAY, },
    { "thin",            400, DC_LIGHTGRAY, },
    { "light",           400, DC_LIGHTGRAY, },
    { "large",          1200, DC_LIGHTGRAY, },
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
    { "clasped",         800, DC_LIGHTGRAY, },
    { "well-thumbed",    800, DC_LIGHTGRAY, },
    { "ragged",          800, DC_LIGHTGRAY, },
    { "dull",            800, DC_DARKGRAY,  },
/*
    reserve descriptions for unimplemented spells:
    chambray, canvas
*/
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
    int idx;
    cJSON *sser = cJSON_CreateArray();

    for (idx = 0; idx < sparr->len; idx++)
    {
        spell *s = g_ptr_array_index(sparr, idx);
        cJSON_AddItemToArray(sser, spell_serialize(s));
    }

    return sser;
}

GPtrArray *spells_deserialize(cJSON *sser)
{
    int idx;
    GPtrArray *spells = g_ptr_array_new();

    for (idx = 0; idx < cJSON_GetArraySize(sser); idx++)
    {
        spell *s = spell_deserialize(cJSON_GetArrayItem(sser, idx));
        g_ptr_array_add(spells, s);
    }

    return spells;
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

int spell_cast(player *p)
{
    int turns = 0;
    gboolean well_done = FALSE;

    spell *spell;

    if (player_effect(p, ET_CONFUSION))
    {
        log_add_entry(nlarn->log, "You can't aim your magic!");
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
        log_add_entry(nlarn->log, "You lack the power to cast %s.",
                      spell_name(spell));

        return turns;
    }

    log_add_entry(nlarn->log, "You cast %s.", spell_name(spell));

    /* time usage */
    turns = 1;

    /* bad luck */
    if (chance(5) || rand_1n(18) > player_get_int(p))
    {
        log_add_entry(nlarn->log, "It didn't work!");
        player_mp_lose(p, spell_level(spell));

        return turns;
    }

    switch (spell_type(spell))
    {
        /* spells that cause an effect on the player */
    case SC_PLAYER:
        well_done = spell_type_player(spell, p);
        break;

        /* spells that cause an effect on a monster */
    case SC_POINT:
        well_done = spell_type_point(spell, p);
        break;

        /* creates a ray */
    case SC_RAY:
        well_done = spell_type_ray(spell, p);
        break;

        /* effect pours like water */
    case SC_FLOOD:
        well_done = spell_type_flood(spell, p);
        break;

        /* effect occurs like an explosion */
    case SC_BLAST:
        well_done = spell_type_blast(spell, p);
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

            /* summon daemon */
            /* TODO: implement (ticket 55)
        case SP_SUM:
            break;
            */

            /* alter reality */
        case SP_ALT:
            well_done = spell_alter_reality(p);
            if (!well_done)
                log_add_entry(nlarn->log, spell_msg_fail_by_id(spell->id));
            break;
        }
        break;

    case SC_NONE:
    case SC_MAX:
        log_add_entry(nlarn->log, "Internal Error in %s:%d.", __FILE__, __LINE__);
        break;
    }

    if (well_done)
    {
        /* spell has been cast successfully, set mp usage accordingly */
        player_mp_lose(p, spell_level(spell));

        /* increase number of spells cast */
        p->stats.spells_cast++;

        /* increase usage counter for this specific spell */
        spell->used++;
    }

    return turns;
}

int spell_learn(player *p, guint spell_type)
{
    spell *s;
    guint idx;

    assert(p != NULL && spell_type > SP_NONE && spell_type < SP_MAX);

    if (!spell_known(p, spell_type))
    {
        s = spell_new(spell_type);

        /* TODO: add a check for intelligence */
        if (spell_level(s) > (int)p->level)
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

    if (e->type == ET_WALL_WALK)
    {
        e->turns = 6 + rand_1n(10);
    }

    player_effect_add(p, e);

    return TRUE;
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

    pos = display_get_position(p, buffer, FALSE, FALSE, 0, FALSE, TRUE);

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

    monster = map_get_monster_at(game_map(nlarn, p->pos.z), pos);

    if (!monster)
    {
        if (s->id == SP_DRY)
        {
            map_tile *tile = map_tile_at(game_map(nlarn, p->pos.z), pos);
            if (tile->type == LT_DEEPWATER)
            {
                tile->type = LT_DIRT;
                log_add_entry(nlarn->log, "The water evaporates!");
                return TRUE;
            }
        }
        return FALSE;
    }

    switch (s->id)
    {
        /* dehydration */
    case SP_DRY:
        amount = (100 * s->knowledge) + p->level;
        monster_damage_take(monster, damage_new(DAM_MAGICAL, ATT_MAGIC, amount, p));
        break; /* SP_DRY */

        /* drain life */
    case SP_DRL:
        amount = min(p->hp - 1, (int)p->hp_max / 2);

        monster_damage_take(monster, damage_new(DAM_MAGICAL, ATT_MAGIC, amount, p));
        player_damage_take(p, damage_new(DAM_MAGICAL, ATT_MAGIC, amount, NULL),
                           PD_SPELL, SP_DRL);

        break; /* SP_DRL */

        /* finger of death */
    case SP_FGR:
        if ((player_get_wis(p) + s->knowledge) > rand_m_n(10,20))
        {
            monster_damage_take(monster, damage_new(DAM_MAGICAL, ATT_MAGIC, 2000, p));
        }
        else
        {
            log_add_entry(nlarn->log, "It didn't work.");
        }
        break; /* SP_FGR */

        /* polymorph */
    case SP_PLY:
        monster_polymorph(monster);
        break;

        /* teleport */
    case SP_TEL:
        if (monster_in_sight(monster))
        {
            log_add_entry(nlarn->log, "The %s disappears.",
                          monster_name(monster));
        }

        map *mmap = game_map(nlarn, monster_pos(monster).z);
        monster_pos_set(monster, mmap, map_find_space(mmap, LE_MONSTER, FALSE));
        break; /* SP_TEL */

    default:
        /* spell has an effect, add that to the monster */
        assert(spell_effect(s) != ET_NONE);

        if (spell_msg_succ(s))
        {
            log_add_entry(nlarn->log, spell_msg_succ(s),
                          monster_get_name(monster));
        }

        e = effect_new(spell_effect(s));

        if (!e->amount)
        {
            e->amount = p->intelligence;
        }

        e->amount *= s->knowledge;
        e = monster_effect_add(monster, e);

        break;
    }

    return TRUE;
}

// #define DEBUG_BEAMS
static position throw_ray(spell *sp, struct player *p,
                          position start, position target, int damage)
{
#ifdef DEBUG_BEAMS
    log_add_entry(nlarn->log, "Beam from (%d, %d) -> (%d, %d)",
                  start.x, start.y, target.x, target.y);
#endif
    assert(sp != NULL && p != NULL && (spell_type(sp) == SC_RAY));
    assert(start.z == target.z);

    map *cmap;
    cmap = game_map(nlarn, start.z);
    int distance = pos_distance(start, target);

    area *ray = NULL;
    ray = area_new_ray(start, target, map_get_obstacles(cmap, start, distance));

    int attrs; /* curses attributes */

    monster *monster = NULL;
    gboolean proceed_y = TRUE;

    position pos = start;
    do
    {
        gboolean proceed_x = TRUE;

        do
        {
#ifdef DEBUG_BEAMS
            log_add_entry(nlarn->log, "current pos: (%d, %d)",
                          pos.x, pos.y);
#endif
            /* check if the current position has been hit by the ray
               and if a monster is standing at the current position */
            if (area_pos_get(ray, pos))
            {
                if ((monster = map_get_monster_at(cmap, pos)))
                {
                    gboolean mis = monster_in_sight(monster);

                    attron((attrs = (mis ? DC_LIGHTRED : DC_LIGHTCYAN)));
                    mvaddch(pos.y, pos.x, (mis ? monster_glyph(monster) : '*'));

                    log_add_entry(nlarn->log, spell_msg_succ(sp), monster_get_name(monster));
                    monster_damage_take(monster, damage_new(spell_damage(sp), ATT_MAGIC, damage, p));
                }
                else
                {
                    attron((attrs = DC_LIGHTCYAN));
                    mvaddch(pos.y, pos.x, '*');
                }

                attroff(attrs);
                refresh();
                /* sleep a while to show the ray's position */
                usleep(100000);
                /* repaint the screen */
                display_paint_screen(p);
            }

            if (!pos_identical(pos, start)
                    && map_sobject_at(cmap, pos) == LS_MIRROR)
            {
#ifdef DEBUG_BEAMS
                log_add_entry(nlarn->log, "Reaching mirror. Breaking loop.");
#endif
                proceed_x = FALSE;
                proceed_y = FALSE;
                break;
            }

            /* modify horizontal position if needed;
               exit the loop when the destination has been reached */
            if (pos.x < target.x)
                pos.x++;
            else if (pos.x > target.x)
                pos.x--;
            else if (pos.x == target.x)
                proceed_x = FALSE;

            /* terminate upon reaching the target */
            if (pos_identical(pos, target))
                proceed_y = FALSE;
        }
        while (proceed_x);

        if (!proceed_y)
            break;

        /* modify vertical position if needed */
        if (pos.y < target.y)
        {
            pos.y++;
            /* reset horizontal position upon vertical movement */
            pos.x = p->pos.x;
        }
        else if (pos.y > target.y)
        {
            pos.y--;
            /* reset horizontal position upon vertical movement */
            pos.x = p->pos.x;
        }
    }
    while (proceed_y);

    area_destroy(ray);

    return pos;
}

int spell_type_ray(spell *s, struct player *p)
{
    map *cmap;
    position target, pos;
    char buffer[61];
    int amount = 0;

    assert(s != NULL && p != NULL && (spell_type(s) == SC_RAY));

    g_snprintf(buffer, 60, "Select a target for the %s.", spell_name(s));
    target = display_get_position(p, buffer, TRUE, FALSE, 0, TRUE, TRUE);
    cmap = game_map(nlarn, p->pos.z);

    /* player pressed ESC */
    if (!pos_valid(target))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    if (pos_identical(pos, p->pos))
    {
        log_add_entry(nlarn->log, "This spell only works on monsters.");
        return FALSE;
    }

    /* determine amount of damage */
    switch (s->id)
    {
    case SP_MLE:
        amount = rand_1n(((p->level + 1) << s->knowledge)) + p->level + 3;
        break;

    case SP_SSP:
        amount = rand_1n(10) + (15 * s->knowledge) + p->level;
        break;

    case SP_CLD:
        amount = rand_1n(25) + (20 * s->knowledge) + p->level;
        break;

    case SP_LIT:
        amount = rand_1n(25) + (20 * s->knowledge) + (p->level << 1);
        break;
    }

    /* use pos as cursor and move it to the target. check if there is
       anything in the way that gets hit by the ray as well */
    pos = p->pos;

    position last_pos = throw_ray(s, p, pos, target, amount);

    if (map_sobject_at(cmap, last_pos) == LS_MIRROR)
    {
        log_add_entry(nlarn->log, "The mirror reflects your spell!");

        throw_ray(s, p, last_pos, p->pos, amount);
        log_add_entry(nlarn->log, "The %s hits you!", spell_name(s));
        player_damage_take(p, damage_new(spell_damage(s), ATT_MAGIC, amount, NULL),
                           PD_SPELL, s->id);
    }

    return TRUE;
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
    pos = display_get_position(p, buffer, FALSE, FALSE, 0, TRUE, TRUE);

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
    }

    area *obstacles = map_get_obstacles(game_map(nlarn, p->pos.z), pos, radius);
    range = area_new_circle_flooded(pos, radius, obstacles);

    map_set_tiletype(game_map(nlarn, p->pos.z), range, type, amount);
    area_destroy(range);

    return TRUE;
}

int spell_type_blast(spell *s, struct player *p)
{
    monster *monster = NULL;
    area *ball;
    position pos, cursor;
    char buffer[61];
    int radius = 0, amount = 0, colour = DC_NONE;
    damage_t dam_t = DAM_NONE;
    item_erosion_type iet = IET_NONE;
    damage *dam;
    inventory **inv;
    map *cmap = game_map(nlarn, p->pos.z);

    assert(s != NULL && p != NULL && (spell_type(s) == SC_BLAST));

    switch (s->id)
    {
        /* currently there is only the fireball */
    case SP_BAL:
    default:
        radius = 2;
        dam_t = DAM_FIRE;
        iet = IET_BURN;
        colour = DC_LIGHTRED;
        amount = (25 * s->knowledge) + p->level + rand_0n(25 + p->level);
        break;
    }

    g_snprintf(buffer, 60, "Point to the center of the %s.", spell_name(s));
    pos = display_get_position(p, buffer, FALSE, TRUE, radius, TRUE, TRUE);

    /* player pressed ESC */
    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "Aborted.");
        return FALSE;
    }

    ball = area_new_circle_flooded(pos, radius, map_get_obstacles(cmap, pos, radius));

    if (area_pos_get(ball, p->pos)
            && !display_get_yesno("The spell is going to hit you. " \
                                  "Cast anyway?", NULL, NULL))
    {
        log_add_entry(nlarn->log, "Aborted.");
        area_destroy(ball);
        return FALSE;
    }

    attron(colour);
    cursor.z = pos.z;

    for (cursor.y = ball->start_y; cursor.y < ball->start_y + ball->size_y; cursor.y++)
    {
        for (cursor.x = ball->start_x; cursor.x < ball->start_x + ball->size_x; cursor.x++)
        {
            if (area_pos_get(ball, cursor))
            {
                /* move cursor to position */
                move(cursor.y, cursor.x);

                if ((monster = map_get_monster_at(cmap, cursor))
                        && monster_in_sight(monster))
                {
                    /* blast hit a visible monster */
                    addch(monster_glyph(monster));

                    dam = damage_new(dam_t, ATT_MAGIC, amount, p);
                    monster_damage_take(monster, dam);
                }
                else if (pos_identical(p->pos, cursor))
                {
                    /* blast hit the player */
                    addch('@');

                    log_add_entry(nlarn->log, "The %s hits you.", spell_name(s));

                    /* damage items in player's inventory */
                    if (iet > IET_NONE) inv_erode(&p->inventory, iet, TRUE);

                    /* damage the player */
                    dam = damage_new(dam_t, ATT_MAGIC, amount, NULL);
                    player_damage_take(p, dam, PD_SPELL, s->id);
                }
                else
                {
                    /* blast hit nothing or an invisible monster */
                    addch('*');
                }

                /* affect items on the position */
                if ((inv = map_ilist_at(cmap, cursor)) && iet > IET_NONE)
                {
                    inv_erode(inv, iet, player_pos_visible(p, cursor));
                }
            }
        }
    }

    area_destroy(ball);
    attroff(colour);

    /* make sure the blast shows up */
    refresh();
    /* sleep a 3/4 second */
    usleep(750000);

    return TRUE;
}

gboolean spell_alter_reality(player *p)
{
    if (p->pos.z == 0)
        return FALSE;

    map *nlevel;

    position pos;
    pos.z = p->pos.z;

    // Overwrite memorised floor tiles with space, to distinguish between
    // the remembered map of the old level, and newly discovered portions
    // of the new one.
    for (pos.y = 0; pos.y < MAP_MAX_Y; pos.y++)
    {
        for (pos.x = 0; pos.x < MAP_MAX_X; pos.x++)
        {
            map_tile_t tile = player_memory_of(p, pos).type;
            if (tile == LT_FLOOR)
                player_memory_of(p, pos).type = LT_NONE;
        }
    }

    map_destroy(game_map(nlarn, p->pos.z));

    /* create new map */
    nlevel = nlarn->maps[p->pos.z] = map_new(p->pos.z, game_mazefile(nlarn));

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
    if (p->pos.z == 0)
    {
        log_add_entry(nlarn->log, "Nothing happens.");
        return FALSE;
    }

    /* try to find a space for the monster near the player */
    mpos = map_find_space_in(game_map(nlarn, p->pos.z),
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
    position pos;
    sphere *sphere;

    assert(p != NULL);

    pos = display_get_position(p, "Where do you want to place the sphere?",
                               FALSE, FALSE, 0, TRUE, TRUE);

    if (pos_valid(pos))
    {
        sphere = sphere_new(pos, p, p->level * 10 * s->knowledge);
        g_ptr_array_add(nlarn->spheres, sphere);

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

    assert(p != NULL);

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

    assert(p != NULL);

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

gboolean spell_make_wall(player *p)
{
    position pos;

    pos = display_get_position(p, "Select a position where you want to place a wall.",
                               FALSE, FALSE, 0, TRUE, TRUE);

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

    if (map_tiletype_at(game_map(nlarn, p->pos.z), pos) != LT_WALL)
    {
        map_tile *tile = map_tile_at(game_map(nlarn, p->pos.z), pos);

        tile->type = tile->base_type = LT_WALL;

        /* destroy all items at that position */
        if (tile->ilist != NULL)
        {
            inv_destroy(tile->ilist, TRUE);
            tile->ilist = NULL;
        }

        log_add_entry(nlarn->log, "You have created a wall.");

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
    position mpos;      /* position for monster that might be generated */
    char *desc = NULL;
    map *map = game_map(nlarn, p->pos.z);

    pos = display_get_position(p, "What do you want to vaporize?",
                               FALSE, FALSE, 0, FALSE, TRUE);

    if (!pos_valid(pos))
    {
        log_add_entry(nlarn->log, "So you chose not to vaporize anything.");
        return FALSE;
    }

    if (map_tiletype_at(map, pos) == LT_WALL)
    {
        map_tiletype_set(map, pos, LT_FLOOR);
    }

    if ((m = map_get_monster_at(map, pos)) && (monster_type(m) == MT_XORN))
    {
        /* xorns take damage from vpr */
        monster_damage_take(m, damage_new(DAM_PHYSICAL, ATT_NONE, divert(200, 10), p));
    }

    mpos = map_find_space_in(map, rect_new_sized(p->pos, 1), LE_MONSTER, FALSE);

    switch (map_sobject_at(map, pos))
    {
    case LS_NONE:
        /* NOP */
        break;

    case LS_ALTAR:
        if (pos_valid(mpos))
        {
            monster_new(MT_DEMON_PRINCE, mpos);
        }
        desc = "altar";
        break;

    case LS_FOUNTAIN:
        if (pos_valid(mpos))
        {
            monster_new(MT_WATER_LORD, mpos);
        }
        desc = "fountain";
        break;

    case LS_STATUE:
        if (game_difficulty(nlarn) < 3)
        {
            item *it = item_new(IT_BOOK, rand_1n(item_max_id(IT_BOOK)));
            inv_add(map_ilist_at(map, pos), it);
        }

        desc = "statue";
        break;

    case LS_THRONE:
    case LS_THRONE2:
        if (pos_valid(mpos))
        {
            monster_new(MT_GNOME_KING, mpos);
        }
        desc = "throne";
        break;

    case LS_DEADFOUNTAIN:
    case LS_DEADTHRONE:
        map_sobject_set(map, pos, LS_NONE);
        break;

    default:
        log_add_entry(nlarn->log, "Somehow that did not work.");
        /* NOP */
    }

    if (desc)
    {
        log_add_entry(nlarn->log, "You destroy the %s.", desc);
        map_sobject_set(map, pos, LS_NONE);
    }

    return TRUE;
}


char *book_desc(int book_id)
{
    assert(book_id > SP_NONE && book_id < SP_MAX);
    return (char *)book_obfuscation[nlarn->book_desc_mapping[book_id - 1]].desc;
}

int book_weight(item *book)
{
    assert (book != NULL && book->type == IT_BOOK);
    return book_obfuscation[nlarn->book_desc_mapping[book->id - 1]].weight;
}

int book_colour(item *book)
{
    assert (book != NULL && book->type == IT_BOOK);
    return book_obfuscation[nlarn->book_desc_mapping[book->id - 1]].colour;
}

item_usage_result book_read(struct player *p, item *book)
{
    item_usage_result result = { FALSE, FALSE };
    char description[61];

    item_describe(book, player_item_known(p, book),
                  TRUE, TRUE, description, 60);

    if (player_effect(p, ET_BLINDNESS))
    {
        log_add_entry(nlarn->log, "As you are blind you can't read %s.",
                      description);

        return result;
    }

    log_add_entry(nlarn->log, "You read %s.", description);

    /* try to complete reading the book */
    if (!player_make_move(p, 1 + spell_level_by_id(book->id),
                          TRUE, "reading %s", description))
    {
        /* the action has been aborted */
        return result;
    }

    /* the book has successfully been read - increase number of books read */
    p->stats.books_read++;

    /* cursed spellbooks have nasty effects */
    if (book->cursed)
    {
        log_add_entry(nlarn->log, "There was something wrong with this book! " \
                      "It crumbles to dust.");

        player_mp_lose(p, rand_0n(p->mp));
    }
    else
    {
        switch (spell_learn(p, book->id))
        {
        case 0:
            log_add_entry(nlarn->log, "You cannot understand the content of this book.");
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
        if (chance(2))
        {
            log_add_entry(nlarn->log, "Reading makes you ingenious.");
            p->intelligence++;
        }
    }

    return result;
}
