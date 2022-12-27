CC = cc

#CFLAGS  = -g3 -ggdb
CFLAGS  = -O2

CFLAGS += -pipe -fdiagnostics-color -Wno-unknown-warning-option -Wpedantic
CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-format-truncation
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wswitch-enum
CFLAGS += -Wuninitialized -Wformat-security -Wformat-overflow=2
CFLAGS += -Wextra -I/usr/local/include
CFLAGS += `pkg-config --cflags json-c`
LDADD   = `pkg-config --libs json-c` -lreadline

BIN   = isscrolls
OBJS  = isscrolls.o rolls.o readline.o character.o oracle.o journey.o fight.o
OBJS += delve.o vows.o

INSTALL ?= install -p

PREFIX ?= /usr/local
BIND ?= $(PREFIX)/bin
MAN ?= $(PREFIX)/man
SHARE ?= $(PREFIX)/share

all: $(BIN)

install: all
	$(INSTALL) -d -m 755 -o root $(MAN)/man1
	$(INSTALL) -d -m 755 -o root $(SHARE)/isscrolls
	cp contrib/* $(SHARE)/isscrolls/
	$(INSTALL) -m 644 -o root isscrolls.1 $(MAN)/man1
	$(INSTALL) -m 755 -o root $(BIN) $(BIND)

uninstall:
	rm -f $(MAN)/man1/isscrolls.1
	rm -rf $(SHARE)/isscrolls/
	rm -f $(BIND)/isscrolls

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDADD)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(BIN) $(OBJS)
