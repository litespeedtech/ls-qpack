/*
 * fuzz-decode: special program for fuzzing.  It still reads encoded
 * files just like interop-decode, but tries to do it faster and
 * forgoes several advanced options.
 */

#ifdef WIN32

#include <stdio.h>

int
main (int argc, char **argv)
{
    fprintf(stderr, "%s is not supported on Windows: need mmap(2)\n", argv[0]);
    return 1;
}

#else

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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "lsqpack.h"
#include "lsxpack_header.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void
usage (const char *name)
{
    fprintf(stderr,
"Usage: %s [options] [-i input] [-o output]\n"
"\n"
"Options:\n"
"   -i FILE     Input file.\n"
"   -f FILE     Fuzz file: this is the stuff the fuzzer will change.\n"
"   -s NUMBER   Maximum number of risked streams.  Defaults to %u.\n"
"   -t NUMBER   Dynamic table size.  Defaults to %u.\n"
"\n"
"   -h          Print this help screen and exit\n"
    , name, LSQPACK_DEF_MAX_RISKED_STREAMS, LSQPACK_DEF_DYN_TABLE_SIZE);
}


static void
hblock_unblocked (void *buf_p)
{
    exit(1);
}


struct header
{
    LIST_ENTRY(header)      next_header;
    struct lsxpack_header   xhdr;
    char                    buf[0x10000];
};


LIST_HEAD(, header)     s_headers;


static struct lsxpack_header *
prepare_decode (void *hblock_ctx, struct lsxpack_header *xhdr, size_t space)
{
    struct header *header;

    if (xhdr)
    {
        /* If the original buffer is not enough, we don't reallocate */
        header = (struct header *) ((char *) xhdr
                                        - offsetof(struct header, xhdr));
        LIST_REMOVE(header, next_header);
        free(header);
        return NULL;
    }
    else if (space <= LSXPACK_MAX_STRLEN)
    {
        header = malloc(sizeof(*header));
        if (!header)
            return NULL;
        LIST_INSERT_HEAD(&s_headers, header, next_header);
        lsxpack_header_prepare_decode(&header->xhdr, header->buf,
                                                0, sizeof(header->buf));
        return &header->xhdr;
    }
    else
        return NULL;
}


static int
process_header (void *hblock_ctx, struct lsxpack_header *xhdr)
{
    struct header *header;

    header = (struct header *) ((char *) xhdr - offsetof(struct header, xhdr));
    LIST_REMOVE(header, next_header);
    free(header);
    return 0;
}


static const struct lsqpack_dec_hset_if hset_if = {
    .dhi_unblocked      = hblock_unblocked,
    .dhi_prepare_decode = prepare_decode,
    .dhi_process_header = process_header,
};


int
main (int argc, char **argv)
{
    int in_fd = -1, fuzz_fd = STDIN_FILENO;
    int opt;
    unsigned dyn_table_size     = LSQPACK_DEF_DYN_TABLE_SIZE,
             max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS;
    struct lsqpack_dec decoder;
    const unsigned char *p, *end;
    unsigned char *begin;
    struct stat st;
    uint64_t stream_id;
    uint32_t size;
    int r;
    struct header *header;

    while (-1 != (opt = getopt(argc, argv, "i:f:s:t:h")))
    {
        switch (opt)
        {
        case 'i':
            in_fd = open(optarg, O_RDONLY);
            if (in_fd < 0)
            {
                fprintf(stderr, "cannot open `%s' for reading: %s\n",
                                            optarg, strerror(errno));
                exit(EXIT_FAILURE);
            }
            break;
        case 'f':
            fuzz_fd = open(optarg, O_RDONLY);
            if (fuzz_fd < 0)
            {
                fprintf(stderr, "cannot open `%s' for reading: %s\n",
                                            optarg, strerror(errno));
                exit(EXIT_FAILURE);
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

    LIST_INIT(&s_headers);
    lsqpack_dec_init(&decoder, NULL, dyn_table_size, max_risked_streams,
                            &hset_if, 0 /* TODO: add command-line option */);

    if (in_fd < 0)
    {
        fprintf(stderr, "Specify input using `-i' option\n");
        exit(1);
    }

    if (0 != fstat(in_fd, &st))
    {
        perror("fstat");
        exit(1);
    }

    begin = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
    if (!begin)
    {
        perror("mmap");
        exit(1);
    }

    p = begin;
    end = begin + st.st_size;
    while (p + sizeof(stream_id) + sizeof(size) < end)
    {
        stream_id = * (uint64_t *) p;
        p += sizeof(uint64_t);
        size = * (uint32_t *) p;
        p += sizeof(uint32_t);
#if __BYTE_ORDER == __LITTLE_ENDIAN
        stream_id = bswap_64(stream_id);
        size = bswap_32(size);
#endif
        if (p + size > end)
        {
            fprintf(stderr, "truncated input at offset %u",
                                                (unsigned) (p - begin));
            abort();
        }
        if (stream_id == 0)
        {
            r = lsqpack_dec_enc_in(&decoder, p, size);
            if (r != 0)
                abort();
        }
        else
        {
            const unsigned char *cur = p;
            enum lsqpack_read_header_status rhs;
            rhs = lsqpack_dec_header_in(&decoder, NULL, stream_id,
                        size, &cur, size, NULL, NULL);
            if (rhs != LQRHS_DONE || (uint32_t) (cur - p) != size)
                abort();
        }
        p += size;
    }

    munmap(begin, st.st_size);
    (void) close(in_fd);

    /* Now let's read the fuzzed part */

    if (0 != fstat(fuzz_fd, &st))
    {
        perror("fstat");
        exit(1);
    }

    begin = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fuzz_fd, 0);
    if (!begin)
    {
        perror("mmap");
        exit(1);
    }

    p = begin;
    end = begin + st.st_size;
    while (p + sizeof(stream_id) + sizeof(size) < end)
    {
        stream_id = * (uint64_t *) p;
        p += sizeof(uint64_t);
        size = * (uint32_t *) p;
        p += sizeof(uint32_t);
#if __BYTE_ORDER == __LITTLE_ENDIAN
        stream_id = bswap_64(stream_id);
        size = bswap_32(size);
#endif
        if (stream_id == 0)
        {
            r = lsqpack_dec_enc_in(&decoder, p, MIN(size, (uint32_t) (end - p)));
            (void) r;
        }
        else
        {
            const unsigned char *cur = p;
            enum lsqpack_read_header_status rhs;
            size = MIN(size, (uint32_t) (end - p));
            rhs = lsqpack_dec_header_in(&decoder, NULL, stream_id,
                        size, &cur, size, NULL, NULL);
            (void) rhs;
        }
        break;
    }

    munmap(begin, st.st_size);
    (void) close(fuzz_fd);

    lsqpack_dec_cleanup(&decoder);
    while (!LIST_EMPTY(&s_headers))
    {
        header = LIST_FIRST(&s_headers);
        LIST_REMOVE(header, next_header);
        free(header);
    }

    exit(0);
}

#endif
