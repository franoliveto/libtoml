/*
  US-ASCII characters

  # this is a comment\n
  key0 = value0\n
  key1 = value1\n
  key2 = value2 # this is a comment\n
  ...
  keyN = valueN(EOF)
*/

#include "toml.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h> /* HUGE_VAL */


#ifdef DEBUG_ENABLE
#include <stdarg.h>
void trace(const char *fmt, ...)
{
    char buf[BUFSIZ];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fputs(buf, stderr);
}

#define debug_trace(...) trace(__VA_ARGS__)
#else
#define debug_trace(...) do {} while (0)
#endif

enum { TOKBUFSIZ = 512 };

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

static int load_inline_table(FILE *fp, const struct toml_key_t *keys);

/* Scans for WORD/STRING tokens that contain any of [0-9a-zA-Z+-_."] */
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
                debug_trace("String value too long.");
                return -1;
            }
            *valp++ = c;
        }
        debug_trace("Saw EOF or '\\n' before '\"'\n");
        return -1;
    }

    /* scan for integer, real, or boolean */
    *valp++ = c;
    while ((c = getc(fp)) != EOF) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (strchr("0123456789+-_.", c) != NULL)) {
            if (valp >= buf + size - 1) {
                debug_trace("Token value too long.");
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

/* Parses arrays of elements. */
static int load_array(FILE *fp, const struct toml_array_t *array)
{
    int tok, offset = 0;
    char tokbuf[TOKBUFSIZ];
    char *sp;

    sp = array->arr.strings.store;

    while ((tok = scan(tokbuf, sizeof(tokbuf), fp)) != -1) {
        if (tok == RBRACKET) {
            debug_trace("End of array found.\n");
            break;
        }
        switch (tok) {
        case STRING:
        case WORD:
            debug_trace("Collected value '%s'\n", tokbuf);
            if (offset >= array->maxlen) {
                debug_trace("Too many elements in array.\n");
                return -1;
            }
            switch (array->type) {
            case string_t:
                {
                    array->arr.strings.ptrs[offset] = sp;
                    size_t used = sp - array->arr.strings.store;
                    size_t free = array->arr.strings.storelen - used;
                    size_t len = strlen(tokbuf);
                    if (len+1 > free) {
                        debug_trace("Ran out of storage for strings.\n");
                        return -1;
                    }
                    memcpy(sp, tokbuf, len);
                    *(sp + len) = '\0';
                    sp = sp + len + 1;
                }
                break;
            case integer_t:
            case uinteger_t:
            case short_t:
            case ushort_t:
            case long_t:
            case ulong_t:
                {
                    char *endptr;
                    long int val;
                    errno = 0;
                    val = strtol(tokbuf, &endptr, 0);
                    if ((errno == ERANGE &&
                         (val == LONG_MAX || val == LONG_MIN))
                        || (errno != 0 && val == 0)) {
                        debug_trace("Error parsing a number.\n");
                        return -1;
                    }
                    if (tokbuf == endptr) {
                        debug_trace("Not a valid number.\n");
                        return -1;
                    }
                    switch (array->type) {
                    case integer_t:
                        array->arr.integers[offset] = (int) val;
                        break;
                    case uinteger_t:
                        array->arr.uintegers[offset] = (unsigned int) val;
                        break;
                    case short_t:
                        array->arr.shortints[offset] = (short) val;
                        break;
                    case ushort_t:
                        array->arr.ushortints[offset] = (unsigned short) val;
                        break;
                    case long_t:
                        array->arr.longints[offset] = val;
                        break;
                    case ulong_t:
                        array->arr.ulongints[offset] = (unsigned long) val;
                        break;
                    default:
                        ;
                    }
                }
                break;
            case real_t:
                {
                    char *endptr;
                    double val;
                    errno = 0;
                    val = strtod(tokbuf, &endptr);
                    if ((errno == ERANGE &&
                         (val == HUGE_VAL || val == -HUGE_VAL))
                        || (errno != 0 && val == 0)) {
                        debug_trace("Error parsing a number.\n");
                        return -1;
                    }
                    if (tokbuf == endptr) {
                        debug_trace("Not a valid number.\n");
                        return -1;
                    }
                    array->arr.reals[offset] = val;
                }
                break;
            case boolean_t:
                {
                    bool val;
                    if (strcmp(tokbuf, "true") == 0)
                        val = true;
                    else if (strcmp(tokbuf, "false") == 0)
                        val = false;
                    else {
                        debug_trace("Got '%s' when expecting boolean.\n",
                                    tokbuf);
                        return -1;
                    }
                    array->arr.booleans[offset] = val;
                }
                break;
            case array_t:
            case table_t:
                debug_trace("Invalid array type.\n");
                return -1;
            }
            break;
        case LBRACE: /* [ { }, { } ] */
            /* TODO: load inline table */
            break;
        case COMMA:
            continue;
        default:
            debug_trace("Invalid array syntax.\n");
            return -1;
        }
        offset++;
    }
    if (array->count != NULL)
        *(array->count) = offset;
    return 0;
}

/* Parses key/value pairs. */
static int load_key_value(FILE *fp, const struct toml_key_t *keys,
                          const char *key)
{
    char tokbuf[TOKBUFSIZ];
    int tok;
    const struct toml_key_t *cursor;
    size_t maxlen = 0;

    debug_trace("Collected key name '%s'\n", key);
    for (cursor = keys; cursor->key != NULL; cursor++) {
        if (strcmp(cursor->key, key) == 0)
            break;
    }
    if (cursor->key == NULL) {
        debug_trace("Unknown key name '%s'\n", key);
        return -1;
    }
    if (cursor->type == string_t)
        maxlen = cursor->len;
    else
        maxlen = sizeof(tokbuf);

    tok = scan(tokbuf, sizeof(tokbuf), fp);
    if (tok != EQUAL) {
        debug_trace("Missing '='\n");
        return -1;
    }

    tok = scan(tokbuf, maxlen, fp);
    switch (tok) {
        case STRING: /* key = "value" */
        case WORD:
            debug_trace("Collected value '%s'\n", tokbuf);
            /* FIXME: validate types */
            if (tok == STRING && cursor->type != string_t) {
                debug_trace("Saw quoted value when expecting "
                            "non-string.\n");
                return -1;
            }
            if (tok != STRING && cursor->type == string_t) {
                debug_trace("Didn't see quoted value when "
                            "expecting string.\n");
                return -1;
            }
            if (cursor->type == boolean_t &&
                (strcmp(tokbuf, "true") != 0 &&
                 strcmp(tokbuf, "false") != 0)) {
                debug_trace("Got '%s' when expecting boolean.\n", tokbuf);
                return -1;
            }
            switch (cursor->type) {
            case string_t:
                {
                    char *sp = cursor->addr.string;
                    strncpy(sp, tokbuf, maxlen-1);
                    sp[maxlen-1] = '\0';
                }
                break;
            case integer_t:
            case uinteger_t:
            case short_t:
            case ushort_t:
            case long_t:
            case ulong_t:
                {
                    char *endptr;
                    long int val;
                    errno = 0;
                    val = strtol(tokbuf, &endptr, 0);
                    if ((errno == ERANGE &&
                         (val == LONG_MAX || val == LONG_MIN))
                        || (errno != 0 && val == 0)) {
                        debug_trace("Error parsing a number.\n");
                        return -1;
                    }
                    if (tokbuf == endptr) {
                        debug_trace("Not a valid number.\n");
                        return -1;
                    }
                    switch (cursor->type) {
                    case integer_t:
                        *(cursor->addr.integer) = (int) val;
                        break;
                    case uinteger_t:
                        *(cursor->addr.uinteger) = (unsigned int) val;
                        break;
                    case short_t:
                        *(cursor->addr.shortint) = (short) val;
                        break;
                    case ushort_t:
                        *(cursor->addr.ushortint) = (unsigned short) val;
                        break;
                    case long_t:
                        *(cursor->addr.longint) = val;
                        break;
                    case ulong_t:
                        *(cursor->addr.ulongint) = (unsigned long) val;
                        break;
                    default:
                        ;
                    }
                }
                break;
            case boolean_t:
                *(cursor->addr.boolean) = strcmp(tokbuf, "true") == 0;
                break;
            case real_t:
                {
                    char *endptr;
                    double val;
                    errno = 0;
                    val = strtod(tokbuf, &endptr);
                    if ((errno == ERANGE &&
                         (val == HUGE_VAL || val == -HUGE_VAL))
                        || (errno != 0 && val == 0)) {
                        debug_trace("Error parsing a number.\n");
                        return -1;
                    }
                    if (tokbuf == endptr) {
                        debug_trace("Not a valid number.\n");
                        return -1;
                    }
                    *(cursor->addr.real) = val;
                }
                break;
            case table_t:
            default:
                ;
            }
            break;
        case LBRACKET: /* key = [ ] */
            if (cursor->type != array_t) {
                debug_trace("Saw [ when not expecting array.\n");
                return -1;
            }
            return load_array(fp, &cursor->addr.array);
        case LBRACE: /* key = { } */
            if (cursor->type != table_t) {
                debug_trace("Saw { when not expecting a table.\n");
                return -1;
            }
            return load_inline_table(fp, cursor->addr.keys);
        default:
            debug_trace("Invalid syntax\n");
            return -1;
    }
    return 0;
}

/* Parses inline tables: { key1 = value1, key2 = value2 } */
static int load_inline_table(FILE *fp, const struct toml_key_t *keys)
{
    char tokbuf[TOKBUFSIZ];
    int tok;

    while ((tok = scan(tokbuf, sizeof(tokbuf), fp)) != -1) {
        if (tok == RBRACE) /* reached the end of the table */
            return 0;
        if (tok == COMMA)
            continue;
        if (tok == STRING || tok == WORD) { /* key/value pairs */
            if (load_key_value(fp, keys, tokbuf) == -1) {
                debug_trace("load_key_value() failed: key is '%s'.\n", tokbuf);
                return -1;
            }
        } else {
            debug_trace("Invalid inline table syntax.\n");
            return -1;
        }
    }
    return -1;
}

int toml_load(FILE *fp, const struct toml_key_t *keys)
{
    const struct toml_key_t *curtab = keys;
    const struct toml_key_t *cursor;
    char tokbuf[TOKBUFSIZ];
    int tok;

    while ((tok = scan(tokbuf, sizeof(tokbuf), fp)) != -1) {
        switch (tok) {
        case LBRACKET: /* [table] or [[table]] */
            tok = scan(tokbuf, sizeof(tokbuf), fp);
            if (tok != WORD && tok != STRING) {
                debug_trace("Invalid syntax.\n");
                return -1;
            }
            debug_trace("Collected table name '%s'\n", tokbuf);
            for (cursor = keys; cursor->key != NULL; cursor++) {
                if (strcmp(cursor->key, tokbuf) == 0)
                    break;
            }
            if (cursor->key == NULL) {
                debug_trace("Unknown table name '%s'\n", tokbuf);
                return -1;
            }
            if (cursor->type != table_t) {
                debug_trace("Saw simple value type when expecting "
                            "a table.\n");
                return -1;
            }
            tok = scan(tokbuf, sizeof(tokbuf), fp);
            if (tok != RBRACKET) {
                debug_trace("Missing ']'\n");
                return -1;
            }
            curtab = cursor->addr.keys;
            break;
        case STRING: /* key/value pairs */
        case WORD:
            if (load_key_value(fp, curtab, tokbuf) == -1) {
                debug_trace("load_key_value() failed: key is '%s'.\n", tokbuf);
                return -1;
            }
            break;
        default:
            debug_trace("Invalid syntax\n");
            return -1;
        }
    }
    return 0;
}

