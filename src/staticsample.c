#include "staticsample.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <fftw3.h>

#include "display.h"
#include "spectrogram.h"


bool static_sample_exec(struct Display* const d,
                        struct DSTFT* const dstft,
                        char const* const fileName,
                        size_t nSamplesIn)
{
	real* samples = NULL;
	size_t nSamples = 0;

	// Populate samples

	if (fileName)
	{
		printf("Reading file: %s\n", fileName);
		FILE* file = fopen(fileName, "r");
		if (!file)
		{
			fprintf(stderr, "Unable to open file\n");
			return false;
		}
		char line[32];
		if (!fgets(line, sizeof(line), file))
		{
			fprintf(stderr, "File is empty\n");
			fclose(file);
			return false;
		}
		nSamples = atol(line);
		if (nSamples < dstft->windowWidth)
		{
			fprintf(stderr, "Number of samples cannot be less than the window width\n");
			fclose(file);
			return false;
		}
		samples = malloc(sizeof(real) * nSamples);

		for (size_t i = 0; i < nSamples; ++i)
		{
			if (!fgets(line, sizeof(line), file))
			{
				fprintf(stderr, "Sample numbers do not match\n");
				free(samples);
				fclose(file);
				return false;
			}
			samples[i] = atof(line);
		}

		fclose(file);
	}
	else
	{
		srand(clock());
		nSamples = nSamplesIn;
		samples = malloc(sizeof(real) * nSamples);

		size_t segmentWidth = dstft->windowWidth * 3;
		fprintf(stdout, "Segment width: %ld\n", segmentWidth);
		size_t nSegments = nSamples / segmentWidth;
		for (size_t i = 0; i < nSegments; ++i)
		{
			real freq0 = rand() / (real) RAND_MAX;
			real freq1 = rand() / (real) RAND_MAX;
			for (size_t j = i * segmentWidth; j < (i + 1) * segmentWidth; ++j)
			{
				samples[j] = (cos(freq0 * j) + sin(freq1 * j));
			}
		}
	}

	// Calculate spectrogram

	uint8_t* image = malloc(3 * d->width * d->height * sizeof(uint8_t));

	clock_t timeStart = clock();
	spectrogram_populate(image, d->width, d->height,
	                     samples, nSamples, false, &d->colourGradient, dstft);
	for (int i = 0; i < d->width; ++i)
	{
			double amp =  12 * (i / (real) d->width - 1.0);
			//printf("%f\n", amp);
		for (int j = d->height / 10; j < d->height / 7; ++j)
		{
			uint8_t* px = image + (i + j * d->width) * 3;
			ColourGradient_eval(&d->colourGradient, amp, px);
		}
	}

	clock_t timeDiff = (clock() - timeStart) * 1000 / CLOCKS_PER_SEC;
	fprintf(stdout, "Time elapsed: %ld ms\n", timeDiff);

	// Convert image to YUV
	uint8_t const* dataIn[3];
	int linesizeIn[3];

	uint8_t* dataOut[3];
	int linesizeOut[3];
	{
		dataIn[0] = dataIn[1] = dataIn[2] = image;
		linesizeIn[0] = linesizeIn[1] = linesizeIn[2] = d->width * 3;
		size_t pitchUV = d->width / 2;
		linesizeOut[0] = d->width;
		linesizeOut[1] = pitchUV;
		linesizeOut[2] = pitchUV;
	}

	{
		struct Picture* p = &d->pictQueue[d->pictQueueIW];
		dataOut[0] = p->planeY;
		dataOut[1] = p->planeU;
		dataOut[2] = p->planeV;

		sws_scale(d->swsContext,
		          dataIn, linesizeIn, 0, d->height,
		          dataOut, linesizeOut);

		++d->pictQueueIW;
		if (d->pictQueueIW == DISPLAY_PICTQUEUE_SIZE_MAX)
			d->pictQueueIW = 0;
		SDL_LockMutex(d->pictQueueMutex);
		++d->pictQueueSize;
		SDL_UnlockMutex(d->pictQueueMutex);

		Display_pictQueue_draw(d);
	}

	free(image);
	free(samples);

	while (true)
	{
		SDL_Event event;
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case SDL_QUIT:
			d->quit = true;
			goto finish;
			break;
		default:
			break;
		}
	}
finish:
	return true;
}
