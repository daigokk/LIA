#pragma once

#include "Settings.h"
#include "ImuGuiWindowBase.h"
#include <iostream>
#include <cmath>
#define PI acos(-1)

class ControlWindow : public ImuGuiWindowBase
{
public:
    ControlWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "Control panel")
    {
		this->pSettings = pSettings;
		this->_freqkHz = pSettings->freq * 1e-3;
    }
    void show(void);
private:
    double _freqkHz = 0;
	Settings* pSettings = nullptr;
};

inline void ControlWindow::show(void)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(450, 625), ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    ImGui::SetNextItemWidth(170.0f);
    ImGui::Text("%s", pSettings->sn.data());

    ImGui::SetNextItemWidth(170.0f);
    if (ImGui::InputDouble("Freq. (kHz)", &(this->_freqkHz), 1.0, 1.0, "%3.0f"))
    {
        if (this->_freqkHz < 10.0) this->_freqkHz = 10.0;
        if (this->_freqkHz > 100.0) this->_freqkHz = 100.0;
        pSettings->freq = this->_freqkHz * 1e3;
#ifdef DAQ
        pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
    }
    ImGui::SetNextItemWidth(170.0f);
    if (ImGui::InputDouble("Volt. (V)", &(pSettings->amp1), 0.1, 0.1, "%4.1f"))
    {
        if (pSettings->amp1 < 0.1) pSettings->amp1 = 0.1;
        if (pSettings->amp1 > 5.0) pSettings->amp1 = 5.0;
#ifdef DAQ
        pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
    }
    ImGui::Separator();
    ImGui::SetNextItemWidth(170.0f);
    if (ImGui::InputDouble("Phase (Deg.)", &(pSettings->offsetPhase), 1.0, 10.0, "%3.0f"))
    {

    }
    ImGui::SetNextItemWidth(170.0f);
    if (ImGui::InputFloat("Limit (V)", &(pSettings->limit), 0.1, 0.1, "%4.2f"))
    {
        if (pSettings->limit < 0.1) pSettings->limit = 0.1;
        if (pSettings->limit > 3.0) pSettings->limit = 3.0;
    }
    ImGui::Separator();
    if (ImGui::Button("Auto offset", ImVec2(300.0f, 100.0f))) {
        pSettings->flagAutoOffset = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Off", ImVec2(100.0f, 100.0f))) {
        pSettings->offsetX = 0.0f; pSettings->offsetY = 0.0f;
    }
    ImGui::Separator();
    if (ImGui::TreeNode("Fg secondly"))
    {
        ImGui::SetNextItemWidth(170.0f);
        if (ImGui::InputDouble("Volt. (V)", &(pSettings->amp2), 0.1, 0.1, "%4.2f"))
        {
            if (pSettings->amp2 < 0.0) pSettings->amp2 = 0.0;
            if (pSettings->amp2 > 5.0) pSettings->amp2 = 5.0;
#ifdef DAQ
            pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
        }
        ImGui::SetNextItemWidth(170.0);
        if (ImGui::InputDouble("Phase (Deg.)", &(pSettings->phase2), 1, 1, "%3.0f"))
        {
#ifdef DAQ
            pSettings->pDaq->fg(pSettings->amp1, pSettings->freq, 0.0, pSettings->amp2, pSettings->phase2);
#endif // DAQ
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::Text("X: %5.2f V, Y: %5.2f V", pSettings->xs[pSettings->idx], pSettings->ys[pSettings->idx]);
    ImGui::Text(
        "Amp:%4.2fV,Phase:%3.0fDeg.",
        pow(pow(pSettings->xs[pSettings->idx], 2) + pow(pSettings->ys[pSettings->idx], 2), 0.5),
        atan2(pSettings->ys[pSettings->idx], pSettings->xs[pSettings->idx]) / PI * 180
    );
    ImGui::Separator();
    ImGui::Text("Offset");
    ImGui::Text("X: %5.2f V, Y: %5.2f V", pSettings->offsetX, pSettings->offsetY);
    int hours = (int)pSettings->times[pSettings->idx] / (60 * 60);
    int mins = ((int)pSettings->times[pSettings->idx] - hours * 60 * 60) / 60;
    double secs = pSettings->times[pSettings->idx] - hours * 60 * 60 - mins * 60;
    ImGui::Separator(); 
    ImGui::Text("FPS:%4.0f,Time:%02d:%02d:%02.0f", ImGui::GetIO().Framerate, hours, mins, secs);

    ImGui::End();
}
