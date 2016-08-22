#ifndef SPECTROGEN__MATRIX_H_
#define SPECTROGEN__MATRIX_H_

#include <stdbool.h>
#include <stddef.h>

#include "spectrogen.h"

/**
 * FIXME: This function has problem determining singular matrices.
 * @brief Calculate the inverse matrix
 * @param coeff A row-major square matrix with size n
 * @return false if the matrix is singular
 */
bool matrix_inverse(real* const* const, size_t n);
void matrix_vector_multiply(real* const vector,
                            real const* const* const matrix,
                            size_t n);

#endif // !SPECTROGEN__MATRIX_H_
