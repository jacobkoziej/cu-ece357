/*
 * node.c -- node utils
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

#include "node.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>


const char *slpath(const char *restrict path)
{
	size_t bufsiz = PATH_MAX;
	char  *buf    = NULL;

	do {
		char *tmp = realloc(buf, bufsiz);
		if (!tmp) goto error;
		buf = tmp;

		ssize_t ret = readlink(path, buf, bufsiz);

		if (ret < 0) goto error;

		if (ret == bufsiz) {
			size_t tmp = bufsiz * 2;

			if ((tmp > SSIZE_MAX) || (tmp < bufsiz)) goto error;

			bufsiz = tmp;
			continue;
		}

		buf[ret] = '\0';
		bufsiz = ret + 1;
		break;
	} while (true);

	char *tmp = realloc(buf, bufsiz);
	if (!tmp) goto error;
	buf = tmp;

	return buf;

error:
	if (buf) free(buf);

	return NULL;
}
