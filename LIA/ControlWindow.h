#pragma once

#include "LiaConfig.h"
#include "ImGuiWindowBase.h"
#include <iostream>
#include <cmath>
#include <numbers> // For std::numbers::pi


class ControlWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    ControlWindow(GLFWwindow* window, LiaConfig& liaConfig)
		: ImGuiWindowBase(window, "Control panel"), liaConfig(liaConfig)
    {
        this->windowSize = ImVec2(450 * liaConfig.windowCfg.monitorScale, 920 * liaConfig.windowCfg.monitorScale);
    }
    void buttonPressed(const ButtonType button, const float value)
    {
        liaConfig.cmds.push_back(std::array<float, 3>{ (float)liaConfig.timer.elapsedSec(), (float)button, value });
	}
    void awg(const float nextItemWidth);
    void plot(const float nextItemWidth);
	void post(const float nextItemWidth);
    void monitor();
	void functionButtons(const float nextItemWidth);
    void show(void);
};

inline void ControlWindow::awg(const float nextItemWidth)
{
    // AWG設定のUIをここに実装
    ButtonType button = ButtonType::NON;
    float value = 0;
    bool fgFlag = false;
    if (ImGui::BeginTabBar("Awg"))
    {
        if (ImGui::BeginTabItem("W1"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awgCfg.ch[0].freq * 1e-3f;
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < LOW_LIMIT_FREQ * 1e-3) freqkHz = LOW_LIMIT_FREQ * 1e-3;
                if (freqkHz > HIGH_LIMIT_FREQ * 1e-3) freqkHz = HIGH_LIMIT_FREQ * 1e-3;
                if (liaConfig.awgCfg.ch[0].freq != freqkHz * 1e3f)
                {
                    liaConfig.awgCfg.ch[0].freq = freqkHz * 1e3f;
                    liaConfig.awgCfg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Freq;
                value = liaConfig.awgCfg.ch[0].freq;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh0Amp = liaConfig.awgCfg.ch[0].amp;
            if (ImGui::InputFloat("Amp. (V)", &(liaConfig.awgCfg.ch[0].amp), 0.1f, 0.1f, "%4.1f"))
            {
                if (liaConfig.awgCfg.ch[0].amp < 0.1f) liaConfig.awgCfg.ch[0].amp = 0.1f;
                if (liaConfig.awgCfg.ch[0].amp > 5.0f) liaConfig.awgCfg.ch[0].amp = AWG_AMP_MAX;
                if (oldCh0Amp != liaConfig.awgCfg.ch[0].amp)
                {
                    oldCh0Amp = liaConfig.awgCfg.ch[0].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Amp;
                value = liaConfig.awgCfg.ch[0].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            //static float oldCh0Phase = liaConfig.awg.ch[0].phase;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(liaConfig.awgCfg.ch[0].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Phase;
                value = liaConfig.awgCfg.ch[0].phase;
            }
            ImGui::EndTabItem();
            ImGui::EndDisabled();
        }
        if (ImGui::BeginTabItem("W2"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awgCfg.ch[1].freq * 1e-3f;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < LOW_LIMIT_FREQ * 1e-3) freqkHz = LOW_LIMIT_FREQ * 1e-3;
                if (freqkHz > HIGH_LIMIT_FREQ * 1e-3) freqkHz = HIGH_LIMIT_FREQ * 1e-3;
                if (liaConfig.awgCfg.ch[1].freq != freqkHz * 1e3f)
                {
                    liaConfig.awgCfg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Freq;
                value = liaConfig.awgCfg.ch[1].freq;
            }
            ImGui::EndDisabled();
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldFg2Amp = liaConfig.awgCfg.ch[1].amp;
            if (ImGui::InputFloat("Amp. (V)", &(liaConfig.awgCfg.ch[1].amp), 0.01f, 0.1f, "%4.2f"))
            {
                if (liaConfig.awgCfg.ch[1].amp < 0.0f) liaConfig.awgCfg.ch[1].amp = 0.0f;
                if (liaConfig.awgCfg.ch[1].amp > 5.0f) liaConfig.awgCfg.ch[1].amp = AWG_AMP_MAX;
                if (oldFg2Amp != liaConfig.awgCfg.ch[1].amp)
                {
                    oldFg2Amp = liaConfig.awgCfg.ch[1].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Amp;
                value = liaConfig.awgCfg.ch[1].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(liaConfig.awgCfg.ch[1].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Phase;
                value = liaConfig.awgCfg.ch[1].phase;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        if (liaConfig.pDaq != nullptr && fgFlag)
        {
            liaConfig.pDaq->awg.start(
                liaConfig.awgCfg.ch[0].freq, liaConfig.awgCfg.ch[0].amp, liaConfig.awgCfg.ch[0].phase,
                liaConfig.awgCfg.ch[1].freq, liaConfig.awgCfg.ch[1].amp, liaConfig.awgCfg.ch[1].phase
            );
        }
        if (button != ButtonType::NON) buttonPressed(button, value);
    }

}

inline void ControlWindow::plot(const float nextItemWidth)
{
    // Plot設定のUIをここに実装
    ButtonType button = ButtonType::NON;
    float value = 0;
    if (ImGui::BeginTabBar("Plot window"))
    {
        if (ImGui::BeginTabItem("XY window"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float step = 0.1f;
            if (liaConfig.plotCfg.limit <= 0.1f) step = 0.01f;
            else step = 0.1f;
            if (ImGui::InputFloat("Limit (V)", &(liaConfig.plotCfg.limit), step, 0.1f, "%.2f"))
            {
                if (liaConfig.plotCfg.limit < 0.01f) liaConfig.plotCfg.limit = 0.01f;
                if (liaConfig.plotCfg.limit > RAW_RANGE * 1.2f) liaConfig.plotCfg.limit = RAW_RANGE * 1.2f;
                if (0.1f < liaConfig.plotCfg.limit && liaConfig.plotCfg.limit < 0.2f) liaConfig.plotCfg.limit = 0.2f;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PlotLimit;
                value = liaConfig.plotCfg.limit;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (liaConfig.pauseCfg.flag) { ImGui::BeginDisabled(); }
            ImGui::InputDouble((char*)u8"Ch1 θ (Deg.)", &(liaConfig.postCfg.offset[0].phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset1Phase;
                value = liaConfig.postCfg.offset[0].phase;
            }
            if (!liaConfig.flagCh2) { ImGui::BeginDisabled(); }
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble((char*)u8"Ch2 θ (Deg.)", &(liaConfig.postCfg.offset[1].phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset2Phase;
                value = liaConfig.postCfg.offset[1].phase;
            }
            if (!liaConfig.flagCh2) { ImGui::EndDisabled(); }
            if (liaConfig.pauseCfg.flag) { ImGui::EndDisabled(); }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            const char* items[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue" , "NeonGreen" , "NeonRed", "Eva"};
            ImGui::ListBox("Thema", &liaConfig.imguiCfg.theme, items, IM_ARRAYSIZE(items), 3);

            ImGui::SameLine();
            if (liaConfig.imguiCfg.windowFlag == ImGuiCond_FirstUseEver)
            {
                if (ImGui::Button("Lock")) { liaConfig.imguiCfg.windowFlag = ImGuiCond_Always; }
            }
            else {
                if (ImGui::Button("Unlock")) { liaConfig.imguiCfg.windowFlag = ImGuiCond_FirstUseEver; }
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        if (button != ButtonType::NON) buttonPressed(button, value);
    }
}

inline void ControlWindow::post(const float nextItemWidth)
{
	// Post設定のUIをここに実装
	ButtonType button = ButtonType::NON;
	float value = 0;
    static ImVec2 autoOffsetSize = ImVec2(200.0f * liaConfig.windowCfg.monitorScale, 100.0f * liaConfig.windowCfg.monitorScale);
    if (liaConfig.pauseCfg.flag) { ImGui::BeginDisabled(); }
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        liaConfig.flagAutoOffset = true;
    }
    bool stateAutoOffset = true;
    if (liaConfig.postCfg.offset[0].x == 0 && liaConfig.postCfg.offset[0].y == 0)
    {
        stateAutoOffset = false;
        ImGui::BeginDisabled();
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostAutoOffset;
        value = 0;
    }
    ImGui::SameLine();
    static ImVec2 offSize = ImVec2(100.0f * liaConfig.windowCfg.monitorScale, autoOffsetSize.y);
    if (ImGui::Button("Off", offSize)) {
        liaConfig.postCfg.offset[0].x = 0.0f; liaConfig.postCfg.offset[0].y = 0.0f;
        liaConfig.postCfg.offset[1].x = 0.0f; liaConfig.postCfg.offset[1].y = 0.0f;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostOffsetOff;
        value = 0;
    }
    if (!stateAutoOffset) ImGui::EndDisabled();
    if (liaConfig.pauseCfg.flag) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * liaConfig.windowCfg.monitorScale, 0.0f));
    ImGui::SameLine();
    if (liaConfig.pauseCfg.flag)
    {
        if (ImGui::Button("Run", offSize)) { liaConfig.pauseCfg.flag = false; }
    }
    else {
        if (ImGui::Button("Pause", offSize)) { liaConfig.pauseCfg.flag = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostPause;
        value = liaConfig.pauseCfg.flag;
    }
    if (ImGui::TreeNode("Offset value"))
    {
        if (!stateAutoOffset) ImGui::BeginDisabled();
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", liaConfig.postCfg.offset[0].x, liaConfig.postCfg.offset[0].y);
        if (!liaConfig.flagCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", liaConfig.postCfg.offset[1].x, liaConfig.postCfg.offset[1].y);
        if (!liaConfig.flagCh2) ImGui::EndDisabled();
        ImGui::TreePop();
        if (!stateAutoOffset) ImGui::EndDisabled();
    }
	if (button != ButtonType::NON) buttonPressed(button, value);
}

inline void ControlWindow::monitor()
{
	// Monitor設定のUIをここに実装
    if (ImGui::TreeNode("Monitor"))
    {
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", liaConfig.ringBuffer.ch[0].x[liaConfig.ringBuffer.idx], liaConfig.ringBuffer.ch[0].y[liaConfig.ringBuffer.idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(liaConfig.ringBuffer.ch[0].x[liaConfig.ringBuffer.idx], 2) + pow(liaConfig.ringBuffer.ch[0].y[liaConfig.ringBuffer.idx], 2), 0.5),
            u8"θ",
            atan2(liaConfig.ringBuffer.ch[0].y[liaConfig.ringBuffer.idx], liaConfig.ringBuffer.ch[0].x[liaConfig.ringBuffer.idx]) / std::numbers::pi * 180
        );
        if (!liaConfig.flagCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", liaConfig.ringBuffer.ch[1].x[liaConfig.ringBuffer.idx], liaConfig.ringBuffer.ch[1].y[liaConfig.ringBuffer.idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(liaConfig.ringBuffer.ch[1].x[liaConfig.ringBuffer.idx], 2) + pow(liaConfig.ringBuffer.ch[1].y[liaConfig.ringBuffer.idx], 2), 0.5),
            u8"θ",
            atan2(liaConfig.ringBuffer.ch[1].y[liaConfig.ringBuffer.idx], liaConfig.ringBuffer.ch[1].x[liaConfig.ringBuffer.idx]) / std::numbers::pi * 180
        );
        if (!liaConfig.flagCh2) { ImGui::EndDisabled(); }
        ImGui::TreePop();
    }
}

inline void ControlWindow::functionButtons(const float nextItemWidth)
{
	ButtonType button = ButtonType::NON;
	float value = 0;
    ImGui::Checkbox("Surface Mode", &liaConfig.plotCfg.surfaceMode);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotSurfaceMode;
        value = liaConfig.plotCfg.surfaceMode;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Beep", &liaConfig.plotCfg.beep);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = liaConfig.plotCfg.surfaceMode;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = liaConfig.plotCfg.beep;
    }
    if (liaConfig.plotCfg.acfm)
    {
        liaConfig.flagCh2 = true;
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Ch2", &liaConfig.flagCh2);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::DispCh2;
        value = liaConfig.flagCh2;
    }
    if (liaConfig.plotCfg.acfm) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &liaConfig.plotCfg.acfm);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotACFM;
        value = liaConfig.plotCfg.acfm;
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("HPF (Hz)", &(liaConfig.postCfg.hpFreq), 0.1f, 1.0f, "%.1f"))
    {
        if (liaConfig.postCfg.hpFreq < 0.0f) liaConfig.postCfg.hpFreq = 0.0f;
        if (liaConfig.postCfg.hpFreq > 10.0f) liaConfig.postCfg.hpFreq = 10.0f;
		liaConfig.setHPFrequency(liaConfig.postCfg.hpFreq);
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostHpFreq;
        value = liaConfig.postCfg.hpFreq;
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("LPF (Hz)", &(liaConfig.postCfg.lpFreq), 1.0f, 10.0f, "%.0f"))
    {
        if (liaConfig.postCfg.lpFreq < 1.0f) liaConfig.postCfg.lpFreq = 1.0f;
        if (liaConfig.postCfg.lpFreq > 100.0f) liaConfig.postCfg.lpFreq = 100.0f;
        liaConfig.setLPFrequency(liaConfig.postCfg.lpFreq);
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostLpFreq;
        value = liaConfig.postCfg.lpFreq;
    }
    if (button != ButtonType::NON) buttonPressed(button, value);
}

inline void ControlWindow::show(void)
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    static float nextItemWidth = 170.0f * liaConfig.windowCfg.monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", liaConfig.device_sn.data());
    ImGui::SameLine();
    if (ImGui::Button("Close")) { liaConfig.statusMeasurement = false; }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::Close;
        value = 0;
    }
    awg(nextItemWidth);
    plot(nextItemWidth);
    ImGui::Separator();
    post(nextItemWidth);
    monitor();
    ImGui::Separator();
	functionButtons(nextItemWidth);
    ImGui::Separator();
	int hours = (int)liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx] / (60 * 60);
    int mins = ((int)liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx] - hours * 60 * 60) / 60;
    double secs = liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx] - hours * 60 * 60 - mins * 60;
    //ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);
    ImGui::Text("Time:%02d:%02d:%02.0f", hours, mins, secs);
    ImGui::End();
    if (button != ButtonType::NON) buttonPressed(button, value);
}