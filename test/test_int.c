#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b10000001, 0b10000010, 0b10000011,
                            0b10000100, 0b10000101, 0b10000110, 0b10000111,
                            0b10001000,
                            0b00000011, },
        .it_enc_sz      = 10,
                       /*     01234560123456012345601234560123456012345601234560123456 */
        .it_decoded     = 0b1100010000000111000011000001010000100000001100000100000001
                                + 0b1111111,
        .it_dec_retval  = 0,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b10000001, 0b10000010, 0b10000011,
                            0b10000100, 0b10000101, 0b10000110, 0b10000111,
                            0b10001000, 0b10001001,
                            0b00000001, },
        .it_enc_sz      = 11,
                       /*    012345601234560123456012345601234560123456012345601234560123456 */
        .it_decoded     = 0b1000100100010000000111000011000001010000100000001100000100000001
                                + 0b1111111,
        .it_dec_retval  = 0,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b10000000, 0b11111111, 0b11111111,
                            0b11111111, 0b11111111, 0b11111111, 0b11111111,
                            0b11111111, 0b11111111,
                            0b00000001, },
        .it_enc_sz      = 11,
        .it_decoded     = UINT64_MAX,
        .it_dec_retval  = 0,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
            /* Same as above, but with extra bit that overflows it */
                                      /* ----v---- */
        .it_encoded     = { 0b01111111, 0b10010000, 0b11111111, 0b11111111,
                            0b11111111, 0b11111111, 0b11111111, 0b11111111,
                            0b11111111, 0b11111111,
                            0b00000001, },
        .it_enc_sz      = 11,
        .it_dec_retval  = -2,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 8,
        .it_encoded     = { 0b11111111, 0b10000001, 0b10000010, 0b10000011,
                            0b10000100, 0b10000101, 0b10000110, 0b10000111,
                            0b10001000, 0b10001001, 0b00000001, },
        .it_enc_sz      = 11,
                       /*    012345601234560123456012345601234560123456012345601234560123456 */
        .it_decoded     = 0b1000100100010000000111000011000001010000100000001100000100000001
                                + 0b11111111,
        .it_dec_retval  = 0,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b11101111, 0b11111111, 0b11111111,
                            0b11111111, 0b11111111, 0b11111111, 0b11111111,
                            0b11111111, 0b11111111,
                            0b00000001, },
        .it_enc_sz      = 11,
        .it_dec_retval  = -2,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0b01111111, 0b10000001, 0b10000010, 0b10000011,
                            0b10000100, 0b10000101, 0b10000110, 0b10000111,
                            0b10001000, 0b10001001,
                            0b00000011, },
        .it_enc_sz      = 11,
        .it_dec_retval  = -2,
    },

    {   .it_lineno      = __LINE__,
        .it_prefix_bits = 7,
        .it_encoded     = { 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                            0xFF, 0xFF, 0xFF, },
        .it_enc_sz      = 11,
        .it_dec_retval  = -2,
    },

};

int
main (void)
{
    const struct int_test *test;
    const unsigned char *src;
    unsigned char *dst;
    unsigned char buf[ sizeof(((struct int_test *) NULL)->it_encoded) ];
    uint64_t val;
    size_t sz;
    int rv;

    /* Test the decoder */
    for (test = tests; test < tests + sizeof(tests) / sizeof(tests[0]); ++test)
    {
        struct lsqpack_dec_int_state state;
        state.resume = 0;
        for (sz = 0; sz < test->it_enc_sz - 1; ++sz)
        {
            src = test->it_encoded + sz;
            rv = lsqpack_dec_int(&src, src + 1, test->it_prefix_bits,
                                                                &val, &state);
            assert(-1 == rv);
        }
        src = test->it_encoded + sz;
        rv = lsqpack_dec_int(&src, src + 1, test->it_prefix_bits,
                                                            &val, &state);
        assert(rv == test->it_dec_retval);
        if (0 == rv)
            assert(val == test->it_decoded);
    }

    /* Test the encoder */
    for (test = tests; test < tests + sizeof(tests) / sizeof(tests[0]); ++test)
    {
        if (test->it_dec_retval != 0)
            continue;
        for (sz = 1; sz < test->it_enc_sz; ++sz)
        {
            dst = lsqpack_enc_int(buf, buf + sz, test->it_decoded,
                                                        test->it_prefix_bits);
            assert(dst == buf);     /* Not enough room */
        }
        for (; sz <= sizeof(buf); ++sz)
        {
            buf[0] = '\0';
            dst = lsqpack_enc_int(buf, buf + sz, test->it_decoded,
                                                        test->it_prefix_bits);
            assert(dst - buf == (intptr_t) test->it_enc_sz);
            assert(0 == memcmp(buf, test->it_encoded, test->it_enc_sz));
        }
    }

    return 0;
}
