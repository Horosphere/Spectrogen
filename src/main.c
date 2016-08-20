
#include <stdio.h>
#include <stdbool.h>

#include "display.h"
#include "fourier.h"
#include "staticsample.h"
#include "record.h"


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
		static_sample_exec(&display, file, &dstft);
		break;
	case ROUTINE_RECORD:
		record_exec(&display, &dstft);
		break;
	}

	// Clean up
	DSTFT_destroy(&dstft);
	Display_pictQueue_destroy(&display);
	Display_destroy(&display);
	SDL_Quit();
	return 0;
}
