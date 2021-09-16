CC = cc

#CFLAGS  = -g3 -ggdb
CFLAGS  = -O0

CFLAGS += -pipe -fdiagnostics-color -Wno-unknown-warning-option -Wpedantic
CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-format-truncation
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wsign-compare -Wswitch-enum
CFLAGS += -I/usr/local/include
LDADD = -L/usr/local/lib -lreadline -ljson-c

BIN  = isscrolls
OBJS = isscrolls.o rolls.o readline.o character.o oracle.o journey.o fight.o

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

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDADD)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(BIN) $(OBJS)
