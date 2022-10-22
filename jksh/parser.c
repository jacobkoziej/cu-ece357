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

#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define DEFAULT_TOKENS 4
#define DEFAULT_DELIMS " \t"


char **argv_gen(job_t *job, int argc, char **argv)
{
	char **args = calloc(DEFAULT_TOKENS, sizeof(char*));
	if (!args) return NULL;

	size_t args_cnt = DEFAULT_TOKENS;

	size_t i;
	for (i = 0; job->tokens[i]; i++) {
		// we want to have enough space for the
		// current arg + NULL terminator
		if (i + 2 >= args_cnt) {
			args_cnt *= 2;
			char **tmp = realloc(args, args_cnt * sizeof(char*));
			if (!tmp) goto error;

			args = tmp;
		}

		char *cur = job->tokens[i];

		// environment variables
		if (*cur == '$') {
			// remove '$'
			++cur;

			// previous return value
			if (*cur == '?' && cur[1] == '\0') {
				// return code is at most 3 digits
				char *buf = calloc(4, sizeof(char));
				if (!buf) goto error;

				snprintf(buf, 4, "%d", job->prv_ret & 0xff);

				args[i] = buf;
				continue;
			}

			char *tmp;

			// positional arguments
			tmp = cur;
			while (*tmp && isdigit(*tmp)) ++tmp;
			if (!*tmp) {
				int val = atoi(cur);

				if (val > argc - 1) continue;

				args[i] = strdup(argv[val]);
				if (!args[i]) goto error;
			}

			tmp = getenv(cur);
			if (!tmp) continue;

			args[i] = strdup(tmp);
			if (!args[i]) goto error;

			continue;
		}

		// redirection
		if (*cur == '<') {
			// remove '<'
			++cur;

			if ((job->stdin >= 0) && (close(job->stdin) < 0)) {
				perror("failed to close input redirection");
				goto error;
			}

			int fd = open(cur, O_RDONLY);
			if (fd < 0) {
				perror("failed to open file for input");
				goto error;
			}

			job->stdin = fd;
			continue;
		}

		if ((*cur == '>') || ((*cur == '2') && (cur[1] == '>'))) {
			int mode = O_CREAT | O_WRONLY;

			bool stderr = *cur == '2';

			// remove '2'
			if (stderr) ++cur;

			// remove '>'
			++cur;

			// determine output mode
			mode |= (*cur == '>') ? ++cur, O_APPEND : O_TRUNC;

			int *existing = (stderr) ? &job->stderr : &job->stdout;

			if ((*existing >= 0) && (close(*existing) < 0)) {
				perror("failed to close output redirection");
				goto error;
			}

			int fd = open(cur, mode, 0666);
			if (fd < 0) {
				perror("failed to open file for output");
				goto error;
			}

			*existing = fd;
			continue;
		}

		args[i] = strdup(cur);
		if (!args[i]) goto error;
	}

	return args;

error:
	while (i) {
		free(args[i]);
		--i;
	}

	free(args);

	return NULL;
}

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
