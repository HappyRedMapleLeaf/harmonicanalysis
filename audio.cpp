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

        // generate shit and fill buffer
        if (!s.playing) {
            continue;
        }

        if (player.push(next_sample) != 0) {
            continue;
        }

        next_sample = s.amplitude * std::sin(time_rad * s.frequency);

        time_rad += SAMPLE_TIME_RAD;
    }

    stream.stop();
}
