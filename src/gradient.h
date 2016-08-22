#ifndef SPECTROGEN__GRADIENT_H_
#define SPECTROGEN__GRADIENT_H_

#include <stddef.h>
#include <stdint.h>

#include "spectrogen.h"

enum Interpolation
{
	INTERP_NEAREST,
	INTERP_LINEAR,
	INTERP_SPLINE3 // Cubic spline
};

struct Gradient
{
	/**
	 * Requirements:
	 * Must be populated in sorted order
	 */
	real* x;
	real* y;
	size_t nPoints;
	enum Interpolation interpolation;
	/*
	 * This value currently cannot be spline3
	 */
	enum Interpolation terminal;

	real* derivatives;
};


void Gradient_init(struct Gradient* const, size_t nPoints);
void Gradient_destroy(struct Gradient* const);
void Gradient_populate(struct Gradient* const);
real Gradient_eval(struct Gradient const* const, real x);

struct ColourGradient
{
	struct Gradient r;
	struct Gradient g;
	struct Gradient b;
};
void ColourGradient_init(struct ColourGradient* const, size_t nPoints);
void ColourGradient_destroy(struct ColourGradient* const);
void ColourGradient_populate(struct ColourGradient* const);
void ColourGradient_eval(struct ColourGradient const* const,
                         real x, uint8_t colour[3]);

#endif // !SPECTROGEN__GRADIENT_H_
