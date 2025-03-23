#include "toml.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void assert_real(const char *key, double want, double got)
{
  if (want != got) {
    printf("'%s' expecting '%f', got '%f'.\n", key, want, got);
    exit(EXIT_FAILURE);
  }
}

static void assert_boolean(const char *key, bool want, bool got)
{
  if (want != got) {
    printf("'%s' expecting '%s', got '%s'.\n", key, want ? "true" : "false",
           got ? "true" : "false");
    exit(EXIT_FAILURE);
  }
}

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

static void assert_string(const char *key, const char *want, const char *got)
{
  if (strcmp(got, want)) {
    printf("fail: '%s' expecting '%s', got '%s'.\n", key, want, got);
    exit(EXIT_FAILURE);
  }
}

void tables_test()
{
  const char *text =
      "type = \"SPI\"\n"
      "device = \"/dev/spidev0.0\"\n"
      "lorawan_public = true\n"
      "clksrc = 0\n"
      "[table-0]\n"
      "enable = true\n"
      "type = \"SX1250\"\n"
      "freq = 917200000  # frequency in Hz.\n"
      "rssi_offset = -215.4\n"
      "[table-1]\n"
      "enable = true\n"
      "radio = 0\n"
      "if = -200000";

  struct config {
    char type[8];
    char device[16];
    bool lorawan_public;
    int clksrc;
    struct {
      bool enable;
      char type[8];
      long freq;
      double rssi_offset;
    } table0;
    struct {
      bool enable;
      unsigned short radio;
      int if_freq;
    } table1;
  } config;

  const struct toml_key table0[] = {
      {"enable", toml_bool_t, .u.boolean = &config.table0.enable},
      {"type", toml_string_t, .u.string = config.table0.type,
       .size = sizeof(config.table0.type)},
      {"freq", toml_long_t, .u.integer.l = &config.table0.freq},
      {"rssi_offset", toml_float_t, .u.real = &config.table0.rssi_offset},
      {NULL}};
  const struct toml_key table1[] = {
      {"enable", toml_bool_t, .u.boolean = &config.table1.enable},
      {"radio", toml_ushort_t, .u.integer.us = &config.table1.radio},
      {"if", toml_int_t, .u.integer.i = &config.table1.if_freq},
      {NULL}};
  const struct toml_key template[] = {
      {"type", toml_string_t, .u.string = config.type,
       .size = sizeof(config.type)},
      {"device", toml_string_t, .u.string = config.device,
       .size = sizeof(config.device)},
      {"clksrc", toml_int_t, .u.integer.i = &config.clksrc},
      {"lorawan_public", toml_bool_t, .u.boolean = &config.lorawan_public},
      {"table-0", toml_table_t, .u.table = table0},
      {"table-1", toml_table_t, .u.table = table1},
      {NULL}};

  if (toml_unmarshal(text, template) == -1) {
    fprintf(stderr, "toml_unmarshal failed: %s\n", toml_strerror());
    exit(1);
  }

  assert_string("type", "SPI", config.type);
  assert_string("device", "/dev/spidev0.0", config.device);
  assert_signed_integer("clksrc", 0, config.clksrc);
  assert_boolean("lorawan_public", true, config.lorawan_public);

  assert_boolean("table-0.enable", true, config.table0.enable);
  assert_string("table-0.type", "SX1250", config.table0.type);
  assert_signed_integer("table-0.freq", 917200000, config.table0.freq);
  assert_real("table-0.rssi_offset", -215.4, config.table0.rssi_offset);

  assert_boolean("table-1.enable", true, config.table1.enable);
  assert_unsigned_integer("table-1.radio", 0, config.table1.radio);
  assert_signed_integer("table-1.if", -200000, config.table1.if_freq);
}

void inline_tables_test()
{
  const char *text =
      "# The following inline table is identical to:\n"
      "# [name]\n"
      "# first = \"Ethan\"\n"
      "# last = \"Hawke\"\n"
      "name = { first = \"Ethan\", last = \"Hawke\" }\n"
      "point = { x = 1, y = 2 }";

  char first[32], last[32];
  struct {
    int x, y;
  } point;

  const struct toml_key name_template[] = {
      {"first", toml_string_t, .u.string = first, .size = sizeof(first)},
      {"last", toml_string_t, .u.string = last, .size = sizeof(last)},
      {NULL}};
  const struct toml_key point_template[] = {
      {"x", toml_int_t, .u.integer.i = &point.x},
      {"y", toml_int_t, .u.integer.i = &point.y},
      {NULL}};
  const struct toml_key template[] = {
      {"name", toml_table_t, .u.table = name_template},
      {"point", toml_table_t, .u.table = point_template},
      {NULL}};

  if (toml_unmarshal(text, template) == -1) {
    printf("toml_unmarshal failed: %s\n", toml_strerror());
    exit(1);
  }

  assert_string("name.first", "Ethan", first);
  assert_string("name.last", "Hawke", last);
  assert_signed_integer("point.x", 1, point.x);
  assert_signed_integer("point.y", 2, point.y);
}

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

void array_tables_test()
{
  const char *text =
      "[[channels]]\n"
      "enable = true\n"
      "radio = 0\n"
      "if = -400000\n"
      "[[channels]]\n"
      "enable = true\n"
      "radio = 0\n"
      "if = -200000\n"
      "[[channels]]\n"
      "enable = false\n"
      "radio = 0\n"
      "if = 0\n"
      "[[channels]]\n"
      "enable = true\n"
      "radio = 0\n"
      "if = 200000\n"
      "[[channels]]\n"
      "enable = false\n"
      "radio = 1\n"
      "if = -300000\n"
      "[[channels]]\n"
      "enable = true\n"
      "radio = 1\n"
      "if = -100000\n"
      "[[channels]]\n"
      "enable = true\n"
      "radio = 1\n"
      "if = 100000\n"
      "[[channels]]\n"
      "enable = false\n"
      "radio = 1\n"
      "if = 300000\n";

  struct channel {
    bool enable;
    int radio;
    int if_freq;
  };
  struct channel channels[8];
  int count;
  const struct toml_key channels_templ[] = {
      {"enable", toml_bool_t, .u.offset = offsetof(struct channel, enable)},
      {"radio", toml_int_t, .u.offset = offsetof(struct channel, radio)},
      {"if", toml_int_t, .u.offset = offsetof(struct channel, if_freq)},
      {NULL}};
  const struct toml_key template[] = {
      {"channels", toml_array_t, .u.array.type = toml_table_t,
       .u.array.u.tables.subtype = channels_templ,
       .u.array.u.tables.base = (char *) channels,
       .u.array.u.tables.structsize = sizeof(channels[0]),
       .u.array.count = &count,
       .u.array.len = (sizeof(channels) / sizeof(channels[0]))},
      {NULL}};

  if (toml_unmarshal(text, template) == -1) {
    fprintf(stderr, "toml_unmarshal failed: %s\n", toml_strerror());
    exit(1);
  }

  struct channel want[] = {{true, 0, -400000},  {true, 0, -200000},
                           {false, 0, 0},       {true, 0, 200000},
                           {false, 1, -300000}, {true, 1, -100000},
                           {true, 1, 100000},   {false, 1, 300000}};

  assert_signed_integer("count", 8, count);

  for (int i = 0; i < 8; i++) {
    char buf[32];

    snprintf(buf, sizeof(buf), "channels[%d].enable", i);
    assert_boolean(buf, want[i].enable, channels[i].enable);

    snprintf(buf, sizeof(buf), "channels[%d].radio", i);
    assert_signed_integer(buf, want[i].radio, channels[i].radio);

    snprintf(buf, sizeof(buf), "channels[%d].if", i);
    assert_signed_integer(buf, want[i].if_freq, channels[i].if_freq);
  }
}

void table_array_tables_test()
{
  const char *text =
      "[channel]\n"
      "enable = true\n"
      "radio = 0\n"
      "if = -400000\n"

      "[[products]]\n"
      "name = \"Hammer\"\n"
      "sku = 738594937\n"

      "[[products]]  # empty table within the array\n"

      "[[products]]\n"
      "name = \"Nail\"\n"
      "sku = 284758393\n"
      "color = \"gray\"";

  struct product {
    long sku;
    char name[16];
    char color[16];
  };
  struct product products[3] = {0};
  int nprods;
  bool enable;
  int radio, if_freq;
  const struct toml_key channel_template[] = {
      {"enable", toml_bool_t, .u.boolean = &enable},
      {"radio", toml_int_t, .u.integer.i = &radio},
      {"if", toml_int_t, .u.integer.i = &if_freq},
      {NULL}};
  const struct toml_key products_template[] = {
      {"name", toml_string_t, toml_table_field(struct product, name),
       .size = sizeof(products[0].name)},
      {"sku", toml_long_t, toml_table_field(struct product, sku)},
      {"color", toml_string_t, toml_table_field(struct product, color),
       .size = sizeof(products[0].color)},
      {NULL}};
  const struct toml_key template[] = {
      {"products", toml_array_t,
       toml_array_tables(products, products_template, &nprods)},
      {"channel", toml_table_t, .u.table = channel_template},
      {NULL}};

  if (toml_unmarshal(text, template) == -1) {
    printf("toml_unmarshal failed: %s\n", toml_strerror());
    exit(1);
  }

  struct product want[] = {
      {738594937, "Hammer", ""}, {0, "", ""}, {284758393, "Nail", "gray"}};

  assert_signed_integer("nprods", 3, nprods);

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
}

void integers_test()
{
  const char *text =
      "int1 = +99\n"
      "int2 = 42\n"
      "int3 = 0\n"
      "int4 = -17\n"
      "int5 = 1_000\n"
      "int6 = 5_349_221\n"
      "int7 = -53_49_221  # Indian number system grouping\n"
      "int8 = 1_2_3_4_5  # VALID but discouraged\n"
      "int9 = +0\n"
      "int10 = -0\n"
      "max = 9223372036854775807\n"
      "min = -9223372036854775808\n";
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

  if (toml_unmarshal(text, template) == -1) {
    fprintf(stderr, "toml_unmarshal failed: %s\n", toml_strerror());
    exit(1);
  }

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

void floats_test()
{
  const char *text =
      "float1 = +1.0\n"
      "float2 = 3.1415\n"
      "float3 = -0.01\n"
      "float4 = 5e+22\n"
      "float5 = 1e06\n"
      "float6 = -2E-2\n"
      "float7 = 6.626e-34\n";

  double float1, float2, float3, float4, float5, float6, float7;

  const struct toml_key template[] = {
      {"float1", toml_float_t, .u.real = &float1},
      {"float2", toml_float_t, .u.real = &float2},
      {"float3", toml_float_t, .u.real = &float3},
      {"float4", toml_float_t, .u.real = &float4},
      {"float5", toml_float_t, .u.real = &float5},
      {"float6", toml_float_t, .u.real = &float6},
      {"float7", toml_float_t, .u.real = &float7},
      {NULL}};

  if (toml_unmarshal(text, template) == -1) {
    fprintf(stderr, "toml_unmarshal failed: %s\n", toml_strerror());
    exit(1);
  }

  assert_real("float1", 1.0, float1);
  assert_real("float2", 3.1415, float2);
  assert_real("float3", -0.01, float3);
  assert_real("float4", 5e+22, float4);
  assert_real("float5", 1e6, float5);
  assert_real("float6", -2E-2, float6);
  assert_real("float7", 6.626e-34, float7);
}

const struct test {
  char *name;
  void (*func)();
} tests[] = {{"integers", integers_test},
             {"floats", floats_test},
             {"tables", tables_test},
             /* {"array_booleans", test_array_booleans}, */
             /* {"array_strings", test_array_strings}, */
             {"inline_tables", inline_tables_test},
             /* {"array_inline_tables", test_array_inline_tables}, */
             {"array_tables", array_tables_test},
             {"table_array_tables", table_array_tables_test},
             {NULL}};

int main()
{
  const struct test *t;

  for (t = tests; t->name != NULL; t++) {
    fprintf(stderr, "TEST %s: ", t->name);
    t->func();
    fputs("ok\n", stderr);
  }
  return 0;
}
