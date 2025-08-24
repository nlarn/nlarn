/*
 * colours.h
 * Copyright (C) 2009-2025 Joachim de Groot <jdegroot@web.de>
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

#ifndef __COLOURS_H_
#define __COLOURS_H_

#include <curses.h>

#include "enumFactory.h"

#define COLOUR_ENUM(COLOUR) \
    COLOUR(COLOURLESS, = -1) \
    COLOUR(BLACK, = 0) \
    COLOUR(DARK_BURGUNDY,) \
    COLOUR(TREE_GREEN,) \
    COLOUR(SWAMP_GREEN,) \
    COLOUR(NAVY_BLUE,) \
    COLOUR(DARK_MAGENTA,) \
    COLOUR(DEEP_SEA,) \
    COLOUR(SILVER_SAND,) \
    COLOUR(GREY50_2,) /* duplicate of 244 */ \
    COLOUR(RED,) \
    COLOUR(GREEN,) \
    COLOUR(YELLOW,) \
    COLOUR(BLUE,) \
    COLOUR(MAGENTA,) \
    COLOUR(CYAN,) \
    COLOUR(WHITE,) \
    COLOUR(BLACK_2,) /* 016 - duplicate */ \
    COLOUR(DARK_BLUE,) \
    COLOUR(DARK_ROYAL_BLUE,) \
    COLOUR(COBALT_BLUE,) \
    COLOUR(MEDIUM_BLUE,) \
    COLOUR(BLUE_2,) /* duplicate */ \
    COLOUR(DARK_FERN,) /* 022 */ \
    COLOUR(DEEP_SEA_GREEN,) \
    COLOUR(DEEP_SEA_BLUE,) \
    COLOUR(LED_BLUE,) \
    COLOUR(AZUL,) \
    COLOUR(BRIGHT_BLUE,) \
    COLOUR(IRELAND_GREEN,) /* 028 */ \
    COLOUR(ABSINTHE_TURQUOISE,) \
    COLOUR(CARIBBEAN_TURQUOISE,) \
    COLOUR(TUSCHE_BLUE,) \
    COLOUR(WATER_BLUE,) \
    COLOUR(DEEP_SKY_BLUE,) \
    COLOUR(GRASS_GREEN,) /* 034 */ \
    COLOUR(GREEN_HAZE,) \
    COLOUR(MOUNTAIN_MEADOW,) \
    COLOUR(TOPAZ,) \
    COLOUR(CERULEAN,) \
    COLOUR(SKY_BLUE,) \
    COLOUR(VIVID_GREEN,) /* 040 */ \
    COLOUR(MALACHITE,) \
    COLOUR(TEALISH_GREEN,) \
    COLOUR(GARISH_BLUE,) \
    COLOUR(AQUA_BLUE,) \
    COLOUR(NEON_BLUE,) \
    COLOUR(GREEN_2,) /* 046 - duplicate */ \
    COLOUR(COOL_GREEN,) \
    COLOUR(OCEAN_GREEN,) \
    COLOUR(BRIGHT_SEA_GREEN,) \
    COLOUR(PALE_CYAN,) \
    COLOUR(CYAN_2,) /* duplicate */ \
    COLOUR(DRIED_BLOOD,) /* 052 */ \
    COLOUR(FUCHSIA,) \
    COLOUR(INDIGO,) \
    COLOUR(DEEP_PURPLE,) \
    COLOUR(BLUE_VIOLET,) \
    COLOUR(ELECTRIC_INDIGO,) \
    COLOUR(GREEN_BROWN,) /* 058 */ \
    COLOUR(CARBON_GREY,) \
    COLOUR(WOOL_VIOLET,) \
    COLOUR(CHALCEDONY_VIOLET,) \
    COLOUR(GLADIOLA_BLUE,) \
    COLOUR(BLUE_LOTUS,) \
    COLOUR(VENOM_GREEN,) /* 064 */ \
    COLOUR(GLADE_GREEN,) \
    COLOUR(SPRUCE_BLUE,) \
    COLOUR(DUSTY_BLUE,) \
    COLOUR(BUTTERFLY_BLUE,) \
    COLOUR(CORNFLOWER_BLUE,) \
    COLOUR(LEAF_GREEN,) /* 070 */ \
    COLOUR(FERN,) \
    COLOUR(FOREST_GREEN,) \
    COLOUR(FOUNTAIN_BLUE,) \
    COLOUR(CARIBBEAN_BLUE,) \
    COLOUR(CRYSTAL_BLUE,) \
    COLOUR(MEDIUM_CHARTREUSE,) /* 076 */ \
    COLOUR(FRESH_GREEN,) \
    COLOUR(PALE_GREEN_ONION,) \
    COLOUR(MEDIUM_AQUAMARINE,) \
    COLOUR(MEDIUM_TURQUOISE,) \
    COLOUR(TURQUOISE_BLUE,) \
    COLOUR(BRIGHT_LIME_GREEN,) /* 082 */ \
    COLOUR(LIGHT_BRIGHT_GREEN,) \
    COLOUR(DRAGON_GREEN,) \
    COLOUR(SEA_GREEN,) \
    COLOUR(AQUAMARINE,) \
    COLOUR(ELECTRIC_BLUE,) \
    COLOUR(DARK_RED,) /* 088 */ \
    COLOUR(LIGHT_FUCHSIA,) \
    COLOUR(DEEP_MAGENTA,) \
    COLOUR(VIOLET_EGGPLANT,) \
    COLOUR(DARK_VIOLET,) \
    COLOUR(BLUE_LILAC,) \
    COLOUR(AUTUMN_LEAF_BROWN,) /* 094 */ \
    COLOUR(SUNSET_RED,) \
    COLOUR(RED_LILAC,) \
    COLOUR(ORCHID_MAUVE,) \
    COLOUR(PALE_PURPLE,) \
    COLOUR(MEDIUM_SLATE_BLUE,) \
    COLOUR(SMOOTHIE_GREEN,) /* 100 */ \
    COLOUR(OLIVE_GREY,) \
    COLOUR(PEARL_LIGHT_GREY,) \
    COLOUR(OPERA_MAUVE,) \
    COLOUR(TRUE_LAVENDER,) \
    COLOUR(LIGHT_SLATE_BLUE,) \
    COLOUR(BRILLIANT_GREEN,) /* 106 */ \
    COLOUR(APPLE_GREEN,) \
    COLOUR(ANISEED_LEAF_GREEN,) \
    COLOUR(JADE_GREEN,) \
    COLOUR(PACIFIC_BLUE,) \
    COLOUR(MEDIUM_SKY_BLUE,) \
    COLOUR(PARIS_GREEN,) /* 112 */\
    COLOUR(PALE_GREEN,) \
    COLOUR(DENTIST_GREEN,) \
    COLOUR(SILK_GREEN,) \
    COLOUR(MOUNTAIN_LAKE_BLUE,) \
    COLOUR(LIGHT_SKY_BLUE,) \
    COLOUR(CHARTREUSE,) /* 118 */\
    COLOUR(CRYSTAL_GREEN,) \
    COLOUR(PASTEL_GREEN,) \
    COLOUR(FOAM_GREEN,) \
    COLOUR(LIGHT_AQUA,) \
    COLOUR(WATERY_GREEN,) \
    COLOUR(BLOOD_RED,) /* 124 */\
    COLOUR(RASPBERRY_RED,) \
    COLOUR(MEDIUM_VIOLET_RED,) \
    COLOUR(VIOLET_PURPLE,) \
    COLOUR(VIBRANT_PURPLE,) \
    COLOUR(PURPLE,) \
    COLOUR(ELM_BROWN_RED,) /* 130 */ \
    COLOUR(ALSIKE_CLOVER_RED,) \
    COLOUR(TULIP_PINK,) \
    COLOUR(ORCHID_PURPLE,) \
    COLOUR(AMETHYST,) \
    COLOUR(MEDIUM_PURPLE,) \
    COLOUR(PIRATE_GOLD,) /* 136 */ \
    COLOUR(DARK_SAND,) \
    COLOUR(GREY_BEIGE,) \
    COLOUR(STAGE_MAUVE,) \
    COLOUR(LAVENDER,) \
    COLOUR(LIGHT_PURPLE,) \
    COLOUR(HONEY_YELLOW,) /* 142 */ \
    COLOUR(PALE_OAK,) \
    COLOUR(STONE,) \
    COLOUR(AGATE_GREY,) \
    COLOUR(ROCK_BLUE,) \
    COLOUR(PASTEL_BLUE,) \
    COLOUR(LEMON_YELLOW,) /* 148 */ \
    COLOUR(CONIFER,) \
    COLOUR(PALE_OLIVE_GREEN,) \
    COLOUR(PALE_LEAF,) \
    COLOUR(AQUA_ISLAND,) \
    COLOUR(CORAL_BLUE,) \
    COLOUR(LIME_GREEN,) /* 154 */ \
    COLOUR(LIGHT_LIME_GREEN,) \
    COLOUR(LIGHT_PALE_GREEN,) \
    COLOUR(LIGHT_SEAFOAM_GREEN,) \
    COLOUR(CAPRI_WATER_BLUE,) \
    COLOUR(PALE_TURQUOISE,) \
    COLOUR(LAVA,) /* 160 */ \
    COLOUR(STRAWBERRY_RED,) \
    COLOUR(RED_VIOLET,) \
    COLOUR(NAIL_POLISH_PINK,) \
    COLOUR(LIGHT_MAGENTA,) \
    COLOUR(PHLOX,) \
    COLOUR(DEEP_ORANGE,) /* 166 */ \
    COLOUR(PALE_RED,) \
    COLOUR(HEATHER_VIOLET,) \
    COLOUR(PERSIAN_PINK,) \
    COLOUR(ORCHID,) \
    COLOUR(MEDIUM_ORCHID,) \
    COLOUR(AUTUMN_LEAF_ORANGE,) /* 172 */ \
    COLOUR(COPPER,) \
    COLOUR(DUSTY_PINK,) \
    COLOUR(TECHNO_PINK,) \
    COLOUR(LAVENDER_PINK,) \
    COLOUR(VIOLET,) \
    COLOUR(BURNT_YELLOW,) /* 178 */ \
    COLOUR(DESERT,) \
    COLOUR(BEIGE,) \
    COLOUR(CLAM_SHELL,) \
    COLOUR(COSMETIC_MAUVE,) \
    COLOUR(PLUM,) \
    COLOUR(COLZA_YELLOW,) /* 184 */ \
    COLOUR(SAND,) \
    COLOUR(STRAW,) \
    COLOUR(IVORY,) \
    COLOUR(IRON,) \
    COLOUR(FOG,) \
    COLOUR(MIDDLE_YELLOW,) /* 190 */ \
    COLOUR(SULFUR_YELLOW,) \
    COLOUR(LIME_JUICE_GREEN,) \
    COLOUR(ICE_COLD_GREEN,) \
    COLOUR(ALABASTER_GREEN,) \
    COLOUR(CHALKY_BLUE_WHITE,) \
    COLOUR(RED_2,) /* 196 - duplicate */ \
    COLOUR(LUMINOUS_RED,) \
    COLOUR(DEEP_PINK,) \
    COLOUR(WARM_MAGENTA,) \
    COLOUR(BRIGHT_MAGENTA,) \
    COLOUR(MAGENTA_2,) /* duplicate */ \
    COLOUR(LUMINOUS_ORANGE,) /* 202 */ \
    COLOUR(CORAL_ORANGE,) \
    COLOUR(BRINK_PINK,) \
    COLOUR(HOT_PINK,) \
    COLOUR(CANDY_PINK,) \
    COLOUR(VIOLET_PINK,) \
    COLOUR(DARK_ORANGE,) /* 208 */ \
    COLOUR(MANGO_ORANGE,) \
    COLOUR(FLAMINGO_RED,) \
    COLOUR(ROSA,) \
    COLOUR(LIGHT_ORCHID,) \
    COLOUR(LIGHT_FUCHSIA_PINK,) \
    COLOUR(YELLOW_ORANGE,) /* 214 */ \
    COLOUR(APRICOT,) \
    COLOUR(PEACH,) \
    COLOUR(NATURE_APRICOT,) \
    COLOUR(SOFT_PINK,) \
    COLOUR(LIGHT_PLUM,) \
    COLOUR(BRIGHT_GOLD,) /* 220 */ \
    COLOUR(PALE_GOLD,) \
    COLOUR(GOLDEN_SAND,) \
    COLOUR(LIGHT_PEACH,) \
    COLOUR(PALE_PINK,) \
    COLOUR(PINK_LACE,) \
    COLOUR(CANARY,) /* 226 */ \
    COLOUR(DYNAMIC_YELLOW,) \
    COLOUR(BUTTER,) \
    COLOUR(WHEAT,) \
    COLOUR(MOON_GLOW,) \
    COLOUR(WHITE_2,) /* duplicate */ \
    COLOUR(NIGHT,) /* 232 - 3% */ \
    COLOUR(ONYX,) /* 7% */ \
    COLOUR(OIL,) /* 11% */ \
    COLOUR(BALTIC_SEA,) /* 15% */ \
    COLOUR(CHARCOAL,) /* 19% */ \
    COLOUR(IRIDIUM,) /* 23% */ \
    COLOUR(GRAVEL,) /* 238 - 27% */ \
    COLOUR(FUSCOUS_GREY,) /* 30% */ \
    COLOUR(VAMPIRE_GREY,) /* 34% */ \
    COLOUR(STORM_DUST,) /* 38% */ \
    COLOUR(DOVE_GREY,) /* 42% */ \
    COLOUR(BOULDER,) /* 46% */ \
    COLOUR(GRANITE,) /* 244 - 50%, actually a duplicate of GREY50 */ \
    COLOUR(OSLO_GREY,) /* 54% */ \
    COLOUR(REGENT_GREY,) /* 58% */ \
    COLOUR(DUSTY_GREY,) /* 62% */ \
    COLOUR(ALUMINIUM,) /* 66% */ \
    COLOUR(GREY_CLOUD,) /* 70% */ \
    COLOUR(ASH_GREY,) /* 250 - 74% */ \
    COLOUR(SILVER,) /* 78% */ \
    COLOUR(GREY_GOOSE,) /* 81% */ \
    COLOUR(LIGHT_GREY,) /* 85% */ \
    COLOUR(PLATINUM,) /* 89% */ \
    COLOUR(GREY93,) \

DECLARE_ENUM(colour, COLOUR_ENUM)

/* default background colour for the playing field */
#define PF_BG BLACK

/*
 * UI colours
 * These are the 8 duplicate colours of the palette, reassigned
 * to use them as UI colour pairs.
 */
#define UI_BRIGHT_FG  BLACK_2
#define UI_FG         BLUE_2
#define UI_BORDER     GREEN_2
#define UI_TITLE      CYAN_2
#define UI_KEY        RED_2
#define UI_FG_REVERSE MAGENTA_2
#define UI_HL_REVERSE GREY50_2
#define UI_BG         WHITE_2

/* UI color pairs */
#define CP_UI_BRIGHT_FG  COLOR_PAIR(UI_BRIGHT_FG)
#define CP_UI_FG         COLOR_PAIR(UI_FG)
#define CP_UI_BORDER     COLOR_PAIR(UI_BORDER)
#define CP_UI_TITLE      COLOR_PAIR(UI_TITLE)
#define CP_UI_KEY        COLOR_PAIR(UI_KEY)
#define CP_UI_FG_REVERSE COLOR_PAIR(UI_FG_REVERSE)
#define CP_UI_HL_REVERSE COLOR_PAIR(UI_HL_REVERSE)

/* UI colour schemes */
#define UI_COLOUR_SCHEME_ENUM(UI_COLOUR_SCHEME) \
    UI_COLOUR_SCHEME(TRADITIONAL,) \
    UI_COLOUR_SCHEME(RETROBOX,) \
    UI_COLOUR_SCHEME(STASIS,) \
    UI_COLOUR_SCHEME(SOURLICK,) \
    UI_COLOUR_SCHEME(FODDER,) \
    UI_COLOUR_SCHEME(MELLOW,) \
    UI_COLOUR_SCHEME(UI_COLOUR_SCHEME_MAX,) \

DECLARE_ENUM(ui_colour_scheme, UI_COLOUR_SCHEME_ENUM)

// Colour pair initialisation
void colours_init(int ui_colour_scheme);

/*
 * Colour pair lookup for colour augmented strings
 *
 * For the playfield, this accepts all available colours,
 * for the UI only a limited amount is accepted:
 *     - WHITE
 *     - TITLE
 *     - EMPH
 *
 * @param color name string
 * @param the background colour (i.e. PF_BG or UI_BG)
 */
int colour_lookup(const char *colour_name, int bg);

#endif
