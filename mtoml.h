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
    real_t
};

struct toml_keys_t {
    char *key;
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
    } addr;
    size_t len;
};

int toml_load(FILE *fp, const struct toml_keys_t *keys);


#endif /* _MICRO_TOML_H_ */
