/* Copyright 2022 Francisco Oliveto. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 * libtoml parses TOML into fixed-extent data structures.
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
 */
#include "toml.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h> /* HUGE_VAL */
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"

/* TODO: UTF-8 support. */

static struct lexer lex;
static const struct toml_key *template;

/* cursor points to the template structure being inspected. */
static const struct toml_key *cursor;

static const struct toml_array *arrayp;
static size_t array_offset = 0;

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

static char errbuf[BUFSIZ];

static int errorf(const char *format, ...)
{
  int sz;
  va_list ap;

  sz = snprintf(errbuf, sizeof(errbuf), "%d:%d: error: ", lex.lineno, lex.pos);

  va_start(ap, format);
  vsnprintf(errbuf + sz, sizeof(errbuf) - sz, format, ap);
  va_end(ap);
  return -1;
}

int keyval();

// target_address returns a pointer to the static location associated with
// cursor structure.
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
    case toml_time_t:
      // TODO
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

// WORK IN PROGRESS
static int array()
{
  const struct toml_array *ap = &cursor->u.array;
  // char *sp = ap->u.strings.store;
  size_t pos = 0;

  do {
    while (lex_scan_next(&lex) == lex_NEWLINE)
      ;
    if (lex.item.type == ']') /* end of array */
      break;
    if (lex.item.type == ',')
      return errorf("got ',' when expecting a value.");

    if (pos >= ap->len)
      return errorf("too many elements in array.");

    switch (lex.item.type) {
      // case '{':
      //   if (ap->type != toml_table_t)
      //     return errorf("saw { when not expecting table.\n");
      //   if (inline_table(ap->u.tables.subtype) == -1)
      //     return -1;
    }
    pos++;
    while (lex_scan_next(&lex) == lex_NEWLINE)
      ;
  } while (lex.item.type == ',');

  if (lex.item.type != ']')
    return errorf("expected ']'");

  if (ap->count != NULL)
    *(ap->count) = pos;
  return 0;
}

static int inline_table(const struct toml_key *templ)
{
  int type;
  const struct toml_key *saved = template;
  // template = cursor->u.table;
  template = templ;

  do {
    type = lex_scan_next(&lex);
    if (type != lex_BARE_KEY && type != lex_STRING)
      return errorf("expected key");

    if (keyval() == -1)
      return -1;
  } while (lex_scan_next(&lex) == ',');

  if (lex.item.type != '}')
    return errorf("expected '}'");

  template = saved;
  return 0;
}

static int val()
{
  char *ptr = NULL;
  char *endptr;

  switch (lex.item.type) {
  case '[':
    if (cursor->type != toml_array_t)
      return errorf("saw [ when not expecting an array.");
    return array();
    break;
  case '{':
    if (cursor->type != toml_table_t)
      return errorf("saw { when not expecting table.\n");
    return inline_table(cursor->u.table);
  case lex_STRING:
    if (cursor->type != toml_string_t)
      return errorf("saw quoted value when expecting non-string");

    ptr = target_address(cursor, arrayp, array_offset);
    if (ptr == NULL)
      return errorf("can't get target address");
    strncpy(ptr, lex.item.val, cursor->size - 1);
    ptr[cursor->size - 1] = '\0';
    break;
  case lex_FLOAT: {
    double val;

    if (cursor->type != toml_float_t)
      return errorf("saw a real value when not expecting a float");

    ptr = target_address(cursor, arrayp, array_offset);
    if (ptr == NULL)
      return errorf("can't get target address");
    errno = 0;
    val = strtod(lex.item.val, &endptr);
    if (errno != 0 || lex.item.val == endptr)
      return errorf("%s is not a valid number %s", lex.item.val);

    memcpy(ptr, &val, sizeof(double));
    break;
  }
  case lex_INTEGER: {
    long val;

    ptr = target_address(cursor, arrayp, array_offset);
    if (ptr == NULL)
      return errorf("can't get target address");
    errno = 0;
    val = strtol(lex.item.val, &endptr, 0);
    if (errno != 0 || lex.item.val == endptr)
      return errorf("%s is not a valid number %s", lex.item.val);

    switch (cursor->type) {
    case toml_short_t: {
      short tmp = (short) val;
      memcpy(ptr, &tmp, sizeof(short));
      break;
    }
    case toml_ushort_t: {
      unsigned short tmp = (unsigned short) val;
      memcpy(ptr, &tmp, sizeof(unsigned short));
      break;
    }
    case toml_int_t: {
      int tmp = (int) val;
      memcpy(ptr, &tmp, sizeof(int));
      break;
    }
    case toml_uint_t: {
      unsigned int tmp = (unsigned int) val;
      memcpy(ptr, &tmp, sizeof(unsigned int));
      break;
    }
    case toml_long_t:
      memcpy(ptr, &val, sizeof(long));
      break;
    case toml_ulong_t: {
      unsigned long tmp = (unsigned long) val;
      memcpy(ptr, &tmp, sizeof(unsigned long));
      break;
    }
    default:
      return errorf("saw integer value when not expecting integers.");
    }
    break;
  }
  case lex_BOOL: {
    bool val;

    ptr = target_address(cursor, arrayp, array_offset);
    if (ptr == NULL)
      return errorf("can't get target address");
    val = strcmp(lex.item.val, "true") == 0;
    memcpy(ptr, &val, sizeof(bool));
    break;
  }
  case lex_TIME:  // TODO
    break;
  default:
    return errorf("invalid token");
  }
  return 0;
}

/* key looks for name in templ. If found, global variable 'cursor'
   will point to the template structure with that name.
   It returns 0 if found, -1 if not. */
static int key(const char *name, const struct toml_key *templ)
{
  // TODO: dotted-key
  for (cursor = templ; cursor->name != NULL; cursor++) {
    if (strcmp(cursor->name, name) == 0)
      break;
  }
  if (cursor->name == NULL)
    return errorf("unknown key name '%s'\n", lex.item.val);
  return 0;
}

bool accept(int type)
{
  if (lex.item.type == type) {
    lex_scan_next(&lex);
    return true;
  }
  return false;
}

int keyval()
{
  key(lex.item.val, template);

  if (lex_scan_next(&lex) != '=')
    return errorf("missing '='");

  lex_scan_next(&lex);
  return val();
  // if (accept('='))
  //   return value();
}

int expr(const struct toml_key *templ)
{
  static const char *name = NULL;

  /* array-table = [[ key ]] */
  if (accept(lex_LEFT_BRACKETS)) {
    if (lex.item.type != lex_BARE_KEY && lex.item.type != lex_STRING)
      return errorf("key was expected on array table");

    key(lex.item.val, templ);

    if (lex_scan_next(&lex) != lex_RIGHT_BRACKETS)
      return errorf("missing ']]'");

    if (cursor->type != toml_array_t || cursor->u.array.type != toml_table_t)
      return errorf("saw [[ when not expecting an array of tables");

    /* new array of tables? */
    if (name == NULL || strcmp(name, cursor->name) != 0) {
      array_offset = 0;
      name = cursor->name;
    } else
      array_offset++;

    if (array_offset >= cursor->u.array.len)
      return errorf("too many elements in array");

    template = cursor->u.array.u.tables.subtype;  // TODO: rename subtype
    arrayp = &cursor->u.array;

    if (arrayp->count != NULL)
      *arrayp->count = array_offset + 1;

  }
  /* table = [ key ] */
  else if (accept('[')) {
    if (lex.item.type != lex_BARE_KEY && lex.item.type != lex_STRING)
      return errorf("key was expected on table");

    if (key(lex.item.val, templ) == -1)
      return -1;

    if (lex_scan_next(&lex) != ']')
      return errorf("missing ']'");

    if (cursor->type != toml_table_t)
      return errorf("saw [ when not expecting a table");

    template = cursor->u.table;
    arrayp = NULL;
    array_offset = 0;
  }
  /* key */
  else if (lex.item.type == lex_BARE_KEY || lex.item.type == lex_STRING) {
    return keyval();
  } else {
    return errorf("invalid token");
  }
  return 0;
}

int toml_unmarshal(const char *text, const struct toml_key *templ)
{
  lex_init(&lex, text);
  template = templ;
  while (lex_scan_next(&lex) != lex_eof) {
    if (lex.item.type == lex_NEWLINE)
      continue;

    if (expr(templ) == -1)
      return -1;

    if (lex_scan_next(&lex) == lex_eof)
      break;
    if (lex.item.type != lex_NEWLINE)
      return errorf("expected newline after expression");
  }
  return 0;
}

const char *toml_strerror()
{
  return errbuf;
}
