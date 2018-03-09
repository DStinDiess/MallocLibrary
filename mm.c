/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Hooli",
    /* First member's full name */
    "Daniel Stinson-Diess",
    /* First member's email address */
    "dstin002@ucr.edu",
    /* Second member's full name (leave blank if none) */
    "Kennen Derenard",
    /* Second member's email address (leave blank if none) */
    "kdere004@ucr.edu"
};

/* single word (4) or double word (8) alignment */
#define WSIZE			4
#define DSIZE			8
#define ALIGNMENT		DSIZE
#define CHUNKSIZE		(1<<12)
#define MINBLK			(3*DSIZE)

/* MAX helper macro */
#define MAX(x, y)		((x) > (y) ? (x) : (y))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size)		(((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_ALIGN    (ALIGN(sizeof(size_t)))

#define PACK(size, alloc)	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)			(*(unsigned int *)(p))
#define SET_INT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~0x1)
#define IS_ALLOC(p)     (GET(p) & 0x1)

/* Given a block ptr bp, compute address of its header and footer */
#define HEADER(bp)		((char *)(bp) - WSIZE)
#define FOOTER(bp)		((char *)(bp) + GET_SIZE(HEADER(bp)) - ALIGNMENT)

/* Given block ptr bp, computer the address of the next and previous blocks */
#define NEXT_BLK(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLK(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Macros to operate on free blocks */
#define NEXT_FREE(bp)   (HEADER(bp) + 1*WSIZE)
#define PREV_FREE(bp)   (HEADER(bp) + 2*WSIZE)
#define SET_PTR(p, val) (*(char *)(p) = (char)(val))


// Global Variable for start of heap.
void* freelist_start;
void* freelist_end;
void* heap_listp;

// Helper Functions:
static void* extend_heap(size_t);
static void* coalesce(void*);
static void* find_fit(size_t);
static void  split(void*, size_t);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {

    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    SET_INT(heap_listp, 0);
    SET_INT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    //Prologue Header
    SET_INT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    //Prologue Footer
    SET_INT(heap_listp + (3*WSIZE), PACK(0, 1));        //Epilogue Header

    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t adjustedSize, extendSize;
    char* ptr;

    // Ignore bad requests.
    if (size == 0)
        return NULL;

    // Adjust the block size to incluse header/footer + alignment.
    if (size <= DSIZE) {
        adjustedSize = MINBLK;
    } else {
        adjustedSize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);
    }

    // Search the free list for a fit
    if ((ptr = find_fit(adjustedSize)) != NULL) {
        split(ptr, adjustedSize);
        return ptr;
    }

    // No fit found. Get more memory and place the block.
    extendSize = MAX(adjustedSize, CHUNKSIZE);
    if ((ptr = extend_heap(extendSize/WSIZE)) == NULL)
        return NULL;
    split(ptr, adjustedSize);
    return ptr;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HEADER(ptr));

    SET_INT(HEADER(ptr), PACK(size, 0));
    SET_INT(FOOTER(ptr), PACK(size, 0));
    coalesce(ptr);


    /* TODO: Add newly freed block to the free list */
    // SET_PTR(NEXT_FREE(freelist_end), ptr);
    // SET_PTR(PREV_FREE(ptr), freelist_end);
    // freelist_end = ptr;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void* new_ptr = mm_malloc(size);
    memcpy(new_ptr, ptr, size);
    mm_free(ptr);
    return new_ptr;
}

/* 
 * extend_heap - Extend the heap with a new free block.
 */
static void* extend_heap(size_t words) {
    char* bp;
    size_t size;

    // Alocate an even number of blocks to maintain double word alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize the free block header/footer and the epilogue block */
    SET_INT(HEADER(bp), PACK(size, 0));             // Free block header
    SET_INT(FOOTER(bp), PACK(size, 0));             // Free block footer
    SET_INT(HEADER(NEXT_BLK(bp)), PACK(0, 1));      // New Epilogue Header

    // Coalesce if the previous block was free
    return coalesce(bp);
}

/* 
 * coalesce - Merge free blocks to prevent too small of free blocks.
 */
static void* coalesce(void* ptr) {
    
    size_t prev_alloc = IS_ALLOC(FOOTER(PREV_BLK(ptr)));
    size_t next_alloc = IS_ALLOC(HEADER(NEXT_BLK(ptr)));
    size_t size = GET_SIZE(HEADER(ptr));

    if (prev_alloc && next_alloc) {                         // Souronding blocks are allocated
        return ptr;

    } else if (prev_alloc && !next_alloc) {                 // Next block is free
        size += GET_SIZE(HEADER(NEXT_BLK(ptr)));
        SET_INT(HEADER(ptr), PACK(size, 0));
        SET_INT(FOOTER(ptr), PACK(size, 0));


    } else if (!prev_alloc && next_alloc) {                 // Previous block is free
        size += GET_SIZE(FOOTER(PREV_BLK(ptr)));
        SET_INT(FOOTER(ptr), PACK(size, 0));
        SET_INT(HEADER(PREV_BLK(ptr)), PACK(size, 0));
        ptr = PREV_BLK(ptr);

    } else {                                                // Both next & prev block are free
        size += GET_SIZE(HEADER(PREV_BLK(ptr))) + GET_SIZE(FOOTER(NEXT_BLK(ptr)));
        SET_INT(HEADER(PREV_BLK(ptr)), PACK(size, 0));
        SET_INT(FOOTER(NEXT_BLK(ptr)), PACK(size, 0));
        ptr = PREV_BLK(ptr);

    }

    return ptr;
}

/* 
 * find_fit - Find a fit for a chunk of aSize, implemented with best fit.
 */
static void* find_fit(size_t aSize) {
    void* ptr;
    void* bestFit = NULL;
    int sizeDiff = INT_MAX;

    for (ptr = heap_listp; GET_SIZE(HEADER(ptr)) > 0; ptr = NEXT_BLK(ptr) ) {
        if (GET_SIZE(HEADER(ptr)) >= aSize && !IS_ALLOC(HEADER(ptr))) {
            if ((GET_SIZE(HEADER(ptr)) - aSize) < sizeDiff) {
                sizeDiff = GET_SIZE(HEADER(ptr)) - aSize;
                bestFit = ptr;
            }
        }
    }

    return bestFit;
}

/* 
 * place - Places the new segment into the free block. Will slice
 *          if there is extra space at the end.
 */
static void split(void* ptr, size_t neededSize) {
    
    size_t blockSize = GET_SIZE(HEADER(ptr));   

    if ((blockSize - neededSize) >= MINBLK) { 
        SET_INT(HEADER(ptr), PACK(neededSize, 1));
        SET_INT(FOOTER(ptr), PACK(neededSize, 1));
        ptr = NEXT_BLK(ptr);
        SET_INT(HEADER(ptr), PACK(blockSize-neededSize, 0));
        SET_INT(FOOTER(ptr), PACK(blockSize-neededSize, 0));
    } else {
        SET_INT(HEADER(ptr), PACK(blockSize, 1));
        SET_INT(FOOTER(ptr), PACK(blockSize, 1));
    }
}
