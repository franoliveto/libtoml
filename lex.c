#include "lex.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* lex_next returns the next character in the input. */
static inline int lex_next(struct lexer *lex)
{
  if (*lex->ptr == '\0') {
    lex->atEOF = true;
    return lex_eof;
  }
  if (*lex->ptr == '\n')
    lex->lineno++;
  return *lex->ptr++;
}

/* lex_backup steps back one character. */
static inline void lex_backup(struct lexer *lex)
{
  if (lex->atEOF) {
    lex->atEOF = false;
    return;
  }
  if (lex->ptr > lex->input) {
    if (*lex->ptr == '\n')
      lex->lineno--;
    lex->ptr--;
  }
}

/* lex_peek returns but does not consume the next character in the input. */
static inline int lex_peek(struct lexer *lex)
{
  int c;

  c = lex_next(lex);
  lex_backup(lex);
  return c;
}

/* lex_accept consumes and returns the next character c if it's from the valid
   set. */
static bool lex_accept(struct lexer *lex, const char *valid, int *c)
{
  *c = lex_next(lex);
  if (strchr(valid, *c) != NULL)
    return true;
  lex_backup(lex);
  return false;
}

/* lex_endofline checks for and consume \r, \n, \r\n, or lex_eof */
static bool lex_endofline(struct lexer *lex, int c)
{
  bool eol;

  eol = (c == '\r' || c == '\n');
  if (c == '\r') {
    c = lex_next(lex);
    if (c != '\n' && c != lex_eof)
      lex_backup(lex); /* read to far, put it back */
  }
  return eol;
}

/* lex_errorf returns an error item and terminates the scan by passing
   back an lex_ERROR item type. */
static int lex_errorf(struct lexer *lex, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  vsnprintf(lex->item.val, sizeof(lex->item.val), format, ap);
  va_end(ap);
  return lex->item.type = lex_ERROR;
}

/* lex_scan_decimal_number scans a decimal integer or float. */
static int lex_scan_decimal_number(struct lexer *lex)
{
  int c;
  char *p = lex->item.val;

  if (lex_accept(lex, "+-", &c))
    *p++ = c;
  for (; isdigit(c = lex_next(lex)); p++)
    *p = c;
  if (c == '.' || c == 'e' || c == 'E') {
    *p++ = c;
    while (lex_accept(lex, "0123456789eE+-._", &c))
      if (c != '_')
        *p++ = c;
    *p = '\0';
    return lex->item.type = lex_FLOAT;
  }
  lex_backup(lex);
  while (isdigit(c = lex_next(lex)) || c == '_')
    if (c != '_')
      *p++ = c;
  *p = '\0';
  lex_backup(lex);
  return lex->item.type = lex_INTEGER;
}

/* lex_scan_number_or_date scans a number or a date. */
static int lex_scan_number_or_date(struct lexer *lex)
{
  int c;
  char *p = lex->item.val;

  if (lex_peek(lex) == '0') {
    *p++ = lex_next(lex);
    if ((c = lex_next(lex)) == 'x') { /* hexadecimal */
      *p++ = c;
      while (isxdigit(c = lex_next(lex)) || c == '_')
        if (c != '_')
          *p++ = c;
      *p = '\0';
      return lex->item.type = lex_INTEGER;
    }
    if (c == 'o') { /* octal */
      *p++ = c;
      while (((c = lex_next(lex)) >= '0' && c <= '7') || c == '_')
        if (c != '_')
          *p++ = c;
      *p = '\0';
      return lex->item.type = lex_INTEGER;
    }
    if (c == 'b') { /* binary */
      *p++ = c;
      while ((c = lex_next(lex)) == '0' || c == '1' || c == '_')
        if (c != '_')
          *p++ = c;
      *p = '\0';
      return lex->item.type = lex_INTEGER;
    }
    lex_backup(lex);
  }

  for (; isdigit(c = lex_next(lex)); p++)
    *p = c;
  if (c == '.' || c == 'e' || c == 'E') {
    *p++ = c;
    while (lex_accept(lex, "0123456789eE+-._", &c))
      if (c != '_')
        *p++ = c;
    *p = '\0';
    return lex->item.type = lex_FLOAT;
  }
  if (c == '_') {
    while (isdigit(c = lex_next(lex)) || c == '_')
      if (c != '_')
        *p++ = c;
    *p = '\0';
    lex_backup(lex);
    return lex->item.type = lex_INTEGER;
  }
  if (c == '-' || c == ':') {
    *p++ = c;
    while (lex_accept(lex, "0123456789+-.tT: Zz", &c))
      *p++ = c;
    *p = '\0';
    return lex->item.type = lex_TIME;
  }
  *p = '\0';
  lex_backup(lex);
  return lex->item.type = lex_INTEGER;
}

/* lex_scan_literal_string scans a literal string. */
static int lex_scan_literal_string(struct lexer *lex)
{
  int c;
  char *p;

  /* FIXME: validate string size */
  p = lex->item.val;
  for (; (c = lex_next(lex)) != lex_eof && c != '\'' && c != '\r' && c != '\n';
       p++)
    *p = c;
  *p = '\0';
  if (c == '\'')
    return lex->item.type = lex_STRING;
  if (c == '\r' || c == '\n')
    return lex_errorf(lex, "saw '\\n' before '\"'");
  return lex_errorf(lex, "saw eof before '\"'");
}

/* lex_scan_ml_literal_string scans a multiline literal string. */
static int lex_scan_ml_literal_string(struct lexer *lex)
{
  (void) lex;
  return lex_STRING;
}

/* lex_escape consumes an escaped character. */
static int lex_escape(struct lexer *lex)
{
  int c;

  switch (c = lex_next(lex)) {
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  case '"':
  case '\\':
    return c;
  case 'u': /* \uXXXX */
  case 'U': /* \UXXXXXXXX */
    break;
  }
  return lex_errorf(lex, "invalid escape sequence '\%c'", c);
}

/* lex_scan_string scans a basic string. */
static int lex_scan_string(struct lexer *lex)
{
  int c;
  char *p;

  /* FIXME: validate string size */
  p = lex->item.val;
  for (; (c = lex_next(lex)) != lex_eof && c != '"' && c != '\r' && c != '\n';
       p++) {
    if (c == '\\') {
      if ((c = lex_escape(lex)) == lex_ERROR)
        return lex_ERROR;
    }
    *p = c;
  }
  *p = '\0';
  if (c == '"')
    return lex->item.type = lex_STRING;
  if (c == '\r' || c == '\n')
    return lex_errorf(lex, "saw '\\n' before '\"'");
  return lex_errorf(lex, "saw eof before '\"'");
}

/* lex_scan_ml_string scans a multiline string. */
static int lex_scan_ml_string(struct lexer *lex)
{
  int c;
  char *p = lex->item.val;

  if (!lex_endofline(lex, c = lex_next(lex)))
    lex_backup(lex); /* was not a newline, put it back */

  /* FIXME: validate string size */
  for (;;) {
    /* the string can contain " and "", including at the end: """str"""""
       6 or more at the end, however, is an error. */
    int n;

    for (n = 0; (c = lex_next(lex)) == '"'; n++)
      ;
    if (n == 3 || n == 4 || n == 5) {
      lex_backup(lex); /* probably \r or \n */
      if (n == 4)      /* one double quote at the end: """" */
        *p++ = '"';
      else if (n == 5) { /* two double quotes at the end: """"" */
        *p++ = '"';
        *p++ = '"';
      }
      *p = '\0';
      return lex->item.type = lex_STRING;
    }
    if (c == lex_eof)
      return lex_errorf(lex, "saw eof before \"\"\"");
    if (n > 5)
      return lex_errorf(lex,
                        "too many double quotes at the end of "
                        "multiline string");
    for (int i = 0; i < n; i++)
      *p++ = '"';
    if (c == '\\') {
      if (isspace(lex_peek(lex))) {
        while (isspace(c = lex_next(lex)))
          ;
        lex_backup(lex);
        continue;
      }
      if ((c = lex_escape(lex)) == lex_ERROR)
        return lex_ERROR;
    }
    *p++ = c;
  }
}

/* lex_scan_identifier scans an alphanumeric. */
static int lex_scan_identifier(struct lexer *lex)
{
  char *p;
  int c;

  /* FIXME: validate item size */
  p = lex->item.val;
  for (; isalnum(c = lex_next(lex)) || c == '_' || c == '-'; p++)
    *p = c;
  *p = '\0';
  lex_backup(lex);
  if (strcmp(lex->item.val, "true") == 0 || strcmp(lex->item.val, "false") == 0)
    return lex->item.type = lex_BOOL;
  if (strcmp(lex->item.val, "inf") == 0 || strcmp(lex->item.val, "-inf") == 0 ||
      strcmp(lex->item.val, "nan") == 0 || strcmp(lex->item.val, "-nan") == 0)
    return lex->item.type = lex_FLOAT;
  return lex->item.type = lex_BARE_KEY;
}

void lex_init(struct lexer *lex, const char *input)
{
  lex->input = input;
  lex->ptr = lex->input;
  lex->pos = 1;
  lex->lineno = 1;
  lex->atEOF = false;
}

int lex_scan_next(struct lexer *lex)
{
  int c, cc;

  /* ignore whitespaces */
  while ((c = lex_next(lex)) == ' ' || c == '\t')
    ;

  /* ignore comments */
  if (c == '#') {
    do {
      while ((c = lex_next(lex)) != lex_eof && c != '\r' && c != '\n')
        ;
    } while (lex_peek(lex) == '#');  // multiline comments?
  }

  if (c == lex_eof) {
    lex->item.val[0] = '\0';
    return lex->item.type = lex_eof;
  }
  if (lex_endofline(lex, c)) {
    lex->item.val[0] = '\0';
    return lex->item.type = lex_NEWLINE;
  }

  if (c == '[') {
    if ((c = lex_next(lex)) == '[') {
      strcpy(lex->item.val, "[[");
      return lex->item.type = lex_LEFT_BRACKETS;
    }
    lex_backup(lex);
    strcpy(lex->item.val, "[");
    return lex->item.type = '[';
  }
  if (c == ']') {
    if ((c = lex_next(lex)) == ']') {
      strcpy(lex->item.val, "]]");
      return lex->item.type = lex_RIGHT_BRACKETS;
    }
    lex_backup(lex);
    strcpy(lex->item.val, "]");
    return lex->item.type = ']';
  }
  if (c == '=') {
    strcpy(lex->item.val, "=");
    return lex->item.type = '=';
  }
  if (c == '{') {
    strcpy(lex->item.val, "{");
    return lex->item.type = '{';
  }
  if (c == '}') {
    strcpy(lex->item.val, "}");
    return lex->item.type = '}';
  }
  if (c == ',') {
    strcpy(lex->item.val, ",");
    return lex->item.type = ',';
  }
  if (c == '.') {
    strcpy(lex->item.val, ".");
    return lex->item.type = '.';
  }
  if (c == '"') {
    if ((c = lex_next(lex)) == '"') {
      if ((c = lex_next(lex)) == '"') /* Got """ */
        return lex_scan_ml_string(lex);
      lex_backup(lex);
      lex->item.val[0] = '\0'; /* Got an empty string. */
      return lex->item.type = lex_STRING;
    }
    lex_backup(lex);
    return lex_scan_string(lex);
  }
  if (c == '\'') {
    if ((c = lex_next(lex)) == '\'') {
      if ((c = lex_next(lex)) == '\'') /* Got ''' */
        return lex_scan_ml_literal_string(lex);
      lex_backup(lex);
      lex->item.val[0] = '\0'; /* Got an empty string. */
      return lex->item.type = lex_STRING;
    }
    lex_backup(lex);
    return lex_scan_literal_string(lex);
  }

  if (c == '-' && (isalpha(cc = lex_peek(lex)) || cc == '-' || cc == '_')) {
    lex_backup(lex); /* put '-' back */
    return lex_scan_identifier(lex);
  }

  if (c == '+' || c == '-') {
    cc = lex_next(lex);
    if (cc == 'i') {
      if (lex_next(lex) != 'n' || lex_next(lex) != 'f')
        return lex_errorf(lex, "invalid float");
      if ((cc = lex_peek(lex)) != lex_eof && cc != '\r' && cc != '\n' &&
          cc != ' ')
        return lex_errorf(lex, "invalid float");
      snprintf(lex->item.val, sizeof(lex->item.val), "%cinf", c);
      return lex->item.type = lex_FLOAT;
    }
    if (cc == 'n') {
      if (lex_next(lex) != 'a' || lex_next(lex) != 'n')
        return lex_errorf(lex, "invalid float");
      if ((cc = lex_peek(lex)) != lex_eof && cc != '\r' && cc != '\n' &&
          cc != ' ')
        return lex_errorf(lex, "invalid float");
      snprintf(lex->item.val, sizeof(lex->item.val), "%cnan", c);
      return lex->item.type = lex_FLOAT;
    }
    if (cc == '.')
      return lex_errorf(lex, "floats cannot start with a '.'");
    if (cc == '0') {
      cc = lex_peek(lex);
      if (cc == 'x' || cc == 'o' || cc == 'b')
        return lex_errorf(lex, "cannot use sign with non-decimal numbers");
    }
    lex_backup(lex); /* put cc back */
    lex_backup(lex); /* put c back */
    return lex_scan_decimal_number(lex);
  }
  if (isdigit(c)) {
    lex_backup(lex);
    return lex_scan_number_or_date(lex);
  }
  if (isalpha(c) || c == '_') {
    lex_backup(lex);
    return lex_scan_identifier(lex);
  }
  return lex_errorf(lex, "unexpected character %c (%x)", c, c);
}