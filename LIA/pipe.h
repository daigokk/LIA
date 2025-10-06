#pragma once
#include "Settings.h"
#include <algorithm> // std::transform
#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <sstream> // std::istringstream
#include <string>

// 文字列を「:」で分割する関数
std::vector<std::string> parseColon(const std::string& input) {
    std::vector<std::string> result;
    std::string temp;

    for (char ch : input) {
        if (ch == ':') {
            if (temp.length() == 0)
            {
                temp.clear();
                continue;
            }
            result.push_back(temp);
            temp.clear();
        }
        else {
            temp += ch;
        }
    }
    result.push_back(temp); // 最後の部分を追加

    return result;
}

void pipe(std::stop_token st, Settings* pSettings)
{
    pSettings->statusPipe = true;
    while (!st.stop_requested())
    {
        static std::string errcmd = "";
        bool fgFlag = false, cmdMissFlag = true;
        std::string cmd, argument;
        float value = 0;
        std::getline(std::cin, cmd);
        if (cmd.length() == 0) continue; // 空白を受信した時は無視
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower); // コマンドを小文字に変換
		std::istringstream iss(cmd);
        iss >> cmd >> argument;
        try {
            value = std::stof(argument);
        }
        catch (...) {
            value = 0;
        }
        std::vector<std::string> parsedCmd = parseColon(cmd);
        if (parsedCmd[0] == "end" || parsedCmd[0] == "exit" || parsedCmd[0] == "quit") { break; }
        else if (parsedCmd[0] == "reset" || parsedCmd[0] == "*rst")
        {
            pSettings->w1Freq = (float)(1.0 / (1000 * pSettings->rawDt));
            pSettings->w2Freq = pSettings->w1Freq;
            pSettings->w1Amp = 1.0f;
            pSettings->w2Amp = 0.0f;
            pSettings->w1Phase = 0.0f;
            pSettings->w2Phase = 0.0f;
            pSettings->flagCh2 = false;
            pSettings->offset1Phase = 0.0;
            pSettings->offset1X = 0.0;
            pSettings->offset1Y = 0.0;
            pSettings->offset2Phase = 0.0;
            pSettings->offset2X = 0.0;
            pSettings->offset2Y = 0.0;
            pSettings->hpFreq = 0.0f;
            pSettings->flagSurfaceMode = false;
            pSettings->flagBeep = false;
            pSettings->flagACFM = false;
            pSettings->limit = 1.5f;
            pSettings->rawLimit = 1.5f;
            pSettings->historySec = 10.0f;
            errcmd = "";
            fgFlag = true;
            cmdMissFlag = false;
        }
        else if (parsedCmd[0] == "*idn?")
        {
            if(pSettings->pDaq == nullptr)
            {
                std::cout << "No DAQ is connected." << std::endl;
                cmdMissFlag = false;
			}
            else {
                std::cout << std::format(
                    "{},{},{},{}\n",
                    pSettings->pDaq->device.manufacturer,
                    pSettings->pDaq->device.name,
                    pSettings->pDaq->device.sn,
                    pSettings->pDaq->device.version
                );
                cmdMissFlag = false;
            }
        }
        else if (parsedCmd[0] == "error?")
        {
            if (errcmd == "")
            {
                std::cout << "No error." << std::endl;
                cmdMissFlag = false;
            }
            else
            {
                std::cout << std::format("Last error: '{}'\n", errcmd);
                errcmd = "";
                cmdMissFlag = false;
            }
        }
        else if (parsedCmd[0] == "pause")
        {
            pSettings->flagPause = true;
            cmdMissFlag = false;
        }
        else if (parsedCmd[0] == "run")
        {
            pSettings->flagPause = false;
            cmdMissFlag = false;
        }
        else if (parsedCmd[0] == "data")
        {
            if (parsedCmd[1] == "raw")
            {
                if (parsedCmd[2] == "save")
                {
                    bool flag = false;
                    if (argument.length() > 0)
                    {
                        flag = pSettings->saveRawData(argument.c_str());
                    }
                    else {
                        flag = pSettings->saveRawData();
                    }
                    if (!flag)
                    {
                        errcmd = std::format("{} {}", cmd, argument);
                        cmdMissFlag = false;
                    }
                }
                else if (parsedCmd[2] == "size?")
                {
                    std::cout << pSettings->rawData1.size() << std::endl;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "raw?")
            {
                if (!pSettings->flagCh2)
                {
                    for (int i = 0; i < pSettings->rawData1.size(); i++)
                    {
                        std::cout << std::format("{:e},{:e}\n", pSettings->rawDt * i, pSettings->rawData1[i]);
                    }
                }
                else
                {
                    for (int i = 0; i < pSettings->rawData1.size(); i++)
                    {
                        std::cout << std::format("{:e},{:e},{:e}\n", pSettings->rawDt * i, pSettings->rawData1[i], pSettings->rawData2[i]);
                    }
                }
                cmdMissFlag = false;
            }
            else if (parsedCmd[1] == "txy?")
            {
                int size = pSettings->size;
                if (value > 0)
                {
                    size = value / MEASUREMENT_DT;
                    if (size > pSettings->size) size = pSettings->size;
                }
                int idx = pSettings->tail - size;
                if (idx < 0)
                {
                    if (pSettings->nofm <= MEASUREMENT_SIZE)
                    {
                        idx = 0;
                        size = pSettings->tail;
                    }
                    else { idx += MEASUREMENT_SIZE; }
                }
                std::cout << size << std::endl;
                if (!pSettings->flagCh2)
                {
                    for (int i = 0; i < size; i++)
                    {
                        std::cout << std::format("{:e},{:e},{:e}\n", pSettings->times[idx], pSettings->x1s[idx], pSettings->y1s[idx]);
                        idx++;
                        idx %= MEASUREMENT_SIZE;
                    }
                }
                else
                {
                    for (int i = 0; i < size; i++)
                    {
                        std::cout << std::format("{:e},{:e},{:e},{:e},{:e}\n", pSettings->times[idx], pSettings->x1s[idx], pSettings->y1s[idx], pSettings->x2s[idx], pSettings->y2s[idx]);
                        idx++;
                        idx %= MEASUREMENT_SIZE;
                    }
                }
                cmdMissFlag = false;
            }
            else if (parsedCmd[1] == "xy?")
            {
                size_t idx = pSettings->idx;
                std::cout << std::format(
                    "{:e},{:e}",
                    pSettings->x1s[idx],
                    pSettings->y1s[idx]
                );
                if (pSettings->flagCh2)
                {
                    std::cout << std::format(
                        "{:e},{:e}",
                        pSettings->x2s[idx],
                        pSettings->y2s[idx]
                    );
                }
				std::cout << std::endl;
                cmdMissFlag = false;
            }
        }
        else if (cmd == ":chan2:disp")
        {
            if (argument == "on")
            {
                pSettings->flagCh2 = true;
                cmdMissFlag = false;
            }
            else if (argument == "off")
            {
                pSettings->flagCh2 = false;
                cmdMissFlag = false;
            }
        }
        else if (parsedCmd[0] == "disp" || parsedCmd[0] == "display")
        {
            if (parsedCmd[1] == "xy" && parsedCmd[2] == "limit")
            {
                if (0.01 <= value && value <= RAW_RANGE * 1.2)
                {
                    pSettings->limit = value;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "raw" && parsedCmd[2]=="limit")
            {
                if (0.1 <= value && value <= RAW_RANGE * 1.2)
                {
                    pSettings->rawLimit = value;
                    cmdMissFlag = false;
                }
            }
        }
        else if (parsedCmd[0] == "w1")
        {
            static double lowLimitFreq = 0.5 / (RAW_SIZE * pSettings->rawDt);
            static double highLimitFreq = std::round(1.0 / (1000 * pSettings->rawDt));
            if (parsedCmd[1] == "phase?")
            {
                std::cout << pSettings->w1Phase << std::endl;
                cmdMissFlag = false;
            }
            else if (parsedCmd[1] == "phase")
            {
                pSettings->w1Phase = value;
                fgFlag = true;
                cmdMissFlag = false;
            }
            else if (parsedCmd[1] == "freq?" || parsedCmd[1] == "frequency?")
            {
                if (argument.length() > 0)
                {
                    if (argument == "min")
                    {
                        std::cout << lowLimitFreq << std::endl;
                        cmdMissFlag = false;
                    }
                    else if (argument == "max")
                    {
                        std::cout << highLimitFreq << std::endl;
                        cmdMissFlag = false;
                    }
                }
                else
                {
                    std::cout << pSettings->w1Freq << std::endl;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "freq" || parsedCmd[1] == "frequency")
            {
                if (lowLimitFreq <= value && value <= highLimitFreq)
                {
                    pSettings->w1Freq = value;
                    fgFlag = true;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "volt?" || parsedCmd[1] == "voltage?")
            {
                if (argument.length() > 0)
                {
                    if (argument == "min")
                    {
                        std::cout << 0.0 << std::endl;
                        cmdMissFlag = false;
                    }
                    else if (argument == "max")
                    {
                        std::cout << 5.0 << std::endl;
                        cmdMissFlag = false;
                    }
                }
                else
                {
                    std::cout << pSettings->w1Amp << std::endl;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "volt" || parsedCmd[1] == "voltage")
            {
                if (0.0 <= value && value <= 5.0)
                {
                    pSettings->w1Amp = value;
                    fgFlag = true;
                    cmdMissFlag = false;
                }
            }
        }
        else if (parsedCmd[0] == "w2")
        {
            static double lowLimitFreq = 0.5 / (RAW_SIZE * pSettings->rawDt);
            static double highLimitFreq = std::round(1.0 / (1000 * pSettings->rawDt));
            if (parsedCmd[1] == "phase?")
            {
                std::cout << pSettings->w2Phase << std::endl;
                cmdMissFlag = false;
            }
            else if (parsedCmd[1] == "phase")
            {
                pSettings->w2Phase = value;
                fgFlag = true;
                cmdMissFlag = false;
            }
            else if (parsedCmd[1] == "freq?" || parsedCmd[1] == "frequency?")
            {
                if (argument.length() > 0)
                {
                    if (argument == "min")
                    {
                        std::cout << lowLimitFreq << std::endl;
                        cmdMissFlag = false;
                    }
                    else if (argument == "max")
                    {
                        std::cout << highLimitFreq << std::endl;
                        cmdMissFlag = false;
                    }
                }
                else
                {
                    std::cout << pSettings->w2Freq << std::endl;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "freq" || parsedCmd[1] == "frequency")
            {
                if (lowLimitFreq <= value && value <= highLimitFreq)
                {
                    pSettings->w2Freq = value;
                    fgFlag = true;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "volt?" || parsedCmd[1] == "voltage?")
            {
                if (argument.length() > 0)
                {
                    if (argument == "min")
                    {
                        std::cout << 0.0 << std::endl;
                        cmdMissFlag = false;
                    }
                    else if (argument == "max")
                    {
                        std::cout << 5.0 << std::endl;
                        cmdMissFlag = false;
                    }
                }
                else
                {
                    std::cout << pSettings->w2Amp << std::endl;
                    cmdMissFlag = false;
                }
            }
            else if (parsedCmd[1] == "volt" || parsedCmd[1] == "voltage")
            {
                if (0.0 <= value && value <= 5.0)
                {
                    pSettings->w2Amp = value;
                    fgFlag = true;
                    cmdMissFlag = false;
                }
            }
        }
        else if (parsedCmd[0] == "calc" || parsedCmd[0] == "calculate")
        {
            if (parsedCmd[1] == "offset")
            {
                if (parsedCmd[2] == "auto" && parsedCmd[3] == "once")
                {
                    pSettings->flagAutoOffset = true;
                    cmdMissFlag = false;
                }
                else if (parsedCmd[2] == "state")
                {
                    if (argument == "on") {
                        pSettings->flagAutoOffset = true;
                        cmdMissFlag = false;
                    }
                    else if (argument == "off")
                    {
                        pSettings->offset1X = 0; pSettings->offset1Y = 0;
                        pSettings->offset2X = 0; pSettings->offset2Y = 0;
                        cmdMissFlag = false;
                    }
                }
            }
            else if (parsedCmd[1] == "hpf")
            {
                if (parsedCmd[2] == "freq?" || parsedCmd[2] == "frequency?")
                {
                    std::cout << pSettings->hpFreq << std::endl;
                    cmdMissFlag = false;
                }
                else if (parsedCmd[2] == "freq" || parsedCmd[2] == "frequency")
                {
                    if (0.0 <= value && value <= 50.0)
                    {
                        pSettings->hpFreq = value;
                        cmdMissFlag = false;
                    }
                }
            }
            else if (cmd == ":calc1:offset:phase")
            {
                pSettings->offset1Phase = value;
                cmdMissFlag = false;
            }
            else if (cmd == ":calc2:offset:phase")
            {
                pSettings->offset2Phase = value;
                cmdMissFlag = false;
            }
        }

        if (cmdMissFlag)
        {
            if (cmd.find("?") != -1)
            {
                errcmd = std::format("{} {}", cmd, value);
                std::cout << std::format("Error: '{}'\n", errcmd);
                errcmd = "";
            }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        std::cout << std::flush;
        std::cin.clear();
        if (pSettings->pDaq != nullptr && fgFlag)
        {
            pSettings->pDaq->awg.start(
                pSettings->w1Freq, pSettings->w1Amp, pSettings->w1Phase,
                pSettings->w2Freq, pSettings->w2Amp, pSettings->w2Phase
            );
        }
    }
    pSettings->statusPipe = false;
}
