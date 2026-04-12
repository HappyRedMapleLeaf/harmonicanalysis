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

#include "portaudiocpp/PortAudioCpp.hxx"

#include "circ_buf.h"

constexpr size_t SINESTATE_BUF_SIZE = 1024;
constexpr double SAMPLE_RATE_HZ = 48000;
constexpr double SAMPLE_TIME_S = 1.0 / SAMPLE_RATE_HZ;
constexpr double SAMPLE_TIME_RAD = SAMPLE_TIME_S * 2 * std::numbers::pi;

// todo: move somewhere else
class ToneGenerator {
public: // todo: i don't think everything needs to be public
    std::atomic_bool program_running; // this is NOT whether there is playback or not.
                                      // Should be on for the whole duration of the program
                                      // used to signal producer thread
    double time_rad;
    double freq_hz;
    CircBuf1Min circ_buf;

    ToneGenerator(double freq) : program_running(false), 
        time_rad(0.0), freq_hz(freq), circ_buf(0.0, 4096) {}

    int pa_callback(const void *input, void *output, unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags) {
        assert(output != NULL);

        // for some reason the memory layout of the callback is different between the C and C++??
        // todo: look into where and why and how
        float **out = static_cast<float **>(output);

        for (size_t i = 0; i < frameCount; ++i) {
            float sample = this->circ_buf.pop();
            out[0][i] = sample; // left
            out[1][i] = sample; // right (same sample)
        }

        return paContinue;
    }

    // add samples to buffer as fast as it can
    // todo: switch to something lazier - precompute one period (slightly off frequency but probs ok)
    void push_samples() {
        double next_sample = 0.0;
        while (this->program_running) {
            if (this->circ_buf.push(next_sample) != 0) {
                continue;
            }
            next_sample = 0.5 * std::sin(this->time_rad * this->freq_hz);
            this->time_rad += SAMPLE_TIME_RAD;
        }
    }
};

void sine_cli(double f) {
    ToneGenerator gen(f);

    // global portaudio init
    portaudio::AutoSystem autoSys;
    portaudio::System &sys = portaudio::System::instance();

    std::cout << "Available devies:" << std::endl;
    for (auto i = sys.devicesBegin(); i != sys.devicesEnd(); ++i) {
        portaudio::Device &dev = *i;
        std::cout << dev.index() << ": " << dev.name() << std::endl;
    }

    PaDeviceIndex dev_idx;
    std::cout << "Select device: ";
    std::cin >> dev_idx;
    portaudio::Device &device = sys.deviceByIndex(dev_idx);

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
    portaudio::MemFunCallbackStream<ToneGenerator> stream(params, gen, &ToneGenerator::pa_callback);

    bool playing = false;
    gen.program_running = true;

    std::cout << "Options:" << std::endl <<
        "p: play" << std::endl <<
        "s: stop" << std::endl <<
        "e: exit" << std::endl;
    
    std::string option = "";
    
    // create a thread that updates buffer
    std::thread sample_writer(&ToneGenerator::push_samples, &gen);

    while (option != "e") {
        std::cin >> option;
        if (option == "p") {
            // start playback if not playing
            if (!playing) {
                std::cout << "starting" << std::endl;
                stream.start();
                playing = true;
            } else {
                std::cout << "already started" << std::endl;
            }
        } else if (option == "s") {
            // stop playback if playing
            if (playing) {
                std::cout << "stopping" << std::endl;
                stream.stop();
                playing = false;
            } else {
                std::cout << "already stopped" << std::endl;
            }
        }
    }

    if (playing) {
        std::cout << "exited, stopping" << std::endl;
        stream.stop();
    }

    // signal to stop then join the thread
    gen.program_running = false;
    sample_writer.join();

    // cleanup
    
}

/// arg 1: real>0: fundamental frequency
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "requires 1 argument" << std::endl;
        return 1;
    }
    double fundamental = std::atof(argv[1]);

    try {
        sine_cli(fundamental);
    } catch (const portaudio::PaException &e) {
        std::cout << "A PortAudio error occurred: " << e.paErrorText() << std::endl;
        throw;
    } catch (const portaudio::PaCppException &e) {
        std::cout << "A PortAudioCpp error occurred: " << e.what() << std::endl;
        throw;
    }

    return 0;
}
