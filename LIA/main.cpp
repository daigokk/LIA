#include <format>
#include <iostream>
#include <numbers> // For std::numbers::pi
#include <stop_token> // std::jthread
#include <thread> // std::jthread
#define NOMINMAX
#include <Windows.h>

#include <Daq_wf.h>

#include "Gui.h"
#include "pipe.h"
#include "Psd.h"
#include "Settings.h"
#include "Timer.h"
void measurement(std::stop_token st, Settings* pSettings);
void measurementWithoutDaq(std::stop_token st, Settings* pSettings);

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false); // For std::cout and cin
    static Settings settings;
    Gui gui(&settings);
    if (gui.initialized == false) return -1;
    int adIdx = Daq_dwf::getIdxFirstEnabledDevice();
    std::jthread th_measurement;
    if (adIdx == -1)
    {
        std::cout << "No DAQ is connected." << std::endl;
       th_measurement = std::jthread{ measurementWithoutDaq, &settings };
    }
    else {
        th_measurement = std::jthread{ measurement, &settings };
    }
    // スレッドの優先度を設定
    HANDLE handle = th_measurement.native_handle();
    if (!SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST)) {
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
    Daq_dwf daq;
    daq.powerSupply(5.0);
    daq.awg.start(
        pSettings->awg.w1Freq, pSettings->awg.w1Amp, pSettings->awg.w1Phase,
        pSettings->awg.w2Freq, pSettings->awg.w2Amp, pSettings->awg.w2Phase
    );
    daq.scope.open(RAW_RANGE, RAW_SIZE, 1.0 / RAW_DT);
    daq.scope.trigger();
    std::cout << std::format("{:s}({:s}) is selected.\n", daq.device.name, daq.device.sn);
    pSettings->pDaq = &daq;
    pSettings->device_sn = daq.device.sn;
    timer.sleepFor(1.0); 
    daq.scope.start();
    timer.start();
    pSettings->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = timer.sleepUntil(t);
        if (pSettings->flagPause) continue;
        if (!pSettings->flagCh2)
        {
            daq.scope.record(pSettings->rawData1.data());
        }
        else {
            daq.scope.record(pSettings->rawData1.data(), pSettings->rawData2.data());
        }
        psd.calc(t);
    }
    pSettings->statusMeasurement = false;
}

void measurementWithoutDaq(std::stop_token st, Settings* pSettings)
{
    static Psd psd(pSettings);
    Timer timer;
    timer.start();
    pSettings->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = timer.sleepUntil(t);
        if (pSettings->flagPause) continue;
        double phase = 2 * std::numbers::pi * t / 60;
        for (size_t i = 0; i < pSettings->rawTime.size(); i++)
        {
            double wt = 2 * std::numbers::pi * pSettings->awg.w1Freq * i * RAW_DT;

            pSettings->rawData1[i] = pSettings->awg.w1Amp * std::sin(wt - phase);
            if (pSettings->flagCh2)
            {
                pSettings->rawData2[i] = pSettings->awg.w2Amp * std::sin(wt - pSettings->awg.w2Phase);
            }
        }
        psd.calc(t);
    }
    pSettings->statusMeasurement = false;
}
