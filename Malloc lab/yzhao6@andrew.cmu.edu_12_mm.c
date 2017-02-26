/*
 * mm.c
 * 
 * Yue Zhao (yzhao6)
 * I use 15 segregate lists to store free blocks in different size
 * range. I revise my malloc based on my checkpoint program,
 * my target is to improve the utility of my malloc. Compared to checkpoint
 * version, I have finished the following revisements.
 * 1) Delete footer in allocated blocks
 * 2) Use a single linkedlist for 16-byte blocks
 * 3) add two more flag bit: 
 *    -second lowest bit: if prev block allocated/not
 *    -third lowest bit: if prev block is a 16-byte block (minimum size block)
 * More specific description of my molloc is as below.
 ******************************************************************************
 *                                mm.c                                        *
 *           64-bit struct-based segregate free list memory allocator         *
 *                  18-600: Foundation of Computor Systems (Lab 7)            *
 *                 The documentation is revised from mm-baseline.c.           * 
 *                                                                            *
 *  ************************************************************************  *
 *                               DOCUMENTATION                                *
 *                                                                            *
 *  ** STRUCTURE. **                                                          *
 *                                                                            *
 *  Both allocated and free blocks share the same header structure.           *
 *  HEADER: 8-byte, aligned to 8th byte of an 16-byte aligned heap, where     *
 *          - The lowest order bit is 1 when the block is allocated, and      *
 *            0 otherwise.                                                    *
 *          - The second lowest order bit is 1 when the previous block on     *
 *            the heap is allocated, and 0 otherwise.                         *
 *          - The third lowest order bit is 1 when the previous block has a   *
 *            minimum block size (16 bytes).                                  *
 *          - The whole 8-byte value with the least 3 significant bits set to *
 *            0 represents the size of the block as a size_t                  *
 *            The size of a block includes the header and footer.             *
 *  The minimum blocksize is 16 bytes.                                        *
 *                                                                            *
 *  Allocated blocks contain the following:                                   *
 *  HEADER, as defined above.                                                 *
 *  PAYLOAD: Memory allocated for program to store information.               *
 *  The size of an allocated block is exactly PAYLOAD + HEADER.               *
 *                                                                            *
 *  Free blocks (size = 16 bytes) contain the following:                      *
 *  HEADER, as defined above.                                                 *
 *  NEXT POINTER, point to the next free block.                               *
 *  Free blocks with size of 16 bytes forms a circulate single linkedlist.    *
 *                                                                            *
 *  Free blocks (size >= 32 bytes) contain the following:                     *
 *  HEADER, as defined above.                                                 *
 *  NEXT POINTER, point to the next free block.                               *
 *  PREV POINTER, point to the prev free block.                               *
 *  FOOTER: 8-byte, aligned to 0th byte of an 16-byte aligned heap. It        *
 *          contains the exact copy of the block's header.                    *
 *  Free blocks larger than 16 bytes forms a circulate double linkedlist.     *
 *                                                                            *
 *  Block Visualization.                                                      *
 *                    block     block+8          block+size-8   block+size    *
 *  Allocated blocks:   |  HEADER  |  ... PAYLOAD ...  |  FOOTER  |           *
 *                                                                            *
 *                            block     block+8   block+size                  *
 *  Unallocated 16-byte blocks: |  HEADER  |   NEXT  |                        *
 *                                                                            *
 *                    block     block+8  block+16  block+size-8 block+size    *
 *  Unalloc (>16) blocks: |  HEADER  |   NEXT  |  PREV   |  FOOTER  |         *
 *                                                                            *
 *  ************************************************************************  *
 *  ** INITIALIZATION. **                                                     *
 *                                                                            *
 *  The following visualization reflects the beginning of the heap.           *
 *      start            start+8           start+16                           *
 *  INIT: | PROLOGUE_FOOTER | EPILOGUE_HEADER |                               *
 *  PROLOGUE_FOOTER: 8-byte footer, as defined above, that simulates the      *
 *                    end of an allocated block. Also serves as padding.      *
 *  EPILOGUE_HEADER: 8-byte block indicating the end of the heap.             *
 *                   It simulates the beginning of an allocated block         *
 *                   The epilogue header is moved when the heap is extended.  *
 *                                                                            *
 *  ************************************************************************  *
 *  ** BLOCK ALLOCATION. **                                                   *
 *                                                                            *
 *  Upon memory request of size S, a block of size S + wsize, rounded up to   *
 *  16 bytes, is allocated on the heap, where wsize is 8.                     *
 *  Selecting the block for allocation is performed based on search depth.    *
 *  Find a better fit in the first search_depth number of free blocks.        *
 *  Based on the size of needed free block, calculte the start index of free  *
 *  list. The search starts from the root of free list based on the size of   *
 *  needed block. It sequentially goes through each block in the segregate    *
 *  free list, until either                                                   *
 *  - A better unallocated block is found, or                                 *
 *  - The end of segregate free lists is reached, which occurs                *
 *    when no sufficiently-large unallocated block is available.              *
 *  In case that a sufficiently-large unallocated block is found, then        *
 *  that block will be used for allocation. Otherwise--that is, when no       *
 *  sufficiently-large unallocated block is found--then more unallocated      *
 *  memory of size chunksize or requested size, whichever is larger, is       *
 *  requested through mem_sbrk, and the search is redone.                     *
 *  After a block is freed, it would be add to a free list based on its size. *
 *                                                                            *
 *  ************************************************************************  *
 *  ** For Final Version **                                                   *
 *  Hardest and most interesting lab!                                         *
 *  It took me many hours to debug it and improve its performance,            *
 *  Even feel desperate sometimes but have fun after all!                     *
 *                                                                            *
 ******************************************************************************
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want debugging output, uncomment the following.  Be sure not
 * to have debugging enabled in your final submission
 */
//#define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__) 
#define dbg_checkheap(...) mm_checkheap(__VA_ARGS__)
#define dbg_printheap(...) print_heap(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#define dbg_requires(...)
#define dbg_ensures(...)
#define dbg_checkheap(...)
#define dbg_printheap(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* def DRIVER */

/* Basic constants */
typedef uint64_t word_t;
static const size_t wsize = sizeof(word_t);   // word and header size (bytes)
static const size_t dsize = 2*wsize;          // double word size (bytes)
static const size_t min_block_size = dsize; // Minimum block size (16 bytes)
static const size_t chunksize = (1 << 12);    // requires (chunksize % 16 == 0)

static const word_t alloc_mask = 0x1;
static const word_t size_mask = ~(word_t)0xF;

static const word_t alloc_prev_mask = 0x2;
static const word_t min_prev_mask = 0x4;
static const int search_depth = 10;         // Search depth in find fit
/* Number of free list with same size block. */
static const int constant_block_num = 4;

typedef struct block block_t;
struct block
{
    /* Header contains size + allocation flag */
    word_t header;
    /* Define a union to store different status of a block */
    union payload
    {
        /* Sub struct for prev and next points */
        struct ptrs
        {
            block_t* next;
            block_t* prev;
        } ptrs;
        /*
         * We don't know how big the payload will be.  Declaring it as an
         * array of size 0 allows computing its starting address using
         * pointer notation.
         */
        char payload[0];
    } payload;
    /*
     * We can't declare the footer as part of the struct, since its starting
     * position is unknown
     */
};
/* What is the correct alignment? */
#define ALIGNMENT 16
/* number of free list */
#define LIST_COUNTER 15

/* Function prototypes for internal helper routines */


/* Global variables (128 bytes) */
/* Pointer to first block */
static block_t *heap_listp = NULL;
/* Pointers to free list */
static block_t *free_root[LIST_COUNTER];


/* Function prototypes for internal helper routines */
static size_t align(size_t x);

static block_t *extend_heap(size_t size);
static void place(block_t *block, size_t asize);
static block_t *find_fit(size_t asize);
static block_t *coalesce(block_t *block);

static size_t max(size_t x, size_t y);
static size_t round_up(size_t size, size_t n);
static word_t pack(size_t size, bool alloc);

static word_t prev_pack(size_t size, bool prev_alloc);

static size_t extract_size(word_t header);
static size_t get_size(block_t *block);
static size_t get_payload_size(block_t *block);

static bool extract_alloc(word_t header);
static bool get_alloc(block_t *block);

static bool extract_prev_alloc(word_t header);
static bool get_prev_alloc(block_t *block);

static bool extract_prev_min(word_t word);
static bool get_prev_min(block_t *block);

static void write_header(block_t *block, size_t size, bool alloc, 
    bool prev_alloc, bool prev_min);
static void write_footer(block_t *block, size_t size, bool alloc);
static void write_header_prev_alloc(block_t *block, bool prev_alloc);
static void write_header_prev_min(block_t *block, bool prev_min);

static block_t *payload_to_header(void *bp);
static void *header_to_payload(block_t *block);

static block_t *find_next(block_t *block);
static word_t *find_prev_footer(block_t *block);
static block_t *find_prev(block_t *block);

static void add_free(block_t *block);
static void delete_free(block_t *block);
static int get_list_index(size_t size);

bool mm_checkheap(int lineno);

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x) 
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/*
 * Initialize: return false on error, true on success.
 */
bool mm_init(void) 
{
    // Create the initial empty heap 
    word_t *start = (word_t *)(mem_sbrk(2*wsize));

    if (start == (void *)-1) //not a valid address
    {
        return false;
    }
    start[0] = pack(0, true); // Prologue footer
    start[1] = pack(0, true); // Epilogue header, set the penultimate bit
    start[1] = prev_pack(0, true);

    // Heap starts with first block header (epilogue)
    heap_listp = (block_t *) &(start[1]);
    int i;
    for (i = 0; i < LIST_COUNTER; i++)
    {
        free_root[i] = NULL;
    }    

    // Extend the empty heap with a free block of chunksize bytes
    if (extend_heap(chunksize) == NULL)
    {
        return false;
    }
    return true;
}

/*
 * malloc
 */
void *malloc (size_t size) 
{
    dbg_printf("->malloc\n");
    dbg_requires(mm_checkheap);
    
    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit is found
    block_t *block;
    void *bp = NULL;

    if (heap_listp == NULL) // Initialize heap if it isn't initialized
    {
        mm_init();
    }

    if (size == 0) // Ignore spurious request
    {
        dbg_ensures(mm_checkheap);
        return bp;
    }

    // Adjust block size to include overhead and to meet alignment requirements
    asize = round_up(size + wsize, dsize);

    // Search the free list for a fit
    block = find_fit(asize);

    // If no fit is found, request more memory, and then and place the block
    if (block == NULL)
    {  
        extendsize = max(asize, chunksize);
        block = extend_heap(extendsize);
        if (block == NULL) // extend_heap returns an error
        {
            return bp;
        }

    }
    delete_free(block);
    place(block, asize);
    bp = header_to_payload(block);

    dbg_printf("Malloc size %zd on address %p.\n", size, bp);
    dbg_ensures(mm_checkheap);
    dbg_checkheap(__LINE__);
    return bp;
}

/*
 * free
 */
void free (void *ptr) 
{
    dbg_printf("->free\n");
    if (ptr == NULL)
    {
        return;
    }
    block_t *block = payload_to_header(ptr);
    size_t size = get_size(block);
    write_header(block, size, false, get_prev_alloc(block), 
        get_prev_min(block));
    write_footer(block, size, false);
   
    block = coalesce(block);
    write_header_prev_alloc(find_next(block), false);
    write_header_prev_min(find_next(block), 
        (bool)(get_size(block) == min_block_size));  
    
}

/*
 * realloc
 */
void *realloc(void *oldptr, size_t size) 
{
    block_t *block = payload_to_header(oldptr);
    size_t copysize;
    void *newptr;

    // If size == 0, then free block and return NULL
    if (size == 0)
    {
        free(oldptr);
        return NULL;
    }

    // If oldptr is NULL, then equivalent to malloc
    if (oldptr == NULL)
    {
        return malloc(size);
    }

    // Otherwise, proceed with reallocation
    newptr = malloc(size);
    // If malloc fails, the original block is left untouched
    if (!newptr)
    {
        return NULL;
    }

    // Copy the old data
    copysize = get_payload_size(block); // gets size of old payload
    if(size < copysize)
    {
        copysize = size;
    }
    memcpy(newptr, oldptr, copysize);

    // Free the old block
    free(oldptr);

    return newptr;
}

/*
 * calloc
 * This function is not tested by mdriver
 */
void *calloc (size_t nmemb, size_t size) 
{
    void *bp;
    size_t asize = nmemb * size;

    if (asize/nmemb != size)
    // Multiplication overflowed
    return NULL;
    
    bp = malloc(asize);
    if (bp == NULL)
    {
        return NULL;
    }
    // Initialize all bits to 0
    memset(bp, 0, asize);

    return bp;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void *p) 
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void *p) 
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno) 
{
    int i, j;
    /* Array to store number of free blocks in different size, initial is 0 */
    int count_free[LIST_COUNTER];
    for (i = 0; i < LIST_COUNTER; i++)
    {
        count_free[i] = 0;
    }    
    /* Check the start of heap */
    if ((unsigned long)heap_listp != (unsigned long)0x800000008)
    {
        dbg_printf("heap_listp wrrong: %p\n", heap_listp);
        return false;
    }
    /* Check Prologue footer */
    block_t *prologue_footer = (block_t *)((char *)heap_listp - 8);
    if (get_size(prologue_footer) != 0 || !get_alloc(prologue_footer))
    {
        dbg_printf("Prologue_footer wring: size:%zu, alloc:%d\n",
            get_size(prologue_footer), get_alloc(prologue_footer));
        return false;
    }
    /* Check the heap */
    block_t *block = heap_listp;
    block_t *next_block = find_next(heap_listp);
    while(block != next_block)
    {
        /* Count number of free blocks in each free list */
        if (!get_alloc(block))
        {
            count_free[get_list_index(get_size(block))]++;
        }
        /* Check alignment */
        if (!align((unsigned long)block - 8))
        {
            dbg_printf("Alignment wrong: %p\n", block);
            return false;
        }
        /* Check if there are two consecutive free block on the heap.
         * (check alloc bit in header). 
         */
        else if (get_alloc(block) == false && get_alloc(next_block) == false)
        {
            dbg_printf("free block not coalesced: %p, %p\n", block,
                next_block);
            return false;
        }
        /* Check prev_alloc bit in header */
        else if (get_prev_alloc(next_block) != get_alloc(block))
        {
            dbg_printf("prev_alloc bit wrong: %p, %p\n", block,
                next_block);
            return false;
        }
        /* Check prev_min bit in header*/
        else if (get_prev_min(next_block) != 
            (get_size(block) == min_block_size))
        {
            dbg_printf("prev_min bit wrong: %p, %p\n", block,
                next_block);
            return false;
        }    
        /* Check header and footer for empty blocks (size >= 32) */
        else if (!get_alloc(block) && !get_prev_min(next_block)
            && *find_prev_footer(next_block) !=
            pack(get_size(block), (unsigned long)get_alloc(block)))
        {
            dbg_printf("header != footer: %p, header: %zu, footer: %zu\n",
                block, pack(get_size(block), (unsigned long)get_alloc(block)),
                *find_prev_footer(next_block));
            return false;
        }
        /* Check the end of heap */
        else {
            block = find_next(block);
            next_block = find_next(next_block);
            if ((unsigned long)next_block > (unsigned long)mem_heap_hi())
            {
                dbg_printf("Block exceed heap: %p\n", next_block);
                return false;
            } 
        }
        
    }
    /* Check free list */

    /* count_free[] is the result of counting free blocks by iterating through 
     * every block Now traversing free list by pointers and see if they match.
     * Also Check if blocks in free lists are free.
     */
    for (i = 0; i < LIST_COUNTER; i++)
    { 
        block_t *curr = free_root[i];
        for (j = 0; j < count_free[i]; j++)
        {
            if (!get_alloc(block))
            {
                dbg_printf("%p is not a free block!\n", curr);
                return false;
            }    
            curr = curr->payload.ptrs.next;
        }
        if (curr != free_root[i])
        {
            dbg_printf("Free list %d has wrong block number, should be %d.\n",
                i, count_free[i]);
            return false;
        }
    }
    /* Check if all next/previous pointers are consistent in free lists
     * (size >= 32).
     */
    for (i = 1; i < LIST_COUNTER; i++)
    {
        block_t *curr = free_root[i];
        for (j = 0; j < count_free[i]; j++)
        {
            if (curr->payload.ptrs.next->payload.ptrs.prev != curr || 
                curr->payload.ptrs.prev->payload.ptrs.next != curr)
            {
                dbg_printf("prev: %p, next: %p are not consistent.\n", 
                curr->payload.ptrs.prev, curr->payload.ptrs.next);
                return false;
            }
            curr = curr->payload.ptrs.next;
        }
    }
    /* Check if all free list pointers are between mem_heap_lo()
     * and mem_heap_high(). 
     */
    for (i = 0; i < LIST_COUNTER; i++)
    {
        block_t *curr = free_root[i];
        if (i == 0)
        {
            for (j = 0; j < count_free[i]; j++)
            {
                if ((unsigned long)curr->payload.ptrs.next > 
                    (unsigned long)mem_heap_hi())
                {
                    dbg_printf("prev: %p, next: %p exceed boundary.\n", 
                    curr->payload.ptrs.prev, curr->payload.ptrs.next);
                    return false;
                }
                curr = curr->payload.ptrs.next;
            }
        }
        else
        { 
            for (j = 0; j < count_free[i]; j++)
            {
                if ((unsigned long)curr->payload.ptrs.next > 
                    (unsigned long)mem_heap_hi() 
                    || (unsigned long)curr->payload.ptrs.prev< 
                    (unsigned long)mem_heap_lo())
                {
                    dbg_printf("prev: %p, next: %p exceed boundary.\n", 
                    curr->payload.ptrs.prev, curr->payload.ptrs.next);
                    return false;
                }
                curr = curr->payload.ptrs.next;
            }
        }
    }
    /* Check if all blocks in each list bucket fall within bucket size range.
     * (segregated list).
     */
    for (i = 0; i < LIST_COUNTER; i++)
    {
        block_t *curr = free_root[i];
        if (i < constant_block_num)
        {
            for (j = 0; j < count_free[i]; j++)
            {
                if (get_size(curr) != min_block_size * (i + 1))
                {
                    dbg_printf("Block %p in a wrong free list %d.\n", curr, i);
                    return false;
                }
                curr = curr->payload.ptrs.next;
            }
        }
        else
        { 
            for (j = 0; j < count_free[i]; j++)
            {
                if (get_size(curr) > 1<<(i + constant_block_num - 1) ||
                    get_size(curr) <= 1<<(i + constant_block_num - 2))
                {
                    dbg_printf("Block %p in a wrong free list %d.\n", curr, i);
                    return false;
                }
                curr = curr->payload.ptrs.next;
            }
        }
    }
    return true;
}



/*
 * pack: returns a header reflecting a specified size and its alloc status.
 *       If the block is allocated, the lowest bit is set to 1, and 0 
 *       otherwise.
 */
static word_t pack(size_t size, bool alloc)
{
    return alloc ? (size | 1) : size;
}

/*
 * prev_pack: returns a header reflecting a specified size and its prev block's
 *            alloc status. If the prev block is allocated, the penultimate
 *           lowest bit is set to 1, and 0 otherwise.
 */
static word_t prev_pack(size_t size, bool prev_alloc)
{
    if (prev_alloc == true)
    {
        return size | alloc_prev_mask;
    }
    else
    {
        return size & (~alloc_prev_mask);
    }
}

/*
 * extend_heap: Extends the heap with the requested number of bytes, and
 *              recreates epilogue header. Returns a pointer to the result of
 *              coalescing the newly-created block with previous free block, if
 *              applicable, or NULL in failure.
 */
static block_t *extend_heap(size_t size) 
{
    void *bp;
    // Allocate an even number of words to maintain alignment
    size = round_up(size, dsize);
    if ((bp = mem_sbrk(size)) == (void *)-1)
    {
        return NULL;
    }
    
    // Initialize free block header/footer 
    block_t *block = payload_to_header(bp);
    write_header(block, size, false, get_prev_alloc(block),
        get_prev_min(block));
    write_footer(block, size, false);

    // Create new epilogue header
    block_t *block_next = find_next(block);
    write_header(block_next, 0, true, false, false);
    // Coalesce in case the previous block was free
    return coalesce(block);
}


/*
 * round_up: Rounds size up to next multiple of n
 */
static size_t round_up(size_t size, size_t n)
{
    return n * ((size + (n-1)) / n);
}

/*
 * payload_to_header: given a payload pointer, returns a pointer to the
 *                    corresponding block.
 */
static block_t *payload_to_header(void *bp)
{
    return (block_t *)(((char *)bp) - offsetof(block_t, payload));
}

/*
 * write_header: given a block and its size and allocation status,
 *               writes an appropriate value to the block header.
 */
static void write_header(block_t *block, size_t size, bool alloc, 
    bool prev_alloc, bool prev_min)
{
    block->header = pack(size, alloc);
    write_header_prev_alloc(block, prev_alloc);
    write_header_prev_min(block, prev_min);
}

/*
 * write_footer: given a block and its size and allocation status,
 *               writes an appropriate value to the block footer by first
 *               computing the position of the footer.
 */
static void write_footer(block_t *block, size_t size, bool alloc)
{
    word_t *footerp = (word_t *)((block->payload.payload) 
        + get_size(block) - dsize);
    *footerp = pack(size, alloc);
}
/* add_free: Add a free block to a free list based on its size,
 *           change pointers.
 */
static void add_free(block_t *block)
{
    int list_index = get_list_index(get_size(block));
    dbg_printf("add to: %d\n", list_index);
    /* Special case for the first free list (single linked list). */
    if (list_index == 0)
    {
        /* List is empty */
        if (free_root[list_index] == NULL)
        {
            free_root[list_index] = block;
            block->payload.ptrs.next = block;
        }
        /* Add after root block, update root */
        else
        {
            block->payload.ptrs.next = free_root[list_index]->payload.ptrs.next;
            free_root[list_index]->payload.ptrs.next = block;
            free_root[list_index] = block;
        }
        return;    
    }
    /* list is empty, set up root */
    if (free_root[list_index] == NULL)
    {
        free_root[list_index] = block;
        block->payload.ptrs.prev = block;
        block->payload.ptrs.next = block;
    }
    /* Add after root block, update root */
    else
    {
        block_t* root_prev = free_root[list_index]->payload.ptrs.prev;
        root_prev->payload.ptrs.next = block;
        block->payload.ptrs.prev = root_prev;
        block->payload.ptrs.next = free_root[list_index];
        free_root[list_index]->payload.ptrs.prev = block;
        free_root[list_index] = block;
    }
}


/* get_list_index: find the index of a block based on its size */
static int get_list_index(size_t size)
{
    if (size == 16)
    {
        return 0;
    }
    else if (size == 32)
    {
        return 1;
    }
    else if (size == 48)
    {
        return 2;
    }
    else if (size == 64)
    {
        return 3;
    }
    else if (size <= 128)
    {
        return 4;
    }
    else if (size <= 256)
    {
        return 5;
    } 
    else if (size <= 512)
    {
        return 6;
    }
    else if (size <= 1024)
    {
        return 7;
    }
    else if (size <= 2048)
    {
        return 8;
    }
    else if (size <= 4096)
    {
        return 9;
    }
    else if (size <= 8192)
    {
        return 10;
    }
    else if (size <= 16384)
    {
        return 11;
    }
    else if (size <= 32768)
    {
        return 12;
    }
    else if (size <= 65536)
    {
        return 13;
    }    
    else
    {
        return 14;
    }
}


/* Coalesce: Coalesces current block with previous and next blocks if either
 *           or both are unallocated; otherwise the block is not modified.
 *           Returns pointer to the coalesced block. After coalescing, the
 *           immediate contiguous previous and next blocks must be allocated.
 */
static block_t *coalesce(block_t * block) 
{
    block_t *block_next = find_next(block);
    bool next_alloc = get_alloc(block_next);
    bool prev_alloc = get_prev_alloc(block);  
    
    size_t size = get_size(block);
    /* Case 1: one more free block to free list*/
    if (prev_alloc && next_alloc)              
    {
        dbg_printf("Coalesce as case 1\n");
        add_free(block);
    }
    /* Case 2: delete next, add one */
    else if (prev_alloc && !next_alloc)        
    {
        dbg_printf("Coalesce as case 2\n");
        delete_free(block_next);
        size += get_size(block_next);
        write_header(block, size, false, get_prev_alloc(block),
            get_prev_min(block));
        write_footer(block, size, false);
        add_free(block);
    }
    /* Case 3: delete prev, add one */
    else if (!prev_alloc && next_alloc)        
    {
        dbg_printf("Coalesce as case 3\n");
        block_t *block_prev = find_prev(block);
        delete_free(block_prev);
        size += get_size(block_prev);
        write_header(block_prev, size, false, get_prev_alloc(block_prev),
            get_prev_min(block_prev));
        write_footer(block_prev, size, false);
        block = block_prev;
        add_free(block);
    }
    /* Case 4: delete prev and next, add one */
    else                                        
    {
        dbg_printf("Coalesce as case 4\n");
        block_t *block_prev = find_prev(block);
        delete_free(block_prev);
        delete_free(block_next);
        size += get_size(block_next) + get_size(block_prev);
        write_header(block_prev, size, false, get_prev_alloc(block_prev),
            get_prev_min(block_prev));
        write_footer(block_prev, size, false);   
        block = block_prev;
        add_free(block);
    }
    return block;
}


/*
 * find_next: returns the next consecutive block on the heap by adding the
 *            size of the block.
 */
static block_t *find_next(block_t *block)
{
    dbg_requires(block != NULL);
    block_t *block_next = (block_t *)(((char *)block) + get_size(block));

    dbg_ensures(block_next != NULL);
    return block_next;
}
/*
 * find_prev: returns the previous block position by checking the previous
 *            block's footer and calculating the start of the previous block
 *            based on its size.
 */
static block_t *find_prev(block_t *block)
{
    if (get_prev_min(block) == true)
    {
        return (block_t *)(((char *)block) - dsize);
    }    
    word_t *footerp = find_prev_footer(block);
    size_t size = extract_size(*footerp);
    return (block_t *)((char *)block - size);
}

/*
 * get_size: returns the size of a given block by clearing the lowest 4 bits
 *           (as the heap is 16-byte aligned).
 */
static size_t get_size(block_t *block)
{
    return extract_size(block->header);
}

/*
 * extract_size: returns the size of a given header value based on the header
 *               specification above.
 */
static size_t extract_size(word_t word)
{
    return (word & size_mask);
}
/*
 * find_prev_footer: returns the footer of the previous block.
 */
static word_t *find_prev_footer(block_t *block)
{
    // Compute previous footer position as one word before the header
    return (&(block->header)) - 1;
}

/*
 * extract_alloc: returns the allocation status of a given header value based
 *                on the header specification above.
 */
static bool extract_alloc(word_t word)
{
    return (bool)(word & alloc_mask);
}

/*
 * extract_prev_alloc: returns the allocation status of a prev block value based
 *                on the header specification above.
 */
static bool extract_prev_alloc(word_t word)
{
    return (bool)(word & alloc_prev_mask);
}

/*
 * find_fit: Looks for a free block with at least asize bytes with
 *           better-fit policy. Find out a better one from 'search_depth'
 *           number of blocks. 
 *           For free lists with only same size blocks, use first-fit.
 *           Returns NULL if none is found.
 */
static block_t *find_fit(size_t asize)
{
    int min_index = get_list_index(asize);
    dbg_printf("find fit in: %d\n",min_index);
    int count = search_depth;
    block_t *res = NULL;

    int i;
    for (i = min_index; i < LIST_COUNTER; i++)
    {
           
        if (free_root[i] == NULL) {
            continue;
        }
        if (!(get_alloc(free_root[i])) && (asize <= get_size(free_root[i])))
        {
            count--;
            if (res == NULL || get_size(free_root[i]) < get_size(res)) 
            {

                res = free_root[i];
                if (min_index < constant_block_num) {
                    return res;
                }
            }    
            if (count == 0)
            {
                return res;
            }    
        }

        block_t *block = free_root[i]->payload.ptrs.next;
        while (block != free_root[i])
        {    
            if (!(get_alloc(block)) && (asize <= get_size(block)))
            {
                count--;
                if (res == NULL || get_size(block) < get_size(res)) 
                {
                    res = block;
                }
                if (count == 0)
                {
                    return res;
                }    
            }
            block = block->payload.ptrs.next;
        }
    }  
    return res;
}

/*
 * get_alloc: returns true when the block is allocated based on the
 *            block header's lowest bit, and false otherwise.
 */
static bool get_alloc(block_t *block)
{
    return extract_alloc(block->header);
}

/*
 * get_prev_alloc: returns true when the prev block is allocated based on the
 *            block header's penultimate lowest bit, and false otherwise.
 */
static bool get_prev_alloc(block_t *block)
{
    return extract_prev_alloc(block->header);
}

/*
 * place: Places block with size of asize at the start of bp. If the remaining
 *        size is at least the minimum block size, then split the block to the
 *        the allocated block and the remaining block as free, which is then
 *        inserted into the segregated list. Requires that the block is
 *        initially unallocated.
 */
static void place(block_t *block, size_t asize)
{
    size_t csize = get_size(block);

    if ((csize - asize) >= min_block_size)
    {
        block_t *block_next;
        write_header(block, asize, true, get_prev_alloc(block), 
            get_prev_min(block));
        /* Revise header and footer for next remaining free block */
        block_next = find_next(block);    
        write_header(block_next, csize-asize, false, true, 
            (bool)(get_size(block) == min_block_size));
        write_footer(block_next, csize-asize, false);
        block_next = coalesce(block_next);
        write_header_prev_min(find_next(block_next), 
            (bool)(get_size(block_next) == min_block_size));
    }

    else
    { 
        write_header(block, csize, true, get_prev_alloc(block), 
            get_prev_min(block));
        write_header_prev_alloc(find_next(block), true);
    }
}

/* delete_free: Remove block from free list */
static void delete_free(block_t *block)
{
    int list_index = get_list_index(get_size(block));
    dbg_printf("delete from: %d\n", list_index);
    if (list_index == 0)
    {
        
        if (block->payload.ptrs.next == block)
        {
            free_root[list_index] = NULL;
        }
        /* Traverse the list to find prev free block */
        else
        {
            block_t *prev = free_root[list_index];
            while (prev->payload.ptrs.next != block)
            {
                prev = prev->payload.ptrs.next;
            }
            prev->payload.ptrs.next = block->payload.ptrs.next;
            if (block == free_root[list_index]) {
                free_root[list_index] = block->payload.ptrs.next;
            }
        }  
        return;
    }    
    block_t *prev = block->payload.ptrs.prev;
    block_t *next = block->payload.ptrs.next;
    /* Only one node in free list */
    if (block->payload.ptrs.next == block)
    {
        free_root[list_index] = NULL;
    }
    /* More than one node in free list, if remove root */
    else if (block == free_root[list_index])
    {
        free_root[list_index] = free_root[list_index]->payload.ptrs.next;
        prev->payload.ptrs.next = next;
        next->payload.ptrs.prev = prev;
    }
    /* Remove normal node */
    else
    {
        prev->payload.ptrs.next = next;
        next->payload.ptrs.prev = prev;
    } 
}

/*
 * max: returns x if x > y, and y otherwise.
 */
static size_t max(size_t x, size_t y)
{
    return (x > y) ? x : y;
}

/*
 * header_to_payload: given a block pointer, returns a pointer to the
 *                    corresponding payload.
 */
static void *header_to_payload(block_t *block)
{
    return (void *)(block->payload.payload);
}
/*
 * get_payload_size: returns the payload size of a given block, equal to
 *                   the entire block size minus the header and footer sizes.
 */
static word_t get_payload_size(block_t *block)
{
    size_t asize = get_size(block);
    return asize - wsize;
}
/*
 * write_header_prev_alloc: wirte second last bit of header,
 *                   prev_alloc is the status of prev block.
 */
static void write_header_prev_alloc(block_t *block, bool prev_alloc)
{
    if (prev_alloc == true)
    {    
        block->header = (block->header) | alloc_prev_mask;
    }    
    else
    {    
        block->header = (block->header) & (~alloc_prev_mask);
    }    
}
/*
 * get_prev_min: returns true when the prev block is 16 byte based on the
 *            block header's third lowest bit, and false otherwise.
 */
static bool get_prev_min(block_t *block)
{
    return extract_prev_min(block->header);
}
/*
 * extract_prev_min: returns if prev block is 16 bytes based
 *                on the header specification above.
 */
static bool extract_prev_min(word_t word)
{
    return (bool)(word & min_prev_mask);
}
/*
 * write_header_prev_min: wirte third lowest bit of header,
 *                   prev_min is if prev block if 16 btye.
 */
static void write_header_prev_min(block_t *block, bool prev_min)
{
    if (prev_min == true)
    {    
        block->header = (block->header) | min_prev_mask;
    }    
    else
    {    
        block->header = (block->header) & (~min_prev_mask);
    }    
}