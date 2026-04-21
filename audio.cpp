#include "audio.hpp"

#include <cassert>
#include <cmath>
#include <chrono>

#include "timer.hpp"

using namespace std::chrono_literals;

namespace {

// shared between audio generator thread and portaudio callback
// contains portaudio callback function
// used to transfer audio samples but nothing else
class AudioOutput : public CircBuf1Min {
public:
    AudioOutput() : CircBuf1Min(0.0, 2048) {}

    int pa_callback(const void *input, void *output, unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags) {
        assert(output != NULL);

        float **out = static_cast<float**>(output);

        for (size_t i = 0; i < frameCount; ++i) {
            float sample = pop();
            out[0][i] = sample; // left
            out[1][i] = sample; // right (same sample)
        }

        return paContinue;
    }
};

} // namespace

void audio_thread_func(AudioGuiInterface &data) {
    CircBufInOnly &capture = data.capture;

    AudioOutput output{};

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
    portaudio::MemFunCallbackStream<AudioOutput> stream(params, output, &AudioOutput::pa_callback);
    stream.start();
    
    float next_sample = 0.0;
    float phase_rad = 0.0;

    // used for triggering capture
    bool above_thres = true;
    uint64_t trigger_counter_samples = 0; // time since trigger, used to wait a
                                          // bit after triggering to capture
    bool trigger_waiting = false;
    Timer capture_timer{};

    // for performance monitoring
    Timer loop_timer{};
    Timer stats_update_timer{};

    constexpr auto TRIGGER_TIME_MAX = 1000ms; // trigger at least this often
    constexpr auto TRIGGER_TIME_MIN = 100ms;  // trigger holdoff

    while (1) {
        // refresh parameters from gui thread
        data.settings.refresh_before_read();
        AudioSettings &s = data.settings.out_ref();

        if (s.stop) {
            break;
        }

        // don't generate new samples if audio output buffer is full
        if (player.push(next_sample) != 0) {
            continue;
        }

        auto now = std::chrono::steady_clock::now();

        // performance monitoring
        // odd results; when adding more overtones, time/loop DROPS from 5ms to 3ms
        // hypothesis: we're always catching timer on slow loops when output buffer just opens up
        //             makes sense why time/loop is in ms range although we know we're going
        //             at least at 48kHz. and if normal loops take longer, these slow loops get shorter
        // todo: once we see slowdowns, change this to update max and avg time/loop
        if (stats_update_timer.has_elapsed(now, 100ms)) {
            data.stats.in_ref().ns_per_frame = loop_timer.time_elapsed(now).count();
            data.stats.refresh_after_write();
            stats_update_timer.update(now);
        }
        loop_timer.update(now);

        // fill with 0 if not playing
        if (!s.playing) {
            next_sample = 0.0;
        } else {
            // generating sample
            if (s.ideal_sawtooth) {
                next_sample = phase_rad * s.amplitude / std::numbers::pi - s.amplitude;
            } else {
                // sawtooth harmonic amplitudes =
                // (2*A)/(pi*n), negative if n is even (n = 1 is fundamental)
                const float amp2_pi = 2.0 * s.amplitude / std::numbers::pi;

                next_sample = 0.0;
                for (int n = 1; n <= s.harmonics; n++) {
                    float amplitude = amp2_pi / n;
                    if (n % 2 == 0) {
                        amplitude = -amplitude;
                    }

                    next_sample += amplitude * std::sin(phase_rad * n);
                }
            }

            // incrementing phase
            phase_rad = std::fmod(phase_rad + SAMPLE_TIME_RAD * s.frequency, std::numbers::pi * 2);
        }

        // wait for gui to unset trigger flag (if it's set, it means buffer being copied)
        // this probably (definitely) sucks but it's ok
        // todo: please fucking fix this
        while (capture.freeze.load());

        // push to internal buffer
        capture.push(next_sample);

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

        // determine if we trigger
        if (!trigger_waiting && (
                (rising_edge && capture_timer.has_elapsed(now, TRIGGER_TIME_MIN)) ||
                capture_timer.has_elapsed(now, TRIGGER_TIME_MAX)
            )) {

            trigger_waiting = true;
            capture_timer.update(now);
            trigger_counter_samples = 0;
        }

        // if we're triggering then wait half the capture then freeze
        if (trigger_waiting && ++trigger_counter_samples > capture.capacity / 2) {
            capture.freeze.store(true);
            trigger_waiting = false;
        }
    }

    stream.stop();
}
