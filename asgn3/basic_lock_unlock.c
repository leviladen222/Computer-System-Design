#include "rwlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_READERS 5
#define NUM_WRITERS 3

void *reader_thread(void *arg) {
    rwlock_t *rw = (rwlock_t *)arg;
    for (int i = 0; i < 5; i++) {
        reader_lock(rw);
        printf("Reader %ld acquired lock\n", pthread_self());
        usleep(50000);
        reader_unlock(rw);
        printf("Reader %ld released lock\n", pthread_self());
    }
    return NULL;
}

void *writer_thread(void *arg) {
    rwlock_t *rw = (rwlock_t *)arg;
    for (int i = 0; i < 3; i++) {
        writer_lock(rw);
        printf("Writer %ld acquired lock\n", pthread_self());
        usleep(100000);
        writer_unlock(rw);
        printf("Writer %ld released lock\n", pthread_self());
    }
    return NULL;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    rwlock_t *rw = rwlock_new(N_WAY, 2);
    if (rw == NULL) {
        return 1;
    }

    pthread_t readers[NUM_READERS], writers[NUM_WRITERS];

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_create(&readers[i], NULL, reader_thread, rw);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_create(&writers[i], NULL, writer_thread, rw);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }

    rwlock_delete(&rw);
    return 0;
}
