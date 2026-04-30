#pragma once

#include <cmath>
#include <numbers>      // std::numbers::pi
#include <numeric>      // std::inner_product
#include <execution>    // std::execution::par
#include <valarray>



class Psd
{
private:
    std::valarray<double> sinTable_;
    std::valarray<double> cosTable_;
    double samplingInterval_ = 0.0;
    size_t sampleSize_ = 0;
    // キャッシュ: 位相回転用の sin/cos を保持して再計算を避ける
    mutable double cachedPhaseDeg_ = std::numeric_limits<double>::quiet_NaN();
    mutable double cachedSin_ = 0.0;
    mutable double cachedCos_ = 1.0;

public:
    double currentFreq = 0.0;
	double currentPhase_deg = 0.0;
    void initialize(double frequency, double samplingInterval, size_t sampleSize)
    {
        if (currentFreq == frequency &&
            samplingInterval_ == samplingInterval &&
            sampleSize_ == sampleSize)
        {
            return; // 再計算不要
        }

        currentFreq = frequency;
        samplingInterval_ = samplingInterval;
        sampleSize_ = sampleSize;

        const size_t halfPeriodSamples = static_cast<size_t>(0.5 / currentFreq / samplingInterval_);
        const size_t usableSize = halfPeriodSamples * (sampleSize_ / halfPeriodSamples);

        sinTable_.resize(usableSize);
        cosTable_.resize(usableSize);

        const double angularFreq = 2.0 * std::numbers::pi * currentFreq;

        // Use trig recurrence to avoid calling sin/cos each iteration.
        // This is faster for large usableSize and friendly to auto-vectorization.
        const double delta = angularFreq * samplingInterval_;
        const double sinDelta = std::sin(delta);
        const double cosDelta = std::cos(delta);

        // start at angle 0
        double s = 0.0;
        double c = 1.0;
        for (size_t i = 0; i < usableSize; ++i)
        {
            sinTable_[i] = 2.0 * s; // scaled
            cosTable_[i] = 2.0 * c;
            // rotate (s,c) by delta
            const double s_next = s * cosDelta + c * sinDelta;
            const double c_next = c * cosDelta - s * sinDelta;
            s = s_next;
            c = c_next;
        }
    }

    auto calculate(const double rawData[]) const -> std::pair<double, double>
    {
        const size_t n = cosTable_.size();
        if (n == 0) return {0.0, 0.0};

        const double* pCos = &cosTable_[0];
        const double* pSin = &sinTable_[0];

        // Use std::inner_product which may be optimized; keep types explicit
        double sumX = std::inner_product(pCos, pCos + n, rawData, 0.0);
        double sumY = std::inner_product(pSin, pSin + n, rawData, 0.0);

        const double invSize = 1.0 / static_cast<double>(n);
        return { sumX * invSize, sumY * invSize };
    }

    // 位相回転を行うヘルパー関数
    auto rotate_phase(double x, double y, double phase_deg) -> std::pair<double, double>
    {
        // cachedPhaseDeg_ を用いて sin/cos の再計算を最小化
        if (cachedPhaseDeg_ != phase_deg) {
            cachedPhaseDeg_ = phase_deg;
            const double theta = cachedPhaseDeg_ / 180.0 * std::numbers::pi;
            cachedSin_ = std::sin(theta);
            cachedCos_ = std::cos(theta);
        }
        return { x * cachedCos_ - y * cachedSin_, x * cachedSin_ + y * cachedCos_ };
    }
};
