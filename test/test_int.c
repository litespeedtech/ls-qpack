#include <assert.h>
#include <stdlib.h>

#include "lsqpack.h"
#include "lsqpack-test.h"

struct int_test
{
    int             it_lineno;
    unsigned        it_prefix_bits;
    unsigned char   it_encoded[20];
    size_t          it_enc_sz;
    uint64_t        it_decoded;
    int             it_dec_retval;
};

static const struct int_test tests[] =
{

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0x7F, 0x02, },
        .it_enc_sz      = 2,
        .it_decoded     = 0x81,
        .it_dec_retval  = 0,
    },

};

int
main (void)
{
    const struct int_test *test;
    const unsigned char *src;
    uint64_t val;
    int rv;

    for (test = tests; test < tests + sizeof(tests) / sizeof(tests[0]); ++test)
    {
        src = test->it_encoded;
        rv = lsqpack_dec_int(&src, src + test->it_enc_sz,
                                                test->it_prefix_bits, &val);
        assert(rv == test->it_dec_retval);
        assert(val == test->it_decoded);
    }

    return 0;
}
