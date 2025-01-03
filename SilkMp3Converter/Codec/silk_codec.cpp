#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "SKP_Silk_SDK_API.h"
#include "SKP_Silk_SigProc_FIX.h"

#include "codec.h"


#define FRAME_LENGTH_MS     20
#define MAX_API_FS_KHZ      48


using namespace std;

size_t SilkDecode(vector<uint8_t> &silk, vector<uint8_t> &pcm, int32_t sr)
{
    if (silk.size() < 10) {
        throw runtime_error("Invalid SILK data size.");
    }

    // Check Silk header
    const char *silk_header = reinterpret_cast<const char *>(silk.data());
    if (strncmp(silk_header, "#!SILK_V3", 9) == 0) {
        // General SILK header
        silk.erase(silk.begin(), silk.begin() + 9);
    } else if (strncmp(silk_header + 1, "#!SILK_V3", 9) == 0) {
        // WeChat SILK header
        silk.erase(silk.begin(), silk.begin() + 10);
    } else {
        throw runtime_error("Invalid SILK header.");
    }

    // Initialize decoder
    int decSizeBytes = 0;
    if (SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes)) {
        throw runtime_error("Failed to get SILK decoder size.");
    }

    unique_ptr<uint8_t[]> decoderMemory(new uint8_t[decSizeBytes]);
    void *psDec = decoderMemory.get();

    if (SKP_Silk_SDK_InitDecoder(psDec)) {
        throw runtime_error("Failed to initialize SILK decoder.");
    }

    SKP_SILK_SDK_DecControlStruct decControl = {};
    decControl.API_sampleRate                = sr;

    size_t total_pcm_size       = 0;
    const size_t max_frame_size = (FRAME_LENGTH_MS * MAX_API_FS_KHZ) << 1;
    vector<int16_t> output_buffer(max_frame_size);

    size_t offset = 0;
    while (offset < silk.size()) {
        if (offset + sizeof(int16_t) > silk.size()) {
            break;
        }

        int16_t payload_size = 0;
        memcpy(&payload_size, silk.data() + offset, sizeof(int16_t));
        offset += sizeof(int16_t);

        if (payload_size <= 0 || offset + payload_size > silk.size()) {
            break;
        }

        const uint8_t *payload = silk.data() + offset;
        offset += payload_size;

        int16_t decoded_samples = 0;
        int ret
            = SKP_Silk_SDK_Decode(psDec, &decControl, 0, payload, payload_size, output_buffer.data(), &decoded_samples);

        if (ret != 0) {
            throw runtime_error("Error decoding SILK payload.");
        }

        size_t pcm_chunk_size = decoded_samples * sizeof(int16_t);
        pcm.insert(pcm.end(), reinterpret_cast<uint8_t *>(output_buffer.data()),
                   reinterpret_cast<uint8_t *>(output_buffer.data()) + pcm_chunk_size);
        total_pcm_size += pcm_chunk_size;
    }

    return total_pcm_size;
}
