#ifndef _MICRO_TOML_H_
#define _MICRO_TOML_H_

#include <stdbool.h>
#include <stddef.h> /* offsetof(3) */
#include <stdio.h>

enum toml_type {
    string_t,
    short_t,
    ushort_t,
    int_t,
    uint_t,
    long_t,
    ulong_t,
    bool_t,
    real_t,
    array_t,
    table_t,
};

struct toml_array_t {
    enum toml_type type;
    int *len;
    int cap;  // the capacity of the array
    union {
        struct {
            const struct toml_key_t *subtype;
            char *base;
            size_t structsize;
        } tables;
        struct {
            char **ptrs;
            char *store;
            int storelen;
        } strings;
        short int *si;
        unsigned short int *usi;
        int *i;
        unsigned int *ui;
        long int *li;
        unsigned long int *uli;
        double *r;
        bool *b;
    } base;
};

struct toml_key_t {
    const char *key;
    enum toml_type type;
    union {
        short int *si;
        unsigned short int *usi;
        int *i;
        unsigned int *ui;
        long int *li;
        unsigned long int *uli;
        double *r;
        bool *b;
        struct {
            char *s;
            size_t len;
        } string;
        const struct toml_key_t *keys;
        const struct toml_array_t array;
        size_t offset;
    } ptr;
};

/* toml_unmarshal parses the TOML-encoded data of fp and stores
   the result into static locations specified in the template
   structure refered by keys. */
int toml_unmarshal(FILE *fp, const struct toml_key_t *tab);

/* int toml_marshal(); */

/* toml_strerror returns a pointer to a string that describes
   the error code err. */
const char *toml_strerror(int err);

/* toml_cap MACRO returns the capacity of the array a. */
#define toml_cap(a) (sizeof(a) / sizeof(a[0]))

/* Use the following macros to declare template initializers
   for arrays of strings and tables. Writing the equivalentes
   out by hand is error-prone. */

/* STRINGARRAY takes the base address of an array of pointers to char,
   the base address of the storage,
   and the address of an integer to store the lenght in. */
#define STRINGARRAY(p, s, n)                                          \
    .ptr.array.type = string_t, .ptr.array.base.strings.ptrs = p,     \
    .ptr.array.base.strings.store = s,                                \
    .ptr.array.base.strings.storelen = sizeof(s), .ptr.array.len = n, \
    .ptr.array.cap = (sizeof(p) / sizeof(p[0]))

/* TABLEFIELD takes a structure name s, and a fieldname f in s.

  TABLEARRAY takes the base address of an array of structs, an
  array of template of structures describing the expected shape
  of the incoming table, and the address of an integer to store
  the length in. */
#define TABLEFIELD(s, f) .ptr.offset = offsetof(s, f)
#define TABLEARRAY(a, t, n)                                               \
    .ptr.array.type = table_t, .ptr.array.arr.tables.subtype = t,         \
    .ptr.array.base.tables.base = (char *) a,                             \
    .ptr.array.base.tables.structsize = sizeof(a[0]), .ptr.array.len = n, \
    .ptr.array.cap = sizeof(a) / sizeof(a[0])

#endif /* _MICRO_TOML_H_ */
