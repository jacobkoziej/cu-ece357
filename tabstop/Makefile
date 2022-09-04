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

BIN    := tabstop
SRC    := $(wildcard *.c)
OBJ    := $(patsubst %.c,%.o,$(SRC))
DEPEND := $(patsubst %.c,%.d,$(SRC))

CFLAGS += -Wall -Wextra -Wpedantic -g


all: $(BIN)


-include $(DEPEND)


.PHONY: clean
clean:
	@rm -rvf $(BIN) $(DEPEND) *.o


$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^


%.o: %.c
	$(CC) $(CFLAGS) -c $<
	$(CC) $(CFLAGS) -MM $< -MF $*.d
