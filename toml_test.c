#include "toml.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// static void assert_real(const char *key, double want, double got)
// {
//   if (want != got) {
//     printf("'%s' expecting '%f', got '%f'.\n", key, want, got);
//     exit(EXIT_FAILURE);
//   }
// }

// static void assert_boolean(const char *key, bool want, bool got)
// {
//   if (want != got) {
//     printf("'%s' expecting '%s', got '%s'.\n", key, want ? "true" : "false",
//            got ? "true" : "false");
//     exit(EXIT_FAILURE);
//   }
// }

static void assert_signed_integer(const char *key, long int want, long int got)
{
  if (want != got) {
    printf("'%s' expecting '%ld', got '%ld'.\n", key, want, got);
    exit(EXIT_FAILURE);
  }
}

static void assert_unsigned_integer(const char *key, unsigned long int want,
                                    unsigned long int got)
{
  if (want != got) {
    printf("'%s' expecting '%lu', got '%lu'.\n", key, want, got);
    exit(EXIT_FAILURE);
  }
}

// static void assert_string(const char *key, const char *want, const char *got)
// {
//   if (strcmp(got, want)) {
//     printf("fail: '%s' expecting '%s', got '%s'.\n", key, want, got);
//     exit(EXIT_FAILURE);
//   }
// }

// int test_tables(FILE *fp)
// {
//   struct toml {
//     char type[8];
//     char device[16];
//     bool lorawan_public;
//     int clksrc;
//     struct {
//       bool enable;
//       char type[8];
//       int freq;
//       double rssi_offset;
//     } table0;
//     struct {
//       bool enable;
//       unsigned short radio;
//       int if_freq;
//     } table1;
//   } toml;
//   int err;

//   const struct toml_key_t table0[] = {
//       {"enable", boolean_t, .ptr.boolean = &toml.table0.enable},
//       {"type", string_t, .ptr.string = toml.table0.type,
//        .len = sizeof(toml.table0.type)},
//       {"freq", toml_int_t, .ptr.integer = &toml.table0.freq},
//       {"rssi_offset", real_t, .ptr.real = &toml.table0.rssi_offset},
//       {NULL}};
//   const struct toml_key_t table1[] = {
//       {"enable", boolean_t, .ptr.boolean = &toml.table1.enable},
//       {"radio", ushort_t, .ptr.ushortint = &toml.table1.radio},
//       {"if", toml_int_t, .ptr.integer = &toml.table1.if_freq},
//       {NULL}};
//   const struct toml_key_t root[] = {
//       {"type", string_t, .ptr.string = toml.type, .len = sizeof(toml.type)},
//       {"device", string_t, .ptr.string = toml.device,
//        .len = sizeof(toml.device)},
//       {"clksrc", toml_int_t, .ptr.integer = &toml.clksrc},
//       {"lorawan_public", boolean_t, .ptr.boolean = &toml.lorawan_public},
//       {"table-0", table_t, .ptr.keys = table0},
//       {"table-1", table_t, .ptr.keys = table1},
//       {NULL}};

//   err = toml_unmarshal(fp, root);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   assert_string("type", "SPI", toml.type);
//   assert_string("device", "/dev/spidev0.0", toml.device);
//   assert_signed_integer("clksrc", 0, toml.clksrc);
//   assert_boolean("lorawan_public", true, toml.lorawan_public);

//   assert_boolean("table-0.enable", true, toml.table0.enable);
//   assert_string("table-0.type", "SX1250", toml.table0.type);
//   assert_signed_integer("table-0.freq", 917200000, toml.table0.freq);
//   assert_real("table-0.rssi_offset", -215.4, toml.table0.rssi_offset);

//   assert_boolean("table-1.enable", true, toml.table1.enable);
//   assert_unsigned_integer("table-1.radio", 0, toml.table1.radio);
//   assert_signed_integer("table-1.if", -200000, toml.table1.if_freq);
//   return 0;
// }

// int test_inline_tables(FILE *fp)
// {
//   char first[32], last[32];
//   int x, y;
//   int err;
//   const struct toml_key_t name_keys[] = {
//       {"first", string_t, .ptr.string = first, .len = sizeof(first)},
//       {"last", string_t, .ptr.string = last, .len = sizeof(last)},
//       {NULL}};
//   const struct toml_key_t point_keys[] = {{"x", toml_int_t, .ptr.integer =
//   &x},
//                                           {"y", toml_int_t, .ptr.integer =
//                                           &y}, {NULL}};
//   const struct toml_key_t math_keys[] = {
//       {"point", table_t, .ptr.keys = point_keys}, {NULL}};
//   const struct toml_key_t root[] = {{"name", table_t, .ptr.keys = name_keys},
//                                     {"math", table_t, .ptr.keys = math_keys},
//                                     {NULL}};

//   err = toml_unmarshal(fp, root);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   assert_string("name.first", "Ethan", first);
//   assert_string("name.last", "Hawke", last);
//   assert_signed_integer("math.point.x", 1, x);
//   assert_signed_integer("math.point.y", 2, y);
//   return 0;
// }

// int test_array_integers(FILE *fp)
// {
//   int err;
//   int integers1[3], integers2[2], integers3[3];
//   int count1, count2, count3;
//   const struct toml_key_t keys[] = {
//       {"integers1", array_t, .ptr.array.type = toml_int_t,
//        .ptr.array.arr.integers = integers1, .ptr.array.count = &count1,
//        .ptr.array.maxlen = sizeof(integers1) / sizeof(integers1[0])},
//       {"integers2", array_t, .ptr.array.type = toml_int_t,
//        .ptr.array.arr.integers = integers2, .ptr.array.count = &count2,
//        .ptr.array.maxlen = sizeof(integers2) / sizeof(integers2[0])},
//       {"integers3", array_t, .ptr.array.type = toml_int_t,
//        .ptr.array.arr.integers = integers3, .ptr.array.count = &count3,
//        .ptr.array.maxlen = sizeof(integers3) / sizeof(integers3[0])},
//       {NULL}};

//   err = toml_unmarshal(fp, keys);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   assert_signed_integer("count1", 3, count1);
//   assert_signed_integer("integers1[0]", 23, integers1[0]);
//   assert_signed_integer("integers1[1]", -12, integers1[1]);
//   assert_signed_integer("integers1[2]", 92, integers1[2]);

//   assert_signed_integer("count2", 2, count2);
//   assert_signed_integer("integers2[0]", 3, integers2[0]);
//   assert_signed_integer("integers2[1]", 18, integers2[1]);

//   assert_signed_integer("count3", 0, count3);
//   return 0;
// }

// int test_array_reals(FILE *fp)
// {
//   int err;
//   double reals1[3], reals2[3], reals3[3];
//   int count1, count2, count3;
//   const struct toml_key_t keys[] = {
//       {"reals1", array_t, .ptr.array.type = real_t,
//        .ptr.array.arr.reals = reals1, .ptr.array.count = &count1,
//        .ptr.array.maxlen = sizeof(reals1) / sizeof(reals1[0])},
//       {"reals2", array_t, .ptr.array.type = real_t,
//        .ptr.array.arr.reals = reals2, .ptr.array.count = &count2,
//        .ptr.array.maxlen = sizeof(reals2) / sizeof(reals2[0])},
//       {"reals3", array_t, .ptr.array.type = real_t,
//        .ptr.array.arr.reals = reals3, .ptr.array.count = &count3,
//        .ptr.array.maxlen = sizeof(reals3) / sizeof(reals3[0])},
//       {NULL}};

//   err = toml_unmarshal(fp, keys);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   assert_signed_integer("count1", 0, count1);

//   assert_signed_integer("count2", 3, count2);
//   assert_real("reals2[0]", 23.112, reals2[0]);
//   assert_real("reals2[1]", -8.32, reals2[1]);
//   assert_real("reals2[2]", 0.72, reals2[2]);

//   assert_signed_integer("count3", 3, count3);
//   assert_real("reals3[0]", 3.1, reals3[0]);
//   assert_real("reals3[1]", -21.0, reals3[1]);
//   assert_real("reals3[2]", -0.7, reals3[2]);
//   return 0;
// }

// int test_array_booleans(FILE *fp)
// {
//   int err;
//   bool booleans1[6], booleans2[2], booleans3[3];
//   int count1, count2, count3;
//   const struct toml_key_t keys[] = {
//       {"booleans1", array_t, .ptr.array.type = boolean_t,
//        .ptr.array.arr.booleans = booleans1, .ptr.array.count = &count1,
//        .ptr.array.maxlen = sizeof(booleans1) / sizeof(booleans1[0])},
//       {"booleans2", array_t, .ptr.array.type = boolean_t,
//        .ptr.array.arr.booleans = booleans2, .ptr.array.count = &count2,
//        .ptr.array.maxlen = sizeof(booleans2) / sizeof(booleans2[0])},
//       {"booleans3", array_t, .ptr.array.type = boolean_t,
//        .ptr.array.arr.booleans = booleans3, .ptr.array.count = &count3,
//        .ptr.array.maxlen = sizeof(booleans3) / sizeof(booleans3[0])},
//       {NULL}};

//   err = toml_unmarshal(fp, keys);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   assert_signed_integer("count1", 6, count1);
//   assert_boolean("booleans1[0]", true, booleans1[0]);
//   assert_boolean("booleans1[1]", false, booleans1[1]);
//   assert_boolean("booleans1[2]", false, booleans1[2]);
//   assert_boolean("booleans1[3]", true, booleans1[3]);
//   assert_boolean("booleans1[4]", false, booleans1[4]);
//   assert_boolean("booleans1[5]", true, booleans1[5]);

//   assert_boolean("count2", 2, count2);
//   assert_boolean("booleans2[0]", false, booleans2[0]);
//   assert_boolean("booleans2[1]", true, booleans2[1]);

//   assert_boolean("count3", 0, count3);
//   return 0;
// }

// int test_array_strings(FILE *fp)
// {
//   int err;
//   char *strings1[3];
//   char strings1store[64];
//   int count1;
//   char *strings2[3];
//   char strings2store[64];
//   int count2;
//   char *strings3[3];
//   char strings3store[2];
//   int count3;
//   const struct toml_key_t keys[] = {
//       {"strings1", array_t, STRINGARRAY(strings1, strings1store, &count1)},
//       {"strings2", array_t, STRINGARRAY(strings2, strings2store, &count2)},
//       {"strings3", array_t, STRINGARRAY(strings3, strings3store, &count3)},
//       {NULL}};

//   err = toml_unmarshal(fp, keys);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   assert_signed_integer("count1", 3, count1);
//   assert_string("strings1[0]", "one", strings1[0]);
//   assert_string("strings1[1]", "two", strings1[1]);
//   assert_string("strings1[2]", "three", strings1[2]);

//   assert_signed_integer("count2", 3, count2);
//   assert_string("strings2[0]", "four", strings2[0]);
//   assert_string("strings2[1]", "five", strings2[1]);
//   assert_string("strings2[2]", "thisisalongstring", strings2[2]);

//   assert_signed_integer("count3", 0, count3);
//   return 0;
// }

// int test_array_inline_tables(FILE *fp)
// {
//   struct point {
//     int x, y, z;
//   };
//   struct point points[4];
//   int count;
//   const struct toml_key_t point_keys[] = {
//       {"x", toml_int_t, TABLEFIELD(struct point, x)},
//       {"y", toml_int_t, TABLEFIELD(struct point, y)},
//       {"z", toml_int_t, TABLEFIELD(struct point, z)},
//       {NULL}};
//   const struct toml_key_t root[] = {
//       {"points", array_t, TABLEARRAY(points, point_keys, &count)}, {NULL}};
//   int err;

//   err = toml_unmarshal(fp, root);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   struct point want[] = {{1, 3, 2}, {5, -2, 4}, {2, 1, 3}, {-4, 7, -1}};
//   assert_signed_integer("count", 4, count);
//   for (int i = 0; i < 4; i++) {
//     char buf[16];

//     snprintf(buf, sizeof(buf), "points[%d].x", i);
//     assert_signed_integer(buf, want[i].x, points[i].x);

//     snprintf(buf, sizeof(buf), "points[%d].y", i);
//     assert_signed_integer(buf, want[i].y, points[i].y);

//     snprintf(buf, sizeof(buf), "points[%d].z", i);
//     assert_signed_integer(buf, want[i].z, points[i].z);
//   }
//   return 0;
// }

// int test_array_tables(FILE *fp)
// {
//   enum { NCHANNELS = 8 };
//   struct channel {
//     bool enable;
//     int radio;
//     int if_freq;
//   };
//   struct channel channels[NCHANNELS];
//   int count;
//   const struct toml_key_t chantab[] = {
//       {"enable", boolean_t, .ptr.offset = offsetof(struct channel, enable)},
//       {"radio", toml_int_t, .ptr.offset = offsetof(struct channel, radio)},
//       {"if", toml_int_t, .ptr.offset = offsetof(struct channel, if_freq)},
//       {NULL}};
//   const struct toml_key_t root[] = {
//       {"channels", array_t, .ptr.array.type = table_t,
//        .ptr.array.arr.tables.subtype = chantab,
//        .ptr.array.arr.tables.base = (char *) channels,
//        .ptr.array.arr.tables.structsize = sizeof(channels[0]),
//        .ptr.array.count = &count,
//        .ptr.array.maxlen = sizeof(channels) / sizeof(channels[0])},
//       {NULL}};
//   int err;

//   err = toml_unmarshal(fp, root);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   struct channel want[] = {{true, 0, -400000},  {true, 0, -200000},
//                            {false, 0, 0},       {true, 0, 200000},
//                            {false, 1, -300000}, {true, 1, -100000},
//                            {true, 1, 100000},   {false, 1, 300000}};

//   assert_signed_integer("count", NCHANNELS, count);

//   for (int i = 0; i < NCHANNELS; i++) {
//     char buf[32];

//     snprintf(buf, sizeof(buf), "channels[%d].enable", i);
//     assert_boolean(buf, want[i].enable, channels[i].enable);

//     snprintf(buf, sizeof(buf), "channels[%d].radio", i);
//     assert_signed_integer(buf, want[i].radio, channels[i].radio);

//     snprintf(buf, sizeof(buf), "channels[%d].if", i);
//     assert_signed_integer(buf, want[i].if_freq, channels[i].if_freq);
//   }
//   return 0;
// }

// int test_array_tables_2(FILE *fp)
// {
//   struct product {
//     long sku;
//     char name[16];
//     char color[16];
//   };
//   struct product products[3] = {0};
//   int count;
//   bool enable;
//   int radio, if_freq;
//   const struct toml_key_t chantab[] = {
//       {"enable", boolean_t, .ptr.boolean = &enable},
//       {"radio", toml_int_t, .ptr.integer = &radio},
//       {"if", toml_int_t, .ptr.integer = &if_freq},
//       {NULL}};
//   const struct toml_key_t prodtab[] = {
//       {"name", string_t, TABLEFIELD(struct product, name),
//        .len = sizeof(products[0].name)},
//       {"sku", long_t, TABLEFIELD(struct product, sku)},
//       {"color", string_t, TABLEFIELD(struct product, color),
//        .len = sizeof(products[0].color)},
//       {NULL}};
//   const struct toml_key_t root[] = {
//       {"products", array_t, TABLEARRAY(products, prodtab, &count)},
//       {"channel", table_t, .ptr.keys = chantab},
//       {NULL}};
//   int err;

//   err = toml_unmarshal(fp, root);
//   if (err != 0) {
//     printf("toml_unmarshal failed: %s\n", toml_strerror(err));
//     return -1;
//   }

//   struct product want[] = {
//       {738594937, "Hammer", ""}, {0, "", ""}, {284758393, "Nail", "gray"}};

//   assert_signed_integer("count", 3, count);

//   for (int i = 0; i < 3; i++) {
//     char buf[32];

//     snprintf(buf, sizeof(buf), "products[%d].name", i);
//     assert_string(buf, want[i].name, products[i].name);

//     snprintf(buf, sizeof(buf), "products[%d].sku", i);
//     assert_signed_integer(buf, want[i].sku, products[i].sku);

//     snprintf(buf, sizeof(buf), "products[%d].color", i);
//     assert_string(buf, want[i].color, products[i].color);
//   }
//   assert_boolean("channel.enable", true, enable);
//   assert_signed_integer("channel.radio", 0, radio);
//   assert_signed_integer("channel.if", -400000, if_freq);
//   return 0;
// }

void integers_test(FILE *f)
{
  short int1;
  unsigned short int2;
  unsigned int int3;
  int int4, int9, int10;
  unsigned int int5;
  long int6, int7, min, max;
  unsigned long int8;
  const struct toml_key template[] = {
      {"int1", toml_short_t, .u.integer.s = &int1},
      {"int2", toml_ushort_t, .u.integer.us = &int2},
      {"int3", toml_uint_t, .u.integer.ui = &int3},
      {"int4", toml_int_t, .u.integer.i = &int4},
      {"int5", toml_uint_t, .u.integer.ui = &int5},
      {"int6", toml_long_t, .u.integer.l = &int6},
      {"int7", toml_long_t, .u.integer.l = &int7},
      {"int8", toml_ulong_t, .u.integer.ul = &int8},
      {"int9", toml_int_t, .u.integer.i = &int9},
      {"int10", toml_int_t, .u.integer.i = &int10},
      {"max", toml_long_t, .u.integer.l = &max},
      {"min", toml_long_t, .u.integer.l = &min},
      {NULL}};
  int errnum;

  errnum = toml_unmarshal(f, template);
  assert_signed_integer("errnum", 0, errnum);

  assert_signed_integer("int1", 99, int1);
  assert_unsigned_integer("int2", 42, int2);
  assert_unsigned_integer("int3", 0, int3);
  assert_signed_integer("int4", -17, int4);
  assert_unsigned_integer("int5", 1000, int5);
  assert_signed_integer("int6", 5349221, int6);
  assert_signed_integer("int7", -5349221, int7);
  assert_unsigned_integer("int8", 12345, int8);
  assert_signed_integer("int9", 0, int9);
  assert_signed_integer("int10", 0, int10);
  assert_signed_integer("max", LONG_MAX, max);
  assert_signed_integer("min", LONG_MIN, min);
}

const struct test {
  char *name;
  void (*func)(FILE *);
} tests[] = {{"integers", integers_test},
             /* {"tables", test_tables}, */
             /* {"array_integers", test_array_integers}, */
             /* {"array_reals", test_array_reals}, */
             /* {"array_booleans", test_array_booleans}, */
             /* {"array_strings", test_array_strings}, */
             /* {"inline_tables", test_inline_tables}, */
             /* {"array_inline_tables", test_array_inline_tables}, */
             /* {"array_tables", test_array_tables}, */
             /* {"array_tables_2", test_array_tables_2}, */
             {NULL}};

int main()
{
  FILE *f;
  char filename[64];
  const struct test *test;

  for (test = tests; test->name != NULL; test++) {
    snprintf(filename, sizeof(filename), "tests/%s.toml", test->name);
    f = fopen(filename, "r");
    if (f == NULL) {
      fprintf(stderr, "can't open file %s\n", filename);
      return 1;
    }
    printf("TEST %s: ", test->name);
    test->func(f);
    puts("ok");
    fclose(f);
  }
  return 0;
}
