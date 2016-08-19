#ifndef SPECTROGEN__STATICSAMPLE_H_
#define SPECTROGEN__STATICSAMPLE_H_

#include <stdbool.h>

#include "fourier.h"
#include "display.h"

/**
 * File format:
 * First line: [NOS]: Number of sample
 * [NOS] lines follow. Each line containing a number
 *
 * @param[in] fileName A file containing samples in a specific format
 *	(see above). If not supplied, the routine uses an internally generated
 *	set of samples
 */
bool static_sample_exec(struct Display* const d,
		char const* fileName,
		struct DSTFT* const);

#endif // !SPECTROGEN__STATICSAMPLE_H_
