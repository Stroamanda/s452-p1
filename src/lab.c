#include "../src/lab.h"
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> 
#include <string.h>


  /**
   * Converts bytes to its equivalent K value defined as bytes <= 2^K
   * @param bytes The bytes needed
   * @return K The number of bytes expressed as 2^K
   */
size_t btok(size_t bytes) {
    unsigned int count = 0;
    if (bytes == 1) return 0;

    if (bytes != 1) {
        bytes--;
    }

    while (bytes > 0) {
        bytes >>= 1;
        count++;
    }

    return count;
}


/**
 * Find the buddy of a given pointer and kval relative to the base address we got from mmap
 * @param pool The memory pool to work on (needed for the base addresses)
 * @param buddy The memory block that we want to find the buddy for
 * @return A pointer to the buddy
 */
struct avail *buddy_calc(struct buddy_pool *pool, struct avail *buddy) {
    // get size of block with bit shift
    size_t blockSize = UINT64_C(1) << buddy->kval;

    // get block address by subtracting the base address
    size_t blockAddress = (char *) buddy - (char *) pool->base;

    // get the buddy address by using xor operator
    size_t buddyAddress = blockAddress ^ blockSize;

    return (struct avail *)((char *)pool->base + buddyAddress); 
}


void *buddy_malloc(struct buddy_pool *pool, size_t size) {
    if (size == 0 || pool == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    size_t kval = btok(size + sizeof(struct avail));

    // terminate if kval is greater than kval_m
    if (kval > pool->kval_m) {
        errno = ENOMEM;
        return NULL;
    }

    // R1 find block of which available block size of 2^k is not empty
    size_t k = 0;
    struct avail *L = NULL;
    for (k = kval; k <= pool->kval_m; k++) {
        if (pool->avail[k].next != &pool->avail[k]) {
            L = pool->avail[k].next;
            break;
        }
    }

    // if no such k terminate unsuccessfully
    if (L == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // R2 remove from list
    struct avail *P = L->next;
    pool->avail[k].next = P;
    P->prev = &pool->avail[k];
    L->tag = BLOCK_RESERVED;

    // R3 if k == kval then the algorithm skips while loop and returns
    // R4 if k > kval, Split and add blocks to list
    while (k > kval) {
        k--;
        struct avail *P = (struct avail *)((char *)L + (UINT64_C(1) << k));
        P->tag = BLOCK_AVAIL;
        P->kval = k;
        P->next = P->prev = &pool->avail[k];
        pool->avail[k].next = pool->avail[k].prev = P;

        L->kval = k;
    }
    
    return (void *)(L + 1);
}


void buddy_free(struct buddy_pool *pool, void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Error: ptr is NULL\n");
        return;
    }
    struct avail *L = (struct avail *)ptr - 1;
    // check for buddy size k_val for block at address ptr
    while (L->kval < pool->kval_m) {
        // S1 is buddy available?
        struct avail *P = buddy_calc(pool, L);
        if (P->tag == BLOCK_RESERVED ||( P->tag == BLOCK_AVAIL && P->kval != L->kval)) {
            break;
        }
        // S2 combine with buddy and remove block P from list
        P->prev->next = P->next;
        P->next->prev = P->prev;
        
        L->kval++;
        if (P < L) {
            L = P;
        }
    }

    // add back to list
    L->tag = BLOCK_AVAIL;
    L->next = L->prev = &pool->avail[L->kval];
    pool->avail[L->kval].next = pool->avail[L->kval].prev = L;
}



  /**
   * Changes the size of the memory block pointed to by ptr.
   * The function may move the memory block to a new location
   * (whose address is returned by the function).
   * The content of the memory block is preserved up to the
   * lesser of the new and old sizes, even if the block is
   * moved to a new location. If the new size is larger,
   * the value of the newly allocated portion is indeterminate.
   *
   * In case that ptr is a null pointer, the function behaves
   * like malloc, assigning a new block of size bytes and
   * returning a pointer to its beginning.
   *
   * if size is equal to zero, and ptr is not NULL, then the  call
   * is equivalent to free(ptr)
   *
   * @param pool The memory pool
   * @param ptr Pointer to a memory block
   * @param size The new size of the memory block
   * @return Pointer to the new memory block
   */
void *buddy_realloc(struct buddy_pool *pool, void *ptr, size_t size) {
    if (size == 0 && ptr != NULL) {
        buddy_free(pool, ptr);
        return NULL;
    } else if (ptr == NULL) {
        return buddy_malloc(pool, size);
    } 

    size_t kval = btok(size) - 1;
    struct avail *block = (struct avail *) ptr;

    // if the same size no need to reallocate 
    if (kval == block->kval) {
        return ptr;
    }

    // allocate a new block for new size
    void *new_block = buddy_malloc(pool, size);
    if (new_block == NULL) {
        return NULL;
    }

    // Copy the data
    size_t old_block_size = 1 << block->kval;
    size_t new_block_size = 1 << kval;
    size_t data_size = old_block_size < new_block_size ? old_block_size : new_block_size;

    memcpy(new_block, ptr, data_size);

    // Free the old block
    buddy_free(pool, ptr);

    return new_block;
}


void buddy_init(struct buddy_pool *pool, size_t size) {
    if (size == 0) {
        size = UINT64_C(1) << DEFAULT_K;
    }
    pool->kval_m = btok(size);
    pool->numbytes = UINT64_C(1) << pool->kval_m;

    pool->base = mmap(NULL, pool->numbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pool->base == MAP_FAILED) {
        perror("mamp-test: could not allocate memory pool!");
    }

    for (unsigned int i = 0; i <= pool->kval_m; i++) {
        pool->avail[i].next = &pool->avail[i];
        pool->avail[i].prev = &pool->avail[i];
        pool->avail[i].kval = i;
        pool->avail[i].tag = BLOCK_UNUSED;
    }

    struct avail *ptr = (struct avail *) pool->base;
    ptr->tag = BLOCK_AVAIL;
    ptr->kval = pool->kval_m;
    ptr->next = &pool->avail[pool->kval_m];
    ptr->prev = &pool->avail[pool->kval_m];

    pool->avail[pool->kval_m].next = ptr;
    pool->avail[pool->kval_m].prev = ptr;

}

  /**
   * Inverse of buddy_init.
   *
   * Notice that this function does not change the value of pool itself,
   * hence it still points to the same (now invalid) location.
   *
   * @param pool The memory pool to destroy
   */
void buddy_destroy(struct buddy_pool *pool) {
    int status = munmap(pool->base, pool->numbytes);

    if (status == -1) {
        perror("buddy: destory failed!");
    }
}

// int myMain(int argc, char** argv) {

// }