/*
  Parse TOML file shaped like:

  device = "/dev/spidev0.0"
  count = 4
  flag = true
  speed = 3.76
*/

#include "mtoml.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


static char device[16];
static int count;
static bool flag;
static double speed;

static const struct toml_keys_t toml_keys[] = {
    {"device", string_t, .addr.string = device, .len = sizeof(device)},
    {"count", integer_t, .addr.integer = &count},
    {"flag", boolean_t, .addr.boolean = &flag},
    {"speed", real_t, .addr.real = &speed},
    {NULL}
};


int main(int argc, char *argv[])
{
    FILE *fp;
    int status = 0;

    /* Usage: test_mtoml file */
    if (argc != 2) {
        status = toml_load(stdin, toml_keys);
    } else {
        if ((fp = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "can't open file %s\n", argv[1]);
        } else {
            status = toml_load(fp, toml_keys);
            fclose(fp);
        }
    }
    if (status == -1)
        fputs("toml_load failed\n", stderr);
    printf("device = %s, count = %d, flag = %d, speed = %f\n",
	   device, count, flag, speed);
    return EXIT_SUCCESS;
}
