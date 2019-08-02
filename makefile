# Copyright (c) 2019 Daniel Abrecht
# SPDX-License-Identifier: GPL-3.0-or-later

CC = gcc
AR = ar

PREFIX = /usr

SOURCES = src/console-keyboard-basic.c

OPTIONS  = -ffunction-sections -fdata-sections -g -Og

CC_OPTS  = -fvisibility=hidden -I include -finput-charset=UTF-8
CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Werror
CC_OPTS += $(shell ncursesw5-config --cflags)
CC_OPTS += -Wno-missing-field-initializers

LD_OPTS  = -Wl,-gc-sections
LIBS += $(shell ncursesw5-config --libs)
LIBS += -lconsolekeyboard

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
	$(CC) $(LD_OPTS) $^ $(LIBS) -o $@

install-all: install install-link
	@true

install:
	mkdir -p "$(DESTDIR)$(PREFIX)/bin/"
	cp bin/console-keyboard-basic "$(DESTDIR)$(PREFIX)/bin/console-keyboard-basic"

install-link:
	ln -s "$(DESTDIR)$(PREFIX)/bin/console-keyboard-basic" "$(DESTDIR)$(PREFIX)/bin/console-keyboard" || true

uninstall:
	[ -f "$(DESTDIR)$(PREFIX)/bin/console-keyboard" ] && [ "$(shell readlink "$(DESTDIR)$(PREFIX)/bin/console-keyboard")" = "$(DESTDIR)$(PREFIX)/bin/console-keyboard-basic" ] && rm -f "$(DESTDIR)$(PREFIX)/bin/console-keyboard" || true
	rm -f "$(DESTDIR)$(PREFIX)/bin/console-keyboard-basic"

clean:
	rm -rf bin/ build/
