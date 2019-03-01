# Copyright (c) 2018 Daniel Abrecht
# SPDX-License-Identifier: AGPL-3.0-or-later

CC = gcc
AR = ar

PREFIX = /usr

SOURCES = src/console-keyboard-basic.c

OPTIONS  = -ffunction-sections -fdata-sections -g -Og

CC_OPTS  = -fvisibility=hidden -I include
CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Werror
CC_OPTS += -Wno-missing-field-initializers

LD_OPTS  = -Wl,-gc-sections
LD_OPTS  = -lncursesw

CC_OPTS += $(OPTIONS)
LD_OPTS += $(OPTIONS)

OBJECTS += $(SOURCES:%=build/%.o)


all: bin/console-keyboard-basic

%/.dir:
	mkdir -p "$(dir $@)"
	touch "$@"

build/%.c.o: %.c
	mkdir -p "$(dir $@)"
	$(CC) $(CC_OPTS) -c -o $@ $^

bin/console-keyboard-basic: $(OBJECTS) | bin/.dir
	$(CC) $(LD_OPTS) $^ -o $@

install:
	cp bin/console-keyboard-basic $(PREFIX)/bin/console-keyboard-basic

uninstall:
	rm -f $(PREFIX)/bin/console-keyboard-basic

clean:
	rm -rf bin/ build/
