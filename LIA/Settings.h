#pragma once

#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <cmath>
#include "inicpp.h"
#ifdef DAQ
#include <daq_dwf.hpp>
#endif // DAQ

#define PI std::acos(-1)

#define RAW_TIME_DEFAULT 5e-5f
//#define RAW_SIZE_RESERVE 16384
#define RAW_SIZE 5000
#define MEASUREMENT_DT 2e-3
#define MEASUREMENT_SEC (60*10)
#define MEASUREMENT_SIZE (size_t)(MEASUREMENT_SEC/MEASUREMENT_DT)
#define XY_HISTORY_SEC 10.0f
#define XY_SIZE (size_t)(XY_HISTORY_SEC/MEASUREMENT_DT)


double conv(std::string str, double defval)
{
    try {
        auto val = std::stod(str); // 例外がスローされる可能性あり
        return val;
    }
    catch (const std::invalid_argument& e) {
        //std::cerr << "無効な文字列: " << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        //std::cerr << "範囲外の値: " << e.what() << std::endl;
    }
    return defval;
}

class Settings
{
public:
#ifdef DAQ
    daq_dwf* pDaq = nullptr;
#endif // DAQ
    // Monitor
    float monitorScale;
    // Window
    int windowWidth = 1500;
    int windowHeight = 900;
    int windowPosX = 0;
    int windowPosY = 30;
    // Fg
    float fgFreq = 100e3, fg1Amp = 1.0, fg2Amp = 0.0, fg2Phase = 0.0;
    // Scope
    double rawDt = 1e-8;
    // Lia
    double offset1Phase = 0, offset1X = 0, offset1Y = 0;
    double offset2Phase = 0, offset2X = 0, offset2Y = 0;
    // Plot
    double rangeSecTimeSeries = 10.0;
    float limit = 1.5, rawLimit = 1.5;
    size_t nofm = 0;
    size_t offset = 0;
    size_t idx = 0;
    volatile bool statusMeasurement, statusServer;
    std::string sn = "None";
    bool flagRawData2 = false;
    std::array<double, RAW_SIZE> rawTime, rawData1, rawData2;
    std::array<double, MEASUREMENT_SIZE> times, x1s, y1s, x2s, y2s;
    bool flagAutoOffset = false;
    Settings()
    {
        ini::IniFile liaIni("lia.ini");
        windowWidth = (int)conv(liaIni["Window"]["windowWidth"].as<std::string>(), windowWidth);
        windowHeight = (int)conv(liaIni["Window"]["windowHeight"].as<std::string>(), windowHeight);
        windowPosX = (int)conv(liaIni["Window"]["windowPosX"].as<std::string>(), windowPosX);
        windowPosY = (int)conv(liaIni["Window"]["windowPosY"].as<std::string>(), windowPosY);
        fgFreq = conv(liaIni["Fg"]["fgFreq"].as<std::string>(), fgFreq);
        fg1Amp = conv(liaIni["Fg"]["fg1Amp"].as<std::string>(), fg1Amp);
        fg2Amp = conv(liaIni["Fg"]["fg2Amp"].as<std::string>(), fg2Amp);
        fg2Phase = conv(liaIni["Fg"]["fg2Phase"].as<std::string>(), fg2Phase);
        offset1Phase = conv(liaIni["Lia"]["offset1Phase"].as<std::string>(), offset1Phase);
        offset1X = conv(liaIni["Lia"]["offset1X"].as<std::string>(), offset1X);
        offset1Y = conv(liaIni["Lia"]["offset1Y"].as<std::string>(), offset1Y);
        offset2Phase = conv(liaIni["Lia"]["offset2Phase"].as<std::string>(), offset2Phase);
        offset2X = conv(liaIni["Lia"]["offset2X"].as<std::string>(), offset2X);
        offset2Y = conv(liaIni["Lia"]["offset2Y"].as<std::string>(), offset2Y);
        rangeSecTimeSeries = conv(liaIni["Plot"]["Measurement_rangeSecTimeSeries"].as<std::string>(), rangeSecTimeSeries);
        limit = conv(liaIni["Plot"]["limit"].as<std::string>(), limit);
        rawLimit = conv(liaIni["Plot"]["rawLimit"].as<std::string>(), rawLimit);

        for (size_t i = 0; i < rawTime.size(); i++)
        {
            rawTime[i] = i * rawDt * 1e6;
        }
    }
    void AddPoint(double t, double x, double y) {
        times[offset] = t;
        x1s[offset] = x;
        y1s[offset] = y;
        idx = offset;
        nofm++;
        offset = nofm % MEASUREMENT_SIZE;
    }
    void AddPoint(double t, double x1, double y1, double x2, double y2) {
        times[offset] = t;
        x1s[offset] = x1;
        y1s[offset] = y1;
        x2s[offset] = x2;
        y2s[offset] = y2;
        idx = offset;
        nofm++;
        offset = nofm % MEASUREMENT_SIZE;
    }
    ~Settings()
    {
        const char* filepath = "result.csv";
        std::ofstream outputFile(filepath);
        if (!outputFile)
        { // ファイルが開けなかった場合
            std::cerr << "Fail: " << filepath << std::endl;
        }
        outputFile << "# t(s), x(V), y(V)" << std::endl;
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
            outputFile << times[idx] << ',' << x1s[idx] << ',' << y1s[idx] << std::endl;
        }
        outputFile.close();

        //std::ofstream outputFile2("raw.csv");
        //if (!outputFile)
        //{ // ファイルが開けなかった場合
        //    std::cerr << "Fail: " << "raw.csv" << std::endl;
        //}
        //outputFile2 << "# t(s), v(V)" << std::endl;
        //for (size_t i = 0; i < this->rawTime.size(); i++)
        //{
        //    outputFile2 << rawTime[i] << ',' << rawData1[i] << std::endl;
        //}
        //outputFile2.close();

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
        liaIni["Plot"]["Measurement_rangeSecTimeSeries"] = this->rangeSecTimeSeries;
        liaIni["Plot"]["limit"] = this->limit;
        liaIni["Plot"]["rawLimit"] = this->rawLimit;

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
#pragma omp parallel for
        for (int i = 0; i < size; i++)
        {
            this->_sin[i] = 2 * std::sin(2 * PI * oldFreq * i * pSettings->rawDt);
            this->_cos[i] = 2 * std::cos(2 * PI * oldFreq * i * pSettings->rawDt);
        }
    }
    void calc(const double t)
    {
        if (oldFreq != pSettings->fgFreq) init();
        double _x1 = 0, _y1 = 0, _x2 = 0, _y2 = 0;
#pragma omp parallel for reduction(+:_x1, _y1, _x2, _y2) // daigokk: For OpenMP, This process is too small.
        for (int i = 0; i < size; i++)
        {
            _x1 += pSettings->rawData1[i] * this->_sin[i];
            _y1 += pSettings->rawData1[i] * this->_cos[i];
#ifdef W2
            _x2 += pSettings->rawData2[i] * this->_sin[i];
            _y2 += pSettings->rawData2[i] * this->_cos[i];
#endif // W2
        }
        _x1 /= this->_sin.size(); _y1 /= this->_sin.size();
#ifdef W2
        _x2 /= this->_sin.size(); _y2 /= this->_sin.size();
#endif // W2
        if (pSettings->flagAutoOffset)
        {
            pSettings->offset1X = _x1; pSettings->offset1Y = _y1;
#ifdef W2
            pSettings->offset2X = _x2; pSettings->offset2Y = _y2;
#endif // W2
            pSettings->flagAutoOffset = false;
        }
        _x1 -= pSettings->offset1X; _y1 -= pSettings->offset1Y;
#ifdef W2
        _x2 -= pSettings->offset2X; _y2 -= pSettings->offset2Y;
#endif // W2
        double theta = pSettings->offset1Phase / 180 * PI;
#ifndef W2
        pSettings->AddPoint(
            t, 
            _x1 * std::cos(theta) - _y1 * std::sin(theta), 
            _x1 * std::sin(theta) + _y1 * std::cos(theta)
        );
#else
        pSettings->AddPoint(
            t,
            _x1 * std::cos(theta) - _y1 * std::sin(theta),
            _x1 * std::sin(theta) + _y1 * std::cos(theta),
            _x2,
            _y2
        );
#endif // W2
    }
};
