/*
 * lsqpack.h - QPACK library
 */

/*
MIT License

Copyright (c) 2018 LiteSpeed Technologies Inc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef LSQPACK_H
#define LSQPACK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Strings up to 65535 characters in length are supported.
 */
typedef uint16_t lsqpack_strlen_t;

/** Let's start with four billion for now */
typedef unsigned lsqpack_abs_id_t;

#define LSQPACK_MAX_ABS_ID (~((lsqpack_abs_id_t) 0))

/** Maximum length is defined for convenience */
#define LSQPACK_MAX_STRLEN UINT16_MAX

#define LSQPACK_DEF_DYN_TABLE_SIZE  4096
#define LSQPACK_DEF_MAX_RISKED_STREAMS 100

#define LSQPACK_MAX_DYN_TABLE_SIZE ((1 << 30) - 1)
#define LSQPACK_MAX_MAX_RISKED_STREAMS ((1 << 16) - 1)

struct lsqpack_enc;

int
lsqpack_enc_init (struct lsqpack_enc *, unsigned dyn_table_size,
                    unsigned max_risked_streams);

/** Start a new header block */
int
lsqpack_enc_start_header (struct lsqpack_enc *, uint64_t stream_id,
                            unsigned seqno);

enum lsqpack_enc_status
{
    LQES_OK,
    LQES_NOBUF_ENC,
    LQES_NOBUF_HEAD,
};

enum lsqpack_enc_flags
{
    LQEF_NO_INDEX   = 1 << 0,
};

/**
 * Encode name/value pair in current header block.
 *
 * enc_sz and header_sz parameters are used for both input and output.  If
 * the return value is LQES_OK, they contain number of bytes written to
 * enc_buf and header_buf, respectively.
 */
enum lsqpack_enc_status
lsqpack_enc_encode (struct lsqpack_enc *,
    unsigned char *enc_buf, size_t *enc_sz,
    unsigned char *header_buf, size_t *header_sz,
    const char *name, lsqpack_strlen_t name_sz,
    const char *value, lsqpack_strlen_t value_sz,
    enum lsqpack_enc_flags);

/**
 * End current header block.  The Header Data Prefix is written to `buf'.
 * Returns number of bytes written or 0 on error.
 */
size_t
lsqpack_enc_end_header (struct lsqpack_enc *, unsigned char *buf, size_t);

void
lsqpack_enc_cleanup (struct lsqpack_enc *);

/*
 * Internals follow
 */

#include <sys/queue.h>

#define LSQPACK_BITMASK_BITS 2
#define LSQPACK_BITMASK_NELEMS (1 << LSQPACK_BITMASK_BITS)

typedef uint64_t lsqpack_bitmask_t[LSQPACK_BITMASK_NELEMS];

#define LSQPACK_MAX_HEADERS (sizeof(lsqpack_bitmask_t) * 8)

struct lsqpack_enc_table_entry;

STAILQ_HEAD(lsqpack_enc_head, lsqpack_enc_table_entry);
struct lsqpack_double_enc_head;

struct lsqpack_header_info
{
    uint64_t    qhi_stream_id;
    unsigned    qhi_seqno;
    signed char qhi_valid;
    signed char qhi_at_risk;
};

struct lsqpack_enc
{
    /* The number of all the entries in the dynamic table that have been
     * created so far.  This is used to calculate the Absolute Index.
     */
    lsqpack_abs_id_t            qpe_ins_count;

    unsigned                    qpe_cur_capacity;
    unsigned                    qpe_max_capacity;

    /* The maximum risked streams is the SETTINGS_QPACK_BLOCKED_STREAMS
     * setting.  Note that streams must be differentiated from headers.
     */
    unsigned                    qpe_max_risked_streams;
    unsigned                    qpe_cur_streams_at_risk;

    /* Max hinfos cannot be larger than LSQPACK_MAX_HEADERS */
    unsigned                    qpe_max_headers;

    /* Number of used entries in qpe_hinfos[].  Cannot exceed
     * qpe_max_headers.
     */
    unsigned                    qpe_n_hinfos;

    /* Dynamic table entries (struct enc_table_entry) live in two hash
     * tables: name/value hash table and name hash table.  These tables
     * are the same size.
     */
    unsigned                    qpe_nelem;
    unsigned                    qpe_nbits;
    struct lsqpack_enc_head     qpe_all_entries;
    struct lsqpack_double_enc_head
                               *qpe_buckets;

    /* This array is a mapping from the full header identification which
     * is the (stream ID, sequence number) tuple to a small integer value
     * that can be used in a bitmask.
     *
     * qpe_max_headers + 1 elements are allocated in order to have room
     * for the sentinel value.
     */
    struct lsqpack_header_info *qpe_hinfos;

    /* Current header state */
    struct {
        /* Negative value means the index is not set. */
        int                 hinfo_idx;
        /* Number of at-risk references in this header block */
        unsigned            n_risked;
        /* True if there are other header blocks with the same stream ID
         * that are at risk.  (This means we can risk this header block
         * as well.)
         */
        int                 others_at_risk;
        /* Maximum absolute dynamic table index used by the current header. */
        lsqpack_abs_id_t    max_ref;
        /* Base index */
        lsqpack_abs_id_t    base_idx;
        /* Search cutoff -- to index, entries at this ID and below will be
         * evicted and thus cannot be found during search.
         */
        lsqpack_abs_id_t    search_cutoff;
    }                           qpe_cur_header;
};

struct lsqpack_arr
{
    unsigned        nalloc,
                    nelem,
                    off;
    uintptr_t      *els;
};

#ifdef __cplusplus
}
#endif

#endif
