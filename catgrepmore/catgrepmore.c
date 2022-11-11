/*
 * catgrepmore.c -- Jeff Hakner's "Swiss Army Knife"
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
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static size_t bytes;
static size_t files;


static void sigusr1_handler(int sig)
{
	(void) sig;

	fprintf(stderr, "files: %lu, bytes: %lu\n", files, bytes);
}


int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "%s: not enough arguments\n", argv[0]);
		fprintf(stderr, "usage: %s pattern [FILE]...\n", argv[0]);

		return EXIT_FAILURE;
	}

	struct sigaction sig;
	memset(&sig, 0, sizeof(sig));

	sig.sa_handler = sigusr1_handler;
	sig.sa_flags   = SA_RESTART;

	sigaction(SIGUSR1, &sig, NULL);

	for (int i = 2; i < argc; i++) {
		int fd;

open_eintr:
		fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			if (errno == EINTR) goto open_eintr;

			perror(argv[i]);
			return EXIT_FAILURE;
		}

close_eintr:
		if (close(fd) < 0) {
			if (errno == EINTR) goto close_eintr;

			perror("close()");
			return EXIT_FAILURE;
		}

		++files;
	}

	return EXIT_SUCCESS;
}
