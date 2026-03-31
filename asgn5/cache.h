// Used as starter code: https://www.geeksforgeeks.org/lru-cache-implementation-using-double-linked-lists/?ref=ml_lbp
#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Node structure for doubly linked list
struct Node {
    char *key;
    char *value;
    struct Node* next;
    struct Node* prev;
};

// Cache structure
typedef struct {
    int capacity;
    struct Node* head;
    struct Node* tail;
    struct Node** cacheMap; // Array of pointers to Node (simulating a map)
    bool isFIFO;
    int count;
} Cache;

// Function to create a new node
struct Node* createNode(char *key, char *value);

// Function to initialize the Cache
Cache* createCache(int capacity, bool isFIFO);

// Function to add a node to the front of the list (most recently used)
void addNode(Cache* cache, struct Node* node);

// Function to remove a node from the list
void removeNode(struct Node* node);

// Function to get the value for a given key
char* get(Cache* cache, char *key);

// Function to put a key-value pair into the cache
void put(Cache* cache, char *key, char *value);

// Function to free the Cache
void freeCache(Cache* cache);

void printCache(Cache* cache);

#endif // CACHE_H
