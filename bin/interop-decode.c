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


static ssize_t
read_header_block (void *buf_p, const unsigned char **dst, size_t size)
{
    struct buf *buf = buf_p;
    size_t avail = buf->size - buf->off;
    if (size > avail)
        size = avail;
    if (size > s_max_read_size)
        size = s_max_read_size;
    *dst = buf->buf + buf->off;
    buf->off += size;
    return size;
}


static void
wantread_header_block (void *buf_p, int wantread)
{
    struct buf *buf = buf_p;
    buf->wantread = wantread;
    if (wantread)
        lsqpack_dec_header_read(buf->dec, buf);
}


static void
header_block_done (void *buf_p, struct lsqpack_header_set *set)
{
    struct buf *buf = buf_p;
    unsigned n;

    fprintf(stderr, "Have headers for stream %"PRIu64":\n", buf->stream_id);
    for (n = 0; n < set->qhs_count; ++n)
        fprintf(stderr, "  %.*s: %.*s\n",
            set->qhs_headers[n]->qh_name_len, set->qhs_headers[n]->qh_name,
            set->qhs_headers[n]->qh_value_len, set->qhs_headers[n]->qh_value);
    fprintf(stderr, "\n");
    lsqpack_dec_destroy_header_set(set);
    TAILQ_REMOVE(&bufs, buf, next_buf);
    free(buf);
}


int
main (int argc, char **argv)
{
    int in = STDIN_FILENO;
    FILE *out = stdout, *recipe = NULL;
    int opt;
    unsigned dyn_table_size     = LSQPACK_DEF_DYN_TABLE_SIZE,
             max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS;
    struct lsqpack_dec decoder;
    ssize_t nr;
    int r;
    uint64_t stream_id;
    uint32_t size;
    struct buf *buf;
    unsigned lineno;
    char *line, *end;
    char command[0x100];
    char line_buf[0x100];

    while (-1 != (opt = getopt(argc, argv, "i:o:r:s:t:h")))
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
                out = fopen(optarg, "w");
                if (!out)
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
                if (!out)
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
        default:
            exit(EXIT_FAILURE);
        }
    }

    lsqpack_dec_init(&decoder, dyn_table_size, max_risked_streams,
                 NULL, NULL, read_header_block, wantread_header_block,
                 header_block_done);

    while (1)
    {
        nr = read(in, &stream_id, sizeof(stream_id));
        if (nr == 0)
            break;
        if (nr != sizeof(stream_id))
            goto read_err;
        nr = read(in, &size, sizeof(size));
        if (nr != sizeof(size))
            goto read_err;
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
                r = lsqpack_dec_header_in(&decoder, buf, buf->size);
                if (r != 0)
                {
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
        if (buf->stream_id == 0)
        {
            r = lsqpack_dec_enc_in(&decoder, buf->buf, buf->size - buf->off);
            if (r != 0)
            {
                fprintf(stderr, "encoder in error\n");
                exit(EXIT_FAILURE);
            }
            TAILQ_REMOVE(&bufs, buf, next_buf);
            free(buf);
            lsqpack_dec_print_table(&decoder, stderr);
        }
        else if (buf->off == 0)
        {
            r = lsqpack_dec_header_in(&decoder, buf, buf->size);
            if (r != 0)
            {
                fprintf(stderr, "header_in error\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            r = lsqpack_dec_header_read(&decoder, buf);
            if (r != 0)
            {
                fprintf(stderr, "header_read error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

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
