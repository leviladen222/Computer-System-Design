// Used as starter code: https://www.geeksforgeeks.org/lru-cache-implementation-using-double-linked-lists/?ref=ml_lbp
#include "cache.h"

// Function to create a new node
struct Node *createNode(char *key, char *value) {
    struct Node *newNode = (struct Node *) malloc(sizeof(struct Node));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed for node\n");
        exit(EXIT_FAILURE);
    }
    newNode->key = strdup(key);
    newNode->value = strdup(value);
    newNode->next = NULL;
    newNode->prev = NULL;
    return newNode;
}

// Function to initialize the Cache
Cache *createCache(int capacity, bool isFIFO) {
    Cache *cache = (Cache *) malloc(sizeof(Cache));
    if (!cache) {
        fprintf(stderr, "Memory allocation failed for cache\n");
        exit(EXIT_FAILURE);
    }
    cache->capacity = capacity;
    cache->head = createNode("", ""); // Dummy head
    cache->tail = createNode("", ""); // Dummy tail
    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    cache->cacheMap = (struct Node **) calloc(
        1024, sizeof(struct Node *)); // Assuming a fixed size for simplicity
    cache->isFIFO = isFIFO;
    cache->count = 0;
    if (!cache->cacheMap) {
        fprintf(stderr, "Memory allocation failed for cache map\n");
        exit(EXIT_FAILURE);
    }
    return cache;
}

// Function to add a node to the front of the list (most recently used)
void addNode(Cache *cache, struct Node *node) {
    struct Node *nextNode = cache->head->next;
    cache->head->next = node;
    node->prev = cache->head;
    node->next = nextNode;
    nextNode->prev = node;
}

// Function to remove a node from the list
void removeNode(struct Node *node) {
    struct Node *prevNode = node->prev;
    struct Node *nextNode = node->next;
    prevNode->next = nextNode;
    nextNode->prev = prevNode;
}

// Function to get the value for a given key
char *get(Cache *cache, char *key) {
    for (struct Node *node = cache->head->next; node != cache->tail; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            if (!cache->isFIFO) {
                removeNode(node); // Remove the node from its current position
                addNode(cache, node); // Add it to the front (most recently used)
            }
            return node->value;
        }
    }
    return NULL; // Key not found
}

// Function to put a key-value pair into the cache
void put(Cache *cache, char *key, char *value) {

    // Create a new node and add it to the cache
    struct Node *newNode = createNode(key, value);
    addNode(cache, newNode);
    cache->count++;

    // If the cache exceeds capacity, remove the least recently used node
    if (cache->count > cache->capacity) {
        cache->count--;
        struct Node *nodeToDelete = cache->tail->prev;
        if (nodeToDelete != cache->head) { // Ensure we're not deleting the dummy head
            int num = remove(nodeToDelete->value);
            fprintf(stderr, "Removed %d\n", num);
            removeNode(nodeToDelete);
            free(nodeToDelete->key);
            free(nodeToDelete->value);
            free(nodeToDelete);
        }
    }
}

// Function to free the Cache
void freeCache(Cache *cache) {
    struct Node *current = cache->head->next;
    while (current != cache->tail) {
        struct Node *temp = current;
        current = current->next;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
    free(cache->head);
    free(cache->tail);
    free(cache->cacheMap);
    free(cache);
}

// Function to print the current contents of the cache to a specified stream
void printCache(Cache *cache) {
    if (!cache) {
        fprintf(stderr, "Cache is NULL\n");
        return;
    }

    fprintf(stderr, "Cache Contents (Capacity: %d, Mode: %s):\n", cache->capacity,
        cache->isFIFO ? "FIFO" : "LRU");

    int count = 0;
    struct Node *current = cache->head->next;

    while (current != cache->tail) {
        fprintf(stderr, "[%d] Key: '%s', Value: '%s'\n", count++, current->key, current->value);
        current = current->next;
    }

    if (count == 0) {
        fprintf(stderr, "Cache is empty\n");
    } else {
        fprintf(stderr, "Total items in cache: %d\n", count);
    }

    fprintf(stderr, "-----------------------------------\n");
}
