#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "threadsafeQueue.h"

/* Alloctes a new ThreadsaveQueue_t data structure and returns a pointer to it. */
ThreadsafeQueue_t * newThreadsafeQueue(size_t capacity) {
    ThreadsafeQueue_t * q = (ThreadsafeQueue_t *)malloc(sizeof(ThreadsafeQueue_t));
    q->queue = NULL;
    q->queue = malloc(sizeof(void *) * capacity);
    q->head = 0;
    q->tail = 0;
    q->capacity = capacity;
    
    q->items = 0;
    q->spaces = capacity;
    
    pthread_cond_init(&(q->producable), NULL);
    pthread_cond_init(&(q->consumable), NULL);
    pthread_mutex_init(&(q->mutex), NULL);
    
    return q;
}

/* Deallocates a ThreadsafeQueue_t data structure. */
void destroyThreadsafeQueue(ThreadsafeQueue_t * queue) {
    pthread_mutex_destroy(&(queue->mutex));
    pthread_cond_destroy(&(queue->consumable));
    pthread_cond_destroy(&(queue->producable));
    if (queue->queue != NULL) {
        free(queue->queue);
    }
    free(queue);
}

/* Pushes a void pointer item to the referenced ThreadsafeQueue_t. A void pointer
is chosen such that a pointer to any type of data structure can be placed on the
queue. */
void pushThreadsafeQueue(ThreadsafeQueue_t * queue, void * item) {
    pthread_mutex_lock(&(queue->mutex));
    while(queue->spaces == 0) {
        pthread_cond_wait(&(queue->producable), &(queue->mutex));
    }
    //put
    queue->queue[queue->tail] = item;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->spaces--;
    queue->items++;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_signal(&(queue->consumable));
}

/* Pops an item from the referenced ThreadsafeQueue_t and returns a pointer
to the popped item. */
void * popThreadsafeQueue(ThreadsafeQueue_t * queue) {
    pthread_mutex_lock(&(queue->mutex));
    while(queue->items == 0) {
        pthread_cond_wait(&(queue->consumable), &(queue->mutex));
    }
    //get
    void * item = NULL;
    item = queue->queue[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->spaces++;
    queue->items--;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_signal(&(queue->producable));
    
    return item;
}

/* Function used by the testThreadsafeQueue function to emulate a producer thread. */
static void * producerTest(void * queue) {
    ThreadsafeQueue_t * q = (ThreadsafeQueue_t *)queue;
    
    const int limit = 26;
    for (int i = 0; i < limit; i++) {
        char * c = (char *)malloc(1);
        (*c) = 97 + i;
        pushThreadsafeQueue(q, c);
    }
    
    return NULL;
}

/* Test cases for thread safe queue functionality. */
void testThreadsafeQueue() {
    char string[256] = { '\0' }; 
    ThreadsafeQueue_t * queue = newThreadsafeQueue(8);

    //have two threads, each produce/consume a known amount
    pthread_t producerThread;
    assert(pthread_create(&producerThread, NULL, producerTest, queue) == 0);
    
    //consumer lives here in the main thread
    int consumed = 0;
    while (consumed < 26) {
        char *c = (char *)(popThreadsafeQueue(queue));
        string[consumed] = *c;
        free(c);
        consumed++;
    }
    
    //ensure that order is preserved
    assert(strcmp(string, "abcdefghijklmnopqrstuvwxyz") == 0);
    
    pthread_join(producerThread, NULL);
    
    destroyThreadsafeQueue(queue);
}
    
