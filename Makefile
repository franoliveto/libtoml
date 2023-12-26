# Makefile for the microtoml library

VERSION = 0.0

CFLAGS = -Wall -Werror -Wextra -Wno-missing-field-initializers
# Add DEBUG_ENABLE for the tracing code
# CFLAGS += -DDEBUG_ENABLE -g


all: microtoml_test mtoml.3

microtoml_test: microtoml_test.o toml.o
	$(CC) $(CFLAGS) -o $@ microtoml_test.o toml.o

example: example.o toml.o
	$(CC) $(CFLAGS) -o $@ example.o toml.o

toml.o: toml.c toml.h
microtoml_test.o: microtoml_test.c
example.o: example.c

mtoml.3: mtoml.adoc
	asciidoctor -b manpage $<

test: microtoml_test
	./microtoml_test

.PHONY: clean version
clean:
	rm -f *.o *.3 microtoml_test example
	rm -f microtoml-*.tar.gz

version:
	@echo $(VERSION)


SOURCES = Makefile *.[ch] tests/*.toml BUILD.bazel WORKSPACE example.toml toml.png
DOCS = COPYING NEWS README.adoc mtoml.adoc
ALL = $(SOURCES) $(DOCS)

microtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:microtoml-$(VERSION)/:' --show-transformed-names -cvzf microtoml-$(VERSION).tar.gz $(ALL)

dist: microtoml-$(VERSION).tar.gz
