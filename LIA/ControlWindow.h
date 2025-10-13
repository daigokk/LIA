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
        this->windowSize = ImVec2(450 * liaConfig.window.monitorScale, 920 * liaConfig.window.monitorScale);
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
        static float lowLimitFreqkHz = (float)(0.5 * 1e-3 / (RAW_SIZE * RAW_DT));
        static float highLimitFreqkHz = (float)(1e-3f / (1000 * RAW_DT));
        if (ImGui::BeginTabItem("W1"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awg.ch[0].freq * 1e-3f;
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (liaConfig.awg.ch[0].freq != freqkHz * 1e3f)
                {
                    liaConfig.awg.ch[0].freq = freqkHz * 1e3f;
                    liaConfig.awg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Freq;
                value = liaConfig.awg.ch[0].freq;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh0Amp = liaConfig.awg.ch[0].amp;
            if (ImGui::InputFloat("Amp. (V)", &(liaConfig.awg.ch[0].amp), 0.1f, 0.1f, "%4.1f"))
            {
                if (liaConfig.awg.ch[0].amp < 0.1f) liaConfig.awg.ch[0].amp = 0.1f;
                if (liaConfig.awg.ch[0].amp > 5.0f) liaConfig.awg.ch[0].amp = 5.0f;
                if (oldCh0Amp != liaConfig.awg.ch[0].amp)
                {
                    oldCh0Amp = liaConfig.awg.ch[0].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Amp;
                value = liaConfig.awg.ch[0].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            //static float oldCh0Phase = liaConfig.awg.ch[0].phase;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(liaConfig.awg.ch[0].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Phase;
                value = liaConfig.awg.ch[0].phase;
            }
            ImGui::EndTabItem();
            ImGui::EndDisabled();
        }
        if (ImGui::BeginTabItem("W2"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awg.ch[1].freq * 1e-3f;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (liaConfig.awg.ch[1].freq != freqkHz * 1e3f)
                {
                    liaConfig.awg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Freq;
                value = liaConfig.awg.ch[1].freq;
            }
            ImGui::EndDisabled();
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldFg2Amp = liaConfig.awg.ch[1].amp;
            if (ImGui::InputFloat("Amp. (V)", &(liaConfig.awg.ch[1].amp), 0.01f, 0.1f, "%4.2f"))
            {
                if (liaConfig.awg.ch[1].amp < 0.0f) liaConfig.awg.ch[1].amp = 0.0f;
                if (liaConfig.awg.ch[1].amp > 5.0f) liaConfig.awg.ch[1].amp = 5.0f;
                if (oldFg2Amp != liaConfig.awg.ch[1].amp)
                {
                    oldFg2Amp = liaConfig.awg.ch[1].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Amp;
                value = liaConfig.awg.ch[1].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(liaConfig.awg.ch[1].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Phase;
                value = liaConfig.awg.ch[1].phase;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        if (liaConfig.pDaq != nullptr && fgFlag)
        {
            liaConfig.pDaq->awg.start(
                liaConfig.awg.ch[0].freq, liaConfig.awg.ch[0].amp, liaConfig.awg.ch[0].phase,
                liaConfig.awg.ch[1].freq, liaConfig.awg.ch[1].amp, liaConfig.awg.ch[1].phase
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
            if (liaConfig.plot.limit <= 0.1f) step = 0.01f;
            else step = 0.1f;
            if (ImGui::InputFloat("Limit (V)", &(liaConfig.plot.limit), step, 0.1f, "%4.2f"))
            {
                if (liaConfig.plot.limit < 0.01f) liaConfig.plot.limit = 0.01f;
                if (liaConfig.plot.limit > RAW_RANGE * 1.2f) liaConfig.plot.limit = RAW_RANGE * 1.2f;
                if (0.1f < liaConfig.plot.limit && liaConfig.plot.limit < 0.2f) liaConfig.plot.limit = 0.2f;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PlotLimit;
                value = liaConfig.plot.limit;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (liaConfig.flagPause) { ImGui::BeginDisabled(); }
            ImGui::InputDouble((char*)u8"Ch1 θ (Deg.)", &(liaConfig.post.offset1Phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset1Phase;
                value = liaConfig.post.offset1Phase;
            }
            if (!liaConfig.flagCh2) { ImGui::BeginDisabled(); }
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble((char*)u8"Ch2 θ (Deg.)", &(liaConfig.post.offset2Phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset2Phase;
                value = liaConfig.post.offset2Phase;
            }
            if (!liaConfig.flagCh2) { ImGui::EndDisabled(); }
            if (liaConfig.flagPause) { ImGui::EndDisabled(); }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            const char* items[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue" , "NeonGreen" , "NeonRed" };
            ImGui::ListBox("Thema", &liaConfig.imgui.theme, items, IM_ARRAYSIZE(items), 3);

            ImGui::SameLine();
            if (liaConfig.imgui.windowFlag == ImGuiCond_FirstUseEver)
            {
                if (ImGui::Button("Lock")) { liaConfig.imgui.windowFlag = ImGuiCond_Always; }
            }
            else {
                if (ImGui::Button("Unlock")) { liaConfig.imgui.windowFlag = ImGuiCond_FirstUseEver; }
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
    static ImVec2 autoOffsetSize = ImVec2(200.0f * liaConfig.window.monitorScale, 100.0f * liaConfig.window.monitorScale);
    if (liaConfig.flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        liaConfig.flagAutoOffset = true;
    }
    bool stateAutoOffset = true;
    if (liaConfig.post.offset1X == 0 && liaConfig.post.offset1Y == 0)
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
    static ImVec2 offSize = ImVec2(100.0f * liaConfig.window.monitorScale, autoOffsetSize.y);
    if (ImGui::Button("Off", offSize)) {
        liaConfig.post.offset1X = 0.0f; liaConfig.post.offset1Y = 0.0f;
        liaConfig.post.offset2X = 0.0f; liaConfig.post.offset2Y = 0.0f;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostOffsetOff;
        value = 0;
    }
    if (!stateAutoOffset) ImGui::EndDisabled();
    if (liaConfig.flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * liaConfig.window.monitorScale, 0.0f));
    ImGui::SameLine();
    if (liaConfig.flagPause)
    {
        if (ImGui::Button("Run", offSize)) { liaConfig.flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause", offSize)) { liaConfig.flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostPause;
        value = liaConfig.flagPause;
    }
    if (ImGui::TreeNode("Offset value"))
    {
        if (!stateAutoOffset) ImGui::BeginDisabled();
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", liaConfig.post.offset1X, liaConfig.post.offset1Y);
        if (!liaConfig.flagCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", liaConfig.post.offset2X, liaConfig.post.offset2Y);
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
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", liaConfig.x1s[liaConfig.idx], liaConfig.y1s[liaConfig.idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(liaConfig.x1s[liaConfig.idx], 2) + pow(liaConfig.y1s[liaConfig.idx], 2), 0.5),
            u8"θ",
            atan2(liaConfig.y1s[liaConfig.idx], liaConfig.x1s[liaConfig.idx]) / std::numbers::pi * 180
        );
        if (!liaConfig.flagCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", liaConfig.x2s[liaConfig.idx], liaConfig.y2s[liaConfig.idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(liaConfig.x2s[liaConfig.idx], 2) + pow(liaConfig.y2s[liaConfig.idx], 2), 0.5),
            u8"θ",
            atan2(liaConfig.y2s[liaConfig.idx], liaConfig.x2s[liaConfig.idx]) / std::numbers::pi * 180
        );
        if (!liaConfig.flagCh2) { ImGui::EndDisabled(); }
        ImGui::TreePop();
    }
}

inline void ControlWindow::functionButtons(const float nextItemWidth)
{
	ButtonType button = ButtonType::NON;
	float value = 0;
    ImGui::Checkbox("Surface Mode", &liaConfig.plot.surfaceMode);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotSurfaceMode;
        value = liaConfig.plot.surfaceMode;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Beep", &liaConfig.plot.beep);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = liaConfig.plot.surfaceMode;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = liaConfig.plot.beep;
    }
    if (liaConfig.plot.acfm)
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
    if (liaConfig.plot.acfm) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &liaConfig.plot.acfm);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotACFM;
        value = liaConfig.plot.acfm;
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    //if (liaConfig.flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::InputFloat("HPF (Hz)", &(liaConfig.post.hpFreq), 0.1f, 1.0f, "%4.1f"))
    {
        if (liaConfig.post.hpFreq < 0.0f) liaConfig.post.hpFreq = 0.0f;
        if (liaConfig.post.hpFreq > 50.0f) liaConfig.post.hpFreq = 50.0f;
		liaConfig.setHPFrequency(liaConfig.post.hpFreq);
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostHpFreq;
        value = liaConfig.post.hpFreq;
    }
    //if (liaConfig.flagPause) { ImGui::EndDisabled(); }
    if (button != ButtonType::NON) buttonPressed(button, value);
}

inline void ControlWindow::show(void)
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    static float nextItemWidth = 170.0f * liaConfig.window.monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), liaConfig.imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imgui.windowFlag);
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
	int hours = (int)liaConfig.times[liaConfig.idx] / (60 * 60);
    int mins = ((int)liaConfig.times[liaConfig.idx] - hours * 60 * 60) / 60;
    double secs = liaConfig.times[liaConfig.idx] - hours * 60 * 60 - mins * 60;
    //ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);
    ImGui::Text("Time:%02d:%02d:%02.0f", hours, mins, secs);
    ImGui::End();
    if (button != ButtonType::NON) buttonPressed(button, value);
}