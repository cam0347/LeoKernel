#include <include/types.h>
#include <crypto/include/hash.h>
#include <include/mem.h>

md5_t md5(void *input, uint32_t size) {
    md5_t ret;
    memclear(ret.bytes, 16);

    //calculate the number of 512 bits (4 bytes * 16) blocks
    uint32_t num_blocks = size / 512 + size % 512 != 0 ? 1 : 0;
    uint32_t blocks[num_blocks][16];

    //copy the data into the blocks
    for (uint32_t i = 0; i < num_blocks - 1; i++) {
        memcpy(&blocks[i], input + 64 * i, 64);
    }

    //the last block must be filled this way
    memcpy(&blocks[num_blocks - 1], input + 64 * (num_blocks - 1), size % 512);

    
}