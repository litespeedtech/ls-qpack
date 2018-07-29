#ifndef LITESPEED_QPACK_TEST_H
#define LITESPEED_QPACK_TEST_H 1

int
lsqpack_enc_enc_str (unsigned prefix_bits, unsigned char *const dst,
    size_t dst_len, const unsigned char *str, lsqpack_strlen_t str_len);

int
lsqpack_dec_int (const unsigned char **src, const unsigned char *src_end,
                                    unsigned prefix_bits, uint64_t *value);

unsigned char *
lsqpack_enc_int (unsigned char *dst, unsigned char *const end, uint64_t value,
                                                        unsigned prefix_bits);

int
lsqpack_dec_int_r (const unsigned char **src_p, const unsigned char *src_end,
                   unsigned prefix_bits, uint64_t *value_p,
                   struct lsqpack_dec_int_state *state);

struct decode_status
{
    uint8_t state;
    uint8_t eos;
};

struct huff_decode_state
{
    int                     resume;
    struct decode_status    status;
};

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
lsqpack_huff_decode_r (const unsigned char *src, int src_len,
            unsigned char *dst, int dst_len, struct huff_decode_state *state,
            int final);

#endif
