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

#include <ctype.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


static size_t   context;
static int      fd;
static char    *file;
static jmp_buf  jmp;
static bool     sigbus_occurred;


void sigbus_handler(int sig)
{
	(void) sig;

	sigbus_occurred = true;
	siglongjmp(jmp, fd);
}


void print_pos(char *path, char *head, char *tail, char *pos)
{
	(void) tail;

	printf("%s:%lu\n", path, pos - head);
}

void print_context(char *path, char *head, char *tail, char *pos)
{
	printf("%s:%08X:", path, (unsigned) (pos - head));

	uintptr_t diff;

	// determine if there's enough context to print
	char *prefix = ((diff = pos - head) >= context)
		? pos - context
		: head;
	char *postfix = ((diff = tail - pos) >= context)
		? pos + context + 1
		: tail;

	for (char *i = prefix; i < pos; i++)
		if (isprint(*i)) printf("  %c", *i);
		else printf(" %02X", *i & 0xff);

	if (isprint(*pos)) printf("  %c", *pos);
	else printf(" %02X", *pos & 0xff);

	for (char *i = pos + 1; i < postfix; i++)
		if (isprint(*i)) printf("  %c", *i);
		else printf(" %02X", *i & 0xff);

	putchar('\n');
}


int main(int argc, char **argv)
{
	static int           fd;
	static char         *path;
	static char         *pattern;
	static size_t        pattern_len;
	static void        (*printer)(char*, char*, char*, char*);
	static struct stat   sb;

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

	printer = (context) ? print_context : print_pos;

	int ret_val = 1;

	struct sigaction sig;
	sig.sa_handler = sigbus_handler;
	if (sigaction(SIGBUS, &sig, NULL) < 0) {
		perror("failed to setup signal handler for SIGBUS");
		return -1;
	}

	// no files specified, read from STDIN
	if (optind >= argc) {
		fd   = STDIN_FILENO;
		path = "/dev/stdin";
		goto skip_open;
	}

	for (;;) {
		// nothing left to parse
		if (optind >= argc) break;

		path = argv[optind];

		fd = open(path, O_RDONLY);
		if (fd < 0) {
			perror(path);
			return -1;
		}

skip_open:
		// process next file on SIGBUS
		if (sigsetjmp(jmp, 1)) goto next_file;

		if (fstat(fd, &sb) < 0) {
			perror(path);
			return -1;
		}

		file = mmap(
			NULL,
			sb.st_size,
			PROT_READ,
			MAP_PRIVATE,
			fd,
			0
		);

		if (file == MAP_FAILED) {
			perror(path);
			return -1;
		}

		if (close(fd) < 0) {
			perror(path);
			return -1;
		}

		for (size_t i = 0; i < sb.st_size - pattern_len; i++)
			if (!memcmp(file + i, pattern, pattern_len)) {
				if (ret_val) ret_val = 0;
				printer(
					path,
					file,
					file + sb.st_size - 1,
					file + i
				);
			}

next_file:
		if (munmap(file, sb.st_size) < 0) {
			perror(path);
			return -1;
		}

		++optind;
	}

	return (sigbus_occurred) ? -1 : ret_val;
}
