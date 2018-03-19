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

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    " Hooli",
    /* First member's full name */
    " Daniel Stinson-Diess",
    /* First member's email address */
    " dstin002@ucr.edu",
    /* Second member's full name (leave blank if none) */
    " Kennen DeRenard",
    /* Second member's email address (leave blank if none) */
    " kdere004@ucr.edu"
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
#define ALIGN(size)			(((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_ALIGN		(ALIGN(sizeof(size_t)))

#define PACK(size, alloc)	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)				(*(unsigned int *)(p))
#define SET_INT(p, val)		(*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)			(GET(p) & ~0x7)
#define IS_ALLOC(p)			(GET(p) & 0x1)

/* Given a block ptr bp, compute address of its header and footer */
#define HEADER(bp)			((char *)(bp) - WSIZE)
#define FOOTER(bp)			((char *)(bp) + GET_SIZE(HEADER(bp)) - ALIGNMENT)

/* Given block ptr bp, computer the address of the next and previous blocks */
#define NEXT_BLK(bp)		((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLK(bp)		((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Macros to operate on free blocks */
#define NEXT_PTR(bp)		(*(char **)(bp + 1*WSIZE))
#define PREV_PTR(bp)		(*(char **)(bp))

#define SET_PTR(bp, val)	(bp = val)

// Global Variable for start of heap.

void* freeLists[10];

//void* freelist_head = NULL;

void* heap_listp = NULL;

// Helper Functions:
static void* extend_heap(size_t);
static void* coalesce(void*);
static void* find_fit(size_t, int);
static void  split(void*, size_t, int);
static void* push_front(void*, int);
static void  erase(void*, int);
static int find_list(size_t);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    int i;
    
    for (i = 0; i < 10; ++i) {
        freeLists[i] = NULL;
    }

    if ((heap_listp = mem_sbrk(8*WSIZE)) == (void *)-1)
        return -1;

    SET_INT(heap_listp, 0);
    SET_INT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    //Prologue Header
    SET_INT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    //Prologue Footer
    SET_INT(heap_listp + (3*WSIZE), PACK(0, 1));        //Epilogue Header

    freeLists[0] = heap_listp + (2*WSIZE);

    if (extend_heap(4) == NULL)
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
        adjustedSize = 2*DSIZE;
    } else {
        adjustedSize = DSIZE * ((size + (DSIZE) + (DSIZE - 1))/ DSIZE);
    }

    // Search the free list for a fit
    int listIndex = find_list(adjustedSize);
    while (listIndex < 10) {
        if ((ptr = find_fit(adjustedSize, listIndex)) != NULL) {
            split(ptr, adjustedSize, listIndex);
            return ptr;
        }
        listIndex++;
    }

    // No fit found. Get more memory and place the block.
    extendSize = MAX(adjustedSize, CHUNKSIZE);
    if ((ptr = extend_heap(extendSize/WSIZE)) == NULL)
        return NULL;
    split(ptr, adjustedSize, listIndex);
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
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free.
 * 					Done to recieve a wholistic score.
 */
void *mm_realloc(void *ptr, size_t size) {
    if (size <= 0) {
        mm_free(ptr);
        return ptr;
    } else if (size + 2*DSIZE <= GET_SIZE(HEADER(ptr))) {
        return ptr;
    } else {
        
        int next_block_available = IS_ALLOC(HEADER(NEXT_BLK(ptr)));
        int current_size = GET_SIZE(HEADER(ptr));
        int next_size = GET_SIZE(HEADER(NEXT_BLK(ptr)));
        int nextListIndex = find_list(next_size);

        if (!next_block_available && (next_size + current_size > size + DSIZE)) {
            erase(NEXT_BLK(ptr), nextListIndex);
            SET_INT(HEADER(ptr), PACK(next_size + current_size, 1));
            SET_INT(FOOTER(ptr), PACK(next_size + current_size, 1));
            return ptr;
        }
        

        void* new_ptr = mm_malloc(size);
        memcpy(new_ptr, ptr, size);
        mm_free(ptr);
        return new_ptr;
        
    }
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
    
    size_t prev_alloc = IS_ALLOC(FOOTER(PREV_BLK(ptr))) || PREV_BLK(ptr) == ptr;
    size_t next_alloc = IS_ALLOC(HEADER(NEXT_BLK(ptr)));
    size_t size = GET_SIZE(HEADER(ptr));

    int prevListIndex = find_list(GET_SIZE(HEADER(PREV_BLK(ptr))));
    int nextListIndex = find_list(GET_SIZE(HEADER(NEXT_BLK(ptr))));
    int newListIndex = find_list(size);    

    if (prev_alloc && next_alloc) {				// Souronding blocks are allocated
    	// Do nothing

    } else if (prev_alloc && !next_alloc) {		// Next block is free
        size += GET_SIZE(HEADER(NEXT_BLK(ptr)));
        newListIndex = find_list(size);
        erase(NEXT_BLK(ptr), nextListIndex);
        SET_INT(HEADER(ptr), PACK(size, 0));
        SET_INT(FOOTER(ptr), PACK(size, 0));

    } else if (!prev_alloc && next_alloc) {		// Previous block is free
        size += GET_SIZE(HEADER(PREV_BLK(ptr)));
        newListIndex = find_list(size);
        ptr = PREV_BLK(ptr);
        erase(ptr, prevListIndex);
        SET_INT(HEADER(ptr), PACK(size, 0));
        SET_INT(FOOTER(ptr), PACK(size, 0));

    } else {									// Both next & prev block are free
        size += GET_SIZE(HEADER(PREV_BLK(ptr))) + GET_SIZE(HEADER(NEXT_BLK(ptr)));
        newListIndex = find_list(size);
        erase(NEXT_BLK(ptr), nextListIndex);
        erase(PREV_BLK(ptr), prevListIndex);
        ptr = PREV_BLK(ptr);

        SET_INT(HEADER(ptr), PACK(size, 0));
        SET_INT(FOOTER(ptr), PACK(size, 0));
        
    }

    return push_front(ptr, newListIndex);
}

/* 
 * find_fit - Find a fit for a chunk of aSize, implemented with first fit.
 */
static void* find_fit(size_t aSize, int freelist_head) {
    void* ptr = NULL;
    if (freeLists[freelist_head]) {
        for (ptr = freeLists[freelist_head]; IS_ALLOC(HEADER(ptr)) == 0; ptr = NEXT_PTR(ptr)) {
            if (GET_SIZE(HEADER(ptr)) >= aSize) {
                return ptr;
            }
        }
    }
    return NULL;
}

/* 
 *i place - Places the new segment into the free block. Will slice
 *          if there is extra space at the end.
 */
static void split(void* ptr, size_t neededSize, int freelist_head) {
    
    size_t blockSize = GET_SIZE(HEADER(ptr));   

    if ((blockSize - neededSize) >= MINBLK) { 
        SET_INT(HEADER(ptr), PACK(neededSize, 1));
        SET_INT(FOOTER(ptr), PACK(neededSize, 1));
        erase(ptr, freelist_head);
        ptr = NEXT_BLK(ptr);
        SET_INT(HEADER(ptr), PACK(blockSize-neededSize, 0));
        SET_INT(FOOTER(ptr), PACK(blockSize-neededSize, 0));
        coalesce(ptr);
    } else {
        SET_INT(HEADER(ptr), PACK(blockSize, 1));
        SET_INT(FOOTER(ptr), PACK(blockSize, 1));
        erase(ptr, freelist_head);
    }
}

/* 
 * push_front - Places the new segment into the freelist.
 */
static void* push_front(void* new_ptr, int freelist_head) {
	SET_PTR(NEXT_PTR(new_ptr), freeLists[freelist_head]);
        SET_PTR(PREV_PTR(new_ptr), NULL);
        if (freeLists[freelist_head])
            SET_PTR(PREV_PTR(freeLists[freelist_head]), new_ptr);

	freeLists[freelist_head] = new_ptr;

	return freeLists[freelist_head];
}

/* 
 * erase - Removes the new segment from the freelist.
 */
static void erase(void* delNode, int freelist_head) {
	if (PREV_PTR(delNode)) {
		SET_PTR(NEXT_PTR(PREV_PTR(delNode)), NEXT_PTR(delNode));
	
	} else {
		freeLists[freelist_head] = NEXT_PTR(delNode);

	}
        if (NEXT_PTR(delNode))
	    SET_PTR(PREV_PTR(NEXT_PTR(delNode)), PREV_PTR(delNode));
}

static int find_list(size_t size) {
    if (size <= 16)
        return 0;
    if (size <= 32)
        return 1;
    if (size <= 64)
        return 2;
    if (size <= 128)
        return 3;
    if (size <= 256)
        return 4;
    if (size <= 512)
        return 5;
    if (size <= 1024)
        return 6;
    if (size <= 2048)
        return 7;
    if (size <= 4096)
        return 8;
    return 9;  
}
