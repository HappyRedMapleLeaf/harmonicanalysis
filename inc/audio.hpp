#pragma once

#include <atomic>
#include <numbers>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "circ_buf.hpp"
#include "shared.hpp"

constexpr size_t SINESTATE_BUF_SIZE = 1024;
constexpr float SAMPLE_RATE_HZ = 48000;
constexpr float SAMPLE_TIME_S = 1.0 / SAMPLE_RATE_HZ;
constexpr float SAMPLE_TIME_RAD = SAMPLE_TIME_S * 2 * std::numbers::pi;

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

struct AudioGuiInterface {
    Shared<AudioSettings> settings;
    CircBufInOnly capture;
    std::atomic_int64_t audio_ns_per_frame;

    // this is temporary
    AudioGuiInterface(size_t cap_size) : capture(cap_size) {}
};

void audio_thread_func(AudioGuiInterface &data);
