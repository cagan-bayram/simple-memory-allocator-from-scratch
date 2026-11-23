#include <stdio.h>
#include <pthread.h>
#include <string.h>

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
    // If a sufficiently large free block is found, the block is marked as not-free
    if (header) {
        // In this case, the header will refer to the header part of the block of memory we just found
        // traversing the linked list via get_free_block()
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }
    // Size of header + block = total size
    total_size = sizeof(header_t) + size;
    // If sufficiently large free block is not found
    // we extend the heap by calling sbrk()
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

void free(void *block) {
    header_t *header, *tmp;
    void *programbreak;

    if (!block) return;
    pthread_mutex_lock(&global_malloc_lock);
    // Get the header of the block that we wanna free
    header = (header_t*)block - 1;

    programbreak = sbrk(0); // sbrk(0) gives the current value of the program break
    if ((char*)block + header->s.size == programbreak) {
        if (head == tail) {
            head = tail = NULL;
        } else {
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        // To release the sum of sizes of header and the actual block amount of memo, call sbrk()
        // with negative of the value
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_lock(&global_malloc_lock);
}

// Allocate memory for an array of num elems of nsize bytes each
// and return a pointer to allocated memory. Additionally,
// the memory is all set to zeroes.
void *calloc(size_t num, size_t nsize) {
    size_t size;
    void *block;
    if (!num || !nsize) return NULL;
    size = num * nsize;
    /*check mul overflow*/
    if (nsize != size / num) {
        return NULL;
    }
    block = malloc(size);
    if (!block) {
        return NULL;
    }

    // set to zeroes
    memset(block, 0, size);
    return block;
}

void *realloc(void *block, size_t size) {
    header_t *header;
    void *ret;
    if (!block || !size) return malloc(size);
    header = (header_t*)block - 1;
    // If the current block doesn't have the requested size, we call malloc() 
    // to get a block of the requested size, and relocate contents to the new
    // bigger block using memcpy()
    // The old memory block is then freed.
    if (header->s.size >= size) {
        return block;
    }
    
    ret = malloc(size);
    if (ret) {
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}