#include "arrayqueue.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

void ArrayQueue_init(struct ArrayQueue* const q)
{
	assert(q);
	memset(q, 0, sizeof(struct ArrayQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}
void ArrayQueue_destroy(struct ArrayQueue* const q)
{
	struct ArrayList* l = q->first;
	while (l)
	{
		free(l->data);
		struct ArrayList* temp = l->next;
		free(l);
		l = temp;
	}
	SDL_DestroyMutex(q->mutex);
	SDL_DestroyCond(q->cond);
}
bool ArrayQueue_enqueue(struct ArrayQueue* const q, void* data, size_t size)
{
	struct ArrayList* l = malloc(sizeof(struct ArrayList));
	if (!l) return false;

	l->data = data;
	l->next = NULL;

	SDL_LockMutex(q->mutex);
	if (q->last)
		q->last->next = l;
	else
		q->first = l;
	q->last = l;
	++q->nArrays;
	q->size += size;

	SDL_CondSignal(q->cond);
	SDL_UnlockMutex(q->mutex);

	return true;
}
int ArrayQueue_dequeue(struct ArrayQueue* const q,
                       void** const data, size_t* const size,
                       bool block, _Atomic bool const* const state)
{
	int result = -1;
	struct ArrayList* l;
	SDL_LockMutex(q->mutex);
	while (*state == false)
	{
		l = q->first;
		if (l)
		{
			q->first = l->next;
			if (!q->first) q->last = NULL; // Array queue depleted
			--q->nArrays;
			q->size -= l->size;
			*data = l->data;
			*size = l->size;
			free(l);
			result = 1;
			break;
		}
		else if (!block)
		{
			result = 0;
			break;
		}
		else SDL_CondWait(q->cond, q->mutex);
	}
	SDL_UnlockMutex(q->mutex);
	return result;
}

