#include "display.h"

#include <assert.h>

void Display_init(struct Display* const d)
{
	assert(d);
	memset(d, 0, sizeof(struct Display));
	d->pictQueueMutex = SDL_CreateMutex();
	d->pictQueueCond = SDL_CreateCond();
}
void Display_destroy(struct Display* const d)
{
	if (!d) return;
	SDL_DestroyMutex(d->pictQueueMutex);
	SDL_DestroyCond(d->pictQueueCond);
	sws_freeContext(d->swsContext);
}
bool Display_pictQueue_init(struct Display* const d)
{
	assert(d);
	assert(d->renderer);
	assert(d->width != 0 && d->height != 0);
	// YYYYUV Format
	size_t planeSizeY = d->width * d->height;
	size_t planeSizeUV = planeSizeY / 4;
	for (size_t i = 0; i < DISPLAY_PICTQUEUE_SIZE_MAX; ++i)
	{
		struct Picture* const p = &d->pictQueue[i];
		p->texture = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_YV12,
		                               SDL_TEXTUREACCESS_STREAMING,
		                               d->width, d->height);
		if (!p->texture) goto fail;
		p->planeY = malloc(sizeof(*p->planeY) * planeSizeY);
		p->planeU = malloc(sizeof(*p->planeU) * planeSizeUV);
		p->planeV = malloc(sizeof(*p->planeV) * planeSizeUV);
		if (!p->planeY || !p->planeU || !p->planeV) goto fail;
	}
	return true;
fail:
	Display_pictQueue_destroy(d);
	return false;
}
void Display_pictQueue_destroy(struct Display* const d)
{
	assert(d);
	for (size_t i = 0; i < DISPLAY_PICTQUEUE_SIZE_MAX; ++i)
	{
		struct Picture* const p = &d->pictQueue[i];
		SDL_DestroyTexture(p->texture);
		free(p->planeY);
		free(p->planeU);
		free(p->planeV);
	}
}
bool Display_pictQueue_write(struct Display* const d)
{
	assert(d);
	SDL_LockMutex(d->pictQueueMutex);
	while (d->pictQueueSize >= DISPLAY_PICTQUEUE_SIZE_MAX &&
	       !d->quit)
	{
		SDL_CondWait(d->pictQueueCond, d->pictQueueMutex);
	}
	SDL_UnlockMutex(d->pictQueueMutex);

	if (d->quit) return false;
	return true;
}
void Display_pictQueue_draw(struct Display* const d)
{
	struct Picture* p = &d->pictQueue[d->pictQueueIR];
	assert(p->texture);
	assert(p->planeY && p->planeU && p->planeV);
	SDL_UpdateYUVTexture(p->texture, NULL,
	                     p->planeY, d->width,
	                     p->planeU, d->width / 2,
	                     p->planeV, d->width / 2);
	SDL_RenderClear(d->renderer);
	SDL_RenderCopy(d->renderer, p->texture, NULL, 0);
	SDL_RenderPresent(d->renderer);

	++d->pictQueueIR;
	if (d->pictQueueIR == DISPLAY_PICTQUEUE_SIZE_MAX)
		d->pictQueueIR = 0;
	SDL_LockMutex(d->pictQueueMutex);
	--d->pictQueueSize;
	SDL_CondSignal(d->pictQueueCond);
	SDL_UnlockMutex(d->pictQueueMutex);
}

