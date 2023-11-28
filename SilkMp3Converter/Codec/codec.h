#pragma once

#include <stdint.h>
#include <string>
#include <vector>

typedef struct {
    double filetime;
    int32_t totPackets;
} DecTime_t;

#define DllExport __declspec(dllexport)

DecTime_t Mp3Encode(std::vector<uint8_t> &pcm, std::vector<uint8_t> &mp3, int32_t sr);
DecTime_t SilkDecode(std::vector<uint8_t> &silk, std::vector<uint8_t> &pcm, int32_t sr);

DllExport int Silk2Mp3(std::string inpath, std::string outpath, int sr);
DllExport int Silk2Mp3(std::vector<uint8_t> &silk, std::vector<uint8_t> &mp3, int sr);
