#pragma once

#include <array>
#include <cstdlib>
#include <atomic>

constexpr size_t CIRCBUF_MAX_SIZE = 1024;

// has the property that there is always at least 1 element so that if consumer is faster than producer,
// we give the consumer the last element but don't remove it
//
// IMPORTANT:
// only ONE producer thread and ONE consumer thread allowed - this algorithm is designed
// such that push/pop at the same time is okay, but push/push and pop/pop has not been considered.
struct CircBuf1Min {
    std::array<float, CIRCBUF_MAX_SIZE> buf;
    size_t out_idx;
    size_t in_idx;
    std::atomic_size_t size;

    float pop();
    int push(float sample); // returns 0 if success, 1 if buffer full
    CircBuf1Min(float start_sample);
    CircBuf1Min() : CircBuf1Min(0.0) {};
};
