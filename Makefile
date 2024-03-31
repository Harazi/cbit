.POSIX:
.SUFFIXES: .c.o

CC		= cc
CFLAGS	= -O2 -pipe -Wall
PREFIX	= /usr/local
BINDIR	= $(PREFIX)/bin
LDLIBS 	= -lcurl
objects	= $(patsubst %.c,%.o,$(filter-out main.c,$(wildcard *.c)))
objects	+= cJSON.o

cbit: main.c $(objects)
	$(CC) $(CFLAGS) -o $@ $< $(objects) $(LDLIBS)

cJSON.o: cJSON/cJSON.c
	$(CC) $(CFLAGS) -c $<

install: cbit
	install -Dm755 $< "$(DESTDIR)$(BINDIR)/cbit"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/cbit"

clean:
	rm -f cbit $(objects)
