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
    "Kennen Derennard",
    /* Second member's email address (leave blank if none) */
    "kdere00x@ucr.edu"
};

/* single word (4) or double word (8) alignment */
#define WSIZE			4
#define DSIZE			8
#define ALIGNMENT		DSIZE
#define CHUNKSIZE		(1<<12)
#define MINBLK			(2*DSIZE)

/* MAX helper macro */
#define MAX(x, y)		((x) > (y) ? (x) : (y))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size)		(((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE		(ALIGN(sizeof(size_t)))

#define PACK(size, alloc)	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)			(*(unsigned int *)(p))
#define PUT(p, val)		(*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)		(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

/* Given a block ptr bp, compute address of its header and footer */
#define HDRP(bp)		((char *)(bp) - WSIZE)
#define FTRP(bp)		((char *)(bp) + GET_SIZE(HDRP(bp)) - ALIGNMENT)

/* Given block ptr bp, computer the address of the next and previous blocks */
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


// Global Variable for start of heap.
void* heap_listp;

// Helper Functions:
static void* extend_heap(size_t);
static void* coalesce(void*);
static void* find_fit(size_t);
static void place(void*, size_t);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    //Prologue Header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    //Prologue Footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        //Epilogue Header

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
    size_t aSize, extendSize;
    char* ptr;

    // Ignore bad requests.
    if (size == 0)
        return NULL;

    // Adjust the block size to incluse header/footer + alignment.
    if (size <= DSIZE) {
        aSize = MINBLK;
    } else {
        aSize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);
    }

    // Search the free list for a fit
    if ((ptr = find_fit(aSize)) != NULL) {
        place(ptr, aSize);
        return ptr;
    }

    // No fit found. Get more memory and place the block.
    extendSize = MAX(aSize, CHUNKSIZE);
    if ((ptr = extend_heap(extendSize/WSIZE)) == NULL)
        return NULL;
    place(ptr, aSize);
    return ptr;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    // Not necesary.
    return (void *)-1;
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
    PUT(HDRP(bp), PACK(size, 0));           // Free block header
    PUT(FTRP(bp), PACK(size, 0));           // Free block header
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   // New Epilogue Header

    // Coalesce if the previous block was free
    return coalesce(bp);
}

/* 
 * coalesce - 
 */
static void* coalesce(void* ptr) {
    
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc) {
        return ptr;

    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));


    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(FTRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);

    } else {
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(FTRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);

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

    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr) ) {
        if (GET_SIZE(HDRP(ptr)) >= aSize && !GET_ALLOC(HDRP(ptr))) {
            if ((GET_SIZE(HDRP(ptr)) - aSize) < sizeDiff) {
                sizeDiff = GET_SIZE(HDRP(ptr)) - aSize;
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
static void place(void* ptr, size_t neededSize) {
    
    size_t blockSize = GET_SIZE(HDRP(ptr));   

    if ((blockSize - neededSize) >= MINBLK) { 
        PUT(HDRP(ptr), PACK(neededSize, 1));
        PUT(FTRP(ptr), PACK(neededSize, 1));
        ptr = NEXT_BLKP(ptr);
        PUT(HDRP(ptr), PACK(blockSize-neededSize, 0));
        PUT(FTRP(ptr), PACK(blockSize-neededSize, 0));
    } else { 
        PUT(HDRP(ptr), PACK(blockSize, 1));
        PUT(FTRP(ptr), PACK(blockSize, 1));
    }
}






