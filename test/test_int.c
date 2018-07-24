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

    /* RFC 7541, Appendinx C.1.1 */
    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 5,
        .it_encoded     = { 0b1010, 0x02, },
        .it_enc_sz      = 1,
        .it_decoded     = 10,
        .it_dec_retval  = 0,
    },

    /* RFC 7541, Appendinx C.1.2 */
    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 5,
        .it_encoded     = { 0b11111, 0b10011010, 0b00001010, },
        .it_enc_sz      = 3,
        .it_decoded     = 1337,
        .it_dec_retval  = 0,
    },

    /* RFC 7541, Appendinx C.1.3 */
    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 8,
        .it_encoded     = { 0b101010, },
        .it_enc_sz      = 1,
        .it_decoded     = 42,
        .it_dec_retval  = 0,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b10000001, 0b10000010, 0b00000011, },
        .it_enc_sz      = 4,
                       /*     01234560123456 */
        .it_decoded     = 0b1100000100000001    + 0b1111111,
        .it_dec_retval  = 0,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b10000001, 0b10000010, 0b10000011,
                            0b00000011, },
        .it_enc_sz      = 5,
                       /*     012345601234560123456 */
        .it_decoded     = 0b11000001100000100000001    + 0b1111111,
        .it_dec_retval  = 0,
    },

};

int
main (void)
{
    const struct int_test *test;
    const unsigned char *src;
    uint64_t val;
    size_t sz;
    int rv;

    for (test = tests; test < tests + sizeof(tests) / sizeof(tests[0]); ++test)
    {
        for (sz = 1; sz < test->it_enc_sz; ++sz)
        {
            src = test->it_encoded;
            rv = lsqpack_dec_int(&src, src + sz, test->it_prefix_bits, &val);
            assert(-1 == rv);
        }
        for (; sz < sizeof(test->it_encoded); ++sz)
        {
            src = test->it_encoded;
            rv = lsqpack_dec_int(&src, src + sz, test->it_prefix_bits, &val);
            assert(rv == test->it_dec_retval);
            assert(val == test->it_decoded);
        }
    }

    return 0;
}
