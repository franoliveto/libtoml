#include <stdio.h>
#include <stdlib.h>

#include "toml.h"

int age;
double pi;
// char *names[4];
// char names_store[30];
// int names_count;
// int slots[6], nslots;
char sentence[64];

const struct toml_key template[] = {
    {"Age", toml_int_t, .u.integer.i = &age},
    // {"Names", toml_array_t,
    //  toml_array_strings(names, names_store, &names_count)},
    {"Pi", toml_float_t, .u.real = &pi},
    // {"Slots", toml_array_t, .u.array.type = toml_int_t,
    //  .u.array.u.integer.i = slots, .u.array.count = &nslots,
    //  .u.array.len = toml_len(slots)},
    {"Sentence", toml_string_t, .u.string = sentence, .size = sizeof(sentence)},
    {NULL},
};

int main()
{
  // const char *filename = "example.toml";
  // FILE *f;
  // int errnum;

  // f = fopen(filename, "r");
  // if (f == NULL) {
  //   fprintf(stderr, "can't open file %s\n", filename);
  //   return 1;
  // }
  // errnum = toml_unmarshal(f, template);
  // if (errnum != 0) {
  //   fprintf(stderr, "toml_unmarshal failed: %s\n",
  //   toml_strerror(errnum)); return 1;
  // }
  // fclose(f);

  // printf("His sister is %d years old\n", age);
  // printf("The constant pi is approximately equal to %.2f\n",
  // pi); printf("Slots: {"); for (int i = 0; i < nslots; i++) {
  //   if (i != 0) {
  //     fputs(", ", stdout);
  //   }
  //   printf("%d", slots[i]);
  // }
  // puts("}");

  // printf("The Beatles are ");
  // for (int i = 0; i < count; i++) {
  //   if (i != 0) {
  //     fputs(", ", stdout);
  //   }
  //   printf("%s", names[i]);
  // }
  // putchar('\n');
  // puts(sentence);

  return 0;
}