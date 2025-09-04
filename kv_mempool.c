#include "kv_mempool.h"
#include <stdlib.h>
#include <stddef.h>

// 内存池结构体，管理所有chunk
typedef struct mem_pool {
    void *free_list;         // 全局空闲链表头
    char *data;              // 实际数据区
    size_t chunk_size;       // 每个chunk的大小
    size_t chunk_count;      // chunk总数
    int initialized;         // 是否已初始化
} mem_pool_t;

// 向上对齐到align的整数倍
static size_t align_up(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

// 创建内存池，一次性分配所有chunk
mem_pool_t *kvs_mempool_create(size_t chunk_size, size_t chunk_count) {
    if (!chunk_size || !chunk_count)
        return NULL;
    size_t align = sizeof(void*);
    chunk_size = align_up(chunk_size, align);
    mem_pool_t *mp = (mem_pool_t *)malloc(sizeof(mem_pool_t));
    if (!mp) return NULL;
    mp->chunk_size = chunk_size;
    mp->chunk_count = chunk_count;
    mp->data = (char *)malloc(chunk_size * chunk_count);
    if (!mp->data) { free(mp); return NULL; }
    // 初始化全局空闲链表
    for (size_t i = 0; i < chunk_count - 1; i++) {
        void **cur = (void **)(mp->data + i * chunk_size);
        void **next = (void **)(mp->data + (i + 1) * chunk_size);
        *cur = next;
    }
    void **last = (void **)(mp->data + (chunk_count - 1) * chunk_size);
    *last = NULL;
    mp->free_list = mp->data;
    mp->initialized = 1;
    return mp;
}

// 分配chunk
void *kvs_mempool_alloc(mem_pool_t *mp) {
    if (!mp || !mp->initialized || !mp->free_list) return NULL;
    void *chunk = mp->free_list;
    mp->free_list = *(void **)chunk;
    return chunk;
}

// 释放chunk
void kvs_mempool_free(mem_pool_t *mp, void *ptr) {
    if (!mp || !ptr) return;
    *(void **)ptr = mp->free_list;
    mp->free_list = ptr;
}

// 销毁内存池
void kvs_mempool_destory(mem_pool_t *mp) {
    if (!mp) return;
    if (mp->data) free(mp->data);
    free(mp);
}

// #include"kv_mempool.h"

// // 定义每个内存块的大小和块数（可根据实际需求调整）
// #define POOL_CHUNK_NUMBER 1024
// #define POOL_BLOCK_COUNT 1024

// extern mem_pool_t *mp;

// // 内存块结构体，每个块管理一大块内存并维护空闲链表
// typedef struct mem_block{
//     size_t used_count;         // 已分配chunk数
//     void *free_list;           // 空闲链表头
//     struct mem_block *next;    // 指向下一个内存块
//     char *data;                // 实际数据区
// }mem_block_t;

// // 内存池结构体，管理所有内存块
// typedef struct mem_pool{
//     mem_block_t *first_block;  // 第一个内存块
//     size_t block_size;         // 每个内存块的大小
//     size_t block_count;        // 最大内存块数量
//     size_t chunk_size;         // 每个chunk的大小
//     int initialized;           // 是否已初始化
// }mem_pool_t;

// // 向上对齐到align的整数倍
// static size_t align_up(size_t size, size_t align) {
//     return (size + align - 1) & ~(align - 1);
// }

// // 创建内存池，预分配所有block
// mem_pool_t *kvs_mempool_create(size_t chunk_size, size_t block_count)
// {   
//     if(!chunk_size || !block_count)
//         return NULL;

//     size_t align = sizeof(void*);
//     chunk_size = align_up(chunk_size, align);

//     mem_pool_t *mp = (mem_pool_t *)malloc(sizeof(mem_pool_t));
//     if(!mp) return NULL;

//     mp->first_block = NULL;
//     mp->chunk_size = chunk_size;
//     mp->block_size = chunk_size * POOL_CHUNK_NUMBER; // 每个block包含1024个chunk
//     mp->block_count = block_count;
//     mp->initialized = 1;

//     mem_block_t *prev = NULL;
//     for (size_t i = 0; i < block_count; ++i) {
//         mem_block_t *block = (mem_block_t *)malloc(sizeof(mem_block_t));
//         if (!block) {
//             kvs_mempool_destory(mp);
//             return NULL;
//         }

//         block->used_count = 0;
//         block->next = NULL;
//         block->data = (char *)malloc(mp->block_size);
//         if (!block->data) {
//             free(block);
//             kvs_mempool_destory(mp);
//             return NULL;
//         }

//         size_t chunk_num = mp->block_size / mp->chunk_size;
//         for(size_t j = 0; j < chunk_num - 1; j++) {
//             void **cur = (void **)(block->data + j * mp->chunk_size);
//             void **next = (void **)(block->data + (j + 1) * mp->chunk_size);
//             *cur = next;
//         }
//         void **last = (void **)(block->data + (chunk_num - 1) * mp->chunk_size);
//         *last = NULL;

//         block->free_list = block->data;

//         if (prev) prev->next = block;
//         else mp->first_block = block;

//         prev = block;
//     }

//     return mp;
// }

// void *kvs_mempool_alloc(mem_pool_t *mp)
// {
//     if(!mp || !mp->initialized) return NULL;
//     mem_block_t *block = mp->first_block;
//     while(block) {
//         if(block->free_list) break;
//         block = block->next;
//     }
//     if(!block) return NULL; // 内存池已用尽

//     void *chunk = block->free_list;
//     block->free_list = *(void **)chunk;
//     block->used_count++;
//     return chunk;
// }

// // 释放指定chunk，将其插回空闲链表
// void kvs_mempool_free(mem_pool_t *mp, void *ptr)
// {
//     if(!mp||!ptr) return ;
//     mem_block_t *block=mp->first_block;
//     while(block)
//     {
//         if(ptr>=(void *)(block->data) && ptr<(void *)(block->data+mp->block_size))
//         {     *(void **)ptr=block->free_list;
//                block->free_list=ptr;
//                block->used_count--;
//                 return ;
//         }
//         block = block->next;
//     }
//     return ;
// }

// // 销毁内存池，释放所有内存块和内存池结构体
// void kvs_mempool_destory(mem_pool_t *mp)
// {
//    if(!mp) return ;
//    mem_block_t *block=mp->first_block;
//    while(block)
//    {
//      mem_block_t *next=block->next;
//      if(block->data) free(block->data);
//      free(block);
//      block=next;
//    } 
//    free(mp);
//    return ;
// }
// #endif
// #if 0
// #include"kv_mempool.h"

// // 定义每个内存块的大小和块数（可根据实际需求调整）
// #define POOL_BLOCK_SIZE 64
// #define POOL_BLOCK_COUNT 1024

// extern mem_pool_t *mp;

// // 内存块结构体，每个块管理一大块内存并维护空闲链表
// typedef struct mem_block{
//     size_t used_count;         // 已分配chunk数
//     void *free_list;           // 空闲链表头
//     struct mem_block *next;    // 指向下一个内存块
//     char *data;                // 实际数据区
// }mem_block_t;

// // 内存池结构体，管理所有内存块
// typedef struct mem_pool{
//     mem_block_t *first_block;  // 第一个内存块
//     size_t block_size;         // 每个内存块的大小
//     size_t block_count;        // 最大内存块数量
//     size_t chunk_size;         // 每个chunk的大小
//     int initialized;           // 是否已初始化
// }mem_pool_t;

// // 创建内存池，不预分配block，按需分配
// mem_pool_t *kvs_mempool_create(size_t chunk_size, size_t block_count)
// {   
//     if(!chunk_size || !block_count)
//         return NULL;
//     mem_pool_t *mp = (mem_pool_t *)malloc(sizeof(mem_pool_t));
//     if(!mp) return NULL;
//     mp->first_block = NULL;
//     mp->chunk_size = chunk_size;
//     mp->block_size = chunk_size * 1024; // 每个block包含1024个chunk，可根据需要调整
//     mp->block_count = block_count;
//     mp->initialized = 1;
//     return mp;
// }

// // 分配一个新的block，并初始化空闲链表
// static mem_block_t *alloc_block(size_t chunk_size, size_t block_size)
// {
//     mem_block_t *block = (mem_block_t *)malloc(sizeof(mem_block_t));
//     if(!block) return NULL;
//     block->used_count = 0;
//     block->next = NULL;
//     block->data = (char *)malloc(block_size);
//     if(!block->data) { free(block); return NULL; }
//     size_t chunk_num = block_size / chunk_size;
//     for(size_t i = 0; i < chunk_num - 1; i++) {
//         void **cur = (void **)(block->data + i * chunk_size);
//         void **next = (void **)(block->data + (i + 1) * chunk_size);
//         *cur = next;
//     }
//     void **last = (void **)(block->data + (chunk_num - 1) * chunk_size);
//     *last = NULL;
//     block->free_list = block->data;
//     return block;
// }

// void *kvs_mempool_alloc(mem_pool_t *mp)
// {
//     if(!mp || !mp->initialized) return NULL;
//     mem_block_t *block = mp->first_block;
//     mem_block_t *prev = NULL;
//     while(block) {
//         if(block->free_list) break;
//         prev = block;
//         block = block->next;
//     }
//     if(!block) {
//         size_t block_count = 0;
//         mem_block_t *tmp = mp->first_block;
//         while(tmp) { block_count++; tmp = tmp->next; }
//         if(block_count >= mp->block_count) return NULL;
//         mem_block_t *new_block = alloc_block(mp->chunk_size, mp->block_size);
//         if(!new_block) return NULL;
//         if(prev) prev->next = new_block;
//         else mp->first_block = new_block;
//         block = new_block;
//     }
//     void *chunk = block->free_list;
//     block->free_list = *(void **)chunk;
//     block->used_count++;
//     return chunk;
// }

// // 释放指定chunk，将其插回空闲链表
// void kvs_mempool_free(mem_pool_t *mp, void *ptr)
// {
//     if(!mp||!ptr) return ;
//     mem_block_t *block=mp->first_block;
//     while(block)
//     {
//         if(ptr>=(void *)(block->data) && ptr<(void *)(block->data+mp->block_size))
//         {     *(void **)ptr=block->free_list;
//                block->free_list=ptr;
//                block->used_count--;
//                 return ;
//         }
//         block = block->next;
//     }
//     return ;
// }

// // 销毁内存池，释放所有内存块和内存池结构体
// void kvs_mempool_destory(mem_pool_t *mp)
// {
//    if(!mp) return ;
//    mem_block_t *block=mp->first_block;
//    while(block)
//    {
//      mem_block_t *next=block->next;
//      if(block->data) free(block->data);
//      free(block);
//      block=next;
//    } 
//    free(mp);
//    return ;
// }#endif
