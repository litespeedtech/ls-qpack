/*
 * Regression test: the encoder must reject an Insert Count Increment (ICI)
 * instruction whose value would overflow the 32-bit accumulation in
 * enc_proc_ici().
 *
 * The running acknowledged-insert count is computed as
 *     (lsqpack_abs_id_t) ins_count + qpe_last_ici
 * in 32 bits.  With a non-zero qpe_last_ici, a peer could send an ICI close
 * to UINT_MAX so that the sum wrapped to a small value, slipping past the
 * "larger than number of inserts" check and being silently accepted instead
 * of rejected.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lsqpack.h"
#include "lsqpack-test.h"
#include "lsxpack_header.h"

#define BUF_SZ 256


static int
feed_ici (struct lsqpack_enc *enc, uint64_t increment)
{
    unsigned char buf[16];
    unsigned char *end;

    buf[0] = 0x00;      /* Insert Count Increment: top two bits 00 */
    end = lsqpack_enc_int(buf, buf + sizeof(buf), increment, 6);
    assert(end > buf);
    return lsqpack_enc_decoder_in(enc, buf, (size_t) (end - buf));
}


int
main (void)
{
    struct lsqpack_enc enc;
    unsigned char dec_buf[LSQPACK_LONGEST_SDTC];
    unsigned char enc_buf[BUF_SZ], hdr_buf[BUF_SZ], prefix[16];
    char namebuf[64];
    struct lsxpack_header xhdr;
    enum lsqpack_enc_status est;
    size_t dec_sz, enc_sz, hdr_sz;
    ssize_t nw;
    int s;

    dec_sz = sizeof(dec_buf);
    s = lsqpack_enc_init(&enc, stderr, 4096, 4096, 100,
                            LSQPACK_ENC_OPT_IX_AGGR, dec_buf, &dec_sz);
    assert(0 == s);

    /* Insert exactly one entry into the dynamic table. */
    s = lsqpack_enc_start_header(&enc, 1, 0);
    assert(0 == s);
    memcpy(namebuf, ":method", 7);
    memcpy(namebuf + 7, "dude!", 5);
    lsxpack_header_set_offset2(&xhdr, namebuf, 0, 7, 7, 5);
    enc_sz = sizeof(enc_buf);
    hdr_sz = sizeof(hdr_buf);
    est = lsqpack_enc_encode(&enc, enc_buf, &enc_sz, hdr_buf, &hdr_sz, &xhdr, 0);
    assert(LQES_OK == est);
    nw = lsqpack_enc_end_header(&enc, prefix, sizeof(prefix), NULL);
    assert(nw > 0);

    /* Acknowledge the one insert: qpe_last_ici becomes 1. */
    s = feed_ici(&enc, 1);
    assert(0 == s);

    /* Now send an ICI close to UINT_MAX.  The 32-bit sum 0xFFFFFFFF + 1 wraps
     * to 0; the pre-fix code accepted this as a (duplicate) no-op.  The fixed
     * code rejects it because the increment exceeds the outstanding inserts.
     */
    s = feed_ici(&enc, 0xFFFFFFFFu);
    assert(0 != s);

    lsqpack_enc_cleanup(&enc);

    printf("enc ici overflow test ok\n");
    return 0;
}
