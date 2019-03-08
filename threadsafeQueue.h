//See threadsaveQueue.c for function documentation. 

#ifndef THREADSAFEQUEUE_H
#define	THREADSAFEQUEUE_H

#include <pthread.h>
#include <semaphore.h>

typedef struct ThreadsafeQueue_s {
    void ** queue;
    size_t head;
    size_t tail;
    size_t capacity;
    size_t items;
    size_t spaces;
    pthread_mutex_t mutex;
    pthread_cond_t producable;
    pthread_cond_t consumable;
} ThreadsafeQueue_t;

ThreadsafeQueue_t * newThreadsafeQueue(size_t capacity);
void destroyThreadsafeQueue(ThreadsafeQueue_t * queue);

void pushThreadsafeQueue(ThreadsafeQueue_t * queue, void * item);
void * popThreadsafeQueue(ThreadsafeQueue_t * queue);

void testThreadsafeQueue();

#endif	/* THREADSAFEQUEUE_H */

