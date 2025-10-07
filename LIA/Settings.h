#pragma once

#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <format>
#include <stdexcept>
#include <cmath>
#include <numbers> // For std::numbers::pi
#include <algorithm> // For std::transform
#include "Daq_wf.h"
#include "inicpp.h"
#include "Timer.h"

enum class ButtonType
{
    NON,
    AwgW1Freq,
    AwgW1Amp,
    AwgW1Phase,
    AwgW2Freq,
    AwgW2Amp,
    AwgW2Phase,
    PlotLimit,
    PostOffset1Phase,
    PostOffset2Phase,
    PlotSurfaceMode,
    PlotBeep,
    PostAutoOffset,
    PostOffsetOff,
    PostPause,
    DispCh2,
    PlotACFM,
    PostHpFreq,
    RawSave,
    RawLimit,
	XYClear,
	XYAutoOffset,
	XYPause,
    TimeHistory,
    TimePause,
};

std::string cmdToString(ButtonType button)
{
    switch (button)
    {
    case ButtonType::NON: return "NON";
    case ButtonType::AwgW1Freq: return "AwgW1Freq";
    case ButtonType::AwgW1Amp: return "AwgW1Amp";
    case ButtonType::AwgW1Phase: return "AwgW1Phase";
    case ButtonType::AwgW2Freq: return "AwgW2Freq";
    case ButtonType::AwgW2Amp: return "AwgW2Amp";
    case ButtonType::AwgW2Phase: return "AwgW2Phase";
    case ButtonType::PlotLimit: return "PlotLimit";
    case ButtonType::PostOffset1Phase: return "PostOffset1Phase";
    case ButtonType::PostOffset2Phase: return "PostOffset2Phase";
    case ButtonType::PlotSurfaceMode: return "PlotSurfaceMode";
    case ButtonType::PlotBeep: return "PlotBeep";
    case ButtonType::PostAutoOffset: return "PostAutoOffset";
    case ButtonType::PostOffsetOff: return "PostOffsetOff";
    case ButtonType::PostPause: return "PostPause";
    case ButtonType::DispCh2: return "DispCh2";
    case ButtonType::PlotACFM: return "PlotACFM";
    case ButtonType::PostHpFreq: return "PostHpFreq";
    case ButtonType::RawSave: return "RawSave";
    case ButtonType::RawLimit: return "RawLimit";
    case ButtonType::XYClear: return "XYClear";
    case ButtonType::XYAutoOffset: return "XYAutoOffset";
    case ButtonType::XYPause: return "XYPause";
	case ButtonType::TimeHistory: return "TimeHistory";
	case ButtonType::TimePause: return "TimePause";
    default: return "Unknown";
    }
}
// --- Constants remain the same ---
constexpr float RAW_RANGE = 2.5f;
constexpr double RAW_DT = 1e-8;
constexpr size_t RAW_SIZE = 5000;
constexpr double MEASUREMENT_DT = 2.0e-3;
constexpr size_t MEASUREMENT_SEC = 60 * 10;
constexpr size_t MEASUREMENT_SIZE = (size_t)(MEASUREMENT_SEC / MEASUREMENT_DT);
constexpr float XY_HISTORY_SEC = 10.0f;
constexpr size_t XY_SIZE = (size_t)(XY_HISTORY_SEC / MEASUREMENT_DT);

//================================================================================
// Refactored HighPassFilter Class
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
class Settings {
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
        double offset1Phase = 0, offset1X = 0, offset1Y = 0;
        double offset2Phase = 0, offset2X = 0, offset2Y = 0;
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
    std::vector<std::array<float,3>> cmds;

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
    std::vector<double> rawData1, rawData2;
    std::vector<double> times, x1s, y1s, x2s, y2s, dts;
    std::vector<double> xy1Xs, xy1Ys, xy2Xs, xy2Ys;

    // Ring Buffer Indices
    int nofm = 0, idx = 0, tail = 0, size = 0;
    int xyNorm = 0, xyIdx = 0, xyTail = 0, xySize = 0;

private:
    // --- Private Helper Members ---
    HighPassFilter hpfX1, hpfY1, hpfX2, hpfY2;
    std::string m_iniFilename = "lia.ini";
    std::string m_resultsFilename = "result.csv";

public:
    Settings() {
        // Allocate memory for data buffers on the heap
        rawTime.resize(RAW_SIZE);
        rawData1.resize(RAW_SIZE);
        rawData2.resize(RAW_SIZE);
        times.resize(MEASUREMENT_SIZE);
        x1s.resize(MEASUREMENT_SIZE);
        y1s.resize(MEASUREMENT_SIZE);
        x2s.resize(MEASUREMENT_SIZE);
        y2s.resize(MEASUREMENT_SIZE);
        dts.resize(MEASUREMENT_SIZE);
        xy1Xs.resize(XY_SIZE);
        xy1Ys.resize(XY_SIZE);
        xy2Xs.resize(XY_SIZE);
        xy2Ys.resize(XY_SIZE);

        loadSettingsFromFile();

        for (size_t i = 0; i < rawTime.size(); ++i) {
            rawTime[i] = i * RAW_DT * 1e6;
        }
    }
    ~Settings() {
        saveResultsToFile();
        saveSettingsToFile();
        saveCmdsToFile();
	}
    // --- Public Methods for Logic and I/O ---

    void AddPoint(double t, double x, double y) {
        x1s[tail] = hpfX1.process(x);
        y1s[tail] = hpfY1.process(y);
        xy1Xs[xyTail] = x1s[tail];
        xy1Ys[xyTail] = y1s[tail];
        updateRingBuffers(t);
    }

    void AddPoint(double t, double x1, double y1, double x2, double y2) {
        x1s[tail] = hpfX1.process(x1);
        y1s[tail] = hpfY1.process(y1);
        x2s[tail] = hpfX2.process(x2);
        y2s[tail] = hpfY2.process(y2);

        xy1Xs[xyTail] = x1s[tail];
        xy1Ys[xyTail] = y1s[tail];
        xy2Xs[xyTail] = x2s[tail];
        xy2Ys[xyTail] = y2s[tail];
        updateRingBuffers(t);
    }

    bool saveRawData(const std::string& filename = "raw.csv") const {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        file << (flagCh2 ? "# t(s), ch1(V), ch2(V)\n" : "# t(s), ch1(V)\n");

        for (size_t i = 0; i < rawData1.size(); ++i) {
            if (flagCh2) {
                file << std::format("{:e},{:e},{:e}\n", RAW_DT * i, rawData1[i], rawData2[i]);
            }
            else {
                file << std::format("{:e},{:e}\n", RAW_DT * i, rawData1[i]);
            }
        }
        return true;
    }

    void saveSettingsToFile() const {
        ini::IniFile liaIni;
        liaIni["Window"]["width"] = window.width;
        liaIni["Window"]["height"] = window.height;
        liaIni["Window"]["posX"] = window.posX;
        liaIni["Window"]["posY"] = window.posY;

        liaIni["ImGui"]["theme"] = imgui.theme;
        liaIni["ImGui"]["windowFlag"] = imgui.windowFlag;

        liaIni["Awg"]["ch[0].freq"] = awg.ch[0].freq;
        liaIni["Awg"]["ch[0].amp"] = awg.ch[0].amp;
        liaIni["Awg"]["ch[1].amp"] = awg.ch[1].amp;
        liaIni["Awg"]["w2Phase"] = awg.ch[1].phase;

        liaIni["Scope"]["flagCh2"] = flagCh2;

        liaIni["Lia"]["offset1Phase"] = post.offset1Phase;
        liaIni["Lia"]["offset1X"] = post.offset1X;
        liaIni["Lia"]["offset1Y"] = post.offset1Y;
        liaIni["Lia"]["offset2Phase"] = post.offset2Phase;
        liaIni["Lia"]["offset2X"] = post.offset2X;
        liaIni["Lia"]["offset2Y"] = post.offset2Y;
        liaIni["Lia"]["hpFreq"] = post.hpFreq;

        liaIni["Plot"]["limit"] = plot.limit;
        liaIni["Plot"]["rawLimit"] = plot.rawLimit;
        liaIni["Plot"]["historySec"] = plot.historySec;
        liaIni["Plot"]["surfaceMode"] = plot.surfaceMode;
        liaIni["Plot"]["beep"] = plot.beep;
        liaIni["Plot"]["acfm"] = plot.acfm;

        liaIni.save(m_iniFilename);
    }

    void saveResultsToFile() const {
        std::ofstream file(m_resultsFilename);
        if (!file) {
            std::cerr << "Error: Could not open file " << m_resultsFilename << std::endl;
            return;
        }

        file << (flagCh2 ? "# t(s), x1(V), y1(V), x2(V), y2(V)\n" : "# t(s), x(V), y(V)\n");

        size_t count = std::min((size_t)nofm, MEASUREMENT_SIZE);
        size_t start_idx = (nofm > MEASUREMENT_SIZE) ? tail : 0;

        for (size_t i = 0; i < count; ++i) {
            size_t idx = (start_idx + i) % MEASUREMENT_SIZE;
            if (flagCh2) {
                file << std::format("{:e},{:e},{:e},{:e},{:e}\n", times[idx], x1s[idx], y1s[idx], x2s[idx], y2s[idx]);
            }
            else {
                file << std::format("{:e},{:e},{:e}\n", times[idx], x1s[idx], y1s[idx]);
            }
        }
    }
    void saveCmdsToFile(const std::string& filename = "commands.csv") const {
		std::ofstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return;
		}
        file << "# Time(s), ButtonNo, ButtonName, Value\n";
        for (const auto& cmd : cmds) {
            file << std::format("{:e},{:.0f},{:s},{:e}\n", cmd[0], cmd[1], cmdToString((ButtonType)cmd[1]), cmd[2]);
        }
	}

private:
    // --- Private Helper Methods ---

    void updateRingBuffers(double t) {
        times[tail] = t;
        dts[tail] = (nofm > 0) ? (times[tail] - times[idx]) * 1e3 : 0.0;
        idx = tail;
        nofm++;
        tail = nofm % MEASUREMENT_SIZE;
        size = std::min(nofm, (int)MEASUREMENT_SIZE);

        xyIdx = xyTail;
        xyNorm++;
        xyTail = xyNorm % XY_SIZE;
        xySize = std::min(xyNorm, (int)XY_SIZE);
    }

    void loadSettingsFromFile() {
        ini::IniFile liaIni(m_iniFilename);

        window.width = loadValue(liaIni, "Window", "width", window.width);
        window.height = loadValue(liaIni, "Window", "height", window.height);
        window.posX = loadValue(liaIni, "Window", "posX", window.posX);
        window.posY = loadValue(liaIni, "Window", "posY", window.posY);

        imgui.theme = loadValue(liaIni, "ImGui", "theme", imgui.theme);
        imgui.windowFlag = loadValue(liaIni, "ImGui", "windowFlag", imgui.windowFlag);

        awg.ch[0].freq = loadValue(liaIni, "Awg", "ch[0].freq", awg.ch[0].freq);
        awg.ch[0].amp = loadValue(liaIni, "Awg", "ch[0].amp", awg.ch[0].amp);
        awg.ch[1].amp = loadValue(liaIni, "Awg", "ch[1].amp", awg.ch[1].amp);
        awg.ch[1].phase = loadValue(liaIni, "Awg", "w2Phase", awg.ch[1].phase);

        flagCh2 = loadValue(liaIni, "Scope", "flagCh2", flagCh2);

        post.offset1Phase = loadValue(liaIni, "Lia", "offset1Phase", post.offset1Phase);
        post.offset1X = loadValue(liaIni, "Lia", "offset1X", post.offset1X);
        post.offset1Y = loadValue(liaIni, "Lia", "offset1Y", post.offset1Y);
        post.offset2Phase = loadValue(liaIni, "Lia", "offset2Phase", post.offset2Phase);
        post.offset2X = loadValue(liaIni, "Lia", "offset2X", post.offset2X);
        post.offset2Y = loadValue(liaIni, "Lia", "offset2Y", post.offset2Y);
        post.hpFreq = loadValue(liaIni, "Lia", "hpFreq", post.hpFreq);

        plot.limit = loadValue(liaIni, "Plot", "limit", plot.limit);
        plot.rawLimit = loadValue(liaIni, "Plot", "rawLimit", plot.rawLimit);
        plot.historySec = loadValue(liaIni, "Plot", "historySec", plot.historySec);
        plot.surfaceMode = loadValue(liaIni, "Plot", "surfaceMode", plot.surfaceMode);
        plot.beep = loadValue(liaIni, "Plot", "beep", plot.beep);
        plot.acfm = loadValue(liaIni, "Plot", "acfm", plot.acfm);

        // --- Validate and Clamp Loaded Values ---
        const float lowLimitFreq = 0.5f / (RAW_SIZE * RAW_DT);
        const float highLimitFreq = 1.0f / (1000 * RAW_DT);
        awg.ch[0].freq = std::clamp(awg.ch[0].freq, lowLimitFreq, highLimitFreq);
        awg.ch[1].freq = awg.ch[0].freq; // Sync frequencies
        awg.ch[0].amp = std::clamp(awg.ch[0].amp, 0.1f, 5.0f);
        awg.ch[1].amp = std::clamp(awg.ch[1].amp, 0.0f, 5.0f);
        plot.limit = std::clamp(plot.limit, 0.0f, 5.0f);
        hpfX1.setCutoffFrequency(post.hpFreq);
        hpfY1.setCutoffFrequency(post.hpFreq);
        hpfX2.setCutoffFrequency(post.hpFreq);
        hpfY2.setCutoffFrequency(post.hpFreq);
    }

    // --- Template and static helpers for INI parsing ---

    template<typename T>
    T loadValue(ini::IniFile& ini, const std::string& section, const std::string& key, T defaultValue) {
        static_assert(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, bool>);
        const std::string& valStr = ini[section][key].as<std::string>();
        if (valStr.empty()) return defaultValue;

        if constexpr (std::is_same_v<T, bool>) {
            return convb(valStr, defaultValue);
        }
        else {
            return static_cast<T>(conv(valStr, static_cast<double>(defaultValue)));
        }
    }

    static double conv(const std::string& str, double defval) {
        try {
            return std::stod(str);
        }
        catch (const std::logic_error&) {
            // Catches both invalid_argument and out_of_range
            return defval;
        }
    }

    static bool convb(std::string str, bool defval) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        if (str == "true") return true;
        if (str == "false") return false;
        return defval;
    }
};