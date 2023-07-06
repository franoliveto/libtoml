/* toml.c - parse TOML into fixed-extent data structures.
 *
 * This module parses a large subset of TOML (Tom’s Obvious Minimal Language).
 * Unlike more general TOML parsers, it doesn’t use malloc(3); you need to
 * give it a set of template structures describing the expected shape of
 * the incoming TOML, and it will error out if that shape is not matched.
 * When the parse succeeds, key values will be extracted into static
 * locations specified in the template structures.
 *
 * The dialect it parses has some limitations. First, only ASCII encoded
 * files are considered to be valid TOML documents. Second, all elements
 * of an array must be of the same type. Third, arrays may not be array
 * elements. Fourth, large numbers may not use underscores between digits.
 *
 * Copyright (c) 2022, Francisco Oliveto <franciscoliveto@gmail.com>
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <math.h> /* HUGE_VAL */

#include "toml.h"

/* UTF-8 support: getwc(), ungetwc(), LC_CTYPE */

enum {
    LBRACKETS,     /* [[ */
    RBRACKETS,     /* ]] */
    HEX_INTEGER,   /* 0x[0-9abcdefABCDEF_] */
    OCT_INTEGER,   /* 0o[0-7_] */
    BIN_INTEGER,   /* 0b[01_] */
    INTEGER,       /* [+-0-9_] */
    FLOAT,
    BARE_KEY,      /* [A-Za-z0-9-_] */
    STRING,
    NEWLINE        /* \r, \n, or \r\n */
};


const struct toml_key_t *curtab;
const struct toml_key_t *cursor;
static FILE *inputfp;
struct token {
    int type;
    char lexeme[BUFSIZ];
    int pos; /* position of the error, starting at 0 */
    int line; /* line number, starting at 1 */
} token;


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

/* Prints error message and exits. */
void error_printf(const char *fmt, ...)
{
    char buf[BUFSIZ];
    va_list ap;

    fprintf(stderr, "syntax error (line %d, column %d): ", token.line, token.pos);
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(2);
}

/* Checks for and consume \r, \n, \r\n, or EOF */
static bool endofline(int c, FILE *fp)
{
    bool eol;

    eol = (c=='\r' || c=='\n');
    if (c == '\r') {
        c = getc(fp);
        if (c != '\n' && c != EOF)
            ungetc(c, fp); /* read to far, put it back */
    }
    return eol;
}

/* Returns but does not consume the next character in the input. */
static int lex_peek(FILE *fp)
{
    int c;
    c = getc(fp);
    ungetc(c, fp);
    return c;
}

/* Scans for a number (integer, float) */
static int lex_scan_number(int c, FILE *fp)
{
    char *p = token.lexeme;

    /* FIXME: validate size */
    for (*p++ = c; isdigit(c = getc(fp)) || c == '_'; ) {
        if (c != '_')
            *p++ = c;
    }
    *p = '\0';
    ungetc(c, fp);
    return INTEGER;
}

/* Scans for a literal string. */
static int lex_scan_literal_string(FILE *fp)
{
    int c;
    char *p = token.lexeme;

    while ((c = getc(fp)) != '\'' && c != '\r' && c != '\n')
        *p++ = c;
    *p = '\0';
    if (c == '\'')
        return STRING;
    if (c == '\r' || c == '\n')
        error_printf("saw '\\n' before '\''");
    else if (c == EOF) {
        if (ferror(fp))
            error_printf("input failed");
        else
            error_printf("saw EOF before '\''");
    }
}

/* Consumes an escaped character. */
static int lex_escape(FILE *fp)
{
    int c;

    if ((c = getc(fp)) == 'b')
        return '\b';
    if (c == 'f')
        return '\f';
    if (c == 'n')
        return '\n';
    if (c == 'r')
        return '\r';
    if (c == 't')
        return '\t';
    if (c == 'u') {} /* \uXXXX */
    if (c == 'U') {} /* \UXXXXXXXX */
    if (c == '"' || c == '\\')
        return c;
    error_printf("invalid escape sequence '\%c'", c);
}

/* Scans for multiline literal strings. */
static int lex_scan_ml_literal_string(FILE *fp)
{
    (void) fp;
    return STRING;
}

/* Scans for multiline strings. */
static int lex_scan_ml_string(FILE *fp)
{
    int c;
    char *p = token.lexeme;

    if (!endofline(c = getc(fp), fp))
        ungetc(c, fp); /* was not a newline, put it back */

    /* FIXME: validate string size */
    for (;;) {
        /* the string can contain " and "", including at the end: """str"""""
           6 or more at the end, however, is an error. */
        int n;
        for (n = 0; (c = getc(fp)) == '"'; )
            n++;
        if (n == 3 || n == 4 || n == 5) {
            ungetc(c, fp); /* probably \r or \n */
            if (n == 4) /* one double quote at the end: """" */
                *p++ = '"';
            else if (n == 5) { /* two double quotes at the end: """"" */
                *p++ = '"';
                *p++ = '"';
            }
            *p = '\0';
            return STRING;
        }
        if (c == EOF)
            error_printf("saw EOF before \"\"\"");
        if (n > 5)
            error_printf("too many double quotes at the end of "
                         "multiline string");
        for (int i = 0; i < n; i++)
            *p++ = '"';
        if (c == '\\') {
            int peek = lex_peek(fp);
            if (isspace(peek)) {
                while (isspace(c = getc(fp)))
                    ;
                ungetc(c, fp);
                continue;
            }
            c = lex_escape(fp);
        }
        *p++ = c;
    }
}

/* Scans for a basic string. */
static int lex_scan_string(FILE *fp)
{
    int c;
    char *p = token.lexeme;

    /* FIXME: validate string size */
    while ((c = getc(fp)) != '"' && c != '\r' && c != '\n') {
        if (c == '\\')
            c = lex_escape(fp);
        *p++ = c;
    }
    *p = '\0';
    if (c == '"')
        return STRING;
    if (c == '\r' || c == '\n')
        error_printf("saw '\\n' before '\"'");
    else if (c == EOF) {
        if (ferror(fp))
            error_printf("input failed");
        else
            error_printf("saw EOF before '\"'");
    }
}

/* Scans for tokens. */
static int lex_scan(FILE *fp)
{
    int c;

    while ((c = getc(fp)) != EOF) {
        if (c == ' ' || c == '\t')
            continue;
        if (c == '#') { /* ignore comments */
            while ((c = getc(fp)) != EOF && c != '\r' && c != '\n')
                ;
            ungetc(c, fp); /* put \r or \n back */
            continue;
        }

        if (c == '[') {
            if ((c = getc(fp)) == '[')
                return token.type = LBRACKETS;
            ungetc(c, fp);
            return token.type = '[';
        }
        if (c == ']') {
            if ((c = getc(fp)) == ']')
                return token.type = RBRACKETS;
            ungetc(c, fp);
            return token.type = ']';
        }
        if (c == '=')
            return token.type = '=';
        if (c == '{')
            return token.type = '{';
        if (c == '}')
            return token.type = '}';
        if (c == ',')
            return token.type = ',';
        if (c == '.')
            return token.type = '.';
        if (c == '"') {
            if ((c = getc(fp)) == '"') {
                if ((c = getc(fp)) == '"') /* Got """ */
                    return token.type = lex_scan_ml_string(fp);
                ungetc(c, fp);
                token.lexeme[0] = '\0'; /* Got an empty string. */
                return token.type = STRING;
            }
            ungetc(c, fp);
            return token.type = lex_scan_string(fp);
        }
        if (c == '\'') {
            if ((c = getc(fp)) == '\'') {
                if ((c = getc(fp)) == '\'') /* Got ''' */
                    return token.type = lex_scan_ml_literal_string(fp);
                ungetc(c, fp);
                token.lexeme[0] = '\0'; /* Got an empty string. */
                return token.type = STRING;
            }
            ungetc(c, fp);
            return token.type = lex_scan_literal_string(fp);
        }
        if (c == '0') {
            int savedc = c;
            char *p = token.lexeme;

            *p++ = c;
            c = getc(fp);
            if (c == 'x') { /* hexadecimal */
                for (*p++ = c; isxdigit(c = getc(fp)) || c == '_'; )
                    *p++ = c;  /* FIXME: validate size */
                *p = '\0';
                ungetc(c, fp);
                return token.type = HEX_INTEGER;
            }
            if (c == 'o') { /* octal */
                for (*p++ = c; ((c = getc(fp)) >= '0' && c <= '7')
                         || c == '_'; )
                    *p++ = c;  /* FIXME: validate size */
                *p = '\0';
                ungetc(c, fp);
                return token.type = OCT_INTEGER;
            }
            if (c == 'b') { /* binary */
                for (*p++ = c; (c = getc(fp)) == '0' || c == '1' || c == '_'; )
                    *p++ = c;  /* FIXME: validate size */
                *p = '\0';
                ungetc(c, fp);
                return token.type = BIN_INTEGER;
            }
            ungetc(c, fp); /* was not a prefix, put it back */
            c = savedc;
        }

        if (c == '+' || c == '-') {
            int nextc = lex_peek(fp);
            if (isdigit(nextc))
                return token.type = lex_scan_number(c, fp); /* INTEGER, FLOAT */
            if (nextc == 'i') {
                (void) getc(fp); /* consume i */
                if (getc(fp) == 'n') {
                    if (getc(fp) == 'f') {
                        sprintf(token.lexeme, "%cinf", c);
                        return token.type = FLOAT;
                    }
                }
                error_printf("invalid float");
            }
            if (nextc == 'n') {
                (void) getc(fp); /* consume n */
                if (getc(fp) == 'a') {
                    if (getc(fp) == 'n') {
                        sprintf(token.lexeme, "%cnan", c);
                        return token.type = FLOAT;
                    }
                }
                error_printf("invalid float");
            }
            error_printf("only numbers can start with + or -");
        }
        if (isdigit(c))
            return token.type = lex_scan_number(c, fp); /* INTEGER, FLOAT */

        /* keywords: inf, nan, true, false */

        if (isalpha(c)) { /* FIXME: could also start with '-' or '_' or digit. */
            char *p = token.lexeme;

            for (*p++ = c; isalpha(c = getc(fp))
                     || isdigit(c) || c == '-' || c == '_'; )
                *p++ = c; /* FIXME: validate token size */
            *p = '\0';
            ungetc(c, fp);
            return token.type = BARE_KEY;
        }

        if (endofline(c, fp)) {
            token.line++;
            return token.type = NEWLINE;
        }

        return token.type = c; /* anything else */
    }
    return EOF;
}

void keyval();

static char *target_address(const struct toml_key_t *cursor,
                            const struct toml_array_t *array,
                            int offset)
{
    char *addr = NULL;

    if (array == NULL) {
        switch (cursor->type) {
            case integer_t:
                addr = (char *) cursor->addr.integer;
                break;
            case uinteger_t:
                addr = (char *) cursor->addr.uinteger;
                break;
            case long_t:
                addr = (char *) cursor->addr.longint;
                break;
            case ulong_t:
                addr = (char *) cursor->addr.ulongint;
                break;
            case short_t:
                addr = (char *) cursor->addr.shortint;
                break;
            case ushort_t:
                addr = (char *) cursor->addr.ushortint;
                break;
            case real_t:
                addr = (char *) cursor->addr.real;
                break;
            case boolean_t:
                addr = (char *) cursor->addr.boolean;
                break;
            case string_t:
                addr = cursor->addr.string;
                break;
            default:
                ;
        }
    } else
        addr = array->arr.tables.base + (offset * array->arr.tables.structsize) +
                cursor->addr.offset;
    debug_trace("target address for %s is %p.\n", cursor->key, addr);
    return addr;
}

void value()
{
    if (token.type == '[') { /* array = [ ] */
        do {
            while (lex_scan(inputfp) == NEWLINE)
                ;
            if (token.type == ']') /* end of array */
                break;
            value();
            while (lex_scan(inputfp) == NEWLINE)
                ;
        } while (token.type == ',');

        if (token.type != ']')
            error_printf("expected ']'");
    } else if (token.type == '{') { /* inline-table = { } */
        do {
            lex_scan(inputfp); /* FIXME: lex_next()??? */
            if (token.type == BARE_KEY || token.type == STRING)
                keyval();
            else
                error_printf("expected key");
        } while (lex_scan(inputfp) == ',');

        if (token.type != '}')
            error_printf("expected '}'");
    } else if (token.type == STRING
             || token.type == INTEGER
             || token.type == FLOAT
             || token.type == HEX_INTEGER
             || token.type == OCT_INTEGER
             || token.type == BIN_INTEGER
             || token.type == BARE_KEY) {
        /* FIXME: validate types */
        /* if (token.type == STRING && cursor->type != string_t) */
        /*     error_printf("saw quoted value when expecting non-string"); */
        /* if (token.type != STRING && cursor->type == string_t) */
        /*     error_printf("Didn't see quoted value when expecting string"); */

        char *valp = target_address(cursor, NULL, 0);
        if (valp != NULL)
            switch (cursor->type) {
            case string_t:
            {
                size_t l = cursor->len;
                strncpy(valp, token.lexeme, l-1);
                valp[l-1] = '\0';
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
                val = strtol(token.lexeme, &endptr, 0);
                if ((errno == ERANGE &&
                     (val == LONG_MAX || val == LONG_MIN))
                    || (errno != 0 && val == 0)) {
                    debug_trace("Error parsing a number.\n");
                    return;
                }
                if (token.lexeme == endptr) {
                    debug_trace("Not a valid number.\n");
                    return;
                }
                switch (cursor->type) {
                case integer_t:
                {
                    int tmp = (int) val;
                    memcpy(valp, &tmp, sizeof(int));
                }
                break;
                case uinteger_t:
                {
                    unsigned int tmp = (unsigned int) val;
                    memcpy(valp, &tmp, sizeof(unsigned int));
                }
                break;
                case short_t:
                {
                    short tmp = (short) val;
                    memcpy(valp, &tmp, sizeof(short));
                }
                break;
                case ushort_t:
                {
                    unsigned short tmp = (unsigned short) val;
                    memcpy(valp, &tmp, sizeof(unsigned short));
                }
                break;
                case long_t:
                    memcpy(valp, &val, sizeof(long int));
                    break;
                case ulong_t:
                {
                    unsigned long tmp = (unsigned long) val;
                    memcpy(valp, &tmp, sizeof(unsigned long));
                }
                break;
                default:
                    ;
                }
            }
            break;
            case boolean_t:
            {
                bool tmp = strcmp(token.lexeme, "true") == 0;
                memcpy(valp, &tmp, sizeof(bool));
            }
            break;
            case real_t:
            {
                char *endptr;
                double val;
                errno = 0;
                val = strtod(token.lexeme, &endptr);
                if ((errno == ERANGE &&
                     (val == HUGE_VAL || val == -HUGE_VAL))
                    || (errno != 0 && val == 0)) {
                    debug_trace("Error parsing a number.\n");
                    exit(1);
                }
                if (token.lexeme == endptr) {
                    debug_trace("Not a valid number.\n");
                    exit(1);
                }
                memcpy(valp, &val, sizeof(double));
            }
            break;
            case table_t:
            default:
                ;
            }

    } else
        error_printf("invalid token");
}

void key()
{
    /* simple-key or dotted-key */

    for (cursor = curtab; cursor->key != NULL; cursor++) {
        if (strcmp(cursor->key, token.lexeme) == 0)
            break;
    }
    if (cursor->key == NULL) {
        fprintf(stderr, "unknown key name '%s'\n", token.lexeme);
        exit(2);
    }

    while (lex_scan(inputfp) == '.') {
        lex_scan(inputfp);
        if (token.type == BARE_KEY || token.type == STRING)
            puts(token.lexeme); /* key is used */
        else
            error_printf("expected dotted key");
    }
}

int accept(int type)
{
    if (token.type == type) {
        lex_scan(inputfp);
        return 1;
    }
    return 0;
}

void keyval()
{
    key();
    if (accept('='))
        value();
    else
        error_printf("missing '='");
}

void expression()
{
    if (accept(LBRACKETS)) { /* array-table = [[ key ]] */
        if (token.type == BARE_KEY || token.type == STRING) {
            key();
            if (token.type != RBRACKETS)
                error_printf("missing ']]'");
        } else
            error_printf("key was expected");
    } else if (accept('[')) { /* table = [ key ] */
        if (token.type == BARE_KEY || token.type == STRING) {
            key();
            if (token.type != ']')
                error_printf("missing ']'");
        } else
            error_printf("key was expected");
    } else if (token.type == BARE_KEY || token.type == STRING) { /* key */
        keyval();
    } else {
        error_printf("invalid token");
    }
}

int toml_unmarshal(FILE *fp, const struct toml_key_t *keys)
{
    inputfp = fp;
    curtab = keys;
    token.line = 1;
    while (lex_scan(fp) != EOF) {
        if (token.type == NEWLINE)
            continue;
        expression();
        if (lex_scan(fp) == EOF)
            break;
        if (token.type != NEWLINE)
            error_printf("expected newline");
    }
    return 0;
}

const char *toml_strerror(int errnum)
{
    (void) errnum;
    return "there was an error";
}

