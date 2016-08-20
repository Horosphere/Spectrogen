#include "spectrogram.h"

#include <string.h>
#include <math.h>

void spectrogram_populate(uint8_t* const image, int width, int height,
                          real const* const samples, size_t nSamples,
                          struct DSTFT* dstft)
{
	for (int col = 0; col < width; ++col)
	{
		size_t i = col * nSamples / width;
		if (i < dstft->windowRadius)
		{
			memcpy(dstft->buffer + dstft->windowRadius - i, samples,
			       sizeof(real) * (dstft->windowRadius + i));
		}
		else if (i + dstft->windowRadius > nSamples)
		{
			dstft->buffer[nSamples - i + dstft->windowRadius - 1] = 0;
			memcpy(dstft->buffer, samples + i - dstft->windowRadius,
			       sizeof(real) * (dstft->windowRadius + nSamples - i));
		}
		else
		{
			memcpy(dstft->buffer, samples + i - dstft->windowRadius,
					sizeof(real) * dstft->windowWidth);
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
			double amplitude = cabs(dstft->spectrum[j]) * 2;

			uint8_t colour = (uint8_t) 255 * sqrt(amplitude);
			int pixel = (col + row * width) * 3;
			image[pixel + 0] = colour;
			image[pixel + 1] = colour;
			image[pixel + 2] = colour;
		}
	}
}
