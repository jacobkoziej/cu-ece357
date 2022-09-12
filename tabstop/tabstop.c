/*
 * tabstop.c -- convert tabs to four spaces
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

#include "jkio.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define TABSTOP_BUFSIZ 4096


static struct MYSTREAM *rfp;
static struct MYSTREAM *wfp;


static void cleanup(void)
{
	if (rfp) {
		if (myfclose(rfp) < 0)
			perror("couldn't close reading stream");
		rfp = NULL;
	}
	if (wfp) {
		if (myfclose(wfp) < 0)
			perror("couldn't close writing stream");
		wfp = NULL;
	}
}

int main(int argc, char **argv)
{
	atexit(cleanup);

	int         opt;
	const char *rpath  = NULL;
	const char *wpath  = NULL;
	size_t      bufsiz = TABSTOP_BUFSIZ;

	opterr = 0;
	while ((opt = getopt(argc, argv, ":b:ho:")) != -1) {
		switch (opt) {
			case 'b':;
				// get largest power of 2 up to 64Ki
				long tmp = labs(atol(optarg));
				if (tmp > (1 << 16)) tmp = 1 << 16;

				for (int i = 16; i >= 0; i--)
					if (tmp & (1 << i)) {
						bufsiz = 1 << i;
						break;
					}
				break;

			case 'h':
				printf(
					"usage: %s [-b bufsiz] [-o output] [FILE]\n",
					argv[0]
				);
				return 0;

			case 'o':
				wpath = optarg;
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
				return 255;

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
				return 255;

			default:
				return 255;
		}
	}

	if (argc - optind) rpath = argv[argc - 1];
	rfp = (rpath)
		? myfopen(rpath, O_RDONLY, bufsiz)
		: myfdopen(STDIN_FILENO, O_RDONLY, bufsiz);
	if (!rfp) {
		perror("couldn't open file for reading");
		return 255;
	}

	wfp = (wpath)
		? myfopen(wpath, O_WRONLY, bufsiz)
		: myfdopen(STDOUT_FILENO, O_WRONLY, bufsiz);
	if (!wfp) {
		perror("couldn't open file for writing");
		return 255;
	}

	errno = 0;
	int val;
	while ((val = myfgetc(rfp)) != -1) {
		if (errno) {
			perror("couldn't get character from stream");
			return 255;
		}

		// check if we need to make a 4-space tabstop
		int lim = (val == '\t') ? val = ' ', 4 : 1;

		for (int i = 0; i < lim; i++)
			if (myfputc(val, wfp) < 0) {
				perror("couldn't put character to stream");
				return 255;
			}
	}

	return 0;
}
