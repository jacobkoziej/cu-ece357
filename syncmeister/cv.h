/*
 * cv.h -- primitive condition variables
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

#ifndef CV_H
#define CV_H


#define CV_MAXPROC 64


#include <stddef.h>
#include <sys/types.h>

#include "spinlock.h"


struct cv {
	size_t head;
	size_t tail;
	size_t use;
	pid_t pid[CV_MAXPROC];
};


void cv_broadcast(struct cv *cv);
void cv_init(struct cv *cv);
int  cv_signal(struct cv *cv);
int  cv_wait(struct cv *cv, struct spinlock *mutex);


#endif /* CV_H */
