/*
 * test_spinlock.c -- test primitive spinlock
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "spinlock.h"


int main(int argc, char **argv)
{
	unsigned long children   = 0;
	unsigned long increments = 0;

	int opt;
	while ((opt = getopt(argc, argv, "c:i:")) != -1) {
		switch (opt) {
			case 'c':
				children = strtoul(optarg, NULL, 0);
				break;

			case 'i':
				increments = strtoul(optarg, NULL, 0);
				break;

			default:
				return EXIT_FAILURE;
		}
	}

	struct spinlock *lock = mmap(
		NULL,
		sizeof(*lock),
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_SHARED,
		-1,
		0);

	if (lock == MAP_FAILED) {
		perror("failed to `mmap()` spinlock");
		return EXIT_FAILURE;
	}

	unsigned long *counter = mmap(
		NULL,
		sizeof(*counter),
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_SHARED,
		-1,
		0);

	if (counter == MAP_FAILED) {
		perror("failed to `mmap()` counter");
		return EXIT_FAILURE;
	}

	pid_t pid;
	for (unsigned long i = 0; i < children; i++) {
		pid = fork();

		if (pid < 0) {
			perror("failed to `fork()` children");
			return EXIT_FAILURE;
		}

		if (!pid) break;
	}

	if (!pid) {
		while (increments--) {
			spinlock_lock(lock);
			++*counter;
			spinlock_unlock(lock);
		}

		return EXIT_SUCCESS;
	}

	for (unsigned long i = 0; i < children; i++) wait(NULL);

	printf("expected: %lu\n", increments * children);
	printf("got:      %lu\n", *counter);

	return EXIT_SUCCESS;
}
