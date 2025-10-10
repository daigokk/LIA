#pragma once

#include "settings.h"
#include "ImGuiWindowBase.h"

constexpr auto MILI_VOLT = 0.2f;

const ImVec4 colors[] = {
        ImVec4(1,0,0,1),   // Red
        ImVec4(0,1,0,1),   // Green
        ImVec4(0,0,1,1),   // Blue
        ImVec4(1,1,0,1),   // Yellow
        ImVec4(1,0,1,1),    // Magenta
        ImVec4(1,0.645f,0,1),    // Orange
        ImVec4(0xea / 256.0, 0xa2 / 256.0, 0x22 / 256.0, 1),    // Marigold
        ImVec4(0xfb / 256.0, 0xbd / 256.0, 0x04 / 256.0, 1),    // Amber
        ImVec4(0xb0 / 256.0, 0xfc / 256.0, 0x38 / 256.0, 1)   // Chartreuse
};

void ScientificFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.1e", value); // 科学的記法で表示
}

void MiliFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e3); // mili: 1e-3
}

class RawPlotWindow : public ImGuiWindowBase
{
public:
    RawPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImGuiWindowBase(window, "Raw waveform")
    {
        this->pSettings = pSettings;
        this->windowPos = ImVec2(450 * pSettings->window.monitorScale, 0 * pSettings->window.monitorScale);
        this->windowSize = ImVec2(430 * pSettings->window.monitorScale, 600 * pSettings->window.monitorScale);
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void RawPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, pSettings->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pSettings->imgui.windowFlag);
    ImGui::Begin(this->name);
    
    if (ImGui::Button("Save"))
    {
        pSettings->saveRawData(std::format("raw_{}.csv", pSettings->getCurrentTimestamp()));
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawSave;
        value = 0;
    }
    //ImGui::SameLine();
    ImGui::SliderFloat("Y limit", &(pSettings->plot.rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawLimit;
        value = pSettings->plot.rawLimit;
    }
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        if (pSettings->plot.rawLimit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (us)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, pSettings->rawTime.data()[0], pSettings->rawTime.data()[pSettings->rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->plot.rawLimit, pSettings->plot.rawLimit, ImGuiCond_Always);
        
        const char* ch1_label = pSettings->flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        ImPlot::PlotLine(ch1_label, pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size());
        ImPlot::PopStyleColor();
        if (pSettings->flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", pSettings->rawTime.data(), pSettings->rawData2.data(), (int)pSettings->rawTime.size());
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
    if (button != ButtonType::NON)
    {
        pSettings->cmds.push_back(std::array<float, 3>{ (float)pSettings->timer.elapsedSec(), (float)button, value });
    }
}

class TimeChartWindow : public ImGuiWindowBase
{
public:
    TimeChartWindow(GLFWwindow* window, Settings* pSettings, ImPlotRect* pTimeChartZoomRect)
        : ImGuiWindowBase(window, "Time chart")
    {
        this->pSettings = pSettings;
        this->pTimeChartZoomRect = pTimeChartZoomRect;
        this->windowPos = ImVec2(450 * pSettings->window.monitorScale, 600 * pSettings->window.monitorScale);
        this->windowSize = ImVec2(990 * pSettings->window.monitorScale, 360 * pSettings->window.monitorScale);
    }
    void show(void);
private:
    Settings* pSettings;
    ImPlotRect* pTimeChartZoomRect;
};

inline void TimeChartWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, pSettings->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pSettings->imgui.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * pSettings->times.size();
    if (pSettings->flagPause) { ImGui::BeginDisabled(); }
    ImGui::SetNextItemWidth(500.0f * pSettings->window.monitorScale);
    ImGui::SliderFloat("History", &pSettings->plot.historySec, 1, historySecMax, "%5.1f s");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimeHistory;
        value = pSettings->plot.historySec;
    }
    if (pSettings->flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        pSettings->saveResultsToFile(
            std::format("ect_{}.csv", pSettings->getCurrentTimestamp()),
            pSettings->plot.historySec
        );
    }
    ImGui::SameLine();
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run")) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pSettings->flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimePause;
        value = pSettings->flagPause;
    }
    
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (pSettings->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time", "v (mV)", ImPlotAxisFlags_NoTickLabels, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (!pSettings->flagPause)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - pSettings->plot.historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->plot.limit, pSettings->plot.limit, ImGuiCond_Always);
        ImPlot::PlotLine(
            "Ch1y", &(pSettings->times[0]), &(pSettings->y1s[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
        if (pSettings->flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(pSettings->times[0]), &(pSettings->y2s[0]),
                pSettings->size, 0, pSettings->tail, sizeof(double)
            );
        }
        static bool flag = true;
        if (pSettings->flagPause)
        {
            if (flag)
            {
                *pTimeChartZoomRect = ImPlotRect(
                    pSettings->times[pSettings->idx] - pSettings->plot.historySec / 2 - pSettings->plot.historySec / 20,
                    pSettings->times[pSettings->idx] - pSettings->plot.historySec / 2 + pSettings->plot.historySec / 20, -pSettings->plot.limit / 2, pSettings->plot.limit / 2);
                flag = false;
            }
            static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
            bool clicked = false;
            bool hovered = false;
            bool held = false;
            ImPlot::DragRect(
                0, &pTimeChartZoomRect->X.Min, &pTimeChartZoomRect->Y.Min, 
                &pTimeChartZoomRect->X.Max, &pTimeChartZoomRect->Y.Max, 
                ImVec4(1, 0, 1, 1), flags, &clicked, &hovered, &held
            );
        }
        else {
            flag=true;
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (button != ButtonType::NON)
    {
        pSettings->cmds.push_back(std::array<float, 3>{ (float)pSettings->timer.elapsedSec(), (float)button, value });
    }
}

class TimeChartZoomWindow : public ImGuiWindowBase
{
public:
    TimeChartZoomWindow(GLFWwindow* window, Settings* pSettings, ImPlotRect* pTimeChartZoomRect)
        : ImGuiWindowBase(window, "Time chart zoom")
    {
        this->pSettings = pSettings;
        this->pTimeChartZoomRect = pTimeChartZoomRect;
        this->windowPos = ImVec2(0 * pSettings->window.monitorScale, 0 * pSettings->window.monitorScale);
        this->windowSize = ImVec2(750 * pSettings->window.monitorScale, 600 * pSettings->window.monitorScale);
    }
    void show(void);
private:
    Settings* pSettings;
    ImPlotRect* pTimeChartZoomRect;
};

inline void TimeChartZoomWindow::show()
{
    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time (s)", "v (V)", 0, 0);
        if (pSettings->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (s)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, pTimeChartZoomRect->X.Min, pTimeChartZoomRect->X.Max, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, pTimeChartZoomRect->Y.Min, pTimeChartZoomRect->Y.Max, ImGuiCond_Always);
        ImPlot::PlotLine(
            "Ch1y", &(pSettings->times[0]), &(pSettings->y1s[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
        if (pSettings->flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(pSettings->times[0]), &(pSettings->y2s[0]),
                pSettings->size, 0, pSettings->tail, sizeof(double)
            );
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
}

class DeltaTimeChartWindow : public ImGuiWindowBase
{
public:
    DeltaTimeChartWindow(GLFWwindow* window, Settings* pSettings)
        : ImGuiWindowBase(window, "DeltTime chart")
    {
        this->pSettings = pSettings;
        this->windowPos = ImVec2(0 * pSettings->window.monitorScale, 750 * pSettings->window.monitorScale);
        this->windowSize = ImVec2(450 * pSettings->window.monitorScale, 210 * pSettings->window.monitorScale);
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void DeltaTimeChartWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, pSettings->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pSettings->imgui.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT)*pSettings->times.size();
    static float historySec = 10;
    ImGui::SliderFloat("History", &historySec, 1, historySecMax, "%5.1f s");
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time", "dt (ms)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, (MEASUREMENT_DT - 2e-3) * 1e3, (MEASUREMENT_DT + 2e-3) * 1e3, ImGuiCond_Always);
        ImPlot::PlotLine(
            "##dt", &(pSettings->times[0]), &(pSettings->dts[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class XYPlotWindow : public ImGuiWindowBase
{
private:
    Settings* pSettings;
public:
    XYPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImGuiWindowBase(window, "XY")
    {
        this->pSettings = pSettings;
        this->windowPos = ImVec2(880 * pSettings->window.monitorScale, 0 * pSettings->window.monitorScale);
        this->windowSize = ImVec2(560 * pSettings->window.monitorScale, 600 * pSettings->window.monitorScale);
    }
    void show(void);
};

inline void XYPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    if (pSettings->plot.surfaceMode)
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    ImGui::SetNextWindowPos(windowPos, pSettings->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pSettings->imgui.windowFlag); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.1f V");
    if (ImGui::Button("Clear"))
    {
        pSettings->xyTail = 0; pSettings->xyNorm = 0;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYClear;
        value = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) {
        pSettings->flagAutoOffset = true;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYAutoOffset;
        value = 0;
    }
    ImGui::SameLine();
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run")) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pSettings->flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYPause;
        value = pSettings->flagPause;
    }
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        if (pSettings->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("x (mV)", "y (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (pSettings->plot.surfaceMode)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->plot.limit * 4, pSettings->plot.limit * 4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->plot.limit / 4, pSettings->plot.limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->plot.limit, pSettings->plot.limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->plot.limit, pSettings->plot.limit, ImGuiCond_Always);
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        const char* ch1_label = pSettings->flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PlotLine(ch1_label, &(pSettings->xy1Xs[0]), &(pSettings->xy1Ys[0]),
            pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pSettings->window.monitorScale, colors[2], -1.0f, colors[2]);
        ImPlot::PlotScatter("##NOW1", &(pSettings->xy1Xs[pSettings->xyIdx]), &(pSettings->xy1Ys[pSettings->xyIdx]), 1);
        if (pSettings->flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", pSettings->xy2Xs.data(), pSettings->xy2Ys.data(),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pSettings->window.monitorScale, colors[7], -1.0f, colors[7]);
            ImPlot::PlotScatter("##NOW2", &(pSettings->xy2Xs[pSettings->xyIdx]), &(pSettings->xy2Ys[pSettings->xyIdx]), 1);
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (pSettings->plot.surfaceMode)
        ImGui::PopStyleColor();
    if (button != ButtonType::NON)
    {
        pSettings->cmds.push_back(std::array<float, 3>{ (float)pSettings->timer.elapsedSec(), (float)button, value });
    }
}


class ACFMPlotWindow : public ImGuiWindowBase
{
private:
    Settings* pSettings;
public:
    ACFMPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImGuiWindowBase(window, "ACFM")
    {
        this->pSettings = pSettings;
        this->windowPos = ImVec2(730 * pSettings->window.monitorScale, 300 * pSettings->window.monitorScale);
        this->windowSize = ImVec2(560 * pSettings->window.monitorScale, 600 * pSettings->window.monitorScale);
    }
    void show(void);
};

inline void ACFMPlotWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.1f V");
    if (ImGui::Button("Clear"))
    {
        pSettings->xyTail = 0; pSettings->xyNorm = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) {
        pSettings->flagAutoOffset = true;
    }
    ImGui::SameLine();
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run")) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pSettings->flagPause = true; }
    }
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vbz (V)", "Vbx (V)", 0, 0);
        if (pSettings->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vbz (mV)", "Vbx (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->plot.limit, pSettings->plot.limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->plot.limit, pSettings->plot.limit, ImGuiCond_Always);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
        ImPlot::PlotLine("##ACFM", &(pSettings->xy1Ys[0]), &(pSettings->xy2Ys[0]),
            pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pSettings->window.monitorScale, colors[8], -1.0f, colors[8]);
        ImPlot::PlotScatter("##NOW", &(pSettings->xy1Ys[pSettings->xyIdx]), &(pSettings->xy2Ys[pSettings->xyIdx]), 1);
        ImPlot::EndPlot();
    }
    ImGui::End();
    
}
