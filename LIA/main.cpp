#include <format>
#include <iostream>
#include <numbers> // For std::numbers::pi
#include <stop_token> // std::jthread
#include <thread> // std::jthread
#include <array>
#include <cstring> // for std::memset
#include <intrin.h> 

#define NOMINMAX
#include <Windows.h>
#include <Daq_wf.h>
#include "Gui.h"
#include "pipe.h"
#include "Psd.h"
#include "Settings.h"

bool is_avx2_supported();
void measurement(std::stop_token st, Settings* pSettings);
void measurementWithoutDaq(std::stop_token st, Settings* pSettings);

int main(int argc, char* argv[])
{
    //is_avx2_supported();
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
    for (int i = 0; i < argc; i++)
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
    Daq_dwf daq;
    daq.powerSupply(5.0);
    daq.awg.start(
        pSettings->awg.ch[0].freq, pSettings->awg.ch[0].amp, pSettings->awg.ch[0].phase,
        pSettings->awg.ch[1].freq, pSettings->awg.ch[1].amp, pSettings->awg.ch[1].phase
    );
    daq.scope.open(RAW_RANGE, RAW_SIZE, 1.0 / RAW_DT);
    daq.scope.trigger();
    std::cout << std::format("{:s}({:s}) is selected.\n", daq.device.name, daq.device.sn);
    pSettings->pDaq = &daq;
    pSettings->device_sn = daq.device.sn;
    pSettings->timer.sleepFor(1.0);
    daq.scope.start();
    pSettings->timer.start();
    pSettings->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = pSettings->timer.sleepUntil(t);
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
    pSettings->timer.start();
    pSettings->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = pSettings->timer.sleepUntil(t);
        if (pSettings->flagPause) continue;
        double phase = 2 * std::numbers::pi * t / 60;
        for (size_t i = 0; i < pSettings->rawTime.size(); i++)
        {
            double wt = 2 * std::numbers::pi * pSettings->awg.ch[0].freq * i * RAW_DT;

            pSettings->rawData1[i] = pSettings->awg.ch[0].amp * std::sin(wt - phase);
            if (pSettings->flagCh2)
            {
                pSettings->rawData2[i] = pSettings->awg.ch[1].amp * std::sin(wt - pSettings->awg.ch[1].phase);
            }
        }
        psd.calc(t);
    }
    pSettings->statusMeasurement = false;
}


/**
 * CPUID命令を使ってAVX2のサポートをチェックする関数
 * @return AVX2がサポートされていれば true
 */
bool is_avx2_supported() {
	bool avx2_supported = false;
#ifdef __AVX2__
    // AVX2が有効な状態でコンパイルされています。
    // ここにAVX2固有のコード（__m256i, _mm256_... 組み込み関数など）を記述できます。
    std::cout << "AVX2 is enabled at compile time." << std::endl;
    avx2_supported = true;
#else
    // AVX2は有効になっていません。
    // AVX2固有のコードを直接使用することはできません。
    std::cout << "AVX2 is NOT enabled at compile time." << std::endl;
#endif
    std::array<int, 4> cpu_info;
    std::memset(cpu_info.data(), 0, sizeof(cpu_info));

    // EAX=7 (Extended Features) のサブリーフ 0 を呼び出す
    // EAX=7, ECX=0
    // MSVC (Windows)
    __cpuidex(cpu_info.data(), 7, 0);


    // EAX=7, サブリーフ 0 の結果は EBX レジスタ (cpu_info[1]) に格納される
    // AVX2 の機能フラグは EBX のビット 5 (0x20)
    constexpr int AVX2_FLAG = 1 << 5; // Bit 5

    // EBX & AVX2_FLAG の結果が非ゼロならAVX2をサポート

    if ((cpu_info[1] & AVX2_FLAG) != 0) {
        std::cout << "Runtime check: AVX2 is supported by the CPU." << std::endl;
        avx2_supported = true;
    }
    else {
        std::cout << "Runtime check: AVX2 is NOT supported by the CPU." << std::endl;
    }

    return avx2_supported;
}