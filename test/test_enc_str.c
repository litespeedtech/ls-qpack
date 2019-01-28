#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "lsqpack.h"
#include "lsqpack-test.h"


struct str_test
{
    int                      strt_lineno;
    unsigned                 strt_prefix_bits;
    const unsigned char     *strt_in_str;
    unsigned                 strt_in_len;
    const unsigned char     *strt_out_buf;
    int                      strt_retval;
};


static const struct str_test tests[] =
{

    {
        __LINE__,
        3,
        (unsigned char *) "",
        0,
        (unsigned char *) "\x00",
        1,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "a",
        1,
        (unsigned char *) "\x01" "a",
        2,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "aa",
        2,
        (unsigned char *) "\x02" "aa",
        3,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "aaa",
        3,
        (unsigned char *) "\x0A\x18\xc7",
        3,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "aaaaaaaaa",
        9,
        (unsigned char *) "\x0E\x18\xc6\x31\x8c\x63\x1f",
        7,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "aaaaaaaaaa",
        10,
        (unsigned char *) "\x0F\x00\x18\xc6\x31\x8c\x63\x18\xff",
        9,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "aaaaaaaaaabbb",
        13,
        (unsigned char *) "\x0F\x02\x18\xc6\x31\x8c\x63\x18\xe3\x8e\x3f",
        11,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "\x80\x90\xA0\xBA",
        4,
        (unsigned char *) "\x04\x80\x90\xA0\xBA",
        5,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "\x80\x90\xA0\xBA\x80\x90\xA0",
        7,
        (unsigned char *) "\x07\x00\x80\x90\xA0\xBA\x80\x90\xA0",
        9,
    },

    {
        __LINE__,
        3,
        (unsigned char *) "\x80\x90\xA0\xBA\x80\x90\xA0" "foo",
        10,
        (unsigned char *) "\x07\x03\x80\x90\xA0\xBA\x80\x90\xA0" "foo",
        12,
    },

};


int
main (void)
{
    const struct str_test *test;
    int r;
    unsigned char *out= malloc(0x1000);

    if (!out)
        return 1;

    for (test = tests; test < tests + sizeof(tests) / sizeof(tests[0]); ++test)
    {
        out[0] = 0;
        r = lsqpack_enc_enc_str(test->strt_prefix_bits, out, 0x1000,
                                         test->strt_in_str, test->strt_in_len);
        assert(r == test->strt_retval);
        if (r > 0)
            assert(0 == memcmp(test->strt_out_buf, out, r));
    }

    free(out);
    return 0;
}
