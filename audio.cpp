#include "audio.hpp"

#include <cassert>
#include <cmath>
#include <chrono>

AudioPlayer::AudioPlayer() : CircBuf1Min(0.0, 2048) {}

int AudioPlayer::pa_callback(const void *input, void *output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags) {
    assert(output != NULL);

    float **out = static_cast<float**>(output);

    for (size_t i = 0; i < frameCount; ++i) {
        float sample = pop();
        out[0][i] = sample; // left
        out[1][i] = sample; // right (same sample)
    }

    return paContinue;
}

void audio_thread_func(Shared<AudioSettings> &in, CircBufInOnly &out) {
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

    // used for triggering capture
    bool above_thres = true;
    uint64_t prev_trigger_ms = 0; // time of previous trigger
    uint64_t trigger_counter_samples = 0; // time since trigger, used to wait a
                                          // bit after triggering to capture
    bool trigger_waiting = false;
    constexpr uint64_t TRIGGER_TIME_MAX_MS = 1000; // trigger at least this often
    constexpr uint64_t TRIGGER_TIME_MIN_MS = 100;  // trigger holdoff

    while (!in.stinky.stop) {
        // don't generate new samples if audio output buffer is full
        if (player.push(next_sample) != 0) {
            continue;
        }

        // refresh parameters from gui thread
        in.refresh_before_read();
        AudioSettings &s = in.stinky;

        // fill with 0 if not playing
        if (!s.playing) {
            next_sample = 0.0;
        } else {
            // generating sample
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

            // incrementing timer
            time_rad += SAMPLE_TIME_RAD;
        }

        // wait for gui to unset trigger flag (if it's set, it means buffer being copied)
        // this probably (definitely) sucks but it's ok
        // todo: please fucking fix this
        while (out.freeze.load());

        // push to internal buffer
        out.push(next_sample);

        uint64_t current_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        // raise trigger flag to tell gui thread to copy data out for processing
        // trigger condition: signal must have gone from below HYST_LOW to above HYST_HIGH
        const float HYST_LOW = -0.1 * s.amplitude;
        const float HYST_HIGH = 0.0;
        bool rising_edge = false;
        if (next_sample > HYST_HIGH && !above_thres) {
            above_thres = true;
            rising_edge = true;
        } else if (next_sample < HYST_LOW && above_thres) {
            above_thres = false;
        }

        if (!trigger_waiting && (
                (rising_edge && current_ms - prev_trigger_ms > TRIGGER_TIME_MIN_MS) ||
                (current_ms - prev_trigger_ms > TRIGGER_TIME_MAX_MS)
            )) {

            trigger_waiting = true;
            prev_trigger_ms = current_ms;
            trigger_counter_samples = 0;
        }

        if (trigger_waiting && ++trigger_counter_samples > out.capacity / 2) {
            out.freeze.store(true);
            prev_trigger_ms = current_ms;
            trigger_waiting = false;
        }
    }

    stream.stop();
}
