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
#include <string.h>
#include <unistd.h>


#define TABSTOP_BUFSIZ 4096


static struct MYSTREAM *rfp;
static struct MYSTREAM *wfp;


static void cleanup(void)
{
	if (rfp) {
		myfclose(rfp);
		rfp = NULL;
	}
	if (wfp) {
		myfclose(wfp);
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

	while ((opt = getopt(argc, argv, "b:o:")) != -1) {
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

			case 'o':
				wpath = optarg;
				break;

			default:
				return 255;
		}
	}

	if (argc - optind) rpath = argv[argc - 1];
	rfp = (rpath)
		? myfopen(rpath, O_RDONLY, bufsiz)
		: myfdopen(STDIN_FILENO, O_RDONLY, bufsiz);
	if (!rfp) {
		fprintf(
			stderr,
			"%s: couldn't open file for reading: %s -- '%s'\n",
			argv[0],
			strerror(errno),
			(rpath) ? rpath : "stdin"
		);
		return 255;
	}

	wfp = (wpath)
		? myfopen(wpath, O_WRONLY, bufsiz)
		: myfdopen(STDOUT_FILENO, O_WRONLY, bufsiz);
	if (!wfp) {
		fprintf(
			stderr,
			"%s: couldn't open file for writing: %s -- '%s'\n",
			argv[0],
			strerror(errno),
			(wpath) ? wpath : "stdout"
		);
		return 255;
	}

	errno = 0;
	int val;
	while ((val = myfgetc(rfp)) != -1) {
		if (errno) {
			fprintf(
				stderr,
				"%s: failed get character: %s -- '%s'\n",
				argv[0],
				strerror(errno),
				(rpath) ? rpath : "stdin"
			);
			return 255;
		}

		// check if we need to make a 4-space tabstop
		int lim;
		if (val == '\t') {
			lim = 4;
			val = ' ';
		} else {
			lim = 1;
		}

		for (int i = 0; i < lim; i++)
			if (myfputc(val, wfp) < 0) {
				fprintf(
					stderr,
					"%s: failed put character '%c': %s -- '%s'\n",
					argv[0],
					val,
					strerror(errno),
					(wpath) ? wpath : "stdout"
				);
				return 255;
			}
	}

	return 0;
}
