#ifndef _MICRO_TOML_H_
#define _MICRO_TOML_H_

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> /* offsetof(3) */


enum toml_type {
    string_t,
    integer_t,
    uinteger_t,
    long_t,
    ulong_t,
    short_t,
    ushort_t,
    boolean_t,
    real_t,
    array_t,
    table_t
};

struct toml_array_t {
    enum toml_type type;
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
        int *integers;
        unsigned int *uintegers;
        short *shortints;
        unsigned short *ushortints;
        long *longints;
        unsigned long *ulongints;
        double *reals;
        bool *booleans;
    } arr;
    int *count;
    int maxlen;
};

struct toml_key_t {
    const char *key;
    enum toml_type type;
    union {
        int *integer;
        unsigned int *uinteger;
        short *shortint;
        unsigned short *ushortint;
        long *longint;
        unsigned long *ulongint;
        double *real;
        bool *boolean;
        char *string;
        const struct toml_key_t *keys;
        const struct toml_array_t array;
        size_t offset;
    } addr;
    size_t len;
};


int toml_unmarshal(FILE *fp, const struct toml_key_t *keys);
/* int toml_marshal(); */
const char *toml_strerror(int errnum);


/* Use the following macros to declare template initializers
   for arrays of strings and tables. Writing the equivalentes
   out by hand is error-prone.

   STRINGARRAY takes the base address of an array of
   pointers to char, the base address of the storage,
   and the address of an integer to store the lenght in. */
#define STRINGARRAY(p, s, n)                        \
    .addr.array.type = string_t,                    \
    .addr.array.arr.strings.ptrs = p,               \
    .addr.array.arr.strings.store = s,              \
    .addr.array.arr.strings.storelen = sizeof(s),   \
    .addr.array.count = n,                          \
    .addr.array.maxlen = (sizeof(p)/sizeof(p[0]))


/* TABLEFIELD takes a structure name s, and a fieldname f in s.

  TABLEARRAY takes the base address of an array of structs, an
  array of template of structures describing the expected shape
  of the incoming table, and the address of an integer to store
  the length in. */
#define TABLEFIELD(s, f) .addr.offset = offsetof(s, f)
#define TABLEARRAY(a, t, n)                            \
    .addr.array.type = table_t,                        \
    .addr.array.arr.tables.subtype = t,                \
    .addr.array.arr.tables.base = (char *) a,          \
    .addr.array.arr.tables.structsize = sizeof(a[0]),  \
    .addr.array.count = n,                             \
    .addr.array.maxlen = sizeof(a)/sizeof(a[0])


#endif /* _MICRO_TOML_H_ */
