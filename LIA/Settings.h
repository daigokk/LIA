#pragma once

#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <cmath>
#include <numbers> // For std::numbers::pi
#include "inicpp.h"

#ifdef DAQ
#include <daq_dwf.hpp>
#endif // DAQ

constexpr float RAW_RANGE = 2.5f;
constexpr double RAW_DT = 1e-8;
constexpr size_t RAW_SIZE = 10000;
constexpr double MEASUREMENT_DT = 2e-3;
constexpr size_t MEASUREMENT_SEC = 60 * 10;
constexpr size_t MEASUREMENT_SIZE = (size_t)(MEASUREMENT_SEC / MEASUREMENT_DT);
constexpr float XY_HISTORY_SEC = 10.0f;
constexpr size_t XY_SIZE = (size_t)(XY_HISTORY_SEC / MEASUREMENT_DT);


double conv(std::string str, double defval)
{
    try {
        auto val = std::stod(str); // 例外がスローされる可能性あり
        return val;
    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }
    return defval;
}

bool convb(std::string str, bool defval)
{
    if (str.compare("false") == 0 || str.compare("False") == 0 || str.compare("FALSE") == 0) return false;
    else if (str.compare("true") == 0 || str.compare("True") == 0 || str.compare("TRUE") == 0) return true;
    return defval;
}

class HighPassFilter {
private:
    double alpha = 1;  // Filter coefficient
    double prevInput = 0;  // Previous input sample
    double prevOutput = 0; // Previous output sample
    double dt = MEASUREMENT_DT;
public:
    double cutoffFrequency = 0;
    double process(const double cutoffFrequency, const double input)
    {
        if (this->cutoffFrequency != cutoffFrequency)
        {
            // Calculate the filter coefficient
            double rc = 1.0 / (2.0 * std::numbers::pi * cutoffFrequency);
            alpha = rc / (rc + dt);
            this->cutoffFrequency = cutoffFrequency;
        }
        if (this->cutoffFrequency == 0)
        {
            prevInput = input;
            prevOutput = input;
            return input;
        }
        // Apply the high-pass filter formula
        double output = alpha * (prevOutput + input - prevInput);

        // Update previous values
        prevInput = input;
        prevOutput = output;

        return output;
    }
};


class Settings
{
private:
    
public:
#ifdef DAQ
    daq_dwf* pDaq = nullptr;
#endif // DAQ
    // Monitor
    float monitorScale = 1.0f;
    // Window
    int windowWidth = 1500;
    int windowHeight = 1000;
    int windowPosX = 0;
    int windowPosY = 30;
    // Fg
    float fgFreq = 100e3, fg1Amp = 1.0, fg2Amp = 0.0, fg2Phase = 0.0;
    // Scope
    const double rawDt = RAW_DT;
    // LIA
    double offset1Phase = 0, offset1X = 0, offset1Y = 0;
    double offset2Phase = 0, offset2X = 0, offset2Y = 0;
    HighPassFilter hpfX1, hpfY1, hpfX2, hpfY2;
    float hpFreq = 0;
    // Plot
    double rangeSecTimeSeries = 10.0;
    float limit = 1.5f, rawLimit = 1.5f, historySec = 10.0f;
    bool flagSurfaceMode = false, flagBeep = false;
    volatile bool statusMeasurement = false, statusPipe = false;
    std::string sn = "SN:XXXXXXXXXX";
    bool flagRawData2 = false;
    int nofm = 0, idx = 0, tail = 0, size = 0;
    std::array<double, RAW_SIZE> rawTime, rawData1, rawData2;
    std::array<double, MEASUREMENT_SIZE> times, x1s, y1s, x2s, y2s, dts;
    int xyNorm = 0, xyIdx = 0, xyTail = 0, xySize;
    std::array<double, XY_SIZE> xy1Xs, xy1Ys, xy2Xs, xy2Ys;
    bool flagAutoOffset = false;

    Settings()
    {
        ini::IniFile liaIni("lia.ini");
        windowWidth = (int)conv(liaIni["Window"]["windowWidth"].as<std::string>(), windowWidth);
        windowHeight = (int)conv(liaIni["Window"]["windowHeight"].as<std::string>(), windowHeight);
        windowPosX = (int)conv(liaIni["Window"]["windowPosX"].as<std::string>(), windowPosX);
        windowPosY = (int)conv(liaIni["Window"]["windowPosY"].as<std::string>(), windowPosY);
        fgFreq = (float)conv(liaIni["Fg"]["fgFreq"].as<std::string>(), fgFreq);
        float lowLimitFreq = 0.5f / (RAW_SIZE * rawDt);
        float highLimitFreq = 1.0f / (1000 * rawDt);
        if (fgFreq < lowLimitFreq) fgFreq = lowLimitFreq;
        if (fgFreq > highLimitFreq) fgFreq = highLimitFreq;
        fg1Amp = (float)conv(liaIni["Fg"]["fg1Amp"].as<std::string>(), fg1Amp);
        fg2Amp = (float)conv(liaIni["Fg"]["fg2Amp"].as<std::string>(), fg2Amp);
        if (fg1Amp < 0.1f) fg1Amp = 0.1f;
        if (fg1Amp > 5.0f) fg1Amp = 5.0f;
        if (fg2Amp < 0.0f) fg2Amp = 0.0f;
        if (fg2Amp > 5.0f) fg2Amp = 5.0f;
        fg2Phase = (float)conv(liaIni["Fg"]["fg2Phase"].as<std::string>(), fg2Phase);
        offset1Phase = conv(liaIni["Lia"]["offset1Phase"].as<std::string>(), offset1Phase);
        offset1X = conv(liaIni["Lia"]["offset1X"].as<std::string>(), offset1X);
        offset1Y = conv(liaIni["Lia"]["offset1Y"].as<std::string>(), offset1Y);
        offset2Phase = conv(liaIni["Lia"]["offset2Phase"].as<std::string>(), offset2Phase);
        offset2X = conv(liaIni["Lia"]["offset2X"].as<std::string>(), offset2X);
        offset2Y = conv(liaIni["Lia"]["offset2Y"].as<std::string>(), offset2Y);
        hpFreq = conv(liaIni["Lia"]["hpFreq"].as<std::string>(), hpFreq);
        rangeSecTimeSeries = conv(liaIni["Plot"]["Measurement_rangeSecTimeSeries"].as<std::string>(), rangeSecTimeSeries);
        limit = (float)conv(liaIni["Plot"]["limit"].as<std::string>(), limit);
        rawLimit = (float)conv(liaIni["Plot"]["rawLimit"].as<std::string>(), rawLimit);
        historySec = (float)conv(liaIni["Plot"]["historySec"].as<std::string>(), historySec);
        flagSurfaceMode = convb(liaIni["Plot"]["flagSurfaceMode"].as<std::string>(), flagSurfaceMode);
        flagBeep = convb(liaIni["Plot"]["flagBeep"].as<std::string>(), flagBeep);

        for (size_t i = 0; i < rawTime.size(); i++)
        {
            rawTime[i] = i * rawDt * 1e6;
        }
    }
    void _AddPoint(const double t)
    {
        times[tail] = t;
        dts[tail] = (times[tail] - times[idx]) * 1e3;

        idx = tail;
        nofm++;
        tail = nofm % MEASUREMENT_SIZE;
        if (nofm <= MEASUREMENT_SIZE) size = nofm;

        xyIdx = xyTail;
        xyNorm++;
        xyTail = xyNorm % XY_SIZE;
        if (xyNorm < XY_SIZE) xySize = xyNorm;
    }
    void AddPoint(const double t, const double x, const double y)
    {   
        x1s[tail] = hpfX1.process(this->hpFreq, x);
        y1s[tail] = hpfY1.process(this->hpFreq, y);
        xy1Xs[xyTail] = x1s[tail];
        xy1Ys[xyTail] = y1s[tail];
        this->_AddPoint(t);
    }
    void AddPoint(const double t, const double x1, const double y1, const double x2, const double y2)
    {
        x1s[tail] = hpfX1.process(this->hpFreq, x1);
        y1s[tail] = hpfY1.process(this->hpFreq, y1);
        x2s[tail] = hpfX2.process(this->hpFreq, x2);
        y2s[tail] = hpfY2.process(this->hpFreq, y2);
        xy1Xs[xyTail] = x1s[tail];
        xy1Ys[xyTail] = y1s[tail];
        xy2Xs[xyTail] = x2s[tail];
        xy2Ys[xyTail] = y2s[tail];
        this->_AddPoint(t);
    }
    ~Settings()
    {
        const char* filepath = "result.csv";
        std::ofstream outputFile(filepath);
        if (!outputFile)
        { // ファイルが開けなかった場合
            std::cerr << "Fail: " << filepath << std::endl;
        }
#ifndef ENABLE_ADCH2
        outputFile << "# t(s), x(V), y(V)" << std::endl;
#else
        outputFile << "# t(s), x1(V), y1(V), x2(V), y2(V)" << std::endl;
#endif
        size_t _size = this->nofm;
        size_t offsetIdx = 0;
        if (_size > this->times.size())
        {
            _size = this->times.size();
            offsetIdx = this->nofm;
        }
        for (size_t i = 0; i < _size; i++)
        {
            size_t idx = (offsetIdx + i) % this->times.size();
#ifndef ENABLE_ADCH2
            outputFile << times[idx] << ',' << x1s[idx] << ',' << y1s[idx] << std::endl;
#else
            outputFile << times[idx] << ',' << x1s[idx] << ',' << y1s[idx] << ',' << x2s[idx] << ',' << y2s[idx] << std::endl;
#endif
        }
        outputFile.close();

        ini::IniFile liaIni;
        liaIni["Window"]["windowWidth"] = this->windowWidth;
        liaIni["Window"]["windowHeight"] = this->windowHeight;
        liaIni["Window"]["windowPosX"] = this->windowPosX;
        liaIni["Window"]["windowPosY"] = this->windowPosY;
        liaIni["Fg"]["fgFreq"] = this->fgFreq;
        liaIni["Fg"]["fg1Amp"] = this->fg1Amp;
        liaIni["Fg"]["fg2Amp"] = this->fg2Amp;
        liaIni["Fg"]["fg2Phase"] = this->fg2Phase;
        liaIni["Lia"]["offset1Phase"] = this->offset1Phase;
        liaIni["Lia"]["offset1X"] = this->offset1X;
        liaIni["Lia"]["offset1Y"] = this->offset1Y;
        liaIni["Lia"]["offset2Phase"] = this->offset2Phase;
        liaIni["Lia"]["offset2X"] = this->offset2X;
        liaIni["Lia"]["offset2Y"] = this->offset2Y;
        liaIni["Lia"]["hpFreq"] = this->hpFreq;
        liaIni["Plot"]["Measurement_rangeSecTimeSeries"] = this->rangeSecTimeSeries;
        liaIni["Plot"]["limit"] = this->limit;
        liaIni["Plot"]["rawLimit"] = this->rawLimit;
        liaIni["Plot"]["historySec"] = this->historySec;
        liaIni["Plot"]["flagSurfaceMode"] = this->flagSurfaceMode;
        liaIni["Plot"]["flagBeep"] = this->flagBeep;
        liaIni.save("lia.ini");
    }
};

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
        oldFreq = pSettings->fgFreq;
        init();
    }
    void init()
    {
        oldFreq = pSettings->fgFreq;
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
        if (oldFreq != pSettings->fgFreq) init();
        double _x1 = 0, _y1 = 0, _x2 = 0, _y2 = 0;
//#pragma omp parallel for reduction(+:_x1, _y1, _x2, _y2)
        // daigokk: For OpenMP, this process is too small.
        for (int i = 0; i < size; i++)
        {
            _x1 += pSettings->rawData1[i] * this->_sin[i];
            _y1 += pSettings->rawData1[i] * this->_cos[i];
#ifdef ENABLE_ADCH2
            _x2 += pSettings->rawData2[i] * this->_sin[i];
            _y2 += pSettings->rawData2[i] * this->_cos[i];
#endif // ENABLE_ADCH2
        }
        _x1 /= this->_sin.size(); _y1 /= this->_sin.size();
#ifdef ENABLE_ADCH2
        _x2 /= this->_sin.size(); _y2 /= this->_sin.size();
#endif // ENABLE_ADCH2
        if (pSettings->flagAutoOffset)
        {
            pSettings->offset1X = _x1; pSettings->offset1Y = _y1;
#ifdef ENABLE_ADCH2
            pSettings->offset2X = _x2; pSettings->offset2Y = _y2;
#endif // ENABLE_ADCH2
            pSettings->flagAutoOffset = false;
        }
        _x1 -= pSettings->offset1X; _y1 -= pSettings->offset1Y;
#ifdef ENABLE_ADCH2
        _x2 -= pSettings->offset2X; _y2 -= pSettings->offset2Y;
#endif // ENABLE_ADCH2
        double theta1 = pSettings->offset1Phase / 180 * std::numbers::pi;
#ifndef ENABLE_ADCH2
        pSettings->AddPoint(
            t, 
            _x1 * std::cos(theta1) - _y1 * std::sin(theta1), 
            _x1 * std::sin(theta1) + _y1 * std::cos(theta1)
        );
#else
        double theta2 = pSettings->offset2Phase / 180 * std::numbers::pi;
        pSettings->AddPoint(
            t,
            _x1 * std::cos(theta1) - _y1 * std::sin(theta1),
            _x1 * std::sin(theta1) + _y1 * std::cos(theta1),
            _x2 * std::cos(theta2) - _y2 * std::sin(theta2),
            _x2 * std::sin(theta2) + _y2 * std::cos(theta2)
        );
#endif // ENABLE_ADCH2
    }
};
