# tarman
# Copyright (C) 2024 Alessandro Salerno
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

CC=gcc
CFLAGS=-O2 -std=c2x -Iinclude/ -DEXT_TARMAN_BUILD="\"$(shell date +%y.%m.%d)\""
BIN=bin
EXEC=$(BIN)/tarman
TARMAN_OS=$(shell uname -s | tr A-Z a-z)

ifeq ($(OS),Windows_NT)
	TARMAN_OS=windows
	EXEC+=.exe
endif

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard ,$d, $2) $(filter $(subst *, %, $2),$d))
SRC=$(call rwildcard, src/common, *.c)
SRC+=$(call rwildcard, src/$(TARMAN_OS), *.c)
OBJ=$(patsubst src/%.c,obj/%.o, $(SRC))

.PHONEY: all
all: info obj $(OBJ) $(EXEC)
	@echo Finished!

.PHONEY: tarman
tarman: $(EXEC)

info:
	@echo Compiling for $(HTMC_OS)

$(EXEC): obj $(OBJ)
	$(CC) -flto -static -static-libgcc $(OBJ) -o $(EXEC)

obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -flto -c $^ -o $@

lib/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -fPIC -g -w -c $^ -o $@

obj:
	@mkdir -p obj/
	@mkdir -p $(BIN)
	@mkdir -p lib/

clean:
	rm -rf obj/; \
	rm -rf $(BIN); \
	rm -rf lib/
