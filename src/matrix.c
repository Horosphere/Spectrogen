#include "matrix.h"

#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

bool matrix_inverse(real* const* const m, size_t n)
{
	assert(m);
	assert(n != 0);
	size_t indexJ[n];
	size_t indexK[n];
	size_t pivot[n];
	memset(pivot, 0, n * sizeof(size_t));

	for (size_t i = 0; i < n; ++i)
	{
		size_t pivotJ = 0;
		size_t pivotK = 0;
		real max = 0.0;
		// Choose pivot
		for (size_t j = 0; j < n; ++j)
		{
			if (pivot[j] != 1)
			{
				for (size_t k = 0; k < n; ++k)
				{
					if (pivot[k] == 0)
					{
						real temp = fabs(m[j][k]);
						if (temp > max)
						{
							max = temp;
							pivotJ = j;
							pivotK = k;
						}
						else if (pivot[k] > 1) return false;
					}
				}
			}
		}
		++pivot[pivotK];
		if (pivotJ != pivotK)
		{
			// Swap rows m[pivotJ] and m[pivotK]
			for (size_t j = 0; j < n; ++j)
			{
				real temp = m[pivotJ][j];
				m[pivotJ][j] = m[pivotK][j];
				m[pivotK][j] = temp;
			}
		}
		indexJ[i] = pivotJ;
		indexK[i] = pivotK;
		if (m[pivotK][pivotK] == 0.0) return false;

		// Scale row m[pivotK]
		real pivotInv = 1.0 / m[pivotK][pivotK];
		m[pivotK][pivotK] = 1.0;
		for (size_t j = 0; j < n; ++j)
		{
			m[pivotK][j] *= pivotInv;
		}

		for (size_t j = 0; j < n; ++j)
		{
			if (j == pivotK) continue;
			real temp = m[j][pivotK];
			m[j][pivotK] = 0.0;
			for (size_t k = 0; k < n; ++k)
				m[j][k] -= m[pivotK][k] * temp;
		}
	}

	for (size_t j = 0; j < n; ++j)
	{
		if (indexJ[j] != indexK[j])
		{
			for (size_t k = 0; k < n; ++k)
			{
				// Swap columns m[][indexJ[j]] and m[][indexK[j]];
				for (size_t r = 0; r < n; ++r)
				{
					real temp = m[k][indexJ[j]];
					m[k][indexJ[j]] = m[k][indexK[j]];
					m[k][indexK[j]] = temp;
				}
			}
		}
	}

	return true;
}
void matrix_vector_multiply(real* const vector,
                            real const* const* const matrix,
                            size_t n)
{
	assert(vector && matrix);
	assert(n != 0);

	real* const temp = malloc(n * sizeof(real));
	for (size_t j = 0; j < n; ++j)
	{
		temp[j] = 0.0;
		for (size_t k = 0; k < n; ++k)
		{
			temp[j] += vector[k] * matrix[j][k];
		}
	}
	memcpy(vector, temp, n * sizeof(real));
	free(temp);
}
