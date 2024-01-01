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

/* The representation of a key/value pair. */
struct toml_key {
  /* The name of the key. */
  const char *name;

  /* The type of the value stored in the field u. */
  enum toml_type type;

  union {
    /* TOML_TYPE_STRING */
    char *string;
    /* TOML_TYPE_FLOAT */
    double *real;
    /* TOML_TYPE_BOOL */
    bool *boolean;
    union {
      /* TOML_TYPE_SHORT */
      short *s;
      /* TOML_TYPE_USHORT */
      unsigned short *us;
      /* TOML_TYPE_INT */
      int *i;
      /* TOML_TYPE_UINT */
      unsigned int *ui;
      /* TOML_TYPE_LONG */
      long *l;
      /* TOML_TYPE_ULONG */
      unsigned long *ul;
    } integer;
    /* TOML_TYPE_TABLE */
    const struct toml_key *table;
    /* TOML_TYPE_ARRAY */
    const struct toml_array array;
    size_t offset;
  } u;

  /* The size of the array of characters pointed to by string. */
  size_t size;
};

/* toml_unmarshal parses the TOML-encoded data of fp and stores
   the result into static locations specified in the template
   structure refered to by template. */
int toml_unmarshal(FILE *fp, const struct toml_key *template);

/* int toml_marshal(); */

/* toml_strerror returns a pointer to a string that describes
   the error code errnum. */
const char *toml_strerror(int errnum);

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
#define toml_array_tables(a, t, n)                               \
  .u.array.type = toml_table_t, .u.array.u.tables.subtype = t,   \
  .u.array.u.tables.base = (char *) a,                           \
  .u.array.u.tables.structsize = sizeof(a[0]), .u.array.len = n, \
  .u.array.cap = (sizeof(a) / sizeof(a[0]))

/* toml_table_field takes a structure name s, and a fieldname
   f in s. */
#define toml_table_field(s, f) .u.offset = offsetof(s, f)

#endif /* TOML_H_ */
