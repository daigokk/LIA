#pragma once
#include <cmath>
#include <numbers>
#include <vector>
#include <utility>
#include <cstddef> // size_t
//#include <omp.h>
#include <cassert>

class Psd
{
private:
    std::vector<double> sinTable_;
    std::vector<double> cosTable_;

    double samplingInterval_ = 0.0;
    double currentFreq_ = 0.0;
    size_t sampleSize_ = 0;
    size_t usableSize_ = 0;
    double invSize_ = 0.0;

    double currentPhase_deg_ = 0.0;
    double sin_t_ = 0.0;
    double cos_t_ = 1.0;

public:
    // [[nodiscard]] は「戻り値を無視してはいけない」というコンパイラへのヒントで、
    // ゲッターに付けると意図しないバグ（呼び出しただけで値を使わない等）を防ぐ。
    [[nodiscard]] double getCurrentFreq() const noexcept
    {
        return currentFreq_;
    }

    [[nodiscard]] double getSamplingDt() const noexcept
    {
        return samplingInterval_;
    }

    void initialize(double frequency, double samplingInterval, size_t sampleSize)
    {
        if (currentFreq_ == frequency &&
            samplingInterval_ == samplingInterval &&
            sampleSize_ == sampleSize)
        {
            return;
        }

        currentFreq_ = frequency;
        samplingInterval_ = samplingInterval;
        sampleSize_ = sampleSize;

        const size_t halfPeriodSamples = static_cast<size_t>(0.5 / (currentFreq_ * samplingInterval_));
        usableSize_ = halfPeriodSamples * (sampleSize_ / halfPeriodSamples);

        if (usableSize_ == 0) {
            invSize_ = 0.0;
            return;
        }

        invSize_ = 1.0 / static_cast<double>(usableSize_);

        sinTable_.resize(usableSize_);
        cosTable_.resize(usableSize_);

        const double angularFreq = 2.0 * std::numbers::pi * currentFreq_;

        double* pSin = sinTable_.data();
        double* pCos = cosTable_.data();

        for (size_t i = 0; i < usableSize_; ++i)
        {
            double wt = angularFreq * i * samplingInterval_;
            pSin[i] = 2.0 * std::sin(wt);
            pCos[i] = 2.0 * std::cos(wt);
        }
    }

    auto calculate(const double* __restrict rawData) const noexcept -> std::pair<double, double>
    {
        if (usableSize_ == 0) return { 0.0, 0.0 };

        double sumX = 0.0;
        double sumY = 0.0;

        const double* __restrict pSin = sinTable_.data();
        const double* __restrict pCos = cosTable_.data();

//#pragma omp simd reduction(+:sumX, sumY)
        for (size_t i = 0; i < usableSize_; ++i)
        {
            sumX += pSin[i] * rawData[i];
            sumY += pCos[i] * rawData[i];
        }

        return { sumX * invSize_, sumY * invSize_ };
    }

    auto rotate_phase(double x, double y, double phase_deg) noexcept -> std::pair<double, double>
    {
        if (currentPhase_deg_ != phase_deg) {
            currentPhase_deg_ = phase_deg;
            const double theta = currentPhase_deg_ * (std::numbers::pi / 180.0);
            sin_t_ = std::sin(theta);
            cos_t_ = std::cos(theta);
        }
        return { x * cos_t_ - y * sin_t_, x * sin_t_ + y * cos_t_ };
    }
};

std::vector<double> generateSignal(double freq, double phase_deg, double amplitude, double interval, size_t size) {
    std::vector<double> signal(size);
    double phase_rad = phase_deg * (std::numbers::pi / 180.0);
    for (size_t i = 0; i < size; ++i) {
        signal[i] = amplitude * std::sin(2.0 * std::numbers::pi * freq * i * interval + phase_rad);
    }
    return signal;
}

void test_psd() {
    Psd psd;

    // パラメータ設定
    const double targetFreq = 1000.0;     // 1kHz
    const double samplingRate = 100000.0; // 100kHz
    const double interval = 1.0 / samplingRate;
    const size_t sampleSize = 2000;      // 20ms分
    const double amplitude = 1.5;
    const double signalPhase = 30.0;     // 信号の初期位相 30度

    std::cout << "--- Psd Class Test Start ---" << std::endl;

    // 1. 初期化
    psd.initialize(targetFreq, interval, sampleSize);
    std::cout << "[1] Initialize: Freq=" << psd.getCurrentFreq() << " Hz" << std::endl;

    // 2. 信号生成と計算
    // 振幅1.5, 位相30度のサイン波を生成
    auto signal = generateSignal(targetFreq, signalPhase, amplitude, interval, sampleSize);

    // PSD実行 (X = A*cos(phi), Y = A*sin(phi) に近い値が出るはず)
    // ※内部で 2.0 * sin/cos を掛けているため、結果は A*cos(phi), A*sin(phi) となる
    auto [x, y] = psd.calculate(signal.data());

    std::cout << "[2] Calculate (Raw):" << std::endl;
    std::cout << "    X (Real): " << x << " (Expected: ~" << amplitude * std::cos(signalPhase * std::numbers::pi / 180.0) << ")" << std::endl;
    std::cout << "    Y (Imag): " << y << " (Expected: ~" << amplitude * std::sin(signalPhase * std::numbers::pi / 180.0) << ")" << std::endl;

    // 3. 位相回転テスト
    // 30度回転させて、位相を0度（あるいは特定の方向）へ戻すシミュレーション
    auto [rx, ry] = psd.rotate_phase(x, y, -30.0);

    std::cout << "[3] Rotate Phase (-30 deg):" << std::endl;
    std::cout << "    Rotated X: " << rx << " (Expected: ~" << amplitude << ")" << std::endl;
    std::cout << "    Rotated Y: " << ry << " (Expected: ~0.0)" << std::endl;

    // 4. 精度チェック（簡易的なアサーション）
    assert(std::abs(rx - amplitude) < 1e-3);
    assert(std::abs(ry - 0.0) < 1e-3);
    std::cout << "\nResult: PASS" << std::endl;
}
