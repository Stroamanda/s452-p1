#include "../src/lab.h"
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h> 
#include <string.h>


  /**
   * Converts bytes to its equivalent K value defined as bytes <= 2^K
   * @param bytes The bytes needed
   * @return K The number of bytes expressed as 2^K
   */
size_t btok(size_t bytes) {
    unsigned int count = 0;

    if (bytes != 1) {
        bytes--;
    }

    while (bytes > 0) {
        bytes >>= 1;
        count++;
    }

    return count - 1;
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
    struct avail *buddyAddress = (struct avail *) ((char *) pool->base + (blockAddress ^ blockSize));

    return buddyAddress; 
}

  /**
   * Allocates a block of size bytes of memory, returning a pointer to
   * the beginning of the block. The content of the newly allocated block
   * of memory is not initialized, remaining with indeterminate values.
   *
   * If size is zero, the return value will be NULL
   * If pool is NULL, the return value will be NULL
   *
   * @param pool The memory pool to alloc from
   * @param size The size of the user requested memory block in bytes
   * @return A pointer to the memory block
   */
void *buddy_malloc(struct buddy_pool *pool, size_t size) {
    if (size == 0 || pool == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // get kval
    size_t kval = btok(size);
    // finds a spot in memory that is at least the size of the kval
    // if not find next larger size until free block
    for (size_t k = kval; k <= pool->kval_m; k++) {
        if (pool->avail[k].tag == BLOCK_AVAIL) {
            printf("Found block at %p\n", (void *)&pool->avail[k]);
            // "remove" from available list so no one else can use it
            pool->avail[k].tag = BLOCK_RESERVED;

            while (k > kval) {
                k--;
                struct avail *buddy = buddy_calc(pool, &pool->avail[k + 1]);
                buddy->kval = k;
                buddy->tag = BLOCK_AVAIL;

                buddy->next = pool->avail[k].next;
                pool->avail[k].next = buddy;
            }
            return (void *)((char *)pool->base + (k << DEFAULT_K));
        }
    }
    // if larger block isn't found, fails
    errno = ENOMEM;
    fprintf(stderr, "No available block found for size %zu\n", size);
    return NULL;
}

  /**
   * A block of memory previously allocated by a call to malloc,
   * calloc or realloc is deallocated, making it available again
   * for further allocations.
   *
   * If ptr does not point to a block of memory allocated with
   * the above functions, it causes undefined behavior.
   *
   * If ptr is a null pointer, the function does nothing.
   * Notice that this function does not change the value of ptr itself,
   * hence it still points to the same (now invalid) location.
   *
   * @param pool The memory pool
   * @param ptr Pointer to the memory block to free
   */
void buddy_free(struct buddy_pool *pool, void *ptr) {
    if (ptr == NULL) {
        return;
    }
    struct avail *block = (struct avail *)ptr;
    size_t blockSize = UINT64_C(1) << block->kval;
    size_t blockAddress = (char *) block - (char *) pool->base;

    // check for buddy size k_val for block at address ptr
    while (block->kval < pool->kval_m) {
        struct avail *buddy = (struct avail *)((char *)pool->base + (blockAddress ^ blockSize));

        // if buddy not available add freed block to kth list
        if (buddy->tag != BLOCK_AVAIL) {
            block->tag = BLOCK_AVAIL;
            pool->avail[block->kval].tag = BLOCK_AVAIL;
            break;
        }

        // merge free block with buddy in kth list, set k = k + 1
        block = (struct avail *) ((char *) pool->base + (blockAddress & ~blockSize));
        block->tag = BLOCK_AVAIL;
        blockAddress = blockAddress & ~blockSize;
        blockSize <<= 1;
        block->kval++;
    }
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

    size_t kval = btok(size);
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
    if (pool == MAP_FAILED) {
        perror("mamp-test: could not allocate memory pool!");
    }

    for (unsigned int i = 0; i <= pool->kval_m; i++) {
        pool->avail[i].next = &pool->avail[i];
        pool->avail[i].prev = &pool->avail[i];
        pool->avail[i].kval = i;
        pool->avail[i].tag = BLOCK_UNUSED;
    }

    pool->avail[pool->kval_m].next = pool->base;
    pool->avail[pool->kval_m].prev = pool->base;
    struct avail *ptr = (struct avail *) pool->base;
    ptr->tag = BLOCK_AVAIL;
    ptr->kval = pool->kval_m;
    ptr->next = &pool->avail[pool->kval_m];
    ptr->prev = &pool->avail[pool->kval_m];

    pool->avail[pool->kval_m].next = ptr;
    pool->avail[pool->kval_m].prev = ptr;
    pool->avail[pool->kval_m].tag = BLOCK_AVAIL;
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