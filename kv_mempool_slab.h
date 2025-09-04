#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>


typedef struct {
    unsigned char *bitmap; // 位图，每bit对应一个chunk
    char *data;            // 实际数据区
    size_t chunk_size;     // 每个chunk大小
    size_t chunk_count;    // chunk总数
} bitmap_pool_t;

bitmap_pool_t *bitmap_pool_create(size_t chunk_size, size_t chunk_count);
void *bitmap_pool_alloc(bitmap_pool_t *pool);
void bitmap_pool_free(bitmap_pool_t *pool, void *ptr) ;
void bitmap_pool_destroy(bitmap_pool_t *pool);
size_t get_rss_kb();