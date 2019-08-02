/*
 * Test QPACK.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsqpack.h"
#include "lsqpack-test.h"

#define ENC_BUF_SZ 200
#define HEADER_BUF_SZ 200
#define PREFIX_BUF_SZ 10

static const struct qpack_header_block_test
{
    /* Identification: */
    int             qhbt_lineno;

    /* Input: */
    unsigned        qhbt_table_size;
    unsigned        qhbt_max_risked_streams;
    unsigned        qhbt_n_headers;
    struct {
        const char *name, *value;
        enum lsqpack_enc_flags flags;
    }               qhbt_headers[10];

    /* Output: */
    size_t          qhbt_enc_sz;
    unsigned char   qhbt_enc_buf[ENC_BUF_SZ];

    size_t          qhbt_prefix_sz;
    unsigned char   qhbt_prefix_buf[PREFIX_BUF_SZ];

    size_t          qhbt_header_sz;
    unsigned char   qhbt_header_buf[HEADER_BUF_SZ];
} header_block_tests[] =
{

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 100,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "GET", 0, },
        },
        .qhbt_enc_sz        = 0,
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 1,
        .qhbt_header_buf    = {
            0x80 | 0x40 | 17,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 100,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "method", LQEF_NEVER_INDEX, },
        },
        .qhbt_enc_sz        = 0,
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 8,
        .qhbt_header_buf    = {
            0x40 | 0x20 | 0x10 | 0x0F, 0x00,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 0,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "method", 0, },
        },
        .qhbt_enc_sz        = 7,
        .qhbt_enc_buf       = {
            0x80 | 0x40 /* static */ | 15 /* name index */,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 8,
        .qhbt_header_buf    = {
            0x40 | 0x10 /* Static */ | 15, 0x00,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 0x1000,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "method", 0, },
        },
        .qhbt_enc_sz        = 7,
        .qhbt_enc_buf       = {
            0x80 | 0x40 /* static */ | 15 /* name index */,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = { 0x02, 0x80, },
        .qhbt_header_sz     = 1,
        .qhbt_header_buf    = {
            0x10 | 0 /* Relative dynamic ID */,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 0x1000,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { "dude", "where is my car?", LQEF_NEVER_INDEX, },
        },
        .qhbt_enc_sz        = 0,
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 17,
        .qhbt_header_buf    = {
            0x20 | 0x10 /* No index */ | 0x08 | 3,
            0x92, 0xd9, 0x0b,
            0x80 | 0xC,
            0xf1, 0x39, 0x6c, 0x2a, 0x86, 0x42, 0x94, 0xfa,
            0x50, 0x83, 0xb3, 0xfc,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 0,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { "dude", "where is my car?", 0, },
        },
        .qhbt_enc_sz        = 17,
        .qhbt_enc_buf       = {
            0x40 | 0x20 | 3,
            0x92, 0xd9, 0x0b,
            0x80 | 0xC,
            0xf1, 0x39, 0x6c, 0x2a, 0x86, 0x42, 0x94, 0xfa,
            0x50, 0x83, 0xb3, 0xfc,
        },
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 17,
        .qhbt_header_buf    = {
            0x20 | 0x08 | 3,
            0x92, 0xd9, 0x0b,
            0x80 | 0xC,
            0xf1, 0x39, 0x6c, 0x2a, 0x86, 0x42, 0x94, 0xfa,
            0x50, 0x83, 0xb3, 0xfc,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = 0x1000,
        .qhbt_max_risked_streams = 1,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { "dude", "where is my car?", 0, },
        },
        .qhbt_enc_sz        = 17,
        .qhbt_enc_buf       = {
            0x40 | 0x20 | 3,
            0x92, 0xd9, 0x0b,
            0x80 | 0xC,
            0xf1, 0x39, 0x6c, 0x2a, 0x86, 0x42, 0x94, 0xfa,
            0x50, 0x83, 0xb3, 0xfc,
        },
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = { 0x02, 0x80, },
        .qhbt_header_sz     = 1,
        .qhbt_header_buf    = {
            0x10 | 0 /* Relative dynamic ID */,
        },
    },

};


static void
run_header_test (const struct qpack_header_block_test *test)
{
    unsigned char header_buf[HEADER_BUF_SZ], enc_buf[ENC_BUF_SZ],
        prefix_buf[PREFIX_BUF_SZ];
    unsigned header_off, enc_off;
    size_t header_sz, enc_sz, dec_sz;
    struct lsqpack_enc enc;
    unsigned i;
    size_t nw;
    int s;
    enum lsqpack_enc_status enc_st;
    float ratio;
    unsigned char dec_buf[LSQPACK_LONGEST_SDTC];

    dec_sz = sizeof(dec_buf);
    s = lsqpack_enc_init(&enc, stderr, test->qhbt_table_size,
                test->qhbt_table_size, test->qhbt_max_risked_streams,
                LSQPACK_ENC_OPT_IX_AGGR, dec_buf, &dec_sz);
    assert(s == 0);

    s = lsqpack_enc_start_header(&enc, 0, 0);
    assert(s == 0);

    header_off = 0, enc_off = 0;
    for (i = 0; i < test->qhbt_n_headers; ++i)
    {
        /* Increase buffers one by one to exercise error conditions */
        enc_sz = 0;
        header_sz = 0;
        while (1)
        {
            enc_st = lsqpack_enc_encode(&enc,
                    enc_buf + enc_off, &enc_sz,
                    header_buf + header_off, &header_sz,
                    test->qhbt_headers[i].name,
                    strlen(test->qhbt_headers[i].name),
                    test->qhbt_headers[i].value,
                    strlen(test->qhbt_headers[i].value),
                    test->qhbt_headers[i].flags);
            switch (enc_st)
            {
            case LQES_NOBUF_ENC:
                if (enc_sz < sizeof(enc_buf) - enc_off)
                    ++enc_sz;
                else
                    assert(0);
                break;
            case LQES_NOBUF_HEAD:
                if (header_sz < sizeof(header_buf) - header_off)
                    ++header_sz;
                else
                    assert(0);
                break;
            default:
                assert(enc_st == LQES_OK);
                goto end_encode_one_header;
            }
        }
  end_encode_one_header:
        enc_off += enc_sz;
        header_off += header_sz;
    }

    nw = lsqpack_enc_end_header(&enc, prefix_buf, sizeof(prefix_buf));
    assert(nw == test->qhbt_prefix_sz);
    assert(0 == memcmp(test->qhbt_prefix_buf, prefix_buf, nw));
    assert(enc_off == test->qhbt_enc_sz);
    assert(0 == memcmp(test->qhbt_enc_buf, enc_buf, enc_off));
    assert(header_off == test->qhbt_header_sz);
    assert(0 == memcmp(test->qhbt_header_buf, header_buf, header_off));
    assert(lsqpack_enc_ratio(&enc) > 0.0);

    lsqpack_enc_cleanup(&enc);
}

static void
run_header_cancellation_test(const struct qpack_header_block_test *test) {
    unsigned char header_buf[HEADER_BUF_SZ];
    size_t header_sz, enc_sz;
    struct lsqpack_enc enc;
    int s;
    enum lsqpack_enc_status enc_st;

    s = lsqpack_enc_init(&enc, stderr, 0, 0, test->qhbt_max_risked_streams, 
                         LSQPACK_ENC_OPT_IX_AGGR, NULL, NULL);
    assert(s == 0);

    s = lsqpack_enc_start_header(&enc, 0, 0);
    assert(s == 0);

    header_sz = HEADER_BUF_SZ;
    enc_sz = 0;

    enc_st = lsqpack_enc_encode(&enc,
                    NULL, &enc_sz,
                    header_buf, &header_sz,
                    test->qhbt_headers[0].name,
                    strlen(test->qhbt_headers[0].name),
                    test->qhbt_headers[0].value,
                    strlen(test->qhbt_headers[0].value),
                    0);
    assert(enc_st == LQES_OK);

    s = lsqpack_enc_cancel_header(&enc);
    assert(s == 0);

    /* Check that we can start again after cancelling. */
    s = lsqpack_enc_start_header(&enc, 0, 0);
    assert(s == 0);

    lsqpack_enc_cleanup(&enc);
}


static void
test_enc_init (void)
{
    struct lsqpack_enc enc;
    size_t dec_sz;
    int s;
    unsigned i;
    const unsigned char *p;
    uint64_t val;
    struct lsqpack_dec_int_state state;
    unsigned char dec_buf[LSQPACK_LONGEST_SDTC];

    const struct {
        unsigned    peer_max_size;  /* Value provided by peer */
        unsigned    our_max_size;   /* Value to use */
        int         expected_tsu;   /* Expecting TSU instruction? */
    } tests[] = {
        {   0x1000,     0x1000,     1,  },
        {   0x1000,     1,          1,  },
        {    0x100,     0x100,      1,  },
        {   0x1000,     0,          0,  },
    };

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
    {
        dec_sz = sizeof(dec_buf);
        s = lsqpack_enc_init(&enc, stderr, tests[i].peer_max_size,
                        tests[i].our_max_size, 0, 0, dec_buf, &dec_sz);
        assert(s == 0);
        if (tests[i].expected_tsu)
        {
            assert(dec_sz > 0);
            assert((dec_buf[0] & 0xE0) == 0x20);
            p = dec_buf;
            state.resume = 0;
            s = lsqpack_dec_int(&p, p + dec_sz, 5, &val, &state);
            assert(s == 0);
            assert(val == tests[i].our_max_size);
        }
        else
            assert(dec_sz == 0);
        lsqpack_enc_cleanup(&enc);
    }
}


/* Test that push promise header does not use the dynamic table, nor does
 * it update history.
 */
static void
test_push_promise (void)
{
    struct lsqpack_enc enc;
    ssize_t nw;
    enum lsqpack_enc_status enc_st;
    int s;
    unsigned i;
    const unsigned char *p;
    uint64_t val;
    struct lsqpack_dec_int_state state;
    unsigned char dec_buf[LSQPACK_LONGEST_SDTC];
    unsigned char header_buf[HEADER_BUF_SZ], enc_buf[ENC_BUF_SZ],
        prefix_buf[PREFIX_BUF_SZ];
    size_t header_sz, enc_sz, dec_sz;

    dec_sz = sizeof(dec_buf);
    s = lsqpack_enc_init(&enc, stderr, 0x1000, 0x1000, 100, 0, dec_buf, &dec_sz);
    assert(0 == s);

    (void) dec_sz;  /* We don't care for this test */

    s = lsqpack_enc_start_header(&enc, 0, 0);
    assert(0 == s);
    enc_sz = sizeof(enc_buf);
    header_sz = sizeof(header_buf);
    enc_st = lsqpack_enc_encode(&enc,
            enc_buf, &enc_sz, header_buf, &header_sz,
            ":method", 7, "dude!", 5, 0);
    assert(LQES_OK == enc_st);
    enc_sz = sizeof(enc_buf);
    header_sz = sizeof(header_buf);
    enc_st = lsqpack_enc_encode(&enc,
            enc_buf, &enc_sz, header_buf, &header_sz,
            ":method", 7, "dude!", 5, 0);
    assert(LQES_OK == enc_st);
    nw = lsqpack_enc_end_header(&enc, prefix_buf, sizeof(prefix_buf));
    assert(2 == nw);
    assert(!(prefix_buf[0] == 0 && prefix_buf[1] == 0)); /* Dynamic table used */

    s = lsqpack_enc_start_header(&enc, 0, 0);
    assert(0 == s);
    enc_sz = sizeof(enc_buf);
    header_sz = sizeof(header_buf);
    enc_st = lsqpack_enc_encode(&enc,
            enc_buf, &enc_sz, header_buf, &header_sz,
            ":method", 7, "dude!", 5, LQEF_NO_HIST_UPD|LQEF_NO_DYN);
    assert(LQES_OK == enc_st);
    enc_sz = sizeof(enc_buf);
    header_sz = sizeof(header_buf);
    enc_st = lsqpack_enc_encode(&enc,
            enc_buf, &enc_sz, header_buf, &header_sz,
            ":method", 7, "where is my car?", 16, LQEF_NO_HIST_UPD|LQEF_NO_DYN);
    nw = lsqpack_enc_end_header(&enc, prefix_buf, sizeof(prefix_buf));
    assert(2 == nw);
    assert(prefix_buf[0] == 0 && prefix_buf[1] == 0); /* Dynamic table not used */

    /* Last check that history was not updated: */
    s = lsqpack_enc_start_header(&enc, 4, 0);
    assert(0 == s);
    enc_sz = sizeof(enc_buf);
    header_sz = sizeof(header_buf);
    enc_st = lsqpack_enc_encode(&enc,
            enc_buf, &enc_sz, header_buf, &header_sz,
            ":method", 7, "where is my car?", 16, 0);
    assert(enc_sz == 0);
    assert(LQES_OK == enc_st);
    nw = lsqpack_enc_end_header(&enc, prefix_buf, sizeof(prefix_buf));
    assert(2 == nw);
    assert(prefix_buf[0] == 0 && prefix_buf[1] == 0); /* Dynamic table not used */

    lsqpack_enc_cleanup(&enc);
}


static void
test_discard_header (int err)
{
    struct lsqpack_dec dec;
    enum lsqpack_read_header_status rhs;
    const unsigned char *buf;
    struct lsqpack_header_list *hlist = NULL;
    unsigned char header_block[] = "\x00\x00\xC0\x80";

    lsqpack_dec_init(&dec, NULL, 0, 0, NULL);

    buf = header_block;
    rhs = lsqpack_dec_header_in(&dec, (void *) 1, 0, 10,
                                    &buf, 3 + !!err, &hlist, NULL, NULL);
    if (err)
    {
        assert(hlist == NULL);
        assert(LQRHS_ERROR == rhs);
    }
    else
    {
        assert(hlist == NULL);
        assert(3 == buf - header_block);
        assert(LQRHS_NEED == rhs);
        lsqpack_dec_cleanup(&dec);
    }
}


static void
test_static_bounds_header_block (void)
{
    struct lsqpack_dec dec;
    enum lsqpack_read_header_status rhs;
    const unsigned char *buf;
    struct lsqpack_header_list *hlist = NULL;
    /* Static table index 1000 */
    unsigned char header_block[] = "\x00\x00\xFF\xA9\x07";

    lsqpack_dec_init(&dec, stderr, 0, 0, NULL);
    buf = header_block;
    rhs = lsqpack_dec_header_in(&dec, (void *) 1, 0, 10,
                                    &buf, 5, &hlist, NULL, NULL);
    assert(hlist == NULL);
    assert(LQRHS_ERROR == rhs);
    lsqpack_dec_cleanup(&dec);
}


static void
test_static_bounds_enc_stream (void)
{
    struct lsqpack_dec dec;
    int r;
    /* Static table index 1000 */
    unsigned char enc_stream[] = "\xFF\xA9\x07\x04" "dude";

    lsqpack_dec_init(&dec, stderr, 0, 0, NULL);
    r = lsqpack_dec_enc_in(&dec, enc_stream, 8);
    assert(r == -1);
    lsqpack_dec_cleanup(&dec);
}


int
main (void)
{
    unsigned i;

    for (i = 0; i < sizeof(header_block_tests)
                                / sizeof(header_block_tests[0]); ++i)
        run_header_test(&header_block_tests[i]);

    run_header_cancellation_test(&header_block_tests[0]);
    test_enc_init();
    test_push_promise();
    test_discard_header(0);
    test_discard_header(1);
    test_static_bounds_header_block();
    test_static_bounds_enc_stream();

    return 0;
}
