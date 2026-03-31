#include <pthread.h>
#include <stdlib.h>
#include "rwlock.h"
#include <assert.h>

// RWLock structure
typedef struct rwlock {
    pthread_mutex_t lock;
    pthread_cond_t read_cond;
    pthread_cond_t write_cond;
    int active_readers;
    int waiting_readers;
    int active_writers;
    int waiting_writers;
    PRIORITY p;
    int n;
} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = malloc(sizeof(rwlock_t));
    if (rw == NULL) {
        return NULL;
    }
    rw->p = p;
    rw->n = n;
    rw->active_readers = 0;
    rw->waiting_readers = 0;
    rw->active_writers = 0;
    rw->waiting_writers = 0;
    pthread_mutex_init(&rw->lock, NULL);
    pthread_cond_init(&rw->read_cond, NULL);
    pthread_cond_init(&rw->write_cond, NULL);
    return rw;
}

void rwlock_delete(rwlock_t **l) {
    if (l != NULL && *l != NULL) {
        pthread_mutex_destroy(&(*l)->lock);
        pthread_cond_destroy(&(*l)->read_cond);
        pthread_cond_destroy(&(*l)->write_cond);
        free(*l);
        *l = NULL;
    }
}

void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->waiting_readers++;
    if (rw->p == READERS) {
        while (rw->active_writers > 0) {
            pthread_cond_wait(&rw->read_cond, &rw->lock);
        }
    } else {
        while (rw->active_writers > 0 || rw->waiting_writers > 0) {
            pthread_cond_wait(&rw->read_cond, &rw->lock);
        }
    }
    rw->waiting_readers--;
    rw->active_readers++;
    pthread_mutex_unlock(&rw->lock);
}

void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->active_readers--;
    if (rw->active_readers == 0) {
        if (rw->p == WRITERS && rw->waiting_writers > 0) {
            pthread_cond_signal(&rw->write_cond);
        } else if (rw->p == READERS && rw->waiting_readers > 0) {
            pthread_cond_broadcast(&rw->read_cond);
        } else {
            if (rw->waiting_writers > 0) {
                pthread_cond_signal(&rw->write_cond);
            } else {
                pthread_cond_broadcast(&rw->read_cond);
            }
        }
    }

    pthread_mutex_unlock(&rw->lock);
}

void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->waiting_writers++;
    if (rw->p == WRITERS) {
        while (rw->active_readers > 0 || rw->active_writers > 0) {
            pthread_cond_wait(&rw->write_cond, &rw->lock);
        }
    } else {
        while (rw->active_readers > 0 || rw->active_writers > 0) {
            pthread_cond_wait(&rw->write_cond, &rw->lock);
        }
    }

    rw->waiting_writers--;
    rw->active_writers++;
    pthread_mutex_unlock(&rw->lock);
}

void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->active_writers--;
    assert(rw->active_writers == 0);

    if (rw->p == WRITERS && rw->waiting_writers > 0) {
        pthread_cond_signal(&rw->write_cond);
    } else if (rw->p == READERS && rw->waiting_readers > 0) {
        pthread_cond_broadcast(&rw->read_cond);
    } else {
        if (rw->waiting_writers > 0) {
            pthread_cond_signal(&rw->write_cond);
        } else {
            pthread_cond_broadcast(&rw->read_cond);
        }
    }

    pthread_mutex_unlock(&rw->lock);
}
