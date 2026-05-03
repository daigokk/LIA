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

    // ステップ1: W1をON、W2をOFF で振幅と位相を測定
    PolarVector w1 = measureAwgResponse(cfg, original_amp, 0.0, RECORD_MS);
    printf("  w1 abs:%f, theta:%f\n", w1.amplitude, w1.phaseDeg);

    // ステップ2: W1をOFF、W2をON で振幅と位相を測定
    PolarVector w2 = measureAwgResponse(cfg, 0.0, original_amp, RECORD_MS);
    printf("  w2 abs:%f, theta:%f\n", w2.amplitude, w2.phaseDeg);

    // ステップ3: W2の振幅と位相をW1に合わせるように調整
    cfg->awgCfg.ch[0].amp = original_amp;
    cfg->awgCfg.ch[1].amp = original_amp * (w1.amplitude / w2.amplitude);
    cfg->awgCfg.ch[1].phase = w2.phaseDeg - w1.phaseDeg;
    cfg->awgStart();

    // 安定化の待機
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

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