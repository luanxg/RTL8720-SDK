/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* baidu_json */
/* JSON parser in C. */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "baidu_json.h"

static const char *global_ep;

const char *baidu_json_GetErrorPtr(void)
{
    return global_ep;
}

/* case insensitive strcmp */
static int baidu_json_strcasecmp(const char *s1, const char *s2)
{
    if (!s1)
    {
        return (s1 == s2) ? 0 : 1; /* both NULL? */
    }
    if (!s2)
    {
        return 1;
    }
    for(; tolower((unsigned char)*s1) == tolower((unsigned char)*s2); ++s1, ++s2)
    {
        if (*s1 == 0)
        {
            return 0;
        }
    }

    return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void* baidu_json_malloc_dummy(size_t sz)
{
    return NULL;
}

static void baidu_json_free_dummy(void *ptr)
{
    //do nothing
}

static void *(*baidu_json_malloc)(size_t sz) = baidu_json_malloc_dummy;
static void (*baidu_json_free)(void *ptr) = baidu_json_free_dummy;

// len indicate the length of str
static char* baidu_json_strdup(const char* str, size_t len)
{
    char* copy;

    if (!str) {
        return NULL;
    }
    if (len <= 0) {
        len = strlen(str) + 1;
    } else {
        len = len + 1;
    }
    if (!(copy = (char*)baidu_json_malloc(len)))
    {
        return 0;
    }
    memcpy(copy, str, len);

    return copy;
}

#ifdef DUER_BJSON_PREALLOC_ITEM
#include "lightduer_bitmap.h"

#define PREALLOC_ITEM_NUM 103
static bitmap_objects_t s_bitmap_objects;

#endif // DUER_BJSON_PREALLOC_ITEM

void baidu_json_InitHooks(baidu_json_Hooks* hooks)
{
    if (!hooks)
    {
        /* Reset hooks */
        baidu_json_malloc = baidu_json_malloc_dummy;
        baidu_json_free = baidu_json_free_dummy;
        return;
    }

    baidu_json_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : baidu_json_malloc_dummy;
    baidu_json_free = (hooks->free_fn) ? hooks->free_fn : baidu_json_free_dummy;

#ifdef DUER_BJSON_PREALLOC_ITEM
    int r = init_bitmap(PREALLOC_ITEM_NUM, &s_bitmap_objects, sizeof(baidu_json));
#endif // DUER_BJSON_PREALLOC_ITEM

}


int baidu_json_Uninit()
{
#ifdef DUER_BJSON_PREALLOC_ITEM
    return uninit_bitmap(&s_bitmap_objects);
#else
    return 0;
#endif // DUER_BJSON_PREALLOC_ITEM
}

/* Internal constructor. */
static baidu_json *baidu_json_New_Item(void)
{
#ifdef DUER_BJSON_PREALLOC_ITEM

    baidu_json* node = alloc_obj(&s_bitmap_objects);
    if (node == NULL) {
        node = (baidu_json*)baidu_json_malloc(sizeof(baidu_json)); // fallback to malloc from heap
    }
#else
    baidu_json* node = (baidu_json*)baidu_json_malloc(sizeof(baidu_json));
#endif // DUER_BJSON_PREALLOC_ITEM
    if (node)
    {
        memset(node, 0, sizeof(baidu_json));
    }

    return node;
}

/* Delete a baidu_json structure. */
void baidu_json_Delete(baidu_json *c)
{
    baidu_json *next;
    while (c)
    {
        next = c->next;
        if (!(c->type & baidu_json_IsReference) && c->child)
        {
            baidu_json_Delete(c->child);
        }
        if (!(c->type & baidu_json_IsReference) && c->valuestring)
        {
            baidu_json_free(c->valuestring);
        }
        if (!(c->type & baidu_json_StringIsConst) && c->string)
        {
            baidu_json_free(c->string);
        }
#ifdef DUER_BJSON_PREALLOC_ITEM
        int r = 0;
        r = free_obj(&s_bitmap_objects, c);
        if (r == -2) {
            baidu_json_free(c); // means this node malloc from heap directly.
        }
#else
        baidu_json_free(c);
#endif // DUER_BJSON_PREALLOC_ITEM
        c = next;
    }
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(baidu_json *item, const char *num)
{
    double n = 0;
    double sign = 1;
    double scale = 0;
    int subscale = 0;
    int signsubscale = 1;

    /* Has sign? */
    if (*num == '-')
    {
        sign = -1;
        num++;
    }
    /* is zero */
    if (*num == '0')
    {
        num++;
    }
    /* Number? */
    if ((*num >= '1') && (*num <= '9'))
    {
        do
        {
            n = (n * 10.0) + (*num++ - '0');
        }
        while ((*num >= '0') && (*num<='9'));
    }
    /* Fractional part? */
    if ((*num == '.') && (num[1] >= '0') && (num[1] <= '9'))
    {
        num++;
        do
        {
            n = (n  *10.0) + (*num++ - '0');
            scale--;
        } while ((*num >= '0') && (*num <= '9'));
    }
    /* Exponent? */
    if ((*num == 'e') || (*num == 'E'))
    {
        num++;
        /* With sign? */
        if (*num == '+')
        {
            num++;
        }
        else if (*num == '-')
        {
            signsubscale = -1;
            num++;
        }
        /* Number? */
        while ((*num>='0') && (*num<='9'))
        {
            subscale = (subscale * 10) + (*num++ - '0');
        }
    }

    /* number = +/- number.fraction * 10^+/- exponent */
    n = sign * n * pow(10.0, (scale + subscale * signsubscale));

    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = baidu_json_Number;

    return num;
}

/* calculate the next largest power of 2 */
static int pow2gt (int x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return x + 1;
}

typedef struct
{
    char *buffer;
    int length;
    int offset;
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static char* ensure(printbuffer *p, int needed)
{
    char *newbuffer;
    int newsize;
    if (!p || !p->buffer)
    {
        return 0;
    }
    needed += p->offset;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    newsize = pow2gt(needed);
    newbuffer = (char*)baidu_json_malloc(newsize);
    if (!newbuffer)
    {
        baidu_json_free(p->buffer);
        p->length = 0;
        p->buffer = 0;

        return 0;
    }
    if (newbuffer)
    {
        memcpy(newbuffer, p->buffer, p->length);
    }
    baidu_json_free(p->buffer);
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer */
static int update(const printbuffer *p)
{
    char *str;
    if (!p || !p->buffer)
    {
        return 0;
    }
    str = p->buffer + p->offset;

    return p->offset + strlen(str);
}

/* Render the number nicely from the given item into a string. */
static char *print_number(const baidu_json *item, printbuffer *p)
{
    char *str = 0;
    double d = item->valuedouble;
    /* special case for 0. */
    if (d == 0)
    {
        if (p)
        {
            str = ensure(p, 2);
        }
        else
        {
            str = (char*)baidu_json_malloc(2);
        }
        if (str)
        {
            strcpy(str,"0");
        }
    }
    /* value is an int */
    else if ((fabs(((double)item->valueint) - d) <= DBL_EPSILON) && (d <= INT_MAX) && (d >= INT_MIN))
    {
        if (p)
        {
            str = ensure(p, 21);
        }
        else
        {
            /* 2^64+1 can be represented in 21 chars. */
            str = (char*)baidu_json_malloc(21);
        }
        if (str)
        {
            sprintf(str, "%d", item->valueint);
        }
    }
    /* value is a floating point number */
    else
    {
        if (p)
        {
            /* This is a nice tradeoff. */
            str = ensure(p, 64);
        }
        else
        {
            /* This is a nice tradeoff. */
            str=(char*)baidu_json_malloc(64);
        }
        if (str)
        {
            /* This checks for NaN and Infinity */
            if ((d * 0) != 0)
            {
                sprintf(str, "null");
            }
            else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60))
            {
                sprintf(str, "%.0f", d);
            }
            else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9))
            {
                sprintf(str, "%e", d);
            }
            else
            {
                sprintf(str, "%f", d);
            }
        }
    }
    return str;
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const char *str)
{
    unsigned h = 0;
    /* first digit */
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }


    /* second digit */
    h = h << 4;
    str++;
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }

    /* third digit */
    h = h << 4;
    str++;
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }

    /* fourth digit */
    h = h << 4;
    str++;
    if ((*str >= '0') && (*str <= '9'))
    {
        h += (*str) - '0';
    }
    else if ((*str >= 'A') && (*str <= 'F'))
    {
        h += 10 + (*str) - 'A';
    }
    else if ((*str >= 'a') && (*str <= 'f'))
    {
        h += 10 + (*str) - 'a';
    }
    else /* invalid */
    {
        return 0;
    }

    return h;
}

/* first bytes of UTF8 encoding for a given length in bytes */
static const unsigned char firstByteMark[7] =
{
    0x00, /* should never happen */
    0x00, /* 0xxxxxxx */
    0xC0, /* 110xxxxx */
    0xE0, /* 1110xxxx */
    0xF0, /* 11110xxx */
    0xF8,
    0xFC
};

/* Parse the input text into an unescaped cstring, and populate item. */
static const char *parse_string(baidu_json *item, const char *str, const char **ep)
{
    const char *ptr = str + 1;
    const char *end_ptr =str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    unsigned uc;
    unsigned uc2;

    /* not a string! */
    if (*str != '\"')
    {
        *ep = str;
        return 0;
    }

    while ((*end_ptr != '\"') && *end_ptr && ++len)
    {
        if (*end_ptr++ == '\\')
        {
            if (*end_ptr == '\0')
            {
                /* prevent buffer overflow when last input character is a backslash */
                return 0;
            }
            /* Skip escaped quotes. */
            end_ptr++;
        }
    }

    /* This is at most how long we need for the string, roughly. */
    out = (char*)baidu_json_malloc(len + 1);
    if (!out)
    {
        return 0;
    }
    item->valuestring = out; /* assign here so out will be deleted during baidu_json_Delete() later */
    item->type = baidu_json_String;

    ptr = str + 1;
    ptr2 = out;
    /* loop through the string literal */
    while (ptr < end_ptr)
    {
        if (*ptr != '\\')
        {
            *ptr2++ = *ptr++;
        }
        /* escape sequence */
        else
        {
            ptr++;
            switch (*ptr)
            {
                case 'b':
                    *ptr2++ = '\b';
                    break;
                case 'f':
                    *ptr2++ = '\f';
                    break;
                case 'n':
                    *ptr2++ = '\n';
                    break;
                case 'r':
                    *ptr2++ = '\r';
                    break;
                case 't':
                    *ptr2++ = '\t';
                    break;
                case 'u':
                    /* transcode utf16 to utf8. See RFC2781 and RFC3629. */
                    uc = parse_hex4(ptr + 1); /* get the unicode char. */
                    ptr += 4;
                    if (ptr >= end_ptr)
                    {
                        /* invalid */
                        *ep = str;
                        return 0;
                    }
                    /* check for invalid. */
                    if (((uc >= 0xDC00) && (uc <= 0xDFFF)) || (uc == 0))
                    {
                        *ep = str;
                        return 0;
                    }

                    /* UTF16 surrogate pairs. */
                    if ((uc >= 0xD800) && (uc<=0xDBFF))
                    {
                        if ((ptr + 6) > end_ptr)
                        {
                            /* invalid */
                            *ep = str;
                            return 0;
                        }
                        if ((ptr[1] != '\\') || (ptr[2] != 'u'))
                        {
                            /* missing second-half of surrogate. */
                            *ep = str;
                            return 0;
                        }
                        uc2 = parse_hex4(ptr + 3);
                        ptr += 6; /* \uXXXX */
                        if ((uc2 < 0xDC00) || (uc2 > 0xDFFF))
                        {
                            /* invalid second-half of surrogate. */
                            *ep = str;
                            return 0;
                        }
                        /* calculate unicode codepoint from the surrogate pair */
                        uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                    }

                    /* encode as UTF8
                     * takes at maximum 4 bytes to encode:
                     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
                    len = 4;
                    if (uc < 0x80)
                    {
                        /* normal ascii, encoding 0xxxxxxx */
                        len = 1;
                    }
                    else if (uc < 0x800)
                    {
                        /* two bytes, encoding 110xxxxx 10xxxxxx */
                        len = 2;
                    }
                    else if (uc < 0x10000)
                    {
                        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
                        len = 3;
                    }
                    ptr2 += len;

                    switch (len) {
                        case 4:
                            /* 10xxxxxx */
                            *--ptr2 = ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 3:
                            /* 10xxxxxx */
                            *--ptr2 = ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 2:
                            /* 10xxxxxx */
                            *--ptr2 = ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 1:
                            /* depending on the length in bytes this determines the
                             * encoding ofthe first UTF8 byte */
                            *--ptr2 = (uc | firstByteMark[len]);
                    }
                    ptr2 += len;
                    break;
                default:
                    *ptr2++ = *ptr;
                    break;
            }
            ptr++;
        }
    }
    *ptr2 = '\0';
    if (*ptr == '\"')
    {
        ptr++;
    }

    return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str, printbuffer *p)
{
    const char *ptr;
    char *ptr2;
    char *out;
    int len = 0;
    int flag = 0;
    unsigned char token;

    /* empty string */
    if (!str)
    {
        if (p)
        {
            out = ensure(p, 3);
        }
        else
        {
            out = (char*)baidu_json_malloc(3);
        }
        if (!out)
        {
            return 0;
        }
        strcpy(out, "\"\"");

        return out;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (ptr = str; *ptr; ptr++)
    {
        flag |= (((*ptr > 0) && (*ptr < 32)) /* unprintable characters */
                || (*ptr == '\"') /* double quote */
                || (*ptr == '\\')) /* backslash */
            ? 1
            : 0;
    }
    /* no characters have to be escaped */
    if (!flag)
    {
        len = ptr - str;
        if (p)
        {
            out = ensure(p, len + 3);
        }
        else
        {
            out = (char*)baidu_json_malloc(len + 3);
        }
        if (!out)
        {
            return 0;
        }

        ptr2 = out;
        *ptr2++ = '\"';
        strcpy(ptr2, str);
        ptr2[len] = '\"';
        ptr2[len + 1] = '\0';

        return out;
    }

    ptr = str;
    /* calculate additional space that is needed for escaping */
    while ((token = *ptr) && ++len)
    {
        if (strchr("\"\\\b\f\n\r\t", token))
        {
            len++; /* +1 for the backslash */
        }
        else if (token < 32)
        {
            len += 5; /* +5 for \uXXXX */
        }
        ptr++;
    }

    if (p)
    {
        out = ensure(p, len + 3);
    }
    else
    {
        out = (char*)baidu_json_malloc(len + 3);
    }
    if (!out)
    {
        return 0;
    }

    ptr2 = out;
    ptr = str;
    *ptr2++ = '\"';
    /* copy the string */
    while (*ptr)
    {
        if (((unsigned char)*ptr > 31) && (*ptr != '\"') && (*ptr != '\\'))
        {
            /* normal character, copy */
            *ptr2++ = *ptr++;
        }
        else
        {
            /* character needs to be escaped */
            *ptr2++ = '\\';
            switch (token = *ptr++)
            {
                case '\\':
                    *ptr2++ = '\\';
                    break;
                case '\"':
                    *ptr2++ = '\"';
                    break;
                case '\b':
                    *ptr2++ = 'b';
                    break;
                case '\f':
                    *ptr2++ = 'f';
                    break;
                case '\n':
                    *ptr2++ = 'n';
                    break;
                case '\r':
                    *ptr2++ = 'r';
                    break;
                case '\t':
                    *ptr2++ = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    sprintf(ptr2, "u%04x", token);
                    ptr2 += 5;
                    break;
            }
        }
    }
    *ptr2++ = '\"';
    *ptr2++ = '\0';

    return out;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static char *print_string(const baidu_json *item, printbuffer *p)
{
    return print_string_ptr(item->valuestring, p);
}

/* Predeclare these prototypes. */
static const char *parse_value(baidu_json *item, const char *value, const char **ep);
static char *print_value(const baidu_json *item, int depth, int fmt, printbuffer *p);
static const char *parse_array(baidu_json *item, const char *value, const char **ep);
static char *print_array(const baidu_json *item, int depth, int fmt, printbuffer *p);
static const char *parse_object(baidu_json *item, const char *value, const char **ep);
static char *print_object(const baidu_json *item, int depth, int fmt, printbuffer *p);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in)
{
    while (in && *in && ((unsigned char)*in<=32))
    {
        in++;
    }

    return in;
}

/* Parse an object - create a new root, and populate. */
baidu_json *baidu_json_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated)
{
    const char *end = 0;
    /* use global error pointer if no specific one was given */
    const char **ep = return_parse_end ? return_parse_end : &global_ep;
    baidu_json *c = baidu_json_New_Item();
    *ep = 0;
    if (!c) /* memory fail */
    {
        return 0;
    }

    end = parse_value(c, skip(value), ep);
    if (!end)
    {
        /* parse failure. ep is set. */
        baidu_json_Delete(c);
        return 0;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated)
    {
        end = skip(end);
        if (*end)
        {
            baidu_json_Delete(c);
            *ep = end;
            return 0;
        }
    }
    if (return_parse_end)
    {
        *return_parse_end = end;
    }

    return c;
}

/* Default options for baidu_json_Parse */
baidu_json *baidu_json_Parse(const char *value)
{
    return baidu_json_ParseWithOpts(value, 0, 0);
}

/* Render a baidu_json item/entity/structure to text. */
char *baidu_json_Print(const baidu_json *item)
{
    return print_value(item, 0, 1, 0);
}

char *baidu_json_PrintUnformatted(const baidu_json *item)
{
    return print_value(item, 0, 0, 0);
}

char *baidu_json_PrintBuffered(const baidu_json *item, int prebuffer, int fmt)
{
    printbuffer p;
    p.buffer = (char*)baidu_json_malloc(prebuffer);
    p.length = prebuffer;
    p.offset = 0;

    return print_value(item, 0, fmt, &p);
}


/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(baidu_json *item, const char *value, const char **ep)
{
    if (!value)
    {
        /* Fail on null. */
        return 0;
    }

    /* parse the different types of values */
    if (!strncmp(value, "null", 4))
    {
        item->type = baidu_json_NULL;
        return value + 4;
    }
    if (!strncmp(value, "false", 5))
    {
        item->type = baidu_json_False;
        return value + 5;
    }
    if (!strncmp(value, "true", 4))
    {
        item->type = baidu_json_True;
        item->valueint = 1;
        return value + 4;
    }
    if (*value == '\"')
    {
        return parse_string(item, value, ep);
    }
    if ((*value == '-') || ((*value >= '0') && (*value <= '9')))
    {
        return parse_number(item, value);
    }
    if (*value == '[')
    {
        return parse_array(item, value, ep);
    }
    if (*value == '{')
    {
        return parse_object(item, value, ep);
    }

    *ep=value;return 0;	/* failure. */
}

/* Render a value to text. */
static char *print_value(const baidu_json *item, int depth, int fmt, printbuffer *p)
{
    char *out = 0;

    if (!item)
    {
        return 0;
    }
    if (p)
    {
        switch ((item->type) & 0xFF)
        {
            case baidu_json_NULL:
                out = ensure(p, 5);
                if (out)
                {
                    strcpy(out, "null");
                }
                break;
            case baidu_json_False:
                out = ensure(p, 6);
                if (out)
                {
                    strcpy(out, "false");
                }
                break;
            case baidu_json_True:
                out = ensure(p, 5);
                if (out)
                {
                    strcpy(out, "true");
                }
                break;
            case baidu_json_Number:
                out = print_number(item, p);
                break;
            case baidu_json_String:
                out = print_string(item, p);
                break;
            case baidu_json_Array:
                out = print_array(item, depth, fmt, p);
                break;
            case baidu_json_Object:
                out = print_object(item, depth, fmt, p);
                break;
        }
    }
    else
    {
        switch ((item->type) & 0xFF)
        {
            case baidu_json_NULL:
                out = baidu_json_strdup("null", 0);
                break;
            case baidu_json_False:
                out = baidu_json_strdup("false", 0);
                break;
            case baidu_json_True:
                out = baidu_json_strdup("true", 0);
                break;
            case baidu_json_Number:
                out = print_number(item, 0);
                break;
            case baidu_json_String:
                out = print_string(item, 0);
                break;
            case baidu_json_Array:
                out = print_array(item, depth, fmt, 0);
                break;
            case baidu_json_Object:
                out = print_object(item, depth, fmt, 0);
                break;
        }
    }

    return out;
}

/* Build an array from input text. */
static const char *parse_array(baidu_json *item,const char *value,const char **ep)
{
    baidu_json *child;
    if (*value != '[')
    {
        /* not an array! */
        *ep = value;
        return 0;
    }

    item->type = baidu_json_Array;
    value = skip(value + 1);
    if (*value == ']')
    {
        /* empty array. */
        return value + 1;
    }

    item->child = child = baidu_json_New_Item();
    if (!item->child)
    {
        /* memory fail */
        return 0;
    }
    /* skip any spacing, get the value. */
    value = skip(parse_value(child, skip(value), ep));
    if (!value)
    {
        return 0;
    }

    /* loop through the comma separated array elements */
    while (*value == ',')
    {
        baidu_json *new_item;
        if (!(new_item = baidu_json_New_Item()))
        {
            /* memory fail */
            return 0;
        }
        /* add new item to end of the linked list */
        child->next = new_item;
        new_item->prev = child;
        child = new_item;

        /* go to the next comma */
        value = skip(parse_value(child, skip(value + 1), ep));
        if (!value)
        {
            /* memory fail */
            return 0;
        }
    }

    if (*value == ']')
    {
        /* end of array */
        return value + 1;
    }

    /* malformed. */
    *ep = value;

    return 0;
}

/* Render an array to text */
static char *print_array(const baidu_json *item, int depth, int fmt, printbuffer *p)
{
    char **entries;
    char *out = 0;
    char *ptr;
    char *ret;
    int len = 5;
    baidu_json *child = item->child;
    int numentries = 0;
    int i = 0;
    int fail = 0;
    size_t tmplen = 0;

    /* How many entries in the array? */
    while (child)
    {
        numentries++;
        child = child->next;
    }

    /* Explicitly handle numentries == 0 */
    if (!numentries)
    {
        if (p)
        {
            out = ensure(p, 3);
        }
        else
        {
            out = (char*)baidu_json_malloc(3);
        }
        if (out)
        {
            strcpy(out,"[]");
        }

        return out;
    }

    if (p)
    {
        /* Compose the output array. */
        /* opening square bracket */
        i = p->offset;
        ptr = ensure(p, 1);
        if (!ptr)
        {
            return 0;
        }
        *ptr = '[';
        p->offset++;

        child = item->child;
        while (child && !fail)
        {
            print_value(child, depth + 1, fmt, p);
            p->offset = update(p);
            if (child->next)
            {
                len = fmt ? 2 : 1;
                ptr = ensure(p, len + 1);
                if (!ptr)
                {
                    return 0;
                }
                *ptr++ = ',';
                if(fmt)
                {
                    *ptr++ = ' ';
                }
                *ptr = 0;
                p->offset += len;
            }
            child = child->next;
        }
        ptr = ensure(p, 2);
        if (!ptr)
        {
            return 0;
        }
        *ptr++ = ']';
        *ptr = '\0';
        out = (p->buffer) + i;
    }
    else
    {
        /* Allocate an array to hold the pointers to all printed values */
        entries = (char**)baidu_json_malloc(numentries * sizeof(char*));
        if (!entries)
        {
            return 0;
        }
        memset(entries, 0, numentries * sizeof(char*));

        /* Retrieve all the results: */
        child = item->child;
        while (child && !fail)
        {
            ret = print_value(child, depth + 1, fmt, 0);
            entries[i++] = ret;
            if (ret)
            {
                len += strlen(ret) + 2 + (fmt ? 1 : 0);
            }
            else
            {
                fail = 1;
            }
            child = child->next;
        }

        /* If we didn't fail, try to malloc the output string */
        if (!fail)
        {
            out = (char*)baidu_json_malloc(len);
        }
        /* If that fails, we fail. */
        if (!out)
        {
            fail = 1;
        }

        /* Handle failure. */
        if (fail)
        {
            /* free all the entries in the array */
            for (i = 0; i < numentries; i++)
            {
                if (entries[i])
                {
                    baidu_json_free(entries[i]);
                }
            }
            baidu_json_free(entries);
            return 0;
        }

        /* Compose the output array. */
        *out='[';
        ptr = out + 1;
        *ptr = '\0';
        for (i = 0; i < numentries; i++)
        {
            tmplen = strlen(entries[i]);
            memcpy(ptr, entries[i], tmplen);
            ptr += tmplen;
            if (i != (numentries - 1))
            {
                *ptr++ = ',';
                if(fmt)
                {
                    *ptr++ = ' ';
                }
                *ptr = 0;
            }
            baidu_json_free(entries[i]);
        }
        baidu_json_free(entries);
        *ptr++ = ']';
        *ptr++ = '\0';
    }

    return out;
}

/* Build an object from the text. */
static const char *parse_object(baidu_json *item, const char *value, const char **ep)
{
    baidu_json *child;
    if (*value != '{')
    {
        /* not an object! */
        *ep = value;
        return 0;
    }

    item->type = baidu_json_Object;
    value = skip(value + 1);
    if (*value == '}')
    {
        /* empty object. */
        return value + 1;
    }

    child = baidu_json_New_Item();
    item->child = child;
    if (!item->child)
    {
        return 0;
    }
    /* parse first key */
    value = skip(parse_string(child, skip(value), ep));
    if (!value)
    {
        return 0;
    }
    /* use string as key, not value */
    child->string = child->valuestring;
    child->valuestring = 0;

    if (*value != ':')
    {
        /* invalid object. */
        *ep = value;
        return 0;
    }
    /* skip any spacing, get the value. */
    value = skip(parse_value(child, skip(value + 1), ep));
    if (!value)
    {
        return 0;
    }

    while (*value == ',')
    {
        baidu_json *new_item;
        if (!(new_item = baidu_json_New_Item()))
        {
            /* memory fail */
            return 0;
        }
        /* add to linked list */
        child->next = new_item;
        new_item->prev = child;

        child = new_item;
        value = skip(parse_string(child, skip(value + 1), ep));
        if (!value)
        {
            return 0;
        }

        /* use string as key, not value */
        child->string = child->valuestring;
        child->valuestring = 0;

        if (*value != ':')
        {
            /* invalid object. */
            *ep = value;
            return 0;
        }
        /* skip any spacing, get the value. */
        value = skip(parse_value(child, skip(value + 1), ep));
        if (!value)
        {
            return 0;
        }
    }
    /* end of object */
    if (*value == '}')
    {
        return value + 1;
    }

    /* malformed */
    *ep = value;
    return 0;
}

/* Render an object to text. */
static char *print_object(const baidu_json *item, int depth, int fmt, printbuffer *p)
{
    char **entries = 0;
    char **names = 0;
    char *out = 0;
    char *ptr;
    char *ret;
    char *str;
    int len = 7;
    int i = 0;
    int j;
    baidu_json *child = item->child;
    int numentries = 0;
    int fail = 0;
    size_t tmplen = 0;

    /* Count the number of entries. */
    while (child)
    {
        numentries++;
        child = child->next;
    }

    /* Explicitly handle empty object case */
    if (!numentries)
    {
        if (p)
        {
            out = ensure(p, fmt ? depth + 4 : 3);
        }
        else
        {
            out = (char*)baidu_json_malloc(fmt ? depth + 4 : 3);
        }
        if (!out)
        {
            return 0;
        }
        ptr = out;
        *ptr++ = '{';
        if (fmt) {
            *ptr++ = '\n';
            for (i = 0; i < depth; i++)
            {
                *ptr++ = '\t';
            }
        }
        *ptr++ = '}';
        *ptr++ = '\0';

        return out;
    }

    if (p)
    {
        /* Compose the output: */
        i = p->offset;
        len = fmt ? 2 : 1; /* fmt: {\n */
        ptr = ensure(p, len + 1);
        if (!ptr)
        {
            return 0;
        }

        *ptr++ = '{';
        if (fmt)
        {
            *ptr++ = '\n';
        }
        *ptr = '\0';
        p->offset += len;

        child = item->child;
        depth++;
        while (child)
        {
            if (fmt)
            {
                ptr = ensure(p, depth);
                if (!ptr)
                {
                    return 0;
                }
                for (j = 0; j < depth; j++)
                {
                    *ptr++ = '\t';
                }
                p->offset += depth;
            }

            /* print key */
            print_string_ptr(child->string, p);
            p->offset = update(p);

            len = fmt ? 2 : 1;
            ptr = ensure(p, len);
            if (!ptr)
            {
                return 0;
            }
            *ptr++ = ':';
            if (fmt)
            {
                *ptr++ = '\t';
            }
            p->offset+=len;

            /* print value */
            print_value(child, depth, fmt, p);
            p->offset = update(p);

            /* print comma if not last */
            len = (fmt ? 1 : 0) + (child->next ? 1 : 0);
            ptr = ensure(p, len + 1);
            if (!ptr)
            {
                return 0;
            }
            if (child->next)
            {
                *ptr++ = ',';
            }

            if (fmt)
            {
                *ptr++ = '\n';
            }
            *ptr = '\0';
            p->offset += len;

            child = child->next;
        }

        ptr = ensure(p, fmt ? (depth + 1) : 2);
        if (!ptr)
        {
            return 0;
        }
        if (fmt)
        {
            for (i = 0; i < (depth - 1); i++)
            {
                *ptr++ = '\t';
            }
        }
        *ptr++ = '}';
        *ptr = '\0';
        out = (p->buffer) + i;
    }
    else
    {
        /* Allocate space for the names and the objects */
        entries = (char**)baidu_json_malloc(numentries * sizeof(char*));
        if (!entries)
        {
            return 0;
        }
        names = (char**)baidu_json_malloc(numentries * sizeof(char*));
        if (!names)
        {
            baidu_json_free(entries);
            return 0;
        }
        memset(entries,0, sizeof(char*) * numentries);
        memset(names, 0, sizeof(char*) * numentries);

        /* Collect all the results into our arrays: */
        child = item->child;
        depth++;
        if (fmt)
        {
            len += depth;
        }
        while (child && !fail)
        {
            names[i] = str = print_string_ptr(child->string, 0); /* print key */
            entries[i++] = ret = print_value(child, depth, fmt, 0);
            if (str && ret)
            {
                len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
            }
            else
            {
                fail = 1;
            }
            child = child->next;
        }

        /* Try to allocate the output string */
        if (!fail)
        {
            out = (char*)baidu_json_malloc(len);
        }
        if (!out)
        {
            fail = 1;
        }

        /* Handle failure */
        if (fail)
        {
            /* free all the printed keys and values */
            for (i = 0; i < numentries; i++)
            {
                if (names[i])
                {
                    baidu_json_free(names[i]);
                }
                if (entries[i])
                {
                    baidu_json_free(entries[i]);
                }
            }
            baidu_json_free(names);
            baidu_json_free(entries);
            return 0;
        }

        /* Compose the output: */
        *out = '{';
        ptr = out + 1;
        if (fmt)
        {
            *ptr++ = '\n';
        }
        *ptr = 0;
        for (i = 0; i < numentries; i++)
        {
            if (fmt)
            {
                for (j = 0; j < depth; j++)
                {
                    *ptr++='\t';
                }
            }
            tmplen = strlen(names[i]);
            memcpy(ptr, names[i], tmplen);
            ptr += tmplen;
            *ptr++ = ':';
            if (fmt)
            {
                *ptr++ = '\t';
            }
            strcpy(ptr, entries[i]);
            ptr += strlen(entries[i]);
            if (i != (numentries - 1))
            {
                *ptr++ = ',';
            }
            if (fmt)
            {
                *ptr++ = '\n';
            }
            *ptr = 0;
            baidu_json_free(names[i]);
            baidu_json_free(entries[i]);
        }

        baidu_json_free(names);
        baidu_json_free(entries);
        if (fmt)
        {
            for (i = 0; i < (depth - 1); i++)
            {
                *ptr++ = '\t';
            }
        }
        *ptr++ = '}';
        *ptr++ = '\0';
    }

    return out;
}

/* Get Array size/item / object item. */
int    baidu_json_GetArraySize(const baidu_json *array)
{
    baidu_json *c = array->child;
    int i = 0;
    while(c)
    {
        i++;
        c = c->next;
    }
    return i;
}

baidu_json *baidu_json_GetArrayItem(const baidu_json *array, int item)
{
    baidu_json *c = array ? array->child : 0;
    while (c && item > 0)
    {
        item--;
        c = c->next;
    }

    return c;
}

baidu_json *baidu_json_GetObjectItem(const baidu_json *object, const char *string)
{
    baidu_json *c = object ? object->child : 0;
    while (c && baidu_json_strcasecmp(c->string, string))
    {
        c = c->next;
    }
    return c;
}

int baidu_json_HasObjectItem(const baidu_json *object,const char *string)
{
    return baidu_json_GetObjectItem(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(baidu_json *prev, baidu_json *item)
{
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static baidu_json *create_reference(const baidu_json *item)
{
    baidu_json *ref = baidu_json_New_Item();
    if (!ref)
    {
        return 0;
    }
    memcpy(ref, item, sizeof(baidu_json));
    ref->string = 0;
    ref->type |= baidu_json_IsReference;
    ref->next = ref->prev = 0;
    return ref;
}

/* Add item to array/object. */
void   baidu_json_AddItemToArray(baidu_json *array, baidu_json *item)
{
    baidu_json *c = array->child;
    if (!item)
    {
        return;
    }
    if (!c)
    {
        /* list is empty, start new one */
        array->child = item;
    }
    else
    {
        /* append to the end */
        while (c && c->next)
        {
            c = c->next;
        }
        suffix_object(c, item);
    }
}

void   baidu_json_AddItemToObject(baidu_json *object, const char *string, baidu_json *item)
{
    if (!item)
    {
        return;
    }

    /* free old key and set new one */
    if (item->string)
    {
        baidu_json_free(item->string);
    }
    item->string = baidu_json_strdup(string, 0);

    baidu_json_AddItemToArray(object,item);
}

/* Add an item to an object with constant string as key */
void   baidu_json_AddItemToObjectCS(baidu_json *object, const char *string, baidu_json *item)
{
    if (!item)
    {
        return;
    }
    if (!(item->type & baidu_json_StringIsConst) && item->string)
    {
        baidu_json_free(item->string);
    }
    item->string = (char*)string;
    item->type |= baidu_json_StringIsConst;
    baidu_json_AddItemToArray(object, item);
}

void baidu_json_AddItemReferenceToArray(baidu_json *array, baidu_json *item)
{
    baidu_json_AddItemToArray(array, create_reference(item));
}

void baidu_json_AddItemReferenceToObject(baidu_json *object, const char *string, baidu_json *item)
{
    baidu_json_AddItemToObject(object, string, create_reference(item));
}

baidu_json *baidu_json_DetachItemFromArray(baidu_json *array, int which)
{
    baidu_json *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        /* item doesn't exist */
        return 0;
    }
	if (c->prev)
    {
        /* not the first element */
        c->prev->next = c->next;
    }
    if (c->next)
    {
        c->next->prev = c->prev;
    }
    if (c==array->child)
    {
        array->child = c->next;
    }
    /* make sure the detached item doesn't point anywhere anymore */
    c->prev = c->next = 0;

    return c;
}

void baidu_json_DeleteItemFromArray(baidu_json *array, int which)
{
    baidu_json_Delete(baidu_json_DetachItemFromArray(array, which));
}

baidu_json *baidu_json_DetachItemFromObject(baidu_json *object, const char *string)
{
    int i = 0;
    baidu_json *c = object->child;
    while (c && baidu_json_strcasecmp(c->string,string))
    {
        i++;
        c = c->next;
    }
    if (c)
    {
        return baidu_json_DetachItemFromArray(object, i);
    }

    return 0;
}

void baidu_json_DeleteItemFromObject(baidu_json *object, const char *string)
{
    baidu_json_Delete(baidu_json_DetachItemFromObject(object, string));
}

/* Replace array/object items with new ones. */
void baidu_json_InsertItemInArray(baidu_json *array, int which, baidu_json *newitem)
{
    baidu_json *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        baidu_json_AddItemToArray(array, newitem);
        return;
    }
    newitem->next = c;
    newitem->prev = c->prev;
    c->prev = newitem;
    if (c == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
}

void baidu_json_ReplaceItemInArray(baidu_json *array, int which, baidu_json *newitem)
{
    baidu_json *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        return;
    }
    newitem->next = c->next;
    newitem->prev = c->prev;
    if (newitem->next)
    {
        newitem->next->prev = newitem;
    }
    if (c == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }
    c->next = c->prev = 0;
    baidu_json_Delete(c);
}

void baidu_json_ReplaceItemInObject(baidu_json *object, const char *string, baidu_json *newitem)
{
    int i = 0;
    baidu_json *c = object->child;
    while(c && baidu_json_strcasecmp(c->string, string))
    {
        i++;
        c = c->next;
    }
    if(c)
    {
        newitem->string = baidu_json_strdup(string, 0);
        baidu_json_ReplaceItemInArray(object, i, newitem);
    }
}

/* Create basic types: */
baidu_json *baidu_json_CreateNull(void)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type = baidu_json_NULL;
    }

    return item;
}

baidu_json *baidu_json_CreateTrue(void)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type = baidu_json_True;
    }

    return item;
}

baidu_json *baidu_json_CreateFalse(void)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type = baidu_json_False;
    }

    return item;
}

baidu_json *baidu_json_CreateBool(int b)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type = b ? baidu_json_True : baidu_json_False;
    }

    return item;
}

baidu_json *baidu_json_CreateNumber(double num)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type = baidu_json_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }

    return item;
}

baidu_json *baidu_json_CreateString(const char *string, size_t s_len)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type = baidu_json_String;
        item->valuestring = baidu_json_strdup(string, s_len);
        if(!item->valuestring)
        {
            baidu_json_Delete(item);
            return 0;
        }
    }

    return item;
}

baidu_json *baidu_json_CreateArray(void)
{
    baidu_json *item = baidu_json_New_Item();
    if(item)
    {
        item->type=baidu_json_Array;
    }

    return item;
}

baidu_json *baidu_json_CreateObject(void)
{
    baidu_json *item = baidu_json_New_Item();
    if (item)
    {
        item->type = baidu_json_Object;
    }

    return item;
}

/* Create Arrays: */
baidu_json *baidu_json_CreateIntArray(const int *numbers, int count)
{
    int i;
    baidu_json *n = 0;
    baidu_json *p = 0;
    baidu_json *a = baidu_json_CreateArray();
    for(i = 0; a && (i < count); i++)
    {
        n = baidu_json_CreateNumber(numbers[i]);
        if (!n)
        {
            baidu_json_Delete(a);
            return 0;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    return a;
}

baidu_json *baidu_json_CreateFloatArray(const float *numbers, int count)
{
    int i;
    baidu_json *n = 0;
    baidu_json *p = 0;
    baidu_json *a = baidu_json_CreateArray();
    for(i = 0; a && (i < count); i++)
    {
        n = baidu_json_CreateNumber(numbers[i]);
        if(!n)
        {
            baidu_json_Delete(a);
            return 0;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    return a;
}

baidu_json *baidu_json_CreateDoubleArray(const double *numbers, int count)
{
    int i;
    baidu_json *n = 0;
    baidu_json *p = 0;
    baidu_json *a = baidu_json_CreateArray();
    for(i = 0;a && (i < count); i++)
    {
        n = baidu_json_CreateNumber(numbers[i]);
        if(!n)
        {
            baidu_json_Delete(a);
            return 0;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p, n);
        }
        p = n;
    }

    return a;
}

baidu_json *baidu_json_CreateStringArray(const char **strings, int count)
{
    int i;
    baidu_json *n = 0;
    baidu_json *p = 0;
    baidu_json *a = baidu_json_CreateArray();
    for (i = 0; a && (i < count); i++)
    {
        n = baidu_json_CreateString(strings[i], 0);
        if(!n)
        {
            baidu_json_Delete(a);
            return 0;
        }
        if(!i)
        {
            a->child = n;
        }
        else
        {
            suffix_object(p,n);
        }
        p = n;
    }

    return a;
}

/* Duplication */
baidu_json *baidu_json_Duplicate(const baidu_json *item, int recurse)
{
    baidu_json *newitem;
    baidu_json *cptr;
    baidu_json *nptr = 0;
    baidu_json *newchild;

    /* Bail on bad ptr */
    if (!item)
    {
        return 0;
    }
    /* Create new item */
    newitem = baidu_json_New_Item();
    if (!newitem)
    {
        return 0;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~baidu_json_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring)
    {
        newitem->valuestring = baidu_json_strdup(item->valuestring, 0);
        if (!newitem->valuestring)
        {
            baidu_json_Delete(newitem);
            return 0;
        }
    }
    if (item->string)
    {
        newitem->string = baidu_json_strdup(item->string, 0);
        if (!newitem->string)
        {
            baidu_json_Delete(newitem);
            return 0;
        }
    }
    /* If non-recursive, then we're done! memory owner change*/
    if (!recurse)
    {
        // old item type set to reference
        // the memory owner change to the newitem
        ((baidu_json*)item)->type |= baidu_json_IsReference;
        newitem->child = item->child;
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    cptr = item->child;
    while (cptr)
    {
        newchild = baidu_json_Duplicate(cptr, 1); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
        {
            baidu_json_Delete(newitem);
            return 0;
        }
        if (nptr)
        {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            nptr->next = newchild;
            newchild->prev = nptr;
            nptr = newchild;
        }
        else
        {
            /* Set newitem->child and move to it */
            newitem->child = newchild; nptr = newchild;
        }
        cptr = cptr->next;
    }

    return newitem;
}

void baidu_json_Minify(char *json)
{
    char *into = json;
    while (*json)
    {
        if (*json == ' ')
        {
            json++;
        }
        else if (*json == '\t')
        {
            /* Whitespace characters. */
            json++;
        }
        else if (*json == '\r')
        {
            json++;
        }
        else if (*json=='\n')
        {
            json++;
        }
        else if ((*json == '/') && (json[1] == '/'))
        {
            /* double-slash comments, to end of line. */
            while (*json && (*json != '\n'))
            {
                json++;
            }
        }
        else if ((*json == '/') && (json[1] == '*'))
        {
            /* multiline comments. */
            while (*json && !((*json == '*') && (json[1] == '/')))
            {
                json++;
            }
            json += 2;
        }
        else if (*json == '\"')
        {
            /* string literals, which are \" sensitive. */
            *into++ = *json++;
            while (*json && (*json != '\"'))
            {
                if (*json == '\\')
                {
                    *into++=*json++;
                }
                *into++ = *json++;
            }
            *into++ = *json++;
        }
        else
        {
            /* All other characters. */
            *into++ = *json++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}

void baidu_json_release(void *ptr)
{
    baidu_json_free(ptr);
}
