#pragma once

#include <algorithm>
#include <array>
#include <atomic> // スレッドセーフかつ高速な状態管理に必須
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

#include "Daq_wf.h"
#include "IniWrapper.h"
#include "Psd.h"
#include "Timer.h"
#include "Filter.h"

// ================================================================================
// Constants
// ================================================================================
constexpr float  RAW_RANGE = 2.5f;
constexpr double RAW_DT = 1.0 / 100e6;
constexpr size_t RAW_SIZE = 5000 * 2;
constexpr double MEASUREMENT_DT = 2e-3;
constexpr size_t MEASUREMENT_SEC = 60 * 10;
constexpr size_t MEASUREMENT_SIZE = static_cast<size_t>(MEASUREMENT_SEC / MEASUREMENT_DT + 1);
//constexpr float  XY_HISTORY_SEC = 10.0f;
//constexpr size_t XY_SIZE = static_cast<size_t>(XY_HISTORY_SEC / MEASUREMENT_DT);

constexpr float LOW_LIMIT_FREQ = static_cast<float>(0.5 / (static_cast<double>(RAW_SIZE) * RAW_DT));
constexpr float HIGH_LIMIT_FREQ = static_cast<float>(1.0 / (1000.0 * RAW_DT));
constexpr float AWG_AMP_MIN = 0.0f;
constexpr float AWG_AMP_MAX = 5.0f;

constexpr auto SETTINGS_FILE = "lia.ini";
constexpr auto ACFM_SETTINGS_FILE = "acfm.ini";
constexpr auto RESULTS_FILE = "ect.csv";
constexpr auto CMDS_FILE = "commands.csv";

constexpr auto CH_VERTICAL = 0;
constexpr auto CH_HORIZONTAL = 1;

// ================================================================================
// Enums & Utilities
// ================================================================================
enum class ButtonType {
    NON = 0, Close = 1001,
    AwgW1Freq = 1011, AwgW1Amp = 1012, AwgW1Phase = 1013,
    AwgW2Freq = 1021, AwgW2Amp = 1022, AwgW2Phase = 1023, AwgW2AutoSetup = 1024,
    PlotLimit = 1101, PostOffset1Phase = 1201, PostOffset2Phase = 1202,
    PlotSurfaceMode = 1301, PlotBeep = 1302,
    PostAutoOffset = 1401, PostOffsetOff = 1402, PostPause = 1403,
    DispCh2 = 1501, PlotACFM = 1502,
    PostHpFreq = 1603, PostLpFreq = 1604,
    RawSave = 2001, RawLimit = 2002,
	XYClear = 3001, XYAutoOffset = 3002, XYPause = 3003, XYRec = 3004,
	TimeHistory = 4001, TimePause = 4002, TimeSave = 4003,
};

constexpr std::string_view cmdToString(ButtonType button) noexcept {
    switch (button) {
    case ButtonType::NON:              return "NON";
    case ButtonType::Close:            return "Close";
    case ButtonType::AwgW1Freq:        return "AwgW1Freq";
    case ButtonType::AwgW1Amp:         return "AwgW1Amp";
    case ButtonType::AwgW1Phase:       return "AwgW1Phase";
    case ButtonType::AwgW2Freq:        return "AwgW2Freq";
    case ButtonType::AwgW2Amp:         return "AwgW2Amp";
    case ButtonType::AwgW2Phase:       return "AwgW2Phase";
    case ButtonType::AwgW2AutoSetup:   return "AwgW2AutoSetup";
    case ButtonType::PlotLimit:        return "PlotLimit";
    case ButtonType::PostOffset1Phase: return "PostOffset1Phase";
    case ButtonType::PostOffset2Phase: return "PostOffset2Phase";
    case ButtonType::PlotSurfaceMode:  return "PlotSurfaceMode";
    case ButtonType::PlotBeep:         return "PlotBeep";
    case ButtonType::PostAutoOffset:   return "PostAutoOffset";
    case ButtonType::PostOffsetOff:    return "PostOffsetOff";
    case ButtonType::PostPause:        return "PostPause";
    case ButtonType::DispCh2:          return "DispCh2";
    case ButtonType::PlotACFM:         return "PlotACFM";
    case ButtonType::PostHpFreq:       return "PostHpFreq";
    case ButtonType::PostLpFreq:       return "PostLpFreq";
    case ButtonType::RawSave:          return "RawSave";
    case ButtonType::RawLimit:         return "RawLimit";
    case ButtonType::XYClear:          return "XYClear";
    case ButtonType::XYAutoOffset:     return "XYAutoOffset";
    case ButtonType::XYPause:          return "XYPause";
    case ButtonType::XYRec:            return "XYRec";
    case ButtonType::TimeHistory:      return "TimeHistory";
    case ButtonType::TimePause:        return "TimePause";
    case ButtonType::TimeSave:         return "TimeSave";
    default:                           return "Unknown";
    }
}

// ================================================================================
// Main Settings Class
// ================================================================================
class LiaConfig {
public:
    struct WindowCfg {
        struct XYint { int x, y; };
        XYint size = { 1440, 960 };
        XYint pos = { 0, 30 };
        float monitorScale = 1.0f;
        float fontSize = 32.0f;
        bool controlWindow = true;
        bool rawWindow = true, xyWindow = true, timeWindow = true, deltaTimeWindow = true, acfmWindow = false;
		bool aboutWindow = false;
    };

    struct ImGuiCfg {
        int theme = 0;
        int windowFlag = 4;
    };

    struct AwgCfg {
        struct Channel {
            bool enable = true;
            int func = funcSine;
            int trigsrc = 0;
            float freq = 100e3f;
            float amp = 1.0f;
            float phase = 0.0f;
        };
        Channel ch[2];
        AwgCfg() { ch[1].amp = 0.0f; }
    };

    struct PostCfg {
        struct Offset { double phase = 0.0, x = 0.0, y = 0.0; };
        Offset offset[2];
        float hpFreq = 0.0f;
        float lpFreq = 100.0f;
    };

    struct PlotCfg {
        float limit = 1.5f, rawLimit = 1.5f, historySec = 10.0f;
        bool surfaceMode = false, beep = false;
        float Vx_limit = 1.5f;
        int size = 0;
        int xyStartIdx = 0, xySize = 0, xyLatestIdx = 0;
    };

    struct PauseCfg {
        bool flag = false;
        struct SelectArea {
            struct vec2d { double Min = 0.0, Max = 0.0; };
            vec2d X, Y;
        } selectArea;

        inline void set(double xMin, double xMax, double yMin, double yMax) noexcept {
            selectArea.X.Min = xMin; selectArea.X.Max = xMax;
            selectArea.Y.Min = yMin; selectArea.Y.Max = yMax;
        }
    };

    struct XYs {
        std::vector<double> x;
        std::vector<double> y;
		void clear() { x.clear(); y.clear(); }
		void push_back(double xVal, double yVal) { x.push_back(xVal); y.push_back(yVal); }
    };

    struct XYRecs {
        XYs ch1xys, ch2xys;
	};
	XYRecs xyRecs;

    struct RingBuffer {
        std::vector<double> times;
        XYs ch[2];
		size_t nofm = 0;  // number of measurements (総測定回数)
		size_t latestIdx = 0;  // リングバッファの最新データのインデックス
		size_t writeIdx = 0;  // リングバッファの書き込み位置のインデックス 
		size_t size = 0;  // リングバッファの有効データ数

        void resize(size_t newSize) {
            times.resize(newSize);
            ch[0].x.resize(newSize); ch[0].y.resize(newSize);
            ch[1].x.resize(newSize); ch[1].y.resize(newSize);
        }
    };

    struct ACFMData {
        std::vector<double> Vhs = { 0.022, 0.023, 0.025, 0.027, 0.058, 0.067 };
        std::vector<double> Vvs = { 0.102, 0.106, 0.115, 0.121, 0.212, 0.239 };
        double mmk[3] = { 2.4827, 3.0433, 0.1393 };
        size_t size = 6;
        double vxpp = 0.0;
        double vpp_vz = 0.0;
    };

    Timer timer;
    Daq_dwf* pDaq = nullptr;
    std::string device_sn = "SN:XXXXXXXXXX";
    std::string dirName = ".";

    WindowCfg windowCfg;
    ImGuiCfg  imguiCfg;
    AwgCfg    awgCfg;
    PostCfg   postCfg;
    PlotCfg   plotCfg;
    PauseCfg  pauseCfg;
    ACFMData  acfmData;

    // スレッドセーフかつ最適化を妨げない atomic 変数へ変更
    bool flagCh2 = false;
    bool flagAutoOffset = false;
    bool flagAutoSetupW2History = false;
    std::atomic<bool> flagAutoSetupW2{ false };
    std::atomic<bool> statusMeasurement{ false };
    std::atomic<bool> statusPipe{ false };

    XYs autoSetupHistoryW1, autoSetupHistoryW2;
    std::vector<std::array<float, 6>> cmds;
    std::vector<double> rawTime;
    std::vector<double> rawData[2];
    std::vector<double> deltaTimes;
    RingBuffer ringBuffer;
    
private:
    Psd psd;
    struct Hpf { HighPassFilter x, y; };
    struct Lpf { LowPassFilter  x, y; };
    Hpf hpfCh[2];
    Lpf lpfCh[2];
    
public:
    LiaConfig() {
        initializeDirectory();
        allocateBuffers();
        loadSettingsFromFile();

        for (size_t i = 0; i < rawTime.size(); ++i) {
            rawTime[i] = i * RAW_DT * 1e6;
        }
    }

    ~LiaConfig() {
        saveResultsToFile();
        saveSettingsToFile();
        saveCmdsToFile();
    }

    void awgStart() {
        if (pDaq) {
            pDaq->awg.start(
                awgCfg.ch[0].freq, awgCfg.ch[0].amp, awgCfg.ch[0].phase,
                awgCfg.ch[1].freq, awgCfg.ch[1].amp, awgCfg.ch[1].phase
            );
        }
    }

    void setHPFrequency(double freq) {
        postCfg.hpFreq = static_cast<float>(freq);
        for (int i = 0; i < 2; ++i) {
            hpfCh[i].x.setCutoffFrequency(freq, MEASUREMENT_DT);
            hpfCh[i].y.setCutoffFrequency(freq, MEASUREMENT_DT);
        }
    }

    void setLPFrequency(double freq) {
        postCfg.lpFreq = static_cast<float>(freq);
        for (int i = 0; i < 2; ++i) {
            lpfCh[i].x.setCutoffFrequency(freq, MEASUREMENT_DT);
            lpfCh[i].y.setCutoffFrequency(freq, MEASUREMENT_DT);
        }
    }

    void reset() {
        awgCfg = AwgCfg();
        postCfg = PostCfg();
        plotCfg = PlotCfg();
        flagCh2 = false;
        flagAutoOffset = false;
        pauseCfg.flag = false;
        setHPFrequency(postCfg.hpFreq);
        setLPFrequency(postCfg.lpFreq);
    }

    // クリティカルパス：inline化による高速化
    inline void AddPoint(double t, double x, double y) noexcept {
        processAndStorePoint(0, x, y);
        updateRingBuffers(t);
    }

    inline void AddPoint(double t, double x1, double y1, double x2, double y2) noexcept {
        processAndStorePoint(0, x1, y1);
        processAndStorePoint(1, x2, y2);
        updateRingBuffers(t);
    }

    inline void update(double t) noexcept {
        if (psd.getCurrentFreq() != awgCfg.ch[0].freq) {
            psd.initialize(awgCfg.ch[0].freq, RAW_DT, rawData[0].size());
        }

        auto [x1, y1] = psd.calculate(rawData[0].data());
        double x2 = 0.0, y2 = 0.0;

        if (flagCh2) {
            std::tie(x2, y2) = psd.calculate(rawData[1].data());
        }

        if (flagAutoOffset) {
            postCfg.offset[0].x = x1;
            postCfg.offset[0].y = y1;
            if (flagCh2) {
                postCfg.offset[1].x = x2;
                postCfg.offset[1].y = y2;
            }
            flagAutoOffset = false;
            if (flagCh2) {
                cmds.push_back(std::array<float, 6>{ (float)timer.elapsedSec(), (float)ButtonType::PostAutoOffset, (float)x1, (float)y1, 0, 0 });
            }
        }

        // 変数をローカルキャッシュしてメモリアクセスを減らす（高速化）
        const double ox1 = postCfg.offset[0].x;
        const double oy1 = postCfg.offset[0].y;
        const double op1 = postCfg.offset[0].phase;

        auto [final_x1, final_y1] = psd.rotate_phase(x1 - ox1, y1 - oy1, op1);

        if (flagCh2) {
            const double ox2 = postCfg.offset[1].x;
            const double oy2 = postCfg.offset[1].y;
            const double op2 = postCfg.offset[1].phase;
            auto [final_x2, final_y2] = psd.rotate_phase(x2 - ox2, y2 - oy2, op2);
            AddPoint(t, final_x1, final_y1, final_x2, final_y2);
        }
        else {
            AddPoint(t, final_x1, final_y1);
        }
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        if (localtime_s(&local_tm, &now_time) != 0) {
            throw std::runtime_error("localtime_s failed");
        }
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y%m%d%H%M%S");
        return oss.str();
    }

    bool saveRawData(const std::string& filename = "raw.csv") const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << (flagCh2 ? "# t(s), ch1(V), ch2(V)\n" : "# t(s), ch1(V)\n");

        // stringstreamの無駄なメモリ確保とコピーを排除し、直接ファイルへ流し込む
        for (size_t i = 0; i < rawData[0].size(); ++i) {
            if (flagCh2) {
                file << std::format("{:e},{:e},{:e}\n", RAW_DT * i, rawData[0][i], rawData[1][i]);
            }
            else {
                file << std::format("{:e},{:e}\n", RAW_DT * i, rawData[0][i]);
            }
        }
        return true;
    }

    bool saveResultsToFile(const std::string& filename = RESULTS_FILE, const double sec = 0) const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << (flagCh2 ? "# t(s), x1(V), y1(V), x2(V), y2(V)\n" : "# t(s), x(V), y(V)\n");

        size_t outputSize = ringBuffer.size;
        if (sec > 0) {
            size_t reqSize = static_cast<size_t>(sec / MEASUREMENT_DT);
            outputSize = (reqSize < ringBuffer.size) ? reqSize : ringBuffer.size;
        }

        // キャストとアンダーフローを安全に処理
        size_t idx = (ringBuffer.writeIdx >= outputSize)
            ? (ringBuffer.writeIdx - outputSize)
            : (MEASUREMENT_SIZE - (outputSize - ringBuffer.writeIdx));

        for (size_t i = 0; i < outputSize; ++i) {
            if (flagCh2) {
                file << std::format("{:e},{:e},{:e},{:e},{:e}\n",
                    ringBuffer.times[idx],
                    ringBuffer.ch[0].x[idx], ringBuffer.ch[0].y[idx],
                    ringBuffer.ch[1].x[idx], ringBuffer.ch[1].y[idx]);
            }
            else {
                file << std::format("{:e},{:e},{:e}\n",
                    ringBuffer.times[idx],
                    ringBuffer.ch[0].x[idx], ringBuffer.ch[0].y[idx]);
            }
            // モジュロ演算（%）を排除し、高速な条件分岐に置換
            if (++idx >= MEASUREMENT_SIZE) idx = 0;
        }
        return true;
    }

    bool saveCmdsToFile(const std::string& filename = CMDS_FILE) const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << "# Time(s), ButtonNo, ButtonName, Value\n";
        for (const auto& cmd : cmds) {
            file << std::format("{:e},{:.0f},{:s},{:e}\n",
                cmd[0], cmd[1], cmdToString(static_cast<ButtonType>(cmd[1])), cmd[2]);
        }
        return true;
    }

    void buttonClear() {
        plotCfg.xyStartIdx = ringBuffer.latestIdx;
        plotCfg.xySize = 0;
        xyRecs.ch1xys.clear(); xyRecs.ch2xys.clear();
        flagAutoSetupW2History = false;
        cmds.push_back(std::array<float, 6>{ (float)timer.elapsedSec(), (float)ButtonType::XYClear, 0, 0, 0, 0 });
    }

    void buttonPause() {
        if (!pauseCfg.flag) {
            pauseCfg.flag = true;
        }
        else {
            buttonClear();
            pauseCfg.flag = false;
        }
        cmds.push_back(std::array<float, 6>{ (float)timer.elapsedSec(), (float)ButtonType::TimePause, (float)pauseCfg.flag, 0, 0, 0 });
	}

    void buttonAutoOffset() {
        flagAutoOffset = true;
	}

    void buttonAutoOffsetOff() {
        flagAutoOffset = false;
        cmds.push_back(std::array<float, 6>{ (float)timer.elapsedSec(), (float)ButtonType::PostOffsetOff, 0, 0, 0, 0 });
	}

    void buttonRec() {
        xyRecs.ch1xys.push_back(ringBuffer.ch[0].x[ringBuffer.latestIdx], ringBuffer.ch[0].y[ringBuffer.latestIdx]);
        xyRecs.ch2xys.push_back(ringBuffer.ch[1].x[ringBuffer.latestIdx], ringBuffer.ch[1].y[ringBuffer.latestIdx]);
        std::ofstream file(std::format("./{}/{}", dirName, "rec.csv"));
        if (!file) {
            std::cerr << "Error: Could not open file " << "rec.csv" << std::endl;
        }
        else {
            std::stringstream ss;
            ss << (flagCh2 ? "# ch1x(V),ch1y(V),ch2x(V),ch2y(V)\n" : "# ch1x(V),ch1y(V)\n");
            for (size_t i = 0; i < xyRecs.ch1xys.x.size(); ++i) {
                ss << (flagCh2 ?
                    std::format("{:e},{:e},{:e},{:e}\n", xyRecs.ch1xys.x[i], xyRecs.ch1xys.y[i], xyRecs.ch2xys.x[i], xyRecs.ch2xys.y[i])
                    : std::format("{:e},{:e}\n", xyRecs.ch1xys.x[i], xyRecs.ch1xys.y[i]));
            }
            file << ss.str(); // メモリからファイルへ一括書き込み
            file.close();
        }
        cmds.push_back(std::array<float, 6>{ (float)timer.elapsedSec(), (float)ButtonType::XYRec, 0, 0, 0, 0 });
    }

private:
    void initializeDirectory() {
        dirName = getCurrentTimestamp();
        try { std::filesystem::create_directory(dirName); }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
    }

    void allocateBuffers() {
        rawTime.resize(RAW_SIZE);
        rawData[0].resize(RAW_SIZE);
        rawData[1].resize(RAW_SIZE);
        deltaTimes.resize(MEASUREMENT_SIZE);
        ringBuffer.resize(MEASUREMENT_SIZE);
    }

    inline void processAndStorePoint(int chIndex, double x, double y) noexcept {
        double processed_x = hpfCh[chIndex].x.process(lpfCh[chIndex].x.process(x));
        double processed_y = hpfCh[chIndex].y.process(lpfCh[chIndex].y.process(y));

        const size_t rTail = ringBuffer.writeIdx;
        ringBuffer.ch[chIndex].x[rTail] = processed_x;
        ringBuffer.ch[chIndex].y[rTail] = processed_y;
    }

    inline void updateRingBuffers(double t) noexcept {
        ringBuffer.times[ringBuffer.writeIdx] = t;
        deltaTimes[ringBuffer.writeIdx] = (ringBuffer.nofm > 0) ? (ringBuffer.times[ringBuffer.writeIdx] - ringBuffer.times[ringBuffer.latestIdx]) * 1e3 : 0.0;

        ringBuffer.latestIdx = ringBuffer.writeIdx;
        ringBuffer.nofm++;
        // モジュロ演算（%）を排除
        if (++ringBuffer.writeIdx >= MEASUREMENT_SIZE) ringBuffer.writeIdx = 0;
        ringBuffer.size = (ringBuffer.nofm < MEASUREMENT_SIZE) ? ringBuffer.nofm : MEASUREMENT_SIZE;

		// XYPlot の表示範囲を更新
        plotCfg.xyLatestIdx = ringBuffer.latestIdx;
        size_t xyMaxSize = (size_t)(plotCfg.historySec / MEASUREMENT_DT);
        if (plotCfg.xySize < xyMaxSize) {
            plotCfg.xySize++;
        }
        else {
            plotCfg.xySize = xyMaxSize;
        }
        int xyStartIdx = plotCfg.xyLatestIdx - plotCfg.xySize + 1;
        if (xyStartIdx >= 0) {
            plotCfg.xyStartIdx = xyStartIdx;
        }
        else {
            plotCfg.xySize = ringBuffer.size - plotCfg.xyStartIdx + ringBuffer.latestIdx + 1;
        }
    }

    void saveSettingsToFile(const std::string& filename = SETTINGS_FILE) const {
        IniWrapper ini;
        ini.set("Window", "pos.x", windowCfg.pos.x);
        ini.set("Window", "pos.y", windowCfg.pos.y);
        ini.set("Window", "size.x", windowCfg.size.x);
        ini.set("Window", "size.y", windowCfg.size.y);
        ini.set("Window", "fontSize", windowCfg.fontSize);
        ini.set("Window", "controlWindow", windowCfg.controlWindow);
        ini.set("Window", "rawWindow", windowCfg.rawWindow);
        ini.set("Window", "xyWindow", windowCfg.xyWindow);
        ini.set("Window", "timeWindow", windowCfg.timeWindow);
        ini.set("Window", "deltaTimeWindow", windowCfg.deltaTimeWindow);
        ini.set("Window", "acfm", windowCfg.acfmWindow);
        ini.set("ImGui", "theme", imguiCfg.theme);
        ini.set("ImGui", "windowFlag", imguiCfg.windowFlag);
        ini.set("Awg", "ch[0].freq", awgCfg.ch[0].freq);
        ini.set("Awg", "ch[0].amp", awgCfg.ch[0].amp);
        ini.set("Awg", "ch[1].amp", awgCfg.ch[1].amp);
        ini.set("Awg", "ch[1].phase", awgCfg.ch[1].phase);
        ini.set("Scope", "flagCh2", flagCh2);
        ini.set("Post", "offset[0].phase", postCfg.offset[0].phase);
        ini.set("Post", "offset[0].x", postCfg.offset[0].x);
        ini.set("Post", "offset[0].y", postCfg.offset[0].y);
        ini.set("Post", "offset[1].phase", postCfg.offset[1].phase);
        ini.set("Post", "offset[1].x", postCfg.offset[1].x);
        ini.set("Post", "offset[1].y", postCfg.offset[1].y);
        ini.set("Post", "hpFreq", postCfg.hpFreq);
        ini.set("Post", "lpFreq", postCfg.lpFreq);
        ini.set("Plot", "limit", plotCfg.limit);
        ini.set("Plot", "rawLimit", plotCfg.rawLimit);
        ini.set("Plot", "historySec", plotCfg.historySec);
        ini.set("Plot", "surfaceMode", plotCfg.surfaceMode);
        ini.set("Plot", "beep", plotCfg.beep);
        ini.set("Plot", "Vx_limt", plotCfg.Vx_limit);
        ini.set("ACFM", "mmk[0]", acfmData.mmk[0]);
        ini.set("ACFM", "mmk[1]", acfmData.mmk[1]);
        ini.set("ACFM", "mmk[2]", acfmData.mmk[2]);
        ini.save(filename);
    }

    void loadSettingsFromFile(const std::string& filename = SETTINGS_FILE) {
        IniWrapper ini;
        ini.load(filename);
        windowCfg.pos.x = ini.get("Window", "pos.x", windowCfg.pos.x);
        windowCfg.pos.y = ini.get("Window", "pos.y", windowCfg.pos.y);
        windowCfg.size.x = ini.get("Window", "size.x", windowCfg.size.x);
        windowCfg.size.y = ini.get("Window", "size.y", windowCfg.size.y);
        windowCfg.fontSize = ini.get("Window", "fontSize", windowCfg.fontSize);
        windowCfg.controlWindow = ini.get("Window", "controlWindow", windowCfg.controlWindow);
        windowCfg.rawWindow = ini.get("Window", "rawWindow", windowCfg.rawWindow);
        windowCfg.xyWindow = ini.get("Window", "xyWindow", windowCfg.xyWindow);
        windowCfg.timeWindow = ini.get("Window", "timeWindow", windowCfg.timeWindow);
        windowCfg.deltaTimeWindow = ini.get("Window", "deltaTimeWindow", windowCfg.deltaTimeWindow);
        windowCfg.acfmWindow = ini.get("Window", "acfmWindow", windowCfg.acfmWindow);
        imguiCfg.theme = ini.get("ImGui", "theme", imguiCfg.theme);
        imguiCfg.windowFlag = ini.get("ImGui", "windowFlag", imguiCfg.windowFlag);
        awgCfg.ch[0].freq = ini.get("Awg", "ch[0].freq", awgCfg.ch[0].freq);
        awgCfg.ch[0].amp = ini.get("Awg", "ch[0].amp", awgCfg.ch[0].amp);
        awgCfg.ch[1].amp = ini.get("Awg", "ch[1].amp", awgCfg.ch[1].amp);
        awgCfg.ch[1].phase = ini.get("Awg", "ch[1].phase", awgCfg.ch[1].phase);
        flagCh2 = ini.get("Scope", "flagCh2", flagCh2);
        postCfg.offset[0].phase = ini.get("Post", "offset[0].phase", postCfg.offset[0].phase);
        postCfg.offset[0].x = ini.get("Post", "offset[0].x", postCfg.offset[0].x);
        postCfg.offset[0].y = ini.get("Post", "offset[0].y", postCfg.offset[0].y);
        postCfg.offset[1].phase = ini.get("Post", "offset[1].phase", postCfg.offset[1].phase);
        postCfg.offset[1].x = ini.get("Post", "offset[1].x", postCfg.offset[1].x);
        postCfg.offset[1].y = ini.get("Post", "offset[1].y", postCfg.offset[1].y);
        postCfg.hpFreq = ini.get("Post", "hpFreq", postCfg.hpFreq);
        postCfg.lpFreq = ini.get("Post", "lpFreq", postCfg.lpFreq);
        plotCfg.limit = ini.get("Plot", "limit", plotCfg.limit);
        plotCfg.rawLimit = ini.get("Plot", "rawLimit", plotCfg.rawLimit);
        plotCfg.historySec = ini.get("Plot", "historySec", plotCfg.historySec);
        plotCfg.surfaceMode = ini.get("Plot", "surfaceMode", plotCfg.surfaceMode);
        plotCfg.beep = ini.get("Plot", "beep", plotCfg.beep);
        plotCfg.Vx_limit = ini.get("Plot", "Vx_limt", plotCfg.Vx_limit);
        acfmData.mmk[0] = ini.get("ACFM", "mmk[0]", acfmData.mmk[0]);
        acfmData.mmk[1] = ini.get("ACFM", "mmk[1]", acfmData.mmk[1]);
        acfmData.mmk[2] = ini.get("ACFM", "mmk[2]", acfmData.mmk[2]);

        awgCfg.ch[0].freq = std::clamp(awgCfg.ch[0].freq, LOW_LIMIT_FREQ, HIGH_LIMIT_FREQ);
        awgCfg.ch[1].freq = awgCfg.ch[0].freq;
        awgCfg.ch[0].amp = std::clamp(awgCfg.ch[0].amp, 0.1f, 5.0f);
        awgCfg.ch[1].amp = std::clamp(awgCfg.ch[1].amp, 0.0f, 5.0f);
        plotCfg.limit = std::clamp(plotCfg.limit, 0.0f, 5.0f);
        setHPFrequency(postCfg.hpFreq);
        setLPFrequency(postCfg.lpFreq);
    }
};