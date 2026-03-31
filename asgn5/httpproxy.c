#include "client_socket.h"
#include "iowrapper.h"
#include "listener_socket.h"
#include "prequest.h"
#include "a5protocol.h"
#include "cache.h"

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <regex.h>

void handle_connection(uintptr_t, int);

// globals:
Listener_Socket_t *sock = NULL;
Cache *cache = NULL;

void usage(FILE *stream, char *exec) {
    fprintf(stream, "usage: %s <port> \n", exec);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    // parse port number
    char *endptr = NULL;
    int port = (size_t) strtoull(argv[1], &endptr, 10);
    if (endptr && *endptr != '\0') {
        fprintf(stderr, "Invalid Argument\n");
        return EXIT_FAILURE;
    }

    char *mode = argv[2];
    if (strcmp(mode, "FIFO") != 0 && strcmp(mode, "LRU") != 0) {
        fprintf(stderr, "Invalid Argument\n");
        return EXIT_FAILURE;
    }

    int cache_size = (size_t) strtoull(argv[3], &endptr, 10);
    if ((endptr && *endptr != '\0') || cache_size < 0 || cache_size > 1024) {
        fprintf(stderr, "Invalid Argument\n");
        return EXIT_FAILURE;
    }
    printf("Port: %d\n", port);
    printf("Mode: %s\n", mode);
    printf("Cache size: %d\n", cache_size);

    if (cache == NULL && cache_size > 0 && strcmp(mode, "FIFO") == 0) {
        cache = createCache(cache_size, true);
    } else if (cache == NULL && cache_size > 0 && strcmp(mode, "LRU") == 0) {
        cache = createCache(cache_size, false);
    }
    sock = ls_new(port);

    if (sock) {
        while (1) {
            uintptr_t connfd = ls_accept(sock);
            assert(connfd > 0);
            handle_connection(connfd, cache_size);
        }
    }

    return EXIT_SUCCESS;
}

void handle_connection(uintptr_t connfd, int cache_size) {
    // Parse the proxy request from connfd
    // Initialize the cache if it is not already initialized
    Prequest_t *preq = prequest_new(connfd);
    if (!preq) {
        fprintf(stderr, "Bad request from %lu\n", connfd);
        close(connfd);
        return;
    }
    char *host = prequest_get_host(preq);
    char *uri = prequest_get_uri(preq);
    int32_t port = prequest_get_port(preq);
    fprintf(stderr, "Host=%s, Port=%d, URI=%s\n", host, port, uri);

    // Create the GET request
    char request[2048];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n",
        uri, host);
    char key[MAX_URI_SIZE + MAX_HOST_SIZE + MAX_PORT_SIZE + 3];
    snprintf(key, sizeof(key), "%s:%d%s", host, port, uri);

    // Check if the response is cached
    char *cached_file = NULL;
    if (cache_size > 0) {
        cached_file = get(cache, key);
    }

    // If the response is cached, send it to the client
    if (cached_file != NULL) {
        int file_fd = open(cached_file, O_RDONLY);
        if (file_fd >= 0) {
            pass_n_bytes(file_fd, connfd, 17);
            write_n_bytes(connfd, "Cached: True\r\n", 14);
            while (pass_n_bytes(file_fd, connfd, 2048) == 2048) {
            }
            close(file_fd);
        } else {
            fprintf(stderr, "Failed to open cached file: %s\n", cached_file);
        }

        prequest_delete(&preq);
        close(connfd);
        printCache(cache);
        return;
    }

    // If the response is not cached, fetch it from the remote server
    int32_t client_sock = cs_new(host, port);
    printf("Client sock: %d\n", client_sock);
    if (client_sock < 0) {
        fprintf(stderr, "Cannot connect to host %s\n", host);
        prequest_delete(&preq);
        close(connfd);
        return;
    }

    // Send the request to the remote server
    if (cache_size == 0) {
        //fprintf(stderr, "here1");
        write_n_bytes(client_sock, request, strlen(request));
        //fprintf(stderr, "here2");
        while (pass_n_bytes(client_sock, connfd, 2048) == 2048) {
        }
    }

    // Cache the response if cache is enabled
    if (cache_size > 0) {
        // Generate a unique filename based on host, port, and URI
        char filename[256];
        snprintf(filename, sizeof(filename), "cache_%s_%d_%s", host, port, uri);

        // Open the file for writing
        int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_fd >= 0) {
            write_n_bytes(client_sock, request, strlen(request));
            // Write the response to the file
            while (pass_n_bytes(client_sock, file_fd, 2048) == 2048) {
            }
            close(file_fd);
            file_fd = open(filename, O_RDONLY);
            while (pass_n_bytes(file_fd, connfd, 2048) == 2048) {
            }
            close(file_fd);
            struct stat st;
            stat(filename, &st);
            if (st.st_size > 1048576) {
                remove(filename);
            } else {
                put(cache, key, filename);
            }
        } else {
            fprintf(stderr, "Failed to cache response to file: %s\n", filename);
        }
    }

    // Clean up
    prequest_delete(&preq);
    close(connfd);
    close(client_sock);
}
