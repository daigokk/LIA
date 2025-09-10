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
    static ImVec2 windowPos = ImVec2(0, 600 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(550 * pSettings->monitorScale, 300 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    ImGui::SliderFloat("Y limit", &(pSettings->rawLimit), 0.1f, 3.0f, "%4.1f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, pSettings->rawTime.data()[0], pSettings->rawTime.data()[pSettings->rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->rawLimit, pSettings->rawLimit, ImGuiCond_Always);
        ImPlot::PlotLine("##Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size());
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
    static ImVec2 windowPos = ImVec2(550 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(550 * pSettings->monitorScale, 300 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * pSettings->times.size();
    static float historySec = 10;
    ImGui::SliderFloat("History", &historySec, 1, historySecMax, "%5.1f s");
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        ImPlot::PlotLine(
            "x", &(pSettings->times[0]), &(pSettings->xs[0]),
            MEASUREMENT_SIZE, 0, pSettings->offset, sizeof(double)
        );
        ImPlot::PlotLine(
            "y", &(pSettings->times[0]), &(pSettings->ys[0]),
            MEASUREMENT_SIZE, 0, pSettings->offset, sizeof(double)
        );
        ImPlot::EndPlot();
    }
    ImGui::End();
}


class XYPlotWindow : public ImuGuiWindowBase
{
public:
    XYPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "XY")
    {
        this->pSettings = pSettings;
        for (size_t i = 0; i < XY_SIZE; i++)
        {
            _xs[i] = 0; _ys[i] = 0;
        }
    }
    void show(void);
private:
    Settings* pSettings;
    std::array<double, XY_SIZE> _xs, _ys;
};

inline void XYPlotWindow::show()
{
    static ImVec2 windowPos = ImVec2(450 * pSettings->monitorScale, 0);
    static ImVec2 windowSize = ImVec2(600 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.1f V");
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        int tail = (int)(pSettings->idx + 1), head = (int)(tail - XY_SIZE), _size = (int)XY_SIZE;
        if (pSettings->nofm < XY_SIZE)
        {
            head = 0; tail = pSettings->nofm; _size = pSettings->nofm;
            for (int i = 0; i < _size; i++)
            {
                _xs[i] = pSettings->xs[head + i];
                _ys[i] = pSettings->ys[head + i];
            }
        }
        else if (0 <= head && tail <= MEASUREMENT_SIZE)
        {
            for (int i = 0; i < _size; i++)
            {
                _xs[i] = pSettings->xs[head + i];
                _ys[i] = pSettings->ys[head + i];
            }
        }
        else
        {
            int offsetIdx = -head;
            head += MEASUREMENT_SIZE;
            for (int i = 0; i < offsetIdx; i++)
            {
                _xs[i] = pSettings->xs[head + i];
                _ys[i] = pSettings->ys[head + i];
            }
            for (int i = 0; i < tail; i++)
            {
                _xs[offsetIdx + i] = pSettings->xs[i];
                _ys[offsetIdx + i] = pSettings->ys[i];
            }
        }
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);

        ImPlot::PlotLine("##XY", _xs.data(), _ys.data(), _size);
        ImPlot::PlotScatter("##NOW", &(pSettings->xs[pSettings->idx]), &(pSettings->ys[pSettings->idx]), 1);
        ImPlot::EndPlot();
    }
    ImGui::End();
}
