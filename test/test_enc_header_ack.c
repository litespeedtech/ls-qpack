/*
 * Regression test: a Header Ack instruction sent by an adversarial peer for
 * a stream whose header block is still being encoded must be rejected and
 * must not release the in-progress header info (enc->qpe_cur_header.hinfo).
 *
 * Before the fix, enc_proc_header_ack() matched the in-progress hinfo by its
 * stream ID and accepted the invalid ack, calling enc_free_hinfo() on it.
 * enc_free_hinfo() does not free heap memory -- it returns the slot to the
 * hinfo pool and unlinks it from qpe_all_hinfos -- but qpe_cur_header.hinfo
 * keeps pointing at the recycled slot, so continued encoding corrupts encoder
 * accounting and end_header() releases the same slot a second time.
 *
 * The bug is observable here as enc_proc_header_ack() wrongly returning 0
 * (ack accepted) instead of -1; the assertions below check that the ack is
 * rejected and that encoding continues consistently.  (This is encoder state
 * corruption, not a heap use-after-free, so it does not depend on ASan.)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsqpack.h"
#include "lsqpack-test.h"
#include "lsxpack_header.h"

#define ENC_BUF_SZ 200
#define HEADER_BUF_SZ 200
#define PREFIX_BUF_SZ 10

struct header_buf
{
    unsigned    off;
    char        buf[UINT16_MAX];
};


static int
header_set_ptr (struct lsxpack_header *hdr, struct header_buf *header_buf,
                const char *name, size_t name_len,
                const char *val, size_t val_len)
{
    if (header_buf->off + name_len + val_len <= sizeof(header_buf->buf))
    {
        memcpy(header_buf->buf + header_buf->off, name, name_len);
        memcpy(header_buf->buf + header_buf->off + name_len, val, val_len);
        lsxpack_header_set_offset2(hdr, header_buf->buf + header_buf->off,
                                            0, name_len, name_len, val_len);
        header_buf->off += name_len + val_len;
        return 0;
    }
    else
        return -1;
}


/* Encode one header (forcing a dynamic-table insertion) into the currently
 * open header block.
 */
static void
encode_one (struct lsqpack_enc *enc, struct header_buf *hbuf,
            const char *name, const char *value)
{
    unsigned char enc_buf[ENC_BUF_SZ], header_buf[HEADER_BUF_SZ];
    struct lsxpack_header xhdr;
    enum lsqpack_enc_status enc_st;
    size_t enc_sz, header_sz;

    enc_sz = sizeof(enc_buf);
    header_sz = sizeof(header_buf);
    hbuf->off = 0;
    header_set_ptr(&xhdr, hbuf, name, strlen(name), value, strlen(value));
    enc_st = lsqpack_enc_encode(enc, enc_buf, &enc_sz, header_buf, &header_sz,
                                                                    &xhdr, 0);
    assert(LQES_OK == enc_st);
}


/* Build and feed a Header Ack instruction for `stream_id'. */
static int
feed_header_ack (struct lsqpack_enc *enc, uint64_t stream_id)
{
    unsigned char buf[16];
    unsigned char *end;

    buf[0] = 0x80;
    end = lsqpack_enc_int(buf, buf + sizeof(buf), stream_id, 7);
    assert(end > buf);
    return lsqpack_enc_decoder_in(enc, buf, (size_t) (end - buf));
}


int
main (void)
{
    struct lsqpack_enc enc;
    unsigned char dec_buf[LSQPACK_LONGEST_SDTC];
    unsigned char prefix_buf[PREFIX_BUF_SZ];
    struct header_buf hbuf;
    size_t dec_sz;
    ssize_t nw;
    int s;

    dec_sz = sizeof(dec_buf);
    s = lsqpack_enc_init(&enc, stderr, 0x1000, 0x1000, 100,
                            LSQPACK_ENC_OPT_IX_AGGR, dec_buf, &dec_sz);
    assert(0 == s);
    (void) dec_sz;

    /* Begin encoding a header block on stream 1.  This registers the
     * in-progress header info with stream ID 1 on enc->qpe_all_hinfos.
     */
    s = lsqpack_enc_start_header(&enc, 1, 0);
    assert(0 == s);
    encode_one(&enc, &hbuf, ":method", "dude!");

    /* Adversarial peer acknowledges stream 1 while we are still encoding it.
     * The only header info matching stream 1 is the in-progress one, so the
     * ACK is invalid and must be rejected -- crucially, without releasing the
     * in-progress header info.
     */
    s = feed_header_ack(&enc, 1);
    assert(0 != s);     /* ACK for an unfinished header block is rejected */

    /* The encoder must still be in a consistent state: keep using the
     * in-progress header block.  With the pre-fix bug, this hinfo slot has
     * been returned to the pool, so continued encoding corrupts encoder state.
     */
    encode_one(&enc, &hbuf, ":path", "/where/is/my/car");
    nw = lsqpack_enc_end_header(&enc, prefix_buf, sizeof(prefix_buf), NULL);
    assert(nw > 0);

    /* And we can start and finish another header block normally afterwards. */
    s = lsqpack_enc_start_header(&enc, 3, 0);
    assert(0 == s);
    encode_one(&enc, &hbuf, ":scheme", "https");
    nw = lsqpack_enc_end_header(&enc, prefix_buf, sizeof(prefix_buf), NULL);
    assert(nw > 0);

    lsqpack_enc_cleanup(&enc);

    printf("enc header ack test ok\n");
    return 0;
}
