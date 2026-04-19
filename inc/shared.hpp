#pragma once

#include <atomic>

// double buffer for one-directional lock-free data transfer between threads
// todo: this doesn't fucking work lol. fresh can be mid-copy when consumer copies it to stinky
template<typename T>
struct Shared {
public:
    // written by producer
    T fresh;

    // read by consumer
    T stinky;

    // called by producer
    void refresh_after_write() {
        dirty.store(true);
    }

    // called by consumer. returns whether data has been updated
    bool refresh_before_read() {
        if (dirty.exchange(false)) {
            stinky = fresh;
            return true;
        }
        return false;
    }

private:
    std::atomic_bool dirty{false};
};