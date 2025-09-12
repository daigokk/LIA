#include <iostream>
#include <stop_token>
#include <thread>
#include <cstring>

#define DAQ
#ifdef DAQ
#include <daq_dwf.hpp>
#endif // DAQ

#include "Settings.h"
#include "Gui.h"
#include "Timer.h"

void measurement(std::stop_token st, Settings* pSettings);
//void server(std::stop_token st, Settings* pSettings);

int main(void)
{
    std::ios::sync_with_stdio(false); // For std::cout and cin
    static Settings settings;
    Gui gui(&settings);
    if (gui.initialized == false) return -1;
    std::jthread th_measurement{ measurement, &settings };
    //std::jthread th_server{ server, &settings };
    while (!settings.statusMeasurement);
    //while (!settings.statusServer);

    while (!gui.windowShouldClose())
    {
        if (settings.statusMeasurement == false) break;
        //if (settings.statusServer == false) break;
        /* Poll for and process events */
        gui.pollEvents();
        gui.show();
        gui.rendering();
        gui.swapBuffers();
    }
    th_measurement.request_stop();
    //th_server.request_stop();

    return 0;
}

void measurement(std::stop_token st, Settings* pSettings)
{
   static Psd psd(pSettings);
    Timer timer;
#ifndef DAQ
    std::cout << "Not connect to AD mode." << std::endl;
#else
    //Daq_wf daq;
    //daq.powerSupply(5.0);
    //daq.Fg.start(pSettings->freq, pSettings->amp, 0.0);
    //timer.sleepFor(1.0);
    //daq.Scope.open();
    //daq.Scope.trigger();
    daq_dwf daq;
    std::cout << std::format("%s(%s) is selected.\n", daq.type, daq.serialNo);
    pSettings->pDaq = &daq;
    pSettings->sn = daq.serialNo;
    daq.powerSupply(5);
    daq.fg(pSettings->amp1, pSettings->freq, 0, 0.0, 0.0);

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
            double wt = 2 * PI * pSettings->freq * i * pSettings->rawDt;
            pSettings->rawData1[i] = pSettings->amp1 * std::sin(wt - phase);
        }
#else
        //daq.Scope.record(pSettings->rawData1.data());
        daq.ad_get(daq.adSettings.numSampsPerChan, pSettings->rawData1.data());// , pSettings->rawData2.data());
        daq.ad_start();
#endif // DAQ
        psd.calc(t);
    }
    pSettings->statusMeasurement = false;
}

//void server(std::stop_token st, Settings* pSettings)
//{
//    pSettings->statusServer = true;
//    while (!st.stop_requested())
//    {
//        std::string recieveCmd;
//        std::cout << "cmd: ";
//        std::cin >> recieveCmd;
//        //std::cout << recieveCmd;
//        if (recieveCmd.compare("get") == 0)
//        {
//            size_t idx = pSettings->idx;
//            std::cout << std::format(": %f,%f,%f", pSettings->times[idx], pSettings->xs[idx], pSettings->ys[idx]);
//        }
//        else if (recieveCmd.compare("end") == 0)
//        {
//            break;
//        }
//    }
//    pSettings->statusServer = false;
//}