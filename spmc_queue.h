#pragma once
#include <atomic>

namespace zjwang
{

using ull = unsigned long long;

template<typename T, uint32_t CAPACITY = 1024>
class SPMCQueue
{
public:
    SPMCQueue() {}
    ~SPMCQueue() {}

    // 用了std::atomic, 起手就是一个disable copy and move
    SPMCQueue(const SPMCQueue &) = delete;
    SPMCQueue& operator = (const SPMCQueue &) = delete;
    SPMCQueue(SPMCQueue &&) = delete;
    SPMCQueue& operator = (SPMCQueue &&) = delete;

public:
    struct Vistor
    {
        T *read() {
            auto &block = q->mBlocks[(idx + 1) % CAPACITY];
            ull newIdx = block.idx.load();
            if (newIdx <= idx) {
                return nullptr;
            }
            data = block.data;
            if (block.idx.load() != newIdx) {
                return nullptr;
            }
            idx = newIdx;
            return &data;
        }

        SPMCQueue<T, CAPACITY> *q;
        ull idx;
        T data;
    };

public:
    Vistor getVistor() {
        Vistor vistor;
        vistor.q = this;
        vistor.idx = mWriteIdx.load(std::memory_order_relaxed);
        return vistor;
    }

    template<typename Writer>
    void write(Writer write) {
        auto &block = mBlocks[++mWriteIdx % CAPACITY];
        write(block.data);
        block.idx.store(mWriteIdx);
    }


private:
    friend class Vistor;

    static constexpr uint32_t kCachelineSize = 64;
    
    struct alignas(kCachelineSize) Block
    {
        std::atomic<ull> idx {0};
        T data;
    } mBlocks[CAPACITY];

    alignas(kCachelineSize) std::atomic<ull> mWriteIdx;
};

} // zjwang