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


bool static_sample_exec(struct Display* const d,
                        char const* fileName,
                        struct DSTFT* const dstft)
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
		nSamples = 44100;
		samples = malloc(sizeof(real) * nSamples);
		for (size_t i = 0; i < 44100 / 4; ++i)
		{
			samples[i] = sin(2 * M_PI * i / 88.2); // 500 Hz
			samples[i + 44100 / 4] = sin(2 * M_PI * i / 22.05); // 2 kHz
			samples[i + 2 * 44100 / 4] = sin(2 * M_PI * i / 7.35); // 6 kHz
			samples[i + 3 * 44100 / 4] = sin(2 * M_PI * i / 2.01); //  22 kHz
		}
	}

	// Calculate spectrogram

	uint8_t* image = malloc(3 * d->width * d->height * sizeof(uint8_t));

	clock_t timeStart = clock();
	for (int col = 0; col < d->width; ++col)
	{
		size_t i = col * nSamples / d->width;
		if (i < dstft->windowRadius)
		{
			memcpy(dstft->buffer + dstft->windowRadius - i, samples,
			       sizeof(real) * (dstft->windowRadius + i));
		}
		else if (i + dstft->windowRadius > nSamples)
		{
			dstft->buffer[nSamples - i + dstft->windowRadius - 1] = 0;
			memcpy(dstft->buffer, samples + i - dstft->windowRadius,
			       sizeof(real) * (dstft->windowRadius + nSamples - i));
		}
		else
		{
			memcpy(dstft->buffer, samples + i - dstft->windowRadius,
					sizeof(real) * dstft->windowWidth);
		}
		convolve(dstft->buffer, dstft->window, dstft->windowWidth);
		fftw_execute(dstft->plan);

		for (int row = 0; row < d->height; ++row)
		{
			/*
			 * (d->height - row) flips the spectrogram upside down
			 * a linear map casts [0, d->height] to [0, windowRadius]. The +1 avoids
			 * the constant term and allows the highest component of frequency to be
			 * shown.
			 */
			size_t j = (d->height - row) * dstft->windowRadius / d->height + 1;
			/*
			 * Must multiply amplitude by 2 so maximum amplitude is 1
			 */
			double amplitude = cabs(dstft->spectrum[j]) * 2;

			uint8_t colour = (uint8_t) 255 * amplitude;
			int pixel = (col + row * d->width) * 3;
			image[pixel + 0] = colour;
			image[pixel + 1] = colour;
			image[pixel + 2] = colour;
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
