# Makefile for the microtoml library

VERSION = 0.1

# Use -g to compile the program for debugging.
DEBUG = -O2
#DEBUG = -g
CFLAGS = $(DEBUG) -Wall -Werror


all: test1 mtoml.3

test1: test1.o mtoml.o
	$(CC) $(CFLAGS) -o test1 test1.o mtoml.o

test1.o:
mtoml.o: mtoml.h

mtoml.3: mtoml.adoc
	asciidoctor -b manpage $<


.PHONY: clean
clean:
	rm -f *.o *.3 test[12]
	rm -f microtoml-*.tar.gz

version:
	@echo $(VERSION)


SOURCES = Makefile *.[ch] test[12].toml
DOCS = COPYING NEWS README.adoc mtoml.adoc
ALL = $(SOURCES) $(DOCS)

microtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:microtoml-$(VERSION)/:' --show-transformed-names -cvzf microtoml-$(VERSION).tar.gz $(ALL)

dist: microtoml-$(VERSION).tar.gz
