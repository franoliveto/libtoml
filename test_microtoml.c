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

static void assert_signed_integer(const char *key, long int want, long int got)
{
    if (want != got) {
        printf("'%s' expecting '%ld', got '%ld'.\n",
               key, want, got);
        exit(EXIT_FAILURE);
    }
}

static void assert_unsigned_integer(const char *key,
                                    unsigned long int want,
                                    unsigned long int got)
{
    if (want != got) {
        printf("'%s' expecting '%lu', got '%lu'.\n",
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
    assert_signed_integer("count", 4, count);
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
            unsigned short radio;
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
        {"radio", ushort_t, .addr.ushortint = &toml.table1.radio},
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
    assert_signed_integer("clksrc", 0, toml.clksrc);
    assert_boolean("lorawan_public", true, toml.lorawan_public);

    assert_boolean("table-0.enable", true, toml.table0.enable);
    assert_string("table-0.type", "SX1250", toml.table0.type);
    assert_signed_integer("table-0.freq", 917200000, toml.table0.freq);
    assert_real("table-0.rssi_offset", -215.4, toml.table0.rssi_offset);

    assert_boolean("table-1.enable", true, toml.table1.enable);
    assert_unsigned_integer("table-1.radio", 0, toml.table1.radio);
    assert_signed_integer("table-1.if", -200000, toml.table1.if_freq);
    return 0;
}

int test_inline_tables(FILE *fp)
{
    char first[32], last[32];
    int x, y;
    int status;
    const struct toml_key_t name_keys[] = {
        {"first", string_t, .addr.string = first, .len = sizeof(first)},
        {"last", string_t, .addr.string = last, .len = sizeof(last)},
        {NULL}
    };
    const struct toml_key_t point_keys[] = {
        {"x", integer_t, .addr.integer = &x},
        {"y", integer_t, .addr.integer = &y},
        {NULL}
    };
    const struct toml_key_t math_keys[] = {
        {"point", table_t, .addr.keys = point_keys},
        {NULL}
    };
    const struct toml_key_t root[] = {
        {"name", table_t, .addr.keys = name_keys},
        {"math", table_t, .addr.keys = math_keys},
        {NULL}
    };

    if ((status = toml_load(fp, root)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_string("name.first", "Ethan", first);
    assert_string("name.last", "Hawke", last);
    assert_signed_integer("math.point.x", 1, x);
    assert_signed_integer("math.point.y", 2, y);
    return 0;
}

int test_array_integers(FILE *fp)
{
    int status;
    int integers1[3], integers2[2], integers3[3];
    int count1, count2, count3;
    const struct toml_key_t keys[] = {
        {"integers1", array_t,
         .addr.array.type = integer_t,
         .addr.array.arr.integers = integers1,
         .addr.array.count = &count1,
         .addr.array.maxlen = sizeof(integers1)/sizeof(integers1[0])},
        {"integers2", array_t,
         .addr.array.type = integer_t,
         .addr.array.arr.integers = integers2,
         .addr.array.count = &count2,
         .addr.array.maxlen = sizeof(integers2)/sizeof(integers2[0])},
        {"integers3", array_t,
         .addr.array.type = integer_t,
         .addr.array.arr.integers = integers3,
         .addr.array.count = &count3,
         .addr.array.maxlen = sizeof(integers3)/sizeof(integers3[0])},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_signed_integer("count1", 3, count1);
    assert_signed_integer("integers1[0]", 23, integers1[0]);
    assert_signed_integer("integers1[1]", -12, integers1[1]);
    assert_signed_integer("integers1[2]", 92, integers1[2]);

    assert_signed_integer("count2", 2, count2);
    assert_signed_integer("integers2[0]", 3, integers2[0]);
    assert_signed_integer("integers2[1]", 18, integers2[1]);

    assert_signed_integer("count3", 0, count3);
    return 0;
}

int test_array_reals(FILE *fp)
{
    int status;
    double reals1[3], reals2[3], reals3[3];
    int count1, count2, count3;
    const struct toml_key_t keys[] = {
        {"reals1", array_t,
         .addr.array.type = real_t,
         .addr.array.arr.reals = reals1,
         .addr.array.count = &count1,
         .addr.array.maxlen = sizeof(reals1)/sizeof(reals1[0])},
        {"reals2", array_t,
         .addr.array.type = real_t,
         .addr.array.arr.reals = reals2,
         .addr.array.count = &count2,
         .addr.array.maxlen = sizeof(reals2)/sizeof(reals2[0])},
        {"reals3", array_t,
         .addr.array.type = real_t,
         .addr.array.arr.reals = reals3,
         .addr.array.count = &count3,
         .addr.array.maxlen = sizeof(reals3)/sizeof(reals3[0])},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_signed_integer("count1", 0, count1);

    assert_signed_integer("count2", 3, count2);
    assert_real("reals2[0]", 23.112, reals2[0]);
    assert_real("reals2[1]", -8.32, reals2[1]);
    assert_real("reals2[2]", 0.72, reals2[2]);

    assert_signed_integer("count3", 3, count3);
    assert_real("reals3[0]", 3.1, reals3[0]);
    assert_real("reals3[1]", -21.0, reals3[1]);
    assert_real("reals3[2]", -0.7, reals3[2]);
    return 0;
}

int test_array_booleans(FILE *fp)
{
    int status;
    bool booleans1[6], booleans2[2], booleans3[3];
    int count1, count2, count3;
    const struct toml_key_t keys[] = {
        {"booleans1", array_t,
         .addr.array.type = boolean_t,
         .addr.array.arr.booleans = booleans1,
         .addr.array.count = &count1,
         .addr.array.maxlen = sizeof(booleans1)/sizeof(booleans1[0])},
        {"booleans2", array_t,
         .addr.array.type = boolean_t,
         .addr.array.arr.booleans = booleans2,
         .addr.array.count = &count2,
         .addr.array.maxlen = sizeof(booleans2)/sizeof(booleans2[0])},
        {"booleans3", array_t,
         .addr.array.type = boolean_t,
         .addr.array.arr.booleans = booleans3,
         .addr.array.count = &count3,
         .addr.array.maxlen = sizeof(booleans3)/sizeof(booleans3[0])},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_signed_integer("count1", 6, count1);
    assert_boolean("booleans1[0]", true, booleans1[0]);
    assert_boolean("booleans1[1]", false, booleans1[1]);
    assert_boolean("booleans1[2]", false, booleans1[2]);
    assert_boolean("booleans1[3]", true, booleans1[3]);
    assert_boolean("booleans1[4]", false, booleans1[4]);
    assert_boolean("booleans1[5]", true, booleans1[5]);

    assert_boolean("count2", 2, count2);
    assert_boolean("booleans2[0]", false, booleans2[0]);
    assert_boolean("booleans2[1]", true, booleans2[1]);

    assert_boolean("count3", 0, count3);
    return 0;
}

int test_array_strings(FILE *fp)
{
    int status;
    char *strings1[3];
    char strings1store[64];
    int count1;
    char *strings2[3];
    char strings2store[64];
    int count2;
    char *strings3[3];
    char strings3store[2];
    int count3;
    const struct toml_key_t keys[] = {
        {"strings1", array_t, STRINGARRAY(strings1, strings1store, &count1)},
        {"strings2", array_t, STRINGARRAY(strings2, strings2store, &count2)},
        {"strings3", array_t, STRINGARRAY(strings3, strings3store, &count3)},
        {NULL}
    };

    if ((status = toml_load(fp, keys)) == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }
    assert_signed_integer("count1", 3, count1);
    assert_string("strings1[0]", "one", strings1[0]);
    assert_string("strings1[1]", "two", strings1[1]);
    assert_string("strings1[2]", "three", strings1[2]);

    assert_signed_integer("count2", 3, count2);
    assert_string("strings2[0]", "four", strings2[0]);
    assert_string("strings2[1]", "five", strings2[1]);
    assert_string("strings2[2]", "thisisalongstring", strings2[2]);

    assert_signed_integer("count3", 0, count3);
    return 0;
}

int test_array_inline_tables(FILE *fp)
{
    struct point {
        int x, y, z;
    };
    struct point points[4];
    int count;
    const struct toml_key_t point_keys[] = {
        {"x", integer_t, TABLEFIELD(struct point, x)},
        {"y", integer_t, TABLEFIELD(struct point, y)},
        {"z", integer_t, TABLEFIELD(struct point, z)},
        {NULL}
    };
    const struct toml_key_t root[] = {
        {"points", array_t, TABLEARRAY(points, point_keys, &count)},
        {NULL}
    };
    int status;

    status = toml_load(fp, root);
    if (status == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }

    struct point want[] = {
        {1, 3, 2}, {5, -2, 4},
        {2, 1, 3}, {-4, 7, -1}
    };
    assert_signed_integer("count", 4, count);
    for (int i = 0; i < 4; i++) {
        char buf[16];

        snprintf(buf, sizeof(buf), "points[%d].x", i);
        assert_signed_integer(buf, want[i].x, points[i].x);

        snprintf(buf, sizeof(buf), "points[%d].y", i);
        assert_signed_integer(buf, want[i].y, points[i].y);

        snprintf(buf, sizeof(buf), "points[%d].z", i);
        assert_signed_integer(buf, want[i].z, points[i].z);
    }
    return 0;
}

int test_array_tables(FILE *fp)
{
    enum { NCHANNELS = 8 };
    struct channel {
        bool enable;
        int radio;
        int if_freq;
    };
    struct channel channels[NCHANNELS];
    int count;
    const struct toml_key_t chantab[] = {
        {"enable", boolean_t, .addr.offset = offsetof(struct channel, enable)},
        {"radio", integer_t, .addr.offset = offsetof(struct channel, radio)},
        {"if", integer_t, .addr.offset = offsetof(struct channel, if_freq)},
        {NULL}
    };
    const struct toml_key_t root[] = {
        {"channels", array_t,
         .addr.array.type = table_t,
         .addr.array.arr.tables.subtype = chantab,
         .addr.array.arr.tables.base = (char *) channels,
         .addr.array.arr.tables.structsize = sizeof(channels[0]),
         .addr.array.count = &count,
         .addr.array.maxlen = sizeof(channels)/sizeof(channels[0])},
        {NULL}
    };
    int status;

    status = toml_load(fp, root);
    if (status == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }

    struct channel want[] = {
        {true, 0, -400000}, {true, 0, -200000},
        {false, 0, 0}, {true, 0, 200000},
        {false, 1, -300000}, {true, 1, -100000},
        {true, 1, 100000}, {false, 1, 300000}
    };

    assert_signed_integer("count", NCHANNELS, count);

    for (int i = 0; i < NCHANNELS; i++) {
        char buf[32];

        snprintf(buf, sizeof(buf), "channels[%d].enable", i);
        assert_boolean(buf, want[i].enable, channels[i].enable);

        snprintf(buf, sizeof(buf), "channels[%d].radio", i);
        assert_signed_integer(buf, want[i].radio, channels[i].radio);

        snprintf(buf, sizeof(buf), "channels[%d].if", i);
        assert_signed_integer(buf, want[i].if_freq, channels[i].if_freq);
    }
    return 0;
}

int test_array_tables_2(FILE *fp)
{
    struct product {
        long sku;
        char name[16];
        char color[16];
    };
    struct product products[3] = {0};
    int count;
    bool enable;
    int radio, if_freq;
    const struct toml_key_t chantab[] = {
        {"enable", boolean_t, .addr.boolean = &enable},
        {"radio", integer_t, .addr.integer = &radio},
        {"if", integer_t, .addr.integer = &if_freq},
        {NULL}
    };
    const struct toml_key_t prodtab[] = {
        {"name", string_t, TABLEFIELD(struct product, name),
         .len = sizeof(products[0].name)},
        {"sku", long_t, TABLEFIELD(struct product, sku)},
        {"color", string_t, TABLEFIELD(struct product, color),
         .len = sizeof(products[0].color)},
        {NULL}
    };
    const struct toml_key_t root[] = {
        {"products", array_t, TABLEARRAY(products, prodtab, &count)},
        {"channel", table_t, .addr.keys = chantab},
        {NULL}
    };
    int status;

    status = toml_load(fp, root);
    if (status == -1) {
        printf("toml_load failed: %d\n", status);
        return -1;
    }

    struct product want[] = {
        {738594937, "Hammer", ""},
        {0, "", ""},
        {284758393, "Nail", "gray"}
    };

    assert_signed_integer("count", 3, count);

    for (int i = 0; i < 3; i++) {
        char buf[32];

        snprintf(buf, sizeof(buf), "products[%d].name", i);
        assert_string(buf, want[i].name, products[i].name);

        snprintf(buf, sizeof(buf), "products[%d].sku", i);
        assert_signed_integer(buf, want[i].sku, products[i].sku);

        snprintf(buf, sizeof(buf), "products[%d].color", i);
        assert_string(buf, want[i].color, products[i].color);
    }
    assert_boolean("channel.enable", true, enable);
    assert_signed_integer("channel.radio", 0, radio);
    assert_signed_integer("channel.if", -400000, if_freq);
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
    {"array_booleans", test_array_booleans},
    {"array_strings", test_array_strings},
    {"inline_tables", test_inline_tables},
    {"array_inline_tables", test_array_inline_tables},
    {"array_tables", test_array_tables},
    {"array_tables_2", test_array_tables_2},
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
