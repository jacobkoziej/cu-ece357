/*
 * cv.c -- primitive condition variables
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

#include "cv.h"

#include <signal.h>
#include <stddef.h>
#include <string.h>


static void cv_sigusr1_handler(int sig)
{
	(void) sig;
}


void cv_init(struct cv *cv)
{
	memset(cv, 0, sizeof(*cv));

	// block SIGUSR1 when we're not expecting it
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, NULL);

	struct sigaction act = {
		.sa_handler = cv_sigusr1_handler,
	};
	sigaction(SIGUSR1, &act, NULL);
}
