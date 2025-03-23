#ifndef TOML_LEX_H_
#define TOML_LEX_H_

#include <stdbool.h>

/* The types of the lex items. */
enum {
  lex_eof = -1,
  lex_ERROR,          /* error ocurred; value is text of error */
  lex_LEFT_BRACKETS,  /* left double brackets "[[" */
  lex_RIGHT_BRACKETS, /* right double branckets "]]" */
  lex_BARE_KEY,       /* alphanumeric key, including '-' and '_' */
  lex_NEWLINE,        /* \r, \n, or \r\n */
  lex_STRING,         /* quoted string */
  lex_INTEGER,        /* an integer number, including, hex, octal, and binary */
  lex_FLOAT,          /* a float number, including inf and nan */
  lex_BOOL,           /* boolean constant: true, false */
  lex_TIME            /* RFC3339 formatted date-time */
};

#define ITEMSIZ 1024

/* The representation of a token returned from the scanner. */
struct item {
  int type;          /* the type of this item */
  char val[ITEMSIZ]; /* the value of this item */
};

/* Holds the state of the lexer */
struct lexer {
  /* the string being scanned */
  const char *input;
  /* pointer to the current character in the input */
  const char *ptr;
  /* we have hit the end of input and returned eof */
  bool atEOF;
  /* the item to return to parser */
  struct item item;
  /* current position in the input */
  int pos;
  /* number of newlines seen  */
  int lineno;
};

/* lex_init initializes a new scanner for the input string. */
void lex_init(struct lexer *lex, const char *input);

/* lex_scan_next scans the next item from the input, and returns its type. */
int lex_scan_next(struct lexer *lex);

#endif /* TOML_LEX_H_ */