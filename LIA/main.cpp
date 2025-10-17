#include <format>
#include <iostream>
#include <numbers> // For std::numbers::pi
#include <stop_token> // std::jthread
#include <thread> // std::jthread
#include <array>
#include <cstring> // for std::memset
#include <intrin.h> // for __cpuid

#define NOMINMAX
#include <Windows.h>
#include <Daq_wf.h>
#include "pipe.h"
#include "Psd.h"
#include "Gui.h"
#include "LiaConfig.h"
#include "GuiSub.h"

bool is_avx2_supported();
void measurement(std::stop_token st, LiaConfig* pLiaConfig);
void measurementWithoutDaq(std::stop_token st, LiaConfig* pLiaConfig);

int main(int argc, char* argv[])
{
    int adIdx = Daq_dwf::getIdxFirstEnabledDevice();
    if (adIdx != -1)
    {
        std::cout << "Analog Discovery is detected." << std::endl;
    }
    else
    {
        std::cout << "No DAQ is connected." << std::endl;
    }
    
    //is_avx2_supported();
    // I/Oの高速化
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr); // std::cin と std::cout の同期を解除
	std::cout.tie(nullptr); // std::cout と std::cin の同期を解除
    static LiaConfig settings;
    bool guiFlag = true;
	bool pipeFlag = false;
    std::jthread* pth_pipe = nullptr;
    std::unique_ptr<GuiSub> pGuiSub;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "pipe") {
            pipeFlag = true;
            //std::cout << "Pipe mode." << std::endl;
        }
        else if (arg == "nogui" || arg == "headless") {
            guiFlag = false;
            pipeFlag = true;
            //std::cout << "Headless pipe mode." << std::endl;
        }
    }
    if (guiFlag)
    {
        Gui::Initialize(
            "Lock-in amplifier",
            settings.window.pos.x, settings.window.pos.y,
            settings.window.size.x, settings.window.size.y
        );
        if (Gui::GetWindow() == nullptr) return -1;
		settings.window.monitorScale = Gui::monitorScale;
        if(Gui::SurfacePro7) {
            settings.imgui.windowFlag = ImGuiCond_Always;
		}
        pGuiSub = std::make_unique<GuiSub>(Gui::GetWindow(), settings);
    }
    if (pipeFlag)
    {
        pth_pipe = new std::jthread(pipe, &settings);
        while (!settings.statusPipe);
    }
    
    std::jthread th_measurement = (adIdx == -1)
        ? std::jthread{ measurementWithoutDaq, &settings }
        : std::jthread{ measurement, &settings };
    // スレッドの優先度を設定
    HANDLE handle = th_measurement.native_handle();
    if (!SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST)) {
        std::cerr << "Failed to set thread priority.\n";
    }
    while (!settings.statusMeasurement);
	std::cout << std::flush; // バッファをフラッシュ
    if (guiFlag)
    {
		//// GUIありモードでは、ウィンドウが閉じられるか測定が終了するまでループ
        while (settings.statusMeasurement &&
            !(pth_pipe && !settings.statusPipe) &&
            !glfwWindowShouldClose(Gui::GetWindow())) {
            Gui::BeginFrame();
            pGuiSub->show();
            Gui::EndFrame();
        }
        glfwGetWindowSize(Gui::GetWindow(), &(settings.window.size.x), &(settings.window.size.y));
        glfwGetWindowPos(Gui::GetWindow(), &(settings.window.pos.x), &(settings.window.pos.y));
        Gui::EndFrame();
    }
    else
    {
		// GUIなしモードでは、パイプが閉じられるか測定が終了するまで待機
        while (settings.statusMeasurement)
        {
            if (pth_pipe != nullptr && settings.statusPipe == false) break;
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    th_measurement.request_stop();
    if (pth_pipe) pth_pipe->request_stop();
    th_measurement.join();
    if (pth_pipe) pth_pipe->join();
    return 0;
}

void measurement(std::stop_token st, LiaConfig* pLiaConfig)
{
    Daq_dwf daq;
    daq.powerSupply(5.0);
    daq.awg.start(
        pLiaConfig->awg.ch[0].freq, pLiaConfig->awg.ch[0].amp, pLiaConfig->awg.ch[0].phase,
        pLiaConfig->awg.ch[1].freq, pLiaConfig->awg.ch[1].amp, pLiaConfig->awg.ch[1].phase
    );
    daq.scope.open(RAW_RANGE, RAW_SIZE, 1.0 / RAW_DT);
    daq.scope.trigger();
    pLiaConfig->pDaq = &daq;
    pLiaConfig->device_sn = daq.device.sn;
    pLiaConfig->timer.sleepFor(1.0);
    daq.scope.start();
    pLiaConfig->timer.start();
    pLiaConfig->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = pLiaConfig->timer.sleepUntil(t);
        if (pLiaConfig->flagPause) continue;
        if (!pLiaConfig->flagCh2)
        {
            daq.scope.record(pLiaConfig->rawData[0].data());
        }
        else {
            daq.scope.record(pLiaConfig->rawData[0].data(), pLiaConfig->rawData[1].data());
        }
        pLiaConfig->update(t);
    }
    pLiaConfig->statusMeasurement = false;
}

void measurementWithoutDaq(std::stop_token st, LiaConfig* pLiaConfig)
{
    pLiaConfig->timer.start();
    pLiaConfig->statusMeasurement = true;
    size_t nloop = 0;
    while (!st.stop_requested())
    {
        double t = nloop * MEASUREMENT_DT;
        nloop++;
        t = pLiaConfig->timer.sleepUntil(t);
        if (pLiaConfig->flagPause) continue;
        double phase = 2 * std::numbers::pi * t / 60;
        for (size_t i = 0; i < pLiaConfig->rawTime.size(); i++) {
            double wt = 2 * std::numbers::pi * pLiaConfig->awg.ch[0].freq * i * RAW_DT;
            pLiaConfig->rawData[0][i] = pLiaConfig->awg.ch[0].amp * std::sin(wt - phase);
            if (pLiaConfig->flagCh2) {
                pLiaConfig->rawData[1][i] = pLiaConfig->awg.ch[1].amp * std::sin(wt - pLiaConfig->awg.ch[1].phase);
            }
        }
        pLiaConfig->update(t);
    }
    pLiaConfig->statusMeasurement = false;
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
