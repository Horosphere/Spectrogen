
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <fftw3.h>

#include "display.h"

typedef double real;
typedef double complex comp;

int calculation_thread(struct Display* const d)
{
	// Populate samples
	size_t const nSamples = 44100;
	size_t const windowSamples = 2048;
	size_t const windowRadius = windowSamples / 2;
	real* samples = malloc(sizeof(real) * nSamples);
	for (size_t i = 0; i < 44100 / 4; ++i)
	{
		samples[i] = sin(2 * M_PI * i / 88.2); // 500 Hz
		samples[i + 44100 / 4] = sin(2 * M_PI * i / 22.05); // 2 kHz
		samples[i + 2 * 44100 / 4] = sin(2 * M_PI * i / 7.35); // 6 kHz
		samples[i + 3 * 44100 / 4] = sin(2 * M_PI * i / 3.675); // 12 kHz
	}
	real* window = malloc(sizeof(real) * windowSamples);
	// Populate rectangular window
	for (size_t i = 0; i < windowSamples; ++i)
	{
		window[i] = 1.0;
	}
	size_t bufferSize = sizeof(real) * windowSamples;
	real* buffer = fftw_malloc(bufferSize);
	comp* spectrum = fftw_malloc(sizeof(comp) * (windowRadius + 1));
	fftw_plan plan = fftw_plan_dft_r2c_1d(windowSamples, buffer, spectrum,
	                                      FFTW_MEASURE);

	fprintf(stdout, "FFTW plan measure complete\n");
	// Populate the image column by column
	uint8_t* image = malloc(3 * d->width * d->height * sizeof(uint8_t));

	clock_t timeStart = clock();

	memset(buffer, 0, bufferSize);
	for (size_t i = 0; i < nSamples; ++i)
	{
		if (i < windowRadius)
		{
			memcpy(buffer + windowRadius - i, samples,
					sizeof(real) * (windowRadius + i));
		}
		else if (i + windowRadius > nSamples)
		{
			size_t validLength = windowRadius + nSamples - i;
			buffer[nSamples - i + windowRadius - 1] = 0;
			memcpy(buffer, samples + i - windowRadius, sizeof(real) * (validLength));
		}
		else
		{
			memcpy(buffer, samples + i - windowRadius, bufferSize);
		}
		fftw_execute(plan);
		int column = i * d->width / nSamples;
		for (size_t j = 0; j < windowRadius + 1; ++j)
		{
			double amplitude = cabs(spectrum[j]) / windowRadius;
			uint8_t a = (uint8_t) 255 * amplitude;
			int row = d->height - j * d->height / (windowRadius + 1);
			int base = (column + row * d->width) * 3;
			image[base + 0] = a;
			image[base + 1] = a;
			image[base + 2] = a;
		}
	}

	clock_t timeDiff = (clock() - timeStart) * 1000 / CLOCKS_PER_SEC;
	fprintf(stdout, "Time elapsed: %ld ms\n", timeDiff);

	fftw_destroy_plan(plan);
	//fftw_free(spectrum);
	//fftw_free(buffer);
	free(samples);
	free(window);

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
	while (true)
	{
		if (!Display_pictQueue_write(d)) break;
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
	}

	free(image);
	return 0;
}

#define EVENT_REFRESH (SDL_USEREVENT + 2)

uint32_t refresh_timer(uint32_t interval, void* data)
{
	(void) interval;
	SDL_Event event;
	event.type = EVENT_REFRESH;
	event.user.data1 = data;
	SDL_PushEvent(&event);
	return 0;
}
void schedule_refresh(void* data, int delay)
{
	if (!SDL_AddTimer(delay, refresh_timer, data))
	{
		fprintf(stderr, "[SDL] %s\n", SDL_GetError());
	}
}
void refresh(struct Display* const d)
{
	if (d->pictQueueSize == 0)
		schedule_refresh(d, 1);
	else
	{
		schedule_refresh(d, 40);
		Display_pictQueue_draw(d);
	}
}

bool test()
{
	struct Display display;
	bool flag = false;
	Display_init(&display);
	display.width = 640;
	display.height = 480;
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

	SDL_Thread* threadCalculation
	  = SDL_CreateThread((SDL_ThreadFunction) calculation_thread,
	                     "calculation", &display);
	if (!threadCalculation)
	{
		fprintf(stderr, "[SDL] %s\n", SDL_GetError());
		goto finish;
	}

	schedule_refresh(&display, 40);
	while (true)
	{
		SDL_Event event;
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case SDL_QUIT:
			display.quit = true;
			flag = true;
			goto finish;
			break;
		case EVENT_REFRESH:
			refresh(event.user.data1);
			break;
		default:
			break;
		}
	}

finish:
	Display_pictQueue_destroy(&display);
	Display_destroy(&display);
	return flag;
};

int main(int argc, char* argv[])
{
	(void) argc;
	(void) argv;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "[SDL] %s\n", SDL_GetError());
		return -1;
	}
	if (!test())
	{
		fprintf(stderr, "Test routine failed\n");
		return -1;
	}
	SDL_Quit();
	return 0;
}
