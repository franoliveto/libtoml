/* toml.c - parse TOML into fixed-extent data structures.
 *
 * This module parses a large subset of TOML (Tom’s Obvious Minimal Language).
 * Unlike more general TOML parsers, it doesn’t use malloc(3); you need to
 * give it a set of template structures describing the expected shape of
 * the incoming TOML, and it will error out if that shape is not matched.
 * When the parse succeeds, key values will be extracted into static
 * locations specified in the template structures.
 *
 * The dialect it parses has some limitations. First, only ASCII encoded
 * files are considered to be valid TOML documents. Second, all elements
 * of an array must be of the same type. Third, arrays may not be array
 * elements.
 *
 * Copyright (c) 2022, Francisco Oliveto <franciscoliveto@gmail.com>
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "toml.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h> /* HUGE_VAL */
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: UTF-8 support. */

enum {
  LBRACKETS,   /* [[ */
  RBRACKETS,   /* ]] */
  HEX_INTEGER, /* 0x[0-9abcdefABCDEF_] */
  OCT_INTEGER, /* 0o[0-7_] */
  BIN_INTEGER, /* 0b[01_] */
  INTEGER,     /* [+-0-9_] */
  BARE_KEY,    /* [A-Za-z0-9-_] */
  NEWLINE,     /* \r, \n, or \r\n */
  STRING,
  FLOAT,
};

const struct toml_key *curtab, *cursor;
static FILE *inputfp;
struct {
  int type;
  char lexeme[BUFSIZ];
  int pos;    /* position of the error, starting at 0 */
  int lineno; /* line number, starting at 1 */
} token;

#ifdef DEBUG_ENABLE
#include <stdarg.h>
void print(const char *fmt, ...)
{
  char buf[BUFSIZ];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  fputs(buf, stderr);
}

#define log_print(...) print(__VA_ARGS__)
#else
#define log_print(...) \
  do {                 \
  } while (0)
#endif

void error_printf(const char *fmt, ...)
{
  char buf[BUFSIZ];
  va_list ap;

  fprintf(stderr, "syntax error (line %d, column %d): ", token.lineno,
          token.pos);
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  fputs(buf, stderr);
  fputc('\n', stderr);
  exit(2);
}

/* Checks for and consume \r, \n, \r\n, or EOF */
static bool endofline(int c, FILE *fp)
{
  bool eol;

  eol = (c == '\r' || c == '\n');
  if (c == '\r') {
    c = getc(fp);
    if (c != '\n' && c != EOF)
      ungetc(c, fp); /* read to far, put it back */
  }
  return eol;
}

/* Returns but does not consume the next character in the input. */
static int lex_peek(FILE *fp)
{
  int c;
  c = getc(fp);
  ungetc(c, fp);
  return c;
}

/* Scans for a number (integer, float) */
static int lex_scan_number(int c, FILE *fp)
{
  bool isfloat = false;
  char *p = token.lexeme;

  /* FIXME: validate size */
  *p++ = c;
  while (isdigit(c = getc(fp)) || c == '_' || c == '.') {
    if (c == '.')
      isfloat = true;
    if (c != '_')
      *p++ = c;
  }
  *p = '\0';
  ungetc(c, fp);
  return isfloat ? FLOAT : INTEGER;
}

/* Scans for a literal string. */
static int lex_scan_literal_string(FILE *fp)
{
  int c;
  char *p = token.lexeme;

  while ((c = getc(fp)) != '\'' && c != '\r' && c != '\n')
    *p++ = c;
  *p = '\0';
  if (c == '\'')
    return STRING;
  if (c == '\r' || c == '\n')
    error_printf("saw '\\n' before '\''");
  else if (c == EOF) {
    if (ferror(fp))
      error_printf("input failed");
    else
      error_printf("saw EOF before '\''");
  }
  return -1;  // FIXME: what to return in case of error?
}

/* Consumes an escaped character. */
static int lex_escape(FILE *fp)
{
  int c;

  switch (c = getc(fp)) {
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
  error_printf("invalid escape sequence '\%c'", c);
  return -1;  // FIXME: what to return in case of error?
}

/* Scans for multiline literal strings. */
static int lex_scan_ml_literal_string(FILE *fp)
{
  (void) fp;
  return STRING;
}

/* Scans for multiline strings. */
static int lex_scan_ml_string(FILE *fp)
{
  int c;
  char *p = token.lexeme;

  if (!endofline(c = getc(fp), fp))
    ungetc(c, fp); /* was not a newline, put it back */

  /* FIXME: validate string size */
  for (;;) {
    /* the string can contain " and "", including at the end: """str"""""
       6 or more at the end, however, is an error. */
    int n;
    for (n = 0; (c = getc(fp)) == '"';)
      n++;
    if (n == 3 || n == 4 || n == 5) {
      ungetc(c, fp); /* probably \r or \n */
      if (n == 4)    /* one double quote at the end: """" */
        *p++ = '"';
      else if (n == 5) { /* two double quotes at the end: """"" */
        *p++ = '"';
        *p++ = '"';
      }
      *p = '\0';
      return STRING;
    }
    if (c == EOF)
      error_printf("saw EOF before \"\"\"");
    if (n > 5)
      error_printf(
          "too many double quotes at the end of "
          "multiline string");
    for (int i = 0; i < n; i++)
      *p++ = '"';
    if (c == '\\') {
      int peek = lex_peek(fp);
      if (isspace(peek)) {
        while (isspace(c = getc(fp)))
          ;
        ungetc(c, fp);
        continue;
      }
      c = lex_escape(fp);
    }
    *p++ = c;
  }
}

/* Scans for a basic string. */
static int lex_scan_string(FILE *fp)
{
  int c;
  char *p = token.lexeme;

  /* FIXME: validate string size */
  while ((c = getc(fp)) != '"' && c != '\r' && c != '\n') {
    if (c == '\\')
      c = lex_escape(fp);
    *p++ = c;
  }
  *p = '\0';
  if (c == '"')
    return STRING;
  if (c == '\r' || c == '\n')
    error_printf("saw '\\n' before '\"'");
  else if (c == EOF) {
    if (ferror(fp))
      error_printf("input failed");
    else
      error_printf("saw EOF before '\"'");
  }
  return -1;  // FIXME: what to return in case of error?
}

/* lex_scan scans for the next valid token. */
static int lex_scan(FILE *fp)
{
  int c;

  while ((c = getc(fp)) != EOF) {
    if (c == ' ' || c == '\t')
      continue;
    if (c == '#') { /* ignore comment */
      while ((c = getc(inputfp)) != EOF && c != '\r' && c != '\n')
        ;
      ungetc(c, inputfp); /* put \r or \n back */
      continue;
    }

    if (c == '[') {
      if ((c = getc(fp)) == '[')
        return token.type = LBRACKETS;
      ungetc(c, fp);
      return token.type = '[';
    }
    if (c == ']') {
      if ((c = getc(fp)) == ']')
        return token.type = RBRACKETS;
      ungetc(c, fp);
      return token.type = ']';
    }
    if (c == '=')
      return token.type = '=';
    if (c == '{')
      return token.type = '{';
    if (c == '}')
      return token.type = '}';
    if (c == ',')
      return token.type = ',';
    if (c == '.')
      return token.type = '.';
    if (c == '"') {
      if ((c = getc(fp)) == '"') {
        if ((c = getc(fp)) == '"') /* Got """ */
          return token.type = lex_scan_ml_string(fp);
        ungetc(c, fp);
        token.lexeme[0] = '\0'; /* Got an empty string. */
        return token.type = STRING;
      }
      ungetc(c, fp);
      return token.type = lex_scan_string(fp);
    }
    if (c == '\'') {
      if ((c = getc(fp)) == '\'') {
        if ((c = getc(fp)) == '\'') /* Got ''' */
          return token.type = lex_scan_ml_literal_string(fp);
        ungetc(c, fp);
        token.lexeme[0] = '\0'; /* Got an empty string. */
        return token.type = STRING;
      }
      ungetc(c, fp);
      return token.type = lex_scan_literal_string(fp);
    }
    if (c == '0') {
      int savedc = c;
      char *p = token.lexeme;

      *p++ = c;
      c = getc(fp);
      if (c == 'x') { /* hexadecimal */
        for (*p++ = c; isxdigit(c = getc(fp)) || c == '_';)
          *p++ = c; /* FIXME: validate size */
        *p = '\0';
        ungetc(c, fp);
        return token.type = HEX_INTEGER;
      }
      if (c == 'o') { /* octal */
        for (*p++ = c; ((c = getc(fp)) >= '0' && c <= '7') || c == '_';)
          *p++ = c; /* FIXME: validate size */
        *p = '\0';
        ungetc(c, fp);
        return token.type = OCT_INTEGER;
      }
      if (c == 'b') { /* binary */
        for (*p++ = c; (c = getc(fp)) == '0' || c == '1' || c == '_';)
          *p++ = c; /* FIXME: validate size */
        *p = '\0';
        ungetc(c, fp);
        return token.type = BIN_INTEGER;
      }
      ungetc(c, fp); /* was not a prefix, put it back */
      c = savedc;
    }

    if (c == '+' || c == '-') {
      int nextc = lex_peek(fp);
      if (isdigit(nextc))
        return token.type = lex_scan_number(c, fp); /* INTEGER, FLOAT */
      if (nextc == 'i') {
        (void) getc(fp); /* consume i */
        if (getc(fp) == 'n') {
          if (getc(fp) == 'f') {
            sprintf(token.lexeme, "%cinf", c);
            return token.type = FLOAT;
          }
        }
        error_printf("invalid float");
      }
      if (nextc == 'n') {
        (void) getc(fp); /* consume n */
        if (getc(fp) == 'a') {
          if (getc(fp) == 'n') {
            sprintf(token.lexeme, "%cnan", c);
            return token.type = FLOAT;
          }
        }
        error_printf("invalid float");
      }
      error_printf("only numbers can start with + or -");
    }
    if (isdigit(c))
      return token.type = lex_scan_number(c, fp); /* INTEGER, FLOAT */

    /* keywords: inf, nan, true, false */

    /* FIXME: could also start with '-' or '_' or digit. */
    if (isalpha(c)) {
      char *p = token.lexeme;

      for (*p++ = c;
           isalpha(c = getc(fp)) || isdigit(c) || c == '-' || c == '_';)
        *p++ = c; /* FIXME: validate token size */
      *p = '\0';
      ungetc(c, fp);
      return token.type = BARE_KEY;
    }

    if (endofline(c, fp)) {
      token.lineno++;
      return token.type = NEWLINE;
    }

    return token.type = c; /* anything else */
  }
  return EOF;
}

void keyval();

static char *target_address(const struct toml_key *cursor,
                            const struct toml_array *array, int offset)
{
  char *addr = NULL;

  if (array == NULL) {
    switch (cursor->type) {
    case toml_short_t:
      addr = (char *) cursor->u.integer.s;
      break;
    case toml_ushort_t:
      addr = (char *) cursor->u.integer.us;
      break;
    case toml_int_t:
      addr = (char *) cursor->u.integer.i;
      break;
    case toml_uint_t:
      addr = (char *) cursor->u.integer.ui;
      break;
    case toml_long_t:
      addr = (char *) cursor->u.integer.l;
      break;
    case toml_ulong_t:
      addr = (char *) cursor->u.integer.ul;
      break;
    case toml_float_t:
      addr = (char *) cursor->u.real;
      break;
    case toml_bool_t:
      addr = (char *) cursor->u.boolean;
      break;
    case toml_string_t:
      addr = cursor->u.string;
      break;
    default:
      break;
    }
  } else {
    addr = array->u.tables.base + (offset * array->u.tables.structsize) +
           cursor->u.offset;
  }
  log_print("target address for %s is %p.\n", cursor->name, addr);
  return addr;
}

static void array()
{
  const struct toml_array *array = &cursor->u.array;
  char *sp = array->u.strings.store;
  size_t offset = 0;

  do {
    while (lex_scan(inputfp) == NEWLINE)
      ;
    if (token.type == ']') /* end of array */
      break;
    if (token.type == ',') {
      log_print("Invalid syntax: got ',' when expecting token.\n");
      exit(1);
    }
    if (offset >= array->len) {
      log_print("Too many elements in array.\n");
      exit(1);
    }

    switch (token.type) {
    case STRING: {
      size_t used, free, len;

      if (array->type != toml_string_t) {
        log_print("not expecting a string.\n");
        exit(1);
      }
      array->u.strings.ptrs[offset] = sp;
      used = sp - array->u.strings.store;
      free = array->u.strings.storelen - used;
      len = strlen(token.lexeme);
      if (len + 1 > free) {
        log_print("Ran out of storage for strings.\n");
        exit(1);
      }
      memcpy(sp, token.lexeme, len);
      sp[len] = '\0';
      sp = sp + len + 1;
      break;
    }
    case INTEGER:
    case HEX_INTEGER:
    case OCT_INTEGER:
    case BIN_INTEGER: {
      char *endptr;
      long val;

      errno = 0;
      val = strtol(token.lexeme, &endptr, 0);
      if (errno != 0 || token.lexeme == endptr) {
        log_print("Error parsing a number.\n");
        exit(1);
      }
      switch (array->type) {
      case toml_short_t:
        array->u.integer.s[offset] = (short) val;
        break;
      case toml_ushort_t:
        array->u.integer.us[offset] = (unsigned short) val;
        break;
      case toml_int_t:
        array->u.integer.i[offset] = (int) val;
        break;
      case toml_uint_t:
        array->u.integer.ui[offset] = (unsigned int) val;
        break;
      case toml_long_t:
        array->u.integer.l[offset] = val;
        break;
      case toml_ulong_t:
        array->u.integer.ul[offset] = (unsigned long) val;
        break;
      default:
        log_print("not expecting an integer value.\n");
        exit(1);
      }
      break;
    }
    case FLOAT: {
      char *endptr;
      double val;

      if (array->type != toml_float_t) {
        log_print("Saw float when not expecting a real value.\n");
        exit(1);
      }
      errno = 0;
      val = strtod(token.lexeme, &endptr);
      if (errno != 0 || token.lexeme == endptr) {
        log_print("Error parsing a number.\n");
        exit(1);
      }
      array->u.real[offset] = val;
      break;
    }
    case BARE_KEY: {
      // case TRUE:
      // case FALSE:
      // {
      //   array->u.boolean[offset] = strcmp(token.lexeme, "true") == 0;
      // }
      bool val;

      if (strcmp(token.lexeme, "true") == 0)
        val = true;
      else if (strcmp(token.lexeme, "false") == 0)
        val = false;
      else {
        log_print("Got '%s' when expecting boolean.\n", token.lexeme);
        exit(1);
      }
      array->u.boolean[offset] = val;
      break;
    }
    case '{':  // inline-tables [ { }, { } ]
      if (cursor->u.array.type != toml_table_t) {
        log_print("Saw { when not expecting inline table.\n");
        exit(1);
      }
      break;
    }
    offset++;
    while (lex_scan(inputfp) == NEWLINE)
      ;
  } while (token.type == ',');

  if (token.type != ']')
    error_printf("expected ']'");

  if (array->count != NULL)
    *(array->count) = offset;
}

void inline_table()
{
  do {
    lex_scan(inputfp); /* FIXME: lex_next()??? */
    if (token.type == BARE_KEY || token.type == STRING)
      keyval();
    else
      error_printf("expected key");
  } while (lex_scan(inputfp) == ',');

  if (token.type != '}')
    error_printf("expected '}'");
}

void value()
{
  switch (token.type) {
  case '[':
    if (cursor->type != toml_array_t) {
      log_print("Saw [ when not expecting array.\n");
      // return ERR_UNEXPECTED_ARRAY;
      exit(1);
    }
    // FIXME: handle errors
    array();
    break;
  case '{':
    if (cursor->type != toml_table_t) {
      log_print("Saw { when not expecting table.\n");
      // return ERR_UNEXPECTED_TABLE;
      exit(1);
    }
    inline_table();
    break;
  case STRING: {
    char *p;

    if (cursor->type != toml_string_t) {
      log_print("saw quoted value when expecting non-string\n");
      exit(1);
    }

    p = target_address(cursor, NULL, 0);
    if (p == NULL)
      return;

    size_t s = cursor->size;
    strncpy(p, token.lexeme, s - 1);
    p[s - 1] = '\0';
    break;
  }
  case FLOAT: {
    char *endptr;
    char *p;
    double val;

    if (cursor->type != toml_float_t) {
      log_print("saw float value when not expecting a real.\n");
      exit(1);
    }

    p = target_address(cursor, NULL, 0);
    if (p == NULL)
      return;

    errno = 0;
    val = strtod(token.lexeme, &endptr);
    if (errno != 0 || token.lexeme == endptr) {
      log_print("Error parsing a number.\n");
      exit(1);
    }
    memcpy(p, &val, sizeof(double));
    break;
  }
  case INTEGER:
  case HEX_INTEGER:
  case OCT_INTEGER:
  case BIN_INTEGER: {
    char *p;
    char *endptr;
    long val;

    p = target_address(cursor, NULL, 0);
    if (p == NULL)
      return;

    errno = 0;
    val = strtol(token.lexeme, &endptr, 0);
    if (errno != 0 || token.lexeme == endptr) {
      log_print("Not a valid number.\n");
      exit(1);
    }
    switch (cursor->type) {
    case toml_short_t: {
      short tmp = (short) val;
      memcpy(p, &tmp, sizeof(short));
      break;
    }
    case toml_ushort_t: {
      unsigned short tmp = (unsigned short) val;
      memcpy(p, &tmp, sizeof(unsigned short));
      break;
    }
    case toml_int_t: {
      int tmp = (int) val;
      memcpy(p, &tmp, sizeof(int));
      break;
    }
    case toml_uint_t: {
      unsigned int tmp = (unsigned int) val;
      memcpy(p, &tmp, sizeof(unsigned int));
      break;
    }
    case toml_long_t:
      memcpy(p, &val, sizeof(long));
      break;
    case toml_ulong_t: {
      unsigned long tmp = (unsigned long) val;
      memcpy(p, &tmp, sizeof(unsigned long));
      break;
    }
    default:
      log_print("saw integer value when not expecting integers.\n");
      exit(1);
    }
    break;
  }
  case BARE_KEY: {
    char *p;
    bool val;

    p = target_address(cursor, NULL, 0);
    if (p == NULL)
      return;

    if (strcmp(token.lexeme, "true") == 0)
      val = true;
    else if (strcmp(token.lexeme, "false") == 0)
      val = false;
    else {
      log_print("Got '%s' when expecting boolean.\n", token.lexeme);
      exit(1);
    }
    memcpy(p, &val, sizeof(bool));
    break;
  }
  default:
    error_printf("invalid token");
  }
}

void key()
{
  /* simple-key or dotted-key */
  for (cursor = curtab; cursor->name != NULL; cursor++) {
    if (strcmp(cursor->name, token.lexeme) == 0)
      break;
  }
  if (cursor->name == NULL) {
    fprintf(stderr, "unknown key name '%s'\n", token.lexeme);
    exit(2);
  }

  while (lex_scan(inputfp) == '.') {
    lex_scan(inputfp);
    if (token.type == BARE_KEY || token.type == STRING)
      puts(token.lexeme); /* key is used */
    else
      error_printf("expected dotted key");
  }
}

int accept(int type)
{
  if (token.type == type) {
    lex_scan(inputfp);
    return 1;
  }
  return 0;
}

void keyval()
{
  key();
  if (accept('='))
    value();
  else
    error_printf("missing '='");
}

void expression()
{
  /* array-table = [[ key ]] */
  if (accept(LBRACKETS)) {
    switch (token.type) {
    case BARE_KEY:
    case STRING:
      key();
      if (token.type != RBRACKETS)
        error_printf("missing ']]'");
      break;
    default:
      error_printf("key was expected");
    }
  }
  /* table = [ key ] */
  else if (accept('[')) {
    switch (token.type) {
    case BARE_KEY:
    case STRING:
      key();
      if (token.type != RBRACKETS)
        error_printf("missing ']'");
      break;
    default:
      error_printf("key was expected");
    }
  }
  /* key */
  else if (token.type == BARE_KEY || token.type == STRING) {
    keyval();
  } else {
    error_printf("invalid token");
    // return ERR_INVALID_TOKEN;
  }
}

int toml_unmarshal(FILE *f, const struct toml_key *template)
{
  inputfp = f;
  curtab = template;
  token.lineno = 1;
  while (lex_scan(fp) != EOF) {
    if (token.type == NEWLINE)
      continue;
    expression();
    if (lex_scan(fp) == EOF)
      break;
    if (token.type != NEWLINE)
      error_printf("expected newline");
  }
  return 0;
}

const char *toml_strerror(int errnum)
{
  (void) errnum;
  return "there was an error";
}
