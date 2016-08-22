/*
 * Main entry for Spectrogen
 */
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "display.h"
#include "fourier.h"
#include "staticsample.h"
#include "record.h"

#include "gradient.h"

void preset_gradient(struct ColourGradient* const grad)
{
	assert(grad);
	size_t nPoints = 9;
	ColourGradient_init(grad, nPoints);
	grad->r.interpolation = grad->g.interpolation = grad->b.interpolation
		= INTERP_SPLINE3;

	real* const r = grad->r.y;
	real* const g = grad->g.y;
	real* const b = grad->b.y;

	real* const x = grad->r.x;

#define COLOUR_SET(index, t, red, green, blue) \
	x[index] = t; \
	r[index] = red; g[index] = green; b[index] = blue;

	COLOUR_SET(0, -12, 0.0, 0.0, 0.0);
	COLOUR_SET(1, -7.2, 31, 0, 0);
	COLOUR_SET(2, -6.5, 61, 13, 2);
	COLOUR_SET(3, -5.5, 204, 34, 22);
	COLOUR_SET(4, -4, 238, 191, 40);
	COLOUR_SET(5, -3, 32, 194, 111);
	COLOUR_SET(6, -2, 37, 105, 245);
	COLOUR_SET(7, -1, 200, 170, 255);
	COLOUR_SET(8, 0, 255, 255, 255);

	for (size_t i = 0; i < nPoints; ++i)
	{
		real fac = 2.0 / (3.0 - i / (real) nPoints);
		r[i] *= fac;
		g[i] *= fac;
		b[i] *= fac;
	}
	memcpy(grad->g.x, x, sizeof(real) * nPoints);
	memcpy(grad->b.x, x, sizeof(real) * nPoints);
	ColourGradient_populate(grad);
}
int main(int argc, char* argv[])
{
	(void) argc;
	(void) argv;

	// Initialisation

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "[SDL] %s\n", SDL_GetError());
		return -1;
	}

	// Default values
	struct Display display;
	Display_init(&display);
	display.width = 640;
	display.height = 480;

	enum
	{
		ROUTINE_STATIC,
		ROUTINE_RECORD
	} routineType = ROUTINE_RECORD;
	enum
	{
		WINDOW_RECT,
		WINDOW_TRI,
		WINDOW_GAUSSIAN,
		WINDOW_EXPCAUSAL
	} windowType = WINDOW_GAUSSIAN;
	real windowVar = 6.0;

	struct DSTFT dstft;
	memset(&dstft, 0, sizeof(struct DSTFT));
	dstft.windowWidth = 1536;
	char const* file = NULL;
	size_t nSamples = 88200;

	// Command line parser
	char** arg= argv;
	char** argEnd= arg + argc;

	// Help
	if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		printf("Usage:\n"
		       "Spectrogram specifications:\n"
		       "--dim WIDTH HEIGHT: Dimensions of the output window\n"
		       "--window TYPE WIDTH VAR: Specs of the window function\n"
		       "    TYPE: Can have the value 'rect', 'tri', 'gauss', 'expc'\n"
		       "    WIDTH: Number of samples for the window.\n"
		       "    VAR: Higher var indicates a narrower window. Ignored for rect"
		       " and tri types\n"
		       "--ns NSAMPLES: The number of samples for various routines\n"
		       "Modes:\n"
		       "(NO FLAG): Accept input from the microphone\n"
		       "--file FILENAME: Read samples from a file. The first line must be"
		       " the number of samples, with sample values following on separate"
		       " lines\n"
		       "--default: Use a set of default generated samples\n"
		      );
		return 1;
	}
	while (++arg != argEnd)
	{
		if (strcmp(*arg, "--dim") == 0)
		{
			if (++arg == argEnd || *arg[0] == '-')
			{
				fprintf(stderr, "Two numbers must be supplied after --dim\n");
				return -1;
			}
			display.width = atoi(*arg);
			if (++arg == argEnd || *arg[0] == '-')
			{
				fprintf(stderr, "Two numbers must be supplied after --dim\n");
				return -1;
			}
			display.height = atoi(*arg);
		}
		else if (strcmp(*arg, "--window") == 0)
		{
			if (++arg == argEnd || *arg[0] == '-')
			{
				fprintf(stderr, "Window type must be supplied after --window\n");
				return -1;
			}
			if (strcmp(*arg, "rect") == 0)
				windowType = WINDOW_RECT;
			else if (strcmp(*arg, "tri") == 0)
				windowType = WINDOW_TRI;
			else if (strcmp(*arg, "gauss") == 0)
				windowType = WINDOW_GAUSSIAN;
			else if (strcmp(*arg, "expc") == 0)
				windowType = WINDOW_EXPCAUSAL;
			else
			{
				fprintf(stderr, "Unrecognised window type\n");
				return -1;
			}
			if (++arg == argEnd || *arg[0] == '-') break;
			dstft.windowWidth = atol(*arg);
			if (dstft.windowWidth == 0)
			{
				fprintf(stderr, "Invalid window width");
				return -1;
			}
			if (++arg == argEnd || *arg[0] == '-') break;
			windowVar = atof(*arg);
		}
		else if (strcmp(*arg, "--file") == 0)
		{
			if (++arg == argEnd)
			{
				fprintf(stderr, "A file name must be provided after --file\n");
				return -1;
			}
			file = *arg;
			routineType = ROUTINE_STATIC;
		}
		else if (strcmp(*arg, "--ns") == 0)
		{
			if (++arg == argEnd || *arg[0] == '-')
			{
				fprintf(stderr, "A sample number must be provided");
				return -1;
			}
			nSamples = atol(*arg);
		}
		else if (strcmp(*arg, "--default") == 0)
		{
			file = NULL;
			routineType = ROUTINE_STATIC;
		}
		else
		{
			fprintf(stderr, "Unknown argument\n");
			return -1;
		}
	}
	if (nSamples < dstft.windowWidth)
	{
		fprintf(stderr, "Sample size cannot be smaller than the window width\n");
		return -1;
	}

	// Parsing complete. Populate fields
	display.window =
	  SDL_CreateWindow("Spectrogen",
	                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                   display.width, display.height, 0);
	display.swsContext = sws_getContext(display.width, display.height,
	                                    AV_PIX_FMT_RGB24,
	                                    display.width, display.height,
	                                    DISPLAY_PICTQUEUE_PIXFMT, SWS_BILINEAR,
	                                    NULL, NULL, NULL);

	Display_pictQueue_init(&display);
	preset_gradient(&display.colourGradient);

	DSTFT_init(&dstft);
	switch (windowType)
	{
	case WINDOW_RECT:
		window_rect(dstft.window, dstft.windowWidth);
		break;
	case WINDOW_TRI:
		window_tri(dstft.window, dstft.windowWidth);
		break;
	case WINDOW_GAUSSIAN:
		window_gaussian(dstft.window, dstft.windowWidth, windowVar);
		break;
	case WINDOW_EXPCAUSAL:
		window_exponential_causal(dstft.window, dstft.windowWidth, windowVar);
		break;
	}

	switch (routineType)
	{
	case ROUTINE_STATIC:
		static_sample_exec(&display, &dstft, file, nSamples);
		break;
	case ROUTINE_RECORD:
		record_exec(&display, &dstft, nSamples);
		break;
	}

	// Clean up
	DSTFT_destroy(&dstft);
	Display_pictQueue_destroy(&display);
	Display_destroy(&display);
	SDL_Quit();
	return 0;
}
