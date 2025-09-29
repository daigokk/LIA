#pragma once

#include "settings.h"
#include "ImGuiWindowBase.h"

#define MILI_VOLT 0.2f

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
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void RawPlotWindow::show()
{
    static ImVec2 windowPos = ImVec2(450 * pSettings->monitorScale, 0 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(430 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    
    if (ImGui::Button("Save"))
    {
        const char* filepath = "raw.csv";
        std::ofstream outputFile(filepath);
        if (!outputFile)
        { // ファイルが開けなかった場合
            std::cerr << "Fail: " << filepath << std::endl;
        }
        if (!pSettings->flagCh2)
        {
            outputFile << "# t(s), ch1(V))" << std::endl;
        }
        else {
            outputFile << "# t(s), ch1(V), ch2(V)" << std::endl;
        }
        for (size_t i = 0; i < pSettings->rawData1.size(); i++)
        {
            if (!pSettings->flagCh2)
            {
                outputFile << std::format("{:e},{:e},{:e}\n", pSettings->rawDt*i, pSettings->rawData1[i]);
            }
            else {
                outputFile << std::format("{:e},{:e},{:e}\n", pSettings->rawDt * i, pSettings->rawData1[i], pSettings->rawData2[i]);
            }
        }
        outputFile.close();
    }
    //ImGui::SameLine();
    ImGui::SliderFloat("Y limit", &(pSettings->rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        if (pSettings->rawLimit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (us)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, pSettings->rawTime.data()[0], pSettings->rawTime.data()[pSettings->rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->rawLimit, pSettings->rawLimit, ImGuiCond_Always);
        ImPlot::PlotLine("Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size());
        if (pSettings->flagCh2)
        {
            ImPlot::PlotLine("Ch1", pSettings->rawTime.data(), pSettings->rawData1.data(), (int)pSettings->rawTime.size());
            ImPlot::PlotLine("Ch2", pSettings->rawTime.data(), pSettings->rawData2.data(), (int)pSettings->rawTime.size());
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class TimeChartWindow : public ImGuiWindowBase
{
public:
    TimeChartWindow(GLFWwindow* window, Settings* pSettings, ImPlotRect* pTimeChartZoomRect)
        : ImGuiWindowBase(window, "Time chart")
    {
        this->pSettings = pSettings;
        this->pTimeChartZoomRect = pTimeChartZoomRect;
    }
    void show(void);
private:
    Settings* pSettings;
    ImPlotRect* pTimeChartZoomRect;
};

inline void TimeChartWindow::show()
{
    static ImVec2 windowPos = ImVec2(450 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(990 * pSettings->monitorScale, 360 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * pSettings->times.size();
    ImGui::SliderFloat("History", &pSettings->historySec, 1, historySecMax, "%5.1f s");
    ImGui::SameLine();
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run")) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pSettings->flagPause = true; }
    }
    //ImGui::SliderFloat("Y limit", &(pSettings->limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (pSettings->limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time", "v (mV)", ImPlotAxisFlags_NoTickLabels, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (!pSettings->flagPause)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - pSettings->historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
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
                    pSettings->times[pSettings->idx] - pSettings->historySec / 2 - pSettings->historySec / 20,
                    pSettings->times[pSettings->idx] - pSettings->historySec / 2 + pSettings->historySec / 20, -pSettings->limit / 2, pSettings->limit / 2);
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
}

class TimeChartZoomWindow : public ImGuiWindowBase
{
public:
    TimeChartZoomWindow(GLFWwindow* window, Settings* pSettings, ImPlotRect* pTimeChartZoomRect)
        : ImGuiWindowBase(window, "Time chart zoom")
    {
        this->pSettings = pSettings;
        this->pTimeChartZoomRect = pTimeChartZoomRect;
    }
    void show(void);
private:
    Settings* pSettings;
    ImPlotRect* pTimeChartZoomRect;
};

inline void TimeChartZoomWindow::show()
{
    static ImVec2 windowPos = ImVec2(0 * pSettings->monitorScale, 0 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(750 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = pSettings->times[pSettings->idx];
        ImPlot::SetupAxes("Time (s)", "v (V)", 0, 0);
        if (pSettings->limit <= MILI_VOLT)
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
    }
    void show(void);
private:
    Settings* pSettings;
};

inline void DeltaTimeChartWindow::show()
{
    static ImVec2 windowPos = ImVec2(0 * pSettings->monitorScale, 750 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(450 * pSettings->monitorScale, 210 * pSettings->monitorScale);
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

class XYPlotWindow : public ImGuiWindowBase
{
private:
    Settings* pSettings;
public:
    XYPlotWindow(GLFWwindow* window, Settings* pSettings)
        : ImGuiWindowBase(window, "XY")
    {
        this->pSettings = pSettings;
    }
    void show(void);
};

inline void XYPlotWindow::show()
{
    static ImVec2 windowPos = ImVec2(880 * pSettings->monitorScale, 0 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(560 * pSettings->monitorScale, 600 * pSettings->monitorScale);
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
    if (pSettings->flagPause)
    {
        if (ImGui::Button("Run")) { pSettings->flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { pSettings->flagPause = true; }
    }
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        if (pSettings->limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("x (mV)", "y (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (pSettings->flagSurfaceMode)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit * 4, pSettings->limit * 4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit / 4, pSettings->limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        if (!pSettings->flagCh2)
        {
            ImPlot::PlotLine("##Ch1", &(pSettings->xy1Xs[0]), &(pSettings->xy1Ys[0]),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
        }
        else {
            ImPlot::PlotLine("Ch1", &(pSettings->xy1Xs[0]), &(pSettings->xy1Ys[0]),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
        }
        if (!pSettings->flagCh2)
        {
            ImPlot::PopStyleColor();
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
            ImPlot::PlotScatter("##NOW1", &(pSettings->xy1Xs[pSettings->xyIdx]), &(pSettings->xy1Ys[pSettings->xyIdx]), 1);
            ImPlot::PopStyleColor();
        }
        else {
            ImPlot::PopStyleColor();
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pSettings->monitorScale);
            ImPlot::PlotScatter("##NOW1", &(pSettings->xy1Xs[pSettings->xyIdx]), &(pSettings->xy1Ys[pSettings->xyIdx]), 1);
            ImPlot::PopStyleColor();

            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", pSettings->xy2Xs.data(), pSettings->xy2Ys.data(),
                pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
            );
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * pSettings->monitorScale);
            ImPlot::PlotScatter("##NOW2", &(pSettings->xy2Xs[pSettings->xyIdx]), &(pSettings->xy2Ys[pSettings->xyIdx]), 1);
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (pSettings->flagSurfaceMode)
        ImGui::PopStyleColor();
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
    }
    void show(void);
};

inline void ACFMPlotWindow::show()
{
    static ImVec2 windowPos = ImVec2(730 * pSettings->monitorScale, 300 * pSettings->monitorScale);
    static ImVec2 windowSize = ImVec2(560 * pSettings->monitorScale, 600 * pSettings->monitorScale);
    if (pSettings->flagSurfaceMode)
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
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
        ImPlot::SetupAxes("Bz (V)", "Bx (V)", 0, 0);
        if (pSettings->limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("x (mV)", "y (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -pSettings->limit, pSettings->limit, ImGuiCond_Always);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
        ImPlot::PlotLine("##ACFM", &(pSettings->xy1Ys[0]), &(pSettings->xy2Ys[0]),
            pSettings->xySize, 0, pSettings->xyTail, sizeof(double)
        );
        ImPlot::PlotScatter("##NOW", &(pSettings->xy1Ys[pSettings->xyIdx]), &(pSettings->xy2Ys[pSettings->xyIdx]), 1);
        ImPlot::PopStyleColor();
        ImPlot::EndPlot();
    }
    ImGui::End();
    
}
