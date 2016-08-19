#include "fourier.h"

#include <math.h>

void window_rect(real* const window, size_t windowSize)
{
	window[0] = 1.0 / windowSize;
	for (size_t i = 1; i < windowSize; ++i)
	{
		window[i] = window[0];
	}
}
void window_gaussian(real* const window, size_t windowSize, real var)
{
	real multiplier = sqrt(var / M_PI) / windowSize;
	real a = var / (windowSize * (real) windowSize);
	int centre = windowSize / 2;
	int iMax = (windowSize + 1) / 2;
	for (int i = -centre; i < iMax; ++i)
	{
		window[i + centre] = multiplier * exp(-a * i * i);
	}
}
void convolve(real* samples, real const* window, size_t n)
{
	window += n;
	for (size_t i = 0; i < n; ++i)
	{
		--window;
		*samples *= *window;
		++samples;
	}
}
