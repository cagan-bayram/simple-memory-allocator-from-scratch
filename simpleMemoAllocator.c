#include <stdio.h>
#include <pthread.h>

typedef char ALIGN[16];

union header {
    struct {
        size_t size;
        unsigned is_free;
        struct header_t *next;
    } s;
    ALIGN stub;
};

typedef union header header_t;

// Pointers for the header
header_t *head, *tail;

// Global lock for preventing two or more threads from concurrently accessing memory
pthread_mutex_t global_malloc_lock;

int main() {
    return 0;
}

void *malloc(size_t size) {
    size_t total_size;
    void *block;
    header_t *header;
    if (!size) {
        return NULL;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if (header) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }
    // Size of header + block = total size
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void*) -1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if (!head) {
        head = header;
    }
    if (tail) {
        tail->s.next = header;
    }
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
}

header_t *get_free_block(size_t size) {
    header_t *curr = head;
    while (curr) {
        if (curr->s.is_free && curr->s.size >= size) return curr;
        curr = curr->s.next;
    }
    return NULL;
}