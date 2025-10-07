#pragma once

#include "Settings.h"
#include "ImGuiWindowBase.h"
#include <iostream>
#include <cmath>
#include <numbers> // For std::numbers::pi


class ControlWindow : public ImGuiWindowBase
{
private:
    Settings* pSettings = nullptr;
public:
    ControlWindow(GLFWwindow* window, Settings* pSettings)
        : ImGuiWindowBase(window, "Control panel")
    {
		this->pSettings = pSettings;
    }
    void show(void);
};

inline void ControlWindow::show(void)
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    bool fgFlag = false;
    static ImVec2 windowSize = ImVec2(450 * pSettings->window.monitorScale, 920 * pSettings->window.monitorScale);
    static float nextItemWidth = 170.0f * pSettings->window.monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), pSettings->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pSettings->imgui.windowFlag);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", pSettings->device_sn.data());
    ImGui::SameLine();
    if (ImGui::Button("Close")) { pSettings->statusMeasurement = false; }
    if (ImGui::BeginTabBar("Awg"))
    {
        static float lowLimitFreqkHz = (float)(0.5 * 1e-3 / (RAW_SIZE * RAW_DT));
        static float highLimitFreqkHz = (float)(1e-3f / (1000 * RAW_DT));
        if (ImGui::BeginTabItem("W1"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = pSettings->awg.ch[0].freq * 1e-3f;
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (pSettings->awg.ch[0].freq != freqkHz * 1e3f)
                {
                    pSettings->awg.ch[0].freq = freqkHz * 1e3f;
                    pSettings->awg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Freq;
                value = pSettings->awg.ch[0].freq;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh0Amp = pSettings->awg.ch[0].amp;
            if (ImGui::InputFloat("Volt. (V)", &(pSettings->awg.ch[0].amp), 0.1f, 0.1f, "%4.1f"))
            {
                if (pSettings->awg.ch[0].amp < 0.1f) pSettings->awg.ch[0].amp = 0.1f;
                if (pSettings->awg.ch[0].amp > 5.0f) pSettings->awg.ch[0].amp = 5.0f;
                if (oldCh0Amp != pSettings->awg.ch[0].amp)
                {
                    oldCh0Amp = pSettings->awg.ch[0].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Amp;
                value = pSettings->awg.ch[0].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            //static float oldCh0Phase = pSettings->awg.ch[0].phase;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(pSettings->awg.ch[0].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW1Phase;
                value = pSettings->awg.ch[0].phase;
            }
            ImGui::EndTabItem();
            ImGui::EndDisabled();
        }
        if (ImGui::BeginTabItem("W2"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = pSettings->awg.ch[1].freq * 1e-3f;
            ImGui::BeginDisabled();
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (pSettings->awg.ch[1].freq != freqkHz * 1e3f)
                {
                    pSettings->awg.ch[1].freq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Freq;
                value = pSettings->awg.ch[1].freq;
            }
            ImGui::EndDisabled();
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldFg2Amp = pSettings->awg.ch[1].amp;
            if (ImGui::InputFloat("Volt. (V)", &(pSettings->awg.ch[1].amp), 0.01f, 0.1f, "%4.2f"))
            {
                if (pSettings->awg.ch[1].amp < 0.0f) pSettings->awg.ch[1].amp = 0.0f;
                if (pSettings->awg.ch[1].amp > 5.0f) pSettings->awg.ch[1].amp = 5.0f;
                if (oldFg2Amp != pSettings->awg.ch[1].amp)
                {
                    oldFg2Amp = pSettings->awg.ch[1].amp;
                    fgFlag = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Amp;
                value = pSettings->awg.ch[1].amp;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat((char*)u8"θ (Deg.)", &(pSettings->awg.ch[1].phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::AwgW2Phase;
                value = pSettings->awg.ch[1].phase;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    bool stateCh2 = pSettings->flagCh2;
    if (ImGui::BeginTabBar("Plot window"))
    {
        if (ImGui::BeginTabItem("XY window"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            float step = 0.1f;
            if (pSettings->plot.limit <= 0.1f) step = 0.01f;
            else step = 0.1f;
            if (ImGui::InputFloat("Limit (V)", &(pSettings->plot.limit), step, 0.1f, "%4.2f"))
            {
                if (pSettings->plot.limit < 0.01f) pSettings->plot.limit = 0.01f;
                if (pSettings->plot.limit > RAW_RANGE * 1.2f) pSettings->plot.limit = RAW_RANGE * 1.2f;
                if (0.1f < pSettings->plot.limit && pSettings->plot.limit < 0.2f) pSettings->plot.limit = 0.2f;
            }
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PlotLimit;
                value = pSettings->plot.limit;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (pSettings->flagPause) { ImGui::BeginDisabled(); }
            ImGui::InputDouble((char*)u8"Ch1 θ (Deg.)", &(pSettings->post.offset1Phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset1Phase;
                value = pSettings->post.offset1Phase;
            }
            if (!stateCh2) { ImGui::BeginDisabled(); }
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble((char*)u8"Ch2 θ (Deg.)", &(pSettings->post.offset2Phase), 1.0, 10.0, "%3.0f");
            if (ImGui::IsItemDeactivated()) {
                // ボタンが離された瞬間（フォーカスが外れた）
                button = ButtonType::PostOffset2Phase;
                value = pSettings->post.offset2Phase;
            }
            if (!stateCh2) { ImGui::EndDisabled(); }
            if (pSettings->flagPause) { ImGui::EndDisabled(); }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            const char* items[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue" , "NeonGreen" , "NeonRed" };
            ImGui::ListBox("Thema", &pSettings->imgui.theme, items, IM_ARRAYSIZE(items), 3);

            ImGui::SameLine();
            if (pSettings->imgui.windowFlag == ImGuiCond_FirstUseEver)
            {
                if (ImGui::Button("Lock")) { pSettings->imgui.windowFlag = ImGuiCond_Always; }
            }
            else {
                if (ImGui::Button("Unlock")) { pSettings->imgui.windowFlag = ImGuiCond_FirstUseEver; }
            }

            ImGui::EndTabItem();
        }
    }
    ImGui::Separator();
    ImGui::Checkbox("Surface Mode", &pSettings->plot.surfaceMode);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotSurfaceMode;
        value = pSettings->plot.surfaceMode;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Beep", &pSettings->plot.beep);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotBeep;
        value = pSettings->plot.beep;
    }
    ImGui::EndTabBar();
    ImGui::Separator();
    static ImVec2 autoOffsetSize = ImVec2(200.0f * pSettings->window.monitorScale, 100.0f * pSettings->window.monitorScale);
    if (pSettings->flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        pSettings->flagAutoOffset = true;
    }
    bool stateAutoOffset = true;
    if (pSettings->post.offset1X == 0 && pSettings->post.offset1Y == 0)
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
    static ImVec2 offSize = ImVec2(100.0f * pSettings->window.monitorScale, autoOffsetSize.y);
    if (ImGui::Button("Off", offSize)) {
        pSettings->post.offset1X = 0.0f; pSettings->post.offset1Y = 0.0f;
        pSettings->post.offset2X = 0.0f; pSettings->post.offset2Y = 0.0f;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostOffsetOff;
        value = 0;
    }
    if (!stateAutoOffset) ImGui::EndDisabled();
    if (pSettings->flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * pSettings->window.monitorScale, 0.0f));
    ImGui::SameLine();
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run", offSize)) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause", offSize)) { pSettings->flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostPause;
        value = pSettings->flagPause;
    }
    if (ImGui::TreeNode("Offset value"))
    {
        if (!stateAutoOffset) ImGui::BeginDisabled();
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", pSettings->post.offset1X, pSettings->post.offset1Y);
        if (!stateCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", pSettings->post.offset2X, pSettings->post.offset2Y);
        if (!stateCh2) ImGui::EndDisabled();
        ImGui::TreePop();
        if (!stateAutoOffset) ImGui::EndDisabled();
    }
    ImGui::Separator();

    //bool stateACFM = false;
    if (pSettings->plot.acfm)
    {
        //stateACFM = true;
        pSettings->flagCh2 = true;
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Ch2", &pSettings->flagCh2);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::DispCh2;
        value = pSettings->flagCh2;
    }
    if (pSettings->plot.acfm) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &pSettings->plot.acfm);
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PlotACFM;
        value = pSettings->plot.acfm;
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    //if (pSettings->flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::InputFloat("HPF (Hz)", &(pSettings->post.hpFreq), 0.1f, 1.0f, "%4.1f"))
    {
        if (pSettings->post.hpFreq < 0.0f) pSettings->post.hpFreq = 0.0f;
        if (pSettings->post.hpFreq > 50.0f) pSettings->post.hpFreq = 50.0f;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::PostHpFreq;
        value = pSettings->post.hpFreq;
    }
    //if (pSettings->flagPause) { ImGui::EndDisabled(); }
    ImGui::Separator();
    if (ImGui::TreeNode("Monitor"))
    {
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", pSettings->x1s[pSettings->idx], pSettings->y1s[pSettings->idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(pSettings->x1s[pSettings->idx], 2) + pow(pSettings->y1s[pSettings->idx], 2), 0.5),
            u8"θ",
            atan2(pSettings->y1s[pSettings->idx], pSettings->x1s[pSettings->idx]) / std::numbers::pi * 180
        );
        if (!stateCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", pSettings->x2s[pSettings->idx], pSettings->y2s[pSettings->idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(pSettings->x2s[pSettings->idx], 2) + pow(pSettings->y2s[pSettings->idx], 2), 0.5),
            u8"θ",
            atan2(pSettings->y2s[pSettings->idx], pSettings->x2s[pSettings->idx]) / std::numbers::pi * 180
        );
        if (!stateCh2) { ImGui::EndDisabled(); }
        ImGui::TreePop();
    }
    int hours = (int)pSettings->times[pSettings->idx] / (60 * 60);
    int mins = ((int)pSettings->times[pSettings->idx] - hours * 60 * 60) / 60;
    double secs = pSettings->times[pSettings->idx] - hours * 60 * 60 - mins * 60;
    ImGui::Separator();
    //ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);
    ImGui::Text("Time:%02d:%02d:%02.0f", hours, mins, secs);

    if (pSettings->pDaq != nullptr && fgFlag)
    {
        pSettings->pDaq->awg.start(
            pSettings->awg.ch[0].freq, pSettings->awg.ch[0].amp, pSettings->awg.ch[0].phase,
            pSettings->awg.ch[1].freq, pSettings->awg.ch[1].amp, pSettings->awg.ch[1].phase
        );
    }
    ImGui::End();
    if (button != ButtonType::NON)
    {
        pSettings->cmds.push_back(std::array<float, 3>{ (float)pSettings->timer.elapsedSec(), (float)button, value });
    }
}