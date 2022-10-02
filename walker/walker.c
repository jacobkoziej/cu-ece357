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

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


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

	return EXIT_SUCCESS;
}
