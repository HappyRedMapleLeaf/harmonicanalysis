#include "audio.hpp"

#include <cassert>
#include <cmath>

AudioPlayer::AudioPlayer() : CircBuf1Min(0.0, 2048) {}

int AudioPlayer::pa_callback(const void *input, void *output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags) {
    assert(output != NULL);

    float **out = static_cast<float**>(output);

    for (size_t i = 0; i < frameCount; ++i) {
        float sample = this->pop();
        out[0][i] = sample; // left
        out[1][i] = sample; // right (same sample)
    }

    return paContinue;
}

void audio_thread_func(Shared<AudioSettings> &settings) {
    AudioPlayer player{};

    // global portaudio init
    portaudio::AutoSystem autoSys;
    portaudio::System &sys = portaudio::System::instance();
    portaudio::Device &device = sys.defaultOutputDevice();

    // set up stream params
    portaudio::DirectionSpecificStreamParameters outParams(
        device, 2, portaudio::FLOAT32, false,
        device.defaultLowOutputLatency(), NULL);

    // setting a constant buffer size instead of paFramesPerBufferUnspecified fixed
    // distortion after opening stream when playing something else on same device
    const portaudio::StreamParameters params(
        portaudio::DirectionSpecificStreamParameters::null(), outParams,
        SAMPLE_RATE_HZ, 256, paNoFlag);

    // create and open stream, set callback
    portaudio::MemFunCallbackStream<AudioPlayer> stream(params, player, &AudioPlayer::pa_callback);
    stream.start();
    
    double next_sample = 0.0;
    double time_rad = 0.0;
    while (!settings.stinky.stop) {
        settings.refresh_before_read();
        AudioSettings &s = settings.stinky;

        if (!s.playing || player.push(next_sample) != 0) {
            continue;
        }

        if (s.ideal_sawtooth) {
            // lol??
            double time_s = time_rad / (2 * std::numbers::pi);
            double period_s = 1 / s.frequency;
            double phase = std::fmod(time_s - (period_s / 2.0), period_s);
            next_sample = ((s.amplitude * 2.0 * phase) / period_s) - s.amplitude;
        } else {
            // sawtooth harmonic amplitudes =
            // (2*A)/(pi*n), negative if n is even (n = 1 is fundamental)
            const double amp2_pi = 2.0 * s.amplitude / std::numbers::pi;

            next_sample = 0.0;
            double current_freq = 0.0;
            for (int n = 1; n <= s.harmonics; n++) {
                current_freq += s.frequency; // set up next harmonic

                double amplitude = amp2_pi / n;
                if (n % 2 == 0) {
                    amplitude = -amplitude;
                }

                next_sample += amplitude * std::sin(time_rad * current_freq);
            }
        }

        time_rad += SAMPLE_TIME_RAD;
    }

    stream.stop();
}
