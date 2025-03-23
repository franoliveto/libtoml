#ifndef TOML_H_
#define TOML_H_

#include <stdbool.h>
#include <stddef.h> /* offsetof(3) */
#include <stdio.h>

/* The different types of the key values. */
enum toml_type {
  toml_short_t,
  toml_ushort_t,
  toml_int_t,
  toml_uint_t,
  toml_long_t,
  toml_ulong_t,
  toml_float_t,
  toml_bool_t,
  toml_string_t,
  toml_array_t,
  toml_table_t,
  toml_time_t
};

/* The representation of an array value. All elements of the
   array must be of the same type. Arrays may not be array
   elements. */
struct toml_array {
  /* The type of the values of the array. */
  enum toml_type type;
  /* The number of elements in the array. */
  int *count;
  /* The maximum capacity of the array.*/
  size_t len;

  union {
    double *real;
    bool *boolean;
    struct {
      const struct toml_key *subtype;
      char *base;
      size_t structsize;
    } tables;
    struct {
      char **ptrs;
      char *store;
      int storelen;
    } strings;
    union {
      short *s;
      unsigned short *us;
      int *i;
      unsigned int *ui;
      long *l;
      unsigned long *ul;
    } integer;
  } u;
};

/* The representation of a single key/value pair. */
struct toml_key {
  /* The name of the key. */
  const char *name;
  /* The type of the value stored in the field u. */
  enum toml_type type;

  union {
    /* toml_string_t */
    char *string;
    /* toml_float_t */
    double *real;
    /* toml_bool_t */
    bool *boolean;
    union {
      /* toml_short_t */
      short *s;
      /* toml_ushort_t */
      unsigned short *us;
      /* toml_int_t */
      int *i;
      /* toml_uint_t */
      unsigned int *ui;
      /* toml_long_t */
      long *l;
      /* toml_ulong_t */
      unsigned long *ul;
    } integer;
    /* toml_table_t */
    const struct toml_key *table;
    /* toml_array_t */
    const struct toml_array array;
    size_t offset;
  } u;
  /* The size of the array of characters pointed to by string. */
  size_t size;
};

/* toml_unmarshal parses the TOML-encoded data of f and stores
   the result into static locations specified in the template
   structure refered to by template. */
// int toml_unmarshal(FILE *f, const struct toml_key *template);

/* toml_unmarshal parses TOML-encoded text and stores the result into
   static locations specified in the set of template structures templ. */
int toml_unmarshal(const char *text, const struct toml_key *templ);

/* toml_strerror returns a pointer to a string that describes
   the last error. */
const char *toml_strerror();

/* toml_len returns the capacity of the array a. */
#define toml_len(a) (sizeof(a) / sizeof(a[0]))

/* Use the following macros to declare template initializers
   for arrays of strings and tables. Writing the equivalentes
   out by hand is error-prone. */

/* toml_array_strings takes the base address of an array of
   pointers to char, the base address of the storage, and the
   address of an integer to store the lenght in. */
#define toml_array_strings(p, s, n)                                      \
  .u.array.type = toml_string_t, .u.array.u.strings.ptrs = p,            \
  .u.array.u.strings.store = s, .u.array.u.strings.storelen = sizeof(s), \
  .u.array.count = n, .u.array.len = (sizeof(p) / sizeof(p[0]))

/* toml_array_tables takes the base address of an array of structs,
   an array of template of structures describing the expected
   shape of the incoming table, and the address of an integer
   to store the length in. */
#define toml_array_tables(a, t, n)                                 \
  .u.array.type = toml_table_t, .u.array.u.tables.subtype = t,     \
  .u.array.u.tables.base = (char *) a,                             \
  .u.array.u.tables.structsize = sizeof(a[0]), .u.array.count = n, \
  .u.array.len = (sizeof(a) / sizeof(a[0]))

/* toml_table_field takes a structure name s, and a field name
   f in s. */
#define toml_table_field(s, f) .u.offset = offsetof(s, f)

#endif /* TOML_H_ */
