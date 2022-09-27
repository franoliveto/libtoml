#ifndef _MICRO_TOML_H_
#define _MICRO_TOML_H_

#include <stdio.h>
#include <stdbool.h>


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
    } addr;
    size_t len;
};

int toml_load(FILE *fp, const struct toml_key_t *keys);

/* Use the following macro to declare template initializers
   for arrays of strings. Writing the equivalentes out by
   hand is error-prone.

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



#endif /* _MICRO_TOML_H_ */
