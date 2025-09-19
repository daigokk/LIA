#pragma once

#include "settings.h"
#include "ImuGuiWindowBase.h"
#include <IMGUI/imgui_internal.h>

class RawPlotWindow : public ImuGuiWindowBase
{
public:
    RawPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "Raw waveform")
    {
        this->pSettings = pSettings;
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void RawPlotWindow::show()
{
    static ImVec2 windowPos = ImVec2(450 * pSettings->monitorScale, 0 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(525 * pSettings->monitorScale, 575 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    ImGui::SliderFloat("Y limit", &(pSettings->rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, pSettings->rawTime.data()[0], pSettings->rawTime.data()[pSettings->rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->rawLimit, pSettings->rawLimit, ImGuiCond_Always);
#ifndef ENABLE_ADCH2
        ImPlot::PlotLine("##Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size());
#else
        ImPlot::PlotLine("Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size()); 
        ImPlot::PlotLine("Ch2", pSettings->rawTime.data(), pSettings->rawData2.data(), (int)pSettings->rawTime.size());
#endif // ENABLE_ADCH2
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class TimeChartWindow : public ImuGuiWindowBase
{
public:
    TimeChartWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "Time chart")
    {
        this->pSettings = pSettings;
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void TimeChartWindow::show()
{
    static ImVec2 windowPos = ImVec2(0 * pSettings->monitorScale, 575 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(1500 * pSettings->monitorScale, 425 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * pSettings->times.size();
    ImGui::SliderFloat("History", &pSettings->historySec, 1, historySecMax, "%5.1f s");
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - pSettings->historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        ImPlot::PlotLine(
            "x1", &(pSettings->times[0]), &(pSettings->x1s[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
        ImPlot::PlotLine(
            "y1", &(pSettings->times[0]), &(pSettings->y1s[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
#ifdef ENABLE_ADCH2
        ImPlot::PlotLine(
            "x2", &(pSettings->times[0]), &(pSettings->x2s[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
        ImPlot::PlotLine(
            "y2", &(pSettings->times[0]), &(pSettings->y2s[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
#endif // ENABLE_ADCH2
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class DeltaTimeChartWindow : public ImuGuiWindowBase
{
public:
    DeltaTimeChartWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "DeltTime chart")
    {
        this->pSettings = pSettings;
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void DeltaTimeChartWindow::show()
{
    static ImVec2 windowPos = ImVec2(975 * pSettings->monitorScale, 0 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(525 * pSettings->monitorScale, 550 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
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
            "x1", &(pSettings->times[0]), &(pSettings->dts[0]),
            pSettings->size, 0, pSettings->tail, sizeof(double)
        );
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class XYPlotWindow : public ImuGuiWindowBase
{
private:
    Settings* pSettings;
public:
    XYPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "XY")
    {
        this->pSettings = pSettings;
    }
    void show(void);
};

inline void XYPlotWindow::show()
{
    static ImVec2 windowPos = ImVec2(975 * pSettings->monitorScale, 0 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(525 * pSettings->monitorScale, 575 * pSettings->monitorScale);
    if (pSettings->flagSurfaceMode)
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name, nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);
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
    if (pSettings->offset1X == 0 && pSettings->offset1Y == 0) ImGui::BeginDisabled();
    if (ImGui::Button("Off")) {
        pSettings->offset1X = 0.0f; pSettings->offset1Y = 0.0f;
    }
    if (pSettings->offset1X == 0 && pSettings->offset1Y == 0) ImGui::EndDisabled();
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        if (pSettings->flagSurfaceMode)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit*4, pSettings->limit*4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit/4, pSettings->limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        }

#ifndef ENABLE_ADCH2
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
            ImPlot::PlotLine("##XY1", &(pSettings->xy1Xs[0]), &(pSettings->xy1Ys[0]),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();
#else
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
            ImPlot::PlotLine("XY1", pSettings->xy1Xs.data(), pSettings->xy1Ys.data(),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();

            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("XY2", pSettings->xy2Xs.data(), pSettings->xy2Ys.data(),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();
#endif // ENABLE_ADCH2
#ifndef ENABLE_ADCH2 
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
        ImPlot::PlotScatter("##NOW1", &(pSettings->xy1Xs[pSettings->xyIdx]), &(pSettings->xy1Ys[pSettings->xyIdx]), 1);
        ImPlot::PopStyleColor();
#else
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        ImPlot::PlotScatter("##NOW1", &(pSettings->xy1Xs[pSettings->xyIdx]), &(pSettings->xy1Ys[pSettings->xyIdx]), 1);
        ImPlot::PopStyleColor();
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
        ImPlot::PlotScatter("##NOW2", &(pSettings->xy2Xs[pSettings->xyIdx]), &(pSettings->xy2Ys[pSettings->xyIdx]), 1);
        ImPlot::PopStyleColor();
#endif // ENABLE_ADCH2
        ImPlot::EndPlot();
    }
    ImGui::End();
    if (pSettings->flagSurfaceMode)
        ImGui::PopStyleColor();
}
