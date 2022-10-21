/*
 * builtin.c -- shell builtin commands
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

#include "builtin.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


extern char **environ;


int cd(char **argv, char *homedir)
{
	if (*argv && argv[1]) {
		fprintf(stderr, "cd: too many arguments\n");
		return 1;
	}

	char *dir = (*argv) ? *argv : homedir;

	if (chdir(dir) < 0) {
		perror("cd");
		return 1;
	}

	return 0;
}

int export(char **argv)
{
	if (!*argv) {
		for (char **var = environ; *var; var++)
			puts(*var);

		return 0;
	}

	char *key = *argv;
	char *val = *argv;

	// split KEY=val
	while (*val && *val != '=') ++val;
	if (*val == '=') *(val++) = '\0';

	if (setenv(key, val, 1) < 0) {
		perror("failed to set environment variable");
		return 1;
	}

	return 0;
}

int pwd(char **argv)
{
	if (*argv) {
		fprintf(stderr, "pwd: too many arguments\n");
		return 1;
	}

	size_t  bufsiz = PATH_MAX;
	char   *buf = malloc(PATH_MAX);
	if (!buf) {
		perror("failed to generate cwd buffer");
		return 1;
	}

	while (1) {
		errno = 0;
		if (!getcwd(buf, bufsiz)) {
			if (errno != ERANGE) goto error;

			bufsiz *= 2;

			char *tmp = realloc(buf, bufsiz);
			if (!tmp) goto error;

			buf = tmp;

			continue;
		}

		break;
	}

	printf("%s\n", buf);

	free(buf);

	return 0;

error:
	free(buf);
	perror("failed to get cwd");

	return 1;
}

void shexit(char **argv)
{
	if (!*argv) exit(EXIT_SUCCESS);

	// exits with 0 on invalid input
	exit(atoi(*argv));
}
