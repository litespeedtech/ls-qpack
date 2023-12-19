#include <stdio.h>
#include <stdlib.h>
#include "lsqpack.h"
#include "lsxpack_header.h"

static void _decoderUnblocked(void *context) {
}

static struct lsxpack_header *_decoderPrepareDecode(void *context, struct lsxpack_header *header, size_t space)
{
    static struct lsxpack_header xpackHeader;
    char *xpackHeaderBuffer = (char *)malloc(space);
    lsxpack_header_prepare_decode(&xpackHeader, xpackHeaderBuffer, 0, space);
    return &xpackHeader;
}

static int _decoderProcessHeader(void *context, struct lsxpack_header *header)
{
    printf("Decode header %.*s: %.*s\n", header->name_len, &header->buf[header->name_offset], header->val_len, &header->buf[header->val_offset]);
    return 0;
}

int main(int argc, const char * argv[]) {
    struct lsqpack_dec qpackDecoder;

    struct lsqpack_dec_hset_if callbacks = {
        .dhi_unblocked = _decoderUnblocked,
        .dhi_prepare_decode = _decoderPrepareDecode,
        .dhi_process_header = _decoderProcessHeader,
    };
    lsqpack_dec_init(&qpackDecoder, stderr, 16384, 100, &callbacks, (enum lsqpack_dec_opts)0);

    size_t size = 50;
    uint8_t buffer[16384] = {
        0x01, 0x32, 0x00, 0x00, 0x50, 0x91, 0x19, 0x08,
        0x54, 0x21, 0x7a, 0x0e, 0x41, 0xd0, 0x92, 0xa1,
        0x2b, 0xd2, 0x5b, 0x8f, 0xbe, 0xd3, 0x3f, 0xd1,
        0x51, 0x85, 0x62, 0x53, 0x9d, 0x25, 0xb3, 0xd7,
        0x5f, 0x10, 0x83, 0x9b, 0xd9, 0xab, 0x5f, 0x50,
        0x8b, 0xed, 0x69, 0x88, 0xb4, 0xc7, 0x53, 0x1e,
        0xfd, 0xfa,
    };
    uint8_t decoderBuffer[LSQPACK_LONGEST_HEADER_ACK];
    size_t decoderBufferSize = sizeof(decoderBuffer);
    const uint8_t *bufferRef = buffer;
    enum lsqpack_read_header_status status = lsqpack_dec_header_in(&qpackDecoder, NULL, 0, size, &bufferRef, size, decoderBuffer, &decoderBufferSize);
    printf("Decode status %d\n", status);

    return 0;
}
