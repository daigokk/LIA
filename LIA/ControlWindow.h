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
    bool fgFlag = false;
    static ImVec2 windowSize = ImVec2(450 * pSettings->monitorScale, 920 * pSettings->monitorScale);
    static float nextItemWidth = 170.0f * pSettings->monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", pSettings->sn.data());
    if (ImGui::BeginTabBar("Fg"))
    {
        if (ImGui::BeginTabItem("W1"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            static float lowLimitFreqkHz = (float)(0.5 * 1e-3 / (RAW_SIZE * pSettings->rawDt));
            static float highLimitFreqkHz = (float)(1e-3f / (1000 * pSettings->rawDt));
            float freqkHz = pSettings->fgFreq * 1e-3f;
            if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
            {
                if (freqkHz < lowLimitFreqkHz) freqkHz = lowLimitFreqkHz;
                if (freqkHz > highLimitFreqkHz) freqkHz = highLimitFreqkHz;
                if (pSettings->fgFreq != freqkHz * 1e3f)
                {
                    pSettings->fgFreq = freqkHz * 1e3f;
                    fgFlag = true;
                }
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldFg1Amp = pSettings->fg1Amp;
            if (ImGui::InputFloat("Volt. (V)", &(pSettings->fg1Amp), 0.1f, 0.1f, "%4.1f"))
            {
                if (pSettings->fg1Amp < 0.1f) pSettings->fg1Amp = 0.1f;
                if (pSettings->fg1Amp > 5.0f) pSettings->fg1Amp = 5.0f;
                if (oldFg1Amp != pSettings->fg1Amp)
                {
                    oldFg1Amp = pSettings->fg1Amp;
                    fgFlag = true;
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("W2"))
        {
            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldFg2Amp = pSettings->fg2Amp;
            if (ImGui::InputFloat("Volt. (V)", &(pSettings->fg2Amp), 0.01f, 0.1f, "%4.2f"))
            {
                if (pSettings->fg2Amp < 0.0f) pSettings->fg2Amp = 0.0f;
                if (pSettings->fg2Amp > 5.0f) pSettings->fg2Amp = 5.0f;
                if (oldFg2Amp != pSettings->fg2Amp)
                {
                    oldFg2Amp = pSettings->fg2Amp;
                    fgFlag = true;
                }
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat("Phase (Deg.)", &(pSettings->fg2Phase), 1, 1, "%3.0f"))
            {
                fgFlag = true;
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
            if (pSettings->limit <= 0.1f) step = 0.01f;
            else step = 0.1f;
            if (ImGui::InputFloat("Limit (V)", &(pSettings->limit), step, 0.1f, "%4.2f"))
            {
                if (pSettings->limit < 0.01f) pSettings->limit = 0.01f;
                if (pSettings->limit > RAW_RANGE * 1.2f) pSettings->limit = RAW_RANGE * 1.2f;
                if (0.1f < pSettings->limit && pSettings->limit < 0.2f) pSettings->limit = 0.2f;
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            if (pSettings->flagPause) { ImGui::BeginDisabled(); }
            ImGui::InputDouble("Ch1 Phase(Deg.)", &(pSettings->offset1Phase), 1.0, 10.0, "%3.0f");
            if (!stateCh2) { ImGui::BeginDisabled(); }
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble("Ch2 Phase(Deg.)", &(pSettings->offset2Phase), 1.0, 10.0, "%3.0f");
            if (!stateCh2) { ImGui::EndDisabled(); }
            if (pSettings->flagPause) { ImGui::EndDisabled(); }
            ImGui::EndTabItem();
        }
        ImGui::Checkbox("Surface Mode", &pSettings->flagSurfaceMode);
        ImGui::EndTabBar();
    }
    ImGui::Separator();
    static ImVec2 autoOffsetSize = ImVec2(200.0f * pSettings->monitorScale, 100.0f * pSettings->monitorScale);
    if (pSettings->flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        pSettings->flagAutoOffset = true;
    }
    bool stateAutoOffset = true;
    if (pSettings->offset1X == 0 && pSettings->offset1Y == 0)
    {
        stateAutoOffset = false;
        ImGui::BeginDisabled();
    }
    ImGui::SameLine();
    static ImVec2 offSize = ImVec2(100.0f * pSettings->monitorScale, autoOffsetSize.y);
    if (ImGui::Button("Off", offSize)) {
        pSettings->offset1X = 0.0f; pSettings->offset1Y = 0.0f;
        pSettings->offset2X = 0.0f; pSettings->offset2Y = 0.0f;
    }
    if (!stateAutoOffset) ImGui::EndDisabled();
    if (pSettings->flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * pSettings->monitorScale, 0.0f));
    ImGui::SameLine();
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run", offSize)) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause", offSize)) { pSettings->flagPause = true; }
    }
    if (ImGui::TreeNode("Offset value"))
    {
        if (!stateAutoOffset) ImGui::BeginDisabled();
        ImGui::Text("Ch1 X:%5.2fV, Y:%5.2fV", pSettings->offset1X, pSettings->offset1Y);
        if (!stateCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV, Y:%5.2fV", pSettings->offset2X, pSettings->offset2Y);
        if (!stateCh2) ImGui::EndDisabled();
        ImGui::TreePop();
        if (!stateAutoOffset) ImGui::EndDisabled();
    }
    ImGui::Separator();
    
    bool stateACFM = false;
    if (pSettings->flagACFM)
    {
        stateACFM = true;
        pSettings->flagCh2 = true;
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Ch2", &pSettings->flagCh2);
    if (stateACFM) ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Checkbox("Beep", &pSettings->flagBeep);
    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &pSettings->flagACFM);
    ImGui::SetNextItemWidth(nextItemWidth);
    if (pSettings->flagPause) { ImGui::BeginDisabled(); }
    if (ImGui::InputFloat("HP Filter(Hz)", &(pSettings->hpFreq), 0.1f, 1.0f, "%4.1f"))
    {
        if (pSettings->hpFreq < 0.0f) pSettings->hpFreq = 0.0f;
        if (pSettings->hpFreq > 50.0f) pSettings->hpFreq = 50.0f;
    }
    if (pSettings->flagPause) { ImGui::EndDisabled(); }
    ImGui::Separator();
    if (ImGui::TreeNode("Monitor"))
    {
        ImGui::Text("Ch1 X:%5.2fV, Y:%5.2fV", pSettings->x1s[pSettings->idx], pSettings->y1s[pSettings->idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(pSettings->x1s[pSettings->idx], 2) + pow(pSettings->y1s[pSettings->idx], 2), 0.5),
            u8"ƒÆ",
            atan2(pSettings->y1s[pSettings->idx], pSettings->x1s[pSettings->idx]) / std::numbers::pi * 180
        );
        if (!stateCh2) { ImGui::BeginDisabled(); }
        ImGui::Text("Ch2 X:%5.2fV, Y:%5.2fV", pSettings->x2s[pSettings->idx], pSettings->y2s[pSettings->idx]);
        ImGui::Text(
            "Amp:%4.2fV, %s:%4.0fDeg.",
            pow(pow(pSettings->x2s[pSettings->idx], 2) + pow(pSettings->y2s[pSettings->idx], 2), 0.5),
            u8"ƒÆ",
            atan2(pSettings->y2s[pSettings->idx], pSettings->x2s[pSettings->idx]) / std::numbers::pi * 180
        );
        if (!stateCh2) { ImGui::EndDisabled(); }
        ImGui::TreePop();
    }
    int hours = (int)pSettings->times[pSettings->idx] / (60 * 60);
    int mins = ((int)pSettings->times[pSettings->idx] - hours * 60 * 60) / 60;
    double secs = pSettings->times[pSettings->idx] - hours * 60 * 60 - mins * 60;
    ImGui::Separator(); 
    ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);
    if (ImGui::Button("Close")) { pSettings->statusMeasurement = false; }
#ifdef DAQ
    if (fgFlag)
    {
        pSettings->pDaq->awg.start(
            pSettings->fgFreq,
            pSettings->fg1Amp, 0.0,
            pSettings->fg2Amp, pSettings->fg2Phase
        );
    }
#endif // DAQ
    ImGui::End();
}
