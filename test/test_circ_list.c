/* Bug report and test case contributed by Guoye Zhang -- thanks! */

#include "lsxpack_header.h"
#include "lsqpack.h"

static void _decoderUnblocked(void *context) {

}

static struct lsxpack_header *_decoderPrepareDecode(void *context, struct lsxpack_header *header, size_t space) {

    return NULL;

}

static int _decoderProcessHeader(void *context, struct lsxpack_header *header) {

    return 0;

}

int main(int argc, const char * argv[]) {

    struct lsqpack_enc qpackEncoder;

    struct lsqpack_dec qpackDecoder;

    static struct lsqpack_dec_hset_if callbacks = {

        .dhi_unblocked = _decoderUnblocked,

        .dhi_prepare_decode = _decoderPrepareDecode,

        .dhi_process_header = _decoderProcessHeader,

    };

    uint8_t stdc[LSQPACK_LONGEST_SDTC];

    size_t stdcSize = sizeof(stdc);

    lsqpack_enc_init(&qpackEncoder, NULL, 4096, 4096, 100, LSQPACK_ENC_OPT_SERVER, stdc, &stdcSize);

    lsqpack_dec_init(&qpackDecoder, stdout, 4096, 100, &callbacks, (enum lsqpack_dec_opts)0);

    lsqpack_dec_enc_in(&qpackDecoder, stdc, stdcSize);

    uint8_t iciBuffer[LSQPACK_LONGEST_ICI];

    ssize_t iciSize = 0;

    for (unsigned i = 0; i < 6; i++) {

        uint8_t encoderBuffer[1024];

        size_t usedEncoderSize = 0, encoderSize = sizeof(encoderBuffer);

        uint8_t headerBuffer[1024];

        size_t usedHeaderSize = 0, headerSize = sizeof(headerBuffer);

        struct lsxpack_header header;
        memset(&header, 0, sizeof(header));

        const char *name, *value;

        lsqpack_enc_start_header(&qpackEncoder, 0, i);

        name = ":authority" "localhost";

        lsxpack_header_set_offset2(&header, name, 0, strlen(":authority"), strlen(":authority"), strlen("localhost"));

        enum lsqpack_enc_status status = lsqpack_enc_encode(&qpackEncoder, &encoderBuffer[usedEncoderSize], &encoderSize, &headerBuffer[usedHeaderSize], &headerSize, &header, (enum lsqpack_enc_flags)0);

        assert(status == LQES_OK);

        usedEncoderSize += encoderSize; usedHeaderSize += headerSize;

        encoderSize = sizeof(encoderBuffer) - usedEncoderSize; headerSize = sizeof(headerBuffer) - usedHeaderSize;

        uint8_t prefixBuffer[22];

        lsqpack_enc_end_header(&qpackEncoder, prefixBuffer, sizeof(prefixBuffer), NULL);

        if (usedEncoderSize > 0) {

            lsqpack_dec_enc_in(&qpackDecoder, encoderBuffer, usedEncoderSize);

            if (lsqpack_dec_ici_pending(&qpackDecoder)) {

                iciSize = lsqpack_dec_write_ici(&qpackDecoder, iciBuffer, sizeof(iciBuffer));

            }

        }

    }

    lsqpack_enc_decoder_in(&qpackEncoder, iciBuffer, iciSize);

    return 0;

}

