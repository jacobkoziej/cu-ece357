/*
 * jkio.c -- Jacob Koziej's stdio library
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

#include "jkio.h"
#include "jkio_private.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>


struct MYSTREAM *myfdopen(int filedesc, int mode, int bufsiz)
{
	if (filedesc < 0 || (mode != O_RDONLY && mode != O_WRONLY) || bufsiz < 0) {
		errno = EINVAL;
		return NULL;
	}

	struct MYSTREAM *s = malloc(sizeof(struct MYSTREAM));
	if (!s) return NULL;

	if (bufsiz) {
		s->buf = malloc(bufsiz);
		if (!s->buf) goto error;
		s->pos    = s->buf;
		s->bufsiz = bufsiz;
	}

	s->fd    = filedesc;
	s->flags = (mode == O_RDONLY) ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC;

	return s;

error:
	free(s);

	return NULL;
}

int myfgetc(struct MYSTREAM *stream)
{
	ssize_t ret;

	// unbuffered
	if (!stream->bufsiz) {
		unsigned char val;

		ret = read(stream->fd, &val, 1);

		if (!ret) errno = 0;
		return (ret <= 0) ? -1 : val;
	}

	if (!stream->bufuse) {
		ret = read(stream->fd, stream->buf, stream->bufsiz);

		if (!ret) errno = 0;
		if (ret <= 0) return -1;

		stream->pos    = stream->buf;
		stream->bufuse = ret;
	}

	--stream->bufuse;
	return *stream->pos++;
}

struct MYSTREAM *myfopen(const char *pathname, int mode, int bufsiz)
{
	if ((mode != O_RDONLY && mode != O_WRONLY) || bufsiz < 0) {
		errno = EINVAL;
		return NULL;
	}

	int flags;

	int fd = (mode == O_RDONLY)
		? open(pathname, flags = O_RDONLY)
		: open(pathname, flags = O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) return NULL;

	struct MYSTREAM *s = malloc(sizeof(struct MYSTREAM));
	if (!s) return NULL;

	if (bufsiz) {
		s->buf = malloc(bufsiz);
		if (!s->buf) goto error;
		s->pos    = s->buf;
		s->bufsiz = bufsiz;
	}

	s->fd    = fd;
	s->flags = flags;

	return s;

error:
	free(s);

	return NULL;
}
