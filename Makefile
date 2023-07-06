# Makefile for the microtoml library

VERSION = 0.1

# CFLAGS = -Wall -Werror -Wextra -Wno-missing-field-initializers
CFLAGS = -Wall -Wno-missing-field-initializers
# Add DEBUG_ENABLE for the tracing code
CFLAGS += -DDEBUG_ENABLE -g


all: test_microtoml mtoml.3

test_microtoml: test_microtoml.o toml.o
	$(CC) $(CFLAGS) -o test_microtoml test_microtoml.o toml.o

toml.o: toml.c toml.h
test_microtoml.o: test_microtoml.c

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


SOURCES = Makefile *.[ch] tests/*.toml 
DOCS = COPYING NEWS README.adoc mtoml.adoc
ALL = $(SOURCES) $(DOCS)

microtoml-$(VERSION).tar.gz: $(ALL)
	tar --transform='s:^:microtoml-$(VERSION)/:' --show-transformed-names -cvzf microtoml-$(VERSION).tar.gz $(ALL)

dist: microtoml-$(VERSION).tar.gz
