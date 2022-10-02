/*
 * walker.c -- recursive file system lister
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

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "node.h"


#define LNK_CHK (1 << 0)
#define MTM_CHK (1 << 1)
#define UID_CHK (1 << 2)
#define VOL_CHK (1 << 3)


static uint_fast8_t flags;

static const char *path = ".";
static dev_t       sl_dev;
static ino_t       sl_ino;
static dev_t       vol;
static uid_t       uid;
static long        mtime;

static struct stat st;
static node_t      node;


static int walk(const char *path)
{
	// handle non-directories
	if (!S_ISDIR(node.stat.st_mode)) {
		node.path = path;
		if (lstat(node.path, &node.stat) < 0) return -1;
		if (node_parse(&node) < 0)            return -1;
		if (node_fprint(stdout, &node) < 0)   return -1;
		return 0;
	}

	DIR *dir = opendir(path);
	if (!dir) return -1;

	size_t  pathsiz    = strlen(path) + 1;
	size_t  bufsiz     = PATH_MAX;
	size_t  maxentsiz  = PATH_MAX;
	char   *buf;

	while (pathsiz > bufsiz) bufsiz *= 2;
	buf = malloc(bufsiz);
	if (!buf) goto error;

	strncpy(buf, path, pathsiz);
	buf[pathsiz - 1] = '/';
	if ((buf[pathsiz - 2] == '/')) --pathsiz;
	maxentsiz = bufsiz - pathsiz;

	struct dirent *ent;

	while (errno = 0, ent = readdir(dir)) {
		if (!strcmp(ent->d_name, "."))  continue;
		if (!strcmp(ent->d_name, "..")) continue;

		size_t entsiz = strlen(ent->d_name) + 1;

		if (entsiz > maxentsiz) {
			do {
				bufsiz    *= 2;
				maxentsiz  = bufsiz - pathsiz;
			} while (entsiz > maxentsiz);

			char *tmp = realloc(buf, bufsiz);
			if (!tmp) goto error;

			buf = tmp;
		}

		strncpy(buf + pathsiz, ent->d_name, entsiz);
		node.path = buf;

		if (lstat(node.path, &node.stat) < 0) goto error;

		// free previous symlink path (if allocated)
		if (node.slpath) {
			free((char*) node.slpath);
			node.slpath = NULL;
		}

		if (node_parse(&node) < 0)          goto error;
		if (node_fprint(stdout, &node) < 0) goto error;

		if (S_ISDIR(node.stat.st_mode) && (walk(buf) < 0)) goto error;
	}
	if (errno) goto error;

	free(buf);
	closedir(dir);

	return 0;

error:
	if (buf) free(buf);
	if (dir) closedir(dir);

	return -1;
}


int main(int argc, char **argv)
{
	{
		int opt;
		const char *strlink  = NULL;
		const char *strmtime = NULL;
		const char *struser  = NULL;

		opterr = 0;
		while ((opt = getopt(argc, argv, ":hl:m:u:x")) != -1) {
			switch (opt) {
			case 'h':
				printf(
					"usage: %s [-l target] [-u user] [-x] [PATH]\n",
					argv[0]
				);
				return 0;

			case 'l':
				flags   |= LNK_CHK;
				strlink  = optarg;
				break;

			case 'm':
				flags    |= MTM_CHK;
				strmtime  = optarg;
				break;

			case 'u':
				flags   |= UID_CHK;
				struser  = optarg;
				break;

			case 'x':
				flags |= VOL_CHK;
				break;

			case ':':
				fprintf(
					stderr,
					"missing argument: '%c'\n",
					optopt
				);
				fprintf(
					stderr,
					"try '%s -h' for usage information\n",
					argv[0]
				);
				return EXIT_FAILURE;

			case '?':
				fprintf(
					stderr,
					"unknown flag: '%c'\n",
					optopt
				);
				fprintf(
					stderr,
					"try '%s -h' for usage information\n",
					argv[0]
				);
				return EXIT_FAILURE;

			default:
				return EXIT_FAILURE;
			}
		}

		if (argc - optind) path = argv[argc - 1];

		if (flags & LNK_CHK) {
			if (lstat(strlink, &st) < 0) {
				perror("failed to get link target file status");
				return EXIT_FAILURE;
			}
			sl_dev = st.st_dev;
			sl_ino = st.st_ino;
		}

		if (flags & MTM_CHK) {
			char *invalid = NULL;
			mtime = strtoul(strmtime, &invalid, 10);

			if (*strmtime && *invalid) {
				fprintf(
					stderr,
					"invalid mtime: %s\n",
					strmtime
				);
				return EXIT_FAILURE;
			}
		}

		if (flags & UID_CHK) {
			char *invalid = NULL;
			uid = strtoul(struser, &invalid, 10);

			uid &= UINT_MAX;  // mask invalid UID

			if (*struser && *invalid) {
				struct passwd *user = getpwnam(struser);
				if (!user) {
					if (
						!errno            ||
						(errno == ENOENT) ||
						(errno == ESRCH)  ||
						(errno == EBADF)  ||
						(errno == EPERM)
					) {
						fprintf(
							stderr,
							"no such user: %s\n",
							struser
						);
					} else {
						perror("getpwnam() failed");
					}

					return EXIT_FAILURE;
				}

				uid = user->pw_uid;
			}
		}

		if (flags & VOL_CHK) {
			if (lstat(path, &st) < 0) {
				perror("failed to get volume information");
				return EXIT_FAILURE;
			}
			vol = st.st_dev;
		}
	}


	if (lstat(path, &node.stat) < 0) {
		perror("walk failed");
		return EXIT_FAILURE;
	}
	if (walk(path) < 0) {
		perror("walk failed");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
