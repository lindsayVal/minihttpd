CC ?= cc 
CPPFLAGS += -Iinclude
CFLAGS ?= -O2
CFLAGS += -std=c17 -Wall -Wextra -Wpedantic

minihttpd: src/main.c src/server.c src/http.c src/files.c src/mime.c $(wildcard include/*.h)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@
.PHONY: all clean
all: minihttpd

clean:
	rm -f minihttpd
