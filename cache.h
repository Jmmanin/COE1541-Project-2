#include <stdlib.h>
#include <stdio.h>

struct cache_blk_t { /* note that no actual data will be stored in the cache */
    unsigned long tag;
    char valid;
    char dirty;
    unsigned int LRU;	/*to be used to build the LRU stack for the blocks in a cache set*/
};

struct cache_t {
    // The cache is represented by a 2-D array of blocks.
    // The first dimension of the 2D array is "nsets" which is the number of sets (entries)
    // The second dimension is "assoc", which is the number of blocks in each set.
    int nsets;					// number of sets
    int blocksize;				// block size
    int assoc;					// associativity
    int mem_latency;				// the miss penalty
    struct cache_blk_t **blocks;	// a pointer to the array of cache blocks
};

struct cache_t * cache_create(int size, int blocksize, int assoc, int mem_latency) {
    int i;
    int nblocks = (size * 1024) / blocksize;  // number of blocks in the cache
    int nsets = nblocks / assoc;	            // number of sets (entries) in the cache

    struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

    C->nsets = nsets;
    C->assoc = assoc;
    C->mem_latency = mem_latency;
    C->blocksize = blocksize;
    C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

    for(i = 0; i < nsets; i++) {
        //calloc sets values of all cache entries to 0 (including valid bit and LRU)
        C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
    }

    return C;
}

int getLogBase2(int);
int calculateIndexFromAddress(unsigned long, int, int, int);
void shift_LRU(struct cache_blk_t **, int, int);
int cache_access(struct cache_t *, unsigned long, int);

int getLogBase2(int num){
    int i = 1;
    while (num > 1) {
        num = num >> 1;
        if (num == 1){
            return i;
        }
        i++;
    }
}

int calculateIndexFromAddress(unsigned long address, int numBitsforByteOffset, int numBitsForWordOffset, int numBitsForIndex){
    int i;
    int val = 1;
    int totalOffset = numBitsforByteOffset + numBitsForWordOffset;
    for(i = 1; i <= numBitsForIndex; i++){
        val = 2 * val;
    }
    val = val - 1;
    //printf("Address: %lx, totalOffset: %d\n", address, totalOffset);
    //printf("Value: %x, Shifted Address: %lx\n", val, address >> totalOffset);
    val = val & (address >> totalOffset);
    //printf("CalculatedINDEX: %d\n", val);
    return val;
}

void shift_LRU(struct cache_blk_t **associativeCaches, int index, int associativity)
{
    int i;
    for (i = 0; i < associativity; i++)
    {
        if ((associativeCaches[index][i].LRU != 0) && (associativeCaches[index][i].valid == 1))
        {
            associativeCaches[index][i].LRU -= 1;
        }
    }
}

int cache_access(struct cache_t *cp, unsigned long address, int access_type)
{
    //
    // Based on "address", determine the set to access in cp and examine the blocks
    // in the set to check hit/miss and update the golbal hit/miss statistics
    // If a miss, determine the victim in the set to replace (LRU).
    //
    // The function should return the hit_latency, which is 0, in case of a hit.
    // In case of a miss, the function should return mem_latency if no write back is needed.
    // If a write back is needed, the function should return 2*mem_latency.
    // access_type (0 for a read and 1 for a write) should be used to set/update the dirty bit.
    // The LRU field of the blocks in the set accessed should also be updated.
    int i;
    int numBitsforByteOffset = 2; // This offset will always be 2 because there are 4 bytes per word
    int numBitsForWordOffset = getLogBase2(cp->blocksize/4);
    //printf("BLOCKSIZE IS %d\n", cp->blocksize);
    //printf("WORD OFFSET IS %d\n", numBitsForWordOffset);
    int numBitsForIndex = getLogBase2(cp->nsets);
    int numBitsForTag = 32 - numBitsForIndex - numBitsForWordOffset - numBitsforByteOffset;

    int index = calculateIndexFromAddress(address, numBitsforByteOffset, numBitsForWordOffset, numBitsForIndex);
    //printf("Index: %d\n", index);
    int tag = address >> (32 - numBitsForTag);
    //printf("Tag: %d\n", tag);

    for (i = 0; i < cp->assoc; i++)
    {
        if (cp->blocks[index][i].tag == tag)
        {
            // cache hit
            if(access_type == 1)
            {
                // write
                cp->blocks[index][i].dirty = 1;
            }
            // no memory latency on hit
            return(0);
        }
    }

    // cache miss
    for (i = 0; i < cp->assoc; i++)
    {
        //checks for uninitialized blocks
        if (cp->blocks[index][i].valid == 0)
        {
            // create entry at this index in the cache
            cp->blocks[index][i].valid = 1;
            cp->blocks[index][i].tag = tag;
            cp->blocks[index][i].LRU = (cp->assoc);

            // update LRU values in the n-way associative cache
            shift_LRU(cp->blocks, index, cp->assoc);

            // return the latency for a single miss
            return(cp->mem_latency);
        }
    }

    int HIGH_DIRTY = 1;
    for (i = 0; i < cp->assoc; i++)
    {
        if (cp->blocks[index][i].LRU == 0)
        {
            // Check the dirty bit for write-back
            if (cp->blocks[index][i].dirty == 1)
            {
                HIGH_DIRTY = 2;
            }
            // Evict current value and replace with new block
            cp->blocks[index][i].valid = 1;
            cp->blocks[index][i].dirty = 0;
            cp->blocks[index][i].LRU = cp->assoc - 1;
            cp->blocks[index][i].tag = tag;
        }
        else
        {
            cp->blocks[index][i].LRU -= 1;
        }
    }

    return(HIGH_DIRTY * cp->mem_latency);

return(0);
}
