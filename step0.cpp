#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cassert>
#include <array>
#include <cmath>
#include <numbers>
#include <cstdint>

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_NO_WCHAR
#include "external/dr_wav.h"

void calculate_harmonics(double fundamental, int n_harmonics, std::vector<double>& result) {
    assert(result.size() == n_harmonics);
    result[0] = fundamental;
    for (int i = 1; i < n_harmonics; i++) {
        result[i] = result[i-1] + result[0];
    }
}

/// arg 1: real>0: fundamental frequency
/// arg 2: int>0: number of harmonics (1 meaning only fundamental)
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "requires 2 arguments" << std::endl;
        return 1;
    }
    double fundamental = std::atof(argv[1]);
    int n_harmonics = std::atoi(argv[2]);

    std::cout << "doing some magic with " << n_harmonics << " harmonics of " << fundamental << "Hz" << std::endl;

    std::vector<double> f_h(n_harmonics);
    calculate_harmonics(fundamental, n_harmonics, f_h);
    
    constexpr int SAMPLE_RATE_HZ = 44100;
    constexpr double SAMPLE_TIME_S = 1.0 / SAMPLE_RATE_HZ;
    constexpr int NUM_SAMPLES = 5 * SAMPLE_RATE_HZ; // 5 seconds of audio

    constexpr double A = 1.0; // amplitude of sawtooth, 0 to max
    constexpr double A2_pi = 2.0 * A / std::numbers::pi;

    // amplitudes: (2*A)/(pi*n), negative if n is even
    // phases: all 0

    // this is extraordinarily stupid, probably
    std::array<int16_t, NUM_SAMPLES> waveform = {};
    double time_rad = 0.0;
    constexpr double SAMPLE_TIME_RAD = SAMPLE_TIME_S * 2 * std::numbers::pi;
    for (int sample = 0; sample < NUM_SAMPLES; sample++) {
        // generate sample value
        double raw_sample = 0.0;
        for (int freq_idx = 0; freq_idx < n_harmonics; freq_idx++) {
            // note that n in fourier series is freq_idx + 1
            double amplitude = A2_pi / (freq_idx + 1);
            if (freq_idx % 2 == 1) {
                amplitude = -amplitude;
            }

            raw_sample += amplitude * std::sin(time_rad * f_h[freq_idx]);
        }
        time_rad += SAMPLE_TIME_RAD;

        // quantize to int16
        constexpr double ratio = (1 << 14) / A;
        waveform[sample] = static_cast<int16_t>(std::clamp(ratio * raw_sample, -32768.0, 32767.0));
    }

    // write samples to file
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;     // normal WAV file
    format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = 1;
    format.sampleRate = 44100;
    format.bitsPerSample = 16;
    drwav_init_file_write(&wav, "data/recording.wav", &format, NULL);
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, NUM_SAMPLES, waveform.data());
    drwav_uninit(&wav);

    std::cout << "wrote " << framesWritten << " frames" << std::endl;

    return 0;
}