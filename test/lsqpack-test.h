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

struct lsqpack_dec_int_state
{
    int         resume;
    unsigned    M, nread;
    uint64_t    val;
};


int
lsqpack_dec_int_r (const unsigned char **src_p, const unsigned char *src_end,
                   unsigned prefix_bits, uint64_t *value_p,
                   struct lsqpack_dec_int_state *state);

#endif
