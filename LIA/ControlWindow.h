#pragma once

#include "LiaConfig.h"
#include "ImGuiWindowBase.h"
#include <iostream>
#include <cmath>
#include <numbers> // For std::numbers::pi


class ControlWindow : public ImGuiWindowBase
{
private:
    LiaConfig* pLiaConfig = nullptr;
public:
    ControlWindow(GLFWwindow* window, LiaConfig* pLiaConfig)
        : ImGuiWindowBase(window, "Control panel")
    {
		this->pLiaConfig = pLiaConfig;
        this->windowSize = ImVec2(450 * pLiaConfig->window.monitorScale, 920 * pLiaConfig->window.monitorScale);
    }
    void buttonPressed(const ButtonType button, const float value)
    {
        pLiaConfig->cmds.push_back(std::array<float, 3>{ (float)pLiaConfig->timer.elapsedSec(), (float)button, value });
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
            float freqkHz = pLiaConfig->awg.ch[0].freq * 1e-3f;
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (pLiaConfig->awg.ch[0].freq != freqkHz * 1e3f)
                {
                    pLiaConfig->awg.ch[0].freq = freqkHz * 1e3f;
                    pLiaConfig->awg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Freq;
                value = pLiaConfig->awg.ch[0].freq;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh0Amp = pLiaConfig->awg.ch[0].amp;
            if (ImGui::InputFloat("Amp. (V)", &(pLiaConfig->awg.ch[0].amp), 0.1f, 0.1f, "%4.1f"))
            {
                if (pLiaConfig->awg.ch[0].amp < 0.1f) pLiaConfig->awg.ch[0].amp = 0.1f;
                if (pLiaConfig->awg.ch[0].amp > 5.0f) pLiaConfig->awg.ch[0].amp = 5.0f;
                if (oldCh0Amp != pLiaConfig->awg.ch[0].amp)
                {
                    oldCh0Amp = pLiaConfig->awg.ch[0].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Amp;
                value = pLiaConfig->awg.ch[0].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            //static float oldCh0Phase = pLiaConfig->awg.ch[0].phase;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(pLiaConfig->awg.ch[0].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Phase;
                value = pLiaConfig->awg.ch[0].phase;
            }
            ImGui::EndTabItem();
            ImGui::EndDisabled();
        }
        if (ImGui::BeginTabItem("W2"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = pLiaConfig->awg.ch[1].freq * 1e-3f;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (pLiaConfig->awg.ch[1].freq != freqkHz * 1e3f)
                {
                    pLiaConfig->awg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Freq;
                value = pLiaConfig->awg.ch[1].freq;
            }
            ImGui::EndDisabled();
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldFg2Amp = pLiaConfig->awg.ch[1].amp;
            if (ImGui::InputFloat("Amp. (V)", &(pLiaConfig->awg.ch[1].amp), 0.01f, 0.1f, "%4.2f"))
            {
                if (pLiaConfig->awg.ch[1].amp < 0.0f) pLiaConfig->awg.ch[1].amp = 0.0f;
                if (pLiaConfig->awg.ch[1].amp > 5.0f) pLiaConfig->awg.ch[1].amp = 5.0f;
                if (oldFg2Amp != pLiaConfig->awg.ch[1].amp)
                {
                    oldFg2Amp = pLiaConfig->awg.ch[1].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Amp;
                value = pLiaConfig->awg.ch[1].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(pLiaConfig->awg.ch[1].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Phase;
                value = pLiaConfig->awg.ch[1].phase;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        if (pLiaConfig->pDaq != nullptr && fgFlag)
        {
            pLiaConfig->pDaq->awg.start(
                pLiaConfig->awg.ch[0].freq, pLiaConfig->awg.ch[0].amp, pLiaConfig->awg.ch[0].phase,
                pLiaConfig->awg.ch[1].freq, pLiaConfig->awg.ch[1].amp, pLiaConfig->awg.ch[1].phase
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
            if (pLiaConfig->plot.limit <= 0.1f) step = 0.01f;
            else step = 0.1f;
            if (ImGui::InputFloat("Limit (V)", &(pLiaConfig->plot.limit), step, 0.1f, "%4.2f"))
            {
                if (pLiaConfig->plot.limit < 0.01f) pLiaConfig->plot.limit = 0.01f;
                if (pLiaConfig->plot.limit > RAW_RANGE * 1.2f) pLiaConfig->plot.limit = RAW_RANGE * 1.2f;
                if (0.1f < pLiaConfig->plot.limit && pLiaConfig->plot.limit < 0.2f) pLiaConfig->plot.limit = 0.2f;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PlotLimit;
                value = pLiaConfig->plot.limit;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (pLiaConfig->flagPause) { ImGui::BeginDisabled(); }
            ImGui::InputDouble((char*)u8"Ch1 θ (Deg.)", &(pLiaConfig->post.offset1Phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset1Phase;
                value = pLiaConfig->post.offset1Phase;
            }
            if (!pLiaConfig->flagCh2) { ImGui::BeginDisabled(); }
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble((char*)u8"Ch2 θ (Deg.)", &(pLiaConfig->post.offset2Phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset2Phase;
                value = pLiaConfig->post.offset2Phase;
            }
            if (!pLiaConfig->flagCh2) { ImGui::EndDisabled(); }
            if (pLiaConfig->flagPause) { ImGui::EndDisabled(); }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            const char* items[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue" , "NeonGreen" , "NeonRed" };
            ImGui::ListBox("Thema", &pLiaConfig->imgui.theme, items, IM_ARRAYSIZE(items), 3);

            ImGui::SameLine();
            if (pLiaConfig->imgui.windowFlag == ImGuiCond_FirstUseEver)
            {
                if (ImGui::Button("Lock")) { pLiaConfig->imgui.windowFlag = ImGuiCond_Always; }
            }
            else {
                if (ImGui::Button("Unlock")) { pLiaConfig->imgui.windowFlag = ImGuiCond_FirstUseEver; }
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
    static ImVec2 autoOffsetSize = ImVec2(200.0f * pLiaConfig->window.monitorScale, 100.0f * pLiaConfig->window.monitorScale);
    if (pLiaConfig->flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        pLiaConfig->flagAutoOffset = true;
    }
    bool stateAutoOffset = true;
    if (pLiaConfig->post.offset1X == 0 && pLiaConfig->post.offset1Y == 0)
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
    static ImVec2 offSize = ImVec2(100.0f * pLiaConfig->window.monitorScale, autoOffsetSize.y);
    if (ImGui::Button("Off", offSize)) {
        pLiaConfig->post.offset1X = 0.0f; pLiaConfig->post.offset1Y = 0.0f;
        pLiaConfig->post.offset2X = 0.0f; pLiaConfig->post.offset2Y = 0.0f;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostOffsetOff;
        value = 0;
    }
    if (!stateAutoOffset) ImGui::EndDisabled();
    if (pLiaConfig->flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * pLiaConfig->window.monitorScale, 0.0f));
    ImGui::SameLine();
    if (pLiaConfig->flagPause)
    {
        if (ImGui::Button("Run", offSize)) { pLiaConfig->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause", offSize)) { pLiaConfig->flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostPause;
        value = pLiaConfig->flagPause;
    }
    if (ImGui::TreeNode("Offset value"))
    {
        if (!stateAutoOffset) ImGui::BeginDisabled();
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", pLiaConfig->post.offset1X, pLiaConfig->post.offset1Y);
        if (!pLiaConfig->flagCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", pLiaConfig->post.offset2X, pLiaConfig->post.offset2Y);
        if (!pLiaConfig->flagCh2) ImGui::EndDisabled();
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
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", pLiaConfig->x1s[pLiaConfig->idx], pLiaConfig->y1s[pLiaConfig->idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(pLiaConfig->x1s[pLiaConfig->idx], 2) + pow(pLiaConfig->y1s[pLiaConfig->idx], 2), 0.5),
            u8"θ",
            atan2(pLiaConfig->y1s[pLiaConfig->idx], pLiaConfig->x1s[pLiaConfig->idx]) / std::numbers::pi * 180
        );
        if (!pLiaConfig->flagCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", pLiaConfig->x2s[pLiaConfig->idx], pLiaConfig->y2s[pLiaConfig->idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(pLiaConfig->x2s[pLiaConfig->idx], 2) + pow(pLiaConfig->y2s[pLiaConfig->idx], 2), 0.5),
            u8"θ",
            atan2(pLiaConfig->y2s[pLiaConfig->idx], pLiaConfig->x2s[pLiaConfig->idx]) / std::numbers::pi * 180
        );
        if (!pLiaConfig->flagCh2) { ImGui::EndDisabled(); }
        ImGui::TreePop();
    }
}

inline void ControlWindow::functionButtons(const float nextItemWidth)
{
	ButtonType button = ButtonType::NON;
	float value = 0;
    ImGui::Checkbox("Surface Mode", &pLiaConfig->plot.surfaceMode);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotSurfaceMode;
        value = pLiaConfig->plot.surfaceMode;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Beep", &pLiaConfig->plot.beep);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = pLiaConfig->plot.surfaceMode;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = pLiaConfig->plot.beep;
    }
    if (pLiaConfig->plot.acfm)
    {
        pLiaConfig->flagCh2 = true;
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Ch2", &pLiaConfig->flagCh2);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::DispCh2;
        value = pLiaConfig->flagCh2;
    }
    if (pLiaConfig->plot.acfm) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &pLiaConfig->plot.acfm);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotACFM;
        value = pLiaConfig->plot.acfm;
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    //if (pLiaConfig->flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::InputFloat("HPF (Hz)", &(pLiaConfig->post.hpFreq), 0.1f, 1.0f, "%4.1f"))
    {
        if (pLiaConfig->post.hpFreq < 0.0f) pLiaConfig->post.hpFreq = 0.0f;
        if (pLiaConfig->post.hpFreq > 50.0f) pLiaConfig->post.hpFreq = 50.0f;
		pLiaConfig->setHPFrequency(pLiaConfig->post.hpFreq);
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostHpFreq;
        value = pLiaConfig->post.hpFreq;
    }
    //if (pLiaConfig->flagPause) { ImGui::EndDisabled(); }
    if (button != ButtonType::NON) buttonPressed(button, value);
}

inline void ControlWindow::show(void)
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    static float nextItemWidth = 170.0f * pLiaConfig->window.monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), pLiaConfig->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pLiaConfig->imgui.windowFlag);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", pLiaConfig->device_sn.data());
    ImGui::SameLine();
    if (ImGui::Button("Close")) { pLiaConfig->statusMeasurement = false; }
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
	int hours = (int)pLiaConfig->times[pLiaConfig->idx] / (60 * 60);
    int mins = ((int)pLiaConfig->times[pLiaConfig->idx] - hours * 60 * 60) / 60;
    double secs = pLiaConfig->times[pLiaConfig->idx] - hours * 60 * 60 - mins * 60;
    //ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);
    ImGui::Text("Time:%02d:%02d:%02.0f", hours, mins, secs);
    ImGui::End();
    if (button != ButtonType::NON) buttonPressed(button, value);
}