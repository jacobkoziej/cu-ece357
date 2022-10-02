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

#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>


int node_parse(node_t *restrict node)
{
	if (S_ISLNK(node->stat.st_mode)) {
		node->slpath = slpath(node->path);
		if (!node->slpath) return -1;
	}

	strmode(node->mode, node->stat.st_mode);

	node->passwd = getpwuid(node->stat.st_uid);
	if (!node->passwd) goto error;

	node->group = getgrgid(node->stat.st_gid);
	if (!node->group) goto error;

	return 0;

error:
	if (node->slpath) free((void*) node->slpath);

	return -1;
}

int node_fprint(FILE *restrict stream, const node_t *restrict node)
{
	int total = 0;
	int tmp;

	// inode
	tmp = fprintf(stream, "%ld", node->stat.st_ino);
	if (tmp < 0) return -1;
	total += tmp;
	if (fputc('\t', stream) == EOF) return -1;
	++total;

	// blocks
	tmp = fprintf(stream, "%ld", node->stat.st_blocks);
	if (tmp < 0) return -1;
	total += tmp;
	if (fputc('\t', stream) == EOF) return -1;
	++total;

	// mode
	tmp = fprintf(stream, "%s", node->mode);
	if (tmp < 0) return -1;
	total += tmp;
	if (fputc('\t', stream) == EOF) return -1;
	++total;

	// links
	tmp = fprintf(stream, "%ld", node->stat.st_nlink);
	if (tmp < 0) return -1;
	total += tmp;
	if (fputc('\t', stream) == EOF) return -1;
	++total;

	// user
	if (node->passwd->pw_name) {
		tmp = fprintf(stream, "%s", node->passwd->pw_name);
		if (tmp < 0) return -1;
		total += tmp;
		if (fputc('\t', stream) == EOF) return -1;
		++total;
	} else {
		tmp = fprintf(stream, "%d", node->passwd->pw_uid);
		if (tmp < 0) return -1;
		total += tmp;
		if (fputc('\t', stream) == EOF) return -1;
		++total;
	}

	// group
	if (node->group->gr_name) {
		tmp = fprintf(stream, "%s", node->group->gr_name);
		if (tmp < 0) return -1;
		total += tmp;
		if (fputc('\t', stream) == EOF) return -1;
		++total;
	} else {
		tmp = fprintf(stream, "%d", node->group->gr_gid);
		if (tmp < 0) return -1;
		total += tmp;
		if (fputc('\t', stream) == EOF) return -1;
		++total;
	}

	// size
	if (S_ISBLK(node->stat.st_mode) || S_ISCHR(node->stat.st_mode)) {
		tmp = fprintf(
			stream,
			"%d,%d",
			major(node->stat.st_dev),
			minor(node->stat.st_dev)
		);
		if (tmp < 0) return -1;
		total += tmp;
		if (fputc('\t', stream) == EOF) return -1;
		++total;
	} else {
		tmp = fprintf(stream, "%ld", node->stat.st_size);
		if (tmp < 0) return -1;
		total += tmp;
		if (fputc('\t', stream) == EOF) return -1;
		++total;
	}

	// mtime
	char *time = ctime(&node->stat.st_mtim.tv_sec);
	time[strlen(time) - 1] = '\0';                   // remove newline
	tmp = fprintf(stream, "%s", time);
	if (tmp < 0) return -1;
	total += tmp;
	if (fputc('\t', stream) == EOF) return -1;
	++total;

	// path
	tmp = fprintf(stream, "%s", node->path);
	if (tmp < 0) return -1;
	if (node->slpath) {
		tmp = fprintf(stream, " -> %s", node->slpath);
		if (tmp < 0) return -1;
	}
	if (fputc('\n', stream) == EOF) return -1;
	++total;

	return total;
}

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

void strmode(char buf[11], const mode_t mode)
{
	switch (mode & S_IFMT) {
		case S_IFBLK:
			buf[0] = 'b';
			break;

		case S_IFCHR:
			buf[0] = 'c';
			break;

		case S_IFIFO:
			buf[0] = 'p';
			break;

		case S_IFREG:
			buf[0] = '-';
			break;

		case S_IFDIR:
			buf[0] = 'd';
			break;

		case S_IFLNK:
			buf[0] = 'l';
			break;

		case S_IFSOCK:
			buf[0] = 's';
			break;

		default:
			buf[0] = ' ';
	}

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
}
