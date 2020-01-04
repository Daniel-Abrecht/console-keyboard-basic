# Copyright (c) 2019 Daniel Abrecht
# SPDX-License-Identifier: GPL-3.0-or-later

PREFIX = /usr

SOURCES = src/console-keyboard-basic.c

ifdef DEBUG
CC_OPTS += -Og -g
endif

ifndef LENIENT
CC_OPTS += -Werror
endif

CC_OPTS += -ffunction-sections -fdata-sections
CC_OPTS += -fvisibility=hidden -I include -finput-charset=UTF-8
CC_OPTS += -std=c99 -Wall -Wextra -pedantic
CC_OPTS += $(shell pkg-config --cflags ncursesw)
CC_OPTS += -Wno-missing-field-initializers

LD_OPTS  += -Wl,-gc-sections
LIBS += $(shell pkg-config --libs ncursesw)
LIBS += -lconsolekeyboard

OBJECTS += $(SOURCES:%=build/%.o)


all: bin/console-keyboard-basic

%/.dir:
	mkdir -p "$(dir $@)"
	touch "$@"

build/%.c.o: %.c
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CPPFLAGS) $(CFLAGS) "$<"

bin/console-keyboard-basic: $(OBJECTS) | bin/.dir
	$(CC) -o "$@" $(LD_OPTS) $^ $(LIBS) $(LDFLAGS)

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
