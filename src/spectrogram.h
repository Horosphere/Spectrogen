#ifndef SPECTROGEN__SPECTROGRAM_H_
#define SPECTROGEN__SPECTROGRAM_H_

#include <stdint.h>
#include <stdbool.h>

#include "fourier.h"
#include "gradient.h"

#define SPECTROGRAM_LOGARITHMIC

/**
 * Define SPECTROGRAM_LOGARITHMIC to draw logarithmic graph
 * @brief Converts the samples to a spectrogram
 * @param[out] image An array of size 3 * width * height in the RGB888, width
 *	major format for storing the pixels.
 * @param[in] width Width of the image
 * @param[in] height Height of the image
 * @param[in] samples An array of reals representing the samples
 * @param[in] nSamples The number of samples.
 * @param[in] crop If set to true, the first and last windowRadius samples will
 *	not be shown. This is useful if the samples are being streamed.
 * @param[in] grad A colour gradient for shading the spectrogram
 * @param dstft A struct DSTFT for the window and the buffer
 */
void spectrogram_populate(uint8_t* const image, int width, int height,
                          real const* const samples, size_t nSamples,
                          bool crop,
                          struct ColourGradient const* const grad,
                          struct DSTFT* const dstft);

#endif // !SPECTROGEN__SPECTROGRAM_H_
