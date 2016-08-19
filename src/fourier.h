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

void window_rect(real* const, size_t windowSize);

/**
 * @brief Fills the window with Gaussian filter
 * @param[out] window An array with size windowSize
 * @param[in] windowSize
 * @param[in] var Higher var results in a more narrow window
 */
void window_gaussian(real* const window, size_t windowSize, real var);
/**
 * @brief Convolves the window with sample. Result will be stored in samples
 */
void convolve(real* samples, real const* window, size_t n);
		

#endif // !SPECTROGEN__FOURIER_H_
