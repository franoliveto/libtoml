# Makefile for the libtoml library

VERSION = 0.0

CFLAGS = -Wall -Werror -Wextra -Wno-missing-field-initializers
# Add DEBUG_ENABLE for the tracing code
# CFLAGS += -DDEBUG_ENABLE -g


all: example toml_test # mtoml.3

toml_test: toml_test.o toml.o
	$(CC) $(CFLAGS) -o $@ toml_test.o toml.o

example: example.o toml.o
	$(CC) $(CFLAGS) -o $@ example.o toml.o

toml.o: toml.c toml.h
toml_test.o: toml_test.c
example.o: example.c

# mtoml.3: mtoml.adoc
#	asciidoctor -b manpage $<

test: toml_test
	./toml_test

.PHONY: clean version
clean:
	rm -f *.o *.3 toml_test example
	rm -f libtoml-*.tar.gz

version:
	@echo $(VERSION)


SOURCES = Makefile *.[ch] tests/*.toml BUILD.bazel WORKSPACE example.toml toml.png
DOCS = COPYING NEWS README.md mtoml.adoc
ALL = $(SOURCES) $(DOCS)

libtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:libtoml-$(VERSION)/:' --show-transformed-names -cvzf libtoml-$(VERSION).tar.gz $(ALL)

dist: libtoml-$(VERSION).tar.gz
