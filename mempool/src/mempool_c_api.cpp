#include "mempool_c_api.h"
#include "Memory.h"

void* mempool_alloc(size_t size) {
    return mempool::MemoryPool::allocate(size);
}

void mempool_free(void* ptr, size_t size) {
    mempool::MemoryPool::deallocate(ptr, size);
} 