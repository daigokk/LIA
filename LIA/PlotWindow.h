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
private:
    LiaConfig& liaConfig;
public:
    RawPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
		: ImGuiWindowBase(window, "Raw waveform"), liaConfig(liaConfig)
    {
        this->windowPos = ImVec2(450 * liaConfig.window.monitorScale, 0 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(430 * liaConfig.window.monitorScale, 600 * liaConfig.window.monitorScale);
    }
    void show(void);
};

inline void RawPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, liaConfig.imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imgui.windowFlag);
    ImGui::Begin(this->name);
    
    if (ImGui::Button("Save"))
    {
        liaConfig.saveRawData(std::format("raw_{}.csv", liaConfig.getCurrentTimestamp()));
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawSave;
        value = 0;
    }
    //ImGui::SameLine();
    ImGui::SliderFloat("Y limit", &(liaConfig.plot.rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawLimit;
        value = liaConfig.plot.rawLimit;
    }
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        if (liaConfig.plot.rawLimit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (us)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, liaConfig.rawTime.data()[0], liaConfig.rawTime.data()[liaConfig.rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plot.rawLimit, liaConfig.plot.rawLimit, ImGuiCond_Always);
        
        const char* ch1_label = liaConfig.flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        ImPlot::PlotLine(ch1_label, liaConfig.rawTime.data(), liaConfig.rawData[0].data(), (int)liaConfig.rawTime.size());
        ImPlot::PopStyleColor();
        if (liaConfig.flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", liaConfig.rawTime.data(), liaConfig.rawData[1].data(), (int)liaConfig.rawTime.size());
            ImPlot::PopStyleColor();
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
    if (button != ButtonType::NON)
    {
        liaConfig.cmds.push_back(std::array<float, 3>{ (float)liaConfig.timer.elapsedSec(), (float)button, value });
    }
}

class TimeChartWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
    ImPlotRect& timeChartZoomRect;
public:
    TimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig, ImPlotRect& timeChartZoomRect)
		: ImGuiWindowBase(window, "Time chart"), liaConfig(liaConfig), timeChartZoomRect(timeChartZoomRect)
    {
        this->windowPos = ImVec2(450 * liaConfig.window.monitorScale, 600 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(990 * liaConfig.window.monitorScale, 360 * liaConfig.window.monitorScale);
    }
    void show(void);
};

inline void TimeChartWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, liaConfig.imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imgui.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * liaConfig.times.size();
    if (liaConfig.flagPause) { ImGui::BeginDisabled(); }
    ImGui::SetNextItemWidth(500.0f * liaConfig.window.monitorScale);
    ImGui::SliderFloat("History", &liaConfig.plot.historySec, 1, historySecMax, "%5.1f s");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimeHistory;
        value = liaConfig.plot.historySec;
    }
    if (liaConfig.flagPause) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        liaConfig.saveResultsToFile(
            std::format("ect_{}.csv", liaConfig.getCurrentTimestamp()),
            liaConfig.plot.historySec
        );
    }
    ImGui::SameLine();
    if (liaConfig.flagPause)
    {
        if (ImGui::Button("Run")) { liaConfig.flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { liaConfig.flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimePause;
        value = liaConfig.flagPause;
    }
    
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.times[liaConfig.idx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (liaConfig.plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time", "v (mV)", ImPlotAxisFlags_NoTickLabels, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (!liaConfig.flagPause)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - liaConfig.plot.historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plot.limit, liaConfig.plot.limit, ImGuiCond_Always);
        ImPlot::PlotLine(
            "Ch1y", &(liaConfig.times[0]), &(liaConfig.y1s[0]),
            liaConfig.size, 0, liaConfig.tail, sizeof(double)
        );
        if (liaConfig.flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(liaConfig.times[0]), &(liaConfig.y2s[0]),
                liaConfig.size, 0, liaConfig.tail, sizeof(double)
            );
        }
        static bool flag = true;
        if (liaConfig.flagPause)
        {
            if (flag)
            {
                timeChartZoomRect = ImPlotRect(
                    liaConfig.times[liaConfig.idx] - liaConfig.plot.historySec / 2 - liaConfig.plot.historySec / 20,
                    liaConfig.times[liaConfig.idx] - liaConfig.plot.historySec / 2 + liaConfig.plot.historySec / 20, -liaConfig.plot.limit / 2, liaConfig.plot.limit / 2);
                flag = false;
            }
            static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
            bool clicked = false;
            bool hovered = false;
            bool held = false;
            ImPlot::DragRect(
                0, &timeChartZoomRect.X.Min, &timeChartZoomRect.Y.Min, 
                &timeChartZoomRect.X.Max, &timeChartZoomRect.Y.Max, 
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
        liaConfig.cmds.push_back(std::array<float, 3>{ (float)liaConfig.timer.elapsedSec(), (float)button, value });
    }
}

class TimeChartZoomWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
    ImPlotRect& timeChartZoomRect;
public:
    TimeChartZoomWindow(GLFWwindow* window, LiaConfig& liaConfig, ImPlotRect& timeChartZoomRect)
		: ImGuiWindowBase(window, "Time chart zoom"), liaConfig(liaConfig), timeChartZoomRect(timeChartZoomRect)
    {
        this->windowPos = ImVec2(0 * liaConfig.window.monitorScale, 0 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(750 * liaConfig.window.monitorScale, 600 * liaConfig.window.monitorScale);
    }
    void show(void);

};

inline void TimeChartZoomWindow::show()
{
    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.times[liaConfig.idx];
        ImPlot::SetupAxes("Time (s)", "v (V)", 0, 0);
        if (liaConfig.plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (s)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, timeChartZoomRect.X.Min, timeChartZoomRect.X.Max, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, timeChartZoomRect.Y.Min, timeChartZoomRect.Y.Max, ImGuiCond_Always);
        ImPlot::PlotLine(
            "Ch1y", &(liaConfig.times[0]), &(liaConfig.y1s[0]),
            liaConfig.size, 0, liaConfig.tail, sizeof(double)
        );
        if (liaConfig.flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(liaConfig.times[0]), &(liaConfig.y2s[0]),
                liaConfig.size, 0, liaConfig.tail, sizeof(double)
            );
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
}

class DeltaTimeChartWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig)
		: ImGuiWindowBase(window, "DeltTime chart"), liaConfig(liaConfig)
    {
        this->windowPos = ImVec2(0 * liaConfig.window.monitorScale, 750 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(450 * liaConfig.window.monitorScale, 210 * liaConfig.window.monitorScale);
    }
    void show(void);
};

inline void DeltaTimeChartWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, liaConfig.imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imgui.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT)*liaConfig.times.size();
    static float historySec = 10;
    ImGui::SliderFloat("History", &historySec, 1, historySecMax, "%5.1f s");
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.times[liaConfig.idx];
        ImPlot::SetupAxes("Time", "dt (ms)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, (MEASUREMENT_DT - 2e-3) * 1e3, (MEASUREMENT_DT + 2e-3) * 1e3, ImGuiCond_Always);
        ImPlot::PlotLine(
            "##dt", &(liaConfig.times[0]), &(liaConfig.dts[0]),
            liaConfig.size, 0, liaConfig.tail, sizeof(double)
        );
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class XYPlotWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    XYPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
		: ImGuiWindowBase(window, "XY"), liaConfig(liaConfig)
    {
        this->windowPos = ImVec2(880 * liaConfig.window.monitorScale, 0 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 600 * liaConfig.window.monitorScale);
    }
    void show(void);
};

inline void XYPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    if (liaConfig.plot.surfaceMode)
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    ImGui::SetNextWindowPos(windowPos, liaConfig.imgui.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imgui.windowFlag); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.1f V");
    if (ImGui::Button("Clear"))
    {
        liaConfig.xyTail = 0; liaConfig.xyNorm = 0;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYClear;
        value = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) {
        liaConfig.flagAutoOffset = true;
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYAutoOffset;
        value = 0;
    }
    ImGui::SameLine();
    if (liaConfig.flagPause)
    {
        if (ImGui::Button("Run")) { liaConfig.flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { liaConfig.flagPause = true; }
    }
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::XYPause;
        value = liaConfig.flagPause;
    }
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        if (liaConfig.plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("x (mV)", "y (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (liaConfig.plot.surfaceMode)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plot.limit * 4, liaConfig.plot.limit * 4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plot.limit / 4, liaConfig.plot.limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plot.limit, liaConfig.plot.limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plot.limit, liaConfig.plot.limit, ImGuiCond_Always);
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0, ImPlotColormap_Deep));
        const char* ch1_label = liaConfig.flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PlotLine(ch1_label, &(liaConfig.xyForXY[0].x[0]), &(liaConfig.xyForXY[0].y[0]),
            liaConfig.xySize, 0, liaConfig.xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[2], -1.0f, colors[2]);
        ImPlot::PlotScatter("##NOW1", &(liaConfig.xyForXY[0].x[liaConfig.xyIdx]), &(liaConfig.xyForXY[0].y[liaConfig.xyIdx]), 1);
        if (liaConfig.flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", liaConfig.xyForXY[1].x.data(), liaConfig.xyForXY[1].y.data(),
                liaConfig.xySize, 0, liaConfig.xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[7], -1.0f, colors[7]);
            ImPlot::PlotScatter("##NOW2", &(liaConfig.xyForXY[1].x[liaConfig.xyIdx]), &(liaConfig.xyForXY[1].y[liaConfig.xyIdx]), 1);
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (liaConfig.plot.surfaceMode)
        ImGui::PopStyleColor();
    if (button != ButtonType::NON)
    {
        liaConfig.cmds.push_back(std::array<float, 3>{ (float)liaConfig.timer.elapsedSec(), (float)button, value });
    }
}


class ACFMPlotWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
		: ImGuiWindowBase(window, "ACFM"), liaConfig(liaConfig)
    {
        this->windowPos = ImVec2(730 * liaConfig.window.monitorScale, 300 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 600 * liaConfig.window.monitorScale);
    }
    void show(void);
};

inline void ACFMPlotWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.1f V");
    if (ImGui::Button("Clear"))
    {
        liaConfig.xyTail = 0; liaConfig.xyNorm = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) {
        liaConfig.flagAutoOffset = true;
    }
    ImGui::SameLine();
    if (liaConfig.flagPause)
    {
        if (ImGui::Button("Run")) { liaConfig.flagPause = false; }
    }
    else {
        if (ImGui::Button("Pause")) { liaConfig.flagPause = true; }
    }
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vbz (V)", "Vbx (V)", 0, 0);
        if (liaConfig.plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vbz (mV)", "Vbx (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plot.limit, liaConfig.plot.limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plot.limit, liaConfig.plot.limit, ImGuiCond_Always);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
        ImPlot::PlotLine("##ACFM", &(liaConfig.xyForXY[0].y[0]), &(liaConfig.xyForXY[1].y[0]),
            liaConfig.xySize, 0, liaConfig.xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[8], -1.0f, colors[8]);
        ImPlot::PlotScatter("##NOW", &(liaConfig.xyForXY[0].y[liaConfig.xyIdx]), &(liaConfig.xyForXY[1].y[liaConfig.xyIdx]), 1);
        ImPlot::EndPlot();
    }
    ImGui::End();
}
