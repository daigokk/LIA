#pragma once

#include <iostream>
#include <cmath>
#include <numbers>
#include <thread>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <format>
#include "LiaConfig.h"
#include "ImGuiWindowBase.h"
#include "W2autosetup.h"

// ============================================================
// UI ユーティリティ関数
// ============================================================
// UI アイテムがDeactivatedされたときにボタンイベントを記録
inline void markButtonIfItemDeactivated(ButtonType& outButton, float& outValue, ButtonType id, float val) {
    if (ImGui::IsItemDeactivated()) {
        outButton = id;
        outValue = val;
    }
}

// 周波数を有効範囲内にクランプ
inline float clampFrequency(const float freqkHz, const LiaConfig& cfg) {
    return std::clamp(freqkHz, cfg.scope.getLowLimitFreq() * 1e-3f, cfg.scope.getHighLimitFreq() * 1e-3f);
}

// 振幅を有効範囲内にクランプ
inline float clampAmplitude(const float amp, const LiaConfig& cfg) {
    return std::clamp(amp, cfg.awg.AWG_AMP_MIN, cfg.awg.AWG_AMP_MAX);
}


// ============================================================
// UI ウィンドウクラス
// ============================================================
class ControlWindow : public ImGuiWindowBase
{
private:
    LiaConfig& cfg;
    void drawContent();

public:
    ControlWindow(GLFWwindow* window, LiaConfig& cfg)
        : ImGuiWindowBase(window, "Control panel"), cfg(cfg)
    {
        this->windowPos = ImVec2(1000 * cfg.window.monitorScale, 37 * cfg.window.monitorScale);
        this->windowSize = ImVec2(455 * cfg.window.monitorScale, 923 * cfg.window.monitorScale);
    }

    // ボタンイベント（時刻、ボタンID、値）をコマンド履歴に記録
    void buttonPressed(const ButtonType button, const float value)
    {
        cfg.cmds.push_back({
            static_cast<float>(cfg.timer.elapsedSec()),
            static_cast<float>(button),
            value
            });
    }

    void awg(const float nextItemWidth);
    void plot(const float nextItemWidth);
    void post(const float nextItemWidth);
    void monitor();
    void functionButtons(const float nextItemWidth);
    void show();
};

inline void ControlWindow::awg(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    bool configChanged = false;

    if (ImGui::BeginTabBar("Awg")) {
        // ===== W1（チャネル0）の設定 =====
        if (ImGui::BeginTabItem("W1")) {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = cfg.awg.ch[0].freq * 1e-3f;

            if (ImGui::InputFloat("Freq. (kHz)", &freqkHz, 1.0f, 1.0f, "%3.0f")) {
                freqkHz = clampFrequency(freqkHz, cfg);
                const float newFreqHz = freqkHz * 1e3f;
                if (cfg.awg.ch[0].freq != newFreqHz) {
                    cfg.awg.ch[0].freq = newFreqHz;
                    cfg.awg.ch[1].freq = newFreqHz; // W2も同期
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Freq, cfg.awg.ch[0].freq);

            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh0Amp = cfg.awg.ch[0].amp;
            if (ImGui::InputFloat("Amp. (V)", &cfg.awg.ch[0].amp, 0.1f, 0.1f, "%4.1f")) {
                cfg.awg.ch[0].amp = clampAmplitude(cfg.awg.ch[0].amp, cfg);
                if (oldCh0Amp != cfg.awg.ch[0].amp) {
                    oldCh0Amp = cfg.awg.ch[0].amp;
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Amp, cfg.awg.ch[0].amp);

            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::BeginDisabled();
            ImGui::InputFloat((const char*)u8"θ (Deg.)", &cfg.awg.ch[0].phase, 1.0f, 1.0f, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Phase, cfg.awg.ch[0].phase);
            ImGui::EndDisabled();
			
            ImGui::EndTabItem();
        }

        // ===== W2（チャネル1）の設定 =====
        if (ImGui::BeginTabItem("W2")) {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = cfg.awg.ch[1].freq * 1e-3f;

            ImGui::BeginDisabled();
            ImGui::InputFloat("Freq. (kHz)", &freqkHz, 1.0f, 1.0f, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Freq, cfg.awg.ch[1].freq);
            ImGui::EndDisabled();

            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh1Amp = cfg.awg.ch[1].amp;
            if (ImGui::InputFloat("Amp. (V)", &cfg.awg.ch[1].amp, 0.01f, 0.1f, "%4.2f")) {
                cfg.awg.ch[1].amp = std::clamp(cfg.awg.ch[1].amp, 0.0f, cfg.awg.AWG_AMP_MAX);
                if (oldCh1Amp != cfg.awg.ch[1].amp) {
                    oldCh1Amp = cfg.awg.ch[1].amp;
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Amp, cfg.awg.ch[1].amp);

            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat((const char*)u8"θ (Deg.)", &cfg.awg.ch[1].phase, 1.0f, 1.0f, "%3.0f")) {
                configChanged = true;
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Phase, cfg.awg.ch[1].phase);

            ImGui::SetNextItemWidth(nextItemWidth);
            if (cfg.flagAutoSetupW2) {
                ImGui::BeginDisabled();
                ImGui::Button("W2 Autosetup in progress");
                ImGui::EndDisabled();
            }
            else {
                if (ImGui::Button("W2 Autosetup")) {
                    cfg.flagAutoSetupW2 = true;
                    std::thread th_autosetup{ autosetupW2, &cfg };
                    th_autosetup.detach();
                }
            }
            static const char* funcNames[] = { "Sine", "Square", "Triangle" };
            int oldFunc = cfg.awg.ch[0].func - 1;
            if (ImGui::ListBox("Func", &oldFunc, funcNames, IM_ARRAYSIZE(funcNames), 3)) {
                cfg.awg.ch[0].func = oldFunc + 1;
                cfg.awg.ch[1].func = oldFunc + 1;
                configChanged = true;
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Func, (float)cfg.awg.ch[0].func);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        if (configChanged) {
            cfg.awgStart();
        }
        if (button != ButtonType::NON) {
            buttonPressed(button, value);
        }
    }
}

inline void ControlWindow::plot(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    bool configChanged = false;
    float value = 0;

    if (ImGui::BeginTabBar("Plot window")) {
        // ===== XY表示設定 =====
        if (ImGui::BeginTabItem("XY")) {
            ImGui::SetNextItemWidth(nextItemWidth);

            const float step = (cfg.plot.limit <= 0.1f) ? 0.01f : 0.1f;
            if (ImGui::InputFloat("Limit (V)", &cfg.plot.limit, step, 0.1f, "%.2f")) {
                cfg.plot.limit = std::clamp(cfg.plot.limit, 0.01f, cfg.scope.ch[0].range * 1.2f);
                if (0.1f < cfg.plot.limit && cfg.plot.limit < 0.2f) {
                    cfg.plot.limit = 0.2f;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::PlotLimit, cfg.plot.limit);

            ImGui::SetNextItemWidth(nextItemWidth);
            if (cfg.pause.flag) ImGui::BeginDisabled();

            ImGui::InputDouble((const char*)u8"Ch1 θ (Deg.)", &cfg.post.offset[0].phase, 1.0, 10.0, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::PostOffset1Phase, (float)cfg.post.offset[0].phase);

            if (!cfg.isCh2Enabled) ImGui::BeginDisabled();

            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble((const char*)u8"Ch2 θ (Deg.)", &cfg.post.offset[1].phase, 1.0, 10.0, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::PostOffset2Phase, (float)cfg.post.offset[1].phase);

            if (!cfg.isCh2Enabled) ImGui::EndDisabled();
            if (cfg.pause.flag) ImGui::EndDisabled();

            ImGui::EndTabItem();
        }
        // ===== Time chart表示設定 =====
        if (ImGui::BeginTabItem("Time chart")) {
            ImGui::SetNextItemWidth(nextItemWidth);
			float historySec = cfg.plot.historySec;
            if (ImGui::InputFloat("History (s)", &historySec, 1.0f, 10.0f, "%3.0f")) {
                cfg.plot.historySec = std::clamp(historySec, 1.0f, (float)cfg.ringBuffer.getSec());
            }
            ImGui::Dummy(ImVec2(0.0f * cfg.window.monitorScale, 80.0f * cfg.window.monitorScale));
            ImGui::EndTabItem();
        }
		// ===== Scope設定 =====
        if (ImGui::BeginTabItem("Scope")) {
            ImGui::SetNextItemWidth(nextItemWidth*0.75f);
            static const char* rangeNames[] = { "+-2.5V", "+-25V" };
            int oldRange1 = ((int)(cfg.scope.ch[0].range / 25.0f));
            if (ImGui::ListBox("Ch1", &oldRange1, rangeNames, IM_ARRAYSIZE(rangeNames), 2)) {
                if (oldRange1 == 0) { cfg.scope.ch[0].range = 2.5f; }
                else { cfg.scope.ch[0].range = 25.0f; }
                configChanged = true;
            }
			ImGui::SameLine(); ImGui::SetNextItemWidth(nextItemWidth*0.75f);
            int oldRange2 = ((int)(cfg.scope.ch[1].range / 25.0f));
            if (ImGui::ListBox("Ch2", &oldRange2, rangeNames, IM_ARRAYSIZE(rangeNames), 2)) {
                if (oldRange2 == 0) { cfg.scope.ch[1].range = 2.5f; }
                else { cfg.scope.ch[1].range = 25.0f; }
                configChanged = true;
            }
            ImGui::Dummy(ImVec2(0.0f * cfg.window.monitorScale, 31.0f * cfg.window.monitorScale));
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        if (configChanged) {
            cfg.scope.setMaxRange();
            cfg.pDaq->scope.open(cfg.scope.ch[0].range, cfg.scope.ch[1].range, cfg.scope.getBufferSize(), 1.0 / cfg.scope.getSamplingDt());
            cfg.pDaq->scope.trigger();
            cfg.pDaq->scope.start();
        }
        if (button != ButtonType::NON) {
            buttonPressed(button, value);
        }
    }
}

inline void ControlWindow::post(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;

    static const ImVec2 autoOffsetSize(200.0f * cfg.window.monitorScale, 100.0f * cfg.window.monitorScale);
    static const ImVec2 buttonSize(100.0f * cfg.window.monitorScale, autoOffsetSize.y);

    const bool offsetIsValid = (cfg.post.offset[0].x != 0.0f || cfg.post.offset[0].y != 0.0f);

    if (cfg.pause.flag) ImGui::BeginDisabled();

    // 自動オフセットボタン
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        cfg.buttonAutoOffset();
    }

    ImGui::SameLine();

    // オフセットリセットボタン
    if (!offsetIsValid) ImGui::BeginDisabled();
    if (ImGui::Button("Off", buttonSize)) {
        cfg.post.offset[0] = { 0.0f, 0.0f, cfg.post.offset[0].phase };
        cfg.post.offset[1] = { 0.0f, 0.0f, cfg.post.offset[1].phase };
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostOffsetOff, 0);
    if (!offsetIsValid) ImGui::EndDisabled();

    if (cfg.pause.flag) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * cfg.window.monitorScale, 0.0f));
    ImGui::SameLine();

    // 実行/一時停止トグル
    if (ImGui::Button(cfg.pause.flag ? "Run" : "Pause", buttonSize)) {
		cfg.buttonPause();
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostPause, cfg.pause.flag);

    // オフセット値の表示
    if (ImGui::TreeNode("Offset value")) {
        if (!offsetIsValid) ImGui::BeginDisabled();

        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", cfg.post.offset[0].x, cfg.post.offset[0].y);

        if (!cfg.isCh2Enabled) ImGui::BeginDisabled();
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", cfg.post.offset[1].x, cfg.post.offset[1].y);
        if (!cfg.isCh2Enabled) ImGui::EndDisabled();

        ImGui::TreePop();
        if (!offsetIsValid) ImGui::EndDisabled();
    }

    if (button != ButtonType::NON) {
        buttonPressed(button, value);
    }
}

inline void ControlWindow::monitor()
{
    if (ImGui::TreeNode("Monitor")) {
        const size_t idx = cfg.ringBuffer.latestIdx;

        // Ch1 データ表示
        const float ch0_x = (float)cfg.ringBuffer.ch[0].x[idx];
        const float ch0_y = (float)cfg.ringBuffer.ch[0].y[idx];
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", ch0_x, ch0_y);
        ImGui::Text((const char*)u8"Amp:%4.2fV, θ:%4.0fDeg.", std::hypot(ch0_x, ch0_y), std::atan2(ch0_y, ch0_x) * 180.0f / 3.14159265358979323846f);

        // Ch2 データ表示
        if (!cfg.isCh2Enabled) ImGui::BeginDisabled();

        const float ch1_x = (float)cfg.ringBuffer.ch[1].x[idx];
        const float ch1_y = (float)cfg.ringBuffer.ch[1].y[idx];
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", ch1_x, ch1_y);
        ImGui::Text((const char*)u8"Amp:%4.2fV, θ:%4.0fDeg.", std::hypot(ch1_x, ch1_y), std::atan2(ch1_y, ch1_x) * 180.0f / 3.14159265358979323846f);

        if (!cfg.isCh2Enabled) ImGui::EndDisabled();

        ImGui::TreePop();
    }
}

inline void ControlWindow::functionButtons(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;

    ImGui::Checkbox("Surface Mode", &cfg.plot.surfaceMode);
    markButtonIfItemDeactivated(button, value, ButtonType::PlotSurfaceMode, cfg.plot.surfaceMode);

    ImGui::SameLine();
    ImGui::Checkbox("Beep", &cfg.plot.beep);
    markButtonIfItemDeactivated(button, value, ButtonType::PlotBeep, cfg.plot.beep);

    // ACFM有効時はCh2を強制有効化
    if (cfg.window.acfmWindow) {
        cfg.isCh2Enabled = true;
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Ch2", &cfg.isCh2Enabled);
    markButtonIfItemDeactivated(button, value, ButtonType::DispCh2, cfg.isCh2Enabled);
    if (cfg.window.acfmWindow) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &cfg.window.acfmWindow);
    markButtonIfItemDeactivated(button, value, ButtonType::PlotACFM, cfg.window.acfmWindow);
    // フィルタ設定
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("HPF (Hz)", &cfg.post.hpFreq, 0.1f, 1.0f, "%.1f")) {
        cfg.post.hpFreq = std::clamp(cfg.post.hpFreq, 0.0f, 10.0f);
        cfg.setHPFrequency(cfg.post.hpFreq);
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostHpFreq, cfg.post.hpFreq);

    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("LPF (Hz)", &cfg.post.lpFreq, 1.0f, 10.0f, "%.0f")) {
        cfg.post.lpFreq = std::clamp(cfg.post.lpFreq, 1.0f, 100.0f);
        cfg.setLPFrequency(cfg.post.lpFreq);
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostLpFreq, cfg.post.lpFreq);

    if (button != ButtonType::NON) {
        buttonPressed(button, value);
    }
}

inline void ControlWindow::drawContent()
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    const float nextItemWidth = 170.0f * cfg.window.monitorScale;

    // ヘッダ
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", cfg.device_sn.data());
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        cfg.statusMeasurement = false;
    }
    markButtonIfItemDeactivated(button, value, ButtonType::Close, 0);

    awg(nextItemWidth);
    plot(nextItemWidth);
    ImGui::Separator();

    post(nextItemWidth);
    monitor();
    ImGui::Separator();

    functionButtons(nextItemWidth);
    ImGui::Separator();

    // 経過時間表示
    const int totalSecs = static_cast<int>(cfg.ringBuffer.times[cfg.ringBuffer.latestIdx]);
    const int hours = totalSecs / 3600;
    const int mins = (totalSecs % 3600) / 60;
    const int secs = (int)cfg.ringBuffer.times[cfg.ringBuffer.latestIdx] - (hours * 3600) - (mins * 60);
    ImGui::Text("Time:%02d:%02d:%02d", hours, mins, secs);

    if (button != ButtonType::NON) {
        buttonPressed(button, value);
    }
}

inline void ControlWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, cfg.window.imGuiCondFlag);
    ImGui::SetNextWindowSize(windowSize, cfg.window.imGuiCondFlag);

    if (ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag)) {
        if (cfg.flagAutoSetupW2) {
            ImGui::BeginDisabled();
            drawContent();
            ImGui::EndDisabled();
        }
        else {
            drawContent();
        }
    }
    ImGui::End();
}