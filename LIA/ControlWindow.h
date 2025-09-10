#pragma once

#include "Settings.h"
#include "ImuGuiWindowBase.h"
#include <iostream>
#include <cmath>

class ControlWindow : public ImuGuiWindowBase
{
private:
    float _freqkHz = 0;
    Settings* pSettings = nullptr;
public:
    ControlWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "Control panel")
    {
		this->pSettings = pSettings;
		this->_freqkHz = pSettings->freq * 1e-3f;
    }
    void show(void);
};

inline void ControlWindow::show(void)
{
    static ImVec2 windowSize = ImVec2(450 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    static float nextItemWidth = 170.0f * pSettings->monitorScale;
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", pSettings->sn.data());

    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("Freq. (kHz)", &(this->_freqkHz), 1.0f, 1.0f, "%3.0f"))
    {
        if (this->_freqkHz < 10.0f) this->_freqkHz = 10.0f;
        if (this->_freqkHz > 100.0f) this->_freqkHz = 100.0f;
        pSettings->freq = this->_freqkHz * 1e3f;
#ifdef DAQ
        pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
    }
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("Volt. (V)", &(pSettings->amp1), 0.1f, 0.1f, "%4.1f"))
    {
        if (pSettings->amp1 < 0.1f) pSettings->amp1 = 0.1f;
        if (pSettings->amp1 > 5.0f) pSettings->amp1 = 5.0f;
#ifdef DAQ
        pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
    }
    ImGui::Separator();
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputDouble("Phase (Deg.)", &(pSettings->offsetPhase), 1.0, 10.0, "%3.0f"))
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
        pSettings->offsetX = 0.0f; pSettings->offsetY = 0.0f;
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Fg secondly"))
    {
        ImGui::SetNextItemWidth(nextItemWidth);
        if (ImGui::InputFloat("Volt. (V)", &(pSettings->amp2), 0.1f, 0.1f, "%4.2f"))
        {
            if (pSettings->amp2 < 0.0f) pSettings->amp2 = 0.0f;
            if (pSettings->amp2 > 5.0f) pSettings->amp2 = 5.0f;
#ifdef DAQ
            pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
        }
        ImGui::SetNextItemWidth(nextItemWidth);
        if (ImGui::InputFloat("Phase (Deg.)", &(pSettings->phase2), 1, 1, "%3.0f"))
        {
#ifdef DAQ
            pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::Text("X: %5.2fV, Y: %5.2fV", pSettings->xs[pSettings->idx], pSettings->ys[pSettings->idx]);
    ImGui::Text(
        "Amp:%4.2fV,Phase:%3.0fDeg.",
        pow(pow(pSettings->xs[pSettings->idx], 2) + pow(pSettings->ys[pSettings->idx], 2), 0.5),
        atan2(pSettings->ys[pSettings->idx], pSettings->xs[pSettings->idx]) / PI * 180
    );
    ImGui::Separator();
    ImGui::Text("Offset");
    ImGui::Text("X: %5.2fV, Y: %5.2fV", pSettings->offsetX, pSettings->offsetY);
    int hours = (int)pSettings->times[pSettings->idx] / (60 * 60);
    int mins = ((int)pSettings->times[pSettings->idx] - hours * 60 * 60) / 60;
    double secs = pSettings->times[pSettings->idx] - hours * 60 * 60 - mins * 60;
    ImGui::Separator(); 
    ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);

    ImGui::End();
}
