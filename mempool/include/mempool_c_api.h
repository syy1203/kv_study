#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void* mempool_alloc(size_t size);
void mempool_free(void* ptr, size_t size);

#ifdef __cplusplus
}
#endif 