#pragma once

#include "Settings.h"
#include "ImuGuiWindowBase.h"
#include <iostream>
#include <cmath>

class ControlWindow : public ImuGuiWindowBase
{
private:
    Settings* pSettings = nullptr;
public:
    ControlWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "Control panel")
    {
		this->pSettings = pSettings;
    }
    void show(void);
};

inline void ControlWindow::show(void)
{
    bool fgFlag = false;
    static ImVec2 windowSize = ImVec2(450 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    static float nextItemWidth = 170.0f * pSettings->monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", pSettings->sn.data());

    ImGui::SetNextItemWidth(nextItemWidth);
    float freqkHz = pSettings->fgFreq * 1e-3;
    if (ImGui::InputFloat("Freq. (kHz)", &(freqkHz), 1.0f, 1.0f, "%3.0f"))
    {
        if (freqkHz < 10.0f) freqkHz = 10.0f;
        if (freqkHz > 100.0f) freqkHz = 100.0f;
        pSettings->fgFreq = freqkHz * 1e3f;
        fgFlag = true;
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("Volt. (V)", &(pSettings->fgCh1Amp), 0.1f, 0.1f, "%4.1f"))
    {
        if (pSettings->fgCh1Amp < 0.1f) pSettings->fgCh1Amp = 0.1f;
        if (pSettings->fgCh1Amp > 5.0f) pSettings->fgCh1Amp = 5.0f;
        fgFlag = true;
    }
    ImGui::Separator();
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputDouble("Phase (Deg.)", &(pSettings->offsetW1Phase), 1.0, 10.0, "%3.0f"))
    {

    }
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("Limit (V)", &(pSettings->limit), 0.1f, 0.1f, "%4.2f"))
    {
        if (pSettings->limit < 0.1f) pSettings->limit = 0.1f;
        if (pSettings->limit > 3.0f) pSettings->limit = 3.0f;
    }
    ImGui::Separator();
    static ImVec2 autoOffsetSize = ImVec2(300.0f * pSettings->monitorScale, 100.0f * pSettings->monitorScale);
    if (ImGui::Button("Auto offset", autoOffsetSize)) {
        pSettings->flagAutoOffset = true;
    }
    ImGui::SameLine();
    static ImVec2 offSize = ImVec2(100.0f * pSettings->monitorScale, autoOffsetSize.y);
    if (ImGui::Button("Off", offSize)) {
        pSettings->offsetW1X = 0.0f; pSettings->offsetW1Y = 0.0f;
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Fg secondly"))
    {
        ImGui::SetNextItemWidth(nextItemWidth);
        if (ImGui::InputFloat("Volt. (V)", &(pSettings->fgCh2Amp), 0.1f, 0.1f, "%4.2f"))
        {
            if (pSettings->fgCh2Amp < 0.0f) pSettings->fgCh2Amp = 0.0f;
            if (pSettings->fgCh2Amp > 5.0f) pSettings->fgCh2Amp = 5.0f;
            fgFlag = true;
        }
        ImGui::SetNextItemWidth(nextItemWidth);
        if (ImGui::InputFloat("Phase (Deg.)", &(pSettings->fgCh2Phase), 1, 1, "%3.0f"))
        {
            fgFlag = true;
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::Text("X: %5.2fV, Y: %5.2fV", pSettings->w1xs[pSettings->idx], pSettings->w1ys[pSettings->idx]);
    ImGui::Text(
        "Amp:%4.2fV,Phase:%3.0fDeg.",
        pow(pow(pSettings->w1xs[pSettings->idx], 2) + pow(pSettings->w1ys[pSettings->idx], 2), 0.5),
        atan2(pSettings->w1ys[pSettings->idx], pSettings->w1xs[pSettings->idx]) / PI * 180
    );
    ImGui::Separator();
    ImGui::Text("Offset");
    ImGui::Text("X: %5.2fV, Y: %5.2fV", pSettings->offsetW1X, pSettings->offsetW1Y);
    int hours = (int)pSettings->times[pSettings->idx] / (60 * 60);
    int mins = ((int)pSettings->times[pSettings->idx] - hours * 60 * 60) / 60;
    double secs = pSettings->times[pSettings->idx] - hours * 60 * 60 - mins * 60;
    ImGui::Separator(); 
    ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);

    ImGui::End();
#ifdef DAQ
    if(fgFlag)
        pSettings->pDaq->fg(pSettings->fgCh1Amp, pSettings->fgFreq, 0.0, pSettings->fgCh2Amp, pSettings->fgCh2Phase);
#endif // DAQ
}
