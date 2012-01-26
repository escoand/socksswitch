/*
 * match.c 
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
