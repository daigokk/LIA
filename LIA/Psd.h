#pragma once

#include <cmath>
#include <numbers>      // std::numbers::pi
#include <numeric>      // std::inner_product
#include <valarray>


class Psd
{
private:
    std::valarray<double> sinTable_;
    std::valarray<double> cosTable_;
    double samplingInterval_ = 0.0;
    size_t sampleSize_ = 0;

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

        for (int i = 0; i < usableSize; ++i)
        {
            double wt = angularFreq * i * samplingInterval_;
            sinTable_[i] = 2.0 * std::sin(wt);
            cosTable_[i] = 2.0 * std::cos(wt);
        }
    }

    auto calculate(const double rawData[]) const -> std::pair<double, double>
    {
		double sumX = 0.0, sumY = 0.0;
        const double* pCos = &cosTable_[0];
        const double* pSin = &sinTable_[0];
//#pragma omp parallel for reduction(+:sumX, sumY)
        for (int i = 0; i < cosTable_.size(); ++i)
        {
            sumX += pCos[i] * rawData[i];
            sumY += pSin[i] * rawData[i];
        }

        const double invSize = 1.0 / static_cast<double>(cosTable_.size());
		return { sumX * invSize, sumY * invSize };
    }

    // 位相回転を行うヘルパー関数
    auto rotate_phase(double x, double y, double phase_deg) -> std::pair<double, double>
    {
        static double currentPhase_deg = phase_deg;
        static double theta = currentPhase_deg / 180.0 * std::numbers::pi;
        static double sin_t = std::sin(theta);
        static double cos_t = std::cos(theta);
        if (currentPhase_deg != phase_deg) {
            currentPhase_deg = phase_deg;
            theta = currentPhase_deg / 180.0 * std::numbers::pi;
            sin_t = std::sin(theta);
            cos_t = std::cos(theta);
        }
        return { x * cos_t - y * sin_t, x * sin_t + y * cos_t };
    }
};
