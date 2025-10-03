#pragma once
#include "Settings.h"
#include <iostream>
#include <string>
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
        std::string cmd;
        float value = 0;
        std::getline(std::cin, cmd);
        if (cmd.length() == 0) continue; // 空白を受信した時は無視
        std::istringstream iss(cmd);
        cmd = std::tolower(cmd.data());
        iss >> cmd >> value;
        if (cmd == "end")
        {
            break;
        }
        else if (cmd == "reset" cmd == "*rst")
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
            if (errcmd == "")
            {
                std::cout << "No error." << std::endl;
            }
            else
            {
                std::cout << std::format("Last error: '{}'\n", errcmd);
                errcmd = "";
            }
        }
        else if (cmd == ":data:txy?")
        {
            size_t idx = pSettings->idx;
            std::cout << std::format(
                "{:e},{:e},{:e}\n",
                pSettings->times[idx],
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
        else if (cmd == ":data:ch2:on")
        {
            pSettings->flagCh2 = true;
        }
        else if (cmd == ":data:ch2:off")
        {
            pSettings->flagCh2 = false;
        }
        else if (cmd == ":data:limit")
        {
            if (0.01 <= value && value <= RAW_RANGE * 1.2)
            {
                pSettings->limit = value;
            }
            else
            {
                errcmd = std::format("{} {}", cmd, value);
            }
        }
        else if (cmd == ":data:raw:limit")
        {
            if (0.1 <= value && value <= RAW_RANGE * 1.2)
            {
                pSettings->rawLimit = value;
            }
            else
            {
                errcmd = std::format("{} {}", cmd, value);
            }
        }
        else if (cmd == ":w1:freq?")
        {
            std::cout << pSettings->w1Freq << std::endl;
        }
        else if (cmd == ":w1:freq")
        {
            double lowLimitFreq = 0.5  / (RAW_SIZE * pSettings->rawDt);
            double highLimitFreq = std::round(1.0 / (1000 * pSettings->rawDt));
            if (lowLimitFreq <= value && value <= highLimitFreq)
            {
                pSettings->w1Freq = value;
                fgFlag = true;
            }
            else
            {
                errcmd = std::format("{} {}", cmd, value);
            }
        }
        else if (cmd == ":w1:volt?")
        {
            std::cout << pSettings->w1Amp << std::endl;
        }
        else if (cmd == ":w1:volt")
        {
            if (0.0 <= value && value <= 5.0)
            {
                pSettings->w1Amp = value;
                fgFlag = true;
            }
            else
            {
                errcmd = std::format("{} {}", cmd, value);
            }
        }
        else if (cmd == ":w2:volt?")
        {
            std::cout << pSettings->w2Amp << std::endl;
        }
        else if (cmd == ":w2:volt")
        {
            if (0.0 <= value && value <= 5.0)
            {
                pSettings->w2Amp = value;
                fgFlag = true;
            }
            else
            {
                errcmd = std::format("{} {}", cmd, value);
            }
        }
        else if (cmd == ":w2:phase?")
        {
            std::cout << pSettings->w2Phase << std::endl;
        }
        else if (cmd == ":w2:phase")
        {
            pSettings->w2Phase = value;
            fgFlag = true;
        }
        else if (cmd == ":calc:hp:freq?")
        {
            std::cout << pSettings->hpFreq << std::endl;
        }
        else if (cmd == ":calc:hp:freq")
        {
            if (0.0 <= value && value <= 50.0)
            {
                pSettings->hpFreq = value;
            }
            else
            {
                errcmd = std::format("{} {}", cmd, value);
            }
        }
        else if (cmd == ":calc:offset:auto:once")
        {
            pSettings->flagAutoOffset = true;
        }
        else if (cmd == ":calc:offset:off")
        {
            pSettings->offset1X = 0; pSettings->offset1Y = 0;
            pSettings->offset2X = 0; pSettings->offset2Y = 0;
        }
        else if (cmd == ":phase1:offset")
        {
            pSettings->offset1Phase = value;
        }
        else if (cmd == ":phase2:offset")
        {
            pSettings->offset2Phase = value;
        }
        else if (cmd.find("?") != -1)
        {
            errcmd = std::format("{} {}", cmd, value);
            std::cout << std::format("Error: '{}'\n", errcmd);
            errcmd = "";
        }
        else
        {
            errcmd = std::format("{} {}", cmd, value);
        }
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
