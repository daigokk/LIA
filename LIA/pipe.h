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
        if (cmd.compare("end") == 0) break;
        else if (cmd.compare(":data:txy?") == 0)
        {
            size_t idx = pSettings->idx;
            std::cout << std::format(
                "{:e},{:e},{:e}\n",
                pSettings->times[idx],
                pSettings->x1s[idx],
                pSettings->y1s[idx]
            );
        }
        else if (cmd.compare(":data:tx1y1x2y2?") == 0)
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
        else if (cmd.compare(":w1:freq?") == 0)
        {
            std::cout << pSettings->fgFreq << std::endl;
        }
        else if (cmd.compare(":w1:freq") == 0)
        {
            if (10e3 <= value && value <= 100e3)
            {
                pSettings->fgFreq = value;
                fgFlag = true;
            }
        }
        else if (cmd.compare(":w1:volt?") == 0)
        {
            std::cout << pSettings->fg1Amp << std::endl;
        }
        else if (cmd.compare(":w1:volt") == 0)
        {
            if (0 <= value && value <= 5.0)
            {
                pSettings->fg1Amp = value;
                fgFlag = true;
            }
        }
        else if (cmd.compare(":w2:volt?") == 0)
        {
            std::cout << pSettings->fg2Amp << std::endl;
        }
        else if (cmd.compare(":w2:volt") == 0)
        {
            if (0 <= value && value <= 5.0)
            {
                pSettings->fg2Amp = value;
                fgFlag = true;
            }
        }
        else if (cmd.compare(":w2:phase?") == 0)
        {
            std::cout << pSettings->fg2Phase << std::endl;
        }
        else if (cmd.compare(":w2:phase") == 0)
        {
            pSettings->fg2Phase = value;
            fgFlag = true;
        }
        else if (cmd.compare(":calc:hp:freq?") == 0)
        {
            std::cout << pSettings->hpFreq << std::endl;
        }
        else if (cmd.compare(":calc:hp:freq") == 0)
        {
            pSettings->hpFreq = value;
        }
        else if (cmd.compare(":calc:offset:auto:once") == 0)
        {
            pSettings->flagAutoOffset = true;
        }
        else if (cmd.compare(":calc:offset:off") == 0)
        {
            pSettings->offset1X = 0; pSettings->offset1Y = 0;
            pSettings->offset2X = 0; pSettings->offset2Y = 0;
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
