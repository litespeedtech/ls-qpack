/*
 * Read encoder stream
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lsqpack.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Dynamic table entry: */
struct lsqpack_dec_table_entry
{
    unsigned    dte_name_len;
    unsigned    dte_val_len;
    unsigned    dte_refcnt;
    char        dte_buf[0];     /* Contains both name and value */
};

#define DTE_NAME(dte) ((dte)->dte_buf)
#define DTE_VALUE(dte) (&(dte)->dte_buf[(dte)->dte_name_len])


struct test_read_encoder_stream
{
    int             lineno;

    /* Input */
    unsigned char   input[0x100];
    size_t          input_sz;

    /* Output */
    unsigned        n_entries;
    struct {
        const char *name, *value;
    }               dyn_table[10];
};


static const struct test_read_encoder_stream tests[] =
{

    {   __LINE__,
        "\xc0\x8b\xf1\xe3\xc2\xf5\x15\x31\xa2\x45\xcf\x64\xdf",
        13,
        1,
        {
            { ":authority", "www.netbsd.org", },
        },
    },

    {   __LINE__,
        "\xc0\x00",
        2,
        1,
        {
            { ":authority", "", },
        },
    },

    {   __LINE__,
        "\xc0\x8b\xf1\xe3\xc2\xf5\x15\x31\xa2\x45\xcf\x64\xdf\x00",
        14,
        2,
        {
            { ":authority", "www.netbsd.org", },
            { ":authority", "www.netbsd.org", },
        },
    },

    {   __LINE__,
        "\xc0\x8b\xf1\xe3\xc2\xf5\x15\x31\xa2\x45\xcf\x64\xdf\x00\x20",
        15,
        0,
        {
            { NULL, NULL, },
        },
    },

    {   __LINE__,
        "\xc0\x15" "Respect my authorata!",
        2 + sizeof("Respect my authorata!") - 1,
        1,
        {
            { ":authority", "Respect my authorata!", },
        },
    },

    {   __LINE__,
        "\x63\x92\xd9\x0b\x8c\xe5\x39\x6c\x2a\x86\x42\x94\xfa\x50\x83\xb3"
        "\xfc",
        17,
        1,
        {
            { "dude", "Where is my car?", },
        },
    },

    {   __LINE__,
        "\x63\x92\xd9\x0b\x03\x61\x61\x7a",
        8,
        1,
        {
            { "dude", "aaz", },
        },
    },

    {   __LINE__,
        "\x43\x7a\x7a\x7a\x88\xcc\x6a\x0d\x48\xea\xe8\x3b\x0f",
        13,
        1,
        {
            { "zzz", "Kilimanjaro", },
        },
    },

    {   __LINE__,
        "\x43\x7a\x7a\x7a\x03\x61\x61\x7a",
        8,
        1,
        {
            { "zzz", "aaz", },
        },
    },

    {   __LINE__,
        "\x43\x7a\x7a\x7a\x03\x61\x61\x61\x80\x03\x61\x61\x7a",
        13,
        2,
        {
            { "zzz", "aaa", },
            { "zzz", "aaz", },
        },
    },

    {   __LINE__,
        "\x40\x88\xcc\x6a\x0d\x48\xea\xe8\x3b\x0f",
        10,
        1,
        {
            { "", "Kilimanjaro", },
        },
    },

    {   __LINE__,
        "\x43\x7a\x7a\x7a\x00",
        5,
        1,
        {
            { "zzz", "", },
        },
    },

    {   __LINE__,
        "\x40\x00",
        2,
        1,
        {
            { "", "", },
        },
    },

};


struct ringbuf_iter
{
    const struct lsqpack_ringbuf *rbuf;
    unsigned next, end;
};


#if LSQPACK_DEVEL_MODE
unsigned
ringbuf_count (const struct lsqpack_ringbuf *rbuf);

void *
ringbuf_iter_next (struct ringbuf_iter *iter);

void *
ringbuf_iter_first (struct ringbuf_iter *iter,
                                        const struct lsqpack_ringbuf *rbuf);

static void
verify_dyn_table (const struct lsqpack_dec *dec,
                            const struct test_read_encoder_stream *test)
{
    const struct lsqpack_dec_table_entry *entry;
    unsigned n;
    struct ringbuf_iter riter;

    assert(ringbuf_count(&dec->qpd_dyn_table) == test->n_entries);

    for (n = 0; n < test->n_entries; ++n)
    {
        if (n == 0)
            entry = ringbuf_iter_first(&riter, &dec->qpd_dyn_table);
        else
            entry = ringbuf_iter_next(&riter);
        assert(entry);
        assert(entry->dte_name_len == strlen(test->dyn_table[n].name));
        assert(entry->dte_val_len == strlen(test->dyn_table[n].value));
        assert(0 == memcmp(DTE_NAME(entry), test->dyn_table[n].name, entry->dte_name_len));
        assert(0 == memcmp(DTE_VALUE(entry), test->dyn_table[n].value, entry->dte_val_len));
    }
}
#else
static void
verify_dyn_table (const struct lsqpack_dec *dec,
                            const struct test_read_encoder_stream *test)
{
    fprintf(stderr, "LSQPACK_DEVEL_MODE is not compiled: cannot verify dynamic table\n");
}
#endif


static void
run_test (const struct test_read_encoder_stream *test)
{
    struct lsqpack_dec dec;
    size_t chunk_sz, off;
    int r;

    for (chunk_sz = 1; chunk_sz <= test->input_sz; ++chunk_sz)
    {
        lsqpack_dec_init(&dec, NULL, 0x1000, 100, NULL);

        off = 0;
        do
        {
            r = lsqpack_dec_enc_in(&dec, test->input + off,
                                        MIN(chunk_sz, test->input_sz - off));
            assert(r == 0);
            off += MIN(chunk_sz, test->input_sz - off);
        }
        while (off < test->input_sz);

        verify_dyn_table(&dec, test);

        lsqpack_dec_cleanup(&dec);
    }
}


int
main (void)
{
    const struct test_read_encoder_stream *test;

    for (test = tests; test < tests + sizeof(tests) / sizeof(tests[0]); ++test)
        run_test(test);

    return 0;
}
