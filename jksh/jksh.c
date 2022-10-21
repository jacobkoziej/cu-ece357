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

#include <stdlib.h>
#include <stdio.h>
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
	}

	return EXIT_SUCCESS;
}
