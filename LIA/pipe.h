#pragma once
#include "Settings.h"
#include <iostream>
#include <string>
#include <algorithm> // std::transform
#include <format>
#include <sstream> // std::istringstream
#include <cmath>

void pipe(std::stop_token st, Settings* pSettings)
{
    pSettings->statusPipe = true;
    while (!st.stop_requested())
    {
        static std::string errcmd = "";
        bool fgFlag = false;
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
        if (cmd == "end" || cmd == "exit" || cmd == "quit") { break; }
        else if (cmd == "reset" || cmd == "*rst")
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
        }
        else if (cmd == "*idn?")
        {
            std::cout << std::format("Digilent,{},{}\n", pSettings->pDaq->device.name, pSettings->pDaq->device.sn);
        }
        else if (cmd == "error?")
        {
            if (errcmd == "") { std::cout << "No error." << std::endl; }
            else
            {
                std::cout << std::format("Last error: '{}'\n", errcmd);
                errcmd = "";
            }
        }
        else if (cmd == "pause") { pSettings->flagPause = true; }
        else if (cmd == "run") { pSettings->flagPause = false; }
        else if (cmd == ":data:raw:save")
        {
            bool flag = false;
            if (argument.length() > 0) { flag = pSettings->saveRawData(argument.c_str()); }
            else { flag = pSettings->saveRawData(); }
            if (!flag) { errcmd = std::format("{} {}", cmd, argument); }
        }
        else if (cmd == ":data:raw:size?") { std::cout << pSettings->rawData1.size() << std::endl; }
        else if (cmd == ":data:raw?")
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
        }
        else if (cmd == ":data:txy?")
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
                else {
                    idx += MEASUREMENT_SIZE;
                }
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
        }
        else if (cmd == ":data:xy?")
        {
            size_t idx = pSettings->idx;
            std::cout << std::format(
                "{:e},{:e}\n",
                pSettings->x1s[idx],
                pSettings->y1s[idx]
            );
        }
        else if (cmd == ":data:tx1y1x2y2?")
        {
            size_t idx = pSettings->idx;
            std::cout << std::format(
                "{:e},{:e},{:e},{:e},{:e}\n",
                pSettings->times[idx],
                pSettings->x1s[idx],
                pSettings->y1s[idx],
                pSettings->x2s[idx],
                pSettings->y2s[idx]
            );
        }
        else if (cmd == ":chan2:disp")
        {
            if (argument == "on")       pSettings->flagCh2 = true;
            else if (argument == "off") pSettings->flagCh2 = false;
            else                        errcmd = std::format("{} {}", cmd, argument);
        }
        else if (cmd == ":disp:xy:limit")
        {
            if (0.01 <= value && value <= RAW_RANGE * 1.2) { pSettings->limit = value; }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        else if (cmd == ":disp:raw:limit")
        {
            if (0.1 <= value && value <= RAW_RANGE * 1.2) { pSettings->rawLimit = value; }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        else if (cmd == ":w1:freq?") { std::cout << pSettings->w1Freq << std::endl; }
        else if (cmd == ":w1:freq")
        {
            double lowLimitFreq = 0.5 / (RAW_SIZE * pSettings->rawDt);
            double highLimitFreq = std::round(1.0 / (1000 * pSettings->rawDt));
            if (lowLimitFreq <= value && value <= highLimitFreq)
            {
                pSettings->w1Freq = value;
                fgFlag = true;
            }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        else if (cmd == ":w1:volt?") { std::cout << pSettings->w1Amp << std::endl; }
        else if (cmd == ":w1:volt")
        {
            if (0.0 <= value && value <= 5.0)
            {
                pSettings->w1Amp = value;
                fgFlag = true;
            }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        else if (cmd == ":w2:volt?") { std::cout << pSettings->w2Amp << std::endl; }
        else if (cmd == ":w2:volt")
        {
            if (0.0 <= value && value <= 5.0)
            {
                pSettings->w2Amp = value;
                fgFlag = true;
            }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        else if (cmd == ":w2:phase?") { std::cout << pSettings->w2Phase << std::endl; }
        else if (cmd == ":w2:phase")
        {
            pSettings->w2Phase = value;
            fgFlag = true;
        }
        else if (cmd == ":calc:offset:auto:once") { pSettings->flagAutoOffset = true; }
        else if (cmd == ":calc:offset:state")
        {
            if (argument == "on") { pSettings->flagAutoOffset = true; }
            else if (argument == "off")
            {
                pSettings->offset1X = 0; pSettings->offset1Y = 0;
                pSettings->offset2X = 0; pSettings->offset2Y = 0;
            }
        }
        else if (cmd == ":calc:hp:freq?") { std::cout << pSettings->hpFreq << std::endl; }
        else if (cmd == ":calc:hp:freq")
        {
            if (0.0 <= value && value <= 50.0) { pSettings->hpFreq = value; }
            else { errcmd = std::format("{} {}", cmd, value); }
        }
        else if (cmd == ":calc1:offset:phase") { pSettings->offset1Phase = value; }
        else if (cmd == ":calc2:offset:phase") { pSettings->offset2Phase = value; }
        else if (cmd.find("?") != -1)
        {
            errcmd = std::format("{} {}", cmd, value);
            std::cout << std::format("Error: '{}'\n", errcmd);
            errcmd = "";
        }
        else { errcmd = std::format("{} {}", cmd, value); }
        std::cout << std::flush;
        std::cin.clear();
#ifdef DAQ
        if (fgFlag)
        {
            pSettings->pDaq->awg.start(
                pSettings->w1Freq, pSettings->w1Amp, pSettings->w1Phase,
                pSettings->w2Freq, pSettings->w2Amp, pSettings->w2Phase
            );
        }
#endif // DAQ
    }
    pSettings->statusPipe = false;
}
