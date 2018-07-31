/*
 * QPACK Interop -- decode from intermediate format and output QIF
 *
 * https://github.com/quicwg/base-drafts/wiki/QPACK-Offline-Interop
 */

#include <byteswap.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lsqpack.h"

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
"   -s NUMBER   Maximum number of risked streams.  Defaults to %u.\n"
"   -t NUMBER   Dynamic table size.  Defaults to %u.\n"
"\n"
"   -h          Print this help screen and exit\n"
    , name, LSQPACK_DEF_MAX_RISKED_STREAMS, LSQPACK_DEF_DYN_TABLE_SIZE);
}


int
main (int argc, char **argv)
{
    int in = STDIN_FILENO;
    FILE *out = stdout;
    int opt;
    unsigned dyn_table_size     = LSQPACK_DEF_DYN_TABLE_SIZE,
             max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS;
    struct lsqpack_dec decoder;
    ssize_t nr;
    int r;
    unsigned char buf[0x1000];
    uint64_t stream_id;
    uint32_t size;

    while (-1 != (opt = getopt(argc, argv, "i:o:s:t:h")))
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
                     NULL, NULL, NULL, NULL);

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
        if (size > sizeof(buf))
        {
            fprintf(stderr, "span larger than the buffer\n");
            exit(EXIT_FAILURE);
        }
        nr = read(in, buf, size);
        if (nr != size)
            goto read_err;
        if (stream_id == 0)
        {
            r = lsqpack_dec_enc_in(&decoder, buf, size);
            if (r != 0)
            {
                fprintf(stderr, "encoder in error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    lsqpack_dec_cleanup(&decoder);

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
