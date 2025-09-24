#include <iostream>
#include <format>
#include <stop_token> // std::jthread
#include <thread> // std::jthread
#include <numbers> // For std::numbers::pi
#include <Windows.h>

#define DAQ
#ifdef DAQ
#include <daq_dwf.hpp>
#endif // DAQ

#include "Settings.h"
#include "Psd.h"
#include "Gui.h"
#include "Timer.h"
#include "pipe.h"
void measurement(std::stop_token st, Settings* pSettings);

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false); // For std::cout and cin
    static Settings settings;
    Gui gui(&settings);
    if (gui.initialized == false) return -1;
    std::jthread th_measurement{ measurement, &settings };
    // スレッドの優先度を設定
    HANDLE handle = th_measurement.native_handle();
    if (SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST)) {
        //std::cout << "Thread priority set to highest.\n";
    }
    else {
        std::cerr << "Failed to set thread priority.\n";
    }
    while (!settings.statusMeasurement);
    std::jthread* pth_pipe = nullptr;
    for(int i = 0; i < argc; i++)
    {
        if (std::string("pipe") == argv[i])
        {
            pth_pipe = new std::jthread(pipe, &settings);
            while (!settings.statusPipe);
        }
    }

    while (!gui.windowShouldClose())
    {
        if (settings.statusMeasurement == false) break;
        if (pth_pipe != nullptr && settings.statusPipe == false) break;
        gui.beep();
        /* Poll for and process events */
        gui.pollEvents();
        gui.show();
        gui.rendering();
        gui.swapBuffers();
    }
    th_measurement.request_stop();
    if (pth_pipe != nullptr) pth_pipe->request_stop();
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
    daq.fg(pSettings->fg1Amp, pSettings->fgFreq, 0, pSettings->fg2Amp, pSettings->fg2Phase);
    daq.adSettings.ch = 0; daq.adSettings.range = RAW_RANGE;
    
    daq.adSettings.numCh = 2;
    
    daq.adSettings.triggerDigCh = -1; daq.adSettings.waitAd = 0;
    daq.adSettings.numSampsPerChan = (int)pSettings->rawTime.size();
    daq.adSettings.rate = 1.0 / (pSettings->rawDt);
    daq.ad_init(daq.adSettings);
    timer.sleepFor(0.5); 
    daq.ad_start();
#endif // DAQ
    timer.start();
    pSettings->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = timer.sleepUntil(t);
        if (pSettings->flagPause) continue;
#ifndef DAQ
        double phase = 2 * std::numbers::pi * t / 60;
        for (size_t i = 0; i < pSettings->rawTime.size(); i++)
        {
            double wt = 2 * std::numbers::pi * pSettings->fgFreq * i * pSettings->rawDt;

            pSettings->rawData1[i] = pSettings->fg1Amp * std::sin(wt - phase);
            if (pSettings->flagCh2)
            {
                pSettings->rawData2[i] = pSettings->fg2Amp * std::sin(wt - pSettings->fg2Phase);
            }
        }
#else
        //daq.Scope.record(pSettings->rawData1.data());
        if (!pSettings->flagCh2)
        {
            daq.ad_get(daq.adSettings.numSampsPerChan, pSettings->rawData1.data());
        }
        else {
            daq.ad_get(daq.adSettings.numSampsPerChan, pSettings->rawData1.data(), pSettings->rawData2.data());
        }
#endif // DAQ
        psd.calc(t);
    }
    pSettings->statusMeasurement = false;
}
