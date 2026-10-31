#pragma once

#include <cstdint>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <cstdlib>

namespace sssv {
// Records only the emulated controller samples actually consumed by the game.
// Replay advances on controller reads, so rendering speed does not alter inputs.
class InputMovie {
    struct Sample {
        int controller;
        bool connected;
        uint16_t buttons;
        float x, y;
    };
    std::ofstream recording;
    std::vector<Sample> samples;
    size_t cursor = 0;
    bool replay = false;
    std::atomic<bool> playbackFinished{false};
    std::mutex mutex;

public:
    bool finished() const { return playbackFinished.load(std::memory_order_relaxed); }
    void finish() {
        std::lock_guard lock(mutex);
        recording.close();
    }

    void configure(const std::filesystem::path& recordPath, const std::filesystem::path& replayPath) {
        if (!recordPath.empty() && !replayPath.empty()) {
            throw std::runtime_error("Choose input recording or replay, not both.");
        }
        if (!recordPath.empty()) {
            if (std::filesystem::exists(recordPath)) {
                throw std::runtime_error("Input recording already exists; choose a new recording file.");
            }
            recording.open(recordPath);
            if (!recording) throw std::runtime_error("Could not create the input recording.");
            recording << "SSSV_INPUT_MOVIE 1\n" << std::setprecision(std::numeric_limits<float>::max_digits10);
            recording.flush();
        }
        if (!replayPath.empty()) {
            std::ifstream input(replayPath);
            std::string magic;
            unsigned version = 0;
            if (!(input >> magic >> version) || magic != "SSSV_INPUT_MOVIE" || version != 1) {
                throw std::runtime_error("Invalid input replay header.");
            }
            while (input >> std::ws && input.peek() != EOF) {
                Sample sample{};
                unsigned connected, buttons;
                if (!(input >> sample.controller >> connected >> buttons >> sample.x >> sample.y) ||
                    sample.controller < 0 || sample.controller > 3 || connected > 1 || buttons > 65535 ||
                    !(sample.x >= -1.0f && sample.x <= 1.0f) || !(sample.y >= -1.0f && sample.y <= 1.0f)) {
                    throw std::runtime_error("Invalid controller sample in input replay.");
                }
                sample.connected = connected != 0;
                sample.buttons = static_cast<uint16_t>(buttons);
                samples.push_back(sample);
            }
            if (samples.empty()) throw std::runtime_error("Input replay is empty.");
            replay = true;
        }
    }

    bool get(int controller, uint16_t* buttons, float* x, float* y,
             bool (*liveInput)(int, uint16_t*, float*, float*)) {
        std::lock_guard lock(mutex);
        // Test-only live control: a file contains buttons, stick X and stick Y.
        // Reading afresh on each poll lets an external tester change inputs
        // without relying on window focus or a prerecorded sequence.
        if (const char* path = std::getenv("SSSV_TEST_CONTROL")) {
            *buttons = 0; *x = *y = 0.0f;
            if (controller != 0) return false;
            std::ifstream input(path);
            unsigned value = 0;
            float sx = 0.0f, sy = 0.0f;
            if ((input >> value >> sx >> sy) && value <= 65535 &&
                sx >= -1.0f && sx <= 1.0f && sy >= -1.0f && sy <= 1.0f) {
                *buttons = static_cast<uint16_t>(value); *x = sx; *y = sy;
            }
            return true;
        }
        if (replay) {
            if (cursor == samples.size()) {
                // Release all buttons once the recorded menu sequence ends.
                *buttons = 0;
                *x = *y = 0.0f;
                playbackFinished.store(true, std::memory_order_relaxed);
                return true;
            }
            const Sample& sample = samples[cursor++];
            if (sample.controller != controller) {
                throw std::runtime_error("Controller read order differs from the input recording.");
            }
            if (sample.connected) {
                *buttons = sample.buttons;
                *x = sample.x;
                *y = sample.y;
            }
            return sample.connected;
        }
        const bool connected = liveInput(controller, buttons, x, y);
        if (recording) {
            recording << controller << ' ' << connected << ' ' << (connected ? *buttons : 0)
                << ' ' << (connected ? *x : 0.0f) << ' ' << (connected ? *y : 0.0f) << '\n';
            // A small controller sample is flushed so closing the game preserves it.
            recording.flush();
        }
        return connected;
    }
};
}
