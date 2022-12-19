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
#include <unistd.h>

#include "spinlock.h"


static void cv_sigusr1_handler(int sig)
{
	(void) sig;
}


void cv_broadcast(struct cv *cv)
{
	// we can miss an unlikely kill() failure here
	while (cv_signal(cv) < 0);
}

void cv_init(struct cv *cv)
{
	memset(cv, 0, sizeof(*cv));

	struct sigaction act = {
		.sa_handler = cv_sigusr1_handler,
	};
	sigaction(SIGUSR1, &act, NULL);
}

int cv_signal(struct cv *cv)
{
	spinlock_lock(&cv->lock);

	if (!cv->use) {
		spinlock_unlock(&cv->lock);
		return -1;
	}

	if (kill(cv->pid[cv->head], SIGUSR1) < 0) {
		spinlock_unlock(&cv->lock);
		return -1;
	}

	cv->head = (cv->head + 1) % CV_MAXPROC;

	spinlock_unlock(&cv->lock);
	return 0;
}

int cv_wait(struct cv *cv, struct spinlock *mutex)
{
	spinlock_lock(&cv->lock);

	if (cv->use >= CV_MAXPROC) {
		spinlock_unlock(&cv->lock);
		return -1;
	}

	++cv->use;

	cv->pid[cv->tail++] = getpid();
	cv->tail %= CV_MAXPROC;

	spinlock_unlock(&cv->lock);

	sigset_t mask;
	sigset_t oldmask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);

	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	spinlock_unlock(mutex);
	sigsuspend(&oldmask);
	spinlock_lock(mutex);

	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	return 0;
}
