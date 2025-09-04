#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "kv_mempool.h"
#include "kv_mempool_slab.h"

#define TEST_COUNT 1000000
#define CHUNK_SIZE 4
#define BLOCK_COUNT 2000  // 单链表池的block数，需保证能分配TEST_COUNT

int main() {
    void **ptrs = (void **)malloc(TEST_COUNT * sizeof(void *));
    if (!ptrs) {
        printf("分配ptrs失败\n");
        return 1;
    }
    clock_t start, end;
    double pool_time, sys_time, slab_time;

    // 1. 单链表内存池
    mem_pool_t *mp = kvs_mempool_create(CHUNK_SIZE, BLOCK_COUNT);
    if (!mp) {
        printf("单链表内存池创建失败\n");
        free(ptrs);
        return 1;
    }
    start = clock();
    for (int i = 0; i < TEST_COUNT; i++) {
        ptrs[i] = kvs_mempool_alloc(mp);
        if (!ptrs[i]) {
            printf("单链表内存池分配失败: %d\n", i);
            break;
        }
    }
    for (int i = 0; i < TEST_COUNT; i++) {
        kvs_mempool_free(mp, ptrs[i]);
    }
    end = clock();
    pool_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("单链表内存池分配+释放 %d 次耗时: %.6f 秒\n", TEST_COUNT, pool_time);

    // 2. slab/位图法内存池
    // bitmap_pool_t *slab = bitmap_pool_create(CHUNK_SIZE, TEST_COUNT);
    // if (!slab) {
    //     printf("slab内存池创建失败\n");
    //     kvs_mempool_destory(mp);
    //     free(ptrs);
    //     return 1;
    // }
    // start = clock();
    // for (int i = 0; i < TEST_COUNT; i++) {
    //     ptrs[i] = bitmap_pool_alloc(slab);
    //     if (!ptrs[i]) {
    //         printf("slab分配失败: %d\n", i);
    //         break;
    //     }
    // }
    // for (int i = 0; i < TEST_COUNT; i++) {
    //     bitmap_pool_free(slab, ptrs[i]);
    // }
    // end = clock();
    // slab_time = (double)(end - start) / CLOCKS_PER_SEC;
    // printf("slab(位图)内存池分配+释放 %d 次耗时: %.6f 秒\n", TEST_COUNT, slab_time);

    // 3. 系统malloc/free
    start = clock();
    for (int i = 0; i < TEST_COUNT; i++) {
        ptrs[i] = malloc(CHUNK_SIZE);
        if (!ptrs[i]) {
            printf("系统malloc失败: %d\n", i);
            break;
        }
    }
    for (int i = 0; i < TEST_COUNT; i++) {
        free(ptrs[i]);
    }
    end = clock();
    sys_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("系统malloc+free %d 次耗时: %.6f 秒\n", TEST_COUNT, sys_time);

    kvs_mempool_destory(mp);
    // bitmap_pool_destroy(slab);
    free(ptrs);
    return 0;
}