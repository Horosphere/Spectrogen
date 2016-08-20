#ifndef SPECTROGEN__RECORD_H_
#define SPECTROGEN__RECORD_H_

#include <stdbool.h>

#include "fourier.h"
#include "display.h"

/**
 * Start recording audio and display the spectrogram in real time
 */
void record_exec(struct Display* const, struct DSTFT* const);

#endif // !SPECTROGEN__RECORD_H_
