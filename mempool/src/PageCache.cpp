#include "PageCache.h"
#include <cstring>
#include <sys/mman.h> // for mmap
#include <unistd.h>

namespace mempool
{

// 分配numPages页的内存，优先从空闲span链表中查找，找不到则向系统申请
void* PageCache::allocateSpan(size_t numPages)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. 查找合适的空闲span（first-fit，找第一个大于等于numPages的span）
    auto it = freeSpans_.lower_bound(numPages);
    if (it != freeSpans_.end())
    {
        Span* span = it->second;

        // 2. 将取出的span从空闲链表中移除
        if (span->next)
        {
            freeSpans_[it->first] = span->next;
        }
        else
        {
            freeSpans_.erase(it);
        }

        // 3. 如果span大于需要的numPages则进行分割
        if (span->numPages > numPages) 
        {
            Span* newSpan = new Span;
            newSpan->pageAddr = static_cast<char*>(span->pageAddr) + numPages * PAGE_SIZE;
            newSpan->numPages = span->numPages - numPages;
            newSpan->next = nullptr;

            // 将超出部分放回空闲Span链表头部
            auto& list = freeSpans_[newSpan->numPages];
            newSpan->next = list;
            list = newSpan;

            span->numPages = numPages;
        }

        // 4. 记录span信息用于回收
        spanMap_[span->pageAddr] = span;
        return span->pageAddr;
    }

    // 5. 没有合适的span，向系统申请
    void* memory = systemAlloc(numPages);
    if (!memory) return nullptr;

    // 6. 创建新的span并记录
    Span* span = new Span;
    span->pageAddr = memory;
    span->numPages = numPages;
    span->next = nullptr;
    spanMap_[memory] = span;
    return memory;
}

// 回收numPages页的内存，尝试与相邻span合并，减少碎片
void PageCache::deallocateSpan(void* ptr, size_t numPages)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. 查找对应的span，没找到代表不是PageCache分配的内存，直接返回
    auto it = spanMap_.find(ptr);
    if (it == spanMap_.end()) return;

    Span* span = it->second;

    // 2. 尝试合并相邻的span（只合并后邻）
    void* nextAddr = static_cast<char*>(ptr) + numPages * PAGE_SIZE;
    auto nextIt = spanMap_.find(nextAddr);
    
    if (nextIt != spanMap_.end())
    {
        Span* nextSpan = nextIt->second;
        // 检查nextSpan是否在空闲链表中
        bool found = false;
        auto& nextList = freeSpans_[nextSpan->numPages];
        // 检查是否是头节点
        if (nextList == nextSpan)
        {
            nextList = nextSpan->next;
            found = true;
        }
        else if (nextList)
        {
            Span* prev = nextList;
            while (prev->next)
            {
                if (prev->next == nextSpan)
                {   // 将nextSpan从空闲链表中移除
                    prev->next = nextSpan->next;
                    found = true;
                    break;
                }
                prev = prev->next;
            }
        }
        // 只有在找到nextSpan的情况下才进行合并
        if (found)
        {
            span->numPages += nextSpan->numPages;
            spanMap_.erase(nextAddr);
            delete nextSpan;
        }
    }

    // 3. 将合并后的span通过头插法插入空闲列表
    auto& list = freeSpans_[span->numPages];
    span->next = list;
    list = span;
}

// 向操作系统申请numPages页的内存（使用mmap）
void* PageCache::systemAlloc(size_t numPages)
{
    size_t size = numPages * PAGE_SIZE;
    // 使用mmap分配物理内存，匿名映射
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return nullptr;
    // 清零内存，防止脏数据
    memset(ptr, 0, size);
    return ptr;
}

} // namespace mempool