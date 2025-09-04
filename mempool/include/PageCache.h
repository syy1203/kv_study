#pragma once
#include "Common.h"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>
#include <map>

namespace mempool {

class PageCache {
public:
    static constexpr size_t PAGE_SIZE = 4096;
    static PageCache& getInstance() {
        static PageCache instance;
        return instance;
    }
    void* allocateSpan(size_t numPages);
    void deallocateSpan(void* ptr, size_t numPages);

private:
    PageCache() = default;
    PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;
    void* systemAlloc(size_t numPages);

    struct Span {
        Span* next;
        void* pageAddr;
        size_t numPages;
    };
    std::mutex mutex_;
    std::map<size_t, Span*> freeSpans_;
    std::unordered_map<void*, Span*> spanMap_;
};

} // namespace mempool