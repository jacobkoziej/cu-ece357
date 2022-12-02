/*
 * bgrep.c -- binary grep
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

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char **argv)
{
	static size_t       context;
	static int          fd;
	static char        *pattern;
	static size_t       pattern_len;
	static struct stat  sb;

	int opt;
	while ((opt = getopt(argc, argv, "c:p:")) != -1) {
		switch (opt) {
			case 'c':
				context = strtoul(optarg, NULL, 0);
				break;

			case 'p':
				// we already have a pattern
				if (pattern) {
					perror("pattern already specified");
					return -1;
				}

				fd = open(optarg, O_RDONLY);
				if (fd < 0) {
					perror(optarg);
					return -1;
				}

				if (fstat(fd, &sb) < 0) {
					perror(optarg);
					return -1;
				}

				pattern_len = sb.st_size;
				pattern = mmap(
					NULL,
					pattern_len,
					PROT_READ,
					MAP_PRIVATE,
					fd,
					0
				);

				if (pattern == MAP_FAILED) {
					perror(optarg);
					return -1;
				}

				if (close(fd) < 0) {
					perror(optarg);
					return -1;
				}

				break;

			default:
				return -1;
		}
	}

	if (!pattern) {
		if (argc <= optind) {
			fprintf(stderr, "%s: no pattern specified\n", *argv);
			return -1;
		}

		pattern     = argv[optind++];
		pattern_len = strlen(pattern);
	}

	return 0;
}
