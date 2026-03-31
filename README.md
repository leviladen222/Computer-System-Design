# CSE 130 Project Repository

This repository contains all CSE 130 assignments, organized into separate directories (`asgn0` through `asgn5`).

## Projects

- `asgn0`  
  Introductory C assignment with a simple `hello.c` program and `Makefile`.

- `asgn1`  
  Memory-focused assignment centered around `memory.c`. Includes test helpers and scripts such as `test.sh`, `test_repo.sh`, and `test_files/`.

- `asgn2`  
  HTTP server assignment based on `httpserver.c` with provided networking headers (`listener_socket.h`, `iowrapper.h`, `protocol.h`) and automated tests in `test_scripts/`.

- `asgn3`  
  Synchronization/concurrency assignment implementing queue and reader-writer lock components (`queue.c`, `rwlock.c`) with usage/testing utilities.

- `asgn4`  
  Extended HTTP server assignment with request/response infrastructure (`request.h`, `response.h`, `connection.h`), workloads, replay tooling, and output artifacts under `outs/`.

- `asgn5`  
  HTTP proxy and caching assignment built around `httpproxy.c` and `cache.c`, with protocol/support headers, workloads, replay tools, and test resources.

## Common Structure

Most assignment directories include:

- `Makefile` for building
- `config.json` for assignment config/autograder settings
- `test_files/` and/or `test_scripts/` for local testing
- `test_repo.sh` for repository-level validation

## Notes

- Keep generated artifacts out of version control when possible.
- Use each assignment's local `README.md` for assignment-specific design notes and testing details.
