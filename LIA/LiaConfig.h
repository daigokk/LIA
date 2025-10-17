#pragma once

#include <algorithm> // For std::transform
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <map> // cmdToString でstd::map を使用
#include <iomanip> // std::put_time
#include <iostream>
#include <numbers> // For std::numbers::pi
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view> // 文字列定数に string_view を使用
#include <vector>

#include "Daq_wf.h"
#include "Psd.h"
#include "Timer.h"
#include "IniWrapper.h"

// --- Constants remain the same ---
constexpr float RAW_RANGE = 2.5f;
constexpr double RAW_DT = 1.0 / 100e6;
constexpr size_t RAW_SIZE = 5000;// 50e-6 / RAW_DT;
constexpr double MEASUREMENT_DT = 2.0e-3;
constexpr size_t MEASUREMENT_SEC = 60 * 10;
constexpr size_t MEASUREMENT_SIZE = (size_t)(MEASUREMENT_SEC / MEASUREMENT_DT);
constexpr float XY_HISTORY_SEC = 10.0f;
constexpr size_t XY_SIZE = (size_t)(XY_HISTORY_SEC / MEASUREMENT_DT);

constexpr float LOW_LIMIT_FREQ = 0.5 / (RAW_SIZE * RAW_DT);
constexpr float HIGH_LIMIT_FREQ = 1.0 / (1000 * RAW_DT);
constexpr float AWG_AMP_MIN = 0.0;
constexpr float AWG_AMP_MAX = 5.0;

// --- Constants for file names ---
constexpr auto SETTINGS_FILE = "lia.ini";
constexpr auto RESULTS_FILE = "ect.csv";
constexpr auto CMDS_FILE = "commands.csv";

enum class ButtonType
{
    NON = 0,
    Close = 1001,
    AwgW1Freq = 1011,
    AwgW1Amp = 1012,
    AwgW1Phase = 1013,
    AwgW2Freq = 1021,
    AwgW2Amp = 1022,
    AwgW2Phase = 1023,
    PlotLimit = 1101,
    PostOffset1Phase = 1201,
    PostOffset2Phase = 1202,
    PlotSurfaceMode = 1301,
    PlotBeep = 1302,
    PostAutoOffset = 1401,
    PostOffsetOff = 1402,
    PostPause = 1403,
    DispCh2 = 1501,
    PlotACFM = 1502,
    PostHpFreq = 1603,
    RawSave = 2001,
    RawLimit = 2002,
    XYClear = 3001,
    XYAutoOffset = 3002,
    XYPause = 3003,
    TimeHistory = 4001,
    TimePause = 4002,
};

inline std::string_view cmdToString(ButtonType button)
{
    static const std::map<ButtonType, std::string_view> buttonMap = {
        {ButtonType::NON, "NON"},
        {ButtonType::Close, "Close"},
        {ButtonType::AwgW1Freq, "AwgW1Freq"},
        {ButtonType::AwgW1Amp, "AwgW1Amp"},
        {ButtonType::AwgW1Phase, "AwgW1Phase"},
        {ButtonType::AwgW2Freq, "AwgW2Freq"},
        {ButtonType::AwgW2Amp, "AwgW2Amp"},
        {ButtonType::AwgW2Phase, "AwgW2Phase"},
        {ButtonType::PlotLimit, "PlotLimit"},
        {ButtonType::PostOffset1Phase, "PostOffset1Phase"},
        {ButtonType::PostOffset2Phase, "PostOffset2Phase"},
        {ButtonType::PlotSurfaceMode, "PlotSurfaceMode"},
        {ButtonType::PlotBeep, "PlotBeep"},
        {ButtonType::PostAutoOffset, "PostAutoOffset"},
        {ButtonType::PostOffsetOff, "PostOffsetOff"},
        {ButtonType::PostPause, "PostPause"},
        {ButtonType::DispCh2, "DispCh2"},
        {ButtonType::PlotACFM, "PlotACFM"},
        {ButtonType::PostHpFreq, "PostHpFreq"},
        {ButtonType::RawSave, "RawSave"},
        {ButtonType::RawLimit, "RawLimit"},
        {ButtonType::XYClear, "XYClear"},
        {ButtonType::XYAutoOffset, "XYAutoOffset"},
        {ButtonType::XYPause, "XYPause"},
        {ButtonType::TimeHistory, "TimeHistory"},
        {ButtonType::TimePause, "TimePause"}
    };
    auto it = buttonMap.find(button);
    return (it != buttonMap.end()) ? it->second : "Unknown";
}

//================================================================================
// HighPassFilter Class
//================================================================================
class HighPassFilter {
private:
    double m_alpha = 1.0;
    double m_prevInput = 0.0;
    double m_prevOutput = 0.0;
    double m_dt = MEASUREMENT_DT;
    double m_cutoffFrequency = 0.0;

public:
    void setCutoffFrequency(double newFrequency) {
        if (m_cutoffFrequency == newFrequency) {
            return; // No change needed
        }

        if (newFrequency <= 0) {
            m_cutoffFrequency = 0.0;
            m_alpha = 1.0;
        }
        else {
            m_cutoffFrequency = newFrequency;
            double rc = 1.0 / (2.0 * std::numbers::pi * m_cutoffFrequency);
            m_alpha = rc / (rc + m_dt);
        }
    }

    double process(double input) {
        if (m_cutoffFrequency == 0.0) {
            m_prevInput = input;
            m_prevOutput = input;
            return input;
        }

        // Apply the high-pass filter formula
        double output = m_alpha * (m_prevOutput + input - m_prevInput);

        // Update state for next iteration
        m_prevInput = input;
        m_prevOutput = output;

        return output;
    }
};

//================================================================================
// Main Settings Class
//================================================================================
class LiaConfig {
public:
    // --- Grouped Configuration Structs ---
    struct WindowCfg {
        int width = 1440;
        int height = 960;
        int posX = 0;
        int posY = 30;
        float monitorScale = 1.0f; // <<< ここに再度追加します
    };

    struct ImGuiCfg {
        int theme = 0;
        int windowFlag = 4; // ImGuiCond_FirstUseEver
    };

    struct AwgCfg {
        struct Channel {
            bool enable = true;
            int func = funcSine;
            int trigsrc = 0; // None
            float freq = 100e3;
            float amp = 1.0;
            float phase = 0.0;
        };
        Channel ch[2];
        AwgCfg() : ch{ Channel(), Channel() } {
            ch[1].amp = 0.0f; // Default amp for channel 2 is 0V
        }
    };
    struct PostCfg {
        struct Offset {
            double phase = 0, x = 0, y = 0;
        };
        Offset offset[2];
        float hpFreq = 0;
    };

    struct PlotCfg {
        float limit = 1.5f, rawLimit = 1.5f, historySec = 10.0f;
        bool surfaceMode = false, beep = false, acfm = false;
    };

    // --- Public Members ---
    Timer timer;
    Daq_dwf* pDaq = nullptr;
    std::string device_sn = "SN:XXXXXXXXXX";
    std::vector<std::array<float, 3>> cmds;

    // Configuration Data
    WindowCfg window;
    ImGuiCfg imgui;
    AwgCfg awg;
    PostCfg post;
    PlotCfg plot;

    // State Flags
    bool flagCh2 = false;
    bool flagAutoOffset = false;
    bool flagPause = false;
    volatile bool statusMeasurement = false;
    volatile bool statusPipe = false;

    // Data Buffers (using std::vector to avoid stack overflow)
    std::vector<double> rawTime;
    std::vector<double> rawData[2];
    std::vector<double> times, x1s, y1s, x2s, y2s, dts;
    struct XYSforXYWindow {
        std::vector<double> x;
        std::vector<double> y;
	};
    XYSforXYWindow xyForXY[2];

    // Ring Buffer Indices
    int nofm = 0, idx = 0, tail = 0, size = 0;
    int xyNorm = 0, xyIdx = 0, xyTail = 0, xySize = 0;

private:
    std::string dirName = ".";
    // --- Private Helper Members ---
    struct Hpf{
        HighPassFilter x, y;
    };
    Hpf hpfCh[2];
    Psd psd;

public:
    LiaConfig() {

        dirName = getCurrentTimestamp();
        try {
            if (std::filesystem::create_directory(dirName)) {
                //std::cout << "ディレクトリを作成しました: " << dirName << std::endl;
            }
            else {
                //std::cout << "ディレクトリの作成に失敗しました（既に存在している可能性あり）" << std::endl;
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }

        // Allocate memory for data buffers on the heap
        rawTime.resize(RAW_SIZE);
        rawData[0].resize(RAW_SIZE);
        rawData[1].resize(RAW_SIZE);
        times.resize(MEASUREMENT_SIZE);
        x1s.resize(MEASUREMENT_SIZE);
        y1s.resize(MEASUREMENT_SIZE);
        x2s.resize(MEASUREMENT_SIZE);
        y2s.resize(MEASUREMENT_SIZE);
        dts.resize(MEASUREMENT_SIZE);
        xyForXY[0].x.resize(XY_SIZE);
        xyForXY[0].y.resize(XY_SIZE);
        xyForXY[1].x.resize(XY_SIZE);
        xyForXY[1].y.resize(XY_SIZE);

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
    // --- Public Methods for Logic and I/O ---
    void setHPFrequency(double freq) {
        post.hpFreq = freq;
        hpfCh[0].x.setCutoffFrequency(freq);
        hpfCh[0].y.setCutoffFrequency(freq);
        hpfCh[1].x.setCutoffFrequency(freq);
        hpfCh[1].y.setCutoffFrequency(freq);
    }
    void reset() {
        awg = AwgCfg();
        post = PostCfg();
        plot = PlotCfg();
        flagCh2 = false;
        flagAutoOffset = false;
        flagPause = false;
        hpfCh[0].x.setCutoffFrequency(post.hpFreq);
        hpfCh[0].y.setCutoffFrequency(post.hpFreq);
        hpfCh[1].x.setCutoffFrequency(post.hpFreq);
        hpfCh[1].y.setCutoffFrequency(post.hpFreq);
    }
    void AddPoint(double t, double x, double y) {
        x1s[tail] = hpfCh[0].x.process(x);
        y1s[tail] = hpfCh[0].y.process(y);
        xyForXY[0].x[xyTail] = x1s[tail];
        xyForXY[0].y[xyTail] = y1s[tail];
        updateRingBuffers(t);
    }

    void AddPoint(double t, double x1, double y1, double x2, double y2) {
        x1s[tail] = hpfCh[0].x.process(x1);
        y1s[tail] = hpfCh[0].y.process(y1);
        x2s[tail] = hpfCh[1].x.process(x2);
        y2s[tail] = hpfCh[1].y.process(y2);

        xyForXY[0].x[xyTail] = x1s[tail];
        xyForXY[0].y[xyTail] = y1s[tail];
        xyForXY[1].x[xyTail] = x2s[tail];
        xyForXY[1].y[xyTail] = y2s[tail];
        updateRingBuffers(t);
    }

    void update(double t) {
        if (psd.currentFreq != awg.ch[0].freq) {
            psd.initialize(awg.ch[0].freq, RAW_DT, rawData[0].size());
        }

        auto [x1, y1] = psd.calculate(rawData[0].data());

        // flagCh2がfalseの場合に不要なオブジェクト構築を回避
        double x2 = 0.0, y2 = 0.0;
        if (flagCh2) {
            std::tie(x2, y2) = psd.calculate(rawData[1].data());
        }

        if (flagAutoOffset) {
            post.offset[0].x = x1; post.offset[0].y = y1;
            if (flagCh2) {
                post.offset[1].x = x2; post.offset[1].y = y2;
            }
            flagAutoOffset = false;
        }

        x1 -= post.offset[0].x;
        y1 -= post.offset[0].y;
        auto [final_x1, final_y1] = psd.rotate_phase(x1, y1, post.offset[0].phase);

        if (flagCh2) {
            x2 -= post.offset[1].x;
            y2 -= post.offset[1].y;
            auto [final_x2, final_y2] = psd.rotate_phase(x2, y2, post.offset[1].phase);
            AddPoint(t, final_x1, final_y1, final_x2, final_y2);
        }
        else {
            AddPoint(t, final_x1, final_y1);
        }
    }

    std::string getCurrentTimestamp() const {
        // 現在時刻を取得
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        // std::tm に変換（ローカルタイム）
        std::tm local_tm;
        // 安全な localtime_s を使用（Windows環境）
        if (localtime_s(&local_tm, &now_time) != 0) {
            throw std::runtime_error("localtime_s failed");
        }
        // 文字列に整形
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y%m%d%H%M%S");
        return oss.str();
    }

    bool saveRawData(const std::string& filename = "raw.csv") const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        std::stringstream ss;
        ss << (flagCh2 ? "# t(s), ch1(V), ch2(V)\n" : "# t(s), ch1(V)\n");

        for (size_t i = 0; i < rawData[0].size(); ++i) {
            if (flagCh2) {
                ss << std::format("{:e},{:e},{:e}\n", RAW_DT * i, rawData[0][i], rawData[1][i]);
            }
            else {
                ss << std::format("{:e},{:e}\n", RAW_DT * i, rawData[0][i]);
            }
        }
        file << ss.str(); // メモリからファイルへ一括書き込み
        file.close();
        return true;
    }

    bool saveResultsToFile(const std::string& filename = RESULTS_FILE, const double sec = 0) const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) {
            std::cerr << std::format("Error: Could not open file: {}\n", filename);
            return false;
        }

        std::stringstream ss;
        ss << (flagCh2 ? "# t(s), x1(V), y1(V), x2(V), y2(V)\n" : "# t(s), x(V), y(V)\n");

        int _size = this->size;
        if (sec > 0) {
            _size = sec / MEASUREMENT_DT;
            if (_size > this->size) _size = this->size;
        }
        int idx = this->tail - _size;
        if (idx < 0) {
            if (this->nofm <= MEASUREMENT_SIZE) {
                idx = 0;
                _size = this->tail;
            }
            else {
                idx += MEASUREMENT_SIZE;
            }
        }
        for (int i = 0; i < _size; ++i) {
            if (!this->flagCh2) {
                ss << std::format("{:e},{:e},{:e}\n", this->times[idx], this->x1s[idx], this->y1s[idx]);
            }
            else {
                ss << std::format("{:e},{:e},{:e},{:e},{:e}\n", this->times[idx], this->x1s[idx], this->y1s[idx], this->x2s[idx], this->y2s[idx]);
            }
            idx = (idx + 1) % MEASUREMENT_SIZE;
        }
        file << ss.str(); // メモリからファイルへ一括書き込み
        file.close();
        return true;
    }

    bool saveCmdsToFile(const std::string& filename = CMDS_FILE) const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }
        file << "# Time(s), ButtonNo, ButtonName, Value\n";
        for (const auto& cmd : cmds) {
            file << std::format("{:e},{:.0f},{:s},{:e}\n", cmd[0], cmd[1], cmdToString((ButtonType)cmd[1]), cmd[2]);
        }
        file.close();
        return true;
    }

private:
    // --- Private Helper Methods ---
    void updateRingBuffers(double t) {
        times[tail] = t;
        dts[tail] = (nofm > 0) ? (times[tail] - times[idx]) * 1e3 : 0.0;
        idx = tail;
        nofm++;
        if (++tail == MEASUREMENT_SIZE) { tail = 0; }
        size = std::min(nofm, (int)MEASUREMENT_SIZE);

        xyIdx = xyTail;
        xyNorm++;
        if (++xyTail == XY_SIZE) { xyTail = 0; }
        xySize = std::min(xyNorm, (int)XY_SIZE);
    }

    void saveSettingsToFile(const std::string& filename = SETTINGS_FILE) const {
        IniWrapper ini;


        ini.set("Window", "posX", window.posX);
        ini.set("Window", "posY", window.posY);
        ini.set("Window", "width", window.width);
        ini.set("Window", "height", window.height);

        ini.set("ImGui", "theme", imgui.theme);
        ini.set("ImGui", "windowFlag", imgui.windowFlag);

        ini.set("Awg", "ch[0].freq", awg.ch[0].freq);
        ini.set("Awg", "ch[0].amp", awg.ch[0].amp);
        ini.set("Awg", "ch[1].amp", awg.ch[1].amp);
        ini.set("Awg", "ch[1].phase", awg.ch[1].phase);

        ini.set("Scope", "flagCh2", flagCh2);

        ini.set("Post", "offset[0].phase", post.offset[0].phase);
        ini.set("Post", "offset[0].x", post.offset[0].x);
        ini.set("Post", "offset[0].y", post.offset[0].y);
        ini.set("Post", "offset[1].phase", post.offset[1].phase);
        ini.set("Post", "offset[1].x", post.offset[1].x);
        ini.set("Post", "offset[1].y", post.offset[1].y);
        ini.set("Post", "hpFreq", post.hpFreq);

        ini.set("Plot", "limit", plot.limit);
        ini.set("Plot", "rawLimit", plot.rawLimit);
        ini.set("Plot", "historySec", plot.historySec);
        ini.set("Plot", "surfaceMode", plot.surfaceMode);
        ini.set("Plot", "beep", plot.beep);
        ini.set("Plot", "acfm", plot.acfm);
        // Save to file
        ini.save(SETTINGS_FILE);
    }

    void loadSettingsFromFile(const std::string& filename = SETTINGS_FILE) {

        IniWrapper ini;
        ini.load(SETTINGS_FILE);

        window.posX = ini.get("Window", "posX", window.posX);
        window.posY = ini.get("Window", "posY", window.posY);
        window.width = ini.get("Window", "Width", window.width);
        window.height = ini.get("Window", "Height", window.height);

        imgui.theme = ini.get("ImGui", "theme", imgui.theme);
        imgui.windowFlag = ini.get("ImGui", "windowFlag", imgui.windowFlag);

        awg.ch[0].freq = ini.get("Awg", "ch[0].freq", awg.ch[0].freq);
        awg.ch[0].amp = ini.get("Awg", "ch[0].amp", awg.ch[0].amp);
        awg.ch[1].amp = ini.get("Awg", "ch[1].amp", awg.ch[1].amp);
        awg.ch[1].phase = ini.get("Awg", "ch[1].phase", awg.ch[1].phase);

        flagCh2 = ini.get("Scope", "flagCh2", flagCh2);

        post.offset[0].phase = ini.get("Post", "offset[0].phase", post.offset[0].phase);
        post.offset[0].x = ini.get("Post", "offset[0].x", post.offset[0].x);
        post.offset[0].y = ini.get("Post", "offset[0].y", post.offset[0].y);
        post.offset[1].phase = ini.get("Post", "offset[1].phase", post.offset[1].phase);
        post.offset[1].x = ini.get("Post", "offset[1].x", post.offset[1].x);
        post.offset[1].y = ini.get("Post", "offset[1].y", post.offset[1].y);
        post.hpFreq = ini.get("Post", "hpFreq", post.hpFreq);

        plot.limit = ini.get("Plot", "limit", plot.limit);
        plot.rawLimit = ini.get("Plot", "rawLimit", plot.rawLimit);
        plot.historySec = ini.get("Plot", "historySec", plot.historySec);
        plot.surfaceMode = ini.get("Plot", "surfaceMode", plot.surfaceMode);
        plot.beep = ini.get("Plot", "beep", plot.beep);
        plot.acfm = ini.get("Plot", "acfm", plot.acfm);

        // --- Validate and Clamp Loaded Values ---
        const float lowLimitFreq = 0.5f / (RAW_SIZE * RAW_DT);
        const float highLimitFreq = 1.0f / (1000 * RAW_DT);
        awg.ch[0].freq = std::clamp(awg.ch[0].freq, lowLimitFreq, highLimitFreq);
        awg.ch[1].freq = awg.ch[0].freq; // Sync frequencies
        awg.ch[0].amp = std::clamp(awg.ch[0].amp, 0.1f, 5.0f);
        awg.ch[1].amp = std::clamp(awg.ch[1].amp, 0.0f, 5.0f);
        plot.limit = std::clamp(plot.limit, 0.0f, 5.0f);
        hpfCh[0].x.setCutoffFrequency(post.hpFreq);
        hpfCh[0].y.setCutoffFrequency(post.hpFreq);
        hpfCh[1].x.setCutoffFrequency(post.hpFreq);
        hpfCh[1].y.setCutoffFrequency(post.hpFreq);
    }
};
