/*
 * QPACK Interop -- decode from intermediate format and output QIF
 *
 * https://github.com/quicwg/base-drafts/wiki/QPACK-Offline-Interop
 */

#include <assert.h>

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__)
#include <sys/endian.h>
#define bswap_16 bswap16
#define bswap_32 bswap32
#define bswap_64 bswap64
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define bswap_16 OSSwapInt16
#define bswap_32 OSSwapInt32
#define bswap_64 OSSwapInt64
#elif defined(WIN32)
#define bswap_16 _byteswap_ushort
#define bswap_32 _byteswap_ulong
#define bswap_64 _byteswap_uint64
#else
#include <byteswap.h>
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lsqpack.h"
#include "lsxpack_header.h"
#include "xxhash.h"
#ifndef DEBUG
#include "lsqpack-test.h"
#endif

#ifndef NDEBUG
struct static_table_entry
{
    const char       *name;
    const char       *val;
    unsigned          name_len;
    unsigned          val_len;
};

/* [draft-ietf-quic-qpack-03] Appendix A */
static const struct static_table_entry static_table[] =
{
    {":authority", "", 10, 0,},
    {":path", "/", 5, 1,},
    {"age", "0", 3, 1,},
    {"content-disposition", "", 19, 0,},
    {"content-length", "0", 14, 1,},
    {"cookie", "", 6, 0,},
    {"date", "", 4, 0,},
    {"etag", "", 4, 0,},
    {"if-modified-since", "", 17, 0,},
    {"if-none-match", "", 13, 0,},
    {"last-modified", "", 13, 0,},
    {"link", "", 4, 0,},
    {"location", "", 8, 0,},
    {"referer", "", 7, 0,},
    {"set-cookie", "", 10, 0,},
    {":method", "CONNECT", 7, 7,},
    {":method", "DELETE", 7, 6,},
    {":method", "GET", 7, 3,},
    {":method", "HEAD", 7, 4,},
    {":method", "OPTIONS", 7, 7,},
    {":method", "POST", 7, 4,},
    {":method", "PUT", 7, 3,},
    {":scheme", "http", 7, 4,},
    {":scheme", "https", 7, 5,},
    {":status", "103", 7, 3,},
    {":status", "200", 7, 3,},
    {":status", "304", 7, 3,},
    {":status", "404", 7, 3,},
    {":status", "503", 7, 3,},
    {"accept", "*/*", 6, 3,},
    {"accept", "application/dns-message", 6, 23,},
    {"accept-encoding", "gzip, deflate, br", 15, 17,},
    {"accept-ranges", "bytes", 13, 5,},
    {"access-control-allow-headers", "cache-control", 28, 13,},
    {"access-control-allow-headers", "content-type", 28, 12,},
    {"access-control-allow-origin", "*", 27, 1,},
    {"cache-control", "max-age=0", 13, 9,},
    {"cache-control", "max-age=2592000", 13, 15,},
    {"cache-control", "max-age=604800", 13, 14,},
    {"cache-control", "no-cache", 13, 8,},
    {"cache-control", "no-store", 13, 8,},
    {"cache-control", "public, max-age=31536000", 13, 24,},
    {"content-encoding", "br", 16, 2,},
    {"content-encoding", "gzip", 16, 4,},
    {"content-type", "application/dns-message", 12, 23,},
    {"content-type", "application/javascript", 12, 22,},
    {"content-type", "application/json", 12, 16,},
    {"content-type", "application/x-www-form-urlencoded", 12, 33,},
    {"content-type", "image/gif", 12, 9,},
    {"content-type", "image/jpeg", 12, 10,},
    {"content-type", "image/png", 12, 9,},
    {"content-type", "text/css", 12, 8,},
    {"content-type", "text/html; charset=utf-8", 12, 24,},
    {"content-type", "text/plain", 12, 10,},
    {"content-type", "text/plain;charset=utf-8", 12, 24,},
    {"range", "bytes=0-", 5, 8,},
    {"strict-transport-security", "max-age=31536000", 25, 16,},
    {"strict-transport-security", "max-age=31536000; includesubdomains",
                                                                25, 35,},
    {"strict-transport-security",
                "max-age=31536000; includesubdomains; preload", 25, 44,},
    {"vary", "accept-encoding", 4, 15,},
    {"vary", "origin", 4, 6,},
    {"x-content-type-options", "nosniff", 22, 7,},
    {"x-xss-protection", "1; mode=block", 16, 13,},
    {":status", "100", 7, 3,},
    {":status", "204", 7, 3,},
    {":status", "206", 7, 3,},
    {":status", "302", 7, 3,},
    {":status", "400", 7, 3,},
    {":status", "403", 7, 3,},
    {":status", "421", 7, 3,},
    {":status", "425", 7, 3,},
    {":status", "500", 7, 3,},
    {"accept-language", "", 15, 0,},
    {"access-control-allow-credentials", "FALSE", 32, 5,},
    {"access-control-allow-credentials", "TRUE", 32, 4,},
    {"access-control-allow-headers", "*", 28, 1,},
    {"access-control-allow-methods", "get", 28, 3,},
    {"access-control-allow-methods", "get, post, options", 28, 18,},
    {"access-control-allow-methods", "options", 28, 7,},
    {"access-control-expose-headers", "content-length", 29, 14,},
    {"access-control-request-headers", "content-type", 30, 12,},
    {"access-control-request-method", "get", 29, 3,},
    {"access-control-request-method", "post", 29, 4,},
    {"alt-svc", "clear", 7, 5,},
    {"authorization", "", 13, 0,},
    {"content-security-policy",
            "script-src 'none'; object-src 'none'; base-uri 'none'", 23, 53,},
    {"early-data", "1", 10, 1,},
    {"expect-ct", "", 9, 0,},
    {"forwarded", "", 9, 0,},
    {"if-range", "", 8, 0,},
    {"origin", "", 6, 0,},
    {"purpose", "prefetch", 7, 8,},
    {"server", "", 6, 0,},
    {"timing-allow-origin", "*", 19, 1,},
    {"upgrade-insecure-requests", "1", 25, 1,},
    {"user-agent", "", 10, 0,},
    {"x-forwarded-for", "", 15, 0,},
    {"x-frame-options", "deny", 15, 4,},
    {"x-frame-options", "sameorigin", 15, 10,},
};
#endif

static size_t s_max_read_size = SIZE_MAX;

static int s_verbose;
static enum lsqpack_dec_opts s_dec_opts = LSQPACK_DEC_OPT_HASH_NAME
                                        | LSQPACK_DEC_OPT_HASH_NAMEVAL;

static int s_check_unset_qpack_idx = 1;

static FILE *s_out;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void
usage (const char *name)
{
    fprintf(stderr,
"Usage: %s [options] [-i input] [-o output]\n"
"\n"
"Options:\n"
"   -i FILE     Input file.  If not specified or set to `-', the input is\n"
"                 read from stdin.\n"
"   -o FILE     Output file.  If not spepcified or set to `-', the output\n"
"                 is written to stdout.\n"
"   -r FILE     Recipe file.  Without a recipe, buffers are processed in\n"
"                 order.\n"
"   -s NUMBER   Maximum number of risked streams.  Defaults to %u.\n"
"   -t NUMBER   Dynamic table size.  Defaults to %u.\n"
"   -m NUMBER   Maximum read size.  Defaults to %zu.\n"
"   -H [0|1]    Use HTTP/1.x mode and test each header (defaults to `off').\n"
"   -v          Verbose: print headers and table state to stderr.\n"
"   -S          Don't swap encoder stream and header blocks.\n"
"   -Q          Don't check static table when LSXPACK_QPACK_IDX is not set.\n"
"\n"
"   -h          Print this help screen and exit\n"
    , name, LSQPACK_DEF_MAX_RISKED_STREAMS, LSQPACK_DEF_DYN_TABLE_SIZE, SIZE_MAX);
}


struct buf
{
    TAILQ_ENTRY(buf)        next_buf;
    struct lsqpack_dec     *dec;
    uint64_t                stream_id;     /* Zero means encoder stream */
    size_t                  size;
    size_t                  off;
    size_t                  file_off;

    /* A single header name/value pair is stored in xhdr_buf.  When the
     * header is done, the whole buffer can be used again for the next
     * header.
     */
    struct lsxpack_header   xhdr;
    unsigned                xhdr_off;       /* Used in xhdr_buf */
    unsigned                xhdr_nalloc;    /* Number of bytes allocated */ /* TODO: use it */

    /* Output is written to out_buf out header at a time and the printed all
     * at once.  This logic is not very important and we use a reasonably
     * large fixed-size buffer.
     */
    unsigned                out_off;
    char                    out_buf[0x1000];

    unsigned char           buf[0];
};


TAILQ_HEAD(, buf) bufs = TAILQ_HEAD_INITIALIZER(bufs);

static void
hblock_unblocked (void *buf_p)
{
    struct buf *buf = buf_p;
    TAILQ_INSERT_HEAD(&bufs, buf, next_buf);
}


static struct lsxpack_header *
prepare_decode (void *hblock_ctx, struct lsxpack_header *xhdr, size_t space)
{
    struct buf *const buf = hblock_ctx;
    char *new;

    if (space > LSXPACK_MAX_STRLEN)
        return NULL;

    if (xhdr)
    {
        assert(xhdr == &buf->xhdr);
        assert(space > xhdr->val_len);
        new = realloc(xhdr->buf, space);
        if (!new)
            return NULL;
        xhdr->buf = new;
        xhdr->val_len = space;
        return xhdr;
    }
    else
    {
        xhdr = &buf->xhdr;
        new = malloc(space);
        if (!new)
            return NULL;
        lsxpack_header_prepare_decode(xhdr, new, 0, space);
        return xhdr;
    }
}


static int
process_header (void *hblock_ctx, struct lsxpack_header *xhdr)
{
    struct buf *const buf = hblock_ctx;
    const char *p;
    const uint32_t seed = 39378473;
    uint32_t hash, name_hash;
    int nw;

    if (s_dec_opts & LSQPACK_DEC_OPT_HTTP1X)
    {
        p = lsxpack_header_get_name(xhdr) + xhdr->name_len;
        assert(0 == memcmp(p, ": ", 2));
        p += 2 + xhdr->val_len;
        assert(0 == memcmp(p, "\r\n", 2));
        assert(xhdr->dec_overhead == 4);
    }
    else
        assert(xhdr->dec_overhead == 0);

    if (s_dec_opts & LSQPACK_DEC_OPT_HASH_NAME)
    {
        assert(xhdr->flags & LSXPACK_NAME_HASH);
        hash = XXH32(lsxpack_header_get_name(xhdr), xhdr->name_len, seed);
        assert(hash == xhdr->name_hash);
    }

    if (s_dec_opts & LSQPACK_DEC_OPT_HASH_NAME)
        assert(xhdr->flags & LSXPACK_NAME_HASH);

    if (xhdr->flags & LSXPACK_NAME_HASH)
    {
        name_hash = XXH32(lsxpack_header_get_name(xhdr), xhdr->name_len, seed);
        assert(hash == xhdr->name_hash);
    }
#ifndef NDEBUG
    else if (!(xhdr->flags & LSXPACK_QPACK_IDX))
        /* Calculate for upcoming "not in static table check" */
        name_hash = XXH32(lsxpack_header_get_name(xhdr), xhdr->name_len, seed);
#endif

    if (s_dec_opts & LSQPACK_DEC_OPT_HASH_NAMEVAL)
    {
        /* This is not required by the API, but internally, if the library
         * calculates nameval hash, it should also set the name hash.
         */
        assert(xhdr->flags & LSXPACK_NAME_HASH);
        assert(xhdr->flags & LSXPACK_NAMEVAL_HASH);
    }

    if (xhdr->flags & LSXPACK_NAMEVAL_HASH)
    {
        hash = XXH32(lsxpack_header_get_name(xhdr), xhdr->name_len, seed);
        hash = XXH32(lsxpack_header_get_value(xhdr), xhdr->val_len, hash);
        assert(hash == xhdr->nameval_hash);
    }

#ifndef NDEBUG
    if (xhdr->flags & LSXPACK_QPACK_IDX)
    {
        assert(xhdr->qpack_index <
                            sizeof(static_table) / sizeof(static_table[0]));
        assert(static_table[xhdr->qpack_index].name_len == xhdr->name_len);
        assert(0 == memcmp(lsxpack_header_get_name(xhdr),
                        static_table[xhdr->qpack_index].name, xhdr->name_len));
    }
    else if (s_check_unset_qpack_idx)
    {
        /* The decoder does best effort: if the encoder did not use the
         * static table, QPACK index is not set.  However, since we are
         * testing our decoder, we assume that the encoder always uses
         * the static table when it can.
         */
        int idx = lsqpack_find_in_static_headers(name_hash,
                            lsxpack_header_get_name(xhdr), xhdr->name_len);
        assert(idx < 0);
    }
#endif

    nw = snprintf(buf->out_buf + buf->out_off,
            sizeof(buf->out_buf) - buf->out_off,
            "%.*s\t%.*s\n",
            (int) xhdr->name_len, lsxpack_header_get_name(xhdr),
            (int) xhdr->val_len, lsxpack_header_get_value(xhdr));
    free(buf->xhdr.buf);
    memset(&buf->xhdr, 0, sizeof(buf->xhdr));
    if (nw > 0 && (size_t) nw <= sizeof(buf->out_buf) - buf->out_off)
    {
        buf->out_off += (unsigned) nw;
        return 0;
    }
    else
    {
        fprintf(stderr, "header list too long\n");
        return -1;
    }
}


static const struct lsqpack_dec_hset_if hset_if = {
    .dhi_unblocked      = hblock_unblocked,
    .dhi_prepare_decode = prepare_decode,
    .dhi_process_header = process_header,
};


static void
header_block_done (const struct buf *buf)
{
    fprintf(s_out, "# stream %"PRIu64"\n", buf->stream_id);
    fprintf(s_out, "# (stream ID above is used for sorting)\n");
    fprintf(s_out, "%.*s\n", (int) buf->out_off, buf->out_buf);
}


int
main (int argc, char **argv)
{
    FILE *in = stdin;
    FILE *recipe = NULL;
    int opt;
    unsigned dyn_table_size     = LSQPACK_DEF_DYN_TABLE_SIZE,
             max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS;
    struct lsqpack_dec decoder;
    const struct lsqpack_dec_err *err;
    const unsigned char *p;
    ssize_t nr;
    int r;
    uint64_t stream_id;
    uint32_t size;
    size_t off;         /* For debugging */
    size_t file_off;
    struct buf *buf;
    unsigned lineno;
    char *line, *end;
    enum lsqpack_read_header_status rhs;
    struct lsqpack_header_list *hlist;
    int do_swap = 1;
    char command[0x100];
    char line_buf[0x100];

    while (-1 != (opt = getopt(argc, argv, "i:o:r:s:t:m:hvH:SQ")))
    {
        switch (opt)
        {
        case 'i':
            if (0 != strcmp(optarg, "-"))
            {
                in = fopen(optarg, "rb");
                if (!in)
                {
                    fprintf(stderr, "cannot open `%s' for reading: %s\n",
                                                optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            break;
        case 'o':
            if (0 != strcmp(optarg, "-"))
            {
                s_out = fopen(optarg, "w");
                if (!s_out)
                {
                    fprintf(stderr, "cannot open `%s' for writing: %s\n",
                                                optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            break;
        case 'r':
            if (0 == strcmp(optarg, "-"))
                recipe = stdin;
            else
            {
                recipe = fopen(optarg, "r");
                if (!recipe)
                {
                    fprintf(stderr, "cannot open `%s' for reading: %s\n",
                                                optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            break;
        case 's':
            max_risked_streams = atoi(optarg);
            break;
        case 't':
            dyn_table_size = atoi(optarg);
            break;
        case 'm':
            s_max_read_size = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'v':
            ++s_verbose;
            break;
        case 'S':
            do_swap = 0;
            break;
        case 'H':
            if (atoi(optarg))
                s_dec_opts |= LSQPACK_DEC_OPT_HTTP1X;
            else
                s_dec_opts &= ~LSQPACK_DEC_OPT_HTTP1X;
            break;
        case 'Q':
            s_check_unset_qpack_idx = 0;
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }

    if (!s_out)
        s_out = stdout;

    lsqpack_dec_init(&decoder, s_verbose ? stderr : NULL, dyn_table_size,
                        max_risked_streams, &hset_if, s_dec_opts);

    off = 0;
    while (1)
    {
        file_off = off;
        nr = fread(&stream_id, 1, sizeof(stream_id), in);
        if (nr == 0)
            break;
        if (nr != sizeof(stream_id))
        {
            fprintf(stderr, "could not read %zu bytes (stream id) at "
                "offset %zu: %s\n", sizeof(stream_id), off, strerror(errno));
            goto read_err;
        }
        off += nr;
        nr = fread(&size, 1, sizeof(size), in);
        if (nr != sizeof(size))
        {
            fprintf(stderr, "could not read %zu bytes (size) at "
                "offset %zu: %s\n", sizeof(size), off, strerror(errno));
            goto read_err;
        }
        off += nr;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        stream_id = bswap_64(stream_id);
        size = bswap_32(size);
#endif
        if (stream_id == 0 && size == 0)
            continue;
        buf = malloc(sizeof(*buf) + size);
        if (!buf)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memset(buf, 0, sizeof(*buf));
        nr = fread(buf->buf, 1, size, in);
        if (nr != (ssize_t) size)
        {
            fprintf(stderr, "could not read %"PRIu32" bytes (buffer) at "
                "offset %zu: %s\n", size, off, strerror(errno));
            goto read_err;
        }
        off += nr;
        buf->dec = &decoder;
        buf->stream_id = stream_id;
        buf->size = size;
        buf->file_off = file_off;
        if (buf->size == 0)
            exit(EXIT_FAILURE);
        TAILQ_INSERT_TAIL(&bufs, buf, next_buf);
    }
    (void) fclose(in);
    in = NULL;

    if (recipe)
    {
        lineno = 0;
        while (line = fgets(line_buf, sizeof(line_buf), recipe), line != NULL)
        {
            ++lineno;
            end = strchr(line, '\n');
            if (!end)
            {
                fprintf(stderr, "no newline on line %u\n", lineno);
                exit(EXIT_FAILURE);
            }
            *end = '\0';

            if (*line == '#')
                continue;

            if (3 == sscanf(line, " %[s] %"PRIu64" %"PRIu32" ", command, &stream_id, &size))
            {
                TAILQ_FOREACH(buf, &bufs, next_buf)
                    if (stream_id == buf->stream_id)
                        break;
                if (!buf)
                {
                    fprintf(stderr, "stream %"PRIu64" not found (recipe line %u)\n",
                        stream_id, lineno);
                    exit(EXIT_FAILURE);
                }
                p = buf->buf;
                rhs = lsqpack_dec_header_in(&decoder, buf, stream_id,
                            buf->size, &p,
                            buf->size /* FIXME: this should be `size' */,
                            NULL, NULL);
                switch (rhs)
                {
                case LQRHS_DONE:
                    assert(p == buf->buf + buf->size);
                    header_block_done(buf);
                    if (s_verbose)
                        fprintf(stderr, "compression ratio: %.3f\n",
                            lsqpack_dec_ratio(&decoder));
                    TAILQ_REMOVE(&bufs, buf, next_buf);
                    free(buf);
                    break;
                case LQRHS_BLOCKED:
                    buf->off += (unsigned) (p - buf->buf);
                    TAILQ_REMOVE(&bufs, buf, next_buf);
                    break;
                case LQRHS_NEED:
                    buf->off += (unsigned) (p - buf->buf);
                    break;
                default:
                    assert(rhs == LQRHS_ERROR);
                    fprintf(stderr, "recipe line %u: stream %"PRIu64": "
                        "header_in error\n", lineno, stream_id);
                    exit(EXIT_FAILURE);
                }
            }
            else if (2 == sscanf(line, " %[z] %u ", command, &size))
            {
                s_max_read_size = size;
            }
            else
            {
                perror("sscanf");
                exit(EXIT_FAILURE);
            }
        }
        fclose(recipe);
    }
    else if (do_swap && max_risked_streams && dyn_table_size
                    && TAILQ_FIRST(&bufs) && TAILQ_FIRST(&bufs)->stream_id)
    {
        /* Swap header blocks and encoder stream bufs to exercise blocked
         * header blocks logic.
         */
        struct buf *saved_hblock = NULL, *next;
        for (buf = TAILQ_FIRST(&bufs); buf; buf = next)
        {
            next = TAILQ_NEXT(buf, next_buf);
            if (buf->stream_id)
                continue;
            if (saved_hblock)
                TAILQ_INSERT_BEFORE(buf, saved_hblock, next_buf);
            saved_hblock = buf;
            TAILQ_REMOVE(&bufs, buf, next_buf);
        }
        TAILQ_INSERT_TAIL(&bufs, saved_hblock, next_buf);
    }

    while (buf = TAILQ_FIRST(&bufs), buf != NULL)
    {
        TAILQ_REMOVE(&bufs, buf, next_buf);
        if (buf->stream_id == 0)
        {
            r = lsqpack_dec_enc_in(&decoder, buf->buf, buf->size - buf->off);
            if (r != 0)
            {
                err = lsqpack_dec_get_err_info(buf->dec);
                fprintf(stderr, "encoder_in error; off %"PRIu64", line %d\n",
                                                            err->off, err->line);
                exit(EXIT_FAILURE);
            }
            if (s_verbose)
                lsqpack_dec_print_table(&decoder, stderr);
            free(buf);
        }
        else
        {
        dec_header:
            p = buf->buf + buf->off;
            if (buf->off == 0)
                rhs = lsqpack_dec_header_in(&decoder, buf, buf->stream_id,
                                buf->size, &p, MIN(s_max_read_size, buf->size),
                                NULL, NULL);
            else
                rhs = lsqpack_dec_header_read(buf->dec, buf, &p,
                                MIN(s_max_read_size, (buf->size - buf->off)),
                                NULL, NULL);
            switch (rhs)
            {
            case LQRHS_DONE:
                assert(p == buf->buf + buf->size);
                header_block_done(buf);
                if (s_verbose)
                    fprintf(stderr, "compression ratio: %.3f\n",
                        lsqpack_dec_ratio(&decoder));
                free(buf);
                break;
            case LQRHS_BLOCKED:
                buf->off = (unsigned) (p - buf->buf);
                break;
            case LQRHS_NEED:
                buf->off = (unsigned) (p - buf->buf);
                goto dec_header;
            default:
                assert(rhs == LQRHS_ERROR);
                fprintf(stderr, "stream %"PRIu64": header block error "
                    "starting at off %zu\n", buf->stream_id, buf->off);
                err = lsqpack_dec_get_err_info(&decoder);
                fprintf(stderr, "encoder_in error; off %"PRIu64", line %d\n",
                                                            err->off, err->line);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (!TAILQ_EMPTY(&bufs))
    {
        fprintf(stderr, "some streams reamain\n");
        exit(EXIT_FAILURE);
    }
    /* TODO: check if decoder has any stream references.  That would be
     * an error.
     */

    if (s_verbose)
        lsqpack_dec_print_table(&decoder, stderr);

    lsqpack_dec_cleanup(&decoder);

    assert(TAILQ_EMPTY(&bufs));

    if (s_out)
        (void) fclose(s_out);

    exit(EXIT_SUCCESS);

  read_err:
    if (nr < 0)
        perror("read");
    else if (nr == 0)
        fprintf(stderr, "unexpected EOF\n");
    else
        fprintf(stderr, "not enough bytes read (%zu)\n", (size_t) nr);
    exit(EXIT_FAILURE);
}
