#pragma once

#include <atomic>

// one-directional lock-free data transfer between threads
// shamelessly stolen from https://medium.com/@sgn00/triple-buffer-lock-free-concurrency-primitive-611848627a1e
template <typename T>
struct Shared {
    T& in_ref() {
        return _buf[_in_idx];
    }

    // sends in buffer to be read while keeping in buffer the same
    void refresh_after_write() {
        T temp = _buf[_in_idx]; // copy current state
        StageState prev_stage = _stage.exchange(
            StageState{_in_idx, true}
        );
        _in_idx = prev_stage.idx;
        _buf[_in_idx] = temp; // restore current state
    }

    // returns whether out data has been modified
    bool refresh_before_read() {
        if (_stage.load().has_update) {
            StageState prev_stage = _stage.exchange(
                StageState{_out_idx, false}
            );
            _out_idx = prev_stage.idx;
            return true;
        }
        return false;
    }

    T& out_ref() {
        return _buf[_out_idx];
    }

private:
    // index of stage data, and whether or
    // not stage has an update to push
    struct StageState {
        uint8_t idx;
        bool has_update;
    };

    static_assert(std::atomic<StageState>::is_always_lock_free);

    T _buf[3];
    uint8_t _out_idx{0};
    std::atomic<StageState> _stage{{1, false}};
    uint8_t _in_idx{2};
};
