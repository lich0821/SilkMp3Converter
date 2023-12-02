#include <vector>

#include <lame/lame.h>

#include "codec.h"

#define PCM_SIZE   8192
#define MP3_SIZE   8192
#define BATCH_SIZE (2 * sizeof(uint16_t) * PCM_SIZE)

int Mp3Encode(std::vector<uint8_t> &pcm, std::vector<uint8_t> &mp3, int32_t sr)
{
    int write, leftsize;
    size_t rsize, oldsz;

    uint8_t *pPcm = pcm.data();
    uint8_t *pMp3 = mp3.data();

    static uint8_t mp3_buffer[MP3_SIZE];
    static int16_t pcm_buffer[PCM_SIZE * 2];

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
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, (int)(rsize / (2 * sizeof(uint16_t))), mp3_buffer,
                                                   MP3_SIZE);
        }

        oldsz = mp3.size();
        mp3.resize(oldsz + write);
        pMp3 = mp3.data() + oldsz;
        memcpy(pMp3, mp3_buffer, write);
    } while (leftsize != 0);

    lame_close(lame);

    return 0;
}

int Mp3Encode(std::vector<uint8_t> &pcm, std::string &mp3path, int32_t sr)
{
    int rv = { 0 };
    size_t rsize;
    int write, leftsize;

    uint8_t *pPcm = pcm.data();
    FILE *fMp3    = fopen(mp3path.c_str(), "wb");
    if (fMp3 == NULL) {
        printf("Error: could not open input file %s\n", mp3path.c_str());
        return -1;
    }

    static uint8_t mp3_buffer[MP3_SIZE];
    static int16_t pcm_buffer[PCM_SIZE * 2];

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
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, (int)(rsize / (2 * sizeof(uint16_t))), mp3_buffer,
                                                   MP3_SIZE);
        }

        fwrite(mp3_buffer, write, 1, fMp3);
    } while (leftsize != 0);

    lame_close(lame);
    fclose(fMp3);

    return 0;
}
