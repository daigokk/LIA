#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <numbers>      // C++20: 数学定数 (pi)
#include <numeric>      // std::ranges::inner_product のために必要
#include <ranges>       // C++20: Rangesライブラリ
#include <span>         // C++20: span

#include "settings.h"


class Psd
{
private:
    std::array<double, RAW_SIZE> _sin_table, _cos_table;
    Settings* pSettings = nullptr;
    double currentFreq = -1.0;
    size_t dataSize = 0;

    // 位相回転を行うヘルパー関数 (C++20の機能ではないがコード整理)
    static auto rotate_phase(double x, double y, double phase_deg) -> std::pair<double, double>
    {
        const double theta = phase_deg / 180.0 * std::numbers::pi;
        const double cos_t = std::cos(theta);
        const double sin_t = std::sin(theta);
        return { x * cos_t - y * sin_t, x * sin_t + y * cos_t };
    }

public:
    explicit Psd(Settings* settings) : pSettings(settings) {}

    void init()
    {
        currentFreq = pSettings->awg.ch[0].freq;
        const size_t halfPeriodSize = static_cast<size_t>(0.5 / currentFreq / RAW_DT);
        if (halfPeriodSize == 0) {
            dataSize = 0;
            return;
        }
        dataSize = halfPeriodSize * (RAW_SIZE / halfPeriodSize);
        if (dataSize > RAW_SIZE) dataSize = RAW_SIZE;

        const double angle_delta = 2.0 * std::numbers::pi * currentFreq * RAW_DT;
        const double s = std::sin(angle_delta);
        const double c = std::cos(angle_delta);

        _sin_table[0] = 0.0;
        _cos_table[0] = 2.0;

        for (size_t i = 0; i < dataSize - 1; ++i) {
            const double sin_i = 0.5 * _sin_table[i];
            const double cos_i = 0.5 * _cos_table[i];
            _sin_table[i + 1] = 2.0 * (sin_i * c + cos_i * s);
            _cos_table[i + 1] = 2.0 * (cos_i * c - sin_i * s);
        }
    }

    void calc(const double t)
    {
        if (currentFreq != pSettings->awg.ch[0].freq) {
            init();
        }
        if (dataSize == 0) return;

        // C++20: std::span を使ってデータへの安全なビューを作成
        std::span<const double> raw1_span(pSettings->rawData1);
        std::span<const double> sin_span(_sin_table);
        std::span<const double> cos_span(_cos_table);

        // C++20: 計算範囲を .first() で指定
        auto raw1_view = raw1_span.first(dataSize);
        auto sin_view = sin_span.first(dataSize);
        auto cos_view = cos_span.first(dataSize);

        // 修正: 第2引数をRange(span)から、その開始イテレータに変更
        double x1 = std::inner_product(raw1_view.begin(), raw1_view.end(), sin_view.begin(), 0.0);
        double y1 = std::inner_product(raw1_view.begin(), raw1_view.end(), cos_view.begin(), 0.0);

        x1 /= dataSize;
        y1 /= dataSize;

        double x2 = 0.0, y2 = 0.0;
        if (pSettings->flagCh2) {
            std::span<const double> raw2_span(pSettings->rawData2);
            auto raw2_view = raw2_span.first(dataSize);

            // こちらも同様に修正
            x2 = std::inner_product(raw2_view.begin(), raw2_view.end(), sin_view.begin(), 0.0);
            y2 = std::inner_product(raw2_view.begin(), raw2_view.end(), cos_view.begin(), 0.0);

            x2 /= dataSize;
            y2 /= dataSize;
        }

        // (以降のロジックは変更なし)
        if (pSettings->flagAutoOffset) {
            pSettings->post.offset1X = x1; pSettings->post.offset1Y = y1;
            if (pSettings->flagCh2) {
                pSettings->post.offset2X = x2; pSettings->post.offset2Y = y2;
            }
            pSettings->flagAutoOffset = false;
        }

        x1 -= pSettings->post.offset1X;
        y1 -= pSettings->post.offset1Y;

        auto [final_x1, final_y1] = rotate_phase(x1, y1, pSettings->post.offset1Phase);

        if (pSettings->flagCh2) {
            x2 -= pSettings->post.offset2X;
            y2 -= pSettings->post.offset2Y;
            auto [final_x2, final_y2] = rotate_phase(x2, y2, pSettings->post.offset2Phase);
            pSettings->AddPoint(t, final_x1, final_y1, final_x2, final_y2);
        }
        else {
            pSettings->AddPoint(t, final_x1, final_y1);
        }
    }
};