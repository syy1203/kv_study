#include "../include/CentralCache.h"
#include "../include/PageCache.h"
#include <cassert>
#include <thread>
#include <chrono>

namespace mempool {

 static const size_t SPAN_PAGES = 8;
CentralCache::CentralCache() {

    
    for (auto& ptr : centralFreelist_) {
        ptr.store(nullptr, std::memory_order_relaxed);
    }
    for (auto& lock : locks_) {
        lock.clear();
    }
    for (auto& count : delayCounts_) {
        count.store(0, std::memory_order_relaxed);
    }
    for (auto& time : lastReturnTime_) {
        time = std::chrono::steady_clock::now();
    }
    spanCount_.store(0, std::memory_order_relaxed);
}

// 从中央缓存获取一段内存（按size class索引）
void* CentralCache::fetchRange(size_t index) {
    if (index >= FREE_LIST_SIZE) return nullptr;
    
    while (locks_[index].test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    void* result = nullptr;
    try {
        result = centralFreelist_[index].load(std::memory_order_relaxed);
        if (!result) {
            // 如果中心缓存为空，从页缓存获取新的内存块
            size_t size = (index + 1) * ALIGNMENT;
            result = fetchFromPageCache(size);
            if (!result) {
                locks_[index].clear(std::memory_order_release);
                return nullptr;
            }
            // 这里应将大块切分成小块，挂到centralFreelist_，并更新SpanTracker
            // 省略具体切分实现（需结合实际需求）
             // 将获取的内存块切分成小块
             char* start = static_cast<char*>(result); // 起始地址

             // 计算实际分配的页数
             size_t numPages = (size <= SPAN_PAGES * PageCache::PAGE_SIZE) ? 
                              SPAN_PAGES : (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE; // 如果size小于等于默认span页数，则用默认span页数，否则按实际需求页数
            // 使用实际页数计算块数
            size_t blockNum = (numPages * PageCache::PAGE_SIZE) / size; // 计算该span可以切分成多少个小块
            
            if (blockNum > 1) 
            {  // 确保至少有两个块才构建链表
                for (size_t i = 1; i < blockNum; ++i) 
                {
                    void* current = start + (i - 1) * size; // 当前小块的地址
                    void* next = start + i * size;          // 下一个小块的地址
                    assert(current < start + numPages * PageCache::PAGE_SIZE);
                    *reinterpret_cast<void**>(current) = next; // 当前小块指向下一个小块
                }
                *reinterpret_cast<void**>(start + (blockNum - 1) * size) = nullptr; // 最后一个小块的next置空
                
                // 保存result的下一个节点
                void* next = *reinterpret_cast<void**>(result); // 取出result原本的next指针
                // 将result与链表断开
                *reinterpret_cast<void**>(result) = nullptr;    // 断开result与后续链表的连接
                // 更新中心缓存
                centralFreelist_[index].store(
                    next, 
                    std::memory_order_release
                ); // 将链表头指向result的下一个节点
                
                // 使用无锁方式记录span信息
                // 做记录是为了将中心缓存多余内存块归还给页缓存做准备。考虑点：
                // 1.CentralCache 管理的是小块内存，这些内存可能不连续
                // 2.PageCache 的 deallocateSpan 要求归还连续的内存
                size_t trackerIndex = spanCount_++; // 获取当前可用的spanTracker下标
                if (trackerIndex < spanTrackers_.size())
                {
                    spanTrackers_[trackerIndex].spanAddr.store(start, std::memory_order_release); // 记录span起始地址
                    spanTrackers_[trackerIndex].numPages.store(numPages, std::memory_order_release); // 记录span页数
                    spanTrackers_[trackerIndex].blockCount.store(blockNum, std::memory_order_release); // 共分配了blockNum个内存块
                    spanTrackers_[trackerIndex].freeCount.store(blockNum - 1, std::memory_order_release); // 第一个块result已被分配出去，所以初始空闲块数为blockNum - 1
                }
            } else {
                void* next = *reinterpret_cast<void**>(result);
                centralFreelist_[index].store(next, std::memory_order_relaxed);
                SpanTracker* tracker = getFromRange(result);
                if (tracker) {
                    tracker->freeCount.fetch_sub(1, std::memory_order_release);
                }
            }
        }
        
    } catch (...) {
        locks_[index].clear(std::memory_order_release);
        throw;
    }
    locks_[index].clear(std::memory_order_release);
    return result;
}

// 归还一段内存到中央缓存
void CentralCache::returnRange(void* start, size_t size, size_t index) {
    if (!start || index >= FREE_LIST_SIZE) return;
    size_t block_size = (index + 1) * ALIGNMENT;
    size_t blockcount = size / block_size;
    while (locks_[index].test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    try {
        void* end = start;
        size_t count = 1;
        while (*reinterpret_cast<void**>(end) != nullptr && count < blockcount) {
            end = *reinterpret_cast<void**>(end);
            count++;
        }
        void* current = centralFreelist_[index].load(std::memory_order_relaxed);
        *reinterpret_cast<void**>(end) = current;
        centralFreelist_[index].store(start, std::memory_order_release);

        size_t currentCount = delayCounts_[index].fetch_add(1, std::memory_order_relaxed) + 1;
        auto currentTime = std::chrono::steady_clock::now();
        if (shouldPerformDelayReturn(index, currentCount, currentTime)) {
            performDelayReturn(index);
        }
    } catch (...) {
        locks_[index].clear(std::memory_order_release);
        throw;
    }
    locks_[index].clear(std::memory_order_release);
}

// 判断是否需要执行延迟归还
bool CentralCache::shouldPerformDelayReturn(size_t index, size_t currentCount, std::chrono::steady_clock::time_point currentTime) {
    if (currentCount >= MAX_DELAY_COUNT) return true;
    auto lastTime = lastReturnTime_[index];
    return (currentTime - lastTime) >= DELAY_INTERVAL;
}

// 执行延迟归还，将多余的内存块归还给PageCache
void CentralCache::performDelayReturn(size_t index) {
    delayCounts_[index].store(0, std::memory_order_relaxed);
    lastReturnTime_[index] = std::chrono::steady_clock::now();
    // 统计每个span的空闲块数
    std::unordered_map<SpanTracker*, size_t> spanFreeCounts;
    void* currentBlock = centralFreelist_[index].load(std::memory_order_relaxed);
    while (currentBlock) {
        SpanTracker* tracker = getFromRange(currentBlock);
        if (tracker) {
            spanFreeCounts[tracker]++;
        }
        currentBlock = *reinterpret_cast<void**>(currentBlock);
    }
    // 更新每个span的空闲计数器并检查是否可以归还
    for (const auto& [tracker, newFreeBlocks] : spanFreeCounts) {
        updateSpanFreeCount(tracker, newFreeBlocks, index);
    }
}

// 更新span的空闲块计数，并在需要时归还给PageCache
void CentralCache::updateSpanFreeCount(SpanTracker* tracker, size_t newFreeBlocks, size_t index) {
    size_t oldFreeCount = tracker->freeCount.load(std::memory_order_relaxed);
    size_t newFreeCount = oldFreeCount + newFreeBlocks;
    // 如果span所有块都空闲，可以归还给PageCache
    if (newFreeCount == tracker->blockCount.load(std::memory_order_relaxed)) {
        void* spanAddr = tracker->spanAddr.load(std::memory_order_relaxed);
        size_t numPages = tracker->numPages.load(std::memory_order_relaxed);
        // 从centralFreelist_移除该span的所有块
        void* head = centralFreelist_[index].load(std::memory_order_relaxed);
        void* newhead = nullptr;
        void* prev = nullptr;
        void* current = head;
        while (current) {
            void* next = *reinterpret_cast<void**>(current);
            if (current >= spanAddr && current < static_cast<char*>(spanAddr) + numPages * PageCache::PAGE_SIZE) {
                if (prev) {
                    *reinterpret_cast<void**>(prev) = next;
                } else {
                    newhead = next;
                }
            } else {
                prev = current;
            }
            current = next;
        }
        centralFreelist_[index].store(newhead, std::memory_order_release);
        PageCache::getInstance().deallocateSpan(spanAddr, numPages);
    }
}

// 从PageCache获取大块内存并切分
void* CentralCache::fetchFromPageCache(size_t size) {
    // 计算实际需要的页数
    size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
    // 按需分配
    if(size<SPAN_PAGES*PageCache::PAGE_SIZE)
    {
        return PageCache::getInstance().allocateSpan(SPAN_PAGES);
    }
    else
    {
        return PageCache::getInstance().allocateSpan(numPages);
    }
 
}

// 通过内存块地址查找所属span
SpanTracker* CentralCache::getFromRange(void* blockAddr) {
    for (size_t i = 0; i < spanCount_; ++i) {
        void* spanStart = spanTrackers_[i].spanAddr.load(std::memory_order_relaxed);
        size_t numPages = spanTrackers_[i].numPages.load(std::memory_order_relaxed);
        if (blockAddr >= spanStart && blockAddr < static_cast<char*>(spanStart) + numPages * PageCache::PAGE_SIZE) {
            return &spanTrackers_[i];
        }
    }
    return nullptr;
}

const std::chrono::milliseconds CentralCache::DELAY_INTERVAL{100}; // 100ms延迟归还间隔

} // namespace mempool