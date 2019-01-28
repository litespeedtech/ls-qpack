/* encode-int: encode an integer into HPACK varint. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


static unsigned char *
enc_int (unsigned char *dst, unsigned char *const end, uint64_t value,
                                                        unsigned prefix_bits)
{
    unsigned char *const dst_orig = dst;

    /* This function assumes that at least one byte is available */
    assert(dst < end);
    if (value < (1u << prefix_bits) - 1)
        *dst++ |= value;
    else
    {
        *dst++ |= (1 << prefix_bits) - 1;
        value -= (1 << prefix_bits) - 1;
        while (value >= 128)
        {
            if (dst < end)
            {
                *dst++ = 0x80 | (unsigned char) value;
                value >>= 7;
            }
            else
                return dst_orig;
        }
        if (dst < end)
            *dst++ = (unsigned char) value;
        else
            return dst_orig;
    }
    return dst;
}

static unsigned
lsqpack_val2len (uint64_t value, unsigned prefix_bits)
{
    uint64_t mask = (1ULL << prefix_bits) - 1;
    return 1
         + (value >=                 mask )
         + (value >= ((1ULL <<  7) + mask))
         + (value >= ((1ULL << 14) + mask))
         + (value >= ((1ULL << 21) + mask))
         + (value >= ((1ULL << 28) + mask))
         + (value >= ((1ULL << 35) + mask))
         + (value >= ((1ULL << 42) + mask))
         + (value >= ((1ULL << 49) + mask))
         + (value >= ((1ULL << 56) + mask))
         + (value >= ((1ULL << 63) + mask))
         ;
}

int
main (int argc, char **argv)
{
    unsigned long long val;
    unsigned char *p;
    unsigned prefix_bits;
    unsigned char buf[20];

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s prefix_bits value\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    prefix_bits = atoi(argv[1]);
    val = strtoull(argv[2], NULL, 10);

    fprintf(stderr, "expected size: %u\n", lsqpack_val2len(val, prefix_bits));
    buf[0] = 0;
    p = enc_int(buf, buf + sizeof(buf), val, prefix_bits);

    if (p > buf)
    {
        fwrite(buf, 1, p - buf, stdout);
        exit(EXIT_SUCCESS);
    }
    else
        exit(EXIT_FAILURE);
}
