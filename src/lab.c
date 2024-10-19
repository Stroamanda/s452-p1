#include <stdlib.h>
#include <stdbool.h>
#include "../src/lab.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

struct queue {
    int maxSize;
    int currSize;
    char ** data;
    bool shutdown;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**Update this file with the starter code**/
queue_t queue_init(int capacity) {
    queue_t queue = malloc(sizeof(struct queue));
    queue->data = (char**)malloc(capacity * sizeof(char*));
    queue->maxSize = capacity;
    queue->currSize = 0;
    queue-> shutdown = 0;
    return queue;
}

/**
    * @brief Frees all memory and related data signals all waiting threads.
    *
    * @param q a queue to free
    */
void queue_destroy(queue_t q) {
    pthread_mutex_lock(&mutex);
    free(q->data);
    free(q);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

/**
    * @brief Adds an element to the back of the queue
    *
    * @param q the queue
    * @param data the data to add
    */
void enqueue(queue_t q, void *data) {
    pthread_mutex_lock(&mutex);

    // waits while the queue is full
    while (q->currSize == q->maxSize) {
        // check to see if a shutdown happens so nothing is stuck waiting
        if (q->shutdown) {
            pthread_mutex_unlock(&mutex);
            return; 
        }

        pthread_cond_wait(&cond, &mutex);
    }

    q->data[q->currSize] = data;
    q->currSize++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

/**
    * @brief Removes the first element in the queue.
    *
    * @param q the queue
    */
void *dequeue(queue_t q) {
    pthread_mutex_lock(&mutex);

    // waits will there is nothing in the queue
    while (q->currSize == 0) {
        // check to see if a shutdown happens so nothing is stuck waiting
        if (q->shutdown) {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        pthread_cond_wait(&cond, &mutex);
    }

    void *dequeuedItem = q->data[0];
    for (int i = 0; i < q->maxSize - 1; i++) {
        q->data[i] = q->data[i + 1];
    }

    q->currSize--;
    q->data[q->currSize] = NULL;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return dequeuedItem;
}

/**
    * @brief Set the shutdown flag in the queue so all threads can
    * complete and exit properly
    *
    * @param q The queue
    */
void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&mutex);
    q->shutdown = 1;

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

/**
    * @brief Returns true is the queue is empty
    *
    * @param q the queue
    */
bool is_empty(queue_t q) {
    if (q->currSize == 0) {
        return true;
    }
    return false;
}

/**
    * @brief
    *
    * @param q The queue
    */
bool is_shutdown(queue_t q) {
    return q->shutdown;
}