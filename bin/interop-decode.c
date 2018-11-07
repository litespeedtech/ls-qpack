/*
 * QPACK Interop -- decode from intermediate format and output QIF
 *
 * https://github.com/quicwg/base-drafts/wiki/QPACK-Offline-Interop
 */

#include <assert.h>
#include <byteswap.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lsqpack.h"

static size_t s_max_read_size = SIZE_MAX;

static int s_verbose;

static FILE *s_out;

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
"   -v          Verbose: print headers and table state to stderr.\n"
"\n"
"   -h          Print this help screen and exit\n"
    , name, LSQPACK_DEF_MAX_RISKED_STREAMS, LSQPACK_DEF_DYN_TABLE_SIZE);
}


struct buf
{
    TAILQ_ENTRY(buf)        next_buf;
    struct lsqpack_dec     *dec;
    uint64_t                stream_id;     /* Zero means encoder stream */
    size_t                  size;
    size_t                  off;
    int                     wantread;
    unsigned char           buf[0];
};


TAILQ_HEAD(, buf) bufs = TAILQ_HEAD_INITIALIZER(bufs);


static void
hblock_unblocked (void *buf_p)
{
    struct buf *buf = buf_p;
    TAILQ_INSERT_HEAD(&bufs, buf, next_buf);
}


static void
header_block_done (const struct buf *buf, struct lsqpack_header_set *set)
{
    unsigned n;

    if (s_verbose)
    {
        fprintf(stderr, "Have headers for stream %"PRIu64":\n", buf->stream_id);
        for (n = 0; n < set->qhs_count; ++n)
            fprintf(stderr, "  %.*s: %.*s\n",
                set->qhs_headers[n]->qh_name_len, set->qhs_headers[n]->qh_name,
                set->qhs_headers[n]->qh_value_len, set->qhs_headers[n]->qh_value);
        fprintf(stderr, "\n");
    }

    fprintf(s_out, "# stream %"PRIu64"\n", buf->stream_id);
    fprintf(s_out, "# (stream ID above is used for sorting)\n");
    for (n = 0; n < set->qhs_count; ++n)
        fprintf(s_out, "%.*s\t%.*s\n",
            set->qhs_headers[n]->qh_name_len, set->qhs_headers[n]->qh_name,
            set->qhs_headers[n]->qh_value_len, set->qhs_headers[n]->qh_value);
    fprintf(s_out, "\n");

    lsqpack_dec_destroy_header_set(set);
}


int
main (int argc, char **argv)
{
    int in = STDIN_FILENO;
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
    struct buf *buf;
    unsigned lineno;
    char *line, *end;
    enum lsqpack_read_header_status rhs;
    struct lsqpack_header_set *hset;
    char command[0x100];
    char line_buf[0x100];

    while (-1 != (opt = getopt(argc, argv, "i:o:r:s:t:hv")))
    {
        switch (opt)
        {
        case 'i':
            if (0 != strcmp(optarg, "-"))
            {
                in = open(optarg, O_RDONLY);
                if (in < 0)
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
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'v':
            ++s_verbose;
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }

    if (!s_out)
        s_out = stdout;

    lsqpack_dec_init(&decoder, dyn_table_size, max_risked_streams,
                 NULL, NULL, hblock_unblocked);

    off = 0;
    while (1)
    {
        nr = read(in, &stream_id, sizeof(stream_id));
        if (nr == 0)
            break;
        if (nr != sizeof(stream_id))
            goto read_err;
        off += nr;
        nr = read(in, &size, sizeof(size));
        if (nr != sizeof(size))
            goto read_err;
        off += nr;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        stream_id = bswap_64(stream_id);
        size = bswap_32(size);
#endif
        buf = malloc(sizeof(*buf) + size);
        if (!buf)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memset(buf, 0, sizeof(*buf));
        nr = read(in, buf->buf, size);
        if (nr != size)
            goto read_err;
        off += nr;
        buf->dec = &decoder;
        buf->stream_id = stream_id;
        buf->size = size;
        TAILQ_INSERT_TAIL(&bufs, buf, next_buf);
    }

    if (recipe)
    {
        lineno = 0;
        while ((line = fgets(line_buf, sizeof(line_buf), recipe)))
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

            if (3 == sscanf(line, " %[s] %lu %u ", command, &stream_id, &size))
            {
                TAILQ_FOREACH(buf, &bufs, next_buf)
                    if (stream_id == buf->stream_id)
                        break;
                if (!buf)
                {
                    fprintf(stderr, "stream %lu not found (recipe line %u)\n",
                        stream_id, lineno);
                    exit(EXIT_FAILURE);
                }
                p = buf->buf;
                rhs = lsqpack_dec_header_in(&decoder, buf, stream_id,
                            buf->size, &p,
                            buf->size /* FIXME: this should be `size' */,
                            &hset);
                switch (rhs)
                {
                case LQRHS_DONE:
                    assert(p == buf->buf + buf->size);
                    header_block_done(buf, hset);
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
                    fprintf(stderr, "recipe line %u: stream %lu: header_in error\n",
                        lineno, stream_id);
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

    while ((buf = TAILQ_FIRST(&bufs)))
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
            p = buf->buf + buf->off;
            if (buf->off == 0)
                rhs = lsqpack_dec_header_in(&decoder, buf, buf->stream_id,
                                            buf->size, &p, buf->size, &hset);
            else
                rhs = lsqpack_dec_header_read(buf->dec, buf, &p,
                                                buf->size - buf->off, &hset);
            switch (rhs)
            {
            case LQRHS_DONE:
                assert(p == buf->buf + buf->size);
                header_block_done(buf, hset);
                free(buf);
                break;
            case LQRHS_BLOCKED:
                buf->off += (unsigned) (p - buf->buf);
                break;
            case LQRHS_NEED:
                fprintf(stderr, "This can't be right: all bytes were given.  "
                    "stream %lu\n", buf->stream_id);
                exit(EXIT_FAILURE);
            default:
                assert(rhs == LQRHS_ERROR);
                fprintf(stderr, "stream %lu: header block error starting at "
                    "off %zu\n", stream_id, buf->off);
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

    lsqpack_dec_cleanup(&decoder);

    assert(TAILQ_EMPTY(&bufs));

    exit(EXIT_SUCCESS);

  read_err:
    if (nr < 0)
        perror("read");
    else if (nr == 0)
        fprintf(stderr, "unexpected EOF\n");
    else
        fprintf(stderr, "not enough bytes read\n");
    exit(EXIT_FAILURE);
}
