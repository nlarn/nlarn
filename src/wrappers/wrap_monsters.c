/*
 * wrap_monsters.c
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
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>

#include "display.h"
#include "monsters.h"
#include "nlarn.h"

void wrap_monsters(lua_State *L)
{
    g_assert (L != NULL);

    struct
    {
        const char *name;
        int value;
    } constants[] =
    {
        /* monster actions */
        { "FLEE",   MA_FLEE },
        { "REMAIN", MA_REMAIN },
        { "WANDER", MA_WANDER },
        { "ATTACK", MA_ATTACK },
        { "SERVE",  MA_SERVE },

        /* monster flags */
        { "HEAD",        MF_HEAD },
        { "NOBEHEAD",    MF_NOBEHEAD },
        { "HANDS",       MF_HANDS },
        { "FLY",         MF_FLY },
        { "SPIRIT",      MF_SPIRIT },
        { "UNDEAD",      MF_UNDEAD },
        { "INVISIBLE",   MF_INVISIBLE },
        { "INFRAVISION", MF_INFRAVISION },
        { "REGENERATE",  MF_REGENERATE },
        { "METALLIVORE", MF_METALLIVORE },
        { "DEMON",       MF_DEMON },
        { "DRAGON",      MF_DRAGON },
        { "MIMIC",       MF_MIMIC },
        { "RES_FIRE",    MF_RES_FIRE },
        { "RES_COLD",    MF_RES_COLD },
        { "RES_ELEC",    MF_RES_ELEC },
        { "RES_POISON",  MF_RES_POISON },
        { "RES_SLEEP",   MF_RES_SLEEP },
        { "RES_CONF",    MF_RES_CONF },
        { "RES_MAGIC",   MF_RES_MAGIC },
        { "SWIM",        MF_SWIM },

        /* monster types */
        { "MT_GIANT_BAT",       MT_GIANT_BAT },
        { "MT_GNOME",           MT_GNOME },
        { "MT_HOBGOBLIN",       MT_HOBGOBLIN },
        { "MT_JACKAL",          MT_JACKAL },
        { "MT_KOBOLD",          MT_KOBOLD },
        { "MT_ORC",             MT_ORC },
        { "MT_SNAKE",           MT_SNAKE },
        { "MT_CENTIPEDE",       MT_CENTIPEDE },
        { "MT_JACULUS",         MT_JACULUS },
        { "MT_TROGLODYTE",      MT_TROGLODYTE },
        { "MT_GIANT_ANT",       MT_GIANT_ANT },
        { "MT_FLOATING_EYE",    MT_FLOATING_EYE },
        { "MT_LEPRECHAUN",      MT_LEPRECHAUN },
        { "MT_NYMPH",           MT_NYMPH },
        { "MT_QUASIT",          MT_QUASIT },
        { "MT_RUST_MONSTER",    MT_RUST_MONSTER },
        { "MT_ZOMBIE",          MT_ZOMBIE },
        { "MT_ASSASSIN_BUG",    MT_ASSASSIN_BUG },
        { "MT_BUGBEAR",         MT_BUGBEAR },
        { "MT_HELLHOUND",       MT_HELLHOUND },
        { "MT_ICE_LIZARD",      MT_ICE_LIZARD },
        { "MT_CENTAUR",         MT_CENTAUR },
        { "MT_TROLL",           MT_TROLL },
        { "MT_YETI",            MT_YETI },
        { "MT_ELF",             MT_ELF },
        { "MT_GELATINOUSCUBE",  MT_GELATINOUSCUBE },
        { "MT_WHITE_DRAGON",    MT_WHITE_DRAGON },
        { "MT_METAMORPH",       MT_METAMORPH },
        { "MT_VORTEX",          MT_VORTEX },
        { "MT_ZILLER",          MT_ZILLER },
        { "MT_VIOLET_FUNGUS",   MT_VIOLET_FUNGUS },
        { "MT_WRAITH",          MT_WRAITH },
        { "MT_FORVALAKA",       MT_FORVALAKA },
        { "MT_LAMA_NOBE",       MT_LAMA_NOBE },
        { "MT_OSQUIP",          MT_OSQUIP },
        { "MT_ROTHE",           MT_ROTHE },
        { "MT_XORN",            MT_XORN },
        { "MT_VAMPIRE",         MT_VAMPIRE },
        { "MT_STALKER",         MT_STALKER },
        { "MT_POLTERGEIST",     MT_POLTERGEIST },
        { "MT_DISENCHANTRESS",  MT_DISENCHANTRESS },
        { "MT_SHAMBLINGMOUND",  MT_SHAMBLINGMOUND },
        { "MT_YELLOW_MOLD",     MT_YELLOW_MOLD },
        { "MT_UMBER_HULK",      MT_UMBER_HULK },
        { "MT_GNOME_KING",      MT_GNOME_KING },
        { "MT_MIMIC",           MT_MIMIC },
        { "MT_WATER_LORD",      MT_WATER_LORD },
        { "MT_PURPLE_WORM",     MT_PURPLE_WORM },
        { "MT_XVART",           MT_XVART },
        { "MT_BRONZE_DRAGON",   MT_BRONZE_DRAGON },
        { "MT_GREEN_DRAGON",    MT_GREEN_DRAGON },
        { "MT_SILVER_DRAGON",   MT_SILVER_DRAGON },
        { "MT_PLATINUM_DRAGON", MT_PLATINUM_DRAGON },
        { "MT_RED_DRAGON",      MT_RED_DRAGON },
        { "MT_SPIRIT_NAGA",     MT_SPIRIT_NAGA },
        { "MT_GREEN_URCHIN",    MT_GREEN_URCHIN },
        { "MT_DEMONLORD_I",     MT_DEMONLORD_I },
        { "MT_DEMONLORD_II",    MT_DEMONLORD_II },
        { "MT_DEMONLORD_III",   MT_DEMONLORD_III },
        { "MT_DEMONLORD_IV",    MT_DEMONLORD_IV },
        { "MT_DEMONLORD_V",     MT_DEMONLORD_V },
        { "MT_DEMONLORD_VI",    MT_DEMONLORD_VI },
        { "MT_DEMONLORD_VII",   MT_DEMONLORD_VII },
        { "MT_DEMON_PRINCE",    MT_DEMON_PRINCE },
        { "MT_TOWN_PERSON",     MT_TOWN_PERSON },

        { NULL, 0 },
    };

    for (int i = 0; constants[i].name != NULL; i++)
    {
        lua_pushinteger(L, constants[i].value);
        lua_setglobal(L, constants[i].name);
    }

    /* read monster data */
    gchar *filename = g_strdup_printf("%s%c%s", nlarn->libdir,
                                      G_DIR_SEPARATOR, "monsters.lua");

    if (luaL_dofile(L, filename) == 1)
    {
        display_shutdown();
        g_printerr("Failed to load monster data: %s\n",
                   lua_tostring(L, -1));

        exit(EXIT_FAILURE);
    }
    g_free(filename);

}
