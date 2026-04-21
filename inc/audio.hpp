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

struct AudioSettings {
    bool stop;
    bool playing;
    float frequency;
    float amplitude;
    int harmonics;
    bool ideal_sawtooth;
};

struct Capture {
    struct Settings {
        size_t num_samples;
    };

    Shared<Settings> settings;
    CircBufInOnly buffer;
    std::atomic_bool request;
};

struct AudioStats {
    uint64_t ns_per_frame;
};

struct AudioGuiInterface {
    Shared<AudioSettings> settings; // audio settings. GUI -> Audio
    Capture capture;                // waveform capture, bidirectional
    Shared<AudioStats> stats;       // performance monitoring data
};

void audio_thread_func(AudioGuiInterface &data);
