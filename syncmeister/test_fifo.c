/*
 * test_fifo.c -- test primitive FIFO
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fifo.h"


int main(int argc, char **argv)
{
	unsigned long children = 0;
	unsigned long writes   = 0;

	int opt;
	while ((opt = getopt(argc, argv, "c:w:")) != -1) {
		switch (opt) {
			case 'c':
				children = strtoul(optarg, NULL, 0);
				break;

			case 'w':
				writes = strtoul(optarg, NULL, 0);
				break;

			default:
				return EXIT_FAILURE;
		}
	}

	struct fifo *fifo = mmap(
		NULL,
		sizeof(*fifo),
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_SHARED,
		-1,
		0);

	if (fifo == MAP_FAILED) {
		perror("failed to `mmap()` spinlock");
		return EXIT_FAILURE;
	}

	fifo_init(fifo);

	pid_t pid[children];
	memset(pid, 0, sizeof(pid));

	unsigned long i = 0;

	while (i < children) {
		pid[i] = fork();

		if (pid[i] < 0) {
			perror("failed to `fork()` children");
			return EXIT_FAILURE;
		}

		if (!pid[i]) break;

		++i;
	}

	if (!pid[i]) {
		pid[i] = getpid();

		unsigned long counter = 0;
		while (writes--) {
			unsigned long val =
				((unsigned long) pid[i] << 32) | counter++;

			fifo_wr(fifo, val);
		}

		return EXIT_SUCCESS;
	}

	unsigned long expected[children];
	memset(expected, 0, sizeof(expected));

	unsigned long read_cnt = children * writes;
	for (i = 0; i < read_cnt; i++) {
		unsigned long raw = fifo_rd(fifo);

		unsigned long val = raw & 0xffff;
		pid_t cpid = raw >> 32;

		printf("pid: %u, val: %lu\n", cpid, val);

		for (unsigned long j = 0; j < children; j++)
			if (pid[j] == cpid) {
				if (val != expected[j]++) {
					printf("%u FAILED\n", cpid);
					return EXIT_FAILURE;
				}

				break;
			}
	}

	for (i = 0; i < children; i++)
		wait(NULL);

	puts("TEST PASSED");

	return EXIT_SUCCESS;
}
