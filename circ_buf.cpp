#include "circ_buf.h"

#include <cassert>

// todo: optimize with batch reads
// maybe support multi producer but i can't see a world where i need to do that
float CircBuf1Min::pop() {
    if (this->_size == 1) {
        // since we assume only 1 thread can pop, either size is actually 1, or it is greater.
        // either way we're safe to return and not remove the latest value
        return this->_data[this->_out_idx];
    }

    // now, size is definitely greater than 1, safe to pop
    float ret = this->_data[this->_out_idx];
    this->_out_idx = (this->_out_idx + 1) % this->capacity;

    // decrement size at the end once we've already read the data,
    // otherwise producer might overwrite it
    this->_size--;

    return ret;
}

int CircBuf1Min::push(float sample) {
    if (this->_size == this->capacity) {
        // since we assume only 1 thread can push, either buffer is actually full, or it is just
        // about to be popped from. Either way we're safe to abort
        return 1;
    }

    // now, size is definitely less than full, safe to push
    this->_data[this->_in_idx] = sample;
    this->_in_idx = (this->_in_idx + 1) % this->capacity;

    // increment size at the end once we've already added data,
    // otherwise consumer might read from the spot we haven't written to yet
    this->_size++;

    return 0;
}

size_t CircBuf1Min::size() {
    return this->_size;
}

CircBuf1Min::CircBuf1Min(float start_sample, size_t cap) : 
        _data(capacity), capacity(cap), _size(1), _out_idx(0), _in_idx(1) {
    this->_data[0] = start_sample;
}

CircBuf1Min::CircBuf1Min() : CircBuf1Min(0.0, CIRCBUF_DEFAULT_SIZE) {};