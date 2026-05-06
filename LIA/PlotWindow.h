#pragma once

#include "ImGuiWindowBase.h"
#include "LiaConfig.h"

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
    RawPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline RawPlotWindow::RawPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Raw waveform"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(450 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(430 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
}

inline void RawPlotWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;
    
    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
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
    ImGui::SliderFloat("Y limit", &(liaConfig.plotCfg.rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::RawLimit;
        value = liaConfig.plotCfg.rawLimit;
    }
    // プロット描画
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time (us)", "v (V)", 0, 0);
        if (liaConfig.plotCfg.rawLimit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (us)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, liaConfig.rawTime.data()[0], liaConfig.rawTime.data()[liaConfig.rawTime.size() - 1], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.rawLimit, liaConfig.plotCfg.rawLimit, ImGuiCond_Always);
        
        const char* ch1_label = liaConfig.flagCh2 ? "Ch1" : "##Ch1";
        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);
        ImPlot::PlotLine(ch1_label, liaConfig.rawTime.data(), liaConfig.rawData[0].data(), (int)liaConfig.rawTime.size(), specLine);
        if (liaConfig.flagCh2)
        {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            ImPlot::PlotLine("Ch2", liaConfig.rawTime.data(), liaConfig.rawData[1].data(), (int)liaConfig.rawTime.size(), specLine);
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
    if (button != ButtonType::NON)
    {
        liaConfig.cmds.push_back(std::array<float, 6>{ (float)liaConfig.timer.elapsedSec(), (float)button, value, 0, 0, 0 });
    }
}

class TimeChartWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    TimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline TimeChartWindow::TimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Time chart"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(450 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(990 * liaConfig.windowCfg.monitorScale, 360 * liaConfig.windowCfg.monitorScale);
}

inline void TimeChartWindow::show()
{
    ButtonType button = ButtonType::NON;
    static float value = 0;

    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT) * (liaConfig.ringBuffer.times.size() - 1);
    if (liaConfig.pauseCfg.flag) { ImGui::BeginDisabled(); }
    ImGui::SetNextItemWidth(500.0f * liaConfig.windowCfg.monitorScale);
    ImGui::SliderFloat("History", &liaConfig.plotCfg.historySec, 1, historySecMax, "%5.1f s");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimeHistory;
        value = liaConfig.plotCfg.historySec;
    }
    if (liaConfig.pauseCfg.flag) { ImGui::EndDisabled(); }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        liaConfig.saveResultsToFile(
            std::format("ect_{}.csv", liaConfig.getCurrentTimestamp()),
            liaConfig.plotCfg.historySec
        );
    }
    ImGui::SameLine();
    // 実行/一時停止トグル
    ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        button = ButtonType::TimePause;
        value = liaConfig.pauseCfg.flag;
        liaConfig.pauseCfg.flag = !liaConfig.pauseCfg.flag;
        liaConfig.buttonPause();
    }

    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx];
        ImPlot::SetupAxes("Time", "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (liaConfig.plotCfg.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time", "v (mV)", ImPlotAxisFlags_NoTickLabels, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (!liaConfig.pauseCfg.flag)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - liaConfig.plotCfg.historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.limit, liaConfig.plotCfg.limit, ImGuiCond_Always);
        ImPlotSpec specLine;
        specLine.Offset = liaConfig.ringBuffer.writeIdx;
        int count = liaConfig.ringBuffer.size; // 描画中にリングバッファが更新される可能性があるため、描画開始時のサイズを取得
        ImPlot::PlotLine(
            "Ch1y", &(liaConfig.ringBuffer.times[liaConfig.plotCfg.idxStart]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.idxStart]),
            count, specLine
        );
        if (liaConfig.flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(liaConfig.ringBuffer.times[0]), &(liaConfig.ringBuffer.ch[1].y[0]),
                count, specLine
            );
        }
        static bool flag = true;
        if (liaConfig.pauseCfg.flag)
        {
            if (flag) // 初回のみ矩形を右端にセット
            {
                liaConfig.pauseCfg.set(
                    liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx] - liaConfig.plotCfg.historySec / 10,
                    liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx],
                    -liaConfig.plotCfg.limit / 2,
                    liaConfig.plotCfg.limit / 2);
                flag = false;
            }
            static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
            bool clicked = false;
            bool hovered = false;
            bool held = false;
            ImPlot::DragRect(
                0, &liaConfig.pauseCfg.selectArea.X.Min, &liaConfig.pauseCfg.selectArea.Y.Min,
                &liaConfig.pauseCfg.selectArea.X.Max, &liaConfig.pauseCfg.selectArea.Y.Max,
                ImVec4(1, 0, 1, 1), flags, &clicked, &hovered, &held
            );
            // Pauseの時は、XYPlotの描画範囲をTimeChartの全体から、矩形選択範囲にする
            bool flag = false;
            double deltaTimeMin = LONG_MAX;
			double deltaTimeMax = LONG_MAX;
			int xyStartIdx = 0, xyLatestIdx = liaConfig.ringBuffer.latestIdx;
            for (int idx = liaConfig.ringBuffer.latestIdx; 0 <= idx; idx--)
            {
                double _deltaTimeMin = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Min);
                double _deltaTimeMax = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Max);
                if (deltaTimeMin > _deltaTimeMin) {
                    deltaTimeMin = _deltaTimeMin;
                    xyStartIdx = idx;
                }
                if (deltaTimeMax > _deltaTimeMax) {
                    deltaTimeMax = _deltaTimeMax;
                    xyLatestIdx = idx;
                }
                if (deltaTimeMin < MEASUREMENT_DT and deltaTimeMax < MEASUREMENT_DT) {
                    flag = true;
                    break;
                }
            }
            if (!flag and liaConfig.ringBuffer.size < liaConfig.ringBuffer.nofm) {
                for (int idx = liaConfig.ringBuffer.size-1; liaConfig.ringBuffer.latestIdx < idx; idx--)
                {
                    double _deltaTimeMin = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Min);
                    double _deltaTimeMax = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Max);
                    if (deltaTimeMin > _deltaTimeMin) {
                        deltaTimeMin = _deltaTimeMin;
                        xyStartIdx = idx;
                    }
                    if (deltaTimeMax > _deltaTimeMax) {
                        deltaTimeMax = _deltaTimeMax;
                        xyLatestIdx = idx;
                    }
                    if (deltaTimeMin < MEASUREMENT_DT and deltaTimeMax < MEASUREMENT_DT) {
                        break;
                    }
                }
            }
            liaConfig.plotCfg.xyStartIdx = xyStartIdx;
			liaConfig.plotCfg.xyLatestIdx = xyLatestIdx;

			int xySize = liaConfig.plotCfg.xyLatestIdx - liaConfig.plotCfg.xyStartIdx;
            if (xySize >= 0) {
                liaConfig.plotCfg.xySize = xySize;
            }
            else {
                liaConfig.plotCfg.xySize = liaConfig.ringBuffer.size - liaConfig.plotCfg.xyStartIdx + liaConfig.plotCfg.xyLatestIdx;
            }
        }
        else {
            flag = true;
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (button != ButtonType::NON)
    {
        liaConfig.cmds.push_back(std::array<float, 6>{ (float)liaConfig.timer.elapsedSec(), (float)button, value, 0, 0, 0 });
    }
}

class TimeChartZoomWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    TimeChartZoomWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);

};

inline TimeChartZoomWindow::TimeChartZoomWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Time chart zoom"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(0 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(750 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
}

inline void TimeChartZoomWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);
    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx];
        ImPlot::SetupAxes("Time (s)", "v (V)", 0, 0);
        if (liaConfig.plotCfg.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Time (s)", "v (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, liaConfig.pauseCfg.selectArea.X.Min, liaConfig.pauseCfg.selectArea.X.Max, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, liaConfig.pauseCfg.selectArea.Y.Min, liaConfig.pauseCfg.selectArea.Y.Max, ImGuiCond_Always);
        ImPlotSpec specLine;
        specLine.Offset = liaConfig.ringBuffer.writeIdx;
        ImPlot::PlotLine(
            "Ch1y", &(liaConfig.ringBuffer.times[0]), &(liaConfig.ringBuffer.ch[0].y[0]),
            liaConfig.ringBuffer.size, specLine
        );
        if (liaConfig.flagCh2)
        {
            ImPlot::PlotLine(
                "Ch2y", &(liaConfig.ringBuffer.times[0]), &(liaConfig.ringBuffer.ch[1].y[0]),
                liaConfig.ringBuffer.size, specLine
            );
        }
        static double _xmin = liaConfig.pauseCfg.selectArea.X.Min, _xmax = liaConfig.pauseCfg.selectArea.X.Max;
        static double ch1ts[2], ch1vs[2], ch1t50s[2], ch1v50s[2];
        static double ch2vs[2], ch2ts[2], ch2v50s[2];
		static int ch1vminIdx, ch1vmaxIdx, ch2vminIdx, ch2vmaxIdx;
        if (_xmin != liaConfig.pauseCfg.selectArea.X.Min || _xmax != liaConfig.pauseCfg.selectArea.X.Max) {
            _xmin = liaConfig.pauseCfg.selectArea.X.Min;
            ch1ts[0] = 0; ch1ts[1] = -1;
            ch1vs[0] = 10; ch1vs[1] = -10;
            ch2vs[0] = 10; ch2vs[1] = -10;
            for (int i = 0; i < liaConfig.ringBuffer.size; i++)
            {
                if (liaConfig.ringBuffer.times[i] < liaConfig.pauseCfg.selectArea.X.Min) continue;
                if (liaConfig.pauseCfg.selectArea.X.Max < liaConfig.ringBuffer.times[i]) break;
                if (ch1vs[0] > liaConfig.ringBuffer.ch[0].y[i]) {
                    ch1vs[0] = liaConfig.ringBuffer.ch[0].y[i];
                    ch1ts[0] = liaConfig.ringBuffer.times[i];
                    ch1vminIdx = i;
                }
                if (ch1vs[1] < liaConfig.ringBuffer.ch[0].y[i]) {
                    ch1vs[1] = liaConfig.ringBuffer.ch[0].y[i];
                    ch1ts[1] = liaConfig.ringBuffer.times[i];
                    ch1vmaxIdx = i;
                }
                if (ch2vs[0] > liaConfig.ringBuffer.ch[1].y[i]) {
                    ch2vs[0] = liaConfig.ringBuffer.ch[1].y[i];
                    ch2ts[0] = liaConfig.ringBuffer.times[i];
                    ch2vminIdx = i;
                }
                if (ch2vs[1] < liaConfig.ringBuffer.ch[1].y[i]) {
                    ch2vs[1] = liaConfig.ringBuffer.ch[1].y[i];
                    ch2ts[1] = liaConfig.ringBuffer.times[i];
                    ch2vmaxIdx = i;
                }
            }
            ch1v50s[0] = ch1v50s[1] = (ch1vs[1] + ch1vs[0]) / 2;
            ch2v50s[0] = ch2v50s[1] = (ch2vs[1] + ch2vs[0]) / 2;
            bool sloop = false;
            // Find nearest points to 50% level
            if (sloop) {
                for (int i = ch1vmaxIdx; i < liaConfig.ringBuffer.size; i++)
                {
                    if (liaConfig.pauseCfg.selectArea.X.Max < liaConfig.ringBuffer.times[i]) break;
                    if (ch1v50s[0] >= liaConfig.ringBuffer.ch[0].y[i]) {
                        ch1t50s[1] = liaConfig.ringBuffer.times[i];
                        break;
                    }
                }
                for (int i = ch1vmaxIdx; i >= 0; i--)
                {
                    if (liaConfig.pauseCfg.selectArea.X.Min > liaConfig.ringBuffer.times[i]) break;
                    if (ch1v50s[0] >= liaConfig.ringBuffer.ch[0].y[i]) {
                        ch1t50s[0] = liaConfig.ringBuffer.times[i];
                        break;
                    }
                }
            }
            else {
                for (int i = ch1vminIdx; i < liaConfig.ringBuffer.size; i++)
                {
                    if (liaConfig.pauseCfg.selectArea.X.Max < liaConfig.ringBuffer.times[i]) break;
                    if (ch1v50s[0] <= liaConfig.ringBuffer.ch[0].y[i]) {
                        ch1t50s[1] = liaConfig.ringBuffer.times[i];
                        break;
                    }
                }
                for (int i = ch1vminIdx; i >= 0; i--)
                {
                    if (liaConfig.pauseCfg.selectArea.X.Min > liaConfig.ringBuffer.times[i]) break;
                    if (ch1v50s[0] <= liaConfig.ringBuffer.ch[0].y[i]) {
                        ch1t50s[0] = liaConfig.ringBuffer.times[i];
                        break;
                    }
                }
            }
        }
        liaConfig.acfmData.ch1vpp = abs(ch1vs[1] - ch1vs[0]);
        liaConfig.acfmData.ch2vpp = abs(ch2vs[1] - ch2vs[0]);

        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.MarkerFillColor = colors[2];
        specScatter.LineWeight = -1.0f;
        specScatter.MarkerLineColor = colors[2];
        if (ch1t50s[0] < ch1t50s[1] && liaConfig.acfmData.ch1vpp > 2e-3) {
            ImPlot::PlotScatter("##ch1 minmax", ch1ts, ch1vs, 2, specScatter);
            specScatter.MarkerFillColor = colors[9];
            specScatter.MarkerLineColor = colors[9];
            ImPlot::PlotScatter("##ch1_50% marker", ch1t50s, ch1v50s, 2, specScatter);
            specLine.LineColor = colors[9];
            ImPlot::PlotLine("##ch1_50% line", ch1t50s, ch1v50s, 2, specLine);
            ImPlot::PlotText(
                std::format("{:.3f}s", ch1t50s[1] - ch1t50s[0]).c_str(),
                (ch1t50s[1] + ch1t50s[0]) / 2,
                ch1v50s[0] - (liaConfig.pauseCfg.selectArea.Y.Max - liaConfig.pauseCfg.selectArea.Y.Min) * 0.05
            );
            ImPlot::PlotText(
                std::format("Ch1 dV: {:.0f}mV", liaConfig.acfmData.ch1vpp * 1e3).c_str(),
                liaConfig.pauseCfg.selectArea.X.Min + (liaConfig.pauseCfg.selectArea.X.Max - liaConfig.pauseCfg.selectArea.X.Min) * 0.5,
                liaConfig.pauseCfg.selectArea.Y.Min + (liaConfig.pauseCfg.selectArea.Y.Max - liaConfig.pauseCfg.selectArea.Y.Min) * 0.2
            );
        }
        if (liaConfig.flagCh2 && liaConfig.acfmData.ch2vpp > 10e-3) {
            specScatter.MarkerFillColor = colors[7];
            specScatter.MarkerLineColor = colors[7];
            ImPlot::PlotScatter("##ch2 marker", ch2ts, ch2vs, 2, specScatter);
            specScatter.MarkerFillColor = colors[9];
            specScatter.MarkerLineColor = colors[9];
            ImPlot::PlotScatter("##ch2_50% marker", ch2ts, ch2v50s, 2, specScatter);
			specLine.LineColor = colors[9];
            ImPlot::PlotLine("##ch2 line", ch2ts, ch2v50s, 2, specLine);
            ImPlot::PlotText(
                std::format("{:.3f}s", abs(ch2ts[1] - ch2ts[0])).c_str(),
                (ch2ts[1] + ch2ts[0]) / 2,
                ch2v50s[0] + (liaConfig.pauseCfg.selectArea.Y.Max - liaConfig.pauseCfg.selectArea.Y.Min) * 0.05
            );
            ImPlot::PlotText(
                std::format("Ch2 dV: {:.0f}mV", liaConfig.acfmData.ch2vpp * 1e3).c_str(),
                liaConfig.pauseCfg.selectArea.X.Min + (liaConfig.pauseCfg.selectArea.X.Max - liaConfig.pauseCfg.selectArea.X.Min) * 0.5,
                liaConfig.pauseCfg.selectArea.Y.Min + (liaConfig.pauseCfg.selectArea.Y.Max - liaConfig.pauseCfg.selectArea.Y.Min) * 0.1
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
    DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline DeltaTimeChartWindow::DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "DeltTime chart"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(0 * liaConfig.windowCfg.monitorScale, 750 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(450 * liaConfig.windowCfg.monitorScale, 210 * liaConfig.windowCfg.monitorScale);
}

inline void DeltaTimeChartWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);
    static float historySecMax = (float)(MEASUREMENT_DT)*liaConfig.ringBuffer.times.size();
    static float historySec = 10;
    ImGui::SliderFloat("History", &historySec, 1, historySecMax, "%5.1f s");
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx];
        ImPlot::SetupAxes("Time", "dt (ms)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, (MEASUREMENT_DT - 2e-3) * 1e3, (MEASUREMENT_DT + 2e-3) * 1e3, ImGuiCond_Always);
        ImPlotSpec specLine;
        specLine.Offset = liaConfig.ringBuffer.writeIdx;
        ImPlot::PlotLine(
            "##dt", &(liaConfig.ringBuffer.times[0]), &(liaConfig.deltaTimes[0]),
            liaConfig.ringBuffer.size, specLine
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
    XYPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline XYPlotWindow::XYPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "XY"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(880 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
}

inline void XYPlotWindow::show()
{
    if (liaConfig.plotCfg.surfaceMode)
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.1f V");
    ImGui::Button("Clear");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        liaConfig.buttonClear();
    }
    ImGui::SameLine();
    ImGui::Button("Auto offset");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        liaConfig.buttonAutoOffset();
    }
    ImGui::SameLine();
    // 実行/一時停止トグル
    ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        
        liaConfig.pauseCfg.flag = !liaConfig.pauseCfg.flag;
        liaConfig.buttonPause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rec.")) { liaConfig.buttonRec(); }

    // プロット描画
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0)); // 凡例の背景を透明に
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("x (V)", "y (V)", 0, 0);
        if (liaConfig.plotCfg.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("x (mV)", "y (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        if (liaConfig.plotCfg.surfaceMode)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plotCfg.limit * 4, liaConfig.plotCfg.limit * 4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.limit / 4, liaConfig.plotCfg.limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plotCfg.limit, liaConfig.plotCfg.limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.limit, liaConfig.plotCfg.limit, ImGuiCond_Always);
        }
		ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);
        const char* ch1_label = liaConfig.flagCh2 ? "Ch1" : "##Ch1";
		if (liaConfig.plotCfg.xyStartIdx + liaConfig.plotCfg.xySize > liaConfig.ringBuffer.size) {
            int firstPartSize = liaConfig.ringBuffer.size - liaConfig.plotCfg.xyStartIdx;
            ImPlot::PlotLine(ch1_label, &(liaConfig.ringBuffer.ch[0].x[liaConfig.plotCfg.xyStartIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyStartIdx]),
                firstPartSize, specLine
            );
            ImPlot::PlotLine(ch1_label, &(liaConfig.ringBuffer.ch[0].x[0]), &(liaConfig.ringBuffer.ch[0].y[0]),
                liaConfig.plotCfg.xySize - firstPartSize, specLine
            );
        }
        else {
            ImPlot::PlotLine(ch1_label, &(liaConfig.ringBuffer.ch[0].x[liaConfig.plotCfg.xyStartIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyStartIdx]),
                liaConfig.plotCfg.xySize, specLine
            );
        }

        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.MarkerFillColor = colors[2];
        specScatter.LineWeight = 2.0f;
        specScatter.MarkerLineColor = colors[2];
        ImPlot::PlotScatter("##NOW1", &(liaConfig.ringBuffer.ch[0].x[liaConfig.plotCfg.xyLatestIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyLatestIdx]), 1, specScatter);
        specScatter.MarkerFillColor = ImVec4(0,0,0,0);
        ImPlot::PlotScatter("##REC1", liaConfig.xyRecs.ch1xys.x.data(), liaConfig.xyRecs.ch1xys.y.data(), (int)liaConfig.xyRecs.ch1xys.x.size(), specScatter);
        if (liaConfig.flagCh2)
        {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            if (liaConfig.plotCfg.xyStartIdx + liaConfig.plotCfg.xySize > liaConfig.ringBuffer.size) {
                int firstPartSize = liaConfig.ringBuffer.size - liaConfig.plotCfg.xyStartIdx;
                ImPlot::PlotLine(ch1_label, &(liaConfig.ringBuffer.ch[1].x[liaConfig.plotCfg.xyStartIdx]), &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyStartIdx]),
                    firstPartSize, specLine
                );
                ImPlot::PlotLine(ch1_label, &(liaConfig.ringBuffer.ch[1].x[0]), &(liaConfig.ringBuffer.ch[1].y[0]),
                    liaConfig.plotCfg.xySize - firstPartSize, specLine
                );
            }
            else {
                ImPlot::PlotLine("Ch2", &(liaConfig.ringBuffer.ch[1].x[liaConfig.plotCfg.xyStartIdx]), &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyStartIdx]),
                    liaConfig.plotCfg.xySize, specLine
                );
            }
            specScatter.MarkerFillColor = colors[7];
            specScatter.MarkerLineColor = colors[7];
            ImPlot::PlotScatter("##NOW2", &(liaConfig.ringBuffer.ch[1].x[liaConfig.plotCfg.xyLatestIdx]), &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyLatestIdx]), 1, specScatter);
            specScatter.MarkerFillColor = ImVec4(0, 0, 0, 0);
            ImPlot::PlotScatter("##REC2", liaConfig.xyRecs.ch2xys.x.data(), liaConfig.xyRecs.ch2xys.y.data(), (int)liaConfig.xyRecs.ch2xys.x.size(), specScatter);
        }
        if(liaConfig.flagAutoSetupW2History){
            specLine.LineColor = ImPlot::GetColormapColor(2, ImPlotColormap_Deep);
            ImPlot::PlotLine("##HISTORY W1", liaConfig.autoSetupHistoryW1.x.data(), liaConfig.autoSetupHistoryW1.y.data(),
                liaConfig.autoSetupHistoryW1.x.size(), specLine
            );
            specLine.LineColor = ImPlot::GetColormapColor(3, ImPlotColormap_Deep);
            ImPlot::PlotLine("##HISTORY W2", liaConfig.autoSetupHistoryW2.x.data(), liaConfig.autoSetupHistoryW2.y.data(),
                liaConfig.autoSetupHistoryW2.x.size(), specLine
            );
		}
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
    if (liaConfig.plotCfg.surfaceMode)
        ImGui::PopStyleColor();
}


class ACFMPlotWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;
public:
    ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline ACFMPlotWindow::ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "ACFM"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(730 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.windowCfg.monitorScale, 650 * liaConfig.windowCfg.monitorScale);
}

inline void ACFMPlotWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    //ImGui::SliderFloat("Y limit", &(liaConfig.limit), 0.1, 2.0, "%4.1f V");
    ImGui::Button("Clear");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        liaConfig.buttonClear();
    }
    ImGui::SameLine();
    ImGui::Button("Auto offset");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        liaConfig.buttonAutoOffset();
    }
    ImGui::SameLine();
    // 実行/一時停止トグル
    ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause");
    if (ImGui::IsItemDeactivated()) {
        // ボタンが離された瞬間（フォーカスが外れた）
        liaConfig.pauseCfg.flag = !liaConfig.pauseCfg.flag;
        liaConfig.buttonPause();
    }
    if (ImGui::SliderFloat("Vx limit", &(liaConfig.plotCfg.Vx_limt), 0.01f, RAW_RANGE * 1.2f, "%4.2f V")) {
        liaConfig.plotCfg.Vz_limt = liaConfig.plotCfg.Vx_limt * 2;
		liaConfig.plotCfg.limit = liaConfig.plotCfg.Vz_limt;
    }
    ImGui::SliderFloat("Vz limit", &(liaConfig.plotCfg.Vz_limt), 0.01f, RAW_RANGE * 1.2f, "%4.2f V");
    // プロット描画
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vz (V)", "Vx (V)", 0, 0);
        if (liaConfig.plotCfg.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vz (mV)", "Vx (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plotCfg.Vz_limt, liaConfig.plotCfg.Vz_limt, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.Vx_limt, liaConfig.plotCfg.Vx_limt, ImGuiCond_Always);
        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(2, ImPlotColormap_Deep);
        if (liaConfig.plotCfg.xyStartIdx + liaConfig.plotCfg.xySize > liaConfig.ringBuffer.size) {
            int firstPartSize = liaConfig.ringBuffer.size - liaConfig.plotCfg.xyStartIdx;
            ImPlot::PlotLine("##ACFM", &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyStartIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyStartIdx]),
                firstPartSize, specLine
            );
            ImPlot::PlotLine("##ACFM", &(liaConfig.ringBuffer.ch[1].y[0]), &(liaConfig.ringBuffer.ch[0].y[0]),
                liaConfig.plotCfg.xySize - firstPartSize, specLine
            );
        }
        else {
            ImPlot::PlotLine("##ACFM", &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyStartIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyStartIdx]),
                liaConfig.plotCfg.xySize, specLine
            );
        }
        
        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.MarkerFillColor = colors[8];
        specScatter.LineWeight = -1.0f;
        specScatter.MarkerLineColor = colors[8];
        ImPlot::PlotScatter("##NOW", &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyLatestIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyLatestIdx]), 1, specScatter);
        double vhreal = liaConfig.ringBuffer.ch[0].x[liaConfig.plotCfg.xyLatestIdx];
        double mm = liaConfig.acfmData.mmk[0] * vhreal * vhreal + liaConfig.acfmData.mmk[1] * vhreal + liaConfig.acfmData.mmk[2];
        const char* thickness = nullptr;
        if (mm < 0) {
            mm = 0;
        }
        if (mm <= 6) {
            thickness = std::format("{:5.2f}V:{:3.1f}mm", vhreal, mm).c_str();
        }
        else {
            thickness = std::format("{:5.2f}V:  out", vhreal).c_str();
        }
        ImPlot::PlotText(
            thickness,
            0.0,
            liaConfig.plotCfg.Vx_limt * 0.9
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
    ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline ACFMVhVvPlotWindow::ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "ACFM Vx-Vz"), liaConfig(liaConfig)
{
    this->windowPos = ImVec2(730 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.windowCfg.monitorScale, 650 * liaConfig.windowCfg.monitorScale);
}

inline void ACFMVhVvPlotWindow::show()
{
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver); //ImGui::GetIO().DisplaySize
    ImGui::Begin(this->name);
    // プロット描画
    if (ImPlot::BeginPlot("##Vx-Vz", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        ImPlot::SetupAxes("Vzpp (V)", "Vhpp (V)", 0, 0);
        if (liaConfig.plotCfg.limit <= MILI_VOLT)
        {
            ImPlot::SetupAxes("Vzpp (mV)", "Vxpp (mV)", 0, 0);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, liaConfig.plotCfg.Vz_limt*2, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, liaConfig.plotCfg.Vx_limt, ImGuiCond_Always);
        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.MarkerFillColor = colors[2];
        specScatter.LineWeight = -1.0f;
        specScatter.MarkerLineColor = colors[2];
        ImPlot::PlotScatter("References", liaConfig.acfmData.Vvs.data(), liaConfig.acfmData.Vhs.data(), liaConfig.acfmData.size, specScatter);
        specScatter.MarkerFillColor = colors[7];
        specScatter.MarkerLineColor = colors[7];
        ImPlot::PlotScatter("Result", &(liaConfig.acfmData.ch2vpp), &(liaConfig.acfmData.ch1vpp), 1, specScatter);
        ImPlot::EndPlot();
    }
    ImGui::End();
}
