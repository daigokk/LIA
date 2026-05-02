#pragma once
#include <cmath>
#include <numbers>
#include <vector>
#include <utility>
#include <cstddef> // size_t
#include <omp.h>

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
    // ゲッターに付けると意図しないバグ（呼び出しただけで値を使わない等）を防げます。
    [[nodiscard]] double getCurrentFreq() const noexcept
    {
        return currentFreq_;
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

        const double* __restrict pCos = cosTable_.data();
        const double* __restrict pSin = sinTable_.data();

//#pragma omp simd reduction(+:sumX, sumY)
        for (size_t i = 0; i < usableSize_; ++i)
        {
            sumX += pCos[i] * rawData[i];
            sumY += pSin[i] * rawData[i];
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