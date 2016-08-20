#ifndef SPECTROGEN__SPECTROGRAM_H_
#define SPECTROGEN__SPECTROGRAM_H_

#include <stdint.h>

#include "fourier.h"

void spectrogram_populate(uint8_t* const image, int width, int height,
                          real const* const samples, size_t nSamples,
                          struct DSTFT*);

#endif // !SPECTROGEN__SPECTROGRAM_H_