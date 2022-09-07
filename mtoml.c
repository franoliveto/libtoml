/*
  US-ASCII characters

  # this is a comment\n
  key0 = value0\n
  key1 = value1\n
  key2 = value2 # this is a comment\n
  ...
  keyN = valueN(EOF)
*/

#include "mtoml.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>


/* Token types */
enum {
      LBRACKET, /* '[' */
      RBRACKET, /* ']' */
      LBRACE,   /* '{' */
      RBRACE,   /* '}' */
      EQUAL,    /* '=' */
      COMMA,    /* ',' */
      DOT,      /* '.' */
      WORD,     /* [0-9a-zA-Z+-_.] */
      STRING    /* "WORD" */
};


/* Scans for WORD/STRING tokens that contain any of [0-9][a-z][A-Z][+-_."] */
static int scan_word(char *buf, size_t size, int c, FILE *fp)
{
    char *valp = buf;

    if (c == '"') { /* scan for a string */
        while ((c = getc(fp)) != EOF && c != '\n') {
            if (c == '"') {
                *valp = '\0';
                return STRING;
            }
            if (valp >= buf + size - 1) {
                fprintf(stderr, "String value too long.");
                return -1;
            }
            *valp++ = c;
        }
        fprintf(stderr, "Saw EOF or '\\n' before '\"'\n");
        return -1;
    }

    /* scan for integer, real, or boolean */
    *valp++ = c;
    while ((c = getc(fp)) != EOF) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (strchr("0123456789+-_.", c) != NULL)) {
            if (valp >= buf + size - 1) {
                fprintf(stderr, "Token value too long.");
                return -1;
            }
            *valp++ = c;
        } else {
            /* stop if c does not match any of [0-9a-zA-Z+-_.] */
            ungetc(c, fp);
            break;
        }
    }
    *valp = '\0';
    return WORD;
}

/* Scans for tokens. */
static int scan(char *buf, size_t size, FILE *fp)
{
    int c;

    buf[0] = '\0';
    while ((c = getc(fp)) != EOF) {
        if (isspace(c))
            continue;
        switch (c) {
        case '#': /* Ignore comments */
            while ((c = getc(fp)) != EOF && c != '\n') /* FIXME: endofline() */
                ;
            break;
        case '=':
            return EQUAL;
        case '[':
            return LBRACKET;
        case ']':
            return RBRACKET;
        case '{':
            return LBRACE;
        case '}':
            return RBRACE;
        case ',':
            return COMMA;
        case '.':
            return DOT;
        default: /* [0-9a-zA-Z+-_."] */
            return scan_word(buf, size, c, fp);
        }
    }
    return -1;
}

static char *target_address(const struct toml_key_t *cursor)
{
    char *addr = NULL;
    switch (cursor->type) {
    case string_t:
        addr = cursor->addr.string;
    case integer_t:
        addr = (char *)cursor->addr.integer;
        break;
    case uinteger_t:
        addr = (char *)cursor->addr.uinteger;
        break;
    case long_t:
        addr = (char *)cursor->addr.longint;
        break;
    case ulong_t:
        addr = (char *)cursor->addr.ulongint;
        break;
    case short_t:
        addr = (char *)cursor->addr.shortint;
        break;
    case ushort_t:
        addr = (char *)cursor->addr.ushortint;
        break;
    case boolean_t:
        addr = (char *)cursor->addr.boolean;
        break;
    case real_t:
        addr = (char *)cursor->addr.real;
        break;
    default:
        ;
    }
    return addr;
}

int toml_load(FILE *fp, const struct toml_key_t *keys)
{
    char tokbuf[128];
    int tok;
    const struct toml_key_t *cursor;
    const struct toml_key_t *curtab = keys;
    size_t maxlen = 0;
    char *valp;

    while ((tok = scan(tokbuf, sizeof(tokbuf), fp)) != -1) {
        switch (tok) {
        /* TODO: [ x.y.z ] or [[ x.y.z ]] */
        case LBRACKET: /* [ x ] */
            tok = scan(tokbuf, sizeof(tokbuf), fp);
            if (tok != WORD) {
                fprintf(stderr, "Invalid syntax\n");
                return -1;
            }

            fprintf(stdout, "Collected table name '%s'\n", tokbuf);
            for (cursor = keys; cursor->key != NULL; cursor++) {
                if (strcmp(cursor->key, tokbuf) == 0)
                    break;
            }
            if (cursor->key == NULL) {
                fprintf(stderr, "Unknown table name '%s'\n", tokbuf);
                return -1;
            }
            if (cursor->type != table_t) {
                fprintf(stderr, "Saw simple value type when expecting "
                        "a table.\n");
                return -1;
            }
            tok = scan(tokbuf, sizeof(tokbuf), fp);
            if (tok != RBRACKET) {
                fprintf(stderr, "Missing ']'\n");
                return -1;
            }
            curtab = cursor->addr.keys;
            break;
        case WORD: /* key/value */
            fprintf(stdout, "Collected key name '%s'\n", tokbuf);
            for (cursor = curtab; cursor->key != NULL; cursor++) {
                if (strcmp(cursor->key, tokbuf) == 0)
                    break;
            }
            if (cursor->key == NULL) {
                fprintf(stderr, "Unknown key name '%s'\n", tokbuf);
                return -1;
            }
            if (cursor->type == string_t)
                maxlen = cursor->len;
            else
                maxlen = sizeof(tokbuf);

            tok = scan(tokbuf, sizeof(tokbuf), fp);
            if (tok != EQUAL) {
                fprintf(stderr, "Missing '='\n");
                return -1;
            }

            tok = scan(tokbuf, maxlen, fp);
            switch (tok) {
            case STRING: /* key = "value" */
            case WORD:
                fprintf(stdout, "Collected value '%s'\n", tokbuf);
                /* FIXME: validate types */
                if (tok == STRING && cursor->type != string_t) {
                    fprintf(stderr, "Saw quoted value when expecting "
                            "non-string.\n");
                    return -1;
                }
                if (tok != STRING && cursor->type == string_t) {
                    fprintf(stderr, "Didn't see quoted value when "
                            "expecting string.\n");
                    return -1;
                }

                if ((valp = target_address(cursor)) != NULL)
                    switch (cursor->type) {
                    case string_t:
                        {
                            size_t vl = strlen(tokbuf);
                            size_t cl = cursor->len - 1;
                            memset(valp, '\0', cl);
                            memcpy(valp, tokbuf, vl < cl ? vl : cl);
                        }
                        break;
                    case integer_t:
                        {
                            int tmp = atoi(tokbuf);
                            memcpy(valp, &tmp, sizeof(int));
                        }
                        break;
                    case uinteger_t:
                        {
                            unsigned int tmp = (unsigned int) atoi(tokbuf);
                            memcpy(valp, &tmp, sizeof(unsigned int));
                        }
                        break;
                    case short_t:
                        {
                            short tmp = (short) atoi(tokbuf);
                            memcpy(valp, &tmp, sizeof(short));
                        }
                        break;
                    case ushort_t:
                        {
                            unsigned short tmp = (unsigned short) atoi(tokbuf);
                            memcpy(valp, &tmp, sizeof(unsigned short));
                        }
                        break;
                    case long_t:
                        {
                            long tmp = atol(tokbuf);
                            memcpy(valp, &tmp, sizeof(long));
                        }
                        break;
                    case ulong_t:
                        {
                            unsigned long tmp = (unsigned long) atol(tokbuf);
                            memcpy(valp, &tmp, sizeof(unsigned long));
                        }
                        break;
                    case boolean_t:
                        {
                            bool tmp = (strcmp(tokbuf, "true") == 0);
                            memcpy(valp, &tmp, sizeof(bool));
                        }
                        break;
                    case real_t:
                        {
                            double tmp = atof(tokbuf);
                            memcpy(valp, &tmp, sizeof(double));
                        }
                        break;
                    case table_t:
                    default:
                        ;
                    }
                break;
            case LBRACKET: /* key = [ ] */
                break;
            case LBRACE: /* key = { } */
                break;
            default:
                fprintf(stderr, "Invalid syntax\n");
                return -1;
            }
            break;
        default:
            fprintf(stderr, "Invalid syntax\n");
            return -1;
        }
    }
    return 0;
}

