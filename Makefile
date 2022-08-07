# Makefile for the microtoml library

VERSION = 0.1

# Use -g to compile the program for debugging.
DEBUG = -O2
#DEBUG = -g
CFLAGS = $(DEBUG) -Wall -Werror

OFILES = microtoml.o

all: mtoml.1


mtoml.1: mtoml.adoc
	asciidoctor -b manpage $<


.PHONY: clean
clean:
	rm -f *.o microtoml-*.tar.gz mtoml.1

version:
	@echo $(VERSION)


SOURCES = Makefile 
DOCS = COPYING NEWS README.adoc mtoml.adoc
ALL = $(SOURCES) $(DOCS)

microtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:microtoml-$(VERSION)/:' --show-transformed-names -cvzf microtoml-$(VERSION).tar.gz $(ALL)

dist: microtoml-$(VERSION).tar.gz
