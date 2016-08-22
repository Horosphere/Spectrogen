#include "gradient.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"

void Gradient_init(struct Gradient* const g, size_t nPoints)
{
	assert(g);
	g->x = malloc(sizeof(real) * nPoints);
	g->y = malloc(sizeof(real) * nPoints);
	g->nPoints = nPoints;
	g->derivatives = NULL;
	g->interpolation = INTERP_NEAREST;
	g->terminal = INTERP_NEAREST;
}
void Gradient_destroy(struct Gradient* const g)
{
	if (!g) return;
	free(g->x);
	free(g->y);
	free(g->derivatives);
}
void Gradient_populate(struct Gradient* const g)
{
	if (g->interpolation != INTERP_SPLINE3) return;

	assert(g->nPoints >= 3);
	size_t const n = g->nPoints;

	free(g->derivatives);
	g->derivatives = malloc(sizeof(real) * n);
	memset(g->derivatives, 0, sizeof(real) * n);
	real* const d = g->derivatives;

	/*
	 * Tridiagonal matrix for solving derivatives
	 */
	real** mat = malloc(sizeof(real*) * n);
	for (size_t i = 0; i < n; ++i)
	{
		mat[i] = malloc(sizeof(real) * n);
		memset(mat[i], 0, sizeof(real) * n);
	}
	// Populate matrix
	{
		// Row 0
		real div = 1 / (g->x[1] - g->x[0]);
		mat[0][0] = 2 * div;
		mat[0][1] = div;
		d[0] = 3 * (g->y[1] - g->y[0]) * (div * div);
	}
	for (size_t i = 1; i < n - 1; ++i)
	{
		// Row 1 to n - 1

		real divL = 1 / (g->x[i] - g->x[i - 1]);
		real divR = 1 / (g->x[i + 1] - g->x[i]);
		mat[i][i - 1] = divL;
		mat[i][i] = 2 * (divL + divR);
		mat[i][i + 1] = divR;
		d[i] = 3 * ((g->y[i] - g->y[i - 1]) * (divL * divL) +
		            ((g->y[i + 1] - g->y[i]) * (divR * divR)));
	}
	{
		real div = 1 / (g->x[n - 1] - g->x[n - 2]);
		mat[n - 1][n - 2] = div;
		mat[n - 1][n - 1] = 2 * div;
		d[n - 1] = 3 * (g->y[n - 1] - g->y[n - 2]) * (div * div);
	}

	bool invertible = matrix_inverse(mat, n);
	(void) invertible; assert(invertible);

	matrix_vector_multiply(d, (real const* const* const) mat, n);
	for (size_t i = 0; i < n; ++i)
		free(mat[i]);
	free(mat);
}
real Gradient_eval(struct Gradient const* const g, real x)
{
	assert(g);
	size_t i = 0;
	size_t const n = g->nPoints;
	assert(n != 0);
	while (g->x[i] < x && i < n) ++i;
	if (x < g->x[0] || i == n) // Terminal behaviour
	{
		if (g->terminal == INTERP_NEAREST)
		{
			return i == 0 ? g->y[0] : g->y[n - 1];
		}
		else // lerp
		{
			if (n == 1)
				return g->y[0];
			else
			{
				if (x < g->x[0])
					return g->y[0] + (g->y[1] - g->y[0]) * (x - g->x[0]) /
					       (g->x[1] - g->x[0]);
				else
					return g->y[n - 2] + (g->y[n - 1] - g->y[n - 2]) *
					       (x - g->x[n - 2]) / (g->x[n - 1] - g->x[n - 2]);
			}
		}
	}

	if (g->interpolation == INTERP_NEAREST)
	{
		real d0 = x - g->x[i - 1];
		real d1 = g->x[i] - x;
		return d0 > d1 ? g->y[i - 1] : g->y[i];
	}
	else if (g->interpolation == INTERP_LINEAR)
	{
		return g->y[i - 1] + (g->y[i] - g->y[i - 1]) * (x - g->x[i - 1]) /
		       (g->x[i] - g->x[i - 1]);
	}
	else if (g->interpolation == INTERP_SPLINE3)
	{
		assert(g->derivatives);
		real* d = g->derivatives;
		real t = (x - g->x[i - 1]) / (g->x[i] - g->x[i - 1]);
		real a = d[i - 1] * (g->x[i] - g->x[i - 1]) - (g->y[i] - g->y[i - 1]);
		real b = -d[i] * (g->x[i] - g->x[i - 1]) + (g->y[i] - g->y[i - 1]);
		return (1 - t) * g->y[i - 1] + t * g->y[i] +
			t * (1 - t) * (a * (1 - t) + b * t);
	}
	assert(false && "Unrecognised interpolation type");
	return 0.0;
}

void ColourGradient_init(struct ColourGradient* const g, size_t nPoints)
{
	assert(g);
	Gradient_init(&g->r, nPoints);
	Gradient_init(&g->g, nPoints);
	Gradient_init(&g->b, nPoints);
	g->r.terminal = g->g.terminal = g->b.terminal = INTERP_NEAREST;
}
void ColourGradient_destroy(struct ColourGradient* const g)
{
	if (!g) return;
	Gradient_destroy(&g->r);
	Gradient_destroy(&g->g);
	Gradient_destroy(&g->b);
}
void ColourGradient_populate(struct ColourGradient* const g)
{
	assert(g);
	Gradient_populate(&g->r);
	Gradient_populate(&g->g);
	Gradient_populate(&g->b);
}
uint8_t real_to_colour(real val)
{
	if (val < 0.0) return 0;
	else if (val > 255.0) return 255;
	else return (uint8_t) val;
}
void ColourGradient_eval(struct ColourGradient const* const g,
                         real x, uint8_t colour[3])
{
	assert(g);
	assert(colour);
	colour[0] = real_to_colour(Gradient_eval(&g->r, x));
	colour[1] = real_to_colour(Gradient_eval(&g->g, x));
	colour[2] = real_to_colour(Gradient_eval(&g->b, x));
}
