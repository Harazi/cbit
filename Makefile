.POSIX:
.SUFFIXES: .c.o

CC		= cc
CFLAGS	= -O2 -pipe -Wall
PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
LDLIBS 	= -lcurl
objects	= $(patsubst %.c,%.o,$(filter-out main.c,$(wildcard *.c)))
objects	+= cJSON.o

qbit-cli: main.c $(objects)
	$(CC) $(CFLAGS) -o $@ $< $(objects) $(LDLIBS)

cJSON.o: cJSON/cJSON.c
	$(CC) $(CFLAGS) -c $<

install: qbit-cli
	install -D -m 755 qbit-cli "$(DESTDIR)$(BINDIR)/qbit-cli"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/qbit-cli"

clean:
	rm -f qbit-cli $(objects)
