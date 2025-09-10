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

#define RAW_TIME_DEFAULT 5e-5
//#define RAW_SIZE_RESERVE 16384
#define RAW_SIZE 5000
#define MEASUREMENT_DT 2e-3
#define MEASUREMENT_SEC (60*10)
#define MEASUREMENT_SIZE (size_t)(MEASUREMENT_SEC/MEASUREMENT_DT)
#define XY_HISTORY_SEC 10.0
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

typedef struct flag
{
    bool status = false, trigger = false;
    bool save = false;
} flag_t;

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
    float freq = 100e3, amp1 = 1.0, amp2 = 0.0, phase2 = 0.0;
    // Scope
    double rawDt = 1e-8;
    // Lia
    double offsetPhase = 0, offsetX = 0, offsetY = 0;
    // Plot
    double rangeSecTimeSeries = 10.0;
    float limit = 1.5, rawLimit = 1.5;
    size_t nofm = 0;
    size_t offset = 0;
    size_t idx = 0;
    volatile flag_t flagMeasurement;
    std::string sn = "None";
    bool flagRawData2 = false;
    std::array<double, RAW_SIZE> rawTime, rawData1;//, rawData2;
    std::array<double, MEASUREMENT_SIZE> times, xs, ys, x2s, y2s;
    bool flagAutoOffset = false;
    Settings()
    {
        ini::IniFile liaIni("lia.ini");
        windowWidth = (int)conv(liaIni["Window"]["windowWidth"].as<std::string>(), windowWidth);
        windowHeight = (int)conv(liaIni["Window"]["windowHeight"].as<std::string>(), windowHeight);
        windowPosX = (int)conv(liaIni["Window"]["windowPosX"].as<std::string>(), windowPosX);
        windowPosY = (int)conv(liaIni["Window"]["windowPosY"].as<std::string>(), windowPosY);
        freq = (float)conv(liaIni["Fg"]["freq"].as<std::string>(), freq);
        amp1 = (float)conv(liaIni["Fg"]["amp1"].as<std::string>(), amp1);
        offsetPhase = conv(liaIni["Lia"]["offsetPhase"].as<std::string>(), offsetPhase);
        offsetX = conv(liaIni["Lia"]["offsetX"].as<std::string>(), offsetX);
        offsetY = conv(liaIni["Lia"]["offsetY"].as<std::string>(), offsetY);
        rangeSecTimeSeries = conv(liaIni["Plot"]["Measurement_rangeSecTimeSeries"].as<std::string>(), rangeSecTimeSeries);
        
        limit = (float)conv(liaIni["Plot"]["limit"].as<std::string>(), limit);
        rawLimit = (float)conv(liaIni["Plot"]["rawLimit"].as<std::string>(), rawLimit);

        for (size_t i = 0; i < rawTime.size(); i++)
        {
            rawTime[i] = i * rawDt * 1e6;
        }
    }
    void AddPoint(double t, double x, double y) {
        times[offset] = t;xs[offset] = x; ys[offset] = y;
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
            outputFile << times[idx] << ',' << xs[idx] << ',' << ys[idx] << std::endl;
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
        liaIni["Fg"]["freq"] = this->freq;
        liaIni["Fg"]["amp1"] = this->amp1;
        liaIni["Lia"]["offsetPhase"] = this->offsetPhase;
        liaIni["Lia"]["offsetX"] = this->offsetX;
        liaIni["Lia"]["offsetY"] = this->offsetY;
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
        oldFreq = pSettings->freq;
        init();
    }
    void init()
    {
        oldFreq = pSettings->freq;
        size_t halfPeriodSize = (size_t)(0.5 / oldFreq / pSettings->rawDt);
        size = halfPeriodSize * (size_t)(RAW_SIZE / halfPeriodSize);
//#pragma omp parallel for
        for (int i = 0; i < size; i++)
        {
            this->_sin[i] = 2 * std::sin(2 * PI * oldFreq * i * pSettings->rawDt);
            this->_cos[i] = 2 * std::cos(2 * PI * oldFreq * i * pSettings->rawDt);
        }
    }
    void calc(const double t)
    {
        if (oldFreq != pSettings->freq) init();
        double _x = 0, _y = 0;
//#pragma omp parallel for reduction(+:_x, _y) // daigokk: For OpenMP, This process is too small.
        for (int i = 0; i < size; i++)
        {
            _x += pSettings->rawData1[i] * this->_sin[i];
            _y += pSettings->rawData1[i] * this->_cos[i];
        }
        _x /= this->_sin.size(); _y /= this->_sin.size();
        if (pSettings->flagAutoOffset)
        {
            pSettings->offsetX = _x; pSettings->offsetY = _y;
            pSettings->flagAutoOffset = false;
        }
        _x -= pSettings->offsetX; _y -= pSettings->offsetY;
        double theta = pSettings->offsetPhase / 180 * PI;
        pSettings->AddPoint(t, _x * std::cos(theta) - _y * std::sin(theta), _x * std::sin(theta) + _y * std::cos(theta));
        //pSettings->AddPoint(t, _x, _y);
    }
};
