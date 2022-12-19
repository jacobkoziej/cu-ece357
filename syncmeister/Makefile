# Makefile
# Copyright (C) 2022  Jacob Koziej <jacobkoziej@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

SRC := $(wildcard *.c)
DEP := $(SRC:.c=.d)

CFLAGS += -D_DEFAULT_SOURCE -Wall -Wextra -Wpedantic -g -std=c17


.PHONY: all
all: test_cv test_fifo test_spinlock


-include $(DEP)


.PHONY: clean
clean:
	@rm -rvf $(BIN) $(DEP) *.o test_cv test_fifo test_spinlock


test_cv: cv.o spinlock.o tas.o test_cv.o
	$(CC) $(CFLAGS) -o $@ $^


test_fifo: cv.o fifo.o spinlock.o tas.o test_fifo.o
	$(CC) $(CFLAGS) -o $@ $^


test_spinlock: spinlock.o tas.o test_spinlock.o
	$(CC) $(CFLAGS) -o $@ $^


tas.o: tas.h tas.s


%.o: %.c
	$(CC) $(CFLAGS) -MM $< -MF $*.d
	$(CC) $(CFLAGS) -c $<
