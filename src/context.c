/*
 * context.c
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
#include <glib/gi18n.h>

#include "context.h"
#include "display.h"
#include "extdefs.h"
#include "game.h"
#include "items.h"
#include "map.h"
#include "monsters.h"
#include "pathfinding.h"
#include "sobjects.h"
#include "spells.h"
#include "weapons.h"

/* A context action: a label carrying `KEY` hotkey markup, a predicate
   deciding whether the action applies at the clicked position, and an
   executor performing it. The executor returns the number of turns spent
   and may set *travel_to to request auto-travel. */
typedef struct {
    const char *label;
    bool (*available)(player *p, position pos);
    int  (*execute)(player *p, position pos, position *travel_to);
} context_action;

/* *** shared predicates *** */

static bool ctx_here(player *p, position pos)
{
    return pos_identical(pos, p->pos);
}

static sobject_t ctx_sobject(position pos)
{
    return map_sobject_at(game_map(nlarn, Z(pos)), pos);
}

static monster *ctx_monster(position pos)
{
    return map_get_monster_at(game_map(nlarn, Z(pos)), pos);
}

/* a monster the player can see at the position, i.e. a combat target */
static bool ctx_target(player *p __attribute__((unused)), position pos)
{
    monster *m = ctx_monster(pos);
    return (m != NULL && monster_in_sight(m));
}

static bool ctx_is_building(sobject_t so)
{
    switch (so)
    {
    case LS_HOME:      case LS_DNDSTORE: case LS_TRADEPOST: case LS_LRS:
    case LS_SCHOOL:    case LS_BANK:     case LS_BANK2:     case LS_MONASTERY:
        return true;
    default:
        return false;
    }
}

static bool ctx_knows_positional_spell(player *p)
{
    if (p->known_spells == NULL)
        return false;

    for (guint i = 0; i < p->known_spells->len; i++)
    {
        spell *s = g_ptr_array_index(p->known_spells, i);
        switch (spell_type(s))
        {
        case SC_POINT: case SC_RAY: case SC_FLOOD: case SC_BLAST:
            return true;
        default:
            break;
        }
    }

    return false;
}

/* *** availability predicates *** */

static bool avail_attack(player *p, position pos)
{
    return ctx_target(p, pos) && pos_adjacent(p->pos, pos);
}

static bool avail_approach(player *p, position pos)
{
    return ctx_target(p, pos) && !ctx_here(p, pos) && !pos_adjacent(p->pos, pos);
}

static bool avail_fire(player *p, position pos)
{
    return ctx_target(p, pos) && p->eq_weapon != NULL
           && weapon_is_ranged(p->eq_weapon);
}

static bool avail_throw(player *p, position pos)
{
    return !ctx_here(p, pos)
           && inv_length_filtered(p->inventory, item_filter_throwable) > 0
           /* the tile must be reachable on a clear straight path */
           && map_pos_is_visible(game_map(nlarn, Z(p->pos)), p->pos, pos);
}

static bool avail_cast(player *p, position pos)
{
    if (ctx_here(p, pos))
        return (p->known_spells != NULL && p->known_spells->len > 0);

    return ctx_knows_positional_spell(p);
}

static bool avail_travel(player *p, position pos)
{
    return !ctx_here(p, pos) && !ctx_target(p, pos);
}

static bool avail_down(player *p, position pos)
{
    if (!ctx_here(p, pos)) return false;
    switch (ctx_sobject(pos))
    {
    case LS_STAIRSDOWN: case LS_ELEVATORDOWN: case LS_CAVERNS_ENTRY:
        return true;
    default:
        return false;
    }
}

static bool avail_up(player *p, position pos)
{
    if (!ctx_here(p, pos)) return false;
    switch (ctx_sobject(pos))
    {
    case LS_STAIRSUP: case LS_ELEVATORUP: case LS_CAVERNS_EXIT:
        return true;
    default:
        return false;
    }
}

static bool avail_enter(player *p, position pos)
{
    return ctx_here(p, pos) && ctx_is_building(ctx_sobject(pos));
}

static bool avail_fountain(player *p, position pos)
{
    return ctx_here(p, pos) && ctx_sobject(pos) == LS_FOUNTAIN;
}

static bool avail_altar(player *p, position pos)
{
    return ctx_here(p, pos) && ctx_sobject(pos) == LS_ALTAR;
}

static bool avail_throne(player *p, position pos)
{
    if (!ctx_here(p, pos)) return false;
    sobject_t so = ctx_sobject(pos);
    return (so == LS_THRONE || so == LS_THRONE2);
}

/* *** executors *** */

static int exec_attack(player *p, position pos, position *tt __attribute__((unused)))
{
    return player_move(p, pos_dir(p->pos, pos), true);
}

static int exec_approach(player *p, position pos, position *tt __attribute__((unused)))
{
    /* step one tile towards the target, routing around walls */
    map *cmap = game_map(nlarn, Z(p->pos));
    path *pth = path_find(cmap, p->pos, pos, LE_GROUND);
    int turns = 0;

    if (pth != NULL && !g_queue_is_empty(pth->path))
    {
        path_element *el = g_queue_pop_head(pth->path);
        turns = player_move(p, pos_dir(p->pos, el->pos), true);
    }

    if (pth != NULL)
        path_destroy(pth);

    return turns;
}

static int exec_fire(player *p, position pos, position *tt __attribute__((unused)))
{
    return weapon_fire(p, pos);
}

static int exec_throw(player *p, position pos, position *tt __attribute__((unused)))
{
    /* the thrown item flies at the clicked tile without a second prompt */
    display_set_pending_target(pos);
    player_throw(p);
    display_set_pending_target(pos_invalid);
    return 0;
}

static int exec_cast(player *p, position pos, position *tt __attribute__((unused)))
{
    /* a spell cast on the player's own tile affects the player; a spell
       cast on another tile is fired at it without a second target prompt */
    if (ctx_here(p, pos))
        return spell_cast_new(p, SC_PLAYER);

    display_set_pending_target(pos);
    int turns = spell_cast_new(p, SC_MAX);
    display_set_pending_target(pos_invalid);
    return turns;
}

static int exec_travel(player *p __attribute__((unused)), position pos, position *tt)
{
    if (tt != NULL)
        *tt = pos;
    return 0;
}

static int exec_down(player *p, position pos __attribute__((unused)),
                     position *tt __attribute__((unused)))
{
    return player_stairs_down(p);
}

static int exec_up(player *p, position pos __attribute__((unused)),
                   position *tt __attribute__((unused)))
{
    return player_stairs_up(p);
}

static int exec_enter(player *p, position pos __attribute__((unused)),
                      position *tt __attribute__((unused)))
{
    return player_building_enter(p);
}

static int exec_drink(player *p, position pos __attribute__((unused)),
                      position *tt __attribute__((unused)))
{
    return player_fountain_drink(p);
}

static int exec_wash(player *p, position pos __attribute__((unused)),
                     position *tt __attribute__((unused)))
{
    return player_fountain_wash(p);
}

static int exec_pray(player *p, position pos __attribute__((unused)),
                     position *tt __attribute__((unused)))
{
    return player_altar_pray(p);
}

static int exec_desecrate(player *p, position pos __attribute__((unused)),
                          position *tt __attribute__((unused)))
{
    return player_altar_desecrate(p);
}

static int exec_sit(player *p, position pos __attribute__((unused)),
                    position *tt __attribute__((unused)))
{
    return player_throne_sit(p);
}

static int exec_pillage(player *p, position pos __attribute__((unused)),
                        position *tt __attribute__((unused)))
{
    return player_throne_pillage(p);
}

static int exec_rest(player *p __attribute__((unused)),
                     position pos __attribute__((unused)),
                     position *tt __attribute__((unused)))
{
    /* wait one turn */
    return 1;
}

/* The action registry. The order is the order shown in the menu. Hotkeys
   are chosen so that no two actions that can appear together collide. */
static const context_action context_actions[] = {
    { N_("`KEY`g`end`)o here"),          avail_travel,   exec_travel },
    { N_("`KEY`a`end`)ttack"),           avail_attack,   exec_attack },
    { N_("`KEY`a`end`)pproach"),         avail_approach, exec_approach },
    { N_("`KEY`f`end`)ire weapon"),      avail_fire,     exec_fire },
    { N_("`KEY`t`end`)hrow item"),       avail_throw,    exec_throw },
    { N_("`KEY`c`end`)ast a spell"),     avail_cast,     exec_cast },
    { N_("go `KEY`d`end`)own"),          avail_down,     exec_down },
    { N_("go `KEY`u`end`)p"),            avail_up,       exec_up },
    { N_("`KEY`e`end`)nter"),            avail_enter,    exec_enter },
    { N_("`KEY`d`end`)rink from fountain"), avail_fountain, exec_drink },
    { N_("`KEY`w`end`)ash at fountain"), avail_fountain, exec_wash },
    { N_("`KEY`p`end`)ray at altar"),    avail_altar,    exec_pray },
    { N_("desecrate the altar (`KEY`x`end`)"), avail_altar, exec_desecrate },
    { N_("`KEY`s`end`)it on throne"),    avail_throne,   exec_sit },
    { N_("`KEY`p`end`)illage throne"),   avail_throne,   exec_pillage },
    { N_("`KEY`r`end`)est here"),        ctx_here,       exec_rest },
};

int context_menu(player *p, position pos, position *travel_to)
{
    g_assert(p != NULL);

    if (travel_to != NULL)
        *travel_to = pos_invalid;

    if (!pos_valid(pos))
        return 0;

    /* collect the applicable actions */
    const guint total = G_N_ELEMENTS(context_actions);
    const char *labels[G_N_ELEMENTS(context_actions)];
    const context_action *chosen[G_N_ELEMENTS(context_actions)];
    guint n = 0;

    for (guint i = 0; i < total; i++)
    {
        if (context_actions[i].available(p, pos))
        {
            labels[n] = _(context_actions[i].label);
            chosen[n] = &context_actions[i];
            n++;
        }
    }

    if (n == 0)
    {
        log_add_entry(nlarn->log, _("There is nothing to do here."));
        return 0;
    }

    /* the examined tile description doubles as the menu's header */
    char *message = map_pos_examine(pos);

    /* anchor the menu at the clicked tile (map cells map 1:1 to screen) */
    int sel = display_menu_at(X(pos), Y(pos), NULL, message, labels,
                              NULL, NULL, n, 0);

    g_free(message);

    if (sel < 0)
        return 0;

    return chosen[sel]->execute(p, pos, travel_to);
}
