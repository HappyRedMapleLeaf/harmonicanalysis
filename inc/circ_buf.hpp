#pragma once

#include <vector>
#include <cstdlib>
#include <atomic>

#include "shared.hpp"

/*
 * contains various synchronized circular buffer implementations
 * for different situations
 * is currently kind of messy, very specialized and has weird rules
 */

// has the property that there is always at least 1 element so that if consumer is faster than producer,
// we give the consumer the last element but don't remove it
//
// IMPORTANT:
// only ONE producer thread and ONE consumer thread allowed - this algorithm is designed
// such that push/pop at the same time is okay, but push/push and pop/pop has not been considered.
class CircBuf1Min {
public:
    const size_t capacity;

    CircBuf1Min(float start_sample, size_t cap);
    float pop();
    int push(float sample); // returns 0 if success, 1 if buffer full
    size_t size();

private:
    std::vector<float> _data;
    size_t _out_idx;
    size_t _in_idx;
    std::atomic_size_t _size;
};

// producer is slave, reader is master
// keeps holds last n elements
class CircBufInOnly {
public:




    const size_t capacity;

    // if freeze is true, producer can't add data
    // if freeze is false, reader can't copy data
    // currently does not support reader freezing
    std::atomic_bool freeze;

    CircBufInOnly(size_t cap);

    // return: 0 if pushed, 1 if frozen
    int push(float sample);

    // return: 0 if copied, 1 if not frozen
    int copy(std::vector<float> &out);

    // buf[0] is latest sample, buf[1] is second latest, etc
    // only for use by producer
    float operator[](size_t index);

private:
    std::vector<float> _data;
    size_t _in_idx;
};
