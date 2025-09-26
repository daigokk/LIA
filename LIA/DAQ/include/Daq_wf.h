#pragma once

#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <format>
#include <stdexcept>
#include <cmath>
#include <numbers> // For std::numbers::pi
#include "inicpp.h"

#ifdef DAQ
#include <Daq_wf.h>
#endif // DAQ

constexpr float RAW_RANGE = 2.5f; // AD3: +-2.5 or +-25V
constexpr double RAW_DT = 1e-8; // Most fase dt is 1e-8.
constexpr size_t RAW_SIZE = 5000; // Maximum size of AD2 is 8192.
constexpr double MEASUREMENT_DT = 2.0e-3;
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
        //std::cerr << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        //std::cerr << e.what() << std::endl;
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
    Daq_wf* pDaq = nullptr;
#endif // DAQ
    // Monitor
    float monitorScale = 1.0f;
    // Window
    int windowWidth = 1440;
    int windowHeight = 960;
    int windowPosX = 0;
    int windowPosY = 30;
    // Fg
    float fgFreq = 100e3, fg1Amp = 1.0, fg2Amp = 0.0, fg2Phase = 0.0;
    // Scope
    const double rawDt = RAW_DT;
    bool flagCh2 = false;
    // LIA
    double offset1Phase = 0, offset1X = 0, offset1Y = 0;
    double offset2Phase = 0, offset2X = 0, offset2Y = 0;
    HighPassFilter hpfX1, hpfY1, hpfX2, hpfY2;
    bool flagAutoOffset = false, flagPause = false;
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
    std::array<double, MEASUREMENT_SIZE> times, x1s, y1s, x2s, y2s, dts;// , abs1, abs2;
    int xyNorm = 0, xyIdx = 0, xyTail = 0, xySize;
    std::array<double, XY_SIZE> xy1Xs, xy1Ys, xy2Xs, xy2Ys;
    

    Settings()
    {
        ini::IniFile liaIni("lia.ini");
        windowWidth = (int)conv(liaIni["Window"]["windowWidth"].as<std::string>(), windowWidth);
        windowHeight = (int)conv(liaIni["Window"]["windowHeight"].as<std::string>(), windowHeight);
        windowPosX = (int)conv(liaIni["Window"]["windowPosX"].as<std::string>(), windowPosX);
        windowPosY = (int)conv(liaIni["Window"]["windowPosY"].as<std::string>(), windowPosY);
        fgFreq = (float)conv(liaIni["Fg"]["fgFreq"].as<std::string>(), fgFreq);
        float lowLimitFreq = (float)(0.5 / (RAW_SIZE * rawDt));
        float highLimitFreq = (float)(1.0 / (1000 * rawDt));
        if (fgFreq < lowLimitFreq) fgFreq = lowLimitFreq;
        if (fgFreq > highLimitFreq) fgFreq = highLimitFreq;
        fg1Amp = (float)conv(liaIni["Fg"]["fg1Amp"].as<std::string>(), fg1Amp);
        fg2Amp = (float)conv(liaIni["Fg"]["fg2Amp"].as<std::string>(), fg2Amp);
        if (fg1Amp < 0.1f) fg1Amp = 0.1f;
        if (fg1Amp > 5.0f) fg1Amp = 5.0f;
        if (fg2Amp < 0.0f) fg2Amp = 0.0f;
        if (fg2Amp > 5.0f) fg2Amp = 5.0f;
        fg2Phase = (float)conv(liaIni["Fg"]["fg2Phase"].as<std::string>(), fg2Phase);
        flagCh2 = convb(liaIni["Scope"]["flagCh2"].as<std::string>(), flagCh2);
        offset1Phase = conv(liaIni["Lia"]["offset1Phase"].as<std::string>(), offset1Phase);
        offset1X = conv(liaIni["Lia"]["offset1X"].as<std::string>(), offset1X);
        offset1Y = conv(liaIni["Lia"]["offset1Y"].as<std::string>(), offset1Y);
        offset2Phase = conv(liaIni["Lia"]["offset2Phase"].as<std::string>(), offset2Phase);
        offset2X = conv(liaIni["Lia"]["offset2X"].as<std::string>(), offset2X);
        offset2Y = conv(liaIni["Lia"]["offset2Y"].as<std::string>(), offset2Y);
        hpFreq = (float)conv(liaIni["Lia"]["hpFreq"].as<std::string>(), hpFreq);
        rangeSecTimeSeries = conv(liaIni["Plot"]["Measurement_rangeSecTimeSeries"].as<std::string>(), rangeSecTimeSeries);
        limit = (float)conv(liaIni["Plot"]["limit"].as<std::string>(), limit);
        if (limit < 0.0f) limit = 0.0f;
        if (limit > 5.0f) limit = 5.0f;
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
        //abs1[tail] = pow(x1s[tail] * x1s[tail] + y1s[tail] * y1s[tail], 0.5);
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
        //abs1[tail] = pow(x1s[tail] * x1s[tail] + y1s[tail] * y1s[tail], 0.5);
        //abs2[tail] = pow(x2s[tail] * x2s[tail] + y2s[tail] * y2s[tail], 0.5);
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
        if (!flagCh2)
        {
            outputFile << "# t(s), x(V), y(V)" << std::endl;
        }
        else {
            outputFile << "# t(s), x1(V), y1(V), x2(V), y2(V)" << std::endl;
        }
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
            if (!flagCh2)
            {
                outputFile << std::format("{:e},{:e},{:e}\n", times[idx], x1s[idx], y1s[idx]);
            }
            else {
                outputFile << std::format("{:e},{:e},{:e},{:e},{:e}\n", times[idx], x1s[idx], y1s[idx], x2s[idx], y2s[idx]);
            }
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
        liaIni["Scope"]["flagCh2"] = this->flagCh2;
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
