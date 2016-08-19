#ifndef SPECTROGEN__FOURIER_H_
#define SPECTROGEN__FOURIER_H_

#include <complex.h>
#include <stddef.h>

#include <fftw3.h>

typedef double real;
typedef double complex comp;

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884197169399375105820974
#endif

// All window functions result in a window with energy 1

void window_rect(real* const, size_t n);
void window_tri(real* const, size_t n);
/**
 * @brief Fills the window with Gaussian filter
 * @param[out] window An array with size n
 * @param[in] n Size of the window
 * @param[in] var Higher var results in a more narrow window
 */
void window_gaussian(real* const window, size_t n, real var);

void window_exponential_causal(real* const window, size_t n, real var);
/**
 * @brief Convolves the window with sample. Result will be stored in samples
 */
void convolve(real* samples, real const* window, size_t n);

struct DSTFT
{
	size_t windowWidth;

	// Populated by DSTFT_init
	size_t windowRadius;
	real* window;
	real* buffer;
	comp* spectrum;
	fftw_plan plan;
};
		
/**
 * Must be called after windowWidth is initialised
 */
void DSTFT_init(struct DSTFT* const);
void DSTFT_destroy(struct DSTFT* const);

#endif // !SPECTROGEN__FOURIER_H_
