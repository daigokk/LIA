#pragma once

#include "ImGuiWindowBase.h"
#include "LiaConfig.h"

// ============================================================================
// Constants & Enums
// ============================================================================

constexpr float MILI_VOLT = 0.2f; // 0.2Vより大きい時はV表示、以下はmV表示

// 色インデックスのマジックナンバーを排除するための列挙型
enum PaletteColor {
    Color_Red = 0,
    Color_Green,
    Color_Blue,
    Color_Yellow,
    Color_Magenta,
    Color_Orange,
    Color_Marigold,
    Color_Amber,
    Color_Chartreuse,
    Color_White,
    Color_Black
};

const ImVec4 colors[] = {
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f), // 0: Red
    ImVec4(0.0f, 1.0f, 0.0f, 1.0f), // 1: Green
    ImVec4(0.0f, 0.0f, 1.0f, 1.0f), // 2: Blue
    ImVec4(1.0f, 1.0f, 0.0f, 1.0f), // 3: Yellow
    ImVec4(1.0f, 0.0f, 1.0f, 1.0f), // 4: Magenta
    ImVec4(1.0f, 0.645f, 0.0f, 1.0f), // 5: Orange
    ImVec4(234.0f / 255.0f, 162.0f / 255.0f, 34.0f / 255.0f, 1.0f), // 6: Marigold
    ImVec4(251.0f / 255.0f, 189.0f / 255.0f, 4.0f / 255.0f, 1.0f),  // 7: Amber
    ImVec4(176.0f / 255.0f, 252.0f / 255.0f, 56.0f / 255.0f, 1.0f), // 8: Chartreuse
    ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // 9: White
    ImVec4(0.0f, 0.0f, 0.0f, 1.0f)  // 10: Black
};

// ============================================================================
// Formatters & Helper Utilities
// ============================================================================

inline void ScientificFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.1e", value);
}

inline void KiloFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e-3);
}

inline void MiliFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e3);
}

inline void MicroFormatter(double value, char* buff, int size, void*) {
    snprintf(buff, size, "%.0f", value * 1e6);
}

// リングバッファの折り返しを考慮して線を描画するヘルパー
inline auto plotRingBufferLine = [](const char* label, const std::vector<double>& x, const std::vector<double>& y, int startIdx, int size, const ImPlotSpec& spec) {
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


// ============================================================================
// Class Declarations (クラス宣言部)
// ============================================================================

class RawPlotWindow : public ImGuiWindowBase {
public:
    RawPlotWindow(GLFWwindow* window, LiaConfig& cfg);
    void show();

private:
    LiaConfig& cfg;

    // 可読性向上のため、複雑なタブ描画処理を分離
    void drawWaveformTab(bool useMv);
    void drawFftTab(bool useMv);
};


class TimeChartWindow : public ImGuiWindowBase {
public:
    TimeChartWindow(GLFWwindow* window, LiaConfig& cfg);
    void show();

private:
    LiaConfig& cfg;
    void calculateXYPlotIndices();
};


class TimeChartZoomWindow : public ImGuiWindowBase {
public:
    TimeChartZoomWindow(GLFWwindow* window, LiaConfig& cfg);
    void show();

private:
    LiaConfig& cfg;

    struct AnalysisResult {
        double ts_vx[2], vxs[2], t50s_vx[2], v50s_vx[2];
        double ts_vz[2], vzs[2], v50s_vz[2];
        int vminIdx_vx, vmaxIdx_vx;
        double x_min_last = -1.0, x_max_last = -1.0;
    } res;

    void analyzeSelection();
};


class DeltaTimeChartWindow : public ImGuiWindowBase {
public:
    DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& cfg);
    void show();

private:
    LiaConfig& cfg;
};


class XYPlotWindow : public ImGuiWindowBase {
public:
    XYPlotWindow(GLFWwindow* window, LiaConfig& cfg);
    void show();

private:
    LiaConfig& cfg;
};


class ACFMPlotWindow : public ImGuiWindowBase {
public:
    ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig);
    void show();

private:
    LiaConfig& cfg;
};


class ACFMVhVvPlotWindow : public ImGuiWindowBase {
public:
    ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& cfg);
    void show();

private:
    LiaConfig& cfg;
};


// ============================================================================
// Class Implementations (クラス実装部)
// ============================================================================

// ----------------------------------------------------------------------------
// RawPlotWindow
// ----------------------------------------------------------------------------
inline RawPlotWindow::RawPlotWindow(GLFWwindow* window, LiaConfig& cfg)
    : ImGuiWindowBase(window, "Raw waveform"), cfg(cfg) {
    this->windowPos = ImVec2(0 * cfg.window.monitorScale, 37 * cfg.window.monitorScale);
    this->windowSize = ImVec2(440 * cfg.window.monitorScale, 600 * cfg.window.monitorScale);
}

inline void RawPlotWindow::show() {
    ButtonType button = ButtonType::NON;
    static float value = 0.0f;

    ImGui::SetNextWindowPos(windowPos, cfg.window.imGuiCondFlag);
    ImGui::SetNextWindowSize(windowSize, cfg.window.imGuiCondFlag);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    // Header Controls
    if (ImGui::Button("Save")) {
		std::string timestamp = cfg.getCurrentTimestamp();
        cfg.saveRawData(std::format("raw_{}.csv", timestamp));
        cfg.raw.calculateFFT(cfg.isCh2Enabled, cfg.awg.ch[0].freq, LiaConfigConst::RAW_DT);
		cfg.saveFftData(std::format("fft_{}.csv", timestamp));
        button = ButtonType::RawSave;
		value = 1.0f; // Saveボタンが押されたことを示すフラグ
    }
    if (ImGui::IsItemDeactivated()) {
        button = ButtonType::RawSave;
        value = 0.0f;
    }

    ImGui::SliderFloat("Y limit", &(cfg.plot.rawLimit), 0.1f, cfg.scope.ch[0].range * 1.2f, "%4.1f V");
    if (ImGui::IsItemDeactivated()) {
        button = ButtonType::RawLimit;
        value = cfg.plot.rawLimit;
    }

    // Tabs
    if (ImGui::BeginTabBar("Raw")) {
        bool useMv = cfg.plot.rawLimit <= MILI_VOLT;

        if (ImGui::BeginTabItem("Waveform")) {
            drawWaveformTab(useMv);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("FFT")) {
            drawFftTab(useMv);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    // Command Logging
    if (button != ButtonType::NON) {
        cfg.cmds.push_back({ (float)cfg.timer.elapsedSec(), (float)button, value, 0.0f, 0.0f, 0.0f });
    }
}

inline void RawPlotWindow::drawWaveformTab(bool useMv) {
    if (ImPlot::BeginPlot("##Raw waveform", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        ImPlot::SetupAxes("Time (us)", useMv ? "v (mV)" : "v (V)", 0, 0);
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));

        ImPlot::SetupAxisLimits(ImAxis_X1, cfg.raw.times.front(), cfg.raw.times.back(), ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -cfg.plot.rawLimit, cfg.plot.rawLimit, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);

        const char* ch1_label = cfg.isCh2Enabled ? "Ch1" : "##Ch1";
        ImPlot::PlotLine(ch1_label, cfg.raw.times.data(), cfg.raw.waveform[0].data(), (int)cfg.raw.times.size(), specLine);

        if (cfg.isCh2Enabled) {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            ImPlot::PlotLine("Ch2", cfg.raw.times.data(), cfg.raw.waveform[1].data(), (int)cfg.raw.times.size(), specLine);
        }
        ImPlot::EndPlot();
    }
}

inline void RawPlotWindow::drawFftTab(bool useMv) {
    if (ImPlot::BeginPlot("##FFT", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        cfg.raw.calculateFFT(cfg.isCh2Enabled, cfg.awg.ch[0].freq, LiaConfigConst::RAW_DT); // FFT計算をここで行うことで、波形とスペクトルの表示が常に同期するようにする
        ImPlot::SetupAxes("Freq. (kHz)", useMv ? "v (mV)" : "v (V)", 0, 0);
        ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(KiloFormatter));
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, cfg.raw.freqs[cfg.raw.freqs.size() / 80], ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, cfg.plot.rawLimit, ImGuiCond_Always);

        ImPlotSpec specLine;
		specLine.LineWeight = 5.0f;
        specLine.LineColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);

        const char* ch1_label = cfg.isCh2Enabled ? "Ch1" : "##Ch1";
		ImPlot::PlotLine(ch1_label, cfg.raw.freqs.data(), cfg.raw.fftAbs[0].data(), (int)cfg.raw.freqs.size() / 80, specLine);

        // Ch2 FFT Calculation & Plot
        if (cfg.isCh2Enabled) {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            ImPlot::PlotLine("Ch2", cfg.raw.freqs.data(), cfg.raw.fftAbs[1].data(), (int)cfg.raw.freqs.size() / 80, specLine);
        }

        ImPlot::EndPlot();
    }
}


// ----------------------------------------------------------------------------
// TimeChartWindow
// ----------------------------------------------------------------------------
inline TimeChartWindow::TimeChartWindow(GLFWwindow* window, LiaConfig& cfg)
    : ImGuiWindowBase(window, "Time chart"), cfg(cfg) {
    this->windowPos = ImVec2(0 * cfg.window.monitorScale, 637 * cfg.window.monitorScale);
    this->windowSize = ImVec2(1000 * cfg.window.monitorScale, 323 * cfg.window.monitorScale);
}

inline void TimeChartWindow::calculateXYPlotIndices() {
    double deltaTimeMin = LONG_MAX;
    double deltaTimeMax = LONG_MAX;
    int xyStartIdx = 0;
    int xyLatestIdx = cfg.ringBuffer.latestIdx;
    bool found = false;

    // 最新から逆順で検索
    for (int idx = cfg.ringBuffer.latestIdx; 0 <= idx; idx--) {
        double diffMin = std::abs(cfg.ringBuffer.times[idx] - cfg.pause.selectArea.X.Min);
        double diffMax = std::abs(cfg.ringBuffer.times[idx] - cfg.pause.selectArea.X.Max);

        if (deltaTimeMin > diffMin) { deltaTimeMin = diffMin; xyStartIdx = idx; }
        if (deltaTimeMax > diffMax) { deltaTimeMax = diffMax; xyLatestIdx = idx; }

        if (deltaTimeMin < LiaConfigConst::MEASUREMENT_DT && deltaTimeMax < LiaConfigConst::MEASUREMENT_DT) {
            found = true;
            break;
        }
    }

    // リングバッファの終端側を検索（ラップアラウンド時）
    if (!found && cfg.ringBuffer.size < cfg.ringBuffer.nofm) {
        for (int idx = cfg.ringBuffer.size - 1; cfg.ringBuffer.latestIdx < idx; idx--) {
            double diffMin = std::abs(cfg.ringBuffer.times[idx] - cfg.pause.selectArea.X.Min);
            double diffMax = std::abs(cfg.ringBuffer.times[idx] - cfg.pause.selectArea.X.Max);

            if (deltaTimeMin > diffMin) { deltaTimeMin = diffMin; xyStartIdx = idx; }
            if (deltaTimeMax > diffMax) { deltaTimeMax = diffMax; xyLatestIdx = idx; }

            if (deltaTimeMin < LiaConfigConst::MEASUREMENT_DT && deltaTimeMax < LiaConfigConst::MEASUREMENT_DT) break;
        }
    }

    cfg.plot.xyStartIdx = xyStartIdx;
    cfg.plot.xyLatestIdx = xyLatestIdx;

    int xySize = xyLatestIdx - xyStartIdx;
    cfg.plot.xySize = (xySize >= 0) ? xySize : (cfg.ringBuffer.size - xyStartIdx + xyLatestIdx);
}

inline void TimeChartWindow::show() {
    ButtonType button = ButtonType::NON;
    static float value = 0.0f;

    ImGui::SetNextWindowPos(windowPos, cfg.window.imGuiCondFlag);
    ImGui::SetNextWindowSize(windowSize, cfg.window.imGuiCondFlag);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    static float historySecMax = (float)(LiaConfigConst::MEASUREMENT_DT) * (cfg.ringBuffer.times.size() - 1);

    if (cfg.pause.flag) ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(500.0f * cfg.window.monitorScale);
    ImGui::SliderFloat("History", &cfg.plot.historySec, 1.0f, historySecMax, "%5.1f s");
    if (ImGui::IsItemDeactivated()) {
        button = ButtonType::TimeHistory;
        value = cfg.plot.historySec;
    }
    if (cfg.pause.flag) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        cfg.saveResultsToFile(std::format("ect_{}.csv", cfg.getCurrentTimestamp()), cfg.plot.historySec);
    }

    ImGui::SameLine();
    if (ImGui::Button(cfg.pause.flag ? "Run" : "Pause")) { cfg.buttonPause(); }

    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0));

    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        double t = cfg.ringBuffer.times[cfg.ringBuffer.latestIdx];
        bool useMv = cfg.plot.limit <= MILI_VOLT;

        ImPlot::SetupAxes("Time", useMv ? "v (mV)" : "v (V)", ImPlotAxisFlags_NoTickLabels, 0);
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        if (!cfg.pause.flag) {
            ImPlot::SetupAxisLimits(ImAxis_X1, t - cfg.plot.historySec, t, ImGuiCond_Always);
        }
        ImPlot::SetupAxisLimits(ImAxis_Y1, -cfg.plot.limit, cfg.plot.limit, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.Offset = (int)cfg.ringBuffer.writeIdx;
        int count = (int)cfg.ringBuffer.size;

        ImPlot::PlotLine("Ch1y", &(cfg.ringBuffer.times[0]), &(cfg.ringBuffer.ch[0].y[0]), count, specLine);
        if (cfg.isCh2Enabled) {
            ImPlot::PlotLine("Ch2y", &(cfg.ringBuffer.times[0]), &(cfg.ringBuffer.ch[1].y[0]), count, specLine);
        }

        static bool isFirstPause = true;
        if (cfg.pause.flag) {
            if (isFirstPause) {
                cfg.pause.set(t - cfg.plot.historySec * 0.2, t - cfg.plot.historySec * 0.1, -cfg.plot.limit * 0.9, cfg.plot.limit * 0.9);
                isFirstPause = false;
            }

            bool clicked = false, hovered = false, held = false;
            ImPlot::DragRect(0, &cfg.pause.selectArea.X.Min, &cfg.pause.selectArea.Y.Min,
                &cfg.pause.selectArea.X.Max, &cfg.pause.selectArea.Y.Max,
                colors[Color_Magenta], ImPlotDragToolFlags_None, &clicked, &hovered, &held);

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
        cfg.cmds.push_back({ (float)cfg.timer.elapsedSec(), (float)button, value, 0.0f, 0.0f, 0.0f });
    }
}


// ----------------------------------------------------------------------------
// TimeChartZoomWindow
// ----------------------------------------------------------------------------
inline TimeChartZoomWindow::TimeChartZoomWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "Time chart zoom"), cfg(liaConfig) {
    this->windowPos = ImVec2(0 * liaConfig.window.monitorScale, 37 * 2 * liaConfig.window.monitorScale);
    this->windowSize = ImVec2(440 * liaConfig.window.monitorScale, 563 * liaConfig.window.monitorScale);
}

inline void TimeChartZoomWindow::analyzeSelection() {
    auto& area = cfg.pause.selectArea;
    if (res.x_min_last == area.X.Min && res.x_max_last == area.X.Max) return;

    res.x_min_last = area.X.Min;
    res.x_max_last = area.X.Max;

    // 初期化
    res.vxs[0] = 10.0; res.vxs[1] = -10.0;
    res.vzs[0] = 10.0; res.vzs[1] = -10.0;

    // 範囲内のMin/Max探索
    for (int i = 0; i < cfg.ringBuffer.size; i++) {
        double t = cfg.ringBuffer.times[i];
        if (t < area.X.Min) continue;
        if (t > area.X.Max) break;

        double v_x = cfg.ringBuffer.ch[LiaConfigConst::CH_HORIZONTAL].y[i];
        if (res.vxs[0] > v_x) { res.vxs[0] = v_x; res.ts_vx[0] = t; res.vminIdx_vx = i; }
        if (res.vxs[1] < v_x) { res.vxs[1] = v_x; res.ts_vx[1] = t; res.vmaxIdx_vx = i; }

        double v_z = cfg.ringBuffer.ch[LiaConfigConst::CH_VERTICAL].y[i];
        if (res.vzs[0] > v_z) { res.vzs[0] = v_z; res.ts_vz[0] = t; }
        if (res.vzs[1] < v_z) { res.vzs[1] = v_z; res.ts_vz[1] = t; }
    }

    // 50% レベルの計算
    res.v50s_vx[0] = res.v50s_vx[1] = (res.vxs[1] + res.vxs[0]) / 2.0;
    res.v50s_vz[0] = res.v50s_vz[1] = (res.vzs[1] + res.vzs[0]) / 2.0;

    // 50% を横切る時間の探索 (vx)
    auto& vx_y = cfg.ringBuffer.ch[LiaConfigConst::CH_HORIZONTAL].y;
    auto& times = cfg.ringBuffer.times;

    // 前方探索
    for (int i = res.vminIdx_vx; i < cfg.ringBuffer.size && times[i] <= area.X.Max; i++) {
        if (res.v50s_vx[0] <= vx_y[i]) { res.t50s_vx[1] = times[i]; break; }
    }
    // 後方探索
    for (int i = res.vminIdx_vx; i >= 0 && times[i] >= area.X.Min; i--) {
        if (res.v50s_vx[0] <= vx_y[i]) { res.t50s_vx[0] = times[i]; break; }
    }

    cfg.acfmData.vxpp = std::abs(res.vxs[1] - res.vxs[0]);
    cfg.acfmData.vpp_vz = std::abs(res.vzs[1] - res.vzs[0]);
}

inline void TimeChartZoomWindow::show() {
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    analyzeSelection();

    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0));
    if (ImPlot::BeginPlot("##ZoomPlot", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        auto& area = cfg.pause.selectArea;
        bool useMv = cfg.plot.limit <= MILI_VOLT;

        ImPlot::SetupAxes("Time (s)", useMv ? "v (mV)" : "v (V)", 0, 0);
        if (useMv) ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));

        ImPlot::SetupAxisLimits(ImAxis_X1, area.X.Min, area.X.Max, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, area.Y.Min, area.Y.Max, ImGuiCond_Always);

        // --- メイン波形のプロット ---
        ImPlotSpec specLine;
        specLine.Offset = cfg.ringBuffer.writeIdx;
        ImPlot::PlotLine("Ch1y", cfg.ringBuffer.times.data(), cfg.ringBuffer.ch[0].y.data(), cfg.ringBuffer.size, specLine);
        if (cfg.isCh2Enabled) {
            ImPlot::PlotLine("Ch2y", cfg.ringBuffer.times.data(), cfg.ringBuffer.ch[1].y.data(), cfg.ringBuffer.size, specLine);
        }

        // --- 解析マーカーのプロット ---
        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * cfg.window.monitorScale;
        specScatter.LineWeight = -1.0f;

        // Vx Vpp マーカーと 50% ライン
        if (cfg.isCh2Enabled && res.t50s_vx[0] < res.t50s_vx[1] && cfg.acfmData.vxpp > 2e-3) {
            specScatter.MarkerFillColor = colors[Color_Amber];
            ImPlot::PlotScatter("##Vx_pk", res.ts_vx, res.vxs, 2, specScatter);

            specScatter.MarkerFillColor = colors[Color_White];
            ImPlot::PlotScatter("##Vx_50%", res.t50s_vx, res.v50s_vx, 2, specScatter);

            specLine.LineColor = colors[Color_White];
            ImPlot::PlotLine("##Vx_line", res.t50s_vx, res.v50s_vx, 2, specLine);

            // テキスト表示
            double y_offset = (area.Y.Max - area.Y.Min) * 0.05;
            ImPlot::PlotText(std::format("{:.3f}s", res.t50s_vx[1] - res.t50s_vx[0]).c_str(), (res.t50s_vx[1] + res.t50s_vx[0]) / 2, res.v50s_vx[0] - y_offset);
            ImPlot::PlotText(std::format("{} dV: {:.0f}mV", cfg.window.acfmWindow ? "Vx" : "Ch1", cfg.acfmData.vxpp * 1e3).c_str(),
                area.X.Min + (area.X.Max - area.X.Min) * 0.5, area.Y.Min + (area.Y.Max - area.Y.Min) * 0.2);
        }

        // Vz についても同様に描画
        if (cfg.acfmData.vpp_vz > 10e-3) {
            specScatter.MarkerFillColor = colors[Color_Blue];
            ImPlot::PlotScatter("##Vz_pk", res.ts_vz, res.vzs, 2, specScatter);
            specScatter.MarkerFillColor = colors[Color_White];
            ImPlot::PlotScatter("##Vz_50% marker", res.ts_vz, res.v50s_vz, 2, specScatter);
            specLine.LineColor = colors[Color_White];
            ImPlot::PlotLine("##ch2 line", res.ts_vz, res.v50s_vz, 2, specLine);

            // テキスト表示
            ImPlot::PlotText(
                std::format("{:.3f}s", abs(res.ts_vz[1] - res.ts_vz[0])).c_str(),
                (res.ts_vz[1] + res.ts_vz[0]) / 2,
                res.v50s_vz[0] + (cfg.pause.selectArea.Y.Max - cfg.pause.selectArea.Y.Min) * 0.05
            );
            ImPlot::PlotText(std::format("{} dV: {:.0f}mV", cfg.window.acfmWindow ? "Vz" : "Ch2", cfg.acfmData.vpp_vz * 1e3).c_str(),
                area.X.Min + (area.X.Max - area.X.Min) * 0.5, area.Y.Min + (area.Y.Max - area.Y.Min) * 0.1);
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();
}


// ----------------------------------------------------------------------------
// DeltaTimeChartWindow
// ----------------------------------------------------------------------------
inline DeltaTimeChartWindow::DeltaTimeChartWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "DeltTime chart"), cfg(liaConfig) {
    this->windowPos = ImVec2(1000 * liaConfig.window.monitorScale, 750 * liaConfig.window.monitorScale);
    this->windowSize = ImVec2(450 * liaConfig.window.monitorScale, 210 * liaConfig.window.monitorScale);
}

inline void DeltaTimeChartWindow::show() {
    ImGui::SetNextWindowPos(windowPos, cfg.window.imGuiCondFlag);
    ImGui::SetNextWindowSize(windowSize, cfg.window.imGuiCondFlag);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    static float historySecMax = (float)(LiaConfigConst::MEASUREMENT_DT)*cfg.ringBuffer.times.size();
    static float historySec = 10.0f;
    ImGui::SliderFloat("History", &historySec, 1.0f, historySecMax, "%5.1f s");

    if (ImPlot::BeginPlot("##Time chart", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        double t = cfg.ringBuffer.times[cfg.ringBuffer.latestIdx];
        ImPlot::SetupAxes("Time", "dt (ms)", ImPlotAxisFlags_NoTickLabels, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - historySec, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, (LiaConfigConst::MEASUREMENT_DT - 2e-3) * 1e3, (LiaConfigConst::MEASUREMENT_DT + 2e-3) * 1e3, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.Offset = cfg.ringBuffer.writeIdx;
        ImPlot::PlotLine("##dt", &(cfg.ringBuffer.times[0]), &(cfg.ringBuffer.deltaTimes[0]), cfg.ringBuffer.size, specLine);
        ImPlot::EndPlot();
    }
    ImGui::End();
}


// ----------------------------------------------------------------------------
// XYPlotWindow
// ----------------------------------------------------------------------------
inline XYPlotWindow::XYPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "XY"), cfg(liaConfig) {
    this->windowPos = ImVec2(440 * liaConfig.window.monitorScale, 37 * liaConfig.window.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 600 * liaConfig.window.monitorScale);
}

inline void XYPlotWindow::show() {
    if (cfg.plot.surfaceMode) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
    }

    ImGui::SetNextWindowPos(windowPos, cfg.window.imGuiCondFlag);
    ImGui::SetNextWindowSize(windowSize, cfg.window.imGuiCondFlag);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    // Toolbar Controls
    if (ImGui::Button("Clear")) { cfg.buttonClear(); }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) { cfg.buttonAutoOffset(); }
    ImGui::SameLine();
    if (ImGui::Button(cfg.pause.flag ? "Run" : "Pause")) { cfg.buttonPause(); }
    ImGui::SameLine();
    if (ImGui::Button("Rec.")) { cfg.buttonRec(); }

    ImPlot::PushStyleColor(ImPlotCol_LegendBg, ImVec4(0, 0, 0, 0));

    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal | cfg.window.imPlotFlag)) {
        bool useMv = cfg.plot.limit <= MILI_VOLT;
        ImPlot::SetupAxes(useMv ? "x (mV)" : "x (V)", useMv ? "y (mV)" : "y (V)", 0, 0);

        if (useMv) {
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }

        if (cfg.plot.surfaceMode) {
            ImPlot::SetupAxisLimits(ImAxis_X1, -cfg.plot.limit * 4, cfg.plot.limit * 4, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -cfg.plot.limit / 4, cfg.plot.limit, ImGuiCond_Always);
        }
        else {
            ImPlot::SetupAxisLimits(ImAxis_X1, -cfg.plot.limit, cfg.plot.limit, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -cfg.plot.limit, cfg.plot.limit, ImGuiCond_Always);
        }

        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);
        const char* ch1_label = cfg.isCh2Enabled ? "Ch1" : "##Ch1";

        plotRingBufferLine(ch1_label, cfg.ringBuffer.ch[0].x, cfg.ringBuffer.ch[0].y,
            cfg.plot.xyStartIdx, cfg.plot.xySize, specLine);
        ImPlotSpec specScatter, specScatterFft;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5.0f * cfg.window.monitorScale;
        specScatter.MarkerFillColor = colors[Color_Blue];
        specScatter.MarkerLineColor = colors[Color_Blue];

        ImPlot::PlotScatter("##NOW1", &(cfg.ringBuffer.ch[0].x[cfg.plot.xyLatestIdx]), &(cfg.ringBuffer.ch[0].y[cfg.plot.xyLatestIdx]), 1, specScatter);
		if (cfg.awg.ch[0].func != 1) { // Sin waveの場合はFFTを表示しない
            specScatterFft.Marker = ImPlotMarker_Circle;
            specScatterFft.MarkerSize = 3.0f * cfg.window.monitorScale;
            specScatterFft.MarkerFillColor = ImPlot::GetColormapColor(0, ImPlotColormap_Deep);
            specScatter.MarkerLineColor = colors[Color_Red];
            ImPlot::PlotScatter("##FFT1", cfg.raw.harmonics[0].x.data(), cfg.raw.harmonics[0].y.data(), (int)cfg.raw.harmonics[0].y.size(), specScatterFft);
        }
        specScatter.MarkerFillColor = ImVec4(0, 0, 0, 0);
        ImPlot::PlotScatter("##REC1", cfg.xyRecs.ch1xys.x.data(), cfg.xyRecs.ch1xys.y.data(), (int)cfg.xyRecs.ch1xys.x.size(), specScatter);

        if (cfg.isCh2Enabled) {
            specLine.LineColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
            plotRingBufferLine("Ch2", cfg.ringBuffer.ch[1].x, cfg.ringBuffer.ch[1].y,
                cfg.plot.xyStartIdx, cfg.plot.xySize, specLine);

            specScatter.MarkerFillColor = colors[Color_Amber];
            specScatter.MarkerLineColor = colors[Color_Amber];
            ImPlot::PlotScatter("##NOW2", &(cfg.ringBuffer.ch[1].x[cfg.plot.xyLatestIdx]), &(cfg.ringBuffer.ch[1].y[cfg.plot.xyLatestIdx]), 1, specScatter);
            if (cfg.awg.ch[1].func != 1) {
                specScatterFft.MarkerFillColor = ImPlot::GetColormapColor(1, ImPlotColormap_Deep);
                ImPlot::PlotScatter("##FFT2", cfg.raw.harmonics[1].x.data(), cfg.raw.harmonics[1].y.data(), (int)cfg.raw.harmonics[1].y.size(), specScatterFft);
            }

            specScatter.MarkerFillColor = ImVec4(0, 0, 0, 0);
            ImPlot::PlotScatter("##REC2", cfg.xyRecs.ch2xys.x.data(), cfg.xyRecs.ch2xys.y.data(), (int)cfg.xyRecs.ch2xys.x.size(), specScatter);
        }

        if (cfg.flagAutoSetupW2History) {
            specLine.LineColor = ImPlot::GetColormapColor(2, ImPlotColormap_Deep);
            ImPlot::PlotLine("##HISTORY W1", cfg.autoSetupHistoryW1.x.data(), cfg.autoSetupHistoryW1.y.data(), (int)cfg.autoSetupHistoryW1.x.size(), specLine);
            specLine.LineColor = ImPlot::GetColormapColor(3, ImPlotColormap_Deep);
            ImPlot::PlotLine("##HISTORY W2", cfg.autoSetupHistoryW2.x.data(), cfg.autoSetupHistoryW2.y.data(), (int)cfg.autoSetupHistoryW2.x.size(), specLine);
        }
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor();
    ImGui::End();

    if (cfg.plot.surfaceMode) ImGui::PopStyleColor();
}


// ----------------------------------------------------------------------------
// ACFMPlotWindow
// ----------------------------------------------------------------------------
inline ACFMPlotWindow::ACFMPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "ACFM"), cfg(liaConfig) {
    this->windowPos = ImVec2(440 * liaConfig.window.monitorScale, 37 * 2 * liaConfig.window.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 560 * liaConfig.window.monitorScale);
}

inline void ACFMPlotWindow::show() {
    ImGui::SetNextWindowPos(windowPos, cfg.window.imGuiCondFlag);
    ImGui::SetNextWindowSize(windowSize, cfg.window.imGuiCondFlag);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    // Toolbar Controls
    if (ImGui::Button("Clear")) { cfg.buttonClear(); }
    ImGui::SameLine();
    if (ImGui::Button("Auto offset")) { cfg.buttonAutoOffset(); }
    ImGui::SameLine();
    if (ImGui::Button(cfg.pause.flag ? "Run" : "Pause")) { cfg.buttonPause(); }

    if (ImGui::SliderFloat("Vz limit", &(cfg.plot.limit), 0.01f, cfg.scope.ch[LiaConfigConst::CH_VERTICAL].range * 1.2f, "%4.2f V")) {
        cfg.plot.Vx_limit = cfg.plot.limit / 2;
    }
    if (ImGui::SliderFloat("Vx limit", &(cfg.plot.Vx_limit), 0.01f, cfg.scope.ch[LiaConfigConst::CH_HORIZONTAL].range * 1.2f, "%4.2f V")) {
    }

    if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        bool useMv = cfg.plot.limit <= MILI_VOLT;
        ImPlot::SetupAxes(useMv ? "Vz (mV)" : "Vz (V)", useMv ? "Vx (mV)" : "Vx (V)", 0, 0);

        if (useMv) {
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }

        ImPlot::SetupAxisLimits(ImAxis_X1, -cfg.plot.limit, cfg.plot.limit, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -cfg.plot.Vx_limit, cfg.plot.Vx_limit, ImGuiCond_Always);

        ImPlotSpec specLine;
        specLine.LineColor = ImPlot::GetColormapColor(2, ImPlotColormap_Deep);

        plotRingBufferLine("##ACFM", cfg.ringBuffer.ch[LiaConfigConst::CH_VERTICAL].y, cfg.ringBuffer.ch[LiaConfigConst::CH_HORIZONTAL].y,
            cfg.plot.xyStartIdx, cfg.plot.xySize, specLine);

        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * cfg.window.monitorScale;
        specScatter.MarkerFillColor = colors[Color_Chartreuse];
        specScatter.LineWeight = -1.0f;
        specScatter.MarkerLineColor = colors[Color_Chartreuse];

        ImPlot::PlotScatter("##NOW", &(cfg.ringBuffer.ch[LiaConfigConst::CH_VERTICAL].y[cfg.plot.xyLatestIdx]), &(cfg.ringBuffer.ch[LiaConfigConst::CH_HORIZONTAL].y[cfg.plot.xyLatestIdx]), 1, specScatter);

        double vhreal = cfg.ringBuffer.ch[LiaConfigConst::CH_HORIZONTAL].x[cfg.plot.xyLatestIdx];
        double mm = std::max(0.0, cfg.acfmData.mmk[0] * vhreal * vhreal + cfg.acfmData.mmk[1] * vhreal + cfg.acfmData.mmk[2]);

        std::string thicknessStr = (mm <= 6.0) ? std::format("{:5.2f}V:{:3.1f}mm", vhreal, mm) : std::format("{:5.2f}V:  out", vhreal);
        ImPlot::PlotText(thicknessStr.c_str(), 0.0, cfg.plot.Vx_limit * 0.9);

        ImPlot::EndPlot();
    }
    ImGui::End();
}


// ----------------------------------------------------------------------------
// ACFMVhVvPlotWindow
// ----------------------------------------------------------------------------
inline ACFMVhVvPlotWindow::ACFMVhVvPlotWindow(GLFWwindow* window, LiaConfig& liaConfig)
    : ImGuiWindowBase(window, "ACFM Vx-Vz"), cfg(liaConfig) {
    this->windowPos = ImVec2(730 * liaConfig.window.monitorScale, 37 * liaConfig.window.monitorScale);
    this->windowSize = ImVec2(560 * liaConfig.window.monitorScale, 650 * liaConfig.window.monitorScale);
}

inline void ACFMVhVvPlotWindow::show() {
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
    ImGui::Begin(this->name, nullptr, cfg.window.imGuiWindowFlag);

    if (ImPlot::BeginPlot("##Vx-Vz", ImVec2(-1, -1), cfg.window.imPlotFlag)) {
        bool useMv = cfg.plot.limit <= MILI_VOLT;

        ImPlot::SetupAxes(useMv ? "Vzpp (mV)" : "Vzpp (V)", useMv ? "Vxpp (mV)" : "Vhpp (V)", 0, 0);
        if (useMv) {
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(MiliFormatter));
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(MiliFormatter));
        }

        ImPlot::SetupAxisLimits(ImAxis_X1, 0, cfg.plot.limit * 2.0f, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, cfg.plot.Vx_limit, ImGuiCond_Always);

        ImPlotSpec specScatter;
        specScatter.Marker = ImPlotMarker_Circle;
        specScatter.MarkerSize = 5 * cfg.window.monitorScale;
        specScatter.LineWeight = -1.0f;

        // References
        specScatter.MarkerFillColor = colors[Color_Blue];
        specScatter.MarkerLineColor = colors[Color_Blue];
        ImPlot::PlotScatter("References", cfg.acfmData.Vvs.data(), cfg.acfmData.Vhs.data(), (int)cfg.acfmData.size, specScatter);

        // Result
        specScatter.MarkerFillColor = colors[Color_Amber];
        specScatter.MarkerLineColor = colors[Color_Amber];
        ImPlot::PlotScatter("Result", &(cfg.acfmData.vpp_vz), &(cfg.acfmData.vxpp), 1, specScatter);

        ImPlot::EndPlot();
    }
    ImGui::End();
}