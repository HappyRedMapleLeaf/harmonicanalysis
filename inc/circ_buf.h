#pragma once

#include <vector>
#include <cstdlib>
#include <atomic>

// has the property that there is always at least 1 element so that if consumer is faster than producer,
// we give the consumer the last element but don't remove it
//
// IMPORTANT:
// only ONE producer thread and ONE consumer thread allowed - this algorithm is designed
// such that push/pop at the same time is okay, but push/push and pop/pop has not been considered.
class CircBuf1Min {
public:
    constexpr static size_t CIRCBUF_DEFAULT_SIZE = 1024;

    CircBuf1Min(float start_sample, size_t capacity);
    CircBuf1Min();
    float pop();
    int push(float sample); // returns 0 if success, 1 if buffer full

private:
    std::vector<float> _data;
    size_t _out_idx;
    size_t _in_idx;
    size_t _capacity;
    std::atomic_size_t _size;
};
