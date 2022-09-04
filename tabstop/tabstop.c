/*
 * tabstop.c -- convert tabs to four spaces
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

#include <stdlib.h>


static struct MYSTREAM *rfp;
static struct MYSTREAM *wfp;


static void cleanup(void)
{
	if (rfp) myfclose(rfp);
	if (wfp) myfclose(wfp);
}

int main(void)
{
	atexit(cleanup);

	return 0;
}
