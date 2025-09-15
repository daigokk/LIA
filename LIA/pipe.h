#pragma once

void pipe(std::stop_token st, Settings* pSettings)
{
    pSettings->statusServer = true;
    while (!st.stop_requested())
    {
        bool fgFlag = false;
        std::string cmd;
        float value = 0;
        std::getline(std::cin, cmd);
        std::istringstream iss(cmd);
        iss >> cmd >> value;
        if (cmd.compare("end") == 0) break;
        else if (cmd.compare("get_w1txy") == 0)
        {
            size_t idx = pSettings->idx;
            std::cout << std::format(
                "{:e},{:e},{:e}\n",
                pSettings->times[idx],
                pSettings->w1xs[idx],
                pSettings->w1ys[idx]
            );
        }
        else if (cmd.compare("get_w12txy") == 0)
        {
            size_t idx = pSettings->idx;
            std::cout << std::format(
                "{:e},{:e},{:e},{:e},{:e}\n",
                pSettings->times[idx],
                pSettings->w1xs[idx],
                pSettings->w1ys[idx],
                pSettings->w2xs[idx],
                pSettings->w2ys[idx]
            );
        }
        else if (cmd.compare("get_fgFreq") == 0)
        {
            std::cout << pSettings->fgFreq << std::endl;
        }
        else if (cmd.compare("set_fgFreq") == 0)
        {
            pSettings->fgFreq = value;
            fgFlag = true;
        }
        else if (cmd.compare("get_fgCh1Amp") == 0)
        {
            std::cout << pSettings->fgCh1Amp << std::endl;
        }
        else if (cmd.compare("set_fgCh1Amp") == 0)
        {
            pSettings->fgCh1Amp = value;
            fgFlag = true;
        }
        else if (cmd.compare("get_fgCh2Amp") == 0)
        {
            std::cout << pSettings->fgCh2Amp << std::endl;
        }
        else if (cmd.compare("set_fgCh2Amp") == 0)
        {
            pSettings->fgCh2Amp = value;
            fgFlag = true;
        }
        else if (cmd.compare("get_fgCh2Phase") == 0)
        {
            std::cout << pSettings->fgCh2Phase << std::endl;
        }
        else if (cmd.compare("set_fgCh2Phase") == 0)
        {
            pSettings->fgCh2Phase = value;
            fgFlag = true;
        }
        std::cin.clear();
#ifdef DAQ
        if (fgFlag)
            pSettings->pDaq->fg(
                pSettings->fgCh1Amp,
                pSettings->fgFreq,
                0.0,
                pSettings->fgCh2Amp,
                pSettings->fgCh2Phase);
#endif // DAQ
    }
    pSettings->statusServer = false;
}
