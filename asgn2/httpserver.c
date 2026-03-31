#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include "listener_socket.h"
#include "iowrapper.h"
#include "protocol.h"

#define HEADER_END  "\r\n\r\n"
#define BUFFER_SIZE 4096

void send_error_response(int client, int status_code, const char *status_text) {
    char response_header[BUFFER_SIZE];
    snprintf(response_header, sizeof(response_header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %lu\r\n\r\n"
        "%s\n",
        status_code, status_text, strlen(status_text) + 1, status_text);

    write_n_bytes(client, response_header, strlen(response_header));
}
void handle_get_request(int client, char *uri, char *version) {

    // Remove leading "/" from URI to get a valid filename
    const char *filename = uri + 1;

    // Check if the file exists
    struct stat file_stat;
    if (stat(filename, &file_stat) == -1 && access(filename, R_OK) != 0) {
        send_error_response(client, 404, "Not Found");
        return;
    }

    // Check if the requested file is a directory
    if (S_ISDIR(file_stat.st_mode)) {
        send_error_response(client, 403, "Forbidden");
        return;
    }

    // Check read permission
    if (access(filename, R_OK) != 0) {
        send_error_response(client, 403, "Forbidden");
        return;
    }

    // Open the file
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        if (errno == EACCES) { // Permission denied
            send_error_response(client, 403, "Forbidden");
        } else {
            send_error_response(client, 500, "Internal Server Error");
        }
        return;
    }

    // Send HTTP Response Header
    char response_header[BUFFER_SIZE];
    snprintf(response_header, sizeof(response_header),
        "%s 200 OK\r\n"
        "Content-Length: %ld\r\n\r\n",
        version, file_stat.st_size);
    write_n_bytes(client, response_header, strlen(response_header));

    // Send file contents
    char file_buf[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read_n_bytes(file, file_buf, sizeof(file_buf))) > 0) {
        write_n_bytes(client, file_buf, bytes_read);
    }

    close(file);
}

void handle_put_request(
    int client, char *uri, char *version, int content_length, char *body, ssize_t body_length) {
    const char *filename = uri + 1;
    int file_exists = access(filename, F_OK) == 0;
    // Open file for writing (create if not exists, truncate if exists)
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        if (errno == EACCES) { // Permission denied
            send_error_response(client, 403, "Forbidden");
        } else {
            send_error_response(client, 500, "Internal Server Error");
        }
        return;
    }
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) {
        send_error_response(client, 403, "Forbidden");
        close(file);
        return;
    }
    if (file_exists && access(filename, W_OK) != 0) {
        send_error_response(client, 403, "Forbidden");
        return;
    }
    if (file == -1) {
        send_error_response(client, 500, "Internal Server Error");
        close(file);
        return;
    }

    // Read content from the client and write to file
    if (body != NULL && body_length > 0) {
        printf("Writing body start to file\n");
        write_n_bytes(file, body, body_length);
        content_length -= body_length;
    }
    printf("Content Length: %d\n", content_length);
    ssize_t bytes_read;
    bytes_read = pass_n_bytes(client, file, content_length);
    printf("bytes read: %zd\n", bytes_read);
    printf("Here\n");
    char response_header[MAX_HEADER_LEN];
    if (file_exists) {
        snprintf(response_header, sizeof(response_header),
            "%s 200 OK\r\n"
            "Content-Length: %zd\r\n\r\nOK\n",
            version, bytes_read);
    } else {
        snprintf(response_header, sizeof(response_header),
            "%s 201 Created\r\n"
            "Content-Length: %zd\r\n\r\nCreated\n",
            version, bytes_read);
    }
    write_n_bytes(client, response_header, strlen(response_header));
    close(file);
}

ssize_t my_read(int in, char buf[], size_t nbytes) {
    size_t total_bytes_read = 0; // Use size_t for total_bytes_read
    ssize_t bytes_read;
    char *crlf_crlf;
    while (total_bytes_read < nbytes) {
        bytes_read = read(in, buf + total_bytes_read, nbytes - total_bytes_read);
        if (bytes_read == -1) {
            // Error occurred
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available right now, try again
                continue;
            } else {
                // Other error
                return -1;
            }
        } else if (bytes_read == 0) {
            // Socket closed
            return (ssize_t) total_bytes_read; // Cast to ssize_t for return
        }

        total_bytes_read += (size_t) bytes_read; // Cast bytes_read to size_t

        // Check if "\r\n\r\n" is in the buffer
        crlf_crlf = strstr(buf, "\r\n\r\n");
        if (crlf_crlf != NULL) {
            return (ssize_t) total_bytes_read; // Cast to ssize_t for return
        }
    }

    // Read nbytes without finding "\r\n\r\n"
    return (ssize_t) total_bytes_read; // Cast to ssize_t for return
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    char buf[BUFFER_SIZE];
    Listener_Socket_t *listener = ls_new(port);

    if (listener == NULL) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    while (true) {
        int client = ls_accept(listener);
        if (client == -1) {
            fprintf(stderr, "Invalid Port\n");
            exit(1);
        }
        ssize_t bytes_read = my_read(client, buf, MAX_HEADER_LEN);
        char *result = strstr(buf, HEADER_END);
        if (result == NULL) {
            send_error_response(client, 400, "Bad Request");
            close(client);
            continue;
        }
        char *body_start = result + strlen(HEADER_END);
        //printf("body_start: %s\n", body_start);
        ssize_t body_length = bytes_read - (body_start - buf);
        if (body_length < 0) {
            body_length = 0;
            body_start = NULL;
        }
        //printf("body_length: %zd\n", body_length);
        // printf("bytes read - body length: %zd\n", bytes_read - body_length);
        buf[bytes_read] = '\0';
        if (bytes_read == -1) {
            close(client);
            continue;
        }

        //char* request_line = strtok(buf, "\r\n");
        //printf("buf: %s\n", buf);
        //Parse Request Here
        regex_t regex;
        if (strstr(buf, "HTTP/1.10")) {
            send_error_response(client, 400, "Bad Request");
            close(client);
            continue;
        }
        char *pattern = "(" TYPE_REGEX ") (" URI_REGEX ") (" HTTP_REGEX ")";
        regcomp(&regex, pattern, REG_EXTENDED);
        regmatch_t pmatch[4];
        int match = regexec(&regex, buf, 4, pmatch, 0);
        regfree(&regex);
        if (match == REG_NOMATCH) {
            send_error_response(client, 400, "Bad Request");
            close(client);
            continue;
        }

        char type[pmatch[1].rm_eo - pmatch[1].rm_so + 1],
            uri[pmatch[2].rm_eo - pmatch[2].rm_so + 1], http[pmatch[3].rm_eo - pmatch[3].rm_so + 1];
        snprintf(type, pmatch[1].rm_eo - pmatch[1].rm_so + 1, "%s", buf + pmatch[1].rm_so);
        snprintf(uri, pmatch[2].rm_eo - pmatch[2].rm_so + 1, "%s", buf + pmatch[2].rm_so);
        snprintf(http, pmatch[3].rm_eo - pmatch[3].rm_so + 1, "%s", buf + pmatch[3].rm_so);

        if (strcmp(http, "HTTP/1.1") != 0) {
            send_error_response(client, 505, "Version Not Supported");
            close(client);
            continue;
        }
        if (strcmp(type, "PUT") == 0) {
            regex_t regex;
            regmatch_t pmatch[2];
            char *pattern = "Content-Length: ([[:digit:]]+)";
            regcomp(&regex, pattern, REG_EXTENDED);
            match = regexec(&regex, buf, 2, pmatch, 0);
            regfree(&regex);
            if (match == REG_NOMATCH) {
                send_error_response(client, 400, "Bad Request");
                close(client);
                continue;
            }
            char content_length[pmatch[1].rm_eo - pmatch[1].rm_so + 1];
            snprintf(
                content_length, pmatch[1].rm_eo - pmatch[1].rm_so + 1, "%s", buf + pmatch[1].rm_so);

            // fprintf(stderr, "buf: %s\n", buf);
            handle_put_request(client, uri, http, atoi(content_length), body_start, body_length);
        } else if (strcmp(type, "GET") == 0) {
            handle_get_request(client, uri, http);
        } else {
            send_error_response(client, 501, "Not Implemented");
        }
        close(client);
    }

    ls_delete(&listener);
    return 0;
}
