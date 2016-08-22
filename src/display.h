#ifndef SPECTROGEN__DISPLAY_H_
#define SPECTROGEN__DISPLAY_H_

#include <stdbool.h>

#include <libavutil/pixfmt.h>
#include <SDL2/SDL.h>
#include <libswscale/swscale.h>

#include "gradient.h"

struct Picture
{
	uint8_t* planeY;
	uint8_t* planeU;
	uint8_t* planeV;
};

#define DISPLAY_PICTQUEUE_SIZE_MAX 1
#define DISPLAY_PICTQUEUE_PIXFMT AV_PIX_FMT_YUV420P

struct Display
{
	_Atomic bool quit;

	struct ColourGradient colourGradient;
	int width;
	int height;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	struct SwsContext* swsContext;

	struct Picture pictQueue[DISPLAY_PICTQUEUE_SIZE_MAX];
	/*
	 * pictQueueIR can only be modified by the render thread
	 * pictQueueIW can only be modified by the writing thread
	 */
	size_t pictQueueSize, pictQueueIR, pictQueueIW;
	SDL_mutex* pictQueueMutex;
	SDL_cond* pictQueueCond;
};

void Display_init(struct Display* const);
void Display_destroy(struct Display* const);
/**
 * Render thread only
 * @brief Initialises the pictQueue and renderer.
 */
bool Display_pictQueue_init(struct Display* const);
/**
 * Render thread only
 * @brief Frees the pictQueue and renderer
 */
void Display_pictQueue_destroy(struct Display* const);
/**
 * @brief Blocks the current thread until the writing position is available.
 * @return false if d->quit is set to true
 */
bool Display_pictQueue_write(struct Display* const);
/**
 * Render thread only
 * @brief Draws the current picture onto the renderer
 */
void Display_pictQueue_draw(struct Display* const);

#define EVENT_REFRESH (SDL_USEREVENT + 2)

void schedule_refresh(void* data, int delay);
void refresh(struct Display* const d);
#endif // !SPECTROGEN__DISPLAY_H_
