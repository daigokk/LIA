#pragma once
#include "settings.h"


class Psd
{
private:
    std::array<double, RAW_SIZE> _sin, _cos;
    Settings* pSettings = nullptr;
    double oldFreq;
    size_t size;
public:
    Psd(Settings* pSettings)
    {
        this->pSettings = pSettings;
        oldFreq = pSettings->w1Freq;
        init();
    }
    void init()
    {
        oldFreq = pSettings->w1Freq;
        size_t halfPeriodSize = (size_t)(0.5 / oldFreq / pSettings->rawDt);
        size = halfPeriodSize * (size_t)(RAW_SIZE / halfPeriodSize);
        //#pragma omp parallel for
        for (int i = 0; i < size; i++)
        {
            this->_sin[i] = 2 * std::sin(2 * std::numbers::pi * oldFreq * i * pSettings->rawDt);
            this->_cos[i] = 2 * std::cos(2 * std::numbers::pi * oldFreq * i * pSettings->rawDt);
        }
    }
    void calc(const double t)
    {
        if (oldFreq != pSettings->w1Freq) init();
        double _x1 = 0, _y1 = 0, _x2 = 0, _y2 = 0;
        if (!pSettings->flagCh2)
        {
            //#pragma omp parallel for reduction(+:_x1, _y1) num_threads(4)
                        // daigokk: For OpenMP, this process may be too small.
            for (int i = 0; i < size; i++)
            {
                _x1 += pSettings->rawData1[i] * this->_sin[i];
                _y1 += pSettings->rawData1[i] * this->_cos[i];
            }
            _x1 /= this->_sin.size(); _y1 /= this->_sin.size();
            if (pSettings->flagAutoOffset)
            {
                pSettings->offset1X = _x1; pSettings->offset1Y = _y1;
                pSettings->flagAutoOffset = false;
            }
            _x1 -= pSettings->offset1X; _y1 -= pSettings->offset1Y;
            double theta1 = pSettings->offset1Phase / 180 * std::numbers::pi;
            pSettings->AddPoint(
                t,
                _x1 * std::cos(theta1) - _y1 * std::sin(theta1),
                _x1 * std::sin(theta1) + _y1 * std::cos(theta1)
            );
        }
        else {
            //#pragma omp parallel for reduction(+:_x1, _y1, _x2, _y2) num_threads(4)
                        // daigokk: For OpenMP, this process may be too small.
            for (int i = 0; i < size; i++)
            {
                _x1 += pSettings->rawData1[i] * this->_sin[i];
                _y1 += pSettings->rawData1[i] * this->_cos[i];
                _x2 += pSettings->rawData2[i] * this->_sin[i];
                _y2 += pSettings->rawData2[i] * this->_cos[i];
            }
            _x1 /= this->_sin.size(); _y1 /= this->_sin.size();
            _x2 /= this->_sin.size(); _y2 /= this->_sin.size();
            if (pSettings->flagAutoOffset)
            {
                pSettings->offset1X = _x1; pSettings->offset1Y = _y1;
                pSettings->offset2X = _x2; pSettings->offset2Y = _y2;
                pSettings->flagAutoOffset = false;
            }
            _x1 -= pSettings->offset1X; _y1 -= pSettings->offset1Y;
            _x2 -= pSettings->offset2X; _y2 -= pSettings->offset2Y;
            double theta1 = pSettings->offset1Phase / 180 * std::numbers::pi;
            double theta2 = pSettings->offset2Phase / 180 * std::numbers::pi;
            pSettings->AddPoint(
                t,
                _x1 * std::cos(theta1) - _y1 * std::sin(theta1),
                _x1 * std::sin(theta1) + _y1 * std::cos(theta1),
                _x2 * std::cos(theta2) - _y2 * std::sin(theta2),
                _x2 * std::sin(theta2) + _y2 * std::cos(theta2)
            );
        }
    }
};
