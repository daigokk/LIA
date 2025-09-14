#include <iostream>
#include <stop_token>
#include <thread>
#include <string>
#include <sstream>

//#define DAQ
#ifdef DAQ
#include <daq_dwf.hpp>
#endif // DAQ

#include "Settings.h"
#include "Gui.h"
#include "Timer.h"

void measurement(std::stop_token st, Settings* pSettings);
void server(std::stop_token st, Settings* pSettings);

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false); // For std::cout and cin
    static Settings settings;
    Gui gui(&settings);
    if (gui.initialized == false) return -1;
    std::jthread th_measurement{ measurement, &settings };
    while (!settings.statusMeasurement);
    std::jthread* pth_server = nullptr;
    for(int i = 0; i < argc; i++)
    {
        if (std::string("pipe") == argv[i])
        {
            pth_server = new std::jthread(server, &settings);
            while (!settings.statusServer);
        }
    }

    while (!gui.windowShouldClose())
    {
        if (settings.statusMeasurement == false) break;
        if (pth_server != nullptr && settings.statusServer == false) break;
        /* Poll for and process events */
        gui.pollEvents();
        gui.show();
        gui.rendering();
        gui.swapBuffers();
    }
    th_measurement.request_stop();
    if (pth_server != nullptr) pth_server->request_stop();

    return 0;
}

void measurement(std::stop_token st, Settings* pSettings)
{
   static Psd psd(pSettings);
    Timer timer;
#ifndef DAQ
    std::cout << "Not connect to AD." << std::endl;
#else
    //Daq_wf daq;
    //daq.powerSupply(5.0);
    //daq.Fg.start(pSettings->freq, pSettings->amp, 0.0);
    //timer.sleepFor(1.0);
    //daq.Scope.open();
    //daq.Scope.trigger();
    daq_dwf daq;
    std::cout << std::format("{:s}({:s}) is selected.\n", daq.type, daq.serialNo);
    pSettings->pDaq = &daq;
    pSettings->sn = daq.serialNo;
    daq.powerSupply(5);
    daq.fg(pSettings->fgCh1Amp, pSettings->fgFreq, 0, pSettings->fgCh2Amp, pSettings->fgCh2Phase);

    daq.adSettings.ch = 0; daq.adSettings.numCh = 1; daq.adSettings.range = 2.5;
    daq.adSettings.triggerDigCh = -1; daq.adSettings.waitAd = 0;
    daq.adSettings.numSampsPerChan = (int)pSettings->rawTime.size();
    daq.adSettings.rate = 1.0 / (pSettings->rawDt);
    daq.ad_init(daq.adSettings);
    daq.ad_start();
#endif // DAQ
    timer.start();
    pSettings->statusMeasurement = true;
    while (!st.stop_requested())
    {
        double t = pSettings->nofm * MEASUREMENT_DT;
        t = timer.sleepUntil(t);
#ifndef DAQ
        double phase = 2 * PI * t / 60;
        for (size_t i = 0; i < pSettings->rawTime.size(); i++)
        {
            double wt = 2 * PI * pSettings->fgFreq * i * pSettings->rawDt;
            pSettings->rawW1[i] = pSettings->fgCh1Amp * std::sin(wt - phase);
        }
#else
        //daq.Scope.record(pSettings->rawData1.data());
        daq.ad_get(daq.adSettings.numSampsPerChan, pSettings->rawW1.data());// , pSettings->rawW2.data());
        daq.ad_start();
#endif // DAQ
        psd.calc(t);
    }
    pSettings->statusMeasurement = false;
}

void server(std::stop_token st, Settings* pSettings)
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
            std::cout << std::format("{:e},{:e},{:e}\n", pSettings->times[idx], pSettings->w1xs[idx], pSettings->w1ys[idx]);
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
            pSettings->pDaq->fg(pSettings->fgCh1Amp, pSettings->fgFreq, 0.0, pSettings->fgCh2Amp, pSettings->fgCh2Phase);
#endif // DAQ
    }
    pSettings->statusServer = false;
}
