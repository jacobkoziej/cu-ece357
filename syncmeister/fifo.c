/*
 * fifo.c -- primitive FIFO
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

#include "fifo.h"

#include <string.h>

#include "cv.h"
#include "spinlock.h"


void fifo_init(struct fifo *fifo)
{
	memset(fifo, 0, sizeof(*fifo));
	cv_init(&fifo->cv);
}

unsigned long fifo_rd(struct fifo *fifo)
{
	spinlock_lock(&fifo->mutex);

	if (!fifo->use) {
		spinlock_unlock(&fifo->mutex);
		return 0;
	}

	--fifo->use;

	unsigned long val = fifo->fifo[fifo->head];

	fifo->head = (fifo->head + 1) % FIFO_BUFSIZ;

	cv_signal(&fifo->cv);

	spinlock_unlock(&fifo->mutex);

	return val;
}

void fifo_wr(struct fifo *fifo, unsigned long val)
{
	spinlock_lock(&fifo->mutex);

	if (fifo->use == FIFO_BUFSIZ)
		cv_wait(&fifo->cv, &fifo->mutex);

	++fifo->use;

	fifo->fifo[fifo->tail] = val;

	fifo->tail = (fifo->tail + 1) % FIFO_BUFSIZ;

	spinlock_unlock(&fifo->mutex);
}
