#include <stdio.h>
#include <stdlib.h>

#include "toml.h"

int age;
double pi;
char *names[4], store[30];
int nameslen;
int slots[6], slotslen;
char description[64];

const struct toml_key_t tab[] = {
    {"Age", int_t, .ptr.i = &age},
    {"Names", array_t, STRINGARRAY(names, store, &nameslen)},
    {"Pi", real_t, .ptr.r = &pi},
    {"Slots", array_t, .ptr.array.type = int_t, .ptr.array.base.i = slots,
     .ptr.array.len = &slotslen, .ptr.array.cap = toml_cap(slots)},
    {"Description", string_t, .ptr.string.s = description,
     .ptr.string.len = sizeof(description)},
    {NULL},
};

int main()
{
    const char *filename = "example.toml";
    FILE *fp;
    int err;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "can't open file %s\n", filename);
        return 1;
    }
    err = toml_unmarshal(fp, tab);
    if (err != 0) {
        fprintf(stderr, "toml_unmarshal failed: %s\n", toml_strerror(err));
        return 1;
    }
    fclose(fp);

    printf("age is %d\n", age);
    printf("pi is %.2f\n", pi);
    printf("Slots: ");
    for (int i = 0; i < slotslen; i++) {
        printf("%d ", slots[i]);
    }
    putchar('\n');

    printf("The Beatles are ");
    for (int i = 0; i < nameslen; i++) {
        if (i != 0) {
            fputs(", ", stdout);
        }
        printf("%s", names[i]);
    }
    putchar('\n');
    puts(description);
    return 0;
}