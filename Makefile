SRC = shell.c
BIN = msh

CC = gcc
RM = rm -f

CFLAGS = -ggdb
override CFLAGS+= -Wall -pipe -MMD -std=gnu99

.PHONY: all
all: build

.PHONY: build
build: $(BIN)

$(BIN): $(BIN).out
	strip -o $@ $<

.PHONY: rebuild
rebuild: | clean
	$(MAKE) -C . build

.PHONY: clean
clean:
	$(RM) $(BIN) $(wildcard rsock-g*.tar) $(BIN).out $(wildcard *.d)

$(BIN).out: $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: archive
VER:=$(shell git rev-parse --verify --short HEAD 2>/dev/null)
archive:
	git archive --prefix='rsock-g$(VER)/' HEAD > rsock-g$(VER).tar

-include $(wildcard *.d)
