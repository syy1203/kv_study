#include "kv_mempool_slab.h"

// 向上对齐到align的整数倍
static size_t align_up(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

// 位图法内存池结构体

// 创建内存池
bitmap_pool_t *bitmap_pool_create(size_t chunk_size, size_t chunk_count) {
    size_t align = sizeof(void*);
    chunk_size = align_up(chunk_size, align);
    bitmap_pool_t *pool = (bitmap_pool_t *)malloc(sizeof(bitmap_pool_t));
    if (!pool) return NULL;
    pool->chunk_size = chunk_size;
    pool->chunk_count = chunk_count;
    size_t bitmap_bytes = (chunk_count + 7) / 8;
    pool->bitmap = (unsigned char *)calloc(bitmap_bytes, 1); // 全部置0
    pool->data = (char *)malloc(chunk_size * chunk_count);
    if (!pool->bitmap || !pool->data) {
        free(pool->bitmap);
        free(pool->data);
        free(pool);
        return NULL;
    }
    return pool;
}

// 分配一个chunk
void *bitmap_pool_alloc(bitmap_pool_t *pool) {
    size_t n = pool->chunk_count;
    for (size_t i = 0; i < n; i++) {
        size_t byte = i / 8, bit = i % 8;
        if (!(pool->bitmap[byte] & (1 << bit))) {
            pool->bitmap[byte] |= (1 << bit);
            return pool->data + i * pool->chunk_size;
        }
    }
    return NULL; // 没有空闲
}

// 释放一个chunk
void bitmap_pool_free(bitmap_pool_t *pool, void *ptr) {
    if (!pool || !ptr) return;
    size_t idx = ((char *)ptr - pool->data) / pool->chunk_size;
    if (idx >= pool->chunk_count) return; // 越界保护
    size_t byte = idx / 8, bit = idx % 8;
    pool->bitmap[byte] &= ~(1 << bit);
}

// 销毁内存池
void bitmap_pool_destroy(bitmap_pool_t *pool) {
    if (!pool) return;
    free(pool->bitmap);
    free(pool->data);
    free(pool);
}

// 获取当前进程的物理内存占用（VmRSS，单位KB）
size_t get_rss_kb() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[256];
    size_t rss = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmRSS: %zu", &rss) == 1) {
            break;
        }
    }
    fclose(f);
    return rss;
}

