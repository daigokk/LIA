#pragma once
#include "LiaConfig.h"

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
// 座標・ベクトル計算 (純粋関数)
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
// データ解析・加工ヘルパー
// ============================================================

// 履歴データから最大距離となる2点を探索し、主変化軸に沿ってソートして返す
std::pair<Point, Point> findMaxDistancePoints(const std::vector<double>& x, const std::vector<double>& y) {
    double maxDist2 = -1.0;
    Point best1{}, best2{};
    const size_t size = x.size();

    // すべての点のペアで最大距離を探索 (要素数が多い場合はアルゴリズムの見直しを推奨)
    for (size_t i = 0; i < size; ++i) {
        const Point p1 = { x[i], y[i] };
        for (size_t j = i + 1; j < size; ++j) {
            const Point p2 = { x[j], y[j] };
            const double d2 = getSquaredDistance(p1, p2);
            if (d2 > maxDist2) {
                maxDist2 = d2;
                best1 = p1;
                best2 = p2;
            }
        }
    }

    // 2点間の位置関係を正規化（主変化軸に基づいて昇順に並び替え）
    const bool isXDominant = std::abs(best1.x - best2.x) > std::abs(best1.y - best2.y);
    if ((isXDominant && best1.x > best2.x) || (!isXDominant && best1.y > best2.y)) {
        std::swap(best1, best2);
    }

    return { best1, best2 };
}

// 基準点を元に履歴データ全体をオフセット補正する
void offsetHistory(LiaConfig::XYs& history, const Point& basePoint) {
    for (size_t i = 0; i < history.x.size(); ++i) {
        history.x[i] -= basePoint.x;
        history.y[i] -= basePoint.y;
    }
}

// ============================================================
// AWG 制御・測定
// ============================================================

// AWGの設定を適用し、指定時間だけ待機する
void applyAwgSettingsAndWait(LiaConfig* cfg, const PolarVector& ch0, const PolarVector& ch1, const int record_ms) {
    cfg->awgCfg.ch[0].amp = ch0.amplitude;
    cfg->awgCfg.ch[0].phase = ch0.phaseDeg;
    cfg->awgCfg.ch[1].amp = ch1.amplitude;
    cfg->awgCfg.ch[1].phase = ch1.phaseDeg;
    cfg->awgStart();

    // 安定化のための待機時間(ms) + 記録時間(ms)
    constexpr int STABILIZE_WAIT_MS = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(STABILIZE_WAIT_MS + record_ms));
}

// リングバッファから最新の指定時間分のデータを履歴バッファに抽出する
void extractRingBufferToHistory(const LiaConfig::RingBuffer& ringBuffer, LiaConfig::XYs& targetHistory, const int record_ms) {
    const int length = static_cast<int>((record_ms / 1000.0) / MEASUREMENT_DT);

    targetHistory.x.resize(length);
    targetHistory.y.resize(length);

    const int latestIdx = ringBuffer.latestIdx;
    const int bufsize = ringBuffer.size;

    for (int i = 0; i < length; ++i) {
        int idx = latestIdx - length + 1 + i;
        // バッファのラップアラウンドを考慮
        while (idx < 0) {
            idx += bufsize;
        }
        targetHistory.x[i] = ringBuffer.ch[0].x[idx];
        targetHistory.y[i] = ringBuffer.ch[0].y[idx];
    }
}

// 履歴データをCSVに出力する
void exportHistoryToCsv(const LiaConfig* cfg) {
    const std::string filepath = std::format("./{}/autosetupw2history.csv", cfg->dirName);
    std::ofstream file(filepath);

    if (!file) {
        std::cerr << "Error: Could not open file " << filepath << std::endl;
        return;
    }

    std::stringstream ss;
    ss << "# t(s),w1x(V),w1y(V),w2x(V),w2y(V)\n";
    for (size_t i = 0; i < cfg->autoSetupHistoryW1.x.size(); ++i) {
        ss << std::format("{:e},{:e},{:e},{:e},{:e}\n",
            MEASUREMENT_DT * i,
            cfg->autoSetupHistoryW1.x[i], cfg->autoSetupHistoryW1.y[i],
            cfg->autoSetupHistoryW2.x[i], cfg->autoSetupHistoryW2.y[i]);
    }
    file << ss.str();
}

// ============================================================
// W2の自動設定 (メインロジック)
// ============================================================

void autosetupW2(LiaConfig* cfg) {
    constexpr int RECORD_MS = 3000;

    printf("Autosetup W2 in progress...\n");
    const double original_amp = cfg->awgCfg.ch[0].amp;

    // ------------------------------------------------------------
    // ステップ1: W1の測定 (W1=ON, W2=OFF)
    // ------------------------------------------------------------
    applyAwgSettingsAndWait(cfg, { original_amp, 0.0 }, { 0.0, 0.0 }, RECORD_MS);
    extractRingBufferToHistory(cfg->ringBuffer, cfg->autoSetupHistoryW1, RECORD_MS);

    auto [w1p1, w1p2] = findMaxDistancePoints(cfg->autoSetupHistoryW1.x, cfg->autoSetupHistoryW1.y);
    offsetHistory(cfg->autoSetupHistoryW1, w1p1); // W1は点1をベースにオフセット

    const PolarVector w1 = calculatePolarVector(w1p1, w1p2);
    printf("  w1 abs:%f, theta:%f\n", w1.amplitude, w1.phaseDeg);

    // ------------------------------------------------------------
    // ステップ2: W2の測定 (W1=OFF, W2=ON)
    // ------------------------------------------------------------
    applyAwgSettingsAndWait(cfg, { 0.0, 0.0 }, { original_amp, 0.0 }, RECORD_MS);
    extractRingBufferToHistory(cfg->ringBuffer, cfg->autoSetupHistoryW2, RECORD_MS);

    auto [w2p1, w2p2] = findMaxDistancePoints(cfg->autoSetupHistoryW2.x, cfg->autoSetupHistoryW2.y);
    offsetHistory(cfg->autoSetupHistoryW2, w2p2); // W2は点2をベースにオフセット

    const PolarVector w2 = calculatePolarVector(w2p1, w2p2);
    printf("  w2 abs:%f, theta:%f\n", w2.amplitude, w2.phaseDeg);

    // ------------------------------------------------------------
    // ステップ3: W2の振幅と位相をW1に合わせるように調整
    // ------------------------------------------------------------
    const double target_amp = original_amp * (w1.amplitude / w2.amplitude);
    const double target_phase = w2.phaseDeg - w1.phaseDeg;
    applyAwgSettingsAndWait(cfg, { original_amp, 0.0 }, { target_amp, target_phase }, 0); // 記録なし

    // ステータス更新と結果の保存
    cfg->flagAutoOffset = true;
    cfg->flagAutoSetupW2 = false;
    cfg->flagAutoSetupW2History = true;

    exportHistoryToCsv(cfg);

    printf("  New w2 amp.:%f, theta:%f\n", cfg->awgCfg.ch[1].amp, cfg->awgCfg.ch[1].phase);
    printf("Done.\n");
}