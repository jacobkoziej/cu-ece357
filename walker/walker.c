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


typedef struct walk_s {
	uint_fast8_t flags;
	dev_t        sl_dev;
	ino_t        sl_ino;
	dev_t        vol;
	uid_t        uid;
	long         mtime;
	node_t       node;
} walk_t;


static char   pathbuf[PATH_MAX];
static walk_t walk;


static int walker(char *path, size_t pathuse, size_t pathsiz)
{
	if (lstat(path, &walk.node.stat) < 0) {
		perror(path);
		return (errno != EACCES) ? 0 : -1;
	}

	walk.node.path = path;
	if (walk.node.slpath) {
		free((char*) walk.node.slpath);
		walk.node.slpath = NULL;
	}

	if (node_parse(&walk.node) < 0) {
		fprintf(
			stderr,
			"failed to parse node %s: %s\n",
			walk.node.path,
			strerror(errno)
		);

		return -1;
	}

	if (node_fprint(stdout, &walk.node) < 0) {
		fprintf(
			stderr,
			"failed to print node %s: %s\n",
			walk.node.path,
			strerror(errno)
		);

		return -1;
	}

	if (!S_ISDIR(walk.node.stat.st_mode)) return 0;

	// make sure we can add a trailing '/' and paths
	if (pathuse + NAME_MAX + 1 > pathsiz) {
		fprintf(
			stderr,
			"can't recurse into %s: %s\n",
			path,
			strerror(ENAMETOOLONG)
		);

		return -1;
	}

	path[pathuse++ - 1] = '/';
	path[pathuse   - 1] = '\0';

	char *tail = path + pathuse - 1;

	DIR *dir = opendir(path);
	if (!dir) {
		fprintf(
			stderr,
			"failed to open directory: %s: %s\n",
			path,
			strerror(errno)
		);

		return -1;
	}

	struct dirent *ent;
	while (errno = 0, ent = readdir(dir)) {
		if (!strcmp(ent->d_name, "."))  continue;
		if (!strcmp(ent->d_name, "..")) continue;

		size_t d_namelen = strlen(ent->d_name);
		strncpy(tail, ent->d_name, NAME_MAX + 1);
		walker(path, pathuse + d_namelen, pathsiz);
	}
	if (errno) goto error;

	closedir(dir);

	return 0;

error:
	if (dir) closedir(dir);

	return -1;
}


static void cleanup(void)
{
	if (walk.node.slpath) free((char*) walk.node.slpath);
}

int main(int argc, char **argv)
{
	atexit(cleanup);

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
				walk.flags |= LNK_CHK;
				strlink     = optarg;
				break;

			case 'm':
				walk.flags |= MTM_CHK;
				strmtime    = optarg;
				break;

			case 'u':
				walk.flags |= UID_CHK;
				struser     = optarg;
				break;

			case 'x':
				walk.flags |= VOL_CHK;
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

		pathbuf[PATH_MAX - 1] = '\0';
		strncpy(
			pathbuf,
			(argc - optind) ? argv[argc - 1] : ".",
			PATH_MAX
		);
		if (pathbuf[PATH_MAX - 1] != '\0') {
			errno = ENAMETOOLONG;
			perror(argv[argc - 1]);
			return EXIT_FAILURE;
		}

		if (walk.flags & LNK_CHK) {
			if (lstat(strlink, &walk.node.stat) < 0) {
				perror("failed to get link target file status");
				return EXIT_FAILURE;
			}
			walk.sl_dev = walk.node.stat.st_dev;
			walk.sl_ino = walk.node.stat.st_ino;
		}

		if (walk.flags & MTM_CHK) {
			char *invalid = NULL;
			walk.mtime = strtoul(strmtime, &invalid, 10);

			if (*strmtime && *invalid) {
				fprintf(
					stderr,
					"invalid mtime: %s\n",
					strmtime
				);
				return EXIT_FAILURE;
			}
		}

		if (walk.flags & UID_CHK) {
			char *invalid = NULL;
			walk.uid = strtoul(struser, &invalid, 10);

			walk.uid &= UINT_MAX;  // mask invalid UID

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

				walk.uid = user->pw_uid;
			}
		}

		if (walk.flags & VOL_CHK) {
			if (lstat(pathbuf, &walk.node.stat) < 0) {
				perror("failed to get volume information");
				return EXIT_FAILURE;
			}
			walk.vol = walk.node.stat.st_dev;
		}
	}

	if (walker(pathbuf, strlen(pathbuf) + 1, PATH_MAX) < 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
