#include "circ_buf.h"

#include <cassert>

// todo: optimize with batch reads
// maybe support multi producer but i can't see a world where i need to do that
float CircBuf1Min::pop() {
    if (this->size == 1) {
        // since we assume only 1 thread can pop, either size is actually 1, or it is greater.
        // either way we're safe to return and not remove the latest value
        return this->buf[this->out_idx];
    }

    // now, size is definitely greater than 1, safe to pop
    float ret = this->buf[this->out_idx];
    this->out_idx = (this->out_idx + 1) % CIRCBUF_MAX_SIZE;

    // decrement size at the end once we've already read the data,
    // otherwise producer might overwrite it
    size--;

    return ret;
}

int CircBuf1Min::push(float sample) {
    if (this->size == CIRCBUF_MAX_SIZE) {
        // since we assume only 1 thread can push, either buffer is actually full, or it is just
        // about to be popped from. Either way we're safe to abort
        return 1;
    }

    // now, size is definitely less than full, safe to push
    this->buf[this->in_idx] = sample;
    this->in_idx = (this->in_idx + 1) % CIRCBUF_MAX_SIZE;

    // increment size at the end once we've already added data,
    // otherwise consumer might read from the spot we haven't written to yet
    size++;

    return 0;
}

CircBuf1Min::CircBuf1Min(float start_sample) {
    this->buf[0] = start_sample;
    this->size = 1;
    this->out_idx = 0;
    this->in_idx = 1;
}
