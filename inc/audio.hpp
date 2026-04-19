#pragma once

#include <atomic>
#include <numbers>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "circ_buf.hpp"

constexpr size_t SINESTATE_BUF_SIZE = 1024;
constexpr double SAMPLE_RATE_HZ = 48000;
constexpr double SAMPLE_TIME_S = 1.0 / SAMPLE_RATE_HZ;
constexpr double SAMPLE_TIME_RAD = SAMPLE_TIME_S * 2 * std::numbers::pi;

// shared between audio generator thread and portaudio callback
// contains portaudio callback function
// used to transfer audio samples but nothing else
class AudioPlayer : public CircBuf1Min {
public:
    AudioPlayer();

    int pa_callback(const void *input, void *output, unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags);
};

struct AudioSettings {
    bool stop;
    bool playing;
    float frequency;
    float amplitude;
    int harmonics;
    bool ideal_sawtooth;
};

// double buffer for one-directional lock-free data transfer between threads
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

    // called by consumer
    void refresh_before_read() {
        if (dirty.exchange(false)) {
            stinky = fresh;
        }
    }

private:
    std::atomic_bool dirty{false};
};

void audio_thread_func(Shared<AudioSettings> &settings, CircBufInOnly &out);
