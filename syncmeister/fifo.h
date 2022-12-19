/*
 * fifo.h -- primitive FIFO
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

#ifndef FIFO_H
#define FIFO_H


#include <stddef.h>

#include "cv.h"
#include "spinlock.h"


#define FIFO_BUFSIZ (1 << 10)


struct fifo {
	size_t head;
	size_t tail;
	size_t use;
	struct cv full;
	struct cv empty;
	struct spinlock mutex;
	unsigned long fifo[FIFO_BUFSIZ];
};


void          fifo_init(struct fifo *fifo);
unsigned long fifo_rd(struct fifo *fifo);
void          fifo_wr(struct fifo *fifo, unsigned long val);


#endif /* FIFO_H */
