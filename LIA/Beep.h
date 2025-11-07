#pragma once
#include "Wave.hpp"
#include <array>
#include <memory>
#include <cmath>

#define PI acos(-1.0)

class Beep9 {
private:
    static constexpr int NumWaves = 9;
    std::array<std::unique_ptr<Wave>, NumWaves> waves;
    int current_idx = -1;
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
        static double PHASE_SCALE = 180 / (360 / NumWaves) / PI; // 約 1.43239...
        static double PHASE_OFFSET = 180 / (360 / NumWaves);

        // O(1) でインデックスを計算
        int index = static_cast<int>(std::floor(phase * PHASE_SCALE + PHASE_OFFSET));

        return index;
    }

public:
    Beep9() {
        for (int i = 0; i < NumWaves; ++i) {
            waves[i] = std::make_unique<Wave>(4800, 1);
            waves[i]->makeSin(Frequencies[i]);
        }
    }
    void update(const bool flag, const double x, const double y) {
        if (!flag || (x * x + y * y) <= pow(0.11, 2)) {
            current_idx = -1;
            stopAll();
            return;
        }

        int index = getWaveIndexFromPhase(atan2(y, x));
        if (current_idx == index) return;
        current_idx = index;
        stopAll();
        if (current_idx >= 0 && current_idx < NumWaves) {
            waves[current_idx]->play();
        }
    }
};