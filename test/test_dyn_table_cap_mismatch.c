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

    FILE *encoder_stream = fopen("testdata/encoder_stream", "r");
    FILE *response = fopen("testdata/response", "r");
    uint8_t buffer[16384];
    size_t size = 0;
    if (!encoder_stream)
    {
        encoder_stream = fopen("../../test/testdata/encoder_stream", "r");
        response = fopen("../../test/testdata/response", "r");
    }
    while ((size = fread(buffer, 1, sizeof(buffer), encoder_stream)) > 0) {
        lsqpack_dec_enc_in(&qpackDecoder, buffer, size);
    }
    fclose(encoder_stream);

    size = fread(buffer, 1, sizeof(buffer), response);
    uint8_t decoderBuffer[LSQPACK_LONGEST_HEADER_ACK];
    size_t decoderBufferSize = sizeof(decoderBuffer);
    const uint8_t *bufferRef = buffer;
    enum lsqpack_read_header_status status = lsqpack_dec_header_in(&qpackDecoder, NULL, 2092, size, &bufferRef, size, decoderBuffer, &decoderBufferSize);
    printf("Decode status %d\n", status);
    fclose(response);

    return 0;
}
