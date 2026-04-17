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
#include <mutex>
#include <memory>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "circ_buf.h"

constexpr size_t SINESTATE_BUF_SIZE = 1024;
constexpr double SAMPLE_RATE_HZ = 48000;
constexpr double SAMPLE_TIME_S = 1.0 / SAMPLE_RATE_HZ;
constexpr double SAMPLE_TIME_RAD = SAMPLE_TIME_S * 2 * std::numbers::pi;

// todo: benchmark the generator
// the scuffed shared pointer thing is probably faster than using a mutex but like eehhh???

struct GeneratorConfig {
    const bool program_running;
    const double freq_hz;
};

// todo: move somewhere else
class TonePlayer {
public: // todo: i don't think everything needs to be public
    // shared between main and producer
    std::atomic<std::shared_ptr<GeneratorConfig>> gen_cfg;

    void update_cfg(const GeneratorConfig &newcfg) {
        // create new config and point to that instead
        std::atomic_store(&this->gen_cfg, std::make_shared<GeneratorConfig>(newcfg));
    }

    void update_playing(bool new_playing) {
        auto old = get_config();
        update_cfg({
            .program_running = new_playing,
            .freq_hz = old->freq_hz
        });
    }

    void update_freq(double new_freq) {
        auto old = get_config();
        update_cfg({
            .program_running = old->program_running,
            .freq_hz = new_freq
        });
    }

    std::shared_ptr<GeneratorConfig> get_config() {
        return std::atomic_load(&this->gen_cfg);
    }

    // used only by producer
    double time_rad;

    CircBuf1Min circ_buf; // shared between pa and producer

    TonePlayer(double init_freq) : time_rad(0.0), circ_buf(0.0, 4096) {
        update_cfg({
            .program_running = false,
            .freq_hz = init_freq
        });
    }

    // portaudio callback for sending data to output device
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

    // generator thread
    // add samples to buffer as fast as it can
    // todo: switch to something lazier - precompute one period (slightly off frequency but probs ok)
    void generate_samples() {
        double next_sample = 0.0;
        while (1) {
            auto cfg = get_config();
            if (!cfg->program_running) {
                break;
            }

            if (this->circ_buf.push(next_sample) != 0) {
                continue;
            }

            next_sample = 0.5 * std::sin(this->time_rad * cfg->freq_hz);

            this->time_rad += SAMPLE_TIME_RAD;
        }
    }
};

void sine_cli() {
    TonePlayer player(440);

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
    portaudio::MemFunCallbackStream<TonePlayer> stream(params, player, &TonePlayer::pa_callback);

    bool playing = false;
    player.update_playing(true);

    std::cout << "Options:" << std::endl <<
        "p: play" << std::endl <<
        "s: stop" << std::endl <<
        "f <double new_freq>: set freq" << std::endl <<
        "e: exit" << std::endl;
    
    std::string option = "";
    
    // create a thread that updates buffer
    std::thread sample_writer(&TonePlayer::generate_samples, &player);

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
        } else if (option == "f") {
            double new_freq;
            std::cin >> new_freq;
            player.update_freq(new_freq);
            // if playing, don't flush buffer to make transition smooth
            // if not playing, flush buffer so that a bit of the previous frequency doesn't spill over
            // once we play again
            if (!playing) {
                // we're safe to do this - if !playing, still only one consumer at a time using the buffer
                for (size_t i = 0; i < player.circ_buf.capacity; i++) {
                    player.circ_buf.pop();
                }
            }
        }
    }

    if (playing) {
        std::cout << "exited, stopping" << std::endl;
        stream.stop();
    }

    // signal to stop then join the thread
    player.update_playing(false);
    sample_writer.join();

    // cleanup
    
}

int main(int argc, char* argv[]) {
    try {
        sine_cli();
    } catch (const portaudio::PaException &e) {
        std::cout << "A PortAudio error occurred: " << e.paErrorText() << std::endl;
        throw;
    } catch (const portaudio::PaCppException &e) {
        std::cout << "A PortAudioCpp error occurred: " << e.what() << std::endl;
        throw;
    }

    return 0;
}
