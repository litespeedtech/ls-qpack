#ifndef LITESPEED_QPACK_TEST_H
#define LITESPEED_QPACK_TEST_H 1

#ifndef LS_QPACK_USE_LARGE_TABLES
#define LS_QPACK_USE_LARGE_TABLES 1
#endif

#if !LS_QPACK_USE_LARGE_TABLES
#define lsqpack_huff_decode_full lsqpack_huff_decode
#endif

int
lsqpack_enc_enc_str (unsigned prefix_bits, unsigned char *const dst,
    size_t dst_len, const unsigned char *str, unsigned str_len);

unsigned char *
lsqpack_enc_int (unsigned char *dst, unsigned char *const end, uint64_t value,
                                                        unsigned prefix_bits);

int
lsqpack_dec_int (const unsigned char **src_p, const unsigned char *src_end,
                 unsigned prefix_bits, uint64_t *value_p,
                 struct lsqpack_dec_int_state *state);

struct huff_decode_retval
{
    enum
    {
        HUFF_DEC_OK,
        HUFF_DEC_END_SRC,
        HUFF_DEC_END_DST,
        HUFF_DEC_ERROR,
    }                       status;
    unsigned                n_dst;
    unsigned                n_src;
};

struct huff_decode_retval
lsqpack_huff_decode_full (const unsigned char *src, int src_len,
            unsigned char *dst, int dst_len,
            struct lsqpack_huff_decode_state *state, int final);

int
lsqpack_find_in_static_headers (uint32_t name_hash, const char *name,
                                                        unsigned name_len);

#endif
