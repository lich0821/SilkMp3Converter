#include <vector>

#include <lame/lame.h>

#include "codec.h"

#define BATCH_SIZE (2 * sizeof(short int) * PCM_SIZE)

DecTime_t Mp3Encode(std::vector<uint8_t> &pcm, std::vector<uint8_t> &mp3, int32_t sr)
{
    DecTime_t rv = { 0 };
    int write, leftsize;

    uint8_t *pPcm = pcm.data();
    uint8_t *pMp3 = mp3.data();

    const int PCM_SIZE = 8192;
    const int MP3_SIZE = 8192;
    size_t rsize, oldsz;

    short int pcm_buffer[PCM_SIZE * 2];
    unsigned char mp3_buffer[MP3_SIZE];

    lame_t lame = lame_init();
    lame_set_in_samplerate(lame, sr);
    lame_set_VBR(lame, vbr_default);
    lame_set_VBR_q(lame, 5);
    lame_set_mode(lame, MONO);
    lame_init_params(lame);

    do {
        leftsize = (int32_t)(pcm.size() - (pPcm - pcm.data()));
        if (leftsize < 0) {
            leftsize = 0;
        } else {
            rsize = leftsize > BATCH_SIZE ? BATCH_SIZE : leftsize;
            memcpy(pcm_buffer, pPcm, rsize);
            pPcm += rsize;
        }

        if (leftsize == 0) {
            write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
        } else {
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, rsize / (2 * sizeof(short int)), mp3_buffer,
                                                   MP3_SIZE);
        }

        oldsz = mp3.size();
        mp3.resize(oldsz + write);
        pMp3 = mp3.data() + oldsz;
        memcpy(pMp3, mp3_buffer, write);
    } while (leftsize != 0);

    lame_close(lame);

    return rv;
}