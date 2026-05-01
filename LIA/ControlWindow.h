#pragma once

#include "LiaConfig.h"
#include "ImGuiWindowBase.h"
#include <iostream>
#include <cmath>
#include <numbers>
#include <thread>

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

// ============================================================
// UI ユーティリティ関数
// ============================================================
// UI アイテムがDeactivatedされたときにボタンイベントを記録
inline void markButtonIfItemDeactivated(ButtonType& outButton, float& outValue, ButtonType id, float val)
{
    if (ImGui::IsItemDeactivated()) {
        outButton = id;
        outValue = val;
    }
}

// 2点の座標から振幅を計算
inline double computeAmplitude(double x, double y) { 
    return std::hypot(x, y); 
}

// 2点の座標から位相（度数法）を計算
inline double computePhaseDeg(double y, double x) { 
    return std::atan2(y, x) * 180.0 / std::numbers::pi; 
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
std::pair<Point, Point> findMaxDistancePoints(LiaConfig* cfg, int record_ms) {
    // ミリ秒を要素数に変換
    const int length = static_cast<int>((record_ms / 1000.0) / MEASUREMENT_DT);

    // 安定化の待機と記録実施
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + record_ms));

    const auto& ringBuffer = cfg->ringBuffer;
    const int idx_end = ringBuffer.idx;
    const int bufsize = ringBuffer.size;

    // リングバッファから指定オフセット位置のPointを取得
    auto getPointAtOffset = [&](int offset) -> Point {
        const int idx = (idx_end - offset + bufsize) % bufsize;
        return { ringBuffer.ch[0].x[idx], ringBuffer.ch[0].y[idx] };
    };

    double maxDist2 = -1.0;
    Point best1{}, best2{};

    // すべての点のペアで最大距離を探索
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

    // 結果を正規化: 1. x座標とy座標のどちらが大きく変化するかを確認
    if (std::abs(best1.x - best2.x) > std::abs(best1.y - best2.y)) {
        // x座標が小さい方をfirstに配置
        if (best1.x > best2.x) {
            std::swap(best1, best2);
        }
    }
    else {
        // y座標が小さい方をfirstに配置
        if (best1.y > best2.y) {
            std::swap(best1, best2);
        }
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

    auto [p1, p2] = findMaxDistancePoints(cfg, record_ms);
    return calculatePolarVector(p1, p2);
}

// ============================================================
// W2の自動設定
// ============================================================
void autosetupW2(LiaConfig* cfg) {
    constexpr int RECORD_MS = 3000;

    printf("In progress...\n");

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

    // 自動オフセット調整によりプロットを原点に移動
	cfg->flagAutoOffset = true;
    cfg->flagAutoSetup = false;
    printf("  w2 amp.:%f, theta:%f\n", cfg->awgCfg.ch[1].amp, cfg->awgCfg.ch[1].phase);
    printf("Done.\n");
}

class ControlWindow : public ImGuiWindowBase
{
private:
	LiaConfig& liaConfig;

public:
	ControlWindow(GLFWwindow* window, LiaConfig& liaConfig)
		: ImGuiWindowBase(window, "Control panel"), liaConfig(liaConfig)
	{
		this->windowSize = ImVec2(450 * liaConfig.windowCfg.monitorScale, 920 * liaConfig.windowCfg.monitorScale);
	}

	// ボタンイベント（時刻、ボタンID、値）をコマンド履歴に記録
	void buttonPressed(const ButtonType button, const float value)
	{
		liaConfig.cmds.push_back(std::array<float, 3>{
			(float)liaConfig.timer.elapsedSec(),
			(float)button,
			value
		});
	}

	void awg(const float nextItemWidth);
	void autosetup(const float nextItemWidth) {
        ImGui::SetNextItemWidth(nextItemWidth);

        if (liaConfig.flagAutoSetup) {
            // 自動設定中は操作不可
            ImGui::BeginDisabled();
            ImGui::Button("W2 Autosetup in progress");
            ImGui::EndDisabled();
        } else {
            // 自動設定開始ボタン
            if (ImGui::Button("W2 Autosetup")) {
                liaConfig.flagAutoSetup = true;
                std::thread th_autosetup = std::thread{ autosetupW2, &liaConfig };
                th_autosetup.detach();
            }
        }
    }
    void plot(const float nextItemWidth);
	void post(const float nextItemWidth);
    void monitor();
	void functionButtons(const float nextItemWidth);
    void show(void);
};

inline void ControlWindow::awg(const float nextItemWidth)
{
    // AWG 周波数、振幅、位相の設定 UI
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
                if (liaConfig.awgCfg.ch[0].freq != freqkHz * 1e3f) {
                    // 両チャネルの周波数を同期
                    liaConfig.awgCfg.ch[0].freq = freqkHz * 1e3f;
                    liaConfig.awgCfg.ch[1].freq = freqkHz * 1e3f;
                    configChanged = true;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Freq, liaConfig.awgCfg.ch[0].freq);

            // 振幅設定
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

            // 位相設定（読み取り専用）
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::BeginDisabled();
            ImGui::InputFloat((char*)"θ (Deg.)", &liaConfig.awgCfg.ch[0].phase, 1, 1, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW1Phase, liaConfig.awgCfg.ch[0].phase);
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        // ===== W2（チャネル1）の設定 =====
        if (ImGui::BeginTabItem("W2")) {
            // 周波数設定（読み取り専用）
            ImGui::SetNextItemWidth(nextItemWidth);
            float freqkHz = liaConfig.awgCfg.ch[1].freq * 1e-3f;
            ImGui::BeginDisabled();
            ImGui::InputFloat("Freq. (kHz)", &freqkHz, 1.0f, 1.0f, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Freq, liaConfig.awgCfg.ch[1].freq);
            ImGui::EndDisabled();

            // 振幅設定
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

            // 位相設定
            ImGui::SetNextItemWidth(nextItemWidth);
            if (ImGui::InputFloat((char*)"θ (Deg.)", &liaConfig.awgCfg.ch[1].phase, 1, 1, "%3.0f")) {
                configChanged = true;
            }
            markButtonIfItemDeactivated(button, value, ButtonType::AwgW2Phase, liaConfig.awgCfg.ch[1].phase);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        // 設定が変更された場合は AWG を再起動
        if (configChanged) {
            liaConfig.awgStart();
        }

        // ボタンイベントを記録
        if (button != ButtonType::NON) {
            buttonPressed(button, value);
        }
    }
}

inline void ControlWindow::plot(const float nextItemWidth)
{
    // プロット表示の設定 UI（表示範囲、オフセット、テーマ）
    ButtonType button = ButtonType::NON;
    float value = 0;

    if (ImGui::BeginTabBar("Plot window")) {
        // ===== XY表示設定 =====
        if (ImGui::BeginTabItem("XY window")) {
            ImGui::SetNextItemWidth(nextItemWidth);

            // 表示範囲（Limit）の設定
            float step = (liaConfig.plotCfg.limit <= 0.1f) ? 0.01f : 0.1f;
            if (ImGui::InputFloat("Limit (V)", &liaConfig.plotCfg.limit, step, 0.1f, "%.2f")) {
                // 表示範囲を有効範囲内にクランプ
                liaConfig.plotCfg.limit = std::clamp(liaConfig.plotCfg.limit, 0.01f, RAW_RANGE * 1.2f);
                // 0.1～0.2 V の範囲では 0.2 V に強制
                if (0.1f < liaConfig.plotCfg.limit && liaConfig.plotCfg.limit < 0.2f) {
                    liaConfig.plotCfg.limit = 0.2f;
                }
            }
            markButtonIfItemDeactivated(button, value, ButtonType::PlotLimit, liaConfig.plotCfg.limit);

            // 位相オフセット設定
            ImGui::SetNextItemWidth(nextItemWidth);
            if (liaConfig.pauseCfg.flag) {
                ImGui::BeginDisabled();
            }
            ImGui::InputDouble((char*)"Ch1 θ (Deg.)", &liaConfig.postCfg.offset[0].phase, 1.0, 10.0, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::PostOffset1Phase, liaConfig.postCfg.offset[0].phase);

            // Ch2 位相オフセット（Ch2有効時のみ操作可能）
            if (!liaConfig.flagCh2) {
                ImGui::BeginDisabled();
            }
            ImGui::SetNextItemWidth(nextItemWidth);
            ImGui::InputDouble((char*)"Ch2 θ (Deg.)", &liaConfig.postCfg.offset[1].phase, 1.0, 10.0, "%3.0f");
            markButtonIfItemDeactivated(button, value, ButtonType::PostOffset2Phase, liaConfig.postCfg.offset[1].phase);
            if (!liaConfig.flagCh2) {
                ImGui::EndDisabled();
            }
            if (liaConfig.pauseCfg.flag) {
                ImGui::EndDisabled();
            }

            ImGui::EndTabItem();
        }

        // ===== UIテーマ・ウィンドウ設定 =====
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::SetNextItemWidth(nextItemWidth);
            const char* themes[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue", "NeonGreen", "NeonRed", "Eva" };
            ImGui::ListBox("Thema", &liaConfig.imguiCfg.theme, themes, IM_ARRAYSIZE(themes), 3);

            ImGui::SameLine();
            // ウィンドウサイズロック/アンロック
            if (liaConfig.imguiCfg.windowFlag == ImGuiCond_FirstUseEver) {
                if (ImGui::Button("Lock")) {
                    liaConfig.imguiCfg.windowFlag = ImGuiCond_Always;
                }
            } else {
                if (ImGui::Button("Unlock")) {
                    liaConfig.imguiCfg.windowFlag = ImGuiCond_FirstUseEver;
                }
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
	// ポスト処理設定：オフセット、一時停止制御
	ButtonType button = ButtonType::NON;
	float value = 0;

	static ImVec2 autoOffsetSize = ImVec2(200.0f * liaConfig.windowCfg.monitorScale, 100.0f * liaConfig.windowCfg.monitorScale);
	static ImVec2 buttonSize = ImVec2(100.0f * liaConfig.windowCfg.monitorScale, autoOffsetSize.y);

	// オフセット値が有効かチェック（x=0 かつ y=0 なら無効）
	const bool offsetIsValid = (liaConfig.postCfg.offset[0].x != 0 || liaConfig.postCfg.offset[0].y != 0);

	// ===== 制御ボタン群（一時停止時は操作不可）=====
	if (liaConfig.pauseCfg.flag) {
		ImGui::BeginDisabled();
	}

	// 自動オフセットボタン
	if (ImGui::Button(" Auto\noffset", autoOffsetSize)) {
		liaConfig.flagAutoOffset = true;
	}
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::PostAutoOffset;
		value = 0;
	}

	ImGui::SameLine();

	// オフセットリセットボタン（有効な場合のみ操作可能）
	if (!offsetIsValid) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Off", buttonSize)) {
		// 全チャネルのオフセットをリセット
		liaConfig.postCfg.offset[0].x = 0.0f;
		liaConfig.postCfg.offset[0].y = 0.0f;
		liaConfig.postCfg.offset[1].x = 0.0f;
		liaConfig.postCfg.offset[1].y = 0.0f;
	}
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::PostOffsetOff;
		value = 0;
	}
	if (!offsetIsValid) {
		ImGui::EndDisabled();
	}

	if (liaConfig.pauseCfg.flag) {
		ImGui::EndDisabled();
	}

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(1.0f * liaConfig.windowCfg.monitorScale, 0.0f));
	ImGui::SameLine();

	// 実行/一時停止トグルボタン
	if (liaConfig.pauseCfg.flag) {
		if (ImGui::Button("Run", buttonSize)) {
			liaConfig.pauseCfg.flag = false;
		}
	} else {
		if (ImGui::Button("Pause", buttonSize)) {
			liaConfig.pauseCfg.flag = true;
		}
	}
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::PostPause;
		value = liaConfig.pauseCfg.flag;
	}

	// ===== オフセット値の表示 =====
	if (ImGui::TreeNode("Offset value")) {
		if (!offsetIsValid) {
			ImGui::BeginDisabled();
		}

		ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", liaConfig.postCfg.offset[0].x, liaConfig.postCfg.offset[0].y);

		if (!liaConfig.flagCh2) {
			ImGui::BeginDisabled();
		}
		ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", liaConfig.postCfg.offset[1].x, liaConfig.postCfg.offset[1].y);
		if (!liaConfig.flagCh2) {
			ImGui::EndDisabled();
		}

		ImGui::TreePop();

		if (!offsetIsValid) {
			ImGui::EndDisabled();
		}
	}

	if (button != ButtonType::NON) {
		buttonPressed(button, value);
	}
}

inline void ControlWindow::monitor()
{
	// リングバッファの現在値をモニタリング表示
	if (ImGui::TreeNode("Monitor")) {
		// 現在のバッファインデックスの値を取得
		const int idx = liaConfig.ringBuffer.idx;
		const auto& ch0_x = liaConfig.ringBuffer.ch[0].x[idx];
		const auto& ch0_y = liaConfig.ringBuffer.ch[0].y[idx];

		// Ch1 データ表示
		ImGui::Text("Ch1 X:%5.2fV,Y:%5.2fV", ch0_x, ch0_y);
		ImGui::Text(
			"Amp:%4.2fV, %s:%4.0fDeg.",
			computeAmplitude(ch0_x, ch0_y),
			"θ",
			computePhaseDeg(ch0_y, ch0_x)
		);

		// Ch2 データ表示（Ch2有効時のみ）
		if (!liaConfig.flagCh2) {
			ImGui::BeginDisabled();
		}

		const auto& ch1_x = liaConfig.ringBuffer.ch[1].x[idx];
		const auto& ch1_y = liaConfig.ringBuffer.ch[1].y[idx];
		ImGui::Text("Ch2 X:%5.2fV,Y:%5.2fV", ch1_x, ch1_y);
		ImGui::Text(
			"Amp:%4.2fV, %s:%4.0fDeg.",
			computeAmplitude(ch1_x, ch1_y),
			"θ",
			computePhaseDeg(ch1_y, ch1_x)
		);

		if (!liaConfig.flagCh2) {
			ImGui::EndDisabled();
		}

		ImGui::TreePop();
	}
}

inline void ControlWindow::functionButtons(const float nextItemWidth)
{
	// 各種フィーチャーの制御ボタンと設定
	ButtonType button = ButtonType::NON;
	float value = 0;

	// ===== 表示モード設定 =====
	ImGui::Checkbox("Surface Mode", &liaConfig.plotCfg.surfaceMode);
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::PlotSurfaceMode;
		value = liaConfig.plotCfg.surfaceMode;
	}

	ImGui::SameLine();
	ImGui::Checkbox("Beep", &liaConfig.plotCfg.beep);
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::PlotBeep;
		value = liaConfig.plotCfg.beep;
	}

	// ===== Ch2 制御（ACFM有効時は強制有効） =====
	if (liaConfig.plotCfg.acfm) {
		liaConfig.flagCh2 = true;
		ImGui::BeginDisabled();
	}

	ImGui::Checkbox("Ch2", &liaConfig.flagCh2);
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::DispCh2;
		value = liaConfig.flagCh2;
	}

	if (liaConfig.plotCfg.acfm) {
		ImGui::EndDisabled();
	}

	ImGui::SameLine();
	ImGui::Checkbox("ACFM", &liaConfig.plotCfg.acfm);
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::PlotACFM;
		value = liaConfig.plotCfg.acfm;
	}

	// ===== フィルタ設定 =====
	ImGui::SetNextItemWidth(nextItemWidth);
	if (ImGui::InputFloat("HPF (Hz)", &liaConfig.postCfg.hpFreq, 0.1f, 1.0f, "%.1f")) {
		// ハイパスフィルタの周波数を有効範囲内にクランプ
		liaConfig.postCfg.hpFreq = std::clamp(liaConfig.postCfg.hpFreq, 0.0f, 10.0f);
		liaConfig.setHPFrequency(liaConfig.postCfg.hpFreq);
	}
	markButtonIfItemDeactivated(button, value, ButtonType::PostHpFreq, liaConfig.postCfg.hpFreq);

	ImGui::SetNextItemWidth(nextItemWidth);
	if (ImGui::InputFloat("LPF (Hz)", &liaConfig.postCfg.lpFreq, 1.0f, 10.0f, "%.0f")) {
		// ローパスフィルタの周波数を有効範囲内にクランプ
		liaConfig.postCfg.lpFreq = std::clamp(liaConfig.postCfg.lpFreq, 1.0f, 100.0f);
		liaConfig.setLPFrequency(liaConfig.postCfg.lpFreq);
	}
	markButtonIfItemDeactivated(button, value, ButtonType::PostLpFreq, liaConfig.postCfg.lpFreq);

	if (button != ButtonType::NON) {
		buttonPressed(button, value);
	}
}

inline void ControlWindow::show(void)
{
	ButtonType button = ButtonType::NON;
	float value = 0;
	static float nextItemWidth = 170.0f * liaConfig.windowCfg.monitorScale;

	// ウィンドウ位置・サイズ設定
	ImGui::SetNextWindowPos(ImVec2(0, 0), liaConfig.imguiCfg.windowFlag);
	ImGui::SetNextWindowSize(windowSize, liaConfig.imguiCfg.windowFlag);
	ImGui::Begin(this->name);

	// ===== ヘッダ（デバイス情報・クローズボタン） =====
	ImGui::SetNextItemWidth(nextItemWidth);
	ImGui::Text("%s", liaConfig.device_sn.data());
	ImGui::SameLine();
	if (ImGui::Button("Close")) {
		liaConfig.statusMeasurement = false;
	}
	if (ImGui::IsItemDeactivated()) {
		button = ButtonType::Close;
		value = 0;
	}

	// ===== AWG 設定 =====
	awg(nextItemWidth);
	autosetup(nextItemWidth);

	// ===== グラフ表示設定 =====
	plot(nextItemWidth);
	ImGui::Separator();

	// ===== ポスト処理設定 =====
	post(nextItemWidth);
	monitor();
	ImGui::Separator();

	// ===== 機能ボタン群 =====
	functionButtons(nextItemWidth);
	ImGui::Separator();

	// ===== 経過時間表示 =====
	const int totalSecs = static_cast<int>(liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx]);
	const int hours = totalSecs / (60 * 60);
	const int mins = (totalSecs - hours * 60 * 60) / 60;
	const double secs = liaConfig.ringBuffer.times[liaConfig.ringBuffer.idx] - hours * 60 * 60 - mins * 60;
	ImGui::Text("Time:%02d:%02d:%02.0f", hours, mins, secs);

	ImGui::End();

	if (button != ButtonType::NON) {
		buttonPressed(button, value);
	}
}
