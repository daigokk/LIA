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

#include <Daq_wf.h>

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
public:
    Daq_dwf* pDaq = nullptr;
    // Monitor
    float monitorScale = 1.0f;
    // Window
    int windowWidth = 1440;
    int windowHeight = 960;
    int windowPosX = 0;
    int windowPosY = 30;
    int ImGuiWindowFlag = 4;// ImGuiCond_FirstUseEver
    // ImGui
    int ImGui_Thema = 0;
    // Fg
    float w1Freq = 100e3, w1Amp = 1.0, w1Phase=0.0, w2Freq = w1Freq, w2Amp = 0.0, w2Phase = 0.0;
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
    //double rangeSecTimeSeries = 10.0;
    float limit = 1.5f, rawLimit = 1.5f, historySec = 10.0f;
    bool flagSurfaceMode = false, flagBeep = false, flagACFM = false;
    volatile bool statusMeasurement = false, statusPipe = false;
    std::string sn = "SN:XXXXXXXXXX";
    bool flagRawData2 = false;
	std::string filenameRaw = "raw.csv";
	// Data
    int nofm = 0, idx = 0, tail = 0, size = 0;
    std::array<double, RAW_SIZE> rawTime, rawData1, rawData2;
    std::array<double, MEASUREMENT_SIZE> times, x1s, y1s, x2s, y2s, dts;
    int xyNorm = 0, xyIdx = 0, xyTail = 0, xySize;
    std::array<double, XY_SIZE> xy1Xs, xy1Ys, xy2Xs, xy2Ys;
    

    Settings()
    {
        ini::IniFile liaIni("lia.ini");
        windowWidth = (int)conv(liaIni["Window"]["windowWidth"].as<std::string>(), windowWidth);
        windowHeight = (int)conv(liaIni["Window"]["windowHeight"].as<std::string>(), windowHeight);
        windowPosX = (int)conv(liaIni["Window"]["windowPosX"].as<std::string>(), windowPosX);
        windowPosY = (int)conv(liaIni["Window"]["windowPosY"].as<std::string>(), windowPosY);
        ImGui_Thema = (int)conv(liaIni["ImGui"]["ImGui_Thema"].as<std::string>(), ImGui_Thema);
        w1Freq = (float)conv(liaIni["Awg"]["w1Freq"].as<std::string>(), w1Freq);
        float lowLimitFreq = (float)(0.5 / (RAW_SIZE * rawDt));
        float highLimitFreq = (float)(1.0 / (1000 * rawDt));
        if (w1Freq < lowLimitFreq) w1Freq = lowLimitFreq;
        if (w1Freq > highLimitFreq) w1Freq = highLimitFreq;
        this->w2Freq = this->w1Freq;
        w1Amp = (float)conv(liaIni["Awg"]["w1Amp"].as<std::string>(), w1Amp);
        w2Amp = (float)conv(liaIni["Awg"]["w2Amp"].as<std::string>(), w2Amp);
        if (w1Amp < 0.1f) w1Amp = 0.1f;
        if (w1Amp > 5.0f) w1Amp = 5.0f;
        if (w2Amp < 0.0f) w2Amp = 0.0f;
        if (w2Amp > 5.0f) w2Amp = 5.0f;
        w2Phase = (float)conv(liaIni["Awg"]["w2Phase"].as<std::string>(), w2Phase);
        flagCh2 = convb(liaIni["Scope"]["flagCh2"].as<std::string>(), flagCh2);
        offset1Phase = conv(liaIni["Lia"]["offset1Phase"].as<std::string>(), offset1Phase);
        offset1X = conv(liaIni["Lia"]["offset1X"].as<std::string>(), offset1X);
        offset1Y = conv(liaIni["Lia"]["offset1Y"].as<std::string>(), offset1Y);
        offset2Phase = conv(liaIni["Lia"]["offset2Phase"].as<std::string>(), offset2Phase);
        offset2X = conv(liaIni["Lia"]["offset2X"].as<std::string>(), offset2X);
        offset2Y = conv(liaIni["Lia"]["offset2Y"].as<std::string>(), offset2Y);
        hpFreq = (float)conv(liaIni["Lia"]["hpFreq"].as<std::string>(), hpFreq);
        //rangeSecTimeSeries = conv(liaIni["Plot"]["Measurement_rangeSecTimeSeries"].as<std::string>(), rangeSecTimeSeries);
        limit = (float)conv(liaIni["Plot"]["limit"].as<std::string>(), limit);
        if (limit < 0.0f) limit = 0.0f;
        if (limit > 5.0f) limit = 5.0f;
        rawLimit = (float)conv(liaIni["Plot"]["rawLimit"].as<std::string>(), rawLimit);
        historySec = (float)conv(liaIni["Plot"]["historySec"].as<std::string>(), historySec);
        flagSurfaceMode = convb(liaIni["Plot"]["flagSurfaceMode"].as<std::string>(), flagSurfaceMode);
        flagBeep = convb(liaIni["Plot"]["flagBeep"].as<std::string>(), flagBeep);
        flagACFM = convb(liaIni["Plot"]["flagACFM"].as<std::string>(), flagACFM);

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
    bool saveRawData(const char* filename)
    {
        std::ofstream outputFile(filename);
        if (!outputFile)
        { // ファイルが開けなかった場合
            std::cerr << "Fail: " << filename << std::endl;
            return false;
        }
        else {
            if (!flagCh2)
            {
                outputFile << "# t(s), ch1(V)" << std::endl;
                for (size_t i = 0; i < rawData1.size(); i++)
                {
                    outputFile << std::format("{:e},{:e}\n", rawDt * i, rawData1[i]);
                }
            }
            else {
                outputFile << "# t(s), ch1(V), ch2(V)" << std::endl; 
                for (size_t i = 0; i < rawData1.size(); i++)
                {
                    outputFile << std::format("{:e},{:e},{:e}\n", rawDt * i, rawData1[i], rawData2[i]);
                }
            }
            outputFile.close();
        }
        return true;
    }
    bool saveRawData()
    {
		return saveRawData(this->filenameRaw.c_str());
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
        liaIni["ImGui"]["ImGui_Thema"] = this->ImGui_Thema;
        liaIni["Awg"]["w1Freq"] = this->w1Freq;
        liaIni["Awg"]["w1Amp"] = this->w1Amp;
        liaIni["Awg"]["w2Amp"] = this->w2Amp;
        liaIni["Awg"]["w2Phase"] = this->w2Phase; 
        liaIni["Scope"]["flagCh2"] = this->flagCh2;
        liaIni["Lia"]["offset1Phase"] = this->offset1Phase;
        liaIni["Lia"]["offset1X"] = this->offset1X;
        liaIni["Lia"]["offset1Y"] = this->offset1Y;
        liaIni["Lia"]["offset2Phase"] = this->offset2Phase;
        liaIni["Lia"]["offset2X"] = this->offset2X;
        liaIni["Lia"]["offset2Y"] = this->offset2Y;
        liaIni["Lia"]["hpFreq"] = this->hpFreq;
        //liaIni["Plot"]["Measurement_rangeSecTimeSeries"] = this->rangeSecTimeSeries;
        liaIni["Plot"]["limit"] = this->limit;
        liaIni["Plot"]["rawLimit"] = this->rawLimit;
        liaIni["Plot"]["historySec"] = this->historySec;
        liaIni["Plot"]["flagSurfaceMode"] = this->flagSurfaceMode;
        liaIni["Plot"]["flagBeep"] = this->flagBeep;
        liaIni["Plot"]["flagACFM"] = this->flagACFM;
        liaIni.save("lia.ini");
    }
};
