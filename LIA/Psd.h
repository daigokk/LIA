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
        oldFreq = pSettings->awg.ch[0].freq;
        init();
    }
    void init()
    {
        oldFreq = pSettings->awg.ch[0].freq;
        size_t halfPeriodSize = (size_t)(0.5 / oldFreq / RAW_DT);
        size = halfPeriodSize * (size_t)(RAW_SIZE / halfPeriodSize);
        //#pragma omp parallel for
        for (int i = 0; i < size; i++)
        {
            this->_sin[i] = 2 * std::sin(2 * std::numbers::pi * oldFreq * i * RAW_DT);
            this->_cos[i] = 2 * std::cos(2 * std::numbers::pi * oldFreq * i * RAW_DT);
        }
    }
    void calc(const double t)
    {
        if (oldFreq != pSettings->awg.ch[0].freq) init();
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
            _x1 /= size; _y1 /= size;
            if (pSettings->flagAutoOffset)
            {
                pSettings->post.offset1X = _x1; pSettings->post.offset1Y = _y1;
                pSettings->flagAutoOffset = false;
            }
            _x1 -= pSettings->post.offset1X; _y1 -= pSettings->post.offset1Y;
            double theta1 = pSettings->post.offset1Phase / 180 * std::numbers::pi;
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
            _x1 /= size; _y1 /= size;
            _x2 /= size; _y2 /= size;
            if (pSettings->flagAutoOffset)
            {
                pSettings->post.offset1X = _x1; pSettings->post.offset1Y = _y1;
                pSettings->post.offset2X = _x2; pSettings->post.offset2Y = _y2;
                pSettings->flagAutoOffset = false;
            }
            _x1 -= pSettings->post.offset1X; _y1 -= pSettings->post.offset1Y;
            _x2 -= pSettings->post.offset2X; _y2 -= pSettings->post.offset2Y;
            double theta1 = pSettings->post.offset1Phase / 180 * std::numbers::pi;
            double theta2 = pSettings->post.offset2Phase / 180 * std::numbers::pi;
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
