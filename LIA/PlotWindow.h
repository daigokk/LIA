#pragma once

#include "ImGuiWindowBase.h"
#include "LiaConfig.h"

constexpr float MILI_VOLT = 0.2f; // 0.2Vより大きい時はV表示、以下はmV表示

const ImVec4 colors[] = {
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f), // Red
    ImVec4(0.0f, 1.0f, 0.0f, 1.0f), // Green
    ImVec4(0.0f, 0.0f, 1.0f, 1.0f), // Blue
    ImVec4(1.0f, 1.0f, 0.0f, 1.0f), // Yellow
    ImVec4(1.0f, 0.0f, 1.0f, 1.0f), // Magenta
    ImVec4(1.0f, 0.645f, 0.0f, 1.0f), // Orange
    ImVec4(234.0f / 255.0f, 162.0f / 255.0f, 34.0f / 255.0f, 1.0f), // Marigold
    ImVec4(251.0f / 255.0f, 189.0f / 255.0f, 4.0f / 255.0f, 1.0f),  // Amber
    ImVec4(176.0f / 255.0f, 252.0f / 255.0f, 56.0f / 255.0f, 1.0f), // Chartreuse
    ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // White
    ImVec4(0.0f, 0.0f, 0.0f, 1.0f)  // Black
};

// --- Formatters ---

void ScientificFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.1e", value);
}

void MiliFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e3);
}

void MicroFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e6);
}

// --- Helper Utilities ---

// リングバッファの折り返しを考慮して線を描画するヘルパー
auto plotRingBufferLine = [](const char* label, const std::vector<double>& x, const std::vector<double>& y, int startIdx, int size, const ImPlotSpec& spec) {
    int totalSize = static_cast<int>(x.size());
    if (startIdx + size > totalSize) {
        int firstPartSize = totalSize - startIdx;
        ImPlot::PlotLine(label, &x[startIdx], &y[startIdx], firstPartSize, spec);
        ImPlot::PlotLine(label, &x[0], &y[0], size - firstPartSize, spec);
    }
    else {
        ImPlot::PlotLine(label, &x[startIdx], &y[startIdx], size, spec);
    }
    };

// --- Windows ---

class RawPlotWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;
public:
    RawPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline RawPlotWindow::RawPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Raw waveform"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(450 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(430 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
}

inline void RawPlotWindow::show() {
    ButtonType button = ButtonType::NON;
    static float value = 0.0f;

    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);

    if (ImGui::Button("Save")) {
        liaConfig.saveRawData(std::format("raw_{}.csv", liaConfig.getCurrentTimestamp()));
    }
    if (ImGui::IsItemDeactivated()) {
        button = ButtonType::RawSave;
        value = 0.0f;
    }

    ImGui::SliderFloat("Y limit", &(liaConfig.plotCfg.rawLimit), 0.1f, RAW_RANGE * 1.2f, "%4.1f V");
    if (ImGui::IsItemDeactivated()) {
        button = ButtonType::RawLimit;
        value = liaConfig.plotCfg.rawLimit;
    }

    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1))) {
        bool useMv = liaConfig.plotCfg.rawLimit <= MILI_VOLT;
        ImPlot::SetupAxes("Time (us)", useMv ? "v (mV)" : "v (V)", 0, 0);
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));

        ImPlot::SetupAxisLimits(ImAxis_X1, liaConfig.rawTime.front(), liaConfig.rawTime.back(), ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.rawLimit, liaConfig.plotCfg.rawLimit, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);

        const char* ch1_label = liaConfig.flagCh2 ? "Ch1" : "##Ch1";
        ImPlot::PlotLine(ch1_label, liaConfig.rawTime.data(), liaConfig.rawData[0].data(), (int)liaConfig.rawTime.size(), specLine);

        if (liaConfig.flagCh2) {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            ImPlot::PlotLine("Ch2", liaConfig.rawTime.data(), liaConfig.rawData[1].data(), (int)liaConfig.rawTime.size(), specLine);
        }
        ImPlot::EndPlot();
    }
    ImGui::End();

    if (button != ButtonType::NON) {
        liaConfig.cmds.push_back({ (float)liaConfig.timer.elapsedSec(), (float)button, value, 0.0f, 0.0f, 0.0f });
    }
}


class TimeChartWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;
    void calculateXYPlotIndices(); // 複雑な計算を分離
public:
    TimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline TimeChartWindow::TimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Time chart"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(450 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(990 * liaConfig.windowCfg.monitorScale, 360 * liaConfig.windowCfg.monitorScale);
}

inline void TimeChartWindow::calculateXYPlotIndices() {
    double deltaTimeMin = LONG_MAX;
    double deltaTimeMax = LONG_MAX;
    int xyStartIdx = 0;
    int xyLatestIdx = liaConfig.ringBuffer.latestIdx;
    bool found = false;

    // 最新から逆順で検索
    for (int idx = liaConfig.ringBuffer.latestIdx; 0 <= idx; idx--) {
        double diffMin = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Min);
        double diffMax = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Max);

        if (deltaTimeMin > diffMin) { deltaTimeMin = diffMin; xyStartIdx = idx; }
        if (deltaTimeMax > diffMax) { deltaTimeMax = diffMax; xyLatestIdx = idx; }

        if (deltaTimeMin < MEASUREMENT_DT && deltaTimeMax < MEASUREMENT_DT) {
            found = true;
            break;
        }
    }

    // リングバッファの終端側を検索（ラップアラウンド時）
    if (!found && liaConfig.ringBuffer.size < liaConfig.ringBuffer.nofm) {
        for (int idx = liaConfig.ringBuffer.size - 1; liaConfig.ringBuffer.latestIdx < idx; idx--) {
            double diffMin = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Min);
            double diffMax = std::abs(liaConfig.ringBuffer.times[idx] - liaConfig.pauseCfg.selectArea.X.Max);

            if (deltaTimeMin > diffMin) { deltaTimeMin = diffMin; xyStartIdx = idx; }
            if (deltaTimeMax > diffMax) { deltaTimeMax = diffMax; xyLatestIdx = idx; }

            if (deltaTimeMin < MEASUREMENT_DT && deltaTimeMax < MEASUREMENT_DT) break;
        }
    }

    liaConfig.plotCfg.xyStartIdx = xyStartIdx;
    liaConfig.plotCfg.xyLatestIdx = xyLatestIdx;

    int xySize = xyLatestIdx - xyStartIdx;
    liaConfig.plotCfg.xySize = (xySize >= 0) ? xySize : (liaConfig.ringBuffer.size - xyStartIdx + xyLatestIdx);
}

inline void TimeChartWindow::show() {
    ButtonType button = ButtonType::NON;
    static float value = 0.0f;

    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);

    static float historySecMax = (float)(MEASUREMENT_DT) * (liaConfig.ringBuffer.times.size() - 1);

    if (liaConfig.pauseCfg.flag) ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(500.0f * liaConfig.windowCfg.monitorScale);
    ImGui::SliderFloat("History", &liaConfig.plotCfg.historySec, 1.0f, historySecMax, "%5.1f s");
    if (ImGui::IsItemDeactivated()) {
        button = ButtonType::TimeHistory;
        value = liaConfig.plotCfg.historySec;
    }
    if (liaConfig.pauseCfg.flag) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        liaConfig.saveResultsToFile(std::format("ect_{}.csv", liaConfig.getCurrentTimestamp()), liaConfig.plotCfg.historySec);
    }

    ImGui::SameLine();
    if (ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause")) { liaConfig.buttonPause(); }

    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0));

    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx];
        bool useMv = liaConfig.plotCfg.limit <= MILI_VOLT;

        ImPlot::SetupAxes("Time", useMv ? "v (mV)" : "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        if (!liaConfig.pauseCfg.flag) {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - liaConfig.plotCfg.historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.limit, liaConfig.plotCfg.limit, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.Offset = liaConfig.ringBuffer.writeIdx;
        int count = liaConfig.ringBuffer.size;

        ImPlot::PlotLine("Ch1y", &(liaConfig.ringBuffer.times[liaConfig.plotCfg.idxStart]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.idxStart]), count, specLine);
        if (liaConfig.flagCh2) {
            ImPlot::PlotLine("Ch2y", &(liaConfig.ringBuffer.times[0]), &(liaConfig.ringBuffer.ch[1].y[0]), count, specLine);
        }

        static bool isFirstPause = true;
        if (liaConfig.pauseCfg.flag) {
            if (isFirstPause) {
                liaConfig.pauseCfg.set(t - liaConfig.plotCfg.historySec * 0.2, t - liaConfig.plotCfg.historySec * 0.1, -liaConfig.plotCfg.limit * 0.9, liaConfig.plotCfg.limit * 0.9);
                isFirstPause = false;
            }

            bool clicked = false, hovered = false, held = false;
            ImPlot::DragRect(0, &liaConfig.pauseCfg.selectArea.X.Min, &liaConfig.pauseCfg.selectArea.Y.Min,
                &liaConfig.pauseCfg.selectArea.X.Max, &liaConfig.pauseCfg.selectArea.Y.Max,
                ImVec4(1, 0, 1, 1), ImPlotDragToolFlags_None, &clicked, &hovered, &held);

            calculateXYPlotIndices();
        }
        else {
            isFirstPause = true;
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();

    if (button != ButtonType::NON) {
        liaConfig.cmds.push_back({ (float)liaConfig.timer.elapsedSec(), (float)button, value, 0.0f, 0.0f, 0.0f });
    }
}


class TimeChartZoomWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;

    // 解析結果を保持する構造体（可読性向上のため）
    struct AnalysisResult {
        double ts_vx[2], vxs[2], t50s_vx[2], v50s_vx[2];
        double ts_vz[2], vzs[2], v50s_vz[2];
        int vminIdx_vx, vmaxIdx_vx;
        double x_min_last = -1.0, x_max_last = -1.0;
    } res;

    void analyzeSelection();

public:
    TimeChartZoomWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline TimeChartZoomWindow::TimeChartZoomWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Time chart zoom"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(0, 0); // monitorScaleは共通初期化で行う想定
    this->windowSize = ImVec2(750 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
}

// データの解析ロジックを分離
void TimeChartZoomWindow::analyzeSelection() {
    auto& area = liaConfig.pauseCfg.selectArea;
    if (res.x_min_last == area.X.Min && res.x_max_last == area.X.Max) return;

    res.x_min_last = area.X.Min;
    res.x_max_last = area.X.Max;

    // 初期化
    res.vxs[0] = 10.0; res.vxs[1] = -10.0;
    res.vzs[0] = 10.0; res.vzs[1] = -10.0;

    // 範囲内のMin/Max探索
    for (int i = 0; i < liaConfig.ringBuffer.size; i++) {
        double t = liaConfig.ringBuffer.times[i];
        if (t < area.X.Min) continue;
        if (t > area.X.Max) break;

        double v_x = liaConfig.ringBuffer.ch[CH_HORIZONTAL].y[i];
        if (res.vxs[0] > v_x) { res.vxs[0] = v_x; res.ts_vx[0] = t; res.vminIdx_vx = i; }
        if (res.vxs[1] < v_x) { res.vxs[1] = v_x; res.ts_vx[1] = t; res.vmaxIdx_vx = i; }

        double v_z = liaConfig.ringBuffer.ch[CH_VERTICAL].y[i];
        if (res.vzs[0] > v_z) { res.vzs[0] = v_z; res.ts_vz[0] = t; }
        if (res.vzs[1] < v_z) { res.vzs[1] = v_z; res.ts_vz[1] = t; }
    }

    // 50% レベルの計算
    res.v50s_vx[0] = res.v50s_vx[1] = (res.vxs[1] + res.vxs[0]) / 2.0;
    res.v50s_vz[0] = res.v50s_vz[1] = (res.vzs[1] + res.vzs[0]) / 2.0;

    // 50% を横切る時間の探索 (vx)
    auto& vx_y = liaConfig.ringBuffer.ch[CH_HORIZONTAL].y;
    auto& times = liaConfig.ringBuffer.times;

    // 前方探索
    for (int i = res.vminIdx_vx; i < liaConfig.ringBuffer.size && times[i] <= area.X.Max; i++) {
        if (res.v50s_vx[0] <= vx_y[i]) { res.t50s_vx[1] = times[i]; break; }
    }
    // 後方探索
    for (int i = res.vminIdx_vx; i >= 0 && times[i] >= area.X.Min; i--) {
        if (res.v50s_vx[0] <= vx_y[i]) { res.t50s_vx[0] = times[i]; break; }
    }

    liaConfig.acfmData.vxpp = std::abs(res.vxs[1] - res.vxs[0]);
    liaConfig.acfmData.vpp_vz = std::abs(res.vzs[1] - res.vzs[0]);
}

inline void TimeChartZoomWindow::show() {
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name);

    analyzeSelection(); // 解析実行

    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0));
    if (ImPlot::BeginPlot("##ZoomPlot", ImVec2(-1, -1))) {
        auto& area = liaConfig.pauseCfg.selectArea;
        bool useMv = liaConfig.plotCfg.limit <= MILI_VOLT;

        ImPlot::SetupAxes("Time (s)", useMv ? "v (mV)" : "v (V)", 0, 0);
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));

        ImPlot::SetupAxisLimits(ImAxis_X1, area.X.Min, area.X.Max, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, area.Y.Min, area.Y.Max, ImGuiCond_Always);

        // --- メイン波形のプロット ---
        ImPlotSpec specLine;
        specLine.Offset = liaConfig.ringBuffer.writeIdx;
        ImPlot::PlotLine("Ch1y", liaConfig.ringBuffer.times.data(), liaConfig.ringBuffer.ch[0].y.data(), liaConfig.ringBuffer.size, specLine);
        if (liaConfig.flagCh2) {
            ImPlot::PlotLine("Ch2y", liaConfig.ringBuffer.times.data(), liaConfig.ringBuffer.ch[1].y.data(), liaConfig.ringBuffer.size, specLine);
        }

        // --- 解析マーカーのプロット ---
        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.LineWeight = -1.0f;

        // Vx Vpp マーカーと 50% ライン
        if (liaConfig.flagCh2 && res.t50s_vx[0] < res.t50s_vx[1] && liaConfig.acfmData.vxpp > 2e-3) {
            specScatter.MarkerFillColor = colors[7]; // Amber
            ImPlot::PlotScatter("##Vx_pk", res.ts_vx, res.vxs, 2, specScatter);

            specScatter.MarkerFillColor = colors[9]; // White
            ImPlot::PlotScatter("##Vx_50%", res.t50s_vx, res.v50s_vx, 2, specScatter);

            specLine.LineColor = colors[9];
            ImPlot::PlotLine("##Vx_line", res.t50s_vx, res.v50s_vx, 2, specLine);

            // テキスト表示
            double y_offset = (area.Y.Max - area.Y.Min) * 0.05;
            ImPlot::PlotText(std::format("{:.3f}s", res.t50s_vx[1] - res.t50s_vx[0]).c_str(), (res.t50s_vx[1] + res.t50s_vx[0]) / 2, res.v50s_vx[0] - y_offset);
            ImPlot::PlotText(std::format("{} dV: {:.0f}mV", liaConfig.plotCfg.acfm ? "Vx" : "Ch1", liaConfig.acfmData.vxpp * 1e3).c_str(),
                area.X.Min + (area.X.Max - area.X.Min) * 0.5, area.Y.Min + (area.Y.Max - area.Y.Min) * 0.2);
        }

        // Vz についても同様に描画
        if (liaConfig.acfmData.vpp_vz > 10e-3) {
            specScatter.MarkerFillColor = colors[2]; // Blue
            ImPlot::PlotScatter("##Vz_pk", res.ts_vz, res.vzs, 2, specScatter);
            specScatter.MarkerFillColor = colors[9]; // White
            ImPlot::PlotScatter("##Vz_50% marker", res.ts_vz, res.v50s_vz, 2, specScatter);
            specLine.LineColor = colors[9]; // White
            ImPlot::PlotLine("##ch2 line", res.ts_vz, res.v50s_vz, 2, specLine);

            // テキスト表示
            ImPlot::PlotText(
                std::format("{:.3f}s", abs(res.ts_vz[1] - res.ts_vz[0])).c_str(),
                (res.ts_vz[1] + res.ts_vz[0]) / 2,
                res.v50s_vz[0] + (liaConfig.pauseCfg.selectArea.Y.Max - liaConfig.pauseCfg.selectArea.Y.Min) * 0.05
            );
            ImPlot::PlotText(std::format("{} dV: {:.0f}mV", liaConfig.plotCfg.acfm ? "Vz" : "Ch2", liaConfig.acfmData.vpp_vz * 1e3).c_str(),
                area.X.Min + (area.X.Max - area.X.Min) * 0.5, area.Y.Min + (area.Y.Max - area.Y.Min) * 0.1);
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
}


class DeltaTimeChartWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;
public:
    DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline DeltaTimeChartWindow::DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "DeltTime chart"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(0 * liaConfig.windowCfg.monitorScale, 750 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(450 * liaConfig.windowCfg.monitorScale, 210 * liaConfig.windowCfg.monitorScale);
}

inline void DeltaTimeChartWindow::show() {
    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);

    static float historySecMax = (float)(MEASUREMENT_DT)*liaConfig.ringBuffer.times.size();
    static float historySec = 10.0f;
    ImGui::SliderFloat("History", &historySec, 1.0f, historySecMax, "%5.1f s");

    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1))) {
        double t = liaConfig.ringBuffer.times[liaConfig.ringBuffer.latestIdx];
        ImPlot::SetupAxes("Time", "dt (ms)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, (MEASUREMENT_DT - 2e-3) * 1e3, (MEASUREMENT_DT + 2e-3) * 1e3, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.Offset = liaConfig.ringBuffer.writeIdx;
        ImPlot::PlotLine("##dt", &(liaConfig.ringBuffer.times[0]), &(liaConfig.deltaTimes[0]), liaConfig.ringBuffer.size, specLine);
        ImPlot::EndPlot();
    }
    ImGui::End();
}


class XYPlotWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;
public:
    XYPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline XYPlotWindow::XYPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "XY"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(880 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.windowCfg.monitorScale, 600 * liaConfig.windowCfg.monitorScale);
}

inline void XYPlotWindow::show() {
    if (liaConfig.plotCfg.surfaceMode) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    }

    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);

    if (ImGui::Button("Clear")) { liaConfig.buttonClear(); }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) { liaConfig.buttonAutoOffset(); }
    ImGui::SameLine();
    if (ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause")) { liaConfig.buttonPause(); }
    ImGui::SameLine();
    if (ImGui::Button("Rec.")) { liaConfig.buttonRec(); }

    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0));

    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        bool useMv = liaConfig.plotCfg.limit <= MILI_VOLT;
        ImPlot::SetupAxes(useMv ? "x (mV)" : "x (V)", useMv ? "y (mV)" : "y (V)", 0, 0);

        if (useMv) {
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }

        if (liaConfig.plotCfg.surfaceMode) {
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

        // ヘルパーを使用して描画を簡略化
        plotRingBufferLine(ch1_label, liaConfig.ringBuffer.ch[0].x, liaConfig.ringBuffer.ch[0].y,
            liaConfig.plotCfg.xyStartIdx, liaConfig.plotCfg.xySize, specLine);

        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.MarkerFillColor = colors[2];
        specScatter.LineWeight = 2.0f;
        specScatter.MarkerLineColor = colors[2];

        ImPlot::PlotScatter("##NOW1", &(liaConfig.ringBuffer.ch[0].x[liaConfig.plotCfg.xyLatestIdx]), &(liaConfig.ringBuffer.ch[0].y[liaConfig.plotCfg.xyLatestIdx]), 1, specScatter);

        specScatter.MarkerFillColor = ImVec4(0, 0, 0, 0);
        ImPlot::PlotScatter("##REC1", liaConfig.xyRecs.ch1xys.x.data(), liaConfig.xyRecs.ch1xys.y.data(), (int)liaConfig.xyRecs.ch1xys.x.size(), specScatter);

        if (liaConfig.flagCh2) {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            plotRingBufferLine("Ch2", liaConfig.ringBuffer.ch[1].x, liaConfig.ringBuffer.ch[1].y,
                liaConfig.plotCfg.xyStartIdx, liaConfig.plotCfg.xySize, specLine);

            specScatter.MarkerFillColor = colors[7];
            specScatter.MarkerLineColor = colors[7];
            ImPlot::PlotScatter("##NOW2", &(liaConfig.ringBuffer.ch[1].x[liaConfig.plotCfg.xyLatestIdx]), &(liaConfig.ringBuffer.ch[1].y[liaConfig.plotCfg.xyLatestIdx]), 1, specScatter);

            specScatter.MarkerFillColor = ImVec4(0, 0, 0, 0);
            ImPlot::PlotScatter("##REC2", liaConfig.xyRecs.ch2xys.x.data(), liaConfig.xyRecs.ch2xys.y.data(), (int)liaConfig.xyRecs.ch2xys.x.size(), specScatter);
        }

        if (liaConfig.flagAutoSetupW2History) {
            specLine.LineColor = ImPlot::GetColormapColor(2, ImPlotColormap_Deep);
            ImPlot::PlotLine("##HISTORY W1", liaConfig.autoSetupHistoryW1.x.data(), liaConfig.autoSetupHistoryW1.y.data(), liaConfig.autoSetupHistoryW1.x.size(), specLine);
            specLine.LineColor = ImPlot::GetColormapColor(3, ImPlotColormap_Deep);
            ImPlot::PlotLine("##HISTORY W2", liaConfig.autoSetupHistoryW2.x.data(), liaConfig.autoSetupHistoryW2.y.data(), liaConfig.autoSetupHistoryW2.x.size(), specLine);
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();

    if (liaConfig.plotCfg.surfaceMode) ImGui::PopStyleColor();
}


class ACFMPlotWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;
public:
    ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline ACFMPlotWindow::ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "ACFM"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(880 * liaConfig.windowCfg.monitorScale, 40 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.windowCfg.monitorScale, 560 * liaConfig.windowCfg.monitorScale);
}

inline void ACFMPlotWindow::show() {
    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);

    if (ImGui::Button("Clear")) { liaConfig.buttonClear(); }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) { liaConfig.buttonAutoOffset(); }
    ImGui::SameLine();
    if (ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause")) { liaConfig.buttonPause(); }

    if (ImGui::SliderFloat("Vz limit", &(liaConfig.plotCfg.Vz_limt), 0.01f, RAW_RANGE * 1.2f, "%4.2f V")) {
        liaConfig.plotCfg.Vx_limt = liaConfig.plotCfg.Vz_limt / 2;
    }
    if (ImGui::SliderFloat("Vx limit", &(liaConfig.plotCfg.Vx_limt), 0.01f, RAW_RANGE * 1.2f, "%4.2f V")) {
        liaConfig.plotCfg.limit = liaConfig.plotCfg.Vz_limt;
    }
    
    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        bool useMv = liaConfig.plotCfg.limit <= MILI_VOLT;
        ImPlot::SetupAxes(useMv ? "Vz (mV)" : "Vz (V)", useMv ? "Vx (mV)" : "Vx (V)", 0, 0);

        if (useMv) {
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }

        ImPlot::SetupAxisLimits(ImAxis_X1, -liaConfig.plotCfg.Vz_limt, liaConfig.plotCfg.Vz_limt, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -liaConfig.plotCfg.Vx_limt, liaConfig.plotCfg.Vx_limt, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(2, ImPlotColormap_Deep);

        // ヘルパーを使用して描画を簡略化
        plotRingBufferLine("##ACFM", liaConfig.ringBuffer.ch[CH_VERTICAL].y, liaConfig.ringBuffer.ch[CH_HORIZONTAL].y,
            liaConfig.plotCfg.xyStartIdx, liaConfig.plotCfg.xySize, specLine);

        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.MarkerFillColor = colors[8];
        specScatter.LineWeight = -1.0f;
        specScatter.MarkerLineColor = colors[8];

        ImPlot::PlotScatter("##NOW", &(liaConfig.ringBuffer.ch[CH_VERTICAL].y[liaConfig.plotCfg.xyLatestIdx]), &(liaConfig.ringBuffer.ch[CH_HORIZONTAL].y[liaConfig.plotCfg.xyLatestIdx]), 1, specScatter);

        double vhreal = liaConfig.ringBuffer.ch[CH_HORIZONTAL].x[liaConfig.plotCfg.xyLatestIdx];
        double mm = std::max(0.0, liaConfig.acfmData.mmk[0] * vhreal * vhreal + liaConfig.acfmData.mmk[1] * vhreal + liaConfig.acfmData.mmk[2]);

        std::string thicknessStr = (mm <= 6.0) ? std::format("{:5.2f}V:{:3.1f}mm", vhreal, mm) : std::format("{:5.2f}V:  out", vhreal);
        ImPlot::PlotText(thicknessStr.c_str(), 0.0, liaConfig.plotCfg.Vx_limt * 0.9);

        ImPlot::EndPlot();
    }
    ImGui::End();
}

class ACFMVhVvPlotWindow : public ImGuiWindowBase {
private:
    LiaConfig& liaConfig;
public:
    ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show(void);
};

inline ACFMVhVvPlotWindow::ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "ACFM Vx-Vz"), liaConfig(liaConfig) {
    this->windowPos = ImVec2(730 * liaConfig.windowCfg.monitorScale, 0 * liaConfig.windowCfg.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.windowCfg.monitorScale, 650 * liaConfig.windowCfg.monitorScale);
}

inline void ACFMVhVvPlotWindow::show() {
    ImGui::SetNextWindowPos(windowPos, liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
    ImGui::Begin(this->name);

    if (ImPlot::BeginPlot("##Vx-Vz", ImVec2(-1, -1), ImPlotFlags_Equal)) {
        bool useMv = liaConfig.plotCfg.limit <= MILI_VOLT;

        // 軸のセットアップ
        ImPlot::SetupAxes(useMv ? "Vzpp (mV)" : "Vzpp (V)", useMv ? "Vxpp (mV)" : "Vhpp (V)", 0, 0);
        if (useMv) {
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }

        ImPlot::SetupAxisLimits(ImAxis_X1, 0, liaConfig.plotCfg.Vz_limt * 2.0f, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, liaConfig.plotCfg.Vx_limt, ImGuiCond_Always);

        // 散布図の共通スタイル設定
        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * liaConfig.windowCfg.monitorScale;
        specScatter.LineWeight = -1.0f;

        // References (青色系)
        specScatter.MarkerFillColor = colors[2];
        specScatter.MarkerLineColor = colors[2];
        ImPlot::PlotScatter("References", liaConfig.acfmData.Vvs.data(), liaConfig.acfmData.Vhs.data(), liaConfig.acfmData.size, specScatter);

        // Result (アンバー系)
        specScatter.MarkerFillColor = colors[7];
        specScatter.MarkerLineColor = colors[7];
        ImPlot::PlotScatter("Result", &(liaConfig.acfmData.vpp_vz), &(liaConfig.acfmData.vxpp), 1, specScatter);

        ImPlot::EndPlot();
    }
    ImGui::End();
}
