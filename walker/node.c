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
#include <sys/stat.h>
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

		if ((size_t) ret == bufsiz) {
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

const char *strmode(const mode_t mode)
{
	char *buf = malloc(11);  // "-rwxrwxrwx\0"
	if (!buf) return NULL;

	buf[0] = ' ';
	if (S_ISBLK(mode))  buf[0] = 'b';
	if (S_ISCHR(mode))  buf[0] = 'c';
	if (S_ISFIFO(mode)) buf[0] = 'p';
	if (S_ISREG(mode))  buf[0] = '-';
	if (S_ISDIR(mode))  buf[0] = 'd';
	if (S_ISLNK(mode))  buf[0] = 'l';
	if (S_ISSOCK(mode)) buf[0] = 's';

	buf[1] = mode & S_IRUSR ? 'r' : '-';
	buf[2] = mode & S_IWUSR ? 'w' : '-';
	buf[3] = mode & S_IXUSR ? 'x' : '-';

	buf[4] = mode & S_IRGRP ? 'r' : '-';
	buf[5] = mode & S_IWGRP ? 'w' : '-';
	buf[6] = mode & S_IXGRP ? 'x' : '-';

	buf[7] = mode & S_IROTH ? 'r' : '-';
	buf[8] = mode & S_IWOTH ? 'w' : '-';
	buf[9] = mode & S_IXOTH ? 'x' : '-';

	if (mode & S_ISUID) buf[3] = mode & S_IXUSR ? 's' : 'S';
	if (mode & S_ISGID) buf[6] = mode & S_IXGRP ? 's' : 'S';
	if (mode & S_ISVTX) buf[9] = mode & S_IXOTH ? 't' : 'T';

	buf[10] = '\0';

	return buf;
}
