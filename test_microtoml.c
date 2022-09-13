#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "toml.h"


static void assert_real(const char *key, double want, double got)
{
    if (want != got) {
        printf("'%s' expecting '%f', got '%f'.\n",
               key, want, got);
        exit(EXIT_FAILURE);
    }
}

static void assert_boolean(const char *key, bool want, bool got)
{
    if (want != got) {
        printf("'%s' expecting '%s', got '%s'.\n",
               key, want ? "true" : "false", got ? "true" : "false");
        exit(EXIT_FAILURE);
    }
}

static void assert_integer(const char *key, int want, int got)
{
    if (want != got) {
        printf("'%s' expecting '%d', got '%d'.\n",
               key, want, got);
        exit(EXIT_FAILURE);
    }
}

static void assert_string(const char *key, const char *want, const char *got)
{
    if (strcmp(got, want)) {
        printf("fail: '%s' expecting '%s', got '%s'.\n",
               key, want, got);
        exit(EXIT_FAILURE);
    }
}

int test_values(FILE *fp)
{
    int status;
    char device[16];
    int count;
    bool flag;
    double speed;
    const struct toml_key_t keys[] = {
        {"device", string_t, .addr.string = device,
         .len = sizeof(device)},
        {"count", integer_t, .addr.integer = &count},
        {"flag", boolean_t, .addr.boolean = &flag},
        {"speed", real_t, .addr.real = &speed},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_string("device", "/dev/spidev0.0", device);
    assert_integer("count", 4, count);
    assert_boolean("flag", true, flag);
    assert_real("speed", 76.213, speed);
    return 0;
}

int test_tables(FILE *fp)
{
    struct toml {
        char type[8];
        char device[16];
        bool lorawan_public;
        int clksrc;
        struct {
            bool enable;
            char type[8];
            int freq;
            double rssi_offset;
        } table0;
        struct {
            bool enable;
            int radio;
            int if_freq;
        } table1;
    } toml;
    int status;

    const struct toml_key_t table0[] = {
        {"enable", boolean_t, .addr.boolean = &toml.table0.enable},
        {"type", string_t, .addr.string = toml.table0.type,
         .len = sizeof(toml.table0.type)},
        {"freq", integer_t, .addr.integer = &toml.table0.freq},
        {"rssi_offset", real_t, .addr.real = &toml.table0.rssi_offset},
        {NULL}
    };
    const struct toml_key_t table1[] = {
        {"enable", boolean_t, .addr.boolean = &toml.table1.enable},
        {"radio", integer_t, .addr.integer = &toml.table1.radio},
        {"if", integer_t, .addr.integer = &toml.table1.if_freq},
        {NULL}
    };
    const struct toml_key_t root[] = {
        {"type", string_t, .addr.string = toml.type,
         .len = sizeof(toml.type)},
        {"device", string_t, .addr.string = toml.device,
         .len = sizeof(toml.device)},
        {"clksrc", integer_t, .addr.integer = &toml.clksrc},
        {"lorawan_public", boolean_t, .addr.boolean = &toml.lorawan_public},
        {"table-0", table_t, .addr.keys = table0},
        {"table-1", table_t, .addr.keys = table1},
        {NULL}
    };

    if ((status = toml_load(fp, root)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_string("type", "SPI", toml.type);
    assert_string("device", "/dev/spidev0.0", toml.device);
    assert_integer("clksrc", 0, toml.clksrc);
    assert_boolean("lorawan_public", true, toml.lorawan_public);

    assert_boolean("table-0.enable", true, toml.table0.enable);
    assert_string("table-0.type", "SX1250", toml.table0.type);
    assert_integer("table-0.freq", 917200000, toml.table0.freq);
    assert_integer("table-0.rssi_offset", -215.4, toml.table0.rssi_offset);

    assert_boolean("table-1.enable", true, toml.table1.enable);
    assert_integer("table-1.radio", 0, toml.table1.radio);
    assert_integer("table-1.if", -200000, toml.table1.if_freq);
    return 0;
}

int test_array_integers(FILE *fp)
{
    int status;
    int integers1[3], integers2[2], integers3[3];
    int count1, count2, count3;
    const struct toml_array_t array1 = {
        .type = integer_t,
        .store.integers = integers1,
        .count = &count1,
        .maxlen = sizeof(integers1)/sizeof(integers1[0])
    };
    const struct toml_array_t array2 = {
        .type = integer_t,
        .store.integers = integers2,
        .count = &count2,
        .maxlen = sizeof(integers2)/sizeof(integers2[0])
    };
    const struct toml_array_t array3 = {
        .type = integer_t,
        .store.integers = integers3,
        .count = &count3,
        .maxlen = sizeof(integers3)/sizeof(integers3[0])
    };
    const struct toml_key_t keys[] = {
        {"integers1", array_t, .addr.array = array1},
        {"integers2", array_t, .addr.array = array2},
        {"integers3", array_t, .addr.array = array3},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_integer("count1", 3, count1);
    assert_integer("integers1[0]", 23, integers1[0]);
    assert_integer("integers1[1]", -12, integers1[1]);
    assert_integer("integers1[2]", 92, integers1[2]);

    assert_integer("count2", 2, count2);
    assert_integer("integers2[0]", 3, integers2[0]);
    assert_integer("integers2[1]", 18, integers2[1]);

    assert_integer("count3", 0, count3);
    return 0;
}

int test_array_reals(FILE *fp)
{
    int status;
    double reals1[3], reals2[3], reals3[3];
    int count1, count2, count3;
    const struct toml_array_t array1 = {
        .type = real_t,
        .store.reals = reals1,
        .count = &count1,
        .maxlen = sizeof(reals1)/sizeof(reals1[0])
    };
    const struct toml_array_t array2 = {
        .type = real_t,
        .store.reals = reals2,
        .count = &count2,
        .maxlen = sizeof(reals2)/sizeof(reals2[0])
    };
    const struct toml_array_t array3 = {
        .type = real_t,
        .store.reals = reals3,
        .count = &count3,
        .maxlen = sizeof(reals3)/sizeof(reals3[0])
    };
    const struct toml_key_t keys[] = {
        {"reals1", array_t, .addr.array = array1},
        {"reals2", array_t, .addr.array = array2},
        {"reals3", array_t, .addr.array = array3},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_integer("count1", 0, count1);

    assert_integer("count2", 3, count2);
    assert_real("reals2[0]", 23.112, reals2[0]);
    assert_real("reals2[1]", -8.32, reals2[1]);
    assert_real("reals2[2]", 0.72, reals2[2]);

    assert_integer("count3", 3, count3);
    assert_real("reals3[0]", 3.1, reals3[0]);
    assert_real("reals3[1]", -21.0, reals3[1]);
    assert_real("reals3[2]", -0.7, reals3[2]);
    return 0;
}


const struct test {
    char *name;
    int (*test)(FILE *);
} tests[] = {
    {"values", test_values},
    {"tables", test_tables},
    {"array_integers", test_array_integers},
    {"array_reals", test_array_reals},
    {NULL}
};

int main(int argc, char *argv[])
{
    FILE *fp;
    char file[64];
    const struct test *t;

    for (t = tests; t->name != NULL; t++) {
        snprintf(file, sizeof(file), "tests/%s.toml", t->name);
        if ((fp = fopen(file, "r")) == NULL) {
            printf("can't open file %s\n", file);
        } else {
            printf("TEST %s: ", t->name);
            if (t->test(fp) == -1) {
                return EXIT_FAILURE;
            }
            puts("ok");
            fclose(fp);
        }
    }
    return EXIT_SUCCESS;
}
