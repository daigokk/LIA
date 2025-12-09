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
constexpr size_t RAW_SIZE = 2*5000;// 50e-6 / RAW_DT;
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
constexpr auto ACFM_SETTINGS_FILE = "acfm.ini";
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
    PostLpFreq = 1604,
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
// Filter Class
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

class LowPassFilter {
private:
    double m_alpha = 1.0;        // 1.0 means no filtering (output = input) by default
    double m_prevOutput = 0.0;   // State: Previous output (y[i-1])
    double m_dt = MEASUREMENT_DT;
    double m_cutoffFrequency = 0.0;
    // m_prevInput is removed as it's not needed for LPF

public:
    void setCutoffFrequency(double newFrequency) {
        if (m_cutoffFrequency == newFrequency) {
            return; // No change needed
        }

        m_cutoffFrequency = newFrequency;

        if (newFrequency <= 0) {
            // Disable filtering (pass-through) logic often implies alpha = 1.0 for LPF
            m_alpha = 1.0;
        }
        else {
            double rc = 1.0 / (2.0 * std::numbers::pi * m_cutoffFrequency);
            // LPF alpha formula: dt / (RC + dt)
            m_alpha = m_dt / (rc + m_dt);
        }
    }

    double process(double input) {
        // Handle initial state or pass-through case
        // If alpha is 1.0, it simply returns the input (no filtering)
        if (m_alpha >= 1.0) {
            m_prevOutput = input;
            return input;
        }

        // Apply the low-pass filter formula (Exponential Moving Average)
        // Formula: y[i] = y[i-1] + alpha * (x[i] - y[i-1])
        // Equivalent to: output = alpha * input + (1 - alpha) * prevOutput
        double output = m_prevOutput + m_alpha * (input - m_prevOutput);

        // Update state for next iteration
        m_prevOutput = output;

        return output;
    }

    // Optional: Helper to reset the filter if needed (e.g. on stream start)
    void reset(double initialValue = 0.0) {
        m_prevOutput = initialValue;
    }
};

//================================================================================
// Main Settings Class
//================================================================================
class LiaConfig {
public:
    // --- Grouped Configuration Structs ---
    struct WindowCfg {
        struct XYint {
            int x, y;
        };
		XYint size = { 1440, 960 };
		XYint pos = { 0, 30 };
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
        float lpFreq = 100;
    };

    struct PlotCfg {
        float limit = 1.5f, rawLimit = 1.5f, historySec = 10.0f;
        bool surfaceMode = false, beep = false, acfm = false;
        float Vh_limit = 1.5f, Vv_limit = 1.5f;
		int idxXYStart = 0, idxXYEnd = 0;
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
    volatile bool statusMeasurement = false;
    volatile bool statusPipe = false;

    struct PauseCfg {
        bool flag = false;
        struct SelectArea {
            struct vec2d {
                double Min = 0.0;
                double Max = 0.0;
            };
            vec2d X, Y;
			int idxXMin = 0, idxXMax = 0;
		} selectArea;
        void set(double xMin, double xMax, double yMin, double yMax) {
            selectArea.X.Min = xMin; selectArea.X.Max = xMax;
            selectArea.Y.Min = yMin; selectArea.Y.Max = yMax;
		}
    };
    PauseCfg pauseCfg;

    // Data Buffers (using std::vector to avoid stack overflow)
    std::vector<double> rawTime;
    std::vector<double> rawData[2];
    std::vector<double> dts;
    struct XYs {
        std::vector<double> x;
        std::vector<double> y;
        
	};
    struct RingBuffer {
        std::vector<double> times;
        XYs ch[2];
        int nofm = 0; // 測定データ数
        int idx = 0; // 最新データが存在するindex
        int tail = 0; // 次にデータを格納するindex
        int size = 0; // 配列における使用要素数
        void resize(int size) {
            times.resize(size);
            ch[0].x.resize(size);
            ch[0].y.resize(size);
            ch[1].x.resize(size);
			ch[1].y.resize(size);
        }
	};
    RingBuffer ringBuffer, xyRingBuffer;

    class ZoomWindowCfg {
        static auto getIdxs(RingBuffer ringBuffer, PauseCfg pauseCfg) {
			int idxStart = -1, idxEnd = -1;
            for (int i = ringBuffer.idx; i >= 0; i--) {
                if (ringBuffer.times[i] <= pauseCfg.selectArea.X.Max) {
                    idxEnd = i;
                }
            }
            if (idxEnd == -1 && ringBuffer.nofm > ringBuffer.size) {
                for (int i = ringBuffer.times.size() - 1; i >= ringBuffer.idx; i--) {
                    if (ringBuffer.times[i] <= pauseCfg.selectArea.X.Max) {
                        idxEnd = i;
                    }
                }
            }
            for (int i = ringBuffer.idx; i >= 0; i--) {
                if (ringBuffer.times[i] <= pauseCfg.selectArea.X.Min) {
                    idxStart = i;
                }
            }
            if (idxStart == -1 && ringBuffer.nofm > ringBuffer.size) {
                for (int i = ringBuffer.times.size() - 1; i >= ringBuffer.idx; i--) {
                    if (ringBuffer.times[i] <= pauseCfg.selectArea.X.Min) {
                        idxStart = i;
                    }
                }
            }
			return std::pair<int, int>{ idxStart, idxEnd };
        }
	};

    struct ACFMData {
        std::vector<double> Vhs = { 0.022,0.023,0.025,0.027,0.058, 0.067 };
        std::vector<double> Vvs = { 0.102,0.106,0.115,0.121,0.212, 0.239 };
        double mmk[3] = { 2.4827, 3.0433, 0.1393 };
        size_t size = 6;
        double ch1vpp = 0;
        double ch2vpp = 0;
    };
    ACFMData acfmData;

private:
    std::string dirName = ".";
    // --- Private Helper Members ---
    struct Hpf{
        HighPassFilter x, y;
    };
    struct Lpf {
        LowPassFilter x, y;
    };
    Hpf hpfCh[2];
    Lpf lpfCh[2];
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
        dts.resize(MEASUREMENT_SIZE); 
        ringBuffer.resize(MEASUREMENT_SIZE);
        xyRingBuffer.resize(XY_SIZE);
        
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
    void setLPFrequency(double freq) {
        post.lpFreq = freq;
        lpfCh[0].x.setCutoffFrequency(freq);
        lpfCh[0].y.setCutoffFrequency(freq);
        lpfCh[1].x.setCutoffFrequency(freq);
        lpfCh[1].y.setCutoffFrequency(freq);
    }
    void reset() {
        awg = AwgCfg();
        post = PostCfg();
        plot = PlotCfg();
        flagCh2 = false;
        flagAutoOffset = false;
        pauseCfg.flag = false;
        setHPFrequency(post.hpFreq);
        setLPFrequency(post.lpFreq);
    }
    void AddPoint(double t, double x, double y) {
        ringBuffer.ch[0].x[ringBuffer.tail] = hpfCh[0].x.process(lpfCh[0].x.process(x));
        ringBuffer.ch[0].y[ringBuffer.tail] = hpfCh[0].y.process(lpfCh[0].y.process(y));
        xyRingBuffer.ch[0].x[xyRingBuffer.tail] = ringBuffer.ch[0].x[ringBuffer.tail];
        xyRingBuffer.ch[0].y[xyRingBuffer.tail] = ringBuffer.ch[0].y[ringBuffer.tail];
        updateRingBuffers(t);
    }

    void AddPoint(double t, double x1, double y1, double x2, double y2) {
        ringBuffer.ch[0].x[ringBuffer.tail] = hpfCh[0].x.process(lpfCh[0].x.process(x1));
        ringBuffer.ch[0].y[ringBuffer.tail] = hpfCh[0].y.process(lpfCh[0].y.process(y1));
        ringBuffer.ch[1].x[ringBuffer.tail] = hpfCh[1].x.process(lpfCh[1].x.process(x2));
        ringBuffer.ch[1].y[ringBuffer.tail] = hpfCh[1].y.process(lpfCh[1].y.process(y2));

        xyRingBuffer.ch[0].x[xyRingBuffer.tail] = ringBuffer.ch[0].x[ringBuffer.tail];
        xyRingBuffer.ch[0].y[xyRingBuffer.tail] = ringBuffer.ch[0].y[ringBuffer.tail];
        xyRingBuffer.ch[1].x[xyRingBuffer.tail] = ringBuffer.ch[1].x[ringBuffer.tail];
        xyRingBuffer.ch[1].y[xyRingBuffer.tail] = ringBuffer.ch[1].y[ringBuffer.tail];
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

        int _size = this->ringBuffer.size;
        if (sec > 0) {
            _size = sec / MEASUREMENT_DT;
            if (_size > this->ringBuffer.size) _size = this->ringBuffer.size;
        }
        int idx = this->ringBuffer.tail - _size;
        if (idx < 0) {
            if (this->ringBuffer.nofm <= MEASUREMENT_SIZE) {
                idx = 0;
                _size = this->ringBuffer.tail;
            }
            else {
                idx += MEASUREMENT_SIZE;
            }
        }
        for (int i = 0; i < _size; ++i) {
            if (!this->flagCh2) {
                ss << std::format("{:e},{:e},{:e}\n", this->ringBuffer.times[idx], this->ringBuffer.ch[0].x[idx], this->ringBuffer.ch[0].y[idx]);
            }
            else {
                ss << std::format("{:e},{:e},{:e},{:e},{:e}\n", this->ringBuffer.times[idx], this->ringBuffer.ch[0].x[idx], this->ringBuffer.ch[0].y[idx], this->ringBuffer.ch[1].x[idx], this->ringBuffer.ch[1].y[idx]);
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
        ringBuffer.times[ringBuffer.tail] = t;
        dts[ringBuffer.tail] = (ringBuffer.nofm > 0) ? (ringBuffer.times[ringBuffer.tail] - ringBuffer.times[ringBuffer.idx]) * 1e3 : 0.0;
        ringBuffer.idx = ringBuffer.tail;
        ringBuffer.nofm++;
        if (++ringBuffer.tail == MEASUREMENT_SIZE) { ringBuffer.tail = 0; }
        ringBuffer.size = std::min(ringBuffer.nofm, (int)MEASUREMENT_SIZE);

        xyRingBuffer.idx = xyRingBuffer.tail;
        xyRingBuffer.nofm++;
        if (++xyRingBuffer.tail == XY_SIZE) { xyRingBuffer.tail = 0; }
        xyRingBuffer.size = std::min(xyRingBuffer.nofm, (int)XY_SIZE);
    }

    void saveSettingsToFile(const std::string& filename = SETTINGS_FILE) const {
        IniWrapper ini;


        ini.set("Window", "pos.x", window.pos.x);
        ini.set("Window", "pos.y", window.pos.y);
        ini.set("Window", "size.x", window.size.x);
        ini.set("Window", "size.y", window.size.y);

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
        ini.set("Post", "lpFreq", post.lpFreq);

        ini.set("Plot", "limit", plot.limit);
        ini.set("Plot", "rawLimit", plot.rawLimit);
        ini.set("Plot", "historySec", plot.historySec);
        ini.set("Plot", "surfaceMode", plot.surfaceMode);
        ini.set("Plot", "beep", plot.beep);
        ini.set("Plot", "acfm", plot.acfm);
		ini.set("Plot", "Vh_limit", plot.Vh_limit);
		ini.set("Plot", "Vv_limit", plot.Vv_limit);

        ini.set("ACFM", "mmk[0]", acfmData.mmk[0]);
        ini.set("ACFM", "mmk[1]", acfmData.mmk[1]);
        ini.set("ACFM", "mmk[2]", acfmData.mmk[2]);

        // Save to file
        ini.save(filename);
    }

    void loadSettingsFromFile(const std::string& filename = SETTINGS_FILE) {

        IniWrapper ini;
        ini.load(filename);

        window.pos.x = ini.get("Window", "pos.x", window.pos.x);
        window.pos.y = ini.get("Window", "pos.y", window.pos.y);
        window.size.x = ini.get("Window", "size.x", window.size.x);
        window.size.y = ini.get("Window", "size.y", window.size.y);

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
        post.lpFreq = ini.get("Post", "lpFreq", post.lpFreq);

        plot.limit = ini.get("Plot", "limit", plot.limit);
        plot.rawLimit = ini.get("Plot", "rawLimit", plot.rawLimit);
        plot.historySec = ini.get("Plot", "historySec", plot.historySec);
        plot.surfaceMode = ini.get("Plot", "surfaceMode", plot.surfaceMode);
        plot.beep = ini.get("Plot", "beep", plot.beep);
        plot.acfm = ini.get("Plot", "acfm", plot.acfm);
		plot.Vh_limit = ini.get("Plot", "Vh_limit", plot.Vh_limit);
		plot.Vv_limit = ini.get("Plot", "Vv_limit", plot.Vv_limit);

        // --- Validate and Clamp Loaded Values ---
        const float lowLimitFreq = 0.5f / (RAW_SIZE * RAW_DT);
        const float highLimitFreq = 1.0f / (1000 * RAW_DT);
        awg.ch[0].freq = std::clamp(awg.ch[0].freq, lowLimitFreq, highLimitFreq);
        awg.ch[1].freq = awg.ch[0].freq; // Sync frequencies
        awg.ch[0].amp = std::clamp(awg.ch[0].amp, 0.1f, 5.0f);
        awg.ch[1].amp = std::clamp(awg.ch[1].amp, 0.0f, 5.0f);
        plot.limit = std::clamp(plot.limit, 0.0f, 5.0f);
        setHPFrequency(post.hpFreq);
        setLPFrequency(post.lpFreq);

        acfmData.mmk[0] = ini.get("ACFM", "mmk[0]", acfmData.mmk[0]);
        acfmData.mmk[1] = ini.get("ACFM", "mmk[1]", acfmData.mmk[1]);
        acfmData.mmk[2] = ini.get("ACFM", "mmk[2]", acfmData.mmk[2]);
    }
};
