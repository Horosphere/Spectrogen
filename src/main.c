
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>

#include <SDL2/SDL.h>
#include <fftw3.h>

#include "display.h"

typedef double real;
typedef double complex comp;

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
int calculation_thread(struct Display* const d)
{
	uint8_t* image = malloc(3 * d->width * d->height * sizeof(uint8_t));
	int checkerSize = 50;
	for (int i = 0; i < d->width; ++i)
	{
		bool flip = (i / checkerSize) & 1;
		for (int j = 0; j < d->height; ++j)
		{
			bool check = flip != ((j / checkerSize) & 1);
			int base = (i + j * d->width) * 3;
			image[base + 0] = check ? 255 : 0;
			image[base + 1] = check ? 255 : 0;
			image[base + 2] = check ? 255 : 0;
		}
	}

	// Convert image to YUV
	uint8_t* dataIn[3];
	dataIn[0] = dataIn[1] = dataIn[2] = image;
	int linesizeIn[3];
	linesizeIn[0] = linesizeIn[1] = linesizeIn[2] = d->width * 3;

	uint8_t* dataOut[3];
	int linesizeOut[3];
	size_t pitchUV = d->width / 2;
	linesizeOut[0] = d->width;
	linesizeOut[1] = pitchUV;
	linesizeOut[2] = pitchUV;

	while (true)
	{
		if (!Display_pictQueue_write(d)) break;
		struct Picture* p = &d->pictQueue[d->pictQueueIW];
		dataOut[0] = p->planeY;
		dataOut[1] = p->planeU;
		dataOut[2] = p->planeV;

		sws_scale(d->swsContext,
		          (uint8_t const* const*) dataIn, linesizeIn, 0, d->height,
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
	display.renderer = SDL_CreateRenderer(display.window, -1, 0);
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
