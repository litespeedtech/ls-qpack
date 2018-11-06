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
#include <stdio.h>

/** Let's start with four billion for now */
typedef unsigned lsqpack_abs_id_t;

#define LSQPACK_MAX_ABS_ID (~((lsqpack_abs_id_t) 0))

#define LSQPACK_DEF_DYN_TABLE_SIZE  4096
#define LSQPACK_DEF_MAX_RISKED_STREAMS 100

#define LSQPACK_MAX_DYN_TABLE_SIZE ((1 << 30) - 1)
#define LSQPACK_MAX_MAX_RISKED_STREAMS ((1 << 16) - 1)

struct lsqpack_enc;
struct lsqpack_dec;

typedef ssize_t (*lsqpack_stream_read_f)(void *stream, const unsigned char **buf, size_t sz);
typedef void (*lsqpack_stream_write_f)(void *stream, void *buf, size_t sz);
typedef void (*lsqpack_stream_wantread_f)(void *stream, int wantread);
typedef void (*lsqpack_stream_wantwrite_f)(void *stream, int wantwrite);

enum lsqpack_enc_opts
{
    /**
     * Client and server follow different heuristics.  The encoder is either
     * in one or the other mode.
     */
    LSQPACK_ENC_OPT_SERVER  = 1 << 0,

    /**
     * Enable emitting dup instructions.
     */
    LSQPACK_ENC_OPT_DUP     = 1 << 1,

    /**
     * Index aggressively: ignore history
     */
    LSQPACK_ENC_OPT_IX_AGGR = 1 << 2,

    /**
     * The encoder was pre-initialized.
     */
    LSQPACK_ENC_OPT_STAGE_2 = 1 << 3,
};


/**
 * Initialize the encoder so that it can be used without using the
 * dynamic table.  Once peer's settings are known, call
 * @ref lsqpack_enc_init().
 */
void
lsqpack_enc_preinit (struct lsqpack_enc *);

int
lsqpack_enc_init (struct lsqpack_enc *,
    /**
     * As specified by the decoder.  This value is used to calculate
     * MaxEntries.
     */
    unsigned max_table_size,
    /**
     * Actual dynamic table size to use.
     */
    unsigned dyn_table_size,
    unsigned max_risked_streams, enum lsqpack_enc_opts);

#if LSQPACK_DEVEL_MODE
void
lsqpack_enc_log (struct lsqpack_enc *, FILE *);
#endif

/** Start a new header block.  Return 0 on success or -1 on error. */
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
    /**
     * Do not index this header field.  No output to the encoder stream
     * will be produced.
     */
    LQEF_NO_INDEX    = 1 << 0,
    /**
     * Never index this field.  This will set the 'N' bit on Literal Header
     * Field With Name Reference, Literal Header Field With Post-Base Name
     * Reference, and Literal Header Field Without Name Reference instructions
     * in the header block.  Implies LQEF_DONT_INDEX.
     */
    LQEF_NEVER_INDEX = 1 << 1,
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
    const char *name, unsigned name_sz,
    const char *value, unsigned value_sz,
    enum lsqpack_enc_flags);

/**
 * End current header block.  The Header Data Prefix is written to `buf'.
 *
 * Returns:
 *  -   A non-negative values indicates success and is the number of bytes
 *      written to `buf'.
 *  -   Zero means that there is not enough room in `buf' to write out the
 *      full prefix.
 *  -   A negative value means an error.  This is returned if there is no
 *      started header to end.
 */
ssize_t
lsqpack_enc_end_header (struct lsqpack_enc *, unsigned char *buf, size_t);

/**
 * Process next chunk of bytes from the decoder stream.  Returns 0 on success,
 * -1 on failure.  The failure should be treated as fatal.
 */
int
lsqpack_enc_decoder_in (struct lsqpack_enc *, const unsigned char *, size_t);

/**
 * Return estimated compression ratio until this point.  Compression ratio
 * is defined as size of the output stream divided by the size of the
 * input stream.
 */
float
lsqpack_enc_ratio (const struct lsqpack_enc *);

/**
 * Return maximum size needed to encode Header Data Prefix
 */
size_t
lsqpack_enc_header_data_prefix_size (const struct lsqpack_enc *);

void
lsqpack_enc_cleanup (struct lsqpack_enc *);

/**
 * The header is a single name/value pair.  The strings are not NUL-terminated.
 */
struct lsqpack_header
{
    const char         *qh_name;
    const char         *qh_value;
    unsigned            qh_name_len;
    unsigned            qh_value_len;
    unsigned            qh_static_id;
    enum {
        /** Must be encoded with a literal representation */
        QH_NEVER    = 1 << 0,
        /** qh_static_id is set */
        QH_ID_SET   = 1 << 1,
    }                   qh_flags;
};

/**
 * The header set represents the decoded header block.
 */
struct lsqpack_header_set
{
    struct lsqpack_header  **qhs_headers;
    unsigned                 qhs_count;
};

/* The callback `header_block_done' is called when the decoder is done
 * reading from the header block.  At this point, the decoder is no
 * longer using the stream reference.  If header block decoding was
 * successful, the header set is not NULL.  This structure should be
 * destroyed when the user is done with it using
 * lsqpack_dec_destroy_header_set().
 */
void
lsqpack_dec_init (struct lsqpack_dec *, unsigned dyn_table_size,
    unsigned max_risked_streams,
    lsqpack_stream_write_f write_decoder, void *decoder_stream,
    lsqpack_stream_read_f read_header_block,
    lsqpack_stream_wantread_f wantread_header_block,
    void (*header_block_done)(void *header_block_stream,
                                        struct lsqpack_header_set *));

/* The decoder will read exactly `header_block_size' bytes from the stream
 * using the read_header_block() function pointer specified during
 * initialization.  If the header block cannot be processed due to blocked
 * references, the decoder keeps the stream until the references become
 * unblocked.  wantread_header_block() is called to indicate whether to
 * the decoder want more bytes.  If yes, lsqpack_dec_header_read() should
 * be called when more data becomes available.
 *
 * Until the `header_block_done()' callback is called, the decoder keeps
 * a reference to the stream and may call associated callbacks.
 */
int
lsqpack_dec_header_in (struct lsqpack_dec *, void *header_block_stream,
                        uint64_t stream_iid, size_t header_block_size);

/**
 * More stream data is available -- have the decoder read it.
 */
int
lsqpack_dec_header_read (struct lsqpack_dec *dec, void *stream);

/**
 * Feed encoder stream data to the decoder.  Zero is returned on success,
 * negative value on error.
 */
int
lsqpack_dec_enc_in (struct lsqpack_dec *, const unsigned char *, size_t);

/**
 * Destroy the header set returned by the header_block_done() callback.
 */
void
lsqpack_dec_destroy_header_set (struct lsqpack_header_set *);

/* Clean up the decoder.  If any there are any blocked header blocks,
 * `header_block_done' will be called for each of them with the second
 * argument set to NULL.
 */
void
lsqpack_dec_cleanup (struct lsqpack_dec *);

/**
 * Print human-readable decoder table.
 */
void
lsqpack_dec_print_table (const struct lsqpack_dec *, FILE *out);


struct lsqpack_dec_err
{
    enum {
        LSQPACK_DEC_ERR_LOC_HEADER_BLOCK,
        LSQPACK_DEC_ERR_LOC_ENC_STREAM,
    }           type;
    int         line;       /* In the source file */
    uint64_t    off;        /* Offset in header block or on encoder stream */
    uint64_t    stream_id;  /* Only applicable to header block */
};


const struct lsqpack_dec_err *
lsqpack_dec_get_err_info (const struct lsqpack_dec *);

/*
 * Internals follow.  The internals are subject to change without notice.
 */

#include <sys/queue.h>

/* It takes 11 bytes to encode UINT64_MAX as HPACK integer */
#define LSQPACK_UINT64_ENC_SZ 11u

struct lsqpack_enc_table_entry;

STAILQ_HEAD(lsqpack_enc_head, lsqpack_enc_table_entry);
struct lsqpack_double_enc_head;

struct lsqpack_header_info_arr;

struct lsqpack_dec_int_state
{
    int         resume;
    unsigned    M, nread;
    uint64_t    val;
};

struct lsqpack_enc
{
    /* The number of all the entries in the dynamic table that have been
     * created so far.  This is used to calculate the Absolute Index.
     */
    lsqpack_abs_id_t            qpe_ins_count;
    lsqpack_abs_id_t            qpe_max_acked_id;
    lsqpack_abs_id_t            qpe_last_tss;
    lsqpack_abs_id_t            qpe_drain_idx;

    enum {
        LSQPACK_ENC_HEADER  = 1 << 0,
        LSQPACK_ENC_USE_DUP = 1 << 1,
    }                           qpe_flags;

    unsigned                    qpe_cur_capacity;
    unsigned                    qpe_max_capacity;
    unsigned                    qpe_max_entries;

    /* The maximum risked streams is the SETTINGS_QPACK_BLOCKED_STREAMS
     * setting.  Note that streams must be differentiated from headers.
     */
    unsigned                    qpe_max_risked_streams;
    unsigned                    qpe_cur_streams_at_risk;

    /* Number of used entries in qpe_hinfos[]. */
    unsigned                    qpe_hinfos_count;
    /* Number of entries allocated in qpe_hinfos_arr[].  There is always
     * an extra element at the end for use as sentinel.
     */
    unsigned                    qpe_hinfos_nalloc;

    /* Dynamic table entries (struct enc_table_entry) live in two hash
     * tables: name/value hash table and name hash table.  These tables
     * are the same size.
     */
    unsigned                    qpe_nelem;
    unsigned                    qpe_nbits;
    struct lsqpack_enc_head     qpe_all_entries;
    struct lsqpack_double_enc_head
                               *qpe_buckets;

    STAILQ_HEAD(, lsqpack_header_info_arr)
                                qpe_hinfo_arrs;
    TAILQ_HEAD(, lsqpack_header_info)
                                qpe_hinfos;

    /* Current header state */
    struct {
        struct lsqpack_header_info  *hinfo;

        /* Number of at-risk references in this header block */
        unsigned            n_risked;
        /* Number of headers in this header list */
        unsigned            n_headers;
        /* True if there are other header blocks with the same stream ID
         * that are at risk.  (This means we can risk this header block
         * as well.)
         */
        int                 others_at_risk;
        /* Base index */
        lsqpack_abs_id_t    base_idx;
    }                           qpe_cur_header;

    struct {
        struct lsqpack_dec_int_state dec_int_state;
        int   (*handler)(struct lsqpack_enc *, uint64_t);
    }                           qpe_dec_stream_state;

    /* Used to calculate estimated compression ratio.  Note that the `out'
     * part contains bytes sent on the decoder stream, as it also counts
     * toward the overhead.
     */
    unsigned long               qpe_bytes_in;
    unsigned long               qpe_bytes_out;
#if LSQPACK_DEVEL_MODE
    FILE                       *qpe_log;
#endif
    struct lsqpack_enc_hist    *qpe_hist;
    void                      (*qpe_hist_add)(struct lsqpack_enc_hist *,
                                                        unsigned, unsigned);
    int                       (*qpe_hist_seen_nameval)(
                                    struct lsqpack_enc_hist *,
                                                        unsigned);
    int                       (*qpe_hist_seen_name)(struct lsqpack_enc_hist *,
                                                        unsigned);
};

struct lsqpack_ringbuf
{
    unsigned        rb_nalloc, rb_head, rb_tail;
    void          **rb_els;
};

TAILQ_HEAD(lsqpack_header_sets, lsqpack_header_set_elem);

struct lsqpack_header_block;

struct lsqpack_decode_status
{
    uint8_t state;
    uint8_t eos;
};

struct lsqpack_huff_decode_state
{
    int                             resume;
    struct lsqpack_decode_status    status;
};

struct lsqpack_enc_int_state
{
    int         resume;
    uint64_t    value;
};

struct lsqpack_dec_inst;

struct lsqpack_dec
{
    /** This is the hard limit set at initialization */
    unsigned                qpd_max_capacity;
    /** The current maximum capacity can be adjusted at run-time */
    unsigned                qpd_cur_max_capacity;
    unsigned                qpd_cur_capacity;
    unsigned                qpd_max_risked_streams;
    unsigned                qpd_max_entries;
    /** ID of the last dynamic table entry.  Has the range
     * [0, qpd_max_entries * 2 - 1 ]
     */
    lsqpack_abs_id_t        qpd_last_id;
    enum {
        LSQPACK_DEC_WANT_WRITE_DECODER  = 1 << 0,
    }                       qpd_flags;
    void                   *qpd_dec_stream;
    lsqpack_stream_wantread_f
                            qpd_wantwrite_decoder;
    lsqpack_stream_write_f  qpd_write_dec;
    lsqpack_stream_wantread_f
                            qpd_wantread_header_block;
    lsqpack_stream_read_f   qpd_read_header_block;
    void                  (*qpd_header_block_done)(void *header_block_stream,
                                        struct lsqpack_header_set *);

    /** Outstanding header sets */
    struct lsqpack_header_sets
                            qpd_header_sets;

    /** This is the dynamic table */
    struct lsqpack_ringbuf  qpd_dyn_table;

    TAILQ_HEAD(, header_block_read_ctx)
                            qpd_hbrcs;

    /** Blocked headers are kept in a small hash */
#define LSQPACK_DEC_BLOCKED_BITS 3
    TAILQ_HEAD(, header_block_read_ctx)
                            qpd_blocked_headers[1 << LSQPACK_DEC_BLOCKED_BITS];
    /** Number of blocked streams (in qpd_blocked_headers) */
    unsigned                qpd_n_blocked;

    /** Decoder instructions to be sent out */
    STAILQ_HEAD(, lsqpack_dec_inst)
                            qpd_dinsts;
    /** Intra-instruction state for the decoder stream */
    struct lsqpack_enc_int_state
                            qpd_dinst_state;

    /** Reading the encoder stream */
    struct {
        int                                                 resume;
        union {
            /* State for reading in the Insert With Named Reference
             * instruction.
             */
            struct {
                struct lsqpack_dec_int_state        dec_int_state;
                struct lsqpack_huff_decode_state    dec_huff_state;
                uint64_t                            name_idx;
                uint64_t                            val_len;
                struct lsqpack_dec_table_entry     *reffed_entry;
                struct lsqpack_dec_table_entry     *entry;
                const char                         *name;
                unsigned                            alloced_val_len;
                unsigned                            val_off;
                unsigned                            nread;
                unsigned                            name_len;
                signed char                         is_huffman;
                signed char                         is_static;
            }                                               with_namref;

            /* State for reading in the Insert Without Named Reference
             * instruction.
             */
            struct {
                struct lsqpack_dec_int_state        dec_int_state;
                struct lsqpack_huff_decode_state    dec_huff_state;
                uint64_t                            str_len;
                struct lsqpack_dec_table_entry     *entry;
                unsigned                            alloced_len;
                unsigned                            str_off;
                unsigned                            nread;
                signed char                         is_huffman;
            }                                               wo_namref;

            struct {
                struct lsqpack_dec_int_state        dec_int_state;
                uint64_t                            index;
            }                                               duplicate;

            struct {
                struct lsqpack_dec_int_state        dec_int_state;
                uint64_t                            new_size;
            }                                               size_update;
        }               ctx_u;
    }                       qpd_enc_state;
    struct lsqpack_dec_err  qpd_err;
};

#ifdef __cplusplus
}
#endif

#endif
