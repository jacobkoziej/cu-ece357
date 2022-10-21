/*
 * parser.c -- shell parser
 * Copyright (C) 2022  Jacob Koziej <jacobkoziej@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "parser.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_TOKENS 4
#define DEFAULT_DELIMS " \t"


void free_tokens(char **tokens)
{
	if (!tokens) return;

	for (char **tmp = tokens; *tmp; tmp++)
		free(*tmp);

	free(tokens);
}

char **tokenize(char *input)
{
	char **tokens = calloc(DEFAULT_TOKENS, sizeof(char*));
	if (!tokens) return NULL;

	size_t token_cnt = DEFAULT_TOKENS;

	size_t  i = 0;
	char   *token = strtok(input, DEFAULT_DELIMS);
	while (token) {
		// we want to have enough space for the
		// current term + NULL terminator
		if (i + 2 >= token_cnt) {
			token_cnt *= 2;
			char **tmp = realloc(tokens, token_cnt * sizeof(char*));
			if (!tmp) goto error;

			tokens = tmp;
		}

		tokens[i] = strdup(token);
		if (!tokens[i]) goto error;
		++i;
		token = strtok(NULL, DEFAULT_DELIMS);
	}

	tokens[i] = NULL;

	return tokens;

error:
	while (i) {
		free(tokens[i]);
		--i;
	}

	free(tokens);

	return NULL;
}
