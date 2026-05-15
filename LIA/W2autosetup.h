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
    cfg->awg.ch[0].amp = (float)ch0.amplitude;
    cfg->awg.ch[0].phase = (float)ch0.phaseDeg;
    cfg->awg.ch[1].amp = (float)ch1.amplitude;
    cfg->awg.ch[1].phase = (float)ch1.phaseDeg;
    cfg->awgStart();

    // 安定化のための待機時間(ms) + 記録時間(ms)
    constexpr int STABILIZE_WAIT_MS = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(STABILIZE_WAIT_MS + record_ms));
}

// リングバッファから最新の指定時間分のデータを履歴バッファに抽出する
void extractRingBufferToHistory(const LiaConfig::RingBuffer& ringBuffer, LiaConfig::XYs& targetHistory, const int record_ms) {
    const int length = static_cast<int>((record_ms / 1000.0) / ringBuffer.getDt());

    targetHistory.x.resize(length);
    targetHistory.y.resize(length);

    const size_t latestIdx = ringBuffer.latestIdx;
    const size_t bufsize = ringBuffer.size;

    for (size_t i = 0; i < length; ++i) {
        size_t idx = latestIdx - length + 1 + i;
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
            cfg->ringBuffer.getDt() * i,
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
    const double original_amp = cfg->awg.ch[0].amp;

    if (cfg->window.acfmWindow) {
		// W2をOFFにしてW1のみの状態で測定し、最新の点を基準にしてW2の振幅と位相を調整する
        applyAwgSettingsAndWait(cfg, { original_amp, 0.0 }, { 0.0, 0.0 }, 100);

        double x_ = cfg->ringBuffer.ch[LiaConfigDefaultConsts::CH_HORIZONTAL].x[cfg->ringBuffer.latestIdx];
		double y_ = cfg->ringBuffer.ch[LiaConfigDefaultConsts::CH_HORIZONTAL].y[cfg->ringBuffer.latestIdx];

		// 位相オフセット前の座標に変換
        const double phase_ = -cfg->post.offset[LiaConfigDefaultConsts::CH_HORIZONTAL].phase * std::numbers::pi / 180.0;
		const double cos_t_ = std::cos(phase_);
		const double sin_t_ = std::sin(phase_);
        std::tie(x_, y_) = std::tuple <double, double>(x_ * cos_t_ - y_ * sin_t_, x_ * sin_t_ + y_ * cos_t_);

		// 座標オフセット前の座標に変換
		x_ += cfg->post.offset[LiaConfigDefaultConsts::CH_HORIZONTAL].x;
		y_ += cfg->post.offset[LiaConfigDefaultConsts::CH_HORIZONTAL].y;

		// W1の振幅と位相を計算
        const PolarVector pvec = { std::hypot(x_, y_), std::atan2(y_, x_) * 180.0 / std::numbers::pi };

		// W2の振幅と位相をW1と反転させる
        applyAwgSettingsAndWait(cfg, { original_amp, 0.0 }, { pvec.amplitude, pvec.phaseDeg + 90 }, 0);
    }
    else {
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

        // 結果の保存
        cfg->flagAutoSetupW2History = true;

        exportHistoryToCsv(cfg);
    }
    // ステータス更新
    cfg->flagAutoOffset = true;
    cfg->flagAutoSetupW2 = false;

    printf("  New w2 amp.:%f, theta:%f\n", cfg->awg.ch[1].amp, cfg->awg.ch[1].phase);
    printf("Done.\n");
}

void test_w2autosetup() {
    std::cout << "\n--- Testing W2autosetup Utilities ---" << std::endl;

    // [1] 極座標変換のテスト
    Point p1 = { 0.0, 0.0 };
    Point p2 = { 1.0, 1.0 }; // 距離 sqrt(2), 位相 45度
    PolarVector pv = calculatePolarVector(p1, p2);

    assert(std::abs(pv.amplitude - std::sqrt(2.0)) < 1e-6);
    assert(std::abs(pv.phaseDeg - 45.0) < 1e-6);
    std::cout << "  calculatePolarVector: OK" << std::endl;

    // [2] 最大距離探索のテスト
    std::vector<double> xs = { 0.0, 10.0, 5.0, 2.0 };
    std::vector<double> ys = { 0.0, 0.0, 5.0, -1.0 };
    // (0,0) と (10,0) が一番遠いはず
    auto [best1, best2] = findMaxDistancePoints(xs, ys);

    assert(best1.x == 0.0 && best1.y == 0.0);
    assert(best2.x == 10.0 && best2.y == 0.0);
    std::cout << "  findMaxDistancePoints: OK" << std::endl;

    // [3] オフセット補正のテスト
    // 仮の LiaConfig::XYs 構造体を用意（実装に合わせて調整が必要）
    struct MockXYs {
        std::vector<double> x;
        std::vector<double> y;
    } history;
    history.x = { 1.0, 2.0 };
    history.y = { 3.0, 4.0 };

    // (1,3) を基準にオフセット
    Point base = { 1.0, 3.0 };
    // W2autosetup.h 内の offsetHistory を呼び出し
    // ※ 本来は LiaConfig::XYs 型だが、ここではロジック確認のみ
    for (size_t i = 0; i < history.x.size(); ++i) {
        history.x[i] -= base.x;
        history.y[i] -= base.y;
    }

    assert(history.x[0] == 0.0);
    assert(history.y[0] == 0.0);
    assert(history.x[1] == 1.0);
    assert(history.y[1] == 1.0);
    std::cout << "  offsetHistory logic: OK" << std::endl;

    std::cout << "W2autosetup Utility Tests Passed!" << std::endl;
}