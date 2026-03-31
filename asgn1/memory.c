#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 4096

// Utility to handle "Operation Failed"
void operation_failed(void) {
    fprintf(stderr, "Operation Failed\n");
    exit(1);
}

// Utility to handle "Invalid Command"
void invalid_command(void) {
    fprintf(stderr, "Invalid Command\n");
    exit(1);
}

// Get command implementation
void get(const char *location) {
    int fd = open(location, O_RDONLY);
    if (fd == -1) {
        invalid_command();
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        ssize_t total_written = 0;
        while (total_written < bytes_read) {
            ssize_t written = write(STDOUT_FILENO, buffer + total_written, bytes_read - total_written);
            if (written <= 0) {
                close(fd);
                operation_failed();
            }
            total_written += written;
        }
    }

    if (bytes_read == -1) {
        if (errno == EISDIR) {
            close(fd);
            invalid_command();
        } else {
            close(fd);
            operation_failed();
        }
    }

    close(fd);
    exit(0);
}

// Set command implementation
void set(const char *location, const char *content_length_str, const char *initial_contents) {
    char buffer[BUFFER_SIZE];
    char *endptr;

    // Parse the content length
    long content_length = strtol(content_length_str, &endptr, 10);
    if (*endptr != '\0' || content_length < 0) {
        invalid_command();
    }

    // Open the file for writing
    int fd = open(location, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        operation_failed();
    }

    printf("Opening file: %s\n", location);

    ssize_t total_written = 0;
    ssize_t initial_len = initial_contents ? strlen(initial_contents) : 0;

    // Write initial contents, up to the specified content length
    if (initial_contents) {
        printf("Writing initial contents to file: %s\n", location);
        ssize_t to_write = initial_len > content_length ? content_length : initial_len;
        ssize_t offset = 0;
        while (to_write > 0) {
            ssize_t written = write(fd, initial_contents + offset, to_write);
            if (written <= 0) {
                close(fd);
                operation_failed();
            }
            printf("Bytes written: %ld\n", written);
            to_write -= written;
            offset += written;
            total_written += written;
        }
    }

    printf("Total written: %ld, Content length: %ld\n", total_written, content_length);

    // Read remaining bytes from stdin, up to the specified content length
    while (total_written < content_length) {
        printf("Reading additional bytes from stdin\n");
        ssize_t to_read = content_length - total_written;
        ssize_t bytes_read = read(STDIN_FILENO, buffer, to_read < BUFFER_SIZE ? to_read : BUFFER_SIZE);
        if (bytes_read < 0) { // Error during reading
            close(fd);
            operation_failed();
        }
        if (bytes_read == 0) { // EOF
            break;
        }

        printf("Bytes read: %ld\n", bytes_read);

        ssize_t to_write = bytes_read;
        ssize_t offset = 0;
        while (to_write > 0) {
            ssize_t written = write(fd, buffer + offset, to_write);
            if (written <= 0) {
                close(fd);
                operation_failed();
            }
            printf("Bytes written: %ld\n", written);
            to_write -= written;
            offset += written;
        }
        total_written += bytes_read;
    }

    close(fd);
    printf("OK\n");
    exit(0);
}

int main(void) {
    char buf[BUFFER_SIZE];
    ssize_t bytes_read = read(STDIN_FILENO, buf, BUFFER_SIZE);
    if (bytes_read <= 0) {
        invalid_command();
    }
    buf[bytes_read] = '\0';

    char *command = strtok(buf, "\n");
    if (command) {
        if (strcmp(command, "get") == 0) {
            if (buf[bytes_read - 1] != '\n') {
                invalid_command();
            }
        }
    }
    char *location = strtok(NULL, "\n");

    if (!command || !location) {
        invalid_command();
    }

    if (strcmp(command, "get") == 0) {
        char *extra = strtok(NULL, "\n");
        if (extra != NULL) {
            invalid_command();
        }
        get(location);
    } else if (strcmp(command, "set") == 0) {
        char *content_length_str = strtok(NULL, "\n");
        if (!content_length_str) {
            invalid_command();
        }
        char *contents = strtok(NULL, "");
        if(contents != NULL) {
            contents[strlen(contents) - 1] = '\0';
        }
        printf("contents: %s\n", contents);
        // Parse and validate content length
        char *endptr;
        long content_length = strtol(content_length_str, &endptr, 10);
        if (*endptr != '\0' || content_length < 0) {
            invalid_command();
        }
        set(location, content_length_str, contents);
    } else {
        invalid_command();
    }

    return 0;
}
