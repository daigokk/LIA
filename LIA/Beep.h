#pragma once
#include "Wave.hpp"
#include <array>
#include <memory>

class Beep9 {
private:
    static constexpr int NumWaves = 9;
    std::array<std::unique_ptr<Wave>, NumWaves> waves;

    static constexpr std::array<double, NumWaves> Frequencies = {
        100.0, 261.626, 293.665, 329.628, 349.228,
        391.995, 440.000, 493.883, 523.251
    };

    void stopAll() {
        for (auto& wave : waves) {
            wave->stop();
        }
    }

    int getWaveIndexFromPhase(double phase) const {
        if (phase < -80) return 1;
        if (phase < -60) return 2;
        if (phase < -55) return 4;
        if (phase < 0)   return 3;
        if (phase < 30)  return 7;
        if (phase < 60)  return 5;
        if (phase >= 60) return 6;
        return -1;
    }

public:
    Beep9() {
        for (int i = 0; i < NumWaves; ++i) {
            waves[i] = std::make_unique<Wave>(4800, 1);
            waves[i]->makeSin(Frequencies[i]);
        }
    }

    void update(bool flag, double amplitude, double phase) {
        if (!flag || amplitude <= 0.1) {
            stopAll();
            return;
        }

        int index = getWaveIndexFromPhase(phase);
        stopAll();
        if (index >= 0 && index < NumWaves) {
            waves[index]->play();
        }
    }
};