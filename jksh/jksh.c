/*
 * jksh.c -- Jacob Koziej's basic shell
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

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"


static char *ps1;


static void cleanup(void)
{
	if (ps1) free(ps1);
}

int main(void)
{
	atexit(cleanup);

	char *tmp;

	// prep PS1
	if ((tmp = getenv("PS1")) && !(ps1 = strdup(tmp))) {
		perror("couldn't allocate buffer for `PS1`");
		return EXIT_FAILURE;
	}
	if (!ps1 && !(ps1 = strdup(""))) {
		perror("couldn't allocate buffer for `PS1`");
		return EXIT_FAILURE;
	}

	while (1) {
		printf("%s", ps1);

		char   *input = NULL;
		size_t  inputsiz;

		if (errno = 0, getline(&input, &inputsiz, stdin) < 0) {
			free(input);

			// EOF reached
			if (!errno) break;

			perror("failed to read shell input");
			return EXIT_FAILURE;
		}

		// remove trailing newline (if found)
		size_t end = strlen(input) - 1;
		if (input[end] == '\n') input[end] = '\0';

		char **tokens = tokenize(input);
		if (!tokens) {
			free(input);
			perror("failed to tokenize shell input");
			return EXIT_FAILURE;
		}

		if (!*tokens || **tokens == '#') goto no_cmd;

no_cmd:
		free(input);
		free_tokens(tokens);
	}

	return EXIT_SUCCESS;
}
