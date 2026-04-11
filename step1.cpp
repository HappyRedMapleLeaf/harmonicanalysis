#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cassert>
#include <array>
#include <cmath>
#include <numbers>
#include <cstdint>
#include <string>
#include <atomic>
#include <thread>

#include "circ_buf.h"

#include "external/portaudio/include/portaudio.h"

constexpr size_t SINESTATE_BUF_SIZE = 1024;
constexpr int SAMPLE_RATE_HZ = 44100;
constexpr double SAMPLE_TIME_S = 1.0 / SAMPLE_RATE_HZ;
constexpr double SAMPLE_TIME_RAD = SAMPLE_TIME_S * 2 * std::numbers::pi;

// todo: proper error handling. maybe even consider gotos...
// todo: does the callback accept and clamp floats between +-1 or do we have to do that?
// (try it and see what happens)

struct SineState {
    std::atomic_bool program_running; // this is NOT whether there is playback or not.
                                      // Should be on for the whole duration of the program
    double time_rad;
    double freq_hz;
    CircBuf1Min buf;
};

int sine_callback(const void *input, void *output, unsigned long frameCount,
                  const PaStreamCallbackTimeInfo* timeInfo,
                  PaStreamCallbackFlags statusFlags, void *userData) {
    SineState *state = (SineState*)userData;

    // unused: timeInfo
    // unused: input

    float *out_p = (float*)output;
    
    for (size_t i = 0; i < frameCount; i++) {
        float sample = state->buf.pop();
        *(out_p++) = sample;  // left
        *(out_p++) = sample;  // right (same sample)
    }

    return 0;
}

// add samples to buffer as fast as it can
void push_samples(SineState *state) {
    double next_sample = 0.0;
    while (state->program_running) {
        if (state->buf.push(next_sample) != 0) {
            continue;
        }
        next_sample = 0.5 * std::sin(state->time_rad * state->freq_hz);
        state->time_rad += SAMPLE_TIME_RAD;
    }
}

/// arg 1: real>0: fundamental frequency
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "requires 1 argument" << std::endl;
        return 1;
    }
    double fundamental = std::atof(argv[1]);

    std::cout << "doing some magic with " << fundamental << "Hz" << std::endl;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "PortAudio could not initialize: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    PaStream *stream;

    SineState state;
    state.freq_hz = fundamental;
    state.program_running = true;
    state.time_rad = 0.0;

    // todo: https://portaudio.com/docs/v19-doxydocs/querying_devices.html
    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE_HZ, paFramesPerBufferUnspecified,
                               sine_callback, (void*)&state);
    if (err != paNoError) {
        std::cout << "PortAudio could not open default stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    bool playing = false;

    std::cout << "Options:" << std::endl <<
        "p: play" << std::endl <<
        "s: stop" << std::endl <<
        "e: exit" << std::endl;
    
    std::string option = "";
    
    // create a thread that updates buffer
    std::thread sample_writer(push_samples, &state);

    while (option != "e") {
        std::cin >> option;
        if (option == "p") {
            // start playback if not playing
            if (!playing) {
                std::cout << "starting" << std::endl;
                err = Pa_StartStream(stream);
                if (err != paNoError) {
                    std::cout << "PortAudio could not start stream: " << Pa_GetErrorText(err) << std::endl;
                    Pa_Terminate();
                    return 1;
                }
                playing = true;
            } else {
                std::cout << "already started" << std::endl;
            }
        } else if (option == "s") {
            // stop playback if playing
            if (playing) {
                std::cout << "stopping" << std::endl;
                err = Pa_StopStream(stream);
                if (err != paNoError) {
                    std::cout << "PortAudio could not stop stream: " << Pa_GetErrorText(err) << std::endl;
                    Pa_Terminate();
                    return 1;
                }
                playing = false;
            } else {
                std::cout << "already stopped" << std::endl;
            }
        }
    }

    if (playing) {
        std::cout << "exited, stopping" << std::endl;
        err = Pa_StopStream(stream);
        if (err != paNoError) {
            std::cout << "PortAudio could not stop stream: " << Pa_GetErrorText(err) << std::endl;
            Pa_Terminate();
            return 1;
        }
    }

    // signal to stop then join the thread
    state.program_running = false;
    sample_writer.join();

    // cleanup
    err = Pa_Terminate();
    if (err != paNoError) {
        std::cout << "PortAudio could not terminate: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    return 0;
}