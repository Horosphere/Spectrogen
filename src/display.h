#ifndef SPECTROGEN__DISPLAY_H_
#define SPECTROGEN__DISPLAY_H_

#include <stdbool.h>

#include <libavutil/pixfmt.h>
#include <SDL2/SDL.h>
#include <libswscale/swscale.h>

struct Picture
{
	SDL_Texture* texture;
	uint8_t* planeY;
	uint8_t* planeU;
	uint8_t* planeV;
};

#define DISPLAY_PICTQUEUE_SIZE_MAX 1
#define DISPLAY_PICTQUEUE_PIXFMT AV_PIX_FMT_YUV420P

struct Display
{
	_Atomic bool quit;

	int width;
	int height;
	SDL_Window* window;
	SDL_Renderer* renderer;
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
 */
bool Display_pictQueue_init(struct Display* const);
/**
 * Render thread only
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

#endif // !SPECTROGEN__DISPLAY_H_
