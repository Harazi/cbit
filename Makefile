CFLAGS ?= -O2 -pipe -pedantic
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=
LIBS ?= `pkgconf --libs libcurl`
objects = config.o auth.o callbacks.o app.o log.o transfer.o sync.o torrents.o cJSON.o session.c settings.o

ifeq (${DEBUG}, 1)
	CFLAGS += -ggdb
else
	CFLAGS += -s
endif

qbit-cli: main.c $(objects)
	$(CC) $(CFLAGS) -o $@ $< $(objects) $(LIBS)

config.o: config.c
	$(CC) $(CFLAGS) -c $<

auth.o: auth.c
	$(CC) $(CFLAGS) -c $<

callbacks.o: callbacks.c
	$(CC) $(CFLAGS) -c $<

app.o: app.c
	$(CC) $(CFLAGS) -c $<

log.o: log.c
	$(CC) $(CFLAGS) -c $<

transfer.o: transfer.c
	$(CC) $(CFLAGS) -c $<

sync.o: sync.c
	$(CC) $(CFLAGS) -c $<

torrents.o: torrents.c
	$(CC) $(CFLAGS) -c $<

cJSON.o: cJSON/cJSON.c
	$(CC) $(CFLAGS) -c $<

session.o: session.c
	$(CC) $(CFLAGS) -c $<

settings.o: settings.c
	$(CC) $(CFLAGS) -c $<

install: qbit-cli
	install -D -m 755 qbit-cli "$(DESTDIR)$(BINDIR)/qbit-cli"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/qbit-cli"

clean:
	rm -f qbit-cli $(objects)
