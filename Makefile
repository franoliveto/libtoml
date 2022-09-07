# Makefile for the microtoml library

VERSION = 0.1

# Use -g to compile the program for debugging.
DEBUG = -O2
#DEBUG = -g
CFLAGS = $(DEBUG) -Wall -Werror


all: test_microtoml mtoml.3

test_microtoml: test_microtoml.o mtoml.o
	$(CC) $(CFLAGS) -o test_microtoml test_microtoml.o mtoml.o

mtoml.o: mtoml.h
test_microtoml.o:

mtoml.3: mtoml.adoc
	asciidoctor -b manpage $<

test: test_microtoml
	./test_microtoml

.PHONY: clean
clean:
	rm -f *.o *.3 test_microtoml
	rm -f microtoml-*.tar.gz

version:
	@echo $(VERSION)


SOURCES = Makefile *.[ch] *.toml 
DOCS = COPYING NEWS README.adoc mtoml.adoc
ALL = $(SOURCES) $(DOCS)

microtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:microtoml-$(VERSION)/:' --show-transformed-names -cvzf microtoml-$(VERSION).tar.gz $(ALL)

dist: microtoml-$(VERSION).tar.gz
