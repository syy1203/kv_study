#ifndef _KV_MEMPOOL_H_
#define _KV_MEMPOOL_H_

#include <stdlib.h>

typedef struct mem_block mem_block_t;
typedef struct mem_pool mem_pool_t;

mem_pool_t *kvs_mempool_create(size_t chunk_size, size_t block_count);
void *kvs_mempool_alloc(mem_pool_t *mp);
void kvs_mempool_free(mem_pool_t *mp, void *ptr);
void kvs_mempool_destory(mem_pool_t *mp);

#endif // _KV_MEMPOOL_H_ 