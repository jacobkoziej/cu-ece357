/*
 * test_cv.c -- test primitive condition variables
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
#include <sys/mman.h>
#include <unistd.h>

#include "cv.h"
#include "spinlock.h"


int main(void)
{
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

	struct cv *cv = mmap(
		NULL,
		sizeof(*cv),
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_SHARED,
		-1,
		0);

	if (cv == MAP_FAILED) {
		perror("failed to `mmap()` cv");
		return EXIT_FAILURE;
	}

	struct spinlock *mutex = mmap(
		NULL,
		sizeof(*mutex),
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_SHARED,
		-1,
		0);

	if (mutex == MAP_FAILED) {
		perror("failed to `mmap()` mutex");
		return EXIT_FAILURE;
	}

	cv_init(cv);

	switch (fork()) {
		case -1:
			perror ("failed to `fork()`");
			return EXIT_FAILURE;

		case 0:
			spinlock_lock(mutex);
			cv_wait(cv, mutex);
			spinlock_unlock(mutex);
			puts("recieved cv signal");
			break;

		default:
			puts("sleeping for 1s");
			sleep(1);
			puts("sending cv signal");
			cv_signal(cv);
	}

	return EXIT_SUCCESS;
}
