#pragma once
#include "Settings.h"
#include <iostream>
#include <string>
#include <format>
#include <sstream> // std::istringstream

void pipe(std::stop_token st, Settings* pSettings)
{
    pSettings->statusPipe = true;
    while (!st.stop_requested())
    {
        bool fgFlag = false;
        std::string cmd;
        float value = 0;
        std::getline(std::cin, cmd);
        std::istringstream iss(cmd);
        iss >> cmd >> value;
        if (cmd == "end")
        {
            break;
        }
        else if (cmd == "reset")
        {
            pSettings->fgFreq = 100e3f;
            pSettings->fg1Amp = 1.0f;
            pSettings->fg2Amp = 0.0f;
            pSettings->fg2Phase = 0.0f;
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
            pSettings->limit = 1.5f;
            pSettings->rawLimit = 1.5f;
            pSettings->historySec = 10.0f;
            fgFlag = true;
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
            pSettings->limit = value;
        }
        else if (cmd == ":data:raw:limit")
        {
            pSettings->rawLimit = value;
        }
        else if (cmd == ":w1:freq?")
        {
            std::cout << pSettings->fgFreq << std::endl;
        }
        else if (cmd == ":w1:freq")
        {
            if (10e3 <= value && value <= 100e3)
            {
                pSettings->fgFreq = value;
                fgFlag = true;
            }
        }
        else if (cmd == ":w1:volt?")
        {
            std::cout << pSettings->fg1Amp << std::endl;
        }
        else if (cmd == ":w1:volt")
        {
            if (0 <= value && value <= 5.0)
            {
                pSettings->fg1Amp = value;
                fgFlag = true;
            }
        }
        else if (cmd == ":w2:volt?")
        {
            std::cout << pSettings->fg2Amp << std::endl;
        }
        else if (cmd == ":w2:volt")
        {
            if (0 <= value && value <= 5.0)
            {
                pSettings->fg2Amp = value;
                fgFlag = true;
            }
        }
        else if (cmd == ":w2:phase?")
        {
            std::cout << pSettings->fg2Phase << std::endl;
        }
        else if (cmd == ":w2:phase")
        {
            pSettings->fg2Phase = value;
            fgFlag = true;
        }
        else if (cmd == ":calc:hp:freq?")
        {
            std::cout << pSettings->hpFreq << std::endl;
        }
        else if (cmd == ":calc:hp:freq")
        {
            pSettings->hpFreq = value;
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
        std::cin.clear();
#ifdef DAQ
        if (fgFlag)
            pSettings->pDaq->fg(
                pSettings->fg1Amp,
                pSettings->fgFreq,
                0.0,
                pSettings->fg2Amp,
                pSettings->fg2Phase);
#endif // DAQ
    }
    pSettings->statusPipe = false;
}
