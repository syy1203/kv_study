#include "mempool/include/mempool_c_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000000
#define BLOCK_SIZE 8

// 计算时间差（单位：秒）
double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + 
           (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    void* ptrs[N];
    struct timespec start, end;

    // 测试 mempool 分配/释放
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        ptrs[i] = mempool_alloc(BLOCK_SIZE);
    }
    for (int i = 0; i < N; i++) {
        mempool_free(ptrs[i], BLOCK_SIZE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("mempool alloc/free time: %f seconds\n", time_diff(start, end));

    // 测试 malloc/free
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        ptrs[i] = malloc(BLOCK_SIZE);
    }
    for (int i = 0; i < N; i++) {
        free(ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("malloc/free time: %f seconds\n", time_diff(start, end));

    return 0;
}
