//#define TEST

#include <array>
#include <iostream>
#include <string_view>
#include <vector>
#include <memory>
#include <thread>
#include <stop_token>
#include <numbers>
#include <cmath>

#define NOMINMAX
#include <Windows.h>

// プロジェクト固有のヘッダー
#include <Daq_wf.h>
#include "pipe.h"
#include "Psd.h"
#include "Gui.h"
#include "LiaConfig.h"
#include "GuiSub.h"

// --- 定数・構造体 ---
struct LaunchOptions {
    bool useGui = true;
    bool usePipe = false;
};

// --- 関数プロトタイプ ---
void runMeasurement(std::stop_token st, LiaConfig* cfg);
void runSimulatedMeasurement(std::stop_token st, LiaConfig* cfg);
LaunchOptions parseArguments(int argc, char* argv[]);

// --- ユーティリティ ---

LaunchOptions parseArguments(int argc, char* argv[]) {
    LaunchOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "pipe") {
            options.usePipe = true;
        }
        else if (arg == "nogui" || arg == "headless") {
            options.useGui = false;
            options.usePipe = true; // headless時は通常pipeを有効化
        }
    }
    return options;
}

// --- メインロジック ---

int main(int argc, char* argv[]) {
#ifdef TEST
    try {
        test_psd();
        test_pipe();
        test_w2autosetup();
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
#else
    // 標準入出力の高速化
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    const auto options = parseArguments(argc, argv);

    static LiaConfig settings;
    const int adIdx = Daq_dwf::getIdxFirstEnabledDevice();
    const bool isDeviceConnected = (adIdx != -1);

    std::cout << (isDeviceConnected ? "Analog Discovery detected." : "DAQ not connected.") << std::endl;

    // 1. GUI初期化
    std::unique_ptr<GuiSub> guiSub;
    if (options.useGui) {
        GuiConfig guiCfg{
            .title = "Lock-in amplifier",
            .posX = settings.window.pos.x,
            .posY = settings.window.pos.y,
            .width = settings.window.size.x,
            .height = settings.window.size.y,
            .fontSize = settings.window.fontSize
        };

        Gui::Initialize(guiCfg);
        if (!Gui::GetWindow()) return -1;

        settings.window.monitorScale = Gui::monitorScale;
        if (Gui::isSurfacePro7) {
            settings.window.imGuiCondFlag = ImGuiCond_Always;
        }
        guiSub = std::make_unique<GuiSub>(Gui::GetWindow(), settings);
    }

    // 2. パイプスレッド開始
    std::unique_ptr<std::jthread> pipeThread;
    if (options.usePipe) {
        pipeThread = std::make_unique<std::jthread>(pipe, &settings);
        while (!settings.statusPipe); // 通信準備完了待機
    }

    // 3. 測定スレッド開始
    auto measurementFunc = isDeviceConnected ? runMeasurement : runSimulatedMeasurement;
    std::jthread measurementThread(measurementFunc, &settings);

    // スレッド優先度の設定
    if (!SetThreadPriority(measurementThread.native_handle(), THREAD_PRIORITY_HIGHEST)) {
        std::cerr << "Warning: Failed to set thread priority.\n";
    }

    while (!settings.statusMeasurement); // 測定開始待機
    std::cout << std::flush;

    // 4. メインループ
    if (options.useGui) {
        auto* window = Gui::GetWindow();
        while (settings.statusMeasurement && !glfwWindowShouldClose(window)) {
            // パイプが切断された場合の終了チェック
            if (pipeThread && !settings.statusPipe) break;

            Gui::BeginFrame();
            guiSub->show();
            Gui::EndFrame();
        }
        // ウィンドウ情報の保存
        glfwGetWindowSize(window, &settings.window.size.x, &settings.window.size.y);
        glfwGetWindowPos(window, &settings.window.pos.x, &settings.window.pos.y);
        Gui::EndFrame();
    }
    else {
        // Headlessモード: 測定終了かパイプ切断まで待機
        while (settings.statusMeasurement) {
            if (pipeThread && !settings.statusPipe) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // 5. 終了処理 (jthreadのデストラクタが自動でjoinを呼ぶが、明示的にstopを要求)
    measurementThread.request_stop();
    if (pipeThread) pipeThread->request_stop();
#endif // !TEST
    return 0;
}

// --- 測定処理の実装 ---

void runMeasurement(std::stop_token st, LiaConfig* pCfg) {
    Daq_dwf daq;

    // DAQ初期設定
    daq.powerSupply(5.0);
    const auto& ch0 = pCfg->awg.ch[0];
    const auto& ch1 = pCfg->awg.ch[1];
    daq.awg.start(ch0.freq, ch0.amp, ch0.phase, ch0.func, ch1.freq, ch1.amp, ch1.phase, ch1.func);

    daq.scope.open(pCfg->scope.ch[0].range, pCfg->scope.ch[1].range, pCfg->scope.getBufferSize(), 1.0 / pCfg->scope.getSamplingDt());
    daq.scope.trigger();

    pCfg->pDaq = &daq;
    pCfg->device_sn = daq.device.sn;
    pCfg->timer.sleepFor(1.0); // 安定待ち

    daq.scope.start();
    pCfg->timer.start();
    pCfg->statusMeasurement = true;

    for (size_t nloop = 0; !st.stop_requested(); ++nloop) {
        double t = pCfg->timer.sleepUntil(nloop * pCfg->ringBuffer.getDt());

        if (pCfg->pause.flag) continue;

        if (pCfg->scope.ch[1].enable) {
            daq.scope.record(pCfg->raw.waveforms[0].data(), pCfg->raw.waveforms[1].data());
        }
        else {
            daq.scope.record(pCfg->raw.waveforms[0].data());
        }

        pCfg->update(t);
    }
    pCfg->statusMeasurement = false;
}

void runSimulatedMeasurement(std::stop_token st, LiaConfig* pCfg) {
    pCfg->timer.start();
    pCfg->statusMeasurement = true;

    for (size_t nloop = 0; !st.stop_requested(); ++nloop) {
        double t = pCfg->timer.sleepUntil(nloop * pCfg->ringBuffer.getDt());

        if (pCfg->pause.flag) continue;

        // シミュレーション波形の生成
        const double phaseShift = 2.0 * std::numbers::pi * t / 60.0;
        const auto& ch0 = pCfg->awg.ch[0];
        const auto& ch1 = pCfg->awg.ch[1];

        for (size_t i = 0; i < pCfg->raw.waveforms[0].size(); ++i) {
            double wt = 2.0 * std::numbers::pi * ch0.freq * i * pCfg->scope.getSamplingDt();
            pCfg->raw.waveforms[0][i] = ch0.amp * std::sin(wt - phaseShift);

            if (pCfg->scope.ch[1].enable) {
                pCfg->raw.waveforms[1][i] = ch1.amp * std::sin(wt - ch1.phase);
            }
        }

        pCfg->update(t);
    }
    pCfg->statusMeasurement = false;
}