#include <stdio.h>
#include <stdlib.h>

#include "codec.h"

static void print_usage(char *argv[])
{
    printf("\nusage: %s in.silk out.mp3 [settings]\n", argv[0]);
    printf("\nin.silk       : Bitstream input to decoder");
    printf("\nout.mp3      : Speech output from decoder");
    printf("\n[Hz]         : Sampling rate of output signal in Hz; default: 24000");
    printf("\n");
}

int main(int argc, char *argv[])
{
    int32_t args;
    int32_t packetSize_ms = 0, sampleRate = 0;
    char speechOutFileName[150], bitInFileName[150];
    if (argc < 3) {
        print_usage(argv);
        exit(0);
    }

    args = 1;
    strcpy(bitInFileName, argv[args]);
    args++;
    strcpy(speechOutFileName, argv[args]);
    args++;
    if (args < argc) {
        sscanf(argv[args], "%d", &sampleRate);
    }

    if (sampleRate == 0) {
        sampleRate = 24000;
    }

    printf("Input:                       %s\n", bitInFileName);
    printf("Output:                      %s\n", speechOutFileName);
    printf("Sample Rate:                 %d\n", sampleRate);

    return Silk2Mp3(bitInFileName, speechOutFileName, sampleRate);
}
