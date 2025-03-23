# Makefile for the libtoml library

VERSION = 0.0

CFLAGS = -Wall -Werror -Wextra -Wno-missing-field-initializers
# Add DEBUG_ENABLE for the tracing code
# CFLAGS += -DDEBUG_ENABLE -g


all: example

lex_test: lex_test.o lex.o
	$(CC) $(CFLAGS) -o $@ lex_test.o lex.o

toml_test: toml_test.o toml.o lex.o
	$(CC) $(CFLAGS) -o $@ toml_test.o toml.o lex.o

example: example.o toml.o lex.o
	$(CC) $(CFLAGS) -o $@ example.o toml.o lex.o

toml.o: toml.c toml.h
toml_test.o: toml_test.c
example.o: example.c
lex.o: lex.c lex.h
lex_test.o: lex_test.c

test: lex_test toml_test
	./lex_test
	./toml_test

.PHONY: clean version
clean:
	rm -f *.o lex_test toml_test example
	rm -f libtoml-*.tar.gz

version:
	@echo $(VERSION)


SOURCES = Makefile *.[ch] example.toml toml.png
DOCS = LICENSE NEWS README.md
ALL = $(SOURCES) $(DOCS)

libtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:libtoml-$(VERSION)/:' --show-transformed-names -cvzf libtoml-$(VERSION).tar.gz $(ALL)

dist: libtoml-$(VERSION).tar.gz
