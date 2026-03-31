#include "listener_socket.h"
#include "connection.h"
#include "response.h"
#include "request.h"
#include "queue.h"
#include "rwlock.h"
#include "iowrapper.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>

// Structure to store URI locks
typedef struct uri_lock {
    char *uri;
    rwlock_t *lock;
    struct uri_lock *next;
} uri_lock_t;

// Global variables
queue_t *request_queue;
pthread_t *worker_threads;
uri_lock_t *uri_locks = NULL;
pthread_mutex_t uri_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function declarations
void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *worker_function(void *arg);
rwlock_t *get_uri_lock(const char *uri);
void audit_log(const char *method, const char *uri, const Response_t *response, const char *request_id);

// Create or retrieve a lock for a URI
rwlock_t *get_uri_lock(const char *uri) {
    pthread_mutex_lock(&uri_mutex);
    
    // Search for existing lock
    uri_lock_t *current = uri_locks;
    while (current) {
        if (strcmp(current->uri, uri) == 0) {
            pthread_mutex_unlock(&uri_mutex);
            return current->lock;
        }
        current = current->next;
    }
    
    // Create new lock WHILE STILL HOLDING the mutex
    uri_lock_t *new_lock = malloc(sizeof(uri_lock_t));
    if (!new_lock) {
        pthread_mutex_unlock(&uri_mutex);
        return NULL;
    }
    
    new_lock->uri = strdup(uri);
    new_lock->lock = rwlock_new(WRITERS, 1);
    new_lock->next = uri_locks;
    uri_locks = new_lock;
    
    pthread_mutex_unlock(&uri_mutex);
    return new_lock->lock;
}


// Write to audit log with proper synchronization
void audit_log(const char *method, const char *uri, const Response_t *response, const char *request_id) {
    pthread_mutex_lock(&log_mutex);
    fprintf(stderr, "%s,%s,%d,%s\n", 
            method, 
            uri, 
            response_get_code(response),
            request_id ? request_id : "0");
    fflush(stderr);
    pthread_mutex_unlock(&log_mutex);
}

// Worker thread function
void *worker_function(void *arg) {
    (void)arg; // Unused parameter
    
    while (1) {
        // Get connection from queue
        void *elem = NULL;
        bool success = queue_pop(request_queue, &elem);
        if (!success || elem == NULL) {
            continue;
        }
        
        int connfd = *(int *)elem;
        free(elem);
        
        // Process the connection
        handle_connection(connfd);
        close(connfd);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int threads = 4;  
    size_t port = 0;  
    int opt;
    char *endptr;
    
    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't':
                threads = atoi(optarg);
                if (threads <= 0) {
                    fprintf(stderr, "Error: Invalid number of threads: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case '?':
                fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // Ensure port argument is provided
    if (optind < argc) {
        errno = 0; 
        port = strtoul(argv[optind], &endptr, 10);
        // Validate conversion
        if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
            fprintf(stderr, "Error: Invalid port number: %s\n", argv[optind]);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Initialize request queue
    request_queue = queue_new(1024); // Queue size of 1024
    if (!request_queue) {
        warnx("cannot create request queue");
        return EXIT_FAILURE;
    }
    
    // Create worker threads
    worker_threads = malloc(threads * sizeof(pthread_t));
    if (!worker_threads) {
        warnx("cannot allocate memory for worker threads");
        return EXIT_FAILURE;
    }
    
    // Start worker threads
    for (int i = 0; i < threads; i++) {
        if (pthread_create(&worker_threads[i], NULL, worker_function, NULL) != 0) {
            warnx("cannot create worker thread");
            return EXIT_FAILURE;
        }
    }
    
    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    // Create listener socket
    Listener_Socket_t *sock = ls_new(port);
    if (!sock) {
        warnx("cannot open socket");
        return EXIT_FAILURE;
    }
    
    // Main dispatcher loop
    while (1) {
        int connfd = ls_accept(sock);
        if (connfd < 0) continue;
        
        // Allocate memory for connection fd
        int *connfd_ptr = malloc(sizeof(int));
        if (!connfd_ptr) {
            close(connfd);
            continue;
        }
        
        *connfd_ptr = connfd;
        
        // Push to queue
        if (!queue_push(request_queue, connfd_ptr)) {
            // Queue is full
            free(connfd_ptr);
            close(connfd);
        }
    }
    
    // Cleanup (not reached in this implementation)
    ls_delete(&sock);
    
    return EXIT_SUCCESS;
}

void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);
    if (!conn) return;
    
    const Response_t *res = conn_parse(conn);
    
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else if (req == &REQUEST_GET) {
            handle_get(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    
    conn_delete(&conn);
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    char *request_id = conn_get_header(conn, "Request-Id");
    const Response_t *res = NULL;
    
    // Get the lock for this URI
    rwlock_t *lock = get_uri_lock(uri);
    if (!lock) {
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        audit_log("GET", uri, res, request_id);
        conn_send_response(conn, res);
        return;
    }
    
    // Acquire reader lock
    reader_lock(lock);
    
    int fd = open(uri, O_RDONLY);
    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR) {
            res = &RESPONSE_FORBIDDEN;
        } else if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        audit_log("GET", uri, res, request_id);
        conn_send_response(conn, res);
        reader_unlock(lock);
        return;
    }
    
    // Get file size using fstat
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        audit_log("GET", uri, res, request_id);
        conn_send_response(conn, res);
        reader_unlock(lock);
        return;
    }
    
    // Send file contents
    res = conn_send_file(conn, fd, st.st_size);
    close(fd);
    
    // If file was sent successfully
    if (res == NULL) {
        res = &RESPONSE_OK;
    }
    
    // Log operation and send response
    audit_log("GET", uri, res, request_id);
    reader_unlock(lock);
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    char *request_id = conn_get_header(conn, "Request-Id");
    const Response_t *res = NULL;

    // Make a writable copy of URI for mkstemp
    char temp_filename[1024];
    snprintf(temp_filename, sizeof(temp_filename), "%sXXXXXX", uri);

    // Create a temporary file safely
    int temp_fd = mkstemp(temp_filename);
    if (temp_fd < 0) {
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        audit_log("PUT", uri, res, request_id);
        conn_send_response(conn, res);
        return;
    }

    // Receive file data from connection into temporary file
    res = conn_recv_file(conn, temp_fd);
    close(temp_fd);  

    if (res != NULL) {
        unlink(temp_filename);  
        audit_log("PUT", uri, res, request_id);
        conn_send_response(conn, res);
        return;
    }

    // Get lock for this URI
    rwlock_t *lock = get_uri_lock(uri);
    if (!lock) {
        unlink(temp_filename);
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        audit_log("PUT", uri, res, request_id);
        conn_send_response(conn, res);
        return;
    }

    // Acquire writer lock
    writer_lock(lock);

    // Check if file already exists AFTER acquiring the lock
    bool existed = access(uri, F_OK) == 0;

    // Rename temporary file to final location
    if (rename(temp_filename, uri) < 0) {
        perror("rename failed");
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        unlink(temp_filename); 
    } else {
        res = existed ? &RESPONSE_OK : &RESPONSE_CREATED;
    }

    // Log operation and send response
    audit_log("PUT", uri, res, request_id);
    conn_send_response(conn, res);

    writer_unlock(lock);
}



void handle_unsupported(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    char *request_id = conn_get_header(conn, "Request-Id");
    
    // Log operation and send response
    audit_log("UNSUPPORTED", uri, &RESPONSE_NOT_IMPLEMENTED, request_id);
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}



