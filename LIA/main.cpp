#include <iostream>
#include <thread>
#include <cstring>

#define DAQ
#ifdef DAQ
#include <daq_dwf.hpp>
#endif // DAQ

#include "Settings.h"
#include "Gui.h"
#include "Timer.h"

void thStart(Settings* pSettings);
void thStop(volatile flag_t* pFlagMeasurement);
void measurement(Settings* pSettings);

int main(void)
{
    static Settings settings;
    Gui gui(&settings);
    if (gui.initialized == false) return -1;
    while (!gui.windowShouldClose())
    {
        settings.flagMeasurement.trigger = true;
        thStart(&settings);
        while (settings.flagMeasurement.status)
        {
            if (gui.windowShouldClose()) break;
            /* Poll for and process events */
            gui.pollEvents();

            gui.show();

            gui.rendering();

            gui.swapBuffers();
        }
        thStop(&(settings.flagMeasurement));
    }
    return 0;
}

void thStart(Settings* pSettings)
{
    if (!pSettings->flagMeasurement.status && pSettings->flagMeasurement.trigger)
    {
        pSettings->flagMeasurement.trigger = true;
        std::thread th_measurement(measurement, pSettings);
        th_measurement.detach();
        while (!pSettings->flagMeasurement.status);
    }
}

void thStop(volatile flag_t* pFlagMeasurement)
{
    pFlagMeasurement->trigger = false;
    while (pFlagMeasurement->status);
}

void measurement(Settings* pSettings)
{
    // Psdオブジェクトをヒープ上に作成し、スタック使用量を削減
    std::unique_ptr<Psd> psd = std::make_unique<Psd>(pSettings);
    Timer timer;
#ifdef DAQ
    //Daq_wf daq;
    //daq.powerSupply(5.0);
    //daq.Fg.start(pSettings->freq, pSettings->amp, 0.0);
    //timer.sleepFor(1.0);
    //daq.Scope.open();
    //daq.Scope.trigger();
    daq_dwf daq;
    pSettings->pDaq = &daq;
    pSettings->sn = daq.serialNo;
    daq.powerSupply(5);
    daq.fg(pSettings->amp1, pSettings->freq, 0, 0.0, 0.0);

    daq.adSettings.ch = 0; daq.adSettings.numCh = 2; daq.adSettings.range = 2.5;
    daq.adSettings.triggerDigCh = -1; daq.adSettings.waitAd = 0;
    daq.adSettings.numSampsPerChan = pSettings->rawTime.size();
    daq.adSettings.rate = 1.0 / (pSettings->rawDt);
    daq.ad_init(daq.adSettings);
    daq.ad_start();
#endif // DAQ
    timer.start();
    pSettings->flagMeasurement.status = true;
    while (pSettings->flagMeasurement.trigger)
    {
        double t = pSettings->nofm * MEASUREMENT_DT;
        t = timer.sleepUntil(t);
#ifndef DAQ
        double phase = pSettings->offsetPhase / 180 * PI;
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
        psd->calc(t);
    }
    pSettings->flagMeasurement.trigger = false;
    pSettings->flagMeasurement.status = false;
}
