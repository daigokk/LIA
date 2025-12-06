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
    ImVec4(0xb0 / 256.0, 0xfc / 256.0, 0x38 / 256.0, 1),   // Chartreuse
    ImVec4(1,1,1,1),   // White
    ImVec4(0,0,0,1)   // Black
};

void ScientificFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.1e", value); // 科学的記法で表示
}

void MiliFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e3); // mili: 1e-3
}

void MicroFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e6); // micro: 1e-6
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
            "Ch1y", &(liaConfig.times[0]), &(liaConfig.xyForTimeWindow[0].y[0]),
            liaConfig.size, 0, liaConfig.tail, sizeof(double)
        );
        if (liaConfig.flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(liaConfig.times[0]), &(liaConfig.xyForTimeWindow[1].y[0]),
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
            "Ch1y", &(liaConfig.times[0]), &(liaConfig.xyForTimeWindow[0].y[0]),
            liaConfig.size, 0, liaConfig.tail, sizeof(double)
        );
        if (liaConfig.flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(liaConfig.times[0]), &(liaConfig.xyForTimeWindow[1].y[0]),
                liaConfig.size, 0, liaConfig.tail, sizeof(double)
            );
        }
        static double _xmin = timeChartZoomRect.X.Min, _xmax = timeChartZoomRect.X.Max;
        static double ch1ts[2], ch1vs[2], ch1t50s[2], ch1v50s[2];
        static double ch2vs[2], ch2ts[2], ch2v50s[2];
		static int ch1vminIdx, ch1vmaxIdx, ch2vminIdx, ch2vmaxIdx;
        if (_xmin != timeChartZoomRect.X.Min || _xmax != timeChartZoomRect.X.Max) {
            _xmin = timeChartZoomRect.X.Min;
            ch1ts[0] = 0; ch1ts[1] = -1;
            ch1vs[0] = 10; ch1vs[1] = -10;
            ch2vs[0] = 10; ch2vs[1] = -10;
            for (int i = 0; i < liaConfig.size; i++)
            {
                if (liaConfig.times[i] < timeChartZoomRect.X.Min) continue;
                if (timeChartZoomRect.X.Max < liaConfig.times[i]) break;
                if (ch1vs[0] > liaConfig.xyForTimeWindow[0].y[i]) {
                    ch1vs[0] = liaConfig.xyForTimeWindow[0].y[i];
                    ch1ts[0] = liaConfig.times[i];
                    ch1vminIdx = i;
                }
                if (ch1vs[1] < liaConfig.xyForTimeWindow[0].y[i]) {
                    ch1vs[1] = liaConfig.xyForTimeWindow[0].y[i];
                    ch1ts[1] = liaConfig.times[i];
                    ch1vmaxIdx = i;
                }
                if (ch2vs[0] > liaConfig.xyForTimeWindow[1].y[i]) {
                    ch2vs[0] = liaConfig.xyForTimeWindow[1].y[i];
                    ch2ts[0] = liaConfig.times[i];
                    ch2vminIdx = i;
                }
                if (ch2vs[1] < liaConfig.xyForTimeWindow[1].y[i]) {
                    ch2vs[1] = liaConfig.xyForTimeWindow[1].y[i];
                    ch2ts[1] = liaConfig.times[i];
                    ch2vmaxIdx = i;
                }
            }
            ch1v50s[0] = ch1v50s[1] = (ch1vs[1] + ch1vs[0]) / 2;
            ch2v50s[0] = ch2v50s[1] = (ch2vs[1] + ch2vs[0]) / 2;
            bool sloop = false;
            // Find nearest points to 50% level
            if (sloop) {
                for (int i = ch1vmaxIdx; i < liaConfig.size; i++)
                {
                    if (timeChartZoomRect.X.Max < liaConfig.times[i]) break;
                    if (ch1v50s[0] >= liaConfig.xyForTimeWindow[0].y[i]) {
                        ch1t50s[1] = liaConfig.times[i];
                        break;
                    }
                }
                for (int i = ch1vmaxIdx; i >= 0; i--)
                {
                    if (timeChartZoomRect.X.Min > liaConfig.times[i]) break;
                    if (ch1v50s[0] >= liaConfig.xyForTimeWindow[0].y[i]) {
                        ch1t50s[0] = liaConfig.times[i];
                        break;
                    }
                }
            }
            else {
                for (int i = ch1vminIdx; i < liaConfig.size; i++)
                {
                    if (timeChartZoomRect.X.Max < liaConfig.times[i]) break;
                    if (ch1v50s[0] <= liaConfig.xyForTimeWindow[0].y[i]) {
                        ch1t50s[1] = liaConfig.times[i];
                        break;
                    }
                }
                for (int i = ch1vminIdx; i >= 0; i--)
                {
                    if (timeChartZoomRect.X.Min > liaConfig.times[i]) break;
                    if (ch1v50s[0] <= liaConfig.xyForTimeWindow[0].y[i]) {
                        ch1t50s[0] = liaConfig.times[i];
                        break;
                    }
                }
            }
        }
        liaConfig.acfmData.ch1vpp = abs(ch1vs[1] - ch1vs[0]);
        liaConfig.acfmData.ch2vpp = abs(ch2vs[1] - ch2vs[0]);
        if (ch1t50s[0] < ch1t50s[1] && liaConfig.acfmData.ch1vpp > 2e-3) {
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[2], -1.0f, colors[2]);
            ImPlot::PlotScatter("##ch1 minmax", ch1ts, ch1vs, 2);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[9], -1.0f, colors[9]);
            ImPlot::PlotScatter("##ch1_50% marker", ch1t50s, ch1v50s, 2);
            ImPlot::PushStyleColor(ImPlotCol_Line, colors[9]);
            ImPlot::PlotLine("##ch1_50% line", ch1t50s, ch1v50s, 2);
            ImPlot::PopStyleColor();
            ImPlot::PlotText(
                std::format("{:.3f}s", ch1t50s[1] - ch1t50s[0]).c_str(),
                (ch1t50s[1] + ch1t50s[0]) / 2,
                ch1v50s[0] - (timeChartZoomRect.Y.Max - timeChartZoomRect.Y.Min) * 0.05
            );
            ImPlot::PlotText(
                std::format("Ch1 dV: {:.0f}mV", liaConfig.acfmData.ch1vpp * 1e3).c_str(),
                timeChartZoomRect.X.Min + (timeChartZoomRect.X.Max - timeChartZoomRect.X.Min) * 0.5,
                timeChartZoomRect.Y.Min + (timeChartZoomRect.Y.Max - timeChartZoomRect.Y.Min) * 0.2
            );
        }
        if (liaConfig.flagCh2 && liaConfig.acfmData.ch2vpp > 10e-3) {
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[7], -1.0f, colors[7]);
            ImPlot::PlotScatter("##ch2 marker", ch2ts, ch2vs, 2);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[9], -1.0f, colors[9]);
            ImPlot::PlotScatter("##ch2_50% marker", ch2ts, ch2v50s, 2);
            ImPlot::PushStyleColor(ImPlotCol_Line, colors[9]);
            ImPlot::PlotLine("##ch2 line", ch2ts, ch2v50s, 2);
            ImPlot::PopStyleColor();
            ImPlot::PlotText(
                std::format("{:.3f}s", abs(ch2ts[1] - ch2ts[0])).c_str(),
                (ch2ts[1] + ch2ts[0]) / 2,
                ch2v50s[0] + (timeChartZoomRect.Y.Max - timeChartZoomRect.Y.Min) * 0.05
            );
            ImPlot::PlotText(
                std::format("Ch2 dV: {:.0f}mV", liaConfig.acfmData.ch2vpp * 1e3).c_str(),
                timeChartZoomRect.X.Min + (timeChartZoomRect.X.Max - timeChartZoomRect.X.Min) * 0.5,
                timeChartZoomRect.Y.Min + (timeChartZoomRect.Y.Max - timeChartZoomRect.Y.Min) * 0.1
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
        ImPlot::PlotLine(ch1_label, &(liaConfig.xyForXYWindow[0].x[0]), &(liaConfig.xyForXYWindow[0].y[0]),
            liaConfig.xySize, 0, liaConfig.xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[2], -1.0f, colors[2]);
        ImPlot::PlotScatter("##NOW1", &(liaConfig.xyForXYWindow[0].x[liaConfig.xyIdx]), &(liaConfig.xyForXYWindow[0].y[liaConfig.xyIdx]), 1);
        if (liaConfig.flagCh2)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
            ImPlot::PlotLine("Ch2", liaConfig.xyForXYWindow[1].x.data(), liaConfig.xyForXYWindow[1].y.data(),
                liaConfig.xySize, 0, liaConfig.xyTail, sizeof(double)
            );
            ImPlot::PopStyleColor();
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[7], -1.0f, colors[7]);
            ImPlot::PlotScatter("##NOW2", &(liaConfig.xyForXYWindow[1].x[liaConfig.xyIdx]), &(liaConfig.xyForXYWindow[1].y[liaConfig.xyIdx]), 1);
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
        this->windowPos = ImVec2(730 * liaConfig.window.monitorScale, 0 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 650 * liaConfig.window.monitorScale);
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
    ImGui::SliderFloat("Vh limit", &(liaConfig.plot.Vh_limit), 0.01f, RAW_RANGE * 1.2f, "%4.2f V");
    ImGui::SliderFloat("Vv limit", &(liaConfig.plot.Vv_limit), 0.01f, RAW_RANGE * 1.2f, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vv (V)", "Vh (V)", 0, 0);
        if (liaConfig.plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vv (mV)", "Vh (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plot.Vv_limit, liaConfig.plot.Vv_limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plot.Vh_limit, liaConfig.plot.Vh_limit, ImGuiCond_Always);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
        ImPlot::PlotLine("##ACFM", &(liaConfig.xyForXYWindow[1].y[0]), &(liaConfig.xyForXYWindow[0].y[0]),
            liaConfig.xySize, 0, liaConfig.xyTail, sizeof(double)
        );
        ImPlot::PopStyleColor();
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[8], -1.0f, colors[8]);
        ImPlot::PlotScatter("##NOW", &(liaConfig.xyForXYWindow[1].y[liaConfig.xyIdx]), &(liaConfig.xyForXYWindow[0].y[liaConfig.xyIdx]), 1);
        double vhreal = liaConfig.xyForXYWindow[0].x[liaConfig.xyIdx];
        double mm = liaConfig.acfmData.mmk[0] * vhreal * vhreal + liaConfig.acfmData.mmk[1] * vhreal + liaConfig.acfmData.mmk[1];
        const char* thickness = nullptr;
        if (mm < 0) {
            mm = 0;
        }
        if (mm <= 5) {
            thickness = std::format("{:5.2f}V:{:3.1f}mm", vhreal, mm).c_str();
        }
        else {
            thickness = std::format("{:5.2f}V:  out", vhreal).c_str();
        }
        ImPlot::PlotText(
            thickness,
            0.0,
            liaConfig.plot.Vh_limit * 0.9
        );
        ImPlot::EndPlot();
    }
    ImGui::End();
}

class ACFMVhVvPlotWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
        : ImGuiWindowBase(window, "ACFM Vh-Vv"), liaConfig(liaConfig)
    {
        this->windowPos = ImVec2(730 * liaConfig.window.monitorScale, 0 * liaConfig.window.monitorScale);
        this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 650 * liaConfig.window.monitorScale);
    }
    void show(void);
};

inline void ACFMVhVvPlotWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    // プロット描画
    if (ImPlot::BeginPlot("##Vh-Vv", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vvpp (V)", "Vhpp (V)", 0, 0);
        if (liaConfig.plot.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vvpp (mV)", "Vhpp (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, liaConfig.plot.Vv_limit*2, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, liaConfig.plot.Vh_limit, ImGuiCond_Always);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[2], -1.0f, colors[2]);
        ImPlot::PlotScatter("References", liaConfig.acfmData.Vvs.data(), liaConfig.acfmData.Vhs.data(), liaConfig.acfmData.size);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 5 * liaConfig.window.monitorScale, colors[7], -1.0f, colors[7]);
        ImPlot::PlotScatter("Result", &(liaConfig.acfmData.ch2vpp), &(liaConfig.acfmData.ch1vpp), 1);
        ImPlot::EndPlot();
    }
    ImGui::End();
}
