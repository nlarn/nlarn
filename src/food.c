/*
 * food.c
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

static const food_data foods[FT_MAX] =
{
    /* id                name              weight*/
    { FT_NONE,           "",               0,   },
    { FT_FORTUNE_COOKIE, "fortune cookie", 2,   },
};

food *food_new(int food_type)
{
    food *nfood;

    assert(food_type > FT_NONE && food_type < FT_MAX);

    nfood = g_malloc0(sizeof(food));
    nfood->type = food_type;

    return nfood;
}

void food_destroy(food *f)
{
    assert(f != NULL);

	g_free(f);
}

inline char *food_get_name(food *f)
{
    assert(f != NULL && f->type > FT_NONE && f->type < FT_MAX);
    return foods[f->type].name;
}

inline int food_get_weight(food *f)
{
    assert(f != NULL && f->type > FT_NONE && f->type < FT_MAX);
    return foods[f->type].weight;
}

/* function to return a random fortune from the fortune file  */
char *food_get_fortune(food *f, char *fortune_file)
{

    /* array of pointers to fortunes */
    static GPtrArray *fortunes = NULL;

    assert(f != NULL && f->type == FT_FORTUNE_COOKIE);

    if (!fortunes) {

    	/* read in the fortunes */

		size_t len = 0;
		char buffer[80];
		char *tmp = 0;
		FILE *fortune_fd;

    	fortunes = g_ptr_array_new();

        /* open the file */
        fortune_fd = fopen(fortune_file, "r");
        if (fortune_fd == NULL) {
        	/* can't find file */
        	tmp = "Help me! I can't find the fortune file!";
            g_ptr_array_add(fortunes, tmp);
        }
        else
        {
            /* read in the entire fortune file */
            while((fgets(buffer, 79, fortune_fd))) {
                /* replace EOL with \0 */
                len = (size_t)(strchr(buffer, '\n') - (char *)&buffer);
                buffer[len] = '\0';

                /* keep the line */
                tmp = g_malloc((len + 1) * sizeof(char));
                memcpy(tmp, &buffer, (len + 1));
                g_ptr_array_add(fortunes, tmp);
            }

            fclose(fortune_fd);
        }
    }

    return g_ptr_array_index(fortunes, rand_0n(fortunes->len));
}
