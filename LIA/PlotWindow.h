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
#ifndef W2
        ImPlot::PlotLine("##Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size());
#else
        ImPlot::PlotLine("Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size()); 
        ImPlot::PlotLine("Ch2", pSettings->rawTime.data(), pSettings->rawData2.data(), (int)pSettings->rawTime.size());
#endif // W2
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
            "x1", &(pSettings->times[0]), &(pSettings->x1s[0]),
            MEASUREMENT_SIZE, 0, pSettings->offset, sizeof(double)
        );
        ImPlot::PlotLine(
            "y1", &(pSettings->times[0]), &(pSettings->y1s[0]),
            MEASUREMENT_SIZE, 0, pSettings->offset, sizeof(double)
        );
#ifdef W2
        ImPlot::PlotLine(
            "x2", &(pSettings->times[0]), &(pSettings->x2s[0]),
            MEASUREMENT_SIZE, 0, pSettings->offset, sizeof(double)
        );
        ImPlot::PlotLine(
            "y2", &(pSettings->times[0]), &(pSettings->y2s[0]),
            MEASUREMENT_SIZE, 0, pSettings->offset, sizeof(double)
        );
#endif // W2
        ImPlot::EndPlot();
    }
    ImGui::End();
}


class XYPlotWindow : public ImuGuiWindowBase
{
private:
    Settings* pSettings;
    std::array<double, XY_SIZE> _x1s, _y1s, _x2s, _y2s;
public:
    XYPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImuGuiWindowBase(window, "XY")
    {
        this->pSettings = pSettings;
#pragma omp parallel for
        for (int i = 0; i < XY_SIZE; i++)
        {
            _x1s[i] = 0; _y1s[i] = 0;
            _x2s[i] = 0; _y2s[i] = 0;
        }
    }
    void show(void);
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
#pragma omp parallel for
            for (int i = 0; i < _size; i++)
            {
                _x1s[i] = pSettings->x1s[head + i];
                _y1s[i] = pSettings->y1s[head + i];
#ifdef W2
                _x2s[i] = pSettings->x2s[head + i];
                _y2s[i] = pSettings->y2s[head + i];
#endif // W2
            }
        }
        else if (0 <= head && tail <= MEASUREMENT_SIZE)
        {
#pragma omp parallel for
            for (int i = 0; i < _size; i++)
            {
                _x1s[i] = pSettings->x1s[head + i];
                _y1s[i] = pSettings->y1s[head + i];
#ifdef W2
                _x2s[i] = pSettings->x2s[head + i];
                _y2s[i] = pSettings->y2s[head + i];
#endif // W2
            }
        }
        else
        {
            int offsetIdx = -head;
            head += MEASUREMENT_SIZE;
#pragma omp parallel for
            for (int i = 0; i < offsetIdx; i++)
            {
                _x1s[i] = pSettings->x1s[head + i];
                _y1s[i] = pSettings->y1s[head + i];
#ifdef W2
                _x2s[i] = pSettings->x2s[head + i];
                _y2s[i] = pSettings->y2s[head + i];
#endif // W2
            }
#pragma omp parallel for
            for (int i = 0; i < tail; i++)
            {
                _x1s[offsetIdx + i] = pSettings->x1s[i];
                _y1s[offsetIdx + i] = pSettings->y1s[i];
#ifdef W2
                _x2s[offsetIdx + i] = pSettings->x2s[i];
                _y2s[offsetIdx + i] = pSettings->y2s[i];
#endif // W2
            }
        }
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
#ifndef W2
        ImPlot::PlotLine("##XY", _x1s.data(), _y1s.data(), _size);
        ImPlot::PlotScatter("##NOW", &(pSettings->x1s[pSettings->idx]), &(pSettings->y1s[pSettings->idx]), 1);
#else
        ImPlot::PlotLine("XY1", _x1s.data(), _y1s.data(), _size);
        ImPlot::PlotLine("XY2", _x2s.data(), _y2s.data(), _size);
        ImPlot::PlotScatter("##NOW1", &(pSettings->x1s[pSettings->idx]), &(pSettings->y1s[pSettings->idx]), 1);
        ImPlot::PlotScatter("##NOW2", &(pSettings->x2s[pSettings->idx]), &(pSettings->y2s[pSettings->idx]), 1);
#endif // W2
        
        ImPlot::EndPlot();
    }
    ImGui::End();
}
