#include "circ_buf.hpp"

#include <cassert>

CircBuf1Min::CircBuf1Min(float start_sample, size_t cap) : 
        capacity(cap), _data(cap), _out_idx(0), _in_idx(1), _size(1) {
    _data[0] = start_sample;
}

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

CircBufInOnly::CircBufInOnly(size_t init_size) :
        _size(init_size), _data(_size), _in_idx(0) {
    std::fill(_data.begin(), _data.end(), 0.0);
}

size_t CircBufInOnly::in_idx() { return _in_idx; }
size_t CircBufInOnly::size() { return _size; }

float CircBufInOnly::push(float sample) {
    float old = _data[_in_idx];
    _data[_in_idx] = sample;
    _in_idx = (_in_idx + 1) % _size;
    return old;
}

void CircBufInOnly::resize(size_t new_size) {
    assert(new_size > 0);
    std::vector<float> new_data(new_size, 0.0);

    size_t from_idx = (_size + _in_idx - 1) % _size;
    for (size_t i = 0; i < std::min(new_size, _size); i++) {
        new_data[new_size - 1 - i] = _data[from_idx];
        from_idx = (_size + from_idx - 1) % _size;
    }

    _in_idx = 0;
    _size = new_size;
    _data = std::move(new_data);
}

float CircBufInOnly::operator[](size_t index) {
    return _data[(_size + _in_idx - 1 - index) % _size];
}
