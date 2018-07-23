/*
 * Test QPACK.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsqpack.h"

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
        .qhbt_table_size    = LSQPACK_DEF_DYN_TABLE_SIZE,
        .qhbt_max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "GET", 0, },
        },
        .qhbt_enc_sz        = 0,
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 1,
        .qhbt_header_buf    = {
            0x80 | 0x40 | 2,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = LSQPACK_DEF_DYN_TABLE_SIZE,
        .qhbt_max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "method", LQEF_NO_INDEX, },
        },
        .qhbt_enc_sz        = 0,
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 7,
        .qhbt_header_buf    = {
            0x40 | 0x20 | 0x10 | 2,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = LSQPACK_DEF_DYN_TABLE_SIZE,
        .qhbt_max_risked_streams = 0,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "method", 0, },
        },
        .qhbt_enc_sz        = 7,
        .qhbt_enc_buf       = {
            0x80 | 0x40 /* static */ | 2 /* name index */,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x00\x00",
        .qhbt_header_sz     = 7,
        .qhbt_header_buf    = {
            0x40 | 0x10 /* Static */ | 2,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
    },

    {
        .qhbt_lineno        = __LINE__,
        .qhbt_table_size    = LSQPACK_DEF_DYN_TABLE_SIZE,
        .qhbt_max_risked_streams = LSQPACK_DEF_DYN_TABLE_SIZE,
        .qhbt_n_headers     = 1,
        .qhbt_headers       = {
            { ":method", "method", 0, },
        },
        .qhbt_enc_sz        = 7,
        .qhbt_enc_buf       = {
            0x80 | 0x40 /* static */ | 2 /* name index */,
            0x80 /* Huffman */ | 5 /* length */,
            0xa4, 0xa9, 0x9c, 0xf2, 0x7f,
        },
        .qhbt_prefix_sz     = 2,
        .qhbt_prefix_buf    = "\x01\x81",
        .qhbt_header_sz     = 1,
        .qhbt_header_buf    = {
            0x80 | 1 /* New dynamic ID */,
        },
    },

};


static void
run_header_test (const struct qpack_header_block_test *test)
{
    unsigned char header_buf[HEADER_BUF_SZ], enc_buf[ENC_BUF_SZ],
        prefix_buf[PREFIX_BUF_SZ];
    unsigned header_off, enc_off;
    size_t header_sz, enc_sz;
    struct lsqpack_enc enc;
    unsigned i;
    size_t nw;
    int s;
    enum lsqpack_enc_status enc_st;

    s = lsqpack_enc_init(&enc, test->qhbt_table_size,
                                            test->qhbt_max_risked_streams);
    assert(s == 0);

    s = lsqpack_enc_start_header(&enc, 0, 0);
    assert(s == 0);

    header_off = 0, enc_off = 0;
    for (i = 0; i < test->qhbt_n_headers; ++i)
    {
        enc_sz = sizeof(enc_buf) - enc_off;
        header_sz = sizeof(header_buf) - header_off;
        enc_st = lsqpack_enc_encode(&enc,
                enc_buf + enc_off, &enc_sz,
                header_buf + header_off, &header_sz,
                test->qhbt_headers[i].name,
                strlen(test->qhbt_headers[i].name),
                test->qhbt_headers[i].value,
                strlen(test->qhbt_headers[i].value),
                test->qhbt_headers[i].flags);
        assert(enc_st == LQES_OK);
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

    lsqpack_enc_cleanup(&enc);
}


int
main (void)
{
    unsigned i;

    for (i = 0; i < sizeof(header_block_tests)
                                / sizeof(header_block_tests[0]); ++i)
        run_header_test(&header_block_tests[i]);

    return 0;
}
