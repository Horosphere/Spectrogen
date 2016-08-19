#include "fourier.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

void window_rect(real* const window, size_t n)
{
	window[0] = 1.0 / n;
	for (size_t i = 1; i < n; ++i)
	{
		window[i] = window[0];
	}
}
void window_tri(real* const window, size_t n)
{
	size_t radius = n / 2;
	real mult = 2 / ((real) radius * radius);
	for (size_t i = 0; i < radius; ++i)
	{
		window[n - i] = window[i] = mult * i;
	}
}
void window_gaussian(real* const window, size_t n, real var)
{
	real multiplier = sqrt(var / M_PI) / n;
	real a = var / ((real)n * n);
	int centre = n / 2;
	int iMax = (n + 1) / 2;
	for (int i = -centre; i < iMax; ++i)
	{
		window[i + centre] = multiplier * exp(-a * i * i);
	}
}
void window_exponential_causal(real* const window, size_t n, real var)
{
	size_t radius = n / 2;
	real a = var / n;
	for (size_t i = 0; i < radius; ++i)
	{
		window[i + radius] = a * exp(-(i * a));
	}
	memset(window, 0, radius);
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

void DSTFT_init(struct DSTFT* const d)
{
	assert(d);
	assert(d->windowWidth);
	d->windowRadius = d->windowWidth / 2;
	d->window = malloc(sizeof(real) * d->windowWidth);
	d->buffer = fftw_malloc(sizeof(real) * d->windowWidth);
	d->spectrum = fftw_malloc(sizeof(comp) * (d->windowRadius + 1));
	d->plan = fftw_plan_dft_r2c_1d(d->windowWidth, d->buffer, d->spectrum,
	                               FFTW_MEASURE);
}
void DSTFT_destroy(struct DSTFT* const d)
{
	if (!d) return;
	free(d->window);
	fftw_free(d->buffer);
	fftw_free(d->spectrum);
	fftw_destroy_plan(d->plan);
}
