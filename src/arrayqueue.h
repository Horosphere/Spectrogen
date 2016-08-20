#ifndef SPECTROGEN__ARRAYQUEUE_H_
#define SPECTROGEN__ARRAYQUEUE_H_

#include <stddef.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

/**
 * Linked list of arrays
 */
struct ArrayList
{
	void* data;
	size_t size;
	struct ArrayList* next;
};

struct ArrayQueue
{
	struct ArrayList* first;
	struct ArrayList* last;
	int nArrays;
	size_t size;
	SDL_mutex* mutex;
	SDL_cond* cond;
};

void ArrayQueue_init(struct ArrayQueue* const);
void ArrayQueue_destroy(struct ArrayQueue* const);
/**
 * @brief Enqueues a data-size pair into the array queue. The ownership is
 *  transfered to the array
 */
bool ArrayQueue_enqueue(struct ArrayQueue* const, void* data, size_t size);
/**
 * @brief Dequeues a data-size pair from the array queue. The ownership is
 *  transfered out from the array
 * @param[in] block If true, the routine will be stuck when no elements are in
 *  the array queue.
 * @return -1 if *state == true, 1 if successful, 0 if no elements are in the
 *  array queue and block is set to false.
 */
int ArrayQueue_dequeue(struct ArrayQueue* const,
                       void** const data, size_t* const size,
                       bool block, _Atomic bool const* const state);

#endif // !SPECTROGEN__ARRAYQUEUE_H_
