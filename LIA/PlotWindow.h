#pragma once

#include "LiaConfig.h"
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
    RawPlotWindow(GLFWwindow* window, LiaConfig* pLiaConfig)
        : ImGuiWindowBase(window, "Raw waveform")
    {
        this->pLiaConfig = pLiaConfig;
        this->windowPos = ImVec2(450 * pLiaConfig->window.monitorScale, 0 * pLiaConfig->window.monitorScale);
        this->windowSize = ImVec2(430 * pLiaConfig->window.monitorScale, 600 * pLiaConfig->window.monitorScale);
    }
    void show(void);
private:
    LiaConfig* pLiaConfig;
};

inline void RawPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, pLiaConfig->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pLiaConfig->imgui.windowFlag);
    ImGui::Begin(this->name);
    
    if (ImGui::Button("Save"))
    {
        pLiaConfig->saveRawData(std::format("raw_{}.csv", pLiaConfig->getCurrentTimestamp()));
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawSave;
        value = 0;
    }
    //ImGui::SameLine();
    ImGui::SliderFloat("Y limit", &(pLiaConfig->plot.rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawLimit;
        value = pLiaConfig->plot.rawLimit;
    }
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        if (pLiaConfig->plot.rawLimit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (us)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, pLiaConfig->rawTime.data()[0], pLiaConfig->rawTime.data()[pLiaConfig->rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pLiaConfig->plot.rawLimit, pLiaConfig->plot.rawLimit, ImGuiCond_Always);
        
        const char* ch1_label = pLiaConfig->flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        ImPlot::PlotLine(ch1_label, pLiaConfig->rawTime.data(), pLiaConfig->rawData1.data(), (int)pLiaConfig->rawTime.size());
        ImPlot::PopStyleColor();
        if (pLiaConfig->flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", pLiaConfig->rawTime.data(), pLiaConfig->rawData2.data(), (int)pLiaConfig->rawTime.size());
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
    if (button != ButtonType::NON)
    {
        pLiaConfig->cmds.push_back(std::array<float, 3>{ (float)pLiaConfig->timer.elapsedSec(), (float)button, value });
    }
}

class TimeChartWindow : public ImGuiWindowBase
{
public:
    TimeChartWindow(GLFWwindow* window, LiaConfig* pLiaConfig, ImPlotRect* pTimeChartZoomRect)
        : ImGuiWindowBase(window, "Time chart")
    {
        this->pLiaConfig = pLiaConfig;
        this->pTimeChartZoomRect = pTimeChartZoomRect;
        this->windowPos = ImVec2(450 * pLiaConfig->window.monitorScale, 600 * pLiaConfig->window.monitorScale);
        this->windowSize = ImVec2(990 * pLiaConfig->window.monitorScale, 360 * pLiaConfig->window.monitorScale);
    }
    void show(void);
private:
    LiaConfig* pLiaConfig;
    ImPlotRect* pTimeChartZoomRect;
};

inline void TimeChartWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, pLiaConfig->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pLiaConfig->imgui.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * pLiaConfig->times.size();
    if (pLiaConfig->flagPause) { ImGui::BeginDisabled(); }
    ImGui::SetNextItemWidth(500.0f * pLiaConfig->window.monitorScale);
    ImGui::SliderFloat("History", &pLiaConfig->plot.historySec, 1, historySecMax, "%5.1f s");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimeHistory;
        value = pLiaConfig->plot.historySec;
    }
    if (pLiaConfig->flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        pLiaConfig->saveResultsToFile(
            std::format("ect_{}.csv", pLiaConfig->getCurrentTimestamp()),
            pLiaConfig->plot.historySec
        );
    }
    ImGui::SameLine();
    if (pLiaConfig->flagPause)
    {
        if (ImGui::Button("Run")) { pLiaConfig->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pLiaConfig->flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimePause;
        value = pLiaConfig->flagPause;
    }
    
    //ImGui::SliderFloat("Y limit", &(pLiaConfig->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pLiaConfig->times[pLiaConfig->idx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (pLiaConfig->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time", "v (mV)", ImPlotAxisFlags_NoTickLabels, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (!pLiaConfig->flagPause)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - pLiaConfig->plot.historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pLiaConfig->plot.limit, pLiaConfig->plot.limit, ImGuiCond_Always);
        ImPlot::PlotLine(
            "Ch1y", &(pLiaConfig->times[0]), &(pLiaConfig->y1s[0]),
            pLiaConfig->size, 0, pLiaConfig->tail, sizeof(double)
        );
        if (pLiaConfig->flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(pLiaConfig->times[0]), &(pLiaConfig->y2s[0]),
                pLiaConfig->size, 0, pLiaConfig->tail, sizeof(double)
            );
        }
        static bool flag = true;
        if (pLiaConfig->flagPause)
        {
            if (flag)
            {
                *pTimeChartZoomRect = ImPlotRect(
                    pLiaConfig->times[pLiaConfig->idx] - pLiaConfig->plot.historySec / 2 - pLiaConfig->plot.historySec / 20,
                    pLiaConfig->times[pLiaConfig->idx] - pLiaConfig->plot.historySec / 2 + pLiaConfig->plot.historySec / 20, -pLiaConfig->plot.limit / 2, pLiaConfig->plot.limit / 2);
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
        pLiaConfig->cmds.push_back(std::array<float, 3>{ (float)pLiaConfig->timer.elapsedSec(), (float)button, value });
    }
}

class TimeChartZoomWindow : public ImGuiWindowBase
{
public:
    TimeChartZoomWindow(GLFWwindow* window, LiaConfig* pLiaConfig, ImPlotRect* pTimeChartZoomRect)
        : ImGuiWindowBase(window, "Time chart zoom")
    {
        this->pLiaConfig = pLiaConfig;
        this->pTimeChartZoomRect = pTimeChartZoomRect;
        this->windowPos = ImVec2(0 * pLiaConfig->window.monitorScale, 0 * pLiaConfig->window.monitorScale);
        this->windowSize = ImVec2(750 * pLiaConfig->window.monitorScale, 600 * pLiaConfig->window.monitorScale);
    }
    void show(void);
private:
    LiaConfig* pLiaConfig;
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
        double t = pLiaConfig->times[pLiaConfig->idx];
        ImPlot::SetupAxes("Time (s)", "v (V)", 0, 0);
        if (pLiaConfig->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (s)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, pTimeChartZoomRect->X.Min, pTimeChartZoomRect->X.Max, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, pTimeChartZoomRect->Y.Min, pTimeChartZoomRect->Y.Max, ImGuiCond_Always);
        ImPlot::PlotLine(
            "Ch1y", &(pLiaConfig->times[0]), &(pLiaConfig->y1s[0]),
            pLiaConfig->size, 0, pLiaConfig->tail, sizeof(double)
        );
        if (pLiaConfig->flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(pLiaConfig->times[0]), &(pLiaConfig->y2s[0]),
                pLiaConfig->size, 0, pLiaConfig->tail, sizeof(double)
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
    DeltaTimeChartWindow(GLFWwindow* window, LiaConfig* pLiaConfig)
        : ImGuiWindowBase(window, "DeltTime chart")
    {
        this->pLiaConfig = pLiaConfig;
        this->windowPos = ImVec2(0 * pLiaConfig->window.monitorScale, 750 * pLiaConfig->window.monitorScale);
        this->windowSize = ImVec2(450 * pLiaConfig->window.monitorScale, 210 * pLiaConfig->window.monitorScale);
    }
    void show(void);
private:
    LiaConfig* pLiaConfig;
};

inline void DeltaTimeChartWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, pLiaConfig->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pLiaConfig->imgui.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT)*pLiaConfig->times.size();
    static float historySec = 10;
    ImGui::SliderFloat("History", &historySec, 1, historySecMax, "%5.1f s");
    //ImGui::SliderFloat("Y limit", &(pLiaConfig->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pLiaConfig->times[pLiaConfig->idx];
        ImPlot::SetupAxes("Time", "dt (ms)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, (MEASUREMENT_DT - 2e-3) * 1e3, (MEASUREMENT_DT + 2e-3) * 1e3, ImGuiCond_Always);
        ImPlot::PlotLine(
            "##dt", &(pLiaConfig->times[0]), &(pLiaConfig->dts[0]),
            pLiaConfig->size, 0, pLiaConfig->tail, sizeof(double)
        );
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class XYPlotWindow : public ImGuiWindowBase
{
private:
    LiaConfig* pLiaConfig;
public:
    XYPlotWindow(GLFWwindow* window, LiaConfig* pLiaConfig)
        : ImGuiWindowBase(window, "XY")
    {
        this->pLiaConfig = pLiaConfig;
        this->windowPos = ImVec2(880 * pLiaConfig->window.monitorScale, 0 * pLiaConfig->window.monitorScale);
        this->windowSize = ImVec2(560 * pLiaConfig->window.monitorScale, 600 * pLiaConfig->window.monitorScale);
    }
    void show(void);
};

inline void XYPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    if (pLiaConfig->plot.surfaceMode)
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    ImGui::SetNextWindowPos(windowPos, pLiaConfig->imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, pLiaConfig->imgui.windowFlag); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(pLiaConfig->limit), 0.1, 2.0, "%4.1f V");
    if (ImGui::Button("Clear"))
    {
        pLiaConfig->xyTail = 0; pLiaConfig->xyNorm = 0;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYClear;
        value = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) {
        pLiaConfig->flagAutoOffset = true;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYAutoOffset;
        value = 0;
    }
    ImGui::SameLine();
    if (pLiaConfig->flagPause)
    {
        if (ImGui::Button("Run")) { pLiaConfig->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pLiaConfig->flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYPause;
        value = pLiaConfig->flagPause;
    }
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        if (pLiaConfig->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("x (mV)", "y (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (pLiaConfig->plot.surfaceMode)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pLiaConfig->plot.limit * 4, pLiaConfig->plot.limit * 4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pLiaConfig->plot.limit / 4, pLiaConfig->plot.limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pLiaConfig->plot.limit, pLiaConfig->plot.limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pLiaConfig->plot.limit, pLiaConfig->plot.limit, ImGuiCond_Always);
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        const char* ch1_label = pLiaConfig->flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PlotLine(ch1_label, &(pLiaConfig->xy1Xs[0]), &(pLiaConfig->xy1Ys[0]),
            pLiaConfig->xySize, 0, pLiaConfig->xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pLiaConfig->window.monitorScale, colors[2], -1.0f, colors[2]);
        ImPlot::PlotScatter("##NOW1", &(pLiaConfig->xy1Xs[pLiaConfig->xyIdx]), &(pLiaConfig->xy1Ys[pLiaConfig->xyIdx]), 1);
        if (pLiaConfig->flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", pLiaConfig->xy2Xs.data(), pLiaConfig->xy2Ys.data(),
                pLiaConfig->xySize, 0, pLiaConfig->xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pLiaConfig->window.monitorScale, colors[7], -1.0f, colors[7]);
            ImPlot::PlotScatter("##NOW2", &(pLiaConfig->xy2Xs[pLiaConfig->xyIdx]), &(pLiaConfig->xy2Ys[pLiaConfig->xyIdx]), 1);
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (pLiaConfig->plot.surfaceMode)
        ImGui::PopStyleColor();
    if (button != ButtonType::NON)
    {
        pLiaConfig->cmds.push_back(std::array<float, 3>{ (float)pLiaConfig->timer.elapsedSec(), (float)button, value });
    }
}


class ACFMPlotWindow : public ImGuiWindowBase
{
private:
    LiaConfig* pLiaConfig;
public:
    ACFMPlotWindow(GLFWwindow* window, LiaConfig* pLiaConfig)
        : ImGuiWindowBase(window, "ACFM")
    {
        this->pLiaConfig = pLiaConfig;
        this->windowPos = ImVec2(730 * pLiaConfig->window.monitorScale, 300 * pLiaConfig->window.monitorScale);
        this->windowSize = ImVec2(560 * pLiaConfig->window.monitorScale, 600 * pLiaConfig->window.monitorScale);
    }
    void show(void);
};

inline void ACFMPlotWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(pLiaConfig->limit), 0.1, 2.0, "%4.1f V");
    if (ImGui::Button("Clear"))
    {
        pLiaConfig->xyTail = 0; pLiaConfig->xyNorm = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) {
        pLiaConfig->flagAutoOffset = true;
    }
    ImGui::SameLine();
    if (pLiaConfig->flagPause)
    {
        if (ImGui::Button("Run")) { pLiaConfig->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pLiaConfig->flagPause = true; }
    }
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vbz (V)", "Vbx (V)", 0, 0);
        if (pLiaConfig->plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vbz (mV)", "Vbx (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, -pLiaConfig->plot.limit, pLiaConfig->plot.limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pLiaConfig->plot.limit, pLiaConfig->plot.limit, ImGuiCond_Always);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
        ImPlot::PlotLine("##ACFM", &(pLiaConfig->xy1Ys[0]), &(pLiaConfig->xy2Ys[0]),
            pLiaConfig->xySize, 0, pLiaConfig->xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pLiaConfig->window.monitorScale, colors[8], -1.0f, colors[8]);
        ImPlot::PlotScatter("##NOW", &(pLiaConfig->xy1Ys[pLiaConfig->xyIdx]), &(pLiaConfig->xy2Ys[pLiaConfig->xyIdx]), 1);
        ImPlot::EndPlot();
    }
    ImGui::End();
    
}
