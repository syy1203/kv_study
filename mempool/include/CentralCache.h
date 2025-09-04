#pragma once
#include "Common.h"
#include <mutex>
#include <unordered_map>
#include <array>
#include <atomic>
#include <chrono>

namespace mempool {

// constexpr size_t MAX_SIZE_CLASS = 64; // 支持的最大内存块类别数
constexpr size_t SPAN_TRACKER_NUM = 1024; // 追踪的span数量

// 追踪span信息的结构体
struct SpanTracker {
    std::atomic<void*> spanAddr{nullptr};   // span起始地址
    std::atomic<size_t> count{0};           // 该span分配出去的块数
    std::atomic<size_t> numPages{0};        // span包含的页数
    std::atomic<size_t> freeCount{0};       // 当前空闲块数
    std::atomic<size_t> blockCount{0};      // 总块数
};

// 中央缓存，负责管理不同大小的空闲内存块
class CentralCache {
public:
    static CentralCache& getInstance() {
        static CentralCache instance;
        return instance;
    }

    // 从中央缓存获取一段内存（按size class索引）
    void* fetchRange(size_t index);

    // 归还一段内存到中央缓存
    void returnRange(void* start, size_t size, size_t index);

private:
    CentralCache();
    CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;

    // 从PageCache获取大块内存并切分
    void* fetchFromPageCache(size_t size);

    // 通过内存块地址查找所属span
    SpanTracker* getFromRange(void* blockAddr);

    // 更新span的空闲块计数，并在需要时归还给PageCache
    void updateSpanFreeCount(SpanTracker* tracker, size_t newFreeBlocks, size_t index);

    // 中央自由链表，每个index对应一种大小的空闲块链表
    std::array<std::atomic<void*>, FREE_LIST_SIZE> centralFreelist_{};

    // 每个自由链表的自旋锁，保证多线程安全
    std::array<std::atomic_flag, FREE_LIST_SIZE> locks_{};

    // 追踪所有span信息，提升查找效率
    std::array<SpanTracker, SPAN_TRACKER_NUM> spanTrackers_{};

    // 当前已分配的span数量
    std::atomic<size_t> spanCount_{0};

    // 延迟归还相关
    static constexpr size_t MAX_DELAY_COUNT = 48; // 最大延迟计数
    std::array<std::atomic<size_t>, FREE_LIST_SIZE> delayCounts_{}; // 延迟计数
    std::array<std::chrono::steady_clock::time_point, FREE_LIST_SIZE> lastReturnTime_{}; // 上次归还时间
    static const std::chrono::milliseconds DELAY_INTERVAL; // 延迟归还的时间间隔

    // 判断是否需要执行延迟归还
    bool shouldPerformDelayReturn(size_t index, size_t currentCount, std::chrono::steady_clock::time_point currentTime);

    // 执行延迟归还
    void performDelayReturn(size_t index);
};

} // namespace mempool