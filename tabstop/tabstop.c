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
	if (rfp) myfclose(rfp);
	if (wfp) myfclose(wfp);
}

int main(int argc, char **argv)
{
	atexit(cleanup);

	int         opt;
	const char *opath  = NULL;
	size_t      bufsiz = TABSTOP_BUFSIZ;

	while ((opt = getopt(argc, argv, "o:")) != -1) {
		switch (opt) {
			case 'o':
				opath = optarg;
				break;

			default:
				return 255;
		}
	}

	wfp = (opath)
		? myfopen(opath, O_WRONLY, bufsiz)
		: myfdopen(STDOUT_FILENO, O_WRONLY, bufsiz);
	if (!wfp) {
		fprintf(
			stderr,
			"%s: couldn't open file for writing: %s -- '%s'\n",
			argv[0],
			strerror(errno),
			(opath) ? opath : "stdout"
		);
		return 255;
	}

	return 0;
}
