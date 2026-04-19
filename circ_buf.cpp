#include "circ_buf.hpp"

#include <cassert>

CircBuf1Min::CircBuf1Min(float start_sample, size_t cap) : 
        capacity(cap), _data(cap), _out_idx(0), _in_idx(1), _size(1) {
    _data[0] = start_sample;
}

// todo: optimize with batch reads
// maybe support multi producer but i can't see a world where i need to do that
float CircBuf1Min::pop() {
    if (_size.load() == 1) {
        // since we assume only 1 thread can pop, either size is actually 1, or it is greater.
        // either way we're safe to return and not remove the latest value
        return _data[_out_idx];
    }

    // now, size is definitely greater than 1, safe to pop
    float ret = _data[_out_idx];
    _out_idx = (_out_idx + 1) % capacity;

    // decrement size at the end once we've already read the data,
    // otherwise producer might overwrite it
    _size.fetch_sub(1);

    return ret;
}

int CircBuf1Min::push(float sample) {
    if (_size.load() == capacity) {
        // since we assume only 1 thread can push, either buffer is actually full, or it is just
        // about to be popped from. Either way we're safe to abort
        return 1;
    }

    // now, size is definitely less than full, safe to push
    _data[_in_idx] = sample;
    _in_idx = (_in_idx + 1) % capacity;

    // increment size at the end once we've already added data,
    // otherwise consumer might read from the spot we haven't written to yet
    _size.fetch_add(1);

    return 0;
}

size_t CircBuf1Min::size() {
    return _size.load();
}

CircBufInOnly::CircBufInOnly(size_t cap) :
        capacity(cap), freeze(false), _data(cap), _in_idx(0) {
    std::fill(_data.begin(), _data.end(), 0.0);
}

int CircBufInOnly::push(float sample) {
    if (freeze.load()) {
        return 1;
    }

    _data[_in_idx] = sample;
    _in_idx = (_in_idx + 1) % capacity;

    return 0;
}

int CircBufInOnly::copy(std::vector<float> &out) {
    if (!freeze.load()) {
        return 1;
    }

    out.resize(capacity);

    std::copy(
        _data.begin() + _in_idx,
        _data.end(),
        out.begin()
    );

    std::copy(
        _data.begin(),
        _data.begin() + _in_idx,
        out.begin() + (capacity - _in_idx)
    );

    return 0;
}

float CircBufInOnly::operator[](size_t index) {
    return _data[(capacity + _in_idx - 1 - index) % capacity];
}
