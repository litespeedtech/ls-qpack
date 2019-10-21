/*
 * QPACK Interop -- encode to intermediate format
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include "lsqpack.h"

static int s_verbose;

unsigned char *
lsqpack_enc_int (unsigned char *dst, unsigned char *const end, uint64_t value,
                                                        unsigned prefix_bits);

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
"   -a MODE     Header acknowledgement mode.  0 means headers are never\n"
"                 acknowledged, non-zero means header blocks are acknowledged\n"
"                 immediately.  Default value is 0.\n"
"   -n          Process annotations.\n"
"   -S          Server mode.\n"
"   -D          Do not emit \"Duplicate\" instructions.\n"
"   -A          Aggressive indexing.\n"
"   -M          Turn off memory guard.\n"
"   -f          Fast: use maximum output buffers.\n"
"   -v          Verbose: print various messages to stderr.\n"
"\n"
"   -h          Print this help screen and exit\n"
    , name, LSQPACK_DEF_MAX_RISKED_STREAMS, LSQPACK_DEF_DYN_TABLE_SIZE);
}


static void
write_enc_stream (FILE *out, const unsigned char *enc_buf, size_t enc_sz)
{
    uint64_t stream_id_enc;
    uint32_t length_enc;
    size_t written;

    if (enc_sz <= 0)
        return;
    stream_id_enc = 0;
    length_enc = enc_sz;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    stream_id_enc = bswap_64(stream_id_enc);
    length_enc = bswap_32(length_enc);
#endif
    written = fwrite(&stream_id_enc, 1, sizeof(stream_id_enc), out);
    if (written != sizeof(stream_id_enc))
    {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
    written = fwrite(&length_enc, 1, sizeof(length_enc), out);
    if (written != sizeof(length_enc))
    {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
    written = fwrite(enc_buf, 1, enc_sz, out);
    if (written != enc_sz)
    {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
}


static void
write_enc_and_header_streams (FILE *out, unsigned stream_id,
                              const unsigned char *enc_buf, size_t enc_sz,
                              const unsigned char *pref_buf, size_t pref_sz,
                              const unsigned char *hea_buf, size_t hea_sz)
{
    uint64_t stream_id_enc;
    uint32_t length_enc;
    size_t written;

    if (s_verbose)
        fprintf(stderr, "%s: stream %"PRIu32"\n", __func__, stream_id);

    if (enc_sz)
    {
        stream_id_enc = 0;
        length_enc = enc_sz;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        stream_id_enc = bswap_64(stream_id_enc);
        length_enc = bswap_32(length_enc);
#endif
        written = fwrite(&stream_id_enc, 1, sizeof(stream_id_enc), out);
        if (written != sizeof(stream_id_enc))
            goto write_err;
        written = fwrite(&length_enc, 1, sizeof(length_enc), out);
        if (written != sizeof(length_enc))
            goto write_err;
        written = fwrite(enc_buf, 1, enc_sz, out);
        if (written != enc_sz)
            goto write_err;
    }

    stream_id_enc = stream_id;
    length_enc = pref_sz + hea_sz;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    stream_id_enc = bswap_64(stream_id_enc);
    length_enc = bswap_32(length_enc);
#endif
    written = fwrite(&stream_id_enc, 1, sizeof(stream_id_enc), out);
    if (written != sizeof(stream_id_enc))
        goto write_err;
    written = fwrite(&length_enc, 1, sizeof(length_enc), out);
    if (written != sizeof(length_enc))
        goto write_err;
    written = fwrite(pref_buf, 1, pref_sz, out);
    if (written != pref_sz)
        goto write_err;
    written = fwrite(hea_buf, 1, hea_sz, out);
    if (written != hea_sz)
        goto write_err;
    return;

  write_err:
    perror("fwrite");
    exit(EXIT_FAILURE);
}


static unsigned s_saved_ins_count;
static int
ack_last_entry_id (struct lsqpack_enc *encoder)
{
    unsigned char *end_cmd;
    unsigned char cmd[80];
    unsigned val;

    if (s_verbose)
        fprintf(stderr, "ACK entry ID %u\n", encoder->qpe_ins_count);

    cmd[0] = 0x00;
    val = encoder->qpe_ins_count - s_saved_ins_count;
    s_saved_ins_count = encoder->qpe_ins_count;
    end_cmd = lsqpack_enc_int(cmd, cmd + sizeof(cmd), val, 6);
    assert(end_cmd > cmd);
    return lsqpack_enc_decoder_in(encoder, cmd, end_cmd - cmd);
}


static int
ack_stream (struct lsqpack_enc *encoder, uint64_t stream_id)
{
    unsigned char *end_cmd;
    unsigned char cmd[80];

    if (s_verbose)
        fprintf(stderr, "ACK stream ID %"PRIu64"\n", stream_id);

    cmd[0] = 0x80;
    end_cmd = lsqpack_enc_int(cmd, cmd + sizeof(cmd), stream_id, 7);
    assert(end_cmd > cmd);
    return lsqpack_enc_decoder_in(encoder, cmd, end_cmd - cmd);
}


static int
sync_table (struct lsqpack_enc *encoder, uint64_t num_inserts)
{
    unsigned char *end_cmd;
    unsigned char cmd[80];

    if (s_verbose)
        fprintf(stderr, "Sync table num inserts %"PRIu64"\n", num_inserts);

    cmd[0] = 0x00;
    end_cmd = lsqpack_enc_int(cmd, cmd + sizeof(cmd), num_inserts, 6);
    assert(end_cmd > cmd);
    return lsqpack_enc_decoder_in(encoder, cmd, end_cmd - cmd);
}


static int
cancel_stream (struct lsqpack_enc *encoder, uint64_t stream_id)
{
    unsigned char *end_cmd;
    unsigned char cmd[80];

    if (s_verbose)
        fprintf(stderr, "Cancel stream ID %"PRIu64"\n", stream_id);

    cmd[0] = 0x40;
    end_cmd = lsqpack_enc_int(cmd, cmd + sizeof(cmd), stream_id, 6);
    assert(end_cmd > cmd);
    return lsqpack_enc_decoder_in(encoder, cmd, end_cmd - cmd);
}


int
main (int argc, char **argv)
{
    FILE *in = stdin;
    FILE *out = stdout;
    int opt;
    unsigned dyn_table_size     = LSQPACK_DEF_DYN_TABLE_SIZE,
             max_risked_streams = LSQPACK_DEF_MAX_RISKED_STREAMS;
    unsigned lineno, stream_id;
    struct lsqpack_enc encoder;
    char *line, *end, *tab;
    ssize_t pref_sz;
    enum lsqpack_enc_status st;
    enum lsqpack_enc_opts enc_opts = 0;
    size_t enc_sz, hea_sz, enc_off, hea_off;
    int header_opened, r;
    unsigned arg;
    enum { ACK_NEVER, ACK_IMMEDIATE, } ack_mode = ACK_NEVER;
    int process_annotations = 0;
    char line_buf[0x1000];
    unsigned char tsu_buf[LSQPACK_LONGEST_SDTC];
    size_t tsu_buf_sz;
    enum lsqpack_enc_header_flags hflags;
    int fast = 0;
    unsigned char enc_buf[0x1000], hea_buf[0x1000], pref_buf[0x20];

    while (-1 != (opt = getopt(argc, argv, "ADMSa:i:no:s:t:hvf")))
    {
        switch (opt)
        {
        case 'S':
            enc_opts |= LSQPACK_ENC_OPT_SERVER;
            break;
        case 'D':
            enc_opts |= LSQPACK_ENC_OPT_NO_DUP;
            break;
        case 'A':
            enc_opts |= LSQPACK_ENC_OPT_IX_AGGR;
            break;
        case 'M':
            enc_opts |= LSQPACK_ENC_OPT_NO_MEM_GUARD;
            break;
        case 'n':
            ++process_annotations;
            break;
        case 'a':
            ack_mode = atoi(optarg) ? ACK_IMMEDIATE : ACK_NEVER;
            break;
        case 'i':
            if (0 != strcmp(optarg, "-"))
            {
                in = fopen(optarg, "r");
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
                out = fopen(optarg, "wb");
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
        case 'f':
            fast = 1;
            break;
        case 'v':
            ++s_verbose;
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }

    tsu_buf_sz = sizeof(tsu_buf);
    if (0 != lsqpack_enc_init(&encoder, s_verbose ? stderr : NULL, dyn_table_size,
                    dyn_table_size, max_risked_streams, enc_opts, tsu_buf,
                    &tsu_buf_sz))
    {
        perror("lsqpack_enc_init");
        exit(EXIT_FAILURE);
    }

    lineno = 0;
    stream_id = 0;
    enc_off = 0;
    hea_off = 0;
    header_opened = 0;

    while (line = fgets(line_buf, sizeof(line_buf), in), line != NULL)
    {
        ++lineno;
        end = strchr(line, '\n');
        if (!end)
        {
            fprintf(stderr, "no newline on line %u\n", lineno);
            exit(EXIT_FAILURE);
        }
        *end = '\0';

        if (end == line)
        {
            if (header_opened)
            {
                size_t sz, pref_max = sizeof(pref_buf);
                for (sz = (fast ? pref_max : 0); sz <= pref_max; sz++)
                {
                    pref_sz = lsqpack_enc_end_header(&encoder, pref_buf, sz, &hflags);
                    if (pref_sz > 0)
                    {
                        if (max_risked_streams == 0)
                            assert(!(hflags & LSQECH_REF_AT_RISK));
                        break;
                    }
                }
                assert(pref_sz <= lsqpack_enc_header_block_prefix_size(&encoder));
                if (pref_sz < 0)
                {
                    fprintf(stderr, "end_header failed: %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                if (ack_mode == ACK_IMMEDIATE)
                {
                    if (!(2 == pref_sz && pref_buf[0] == 0 && pref_buf[1] == 0))
                        r = ack_stream(&encoder, stream_id);
                    else
                        r = 0;
                    if (r == 0 && encoder.qpe_ins_count > s_saved_ins_count)
                        r = ack_last_entry_id(&encoder);
                    else
                        r = 0;
                    if (r != 0)
                    {
                        fprintf(stderr, "acking stream %u failed: %s", stream_id,
                                                                    strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                }
                if (s_verbose)
                    fprintf(stderr, "compression ratio: %.3f\n",
                        lsqpack_enc_ratio(&encoder));
                write_enc_and_header_streams(out, stream_id, enc_buf, enc_off,
                                             pref_buf, pref_sz, hea_buf, hea_off);
                enc_off = 0;
                hea_off = 0;
                header_opened = 0;
            }
            continue;
        }

        if (*line == '#')
        {
            if (!process_annotations)
                continue;

            /* Lines starting with ## are potential annotations */
            if (ack_mode != ACK_IMMEDIATE
                /* Ignore ACK annotations in immediate ACK mode, as we do
                 * not tolerate duplicate ACKs.
                 */
                                && 1 == sscanf(line, "## %*[a] %u ", &arg))
            {
                if (0 != ack_stream(&encoder, arg))
                {
                    fprintf(stderr, "ACKing stream ID %u failed\n", arg);
                    exit(EXIT_FAILURE);
                }
            }
            else if (1 == sscanf(line, "## %*[s] %u", &arg))
                sync_table(&encoder, arg);
            else if (1 == sscanf(line, "## %*[c] %u", &arg))
                cancel_stream(&encoder, arg);
            else if (1 == sscanf(line, "## %*[t] %u", &arg))
            {
                tsu_buf_sz = sizeof(tsu_buf);
                if (0 != lsqpack_enc_set_max_capacity(&encoder, arg, tsu_buf,
                                                                &tsu_buf_sz))
                {
                    fprintf(stderr, "cannot set capacity to %u: %s\n", arg,
                        strerror(errno));
                    exit(EXIT_FAILURE);
                }
                write_enc_stream(out, tsu_buf, tsu_buf_sz);
            }
            continue;
        }

        tab = strchr(line, '\t');
        if (!tab)
        {
            fprintf(stderr, "no TAB on line %u\n", lineno);
            exit(EXIT_FAILURE);
        }

        if (!header_opened)
        {
            ++stream_id;
            if (0 != lsqpack_enc_start_header(&encoder, stream_id, 0))
            {
                fprintf(stderr, "start_header failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            header_opened = 1;
        }
        if (fast)
        {
            enc_sz = sizeof(enc_buf) - enc_off;
            hea_sz = sizeof(hea_buf) - hea_off;
        }
        else
        {
            /* Increase buffers one by one to exercise error conditions */
            enc_sz = 0;
            hea_sz = 0;
        }
        while (1)
        {
            st = lsqpack_enc_encode(&encoder, enc_buf + enc_off, &enc_sz,
                        hea_buf + hea_off, &hea_sz, line, tab - line,
                        tab + 1, end - tab - 1, 0);
            switch (st)
            {
            case LQES_NOBUF_ENC:
                if (enc_sz < sizeof(enc_buf) - enc_off)
                    ++enc_sz;
                else
                    assert(0);
                break;
            case LQES_NOBUF_HEAD:
                if (hea_sz < sizeof(hea_buf) - hea_off)
                    ++hea_sz;
                else
                    assert(0);
                break;
            default:
                assert(st == LQES_OK);
                goto end_encode_one_header;
            }
        }
        if (st != LQES_OK)
        {
            /* It could only run of of output space, so it's not really an
             * error, but we make no provision in the interop encoder to
             * grow the buffers.
             */
            fprintf(stderr, "Could not encode header on line %u: %u\n",
                                                                lineno, st);
            exit(EXIT_FAILURE);
        }
    end_encode_one_header:
        enc_off += enc_sz;
        hea_off += hea_sz;
    }

    if (s_verbose)
        fprintf(stderr, "exited while loop\n");

    (void) fclose(in);

    if (header_opened)
    {
        if (s_verbose)
            fprintf(stderr, "close opened header\n");
        pref_sz = lsqpack_enc_end_header(&encoder, pref_buf, sizeof(pref_buf),
                                                                    &hflags);
        if (pref_sz < 0)
        {
            fprintf(stderr, "end_header failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (max_risked_streams == 0)
            assert(!(hflags & LSQECH_REF_AT_RISK));
        if (ack_mode == ACK_IMMEDIATE
            && !(2 == pref_sz && pref_buf[0] == 0 && pref_buf[1] == 0)
            && 0 != ack_stream(&encoder, stream_id))
        {
            fprintf(stderr, "acking stream %u failed: %s", stream_id,
                                                                strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (s_verbose)
            fprintf(stderr, "compression ratio: %.3f\n",
                lsqpack_enc_ratio(&encoder));
        write_enc_and_header_streams(out, stream_id, enc_buf, enc_off, pref_buf,
                                     pref_sz, hea_buf, hea_off);
    }

    lsqpack_enc_cleanup(&encoder);

    if (0 != fclose(out))
    {
        perror("fclose(out)");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
