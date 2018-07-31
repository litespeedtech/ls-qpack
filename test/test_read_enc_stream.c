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
        "\xc1\x8b\xf1\xe3\xc2\xf5\x15\x31\xa2\x45\xcf\x64\xdf",
        13,
        1,
        {
            { ":authority", "www.netbsd.org", },
        },
    },

    {   __LINE__,
        "\xc1\x8b\xf1\xe3\xc2\xf5\x15\x31\xa2\x45\xcf\x64\xdf\x01",
        14,
        2,
        {
            { ":authority", "www.netbsd.org", },
            { ":authority", "www.netbsd.org", },
        },
    },

    {   __LINE__,
        "\xc1\x8b\xf1\xe3\xc2\xf5\x15\x31\xa2\x45\xcf\x64\xdf\x01\x20",
        15,
        0,
        {
        },
    },

};


static void
verify_dyn_table (const struct lsqpack_dec *dec,
                            const struct test_read_encoder_stream *test)
{
    const struct lsqpack_dec_table_entry *entry;
    unsigned n;

    assert(dec->qpd_dyn_table.nelem == test->n_entries);

    for (n = 0; n < test->n_entries; ++n)
    {
        entry = (void *) dec->qpd_dyn_table.els[ dec->qpd_dyn_table.off + n ];
        assert(entry->dte_name_len == strlen(test->dyn_table[n].name));
        assert(entry->dte_val_len == strlen(test->dyn_table[n].value));
        assert(0 == memcmp(DTE_NAME(entry), test->dyn_table[n].name, entry->dte_name_len));
        assert(0 == memcmp(DTE_VALUE(entry), test->dyn_table[n].value, entry->dte_val_len));
    }
}


static void
run_test (const struct test_read_encoder_stream *test)
{
    struct lsqpack_dec dec;
    size_t chunk_sz, off;
    int r;

    for (chunk_sz = 1; chunk_sz <= test->input_sz; ++chunk_sz)
    {
        lsqpack_dec_init(&dec, LSQPACK_DEF_DYN_TABLE_SIZE,
            LSQPACK_DEF_MAX_RISKED_STREAMS, NULL, NULL, NULL, NULL);

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
}
