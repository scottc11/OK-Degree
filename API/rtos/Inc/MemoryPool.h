#pragma once

#include "cmsis_os.h"

/// Memory size in bytes for Memory Pool storage.
/// \param         block_count   maximum number of memory blocks in memory pool.
/// \param         block_size    memory block size in bytes.
#define osRtxMemoryPoolMemSize(block_count, block_size) \
    (4 * (block_count) * (((block_size) + 3) / 4))

template <typename T, uint32_t pool_sz>
class MemoryPool {
public:
    MemoryPool() {
        // The C library function void *memset(void *str, int c, size_t n) copies the character c (an unsigned char) to the first n characters of the string pointed to, by the argument str.
        memset(_pool_mem, 0, sizeof(_pool_mem));
    }
private:
    char _pool_mem[osRtxMemoryPoolMemSize(pool_sz, sizeof(T))];
};