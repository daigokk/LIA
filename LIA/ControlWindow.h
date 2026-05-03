#pragma once

#include "LiaConfig.h"
#include "ImGuiWindowBase.h"
#include <iostream>
#include <cmath>
#include <numbers>
#include <thread>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <format>

// ============================================================
// 型定義
// ============================================================
struct Point {
    double x;
    double y;
};

// 極座標を表す構造体（振幅と位相（度数法））
struct PolarVector {
    double amplitude;
    double phaseDeg;
};

// ============================================================
// 座標・ベクトル計算
// ============================================================
// 2点間の距離の2乗を計算
inline double getSquaredDistance(const Point& a, const Point& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// 2点間のベクトルから振幅と位相（度数法）を計算
inline PolarVector calculatePolarVector(const Point& p1, const Point& p2) {
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    return {
        std::hypot(dx, dy),
        std::atan2(dy, dx) * 180.0 / std::numbers::pi
    };
}

// 2点の座標から振幅を計算
inline double computeAmplitude(double x, double y) {
    return std::hypot(x, y);
}

// 2点の座標から位相（度数法）を計算
inline double computePhaseDeg(double y, double x) {
    return std::atan2(y, x) * 180.0 / std::numbers::pi;
}

// ============================================================
// UI ユーティリティ関数
// ============================================================
// UI アイテムがDeactivatedされたときにボタンイベントを記録
inline void markButtonIfItemDeactivated(ButtonType& outButton, float& outValue, ButtonType id, float val) {
    if (ImGui::IsItemDeactivated()) {
        outButton = id;
        outValue = val;
    }
}

// 周波数を有効範囲内にクランプ
inline float clampFrequency(float freqkHz) {
    return std::clamp(freqkHz, LOW_LIMIT_FREQ * 1e-3f, HIGH_LIMIT_FREQ * 1e-3f);
}

// 振幅を有効範囲内にクランプ
inline float clampAmplitude(float amp) {
    return std::clamp(amp, AWG_AMP_MIN, AWG_AMP_MAX);
}

// ============================================================
// AWG 測定・解析
// ============================================================
// 指定した時間だけ記録し、リングバッファから最大距離の2点を返す
std::pair<Point, Point> findMaxDistancePoints(LiaConfig* cfg, const int record_ms, const bool flagW1) {
    // AWGを開始して安定化のために少し待機する時間ms
    constexpr int WAIT_MS = 2;

    // ミリ秒を要素数に変換
    const int length = static_cast<int>((record_ms / 1000.0) / MEASUREMENT_DT);

    // ヒストリーバッファの取得とリサイズ
    auto& targetHistory = flagW1 ? cfg->autoSetupHistoryW1 : cfg->autoSetupHistoryW2;
    targetHistory.x.resize(length);
    targetHistory.y.resize(length);

    // 安定化の待機と記録実施
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS + record_ms));

    const auto& ringBuffer = cfg->ringBuffer;
    const int idx_end = ringBuffer.idx;
    const int bufsize = ringBuffer.size;

    // リングバッファから指定オフセット位置のPointを取得するラムダ
    auto getPointAtOffset = [&](int offset) -> Point {
        const int idx = (idx_end - offset + bufsize) % bufsize;
        return { ringBuffer.ch[0].x[idx], ringBuffer.ch[0].y[idx] };
        };

    double maxDist2 = -1.0;
    Point best1{}, best2{};

    // すべての点のペアで最大距離を探索 (O(N^2) - 要素数に注意)
    for (int i = 0; i < length; i++) {
        const Point p1 = getPointAtOffset(i);
        for (int j = i + 1; j < length; j++) {
            const Point p2 = getPointAtOffset(j);
            const double d2 = getSquaredDistance(p1, p2);

            if (d2 > maxDist2) {
                maxDist2 = d2;
                best1 = p1;
                best2 = p2;
            }
        }
    }

    // 結果を正規化: x座標とy座標のどちらが大きく変化するかを確認し、小さい方をfirstに配置
    const bool isXDominant = std::abs(best1.x - best2.x) > std::abs(best1.y - best2.y);
    if ((isXDominant && best1.x > best2.x) || (!isXDominant && best1.y > best2.y)) {
        std::swap(best1, best2);
    }

    // 履歴のオフセット補正
    const Point basePoint = flagW1 ? best1 : best2;
    for (int i = 0; i < length; i++) {
        const Point p = getPointAtOffset(i);
        targetHistory.x[i] = p.x - basePoint.x;
        targetHistory.y[i] = p.y - basePoint.y;
    }

    return { best1, best2 };
}

// AWG の設定、計測、極座標計算を一連で実施
PolarVector measureAwgResponse(LiaConfig* cfg, double ch0_amp, double ch1_amp, int record_ms) {
    cfg->awgCfg.ch[0].amp = ch0_amp;
    cfg->awgCfg.ch[0].phase = 0.0;
    cfg->awgCfg.ch[1].amp = ch1_amp;
    cfg->awgCfg.ch[1].phase = 0.0;
    cfg->awgStart();

    const bool flagW1 = (ch0_amp != 0.0);
    auto [p1, p2] = findMaxDistancePoints(cfg, record_ms, flagW1);

    return calculatePolarVector(p1, p2);
}

// ============================================================
// W2の自動設定
// ============================================================
void autosetupW2(LiaConfig* cfg) {
    constexpr int RECORD_MS = 3000; // 記録時間を3秒に設定

    printf("Autosetup W2 in progress...\n");

    const double original_amp = cfg->awgCfg.ch[0].amp;

    // ステップ1: W1をON、W2をOFF で測定
    PolarVector w1 = measureAwgResponse(cfg, original_amp, 0.0, RECORD_MS);
    printf("  w1 abs:%f, theta:%f\n", w1.amplitude, w1.phaseDeg);

    // ステップ2: W1をOFF、W2をON で測定
    PolarVector w2 = measureAwgResponse(cfg, 0.0, original_amp, RECORD_MS);
    printf("  w2 abs:%f, theta:%f\n", w2.amplitude, w2.phaseDeg);

    // ステップ3: W2の振幅と位相をW1に合わせるように調整
    cfg->awgCfg.ch[0].amp = original_amp;
    cfg->awgCfg.ch[1].amp = original_amp * (w1.amplitude / w2.amplitude);
    cfg->awgCfg.ch[1].phase = w2.phaseDeg - w1.phaseDeg;
    cfg->awgStart();

    // 安定化の待機
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // W2の自動設定後、ステータスを更新
    cfg->flagAutoOffset = true;
    cfg->flagAutoSetupW2 = false;
    cfg->flagAutoSetupW2History = true;

    // autoSetupHistoryをCSVファイルに保存
    const std::string filepath = std::format("./{}/autosetupw2history.csv", cfg->dirName);
    if (std::ofstream file{ filepath }) {
        std::stringstream ss;
        ss << "# w1x(V),w1y(V),w2x(V),w2y(V)\n";
        for (size_t i = 0; i < cfg->autoSetupHistoryW1.x.size(); ++i) {
            ss << std::format("{:e},{:e},{:e},{:e}\n",
                cfg->autoSetupHistoryW1.x[i], cfg->autoSetupHistoryW1.y[i],
                cfg->autoSetupHistoryW2.x[i], cfg->autoSetupHistoryW2.y[i]);
        }
        file << ss.str(); // メモリからファイルへ一括書き込み
    }
    else {
        std::cerr << "Error: Could not open file autosetupw2history.csv" << std::endl;
    }

    printf("  New w2 amp.:%f, theta:%f\n", cfg->awgCfg.ch[1].amp, cfg->awgCfg.ch[1].phase);
    printf("Done.\n");
}

// ============================================================
// UI ウィンドウクラス
// ============================================================
class ControlWindow : public ImGuiWindowBase
{
private:
    LiaConfig& liaConfig;

    // 内部用の描画メソッド（旧 show_）
    void drawContent();

public:
    ControlWindow(GLFWwindow* window, LiaConfig& liaConfig)
        : ImGuiWindowBase(window, "Control panel"), liaConfig(liaConfig)
    {
        this->windowSize = ImVec2(450 * liaConfig.windowCfg.monitorScale, 920 * liaConfig.windowCfg.monitorScale);
    }

    // ボタンイベント（時刻、ボタンID、値）をコマンド履歴に記録
    void buttonPressed(const ButtonType button, const float value)
    {
        liaConfig.cmds.push_back({
            static_cast<float>(liaConfig.timer.elapsedSec()),
            static_cast<float>(button),
            value
            });
    }

    void awg(const float nextItemWidth);
    void plot(const float nextItemWidth);
    void post(const float nextItemWidth);
    void monitor();
    void functionButtons(const float nextItemWidth);
    void show();
};

inline void ControlWindow::awg(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    bool configChanged = false;

    if (ImGui::BeginTabBar("Awg")) {
        // ===== W1（チャネル0）の設定 =====
        if (ImGui::BeginTabItem("W1")) {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awgCfg.ch[0].freq * 1e-3f;

            if (ImGui::InputFloat("Freq. (kHz)", &freqkHz, 1.0f, 1.0f, "%3.0f")) {
                freqkHz = clampFrequency(freqkHz);
                const float newFreqHz = freqkHz * 1e3f;
                if (liaConfig.awgCfg.ch[0].freq != newFreqHz) {
                    liaConfig.awgCfg.ch[0].freq = newFreqHz;
                    liaConfig.awgCfg.ch[1].freq = newFreqHz; // W2も同期
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Freq, liaConfig.awgCfg.ch[0].freq);

            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh0Amp = liaConfig.awgCfg.ch[0].amp;
            if (ImGui::InputFloat("Amp. (V)", &liaConfig.awgCfg.ch[0].amp, 0.1f, 0.1f, "%4.1f")) {
                liaConfig.awgCfg.ch[0].amp = clampAmplitude(liaConfig.awgCfg.ch[0].amp);
                if (oldCh0Amp != liaConfig.awgCfg.ch[0].amp) {
                    oldCh0Amp = liaConfig.awgCfg.ch[0].amp;
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Amp, liaConfig.awgCfg.ch[0].amp);

            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::BeginDisabled();
            ImGui::InputFloat("θ (Deg.)", &liaConfig.awgCfg.ch[0].phase, 1.0f, 1.0f, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Phase, liaConfig.awgCfg.ch[0].phase);
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        // ===== W2（チャネル1）の設定 =====
        if (ImGui::BeginTabItem("W2")) {
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awgCfg.ch[1].freq * 1e-3f;

            ImGui::BeginDisabled();
            ImGui::InputFloat("Freq. (kHz)", &freqkHz, 1.0f, 1.0f, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Freq, liaConfig.awgCfg.ch[1].freq);
            ImGui::EndDisabled();

            ImGui::SetNextItemWidth(nextItemWidth);
            static float oldCh1Amp = liaConfig.awgCfg.ch[1].amp;
            if (ImGui::InputFloat("Amp. (V)", &liaConfig.awgCfg.ch[1].amp, 0.01f, 0.1f, "%4.2f")) {
                liaConfig.awgCfg.ch[1].amp = std::clamp(liaConfig.awgCfg.ch[1].amp, 0.0f, AWG_AMP_MAX);
                if (oldCh1Amp != liaConfig.awgCfg.ch[1].amp) {
                    oldCh1Amp = liaConfig.awgCfg.ch[1].amp;
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Amp, liaConfig.awgCfg.ch[1].amp);

            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat("θ (Deg.)", &liaConfig.awgCfg.ch[1].phase, 1.0f, 1.0f, "%3.0f")) {
                configChanged = true;
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Phase, liaConfig.awgCfg.ch[1].phase);

            ImGui::SetNextItemWidth(nextItemWidth);
            if (liaConfig.flagAutoSetupW2) {
                ImGui::BeginDisabled();
                ImGui::Button("W2 Autosetup in progress");
                ImGui::EndDisabled();
            }
            else {
                if (ImGui::Button("W2 Autosetup")) {
                    liaConfig.flagAutoSetupW2 = true;
                    std::thread th_autosetup{ autosetupW2, &liaConfig };
                    th_autosetup.detach();
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2AutoSetup, 0);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        if (configChanged) {
            liaConfig.awgStart();
        }
        if (button != ButtonType::NON) {
            buttonPressed(button, value);
        }
    }
}

inline void ControlWindow::plot(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;

    if (ImGui::BeginTabBar("Plot window")) {
        // ===== XY表示設定 =====
        if (ImGui::BeginTabItem("XY window")) {
            ImGui::SetNextItemWidth(nextItemWidth);

            const float step = (liaConfig.plotCfg.limit <= 0.1f) ? 0.01f : 0.1f;
            if (ImGui::InputFloat("Limit (V)", &liaConfig.plotCfg.limit, step, 0.1f, "%.2f")) {
                liaConfig.plotCfg.limit = std::clamp(liaConfig.plotCfg.limit, 0.01f, RAW_RANGE * 1.2f);
                if (0.1f < liaConfig.plotCfg.limit && liaConfig.plotCfg.limit < 0.2f) {
                    liaConfig.plotCfg.limit = 0.2f;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::PlotLimit, liaConfig.plotCfg.limit);

            ImGui::SetNextItemWidth(nextItemWidth);
            if (liaConfig.pauseCfg.flag) ImGui::BeginDisabled();

            ImGui::InputDouble("Ch1 θ (Deg.)", &liaConfig.postCfg.offset[0].phase, 1.0, 10.0, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::PostOffset1Phase, liaConfig.postCfg.offset[0].phase);

            if (!liaConfig.flagCh2) ImGui::BeginDisabled();

            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble("Ch2 θ (Deg.)", &liaConfig.postCfg.offset[1].phase, 1.0, 10.0, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::PostOffset2Phase, liaConfig.postCfg.offset[1].phase);

            if (!liaConfig.flagCh2) ImGui::EndDisabled();
            if (liaConfig.pauseCfg.flag) ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        // ===== UIテーマ・ウィンドウ設定 =====
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::SetNextItemWidth(nextItemWidth);
            const char* themes[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue", "NeonGreen", "NeonRed", "Eva" };
            ImGui::ListBox("Thema", &liaConfig.imguiCfg.theme, themes, IM_ARRAYSIZE(themes), 3);

            ImGui::SameLine();
            const bool isFirstUse = (liaConfig.imguiCfg.windowFlag == ImGuiCond_FirstUseEver);
            if (ImGui::Button(isFirstUse ? "Lock" : "Unlock")) {
                liaConfig.imguiCfg.windowFlag = isFirstUse ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        if (button != ButtonType::NON) {
            buttonPressed(button, value);
        }
    }
}

inline void ControlWindow::post(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;

    static const ImVec2 autoOffsetSize(200.0f * liaConfig.windowCfg.monitorScale, 100.0f * liaConfig.windowCfg.monitorScale);
    static const ImVec2 buttonSize(100.0f * liaConfig.windowCfg.monitorScale, autoOffsetSize.y);

    const bool offsetIsValid = (liaConfig.postCfg.offset[0].x != 0.0f || liaConfig.postCfg.offset[0].y != 0.0f);

    if (liaConfig.pauseCfg.flag) ImGui::BeginDisabled();

    // 自動オフセットボタン
    if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
        liaConfig.flagAutoOffset = true;
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostAutoOffset, 0);

    ImGui::SameLine();

    // オフセットリセットボタン
    if (!offsetIsValid) ImGui::BeginDisabled();
    if (ImGui::Button("Off", buttonSize)) {
        liaConfig.postCfg.offset[0] = { 0.0f, 0.0f, liaConfig.postCfg.offset[0].phase };
        liaConfig.postCfg.offset[1] = { 0.0f, 0.0f, liaConfig.postCfg.offset[1].phase };
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostOffsetOff, 0);
    if (!offsetIsValid) ImGui::EndDisabled();

    if (liaConfig.pauseCfg.flag) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(1.0f * liaConfig.windowCfg.monitorScale, 0.0f));
    ImGui::SameLine();

    // 実行/一時停止トグル
    if (ImGui::Button(liaConfig.pauseCfg.flag ? "Run" : "Pause", buttonSize)) {
        liaConfig.pauseCfg.flag = !liaConfig.pauseCfg.flag;
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostPause, liaConfig.pauseCfg.flag);

    // オフセット値の表示
    if (ImGui::TreeNode("Offset value")) {
        if (!offsetIsValid) ImGui::BeginDisabled();

        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", liaConfig.postCfg.offset[0].x, liaConfig.postCfg.offset[0].y);

        if (!liaConfig.flagCh2) ImGui::BeginDisabled();
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", liaConfig.postCfg.offset[1].x, liaConfig.postCfg.offset[1].y);
        if (!liaConfig.flagCh2) ImGui::EndDisabled();

        ImGui::TreePop();
        if (!offsetIsValid) ImGui::EndDisabled();
    }

    if (button != ButtonType::NON) {
        buttonPressed(button, value);
    }
}

inline void ControlWindow::monitor()
{
    if (ImGui::TreeNode("Monitor")) {
        const int idx = liaConfig.ringBuffer.idx;

        // Ch1 データ表示
        const float ch0_x = liaConfig.ringBuffer.ch[0].x[idx];
        const float ch0_y = liaConfig.ringBuffer.ch[0].y[idx];
        ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", ch0_x, ch0_y);
        ImGui::Text("Amp:%4.2fV, θ:%4.0fDeg.", computeAmplitude(ch0_x, ch0_y), computePhaseDeg(ch0_y, ch0_x));

        // Ch2 データ表示
        if (!liaConfig.flagCh2) ImGui::BeginDisabled();

        const float ch1_x = liaConfig.ringBuffer.ch[1].x[idx];
        const float ch1_y = liaConfig.ringBuffer.ch[1].y[idx];
        ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", ch1_x, ch1_y);
        ImGui::Text("Amp:%4.2fV, θ:%4.0fDeg.", computeAmplitude(ch1_x, ch1_y), computePhaseDeg(ch1_y, ch1_x));

        if (!liaConfig.flagCh2) ImGui::EndDisabled();

        ImGui::TreePop();
    }
}

inline void ControlWindow::functionButtons(const float nextItemWidth)
{
    ButtonType button = ButtonType::NON;
    float value = 0;

    ImGui::Checkbox("Surface Mode", &liaConfig.plotCfg.surfaceMode);
    markButtonIfItemDeactivated(button, value, ButtonType::PlotSurfaceMode, liaConfig.plotCfg.surfaceMode);

    ImGui::SameLine();
    ImGui::Checkbox("Beep", &liaConfig.plotCfg.beep);
    markButtonIfItemDeactivated(button, value, ButtonType::PlotBeep, liaConfig.plotCfg.beep);

    // ACFM有効時はCh2を強制有効化
    if (liaConfig.plotCfg.acfm) {
        liaConfig.flagCh2 = true;
        ImGui::BeginDisabled();
    }
    ImGui::Checkbox("Ch2", &liaConfig.flagCh2);
    markButtonIfItemDeactivated(button, value, ButtonType::DispCh2, liaConfig.flagCh2);
    if (liaConfig.plotCfg.acfm) ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::Checkbox("ACFM", &liaConfig.plotCfg.acfm);
    markButtonIfItemDeactivated(button, value, ButtonType::PlotACFM, liaConfig.plotCfg.acfm);

    // フィルタ設定
    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("HPF (Hz)", &liaConfig.postCfg.hpFreq, 0.1f, 1.0f, "%.1f")) {
        liaConfig.postCfg.hpFreq = std::clamp(liaConfig.postCfg.hpFreq, 0.0f, 10.0f);
        liaConfig.setHPFrequency(liaConfig.postCfg.hpFreq);
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostHpFreq, liaConfig.postCfg.hpFreq);

    ImGui::SetNextItemWidth(nextItemWidth);
    if (ImGui::InputFloat("LPF (Hz)", &liaConfig.postCfg.lpFreq, 1.0f, 10.0f, "%.0f")) {
        liaConfig.postCfg.lpFreq = std::clamp(liaConfig.postCfg.lpFreq, 1.0f, 100.0f);
        liaConfig.setLPFrequency(liaConfig.postCfg.lpFreq);
    }
    markButtonIfItemDeactivated(button, value, ButtonType::PostLpFreq, liaConfig.postCfg.lpFreq);

    if (button != ButtonType::NON) {
        buttonPressed(button, value);
    }
}

inline void ControlWindow::drawContent()
{
    ButtonType button = ButtonType::NON;
    float value = 0;
    const float nextItemWidth = 170.0f * liaConfig.windowCfg.monitorScale;

    // ヘッダ
    ImGui::SetNextItemWidth(nextItemWidth);
    ImGui::Text("%s", liaConfig.device_sn.data());
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        liaConfig.statusMeasurement = false;
    }
    markButtonIfItemDeactivated(button, value, ButtonType::Close, 0);

    awg(nextItemWidth);
    plot(nextItemWidth);
    ImGui::Separator();

    post(nextItemWidth);
    monitor();
    ImGui::Separator();

    functionButtons(nextItemWidth);
    ImGui::Separator();

    // 経過時間表示
    const int totalSecs = static_cast<int>(liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx]);
    const int hours = totalSecs / 3600;
    const int mins = (totalSecs % 3600) / 60;
    const double secs = liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx] - (hours * 3600) - (mins * 60);
    ImGui::Text("Time:%02d:%02d:%02.0f", hours, mins, secs);

    if (button != ButtonType::NON) {
        buttonPressed(button, value);
    }
}

inline void ControlWindow::show()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), liaConfig.imguiCfg.windowFlag);
    ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);

    if (ImGui::Begin(this->name)) {
        if (liaConfig.flagAutoSetupW2) {
            ImGui::BeginDisabled();
            drawContent();
            ImGui::EndDisabled();
        }
        else {
            drawContent();
        }
    }
    ImGui::End();
}