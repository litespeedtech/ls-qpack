/*
 * Regression test: the decoder must reject a header field whose declared name
 * or value length cannot be represented (greater than LSXPACK_MAX_STRLEN)
 * instead of asking the application to allocate a huge output buffer.
 *
 * The length fields are decoded with lsqpack_dec_int24, so a peer can declare
 * a length of up to ~16 MB using only a few bytes.  Before the fix the decoder
 * would forward such a length (multiplied by 1.5 for Huffman) to
 * dhi_prepare_decode, an allocation-amplification denial-of-service.  Now the
 * decoder rejects the block before requesting the buffer.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsqpack.h"
#include "lsqpack-test.h"
#include "lsxpack_header.h"

#define BUF_SZ 0x1000

struct hblock_ctx
{
    size_t                  max_space;  /* Largest space requested */
    struct lsxpack_header   xhdr;
    char                    buf[BUF_SZ];
};


static void
unblocked (void *p)
{
    assert(0);
}


static struct lsxpack_header *
prepare_decode (void *p, struct lsxpack_header *xhdr, size_t space)
{
    struct hblock_ctx *const hctx = p;

    if (space > hctx->max_space)
        hctx->max_space = space;
    if (space > sizeof(hctx->buf) || xhdr)
        return NULL;
    lsxpack_header_prepare_decode(&hctx->xhdr, hctx->buf, 0, sizeof(hctx->buf));
    return &hctx->xhdr;
}


static int
process_header (void *p, struct lsxpack_header *xhdr)
{
    return 0;
}


static const struct lsqpack_dec_hset_if hset_if = {
    .dhi_unblocked      = unblocked,
    .dhi_prepare_decode = prepare_decode,
    .dhi_process_header = process_header,
};


/* Feed `block' (size `sz') as a complete header block for `stream_id' and
 * return the status.  Records the largest space requested via *max_space_out.
 */
static enum lsqpack_read_header_status
decode_block (const unsigned char *block, size_t sz, uint64_t stream_id,
                                                    size_t *max_space_out)
{
    struct lsqpack_dec dec;
    struct hblock_ctx hctx;
    const unsigned char *p = block;
    enum lsqpack_read_header_status rhs;

    /* Generous maximum table capacity so length checks are about the field
     * size, not the table.
     */
    lsqpack_dec_init(&dec, stderr, 4096, 100, &hset_if,
                                            (enum lsqpack_dec_opts) 0);
    memset(&hctx, 0, sizeof(hctx));
    rhs = lsqpack_dec_header_in(&dec, &hctx, stream_id, sz, &p, sz, NULL, NULL);
    *max_space_out = hctx.max_space;
    lsqpack_dec_cleanup(&dec);
    return rhs;
}


int
main (void)
{
#if LSXPACK_MAX_STRLEN == UINT16_MAX
    unsigned char block[64];
    unsigned char *p, *const end = block + sizeof(block);
    enum lsqpack_read_header_status rhs;
    size_t max_space;
    /* Larger than LSXPACK_MAX_STRLEN but smaller than 2^24, so the integer
     * decoder accepts it and our explicit length check is what rejects it.
     */
    const unsigned bignum = 1000000;

    assert(bignum > LSXPACK_MAX_STRLEN);

    /* Case 1: Literal Field Line With Name Reference (static name index 0)
     * followed by an over-long value length.
     */
    p = block;
    *p++ = 0x00;            /* Required Insert Count = 0 */
    *p++ = 0x00;            /* Base delta = 0 */
    *p++ = 0x40 | 0x10 | 0; /* LFINR, static, name index 0 (4-bit prefix) */
    *p   = 0x00;            /* value: Huffman bit clear; 7-bit length prefix */
    p = lsqpack_enc_int(p, end, bignum, 7);
    assert(p > block && p <= end);
    rhs = decode_block(block, (size_t) (p - block), 1, &max_space);
    assert(rhs == LQRHS_ERROR);
    /* Only the (small) static name buffer should have been requested. */
    assert(max_space <= LSXPACK_MAX_STRLEN);
    assert(max_space < bignum);

    /* Case 2: Literal Field Line Without Name Reference with an over-long
     * name length.
     */
    p = block;
    *p++ = 0x00;            /* Required Insert Count = 0 */
    *p++ = 0x00;            /* Base delta = 0 */
    *p   = 0x20;            /* LFONR, not-never, not-huffman; 3-bit prefix */
    p = lsqpack_enc_int(p, end, bignum, 3);
    assert(p > block && p <= end);
    rhs = decode_block(block, (size_t) (p - block), 1, &max_space);
    assert(rhs == LQRHS_ERROR);
    assert(max_space < bignum);

    printf("header alloc clamp test ok\n");
#else
    printf("test skipped: LSXPACK_MAX_STRLEN = %lld\n",
                                    (unsigned long long) LSXPACK_MAX_STRLEN);
#endif
    return 0;
}
