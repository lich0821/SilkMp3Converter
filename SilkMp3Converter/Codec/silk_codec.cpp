#include <stdio.h>
#include <string.h>
#include <vector>

#include "SKP_Silk_SDK_API.h"
#include "SKP_Silk_SigProc_FIX.h"

#include "codec.h"

/* Define codec specific settings should be moved to h file */
#define MAX_BYTES_PER_FRAME 1024
#define MAX_INPUT_FRAMES    5
#define MAX_FRAME_LENGTH    480
#define FRAME_LENGTH_MS     20
#define MAX_API_FS_KHZ      48
#define MAX_LBRR_DELAY      2

using namespace std;

size_t SilkDecode(vector<uint8_t> &silk, vector<uint8_t> &pcm, int32_t sr)
{
    // TODO: Fix stack size
    size_t counter = 0;
    int32_t totPackets, i, k;
    int16_t ret, len, tot_len;
    int16_t nBytes;
    uint8_t payload[MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES * (MAX_LBRR_DELAY + 1)];
    uint8_t *payloadEnd = NULL, *payloadToDec = NULL;
    int16_t nBytesPerPacket[MAX_LBRR_DELAY + 1], totBytes;
    int16_t out[((FRAME_LENGTH_MS * MAX_API_FS_KHZ) << 1) * MAX_INPUT_FRAMES], *outPtr;
    int32_t decSizeBytes;
    void *psDec;
    int32_t frames;
    SKP_SILK_SDK_DecControlStruct DecControl;

    uint8_t *pSilk = silk.data();
    uint8_t *pPcm  = pcm.data();
    /* Check Silk header */
    {
        char header_buf[50] = { 0 };
        memcpy(header_buf, pSilk, 10);
        header_buf[10] = '\0'; /* Terminate with a null character */
        if (strcmp(header_buf, "#!SILK_V3") == 0) {
            pSilk += 9; // 一般 SILK
        } else if (strcmp(header_buf + 1, "#!SILK_V3") == 0) {
            pSilk += 10; // 微信 SILK
        } else {
            printf("Error: Wrong Header %s\n", header_buf);
            exit(0);
        }
    }

    DecControl.API_sampleRate = sr;
    /* Initialize to one frame per packet, for proper concealment before first packet arrives */
    DecControl.framesPerPacket = 1;

    /* Create decoder */
    ret = SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes);
    if (ret) {
        printf("\nSKP_Silk_SDK_Get_Decoder_Size returned %d", ret);
    }
    psDec = malloc(decSizeBytes);

    /* Reset decoder */
    ret = SKP_Silk_SDK_InitDecoder(psDec);
    if (ret) {
        printf("\nSKP_Silk_InitDecoder returned %d", ret);
    }

    totPackets = 0;
    payloadEnd = payload;

    /* Simulate the jitter buffer holding MAX_FEC_DELAY packets */
    for (i = 0; i < MAX_LBRR_DELAY; i++) {
        memcpy(&nBytes, pSilk, sizeof(int16_t));
        pSilk += sizeof(int16_t);

        memcpy(payloadEnd, pSilk, sizeof(uint8_t) * nBytes);
        pSilk += sizeof(uint8_t) * nBytes;
        if ((size_t)(pSilk - silk.data()) > silk.size()) {
            break;
        }
        nBytesPerPacket[i] = nBytes;
        payloadEnd += nBytes;
        totPackets++;
    }

    while (1) {
        /* Read payload size */
        memcpy(&nBytes, pSilk, sizeof(int16_t));
        pSilk += sizeof(int16_t);

        if (nBytes < 0 || ((size_t)(pSilk - silk.data()) >= silk.size())) {
            break;
        }

        memcpy(payloadEnd, pSilk, sizeof(uint8_t) * nBytes);
        pSilk += sizeof(uint8_t) * nBytes;
        if ((size_t)(pSilk - silk.data()) > silk.size()) {
            break;
        }
        nBytesPerPacket[MAX_LBRR_DELAY] = nBytes;
        payloadEnd += nBytes;
        nBytes       = nBytesPerPacket[0];
        payloadToDec = payload;

        /* Silk decoder */
        outPtr    = out;
        tot_len   = 0;

        /* No Loss: Decode all frames in the packet */
        frames = 0;
        do {
            /* Decode 20 ms */
            ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len);
            if (ret) {
                printf("\nSKP_Silk_SDK_Decode returned %d\n", ret);
                return 0;
            }

            frames++;
            outPtr += len;
            tot_len += len;
            if (frames > MAX_INPUT_FRAMES) {
                /* Hack for corrupt stream that could generate too many frames */
                outPtr  = out;
                tot_len = 0;
                frames  = 0;
            }
            /* Until last 20 ms frame of packet has been decoded */
        } while (DecControl.moreInternalDecoderFrames);

        totPackets++;

        size_t oldSz = pcm.size();
        pcm.resize(oldSz + sizeof(int16_t) * tot_len);
        pPcm = pcm.data() + oldSz;
        memcpy(pPcm, out, sizeof(int16_t) * tot_len);
        pPcm += sizeof(int16_t) * tot_len;

        /* Update buffer */
        totBytes = 0;
        for (i = 0; i < MAX_LBRR_DELAY; i++) {
            totBytes += nBytesPerPacket[i + 1];
        }

        memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(uint8_t));
        payloadEnd -= nBytesPerPacket[0];
        memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(int16_t));
    }

    /* Empty the recieve buffer */
    for (k = 0; k < MAX_LBRR_DELAY; k++) {
        nBytes       = nBytesPerPacket[0];
        payloadToDec = payload;

        /* Silk decoder */
        outPtr    = out;
        tot_len   = 0;

        /* No loss: Decode all frames in the packet */
        frames = 0;
        do {
            /* Decode 20 ms */
            ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len);
            if (ret) {
                printf("\nSKP_Silk_SDK_Decode returned %d", ret);
            }

            frames++;
            outPtr += len;
            tot_len += len;
            if (frames > MAX_INPUT_FRAMES) {
                /* Hack for corrupt stream that could generate too many frames */
                outPtr  = out;
                tot_len = 0;
                frames  = 0;
            }
            /* Until last 20 ms frame of packet has been decoded */
        } while (DecControl.moreInternalDecoderFrames);

        totPackets++;

        pcm.resize(pcm.size() + sizeof(int16_t) * tot_len);
        memcpy(pPcm, out, sizeof(int16_t) * tot_len);
        pPcm += sizeof(int16_t) * tot_len;

        /* Update Buffer */
        totBytes = 0;
        for (i = 0; i < MAX_LBRR_DELAY; i++) {
            totBytes += nBytesPerPacket[i + 1];
        }
        memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(uint8_t));
        payloadEnd -= nBytesPerPacket[0];
        memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(int16_t));
    }

    /* Free decoder */
    free(psDec);

    return pcm.size();
}
