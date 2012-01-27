
/*
 *     Copyright (C) 2011-2012 Andreas Schönfelder
 *     https://github.com/escoand/socksswitch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "match.h"

int matching(char *text, char *pattern) {
    char *orig_pattern = pattern;
    //char *orig_text = text;

    //printf("%s\t~\t%s", text, pattern);

    /* matching beginning */
    if (*pattern == '^')
	pattern++;

    /* run through text */
    for (; *text != '\0'; text++) {
	//printf("\n %s\t~\t%s", text, pattern);

	/* beginning */
	if (*pattern == '^')
	    return 0;

	/* ending */
	else if (*pattern == '\0')
	    return 1;
	else if (*pattern == '$' && *(pattern + 1) != '\0')
	    return 0;

	/* any char */
	else if (*pattern == '?')
	    pattern++;

	/* any char */
	else if (*pattern == '*') {
	    pattern++;
	    orig_pattern = pattern;
	}

	/* matching */
	else if (*text == *pattern)
	    pattern++;

	/* not matching */
	else if (*text != *pattern && pattern != orig_pattern) {
	    pattern = orig_pattern;
	    text--;
	}
    }

    if (*pattern == '\0' || (*pattern == '$' && *(pattern + 1) == '\0'))
	return 1;
    else
	return 0;
}
