#include "spectrogram.h"

#include <string.h>
#include <assert.h>
#include <math.h>

void spectrogram_populate(uint8_t* const image, int width, int height,
                          real const* const samples, size_t nSamples,
                          bool crop,
                          struct ColourGradient const* const grad,
                          struct DSTFT* const dstft)
{
	assert(nSamples >= dstft->windowWidth);

	memset(dstft->buffer, 0, sizeof(real) * dstft->windowWidth);

	size_t n = crop ? nSamples - dstft->windowWidth : nSamples;
	size_t offset = crop ? dstft->windowRadius : 0;
	for (int col = 0; col < width; ++col)
	{
		size_t i = col * n / (real) width + offset;
		if (crop || (i >= dstft->windowRadius &&
		             i + dstft->windowRadius <= nSamples))
		{
			memcpy(dstft->buffer, samples + i - dstft->windowRadius,
			       sizeof(real) * dstft->windowWidth);
		}
		else if (i < dstft->windowRadius)
		{
			memcpy(dstft->buffer + dstft->windowRadius - i, samples,
			       sizeof(real) * (dstft->windowRadius + i));
		}
		else if (i + dstft->windowRadius > nSamples)
		{
			memset(dstft->buffer + dstft->windowRadius + nSamples - i, 0,
			       sizeof(real) * (dstft->windowRadius + i - nSamples));
			memcpy(dstft->buffer, samples + i - dstft->windowRadius,
			       sizeof(real) * (dstft->windowRadius + nSamples - i));
		}
		convolve(dstft->buffer, dstft->window, dstft->windowWidth);
		fftw_execute(dstft->plan);

		for (int row = 0; row < height; ++row)
		{
			/*
			 * (height - row) flips the spectrogram upside down
			 * a linear map casts [0, height] to [0, windowRadius]. The +1 avoids
			 * the constant term and allows the highest component of frequency to be
			 * shown.
			 */
			size_t j = (height - row) * dstft->windowRadius / height + 1;
			/*
			 * Must multiply amplitude by 2 so maximum amplitude is 1
			 */
			double amplitude = log(cabs(dstft->spectrum[j]) * 2);

			int pixel = (col + row * width) * 3;
			ColourGradient_eval(grad, amplitude, image + pixel);
		}
	}
}
