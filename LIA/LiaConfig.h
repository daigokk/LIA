#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
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
#include "pocketfft_hdronly.h"

// ================================================================================
// Constants
// ================================================================================
namespace LiaConfigDefaultConsts {
    constexpr float AWG_FREQ = 100e3f;
	constexpr float AWG_AMP = 1.0f;
	constexpr float SCOPE_RANGE = 2.5f;
    constexpr double SCOPE_DT = 1e-8; // 100 MHz sampling rate
	constexpr int SCOPE_BUFFER_SIZE = 10000; // 0.1ms分のデータを保存
    constexpr double RINGBUFFER_DT = 2e-3;
	constexpr int RINGBUFFER_SEC = 60 * 10; // 10 minutes

    constexpr auto SETTINGS_FILE = "lia.ini";
    constexpr auto ACFM_SETTINGS_FILE = "acfm.ini";
    constexpr auto RESULTS_FILE = "ect.csv";
    constexpr auto CMDS_FILE = "commands.csv";

    constexpr auto CH_VERTICAL = 0;
    constexpr auto CH_HORIZONTAL = 1;
}

// ================================================================================
// Enums & Utilities
// ================================================================================
enum class ButtonType {
    NON = 0, Close = 1001,
    AwgW1Freq = 1011, AwgW1Amp = 1012, AwgW1Phase = 1013, AwgW1Func = 1014,
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
// Core Application State & Config
// ================================================================================
class LiaConfig {
public:
    // ---------------------------------------------------------
    // [1] Settings & Configurations
    // ---------------------------------------------------------
    struct WindowCfg {
        struct XYint { int x, y; };
        XYint size = { 1440, 960 };
        XYint pos = { 0, 30 };
        float monitorScale = 1.0f;
        float fontSize = 32.0f;
        bool controlWindow = true;
        bool rawWindow = true, xyWindow = true, timeWindow = true, deltaTimeWindow = true, acfmWindow = false;
        bool aboutWindow = false;
        int theme = 3;
        int imGuiCondFlag = 4;  // ImGuiCond_FirstUseEver
        int imGuiWindowFlag = 0;
        int imPlotFlag = 4;     // ImPlotFlags_NoMouseText
    } window;

    struct AwgCfg {
        struct Channel {
            bool enable = true;
            int func = funcSine;
            int trigsrc = 0;
            float freq = LiaConfigDefaultConsts::AWG_FREQ;
            float amp = LiaConfigDefaultConsts::AWG_AMP;
            float phase = 0.0f;
        };
        Channel ch[2];
        const float AWG_AMP_MIN = 0.0f;
        const float AWG_AMP_MAX = 5.0f;
        AwgCfg() { ch[1].amp = 0.0f; }
        void reset() {
            ch[0] = Channel();
            ch[1] = Channel();
        }
    } awg;

    struct ScopeCfg {
    public:
        struct Channel {
            float range = LiaConfigDefaultConsts::SCOPE_RANGE;
        };
        std::vector<Channel> ch;
        float getMaxRange() { return maxRange; }
        int getNumChannels() const { return static_cast<int>(ch.size()); }
		int getBufferSize() const { return bufferSize; }
        double getSamplingDt() const { return samplingDt; }
        float getLowLimitFreq() const { return lowLimitFreq; }
        float getHighLimitFreq() const { return highLimitFreq; }
		ScopeCfg(int numChannels = 2) : ch(numChannels) {
			updateFreqLimits(bufferSize, samplingDt);
        }
		void updateFreqLimits(const int bufferSize_, const double samplingDt_) {
			bufferSize = bufferSize_;
			samplingDt = samplingDt_;
            // ナイキスト周波数や分解能に基づく制限計算
            // 0.5 / (Size * dt) は、このバッファ長で観測できる最低周波数（の半分）
            lowLimitFreq = static_cast<float>(0.5f / (static_cast<float>(bufferSize) * samplingDt));
            // 1.0 / (100 * dt) は、サンプリングレートの 1/100
            highLimitFreq = static_cast<float>(1.0f / (1000.0f * samplingDt));
		}
        void setMaxRange() {
			maxRange = 0.0f;
			for (auto& channel : ch) {
				if (maxRange < channel.range) {
					maxRange = channel.range;
				}
			}
        }
        void reset() {
            for (auto& channel : ch) {
                channel.range = LiaConfigDefaultConsts::SCOPE_RANGE;
            }
            setMaxRange();
			updateFreqLimits(bufferSize, samplingDt);
        }
    private:
        double samplingDt = LiaConfigDefaultConsts::SCOPE_DT;
        int bufferSize = LiaConfigDefaultConsts::SCOPE_BUFFER_SIZE;
        float lowLimitFreq;
        float highLimitFreq;
		float maxRange = LiaConfigDefaultConsts::SCOPE_RANGE;
    } scope;

    struct PostCfg {
        struct Offset { double phase = 0.0, x = 0.0, y = 0.0; };
        Offset offset[2];
        float hpFreq = 0.0f;
        float lpFreq = 100.0f;
		void reset() {
			offset[0] = Offset();
			offset[1] = Offset();
			hpFreq = 0.0f;
			lpFreq = 100.0f;
		}
    } post;

    struct PlotCfg {
        float limit = 1.5f, rawLimit = 1.5f, historySec = 10.0f;
        bool surfaceMode = false, beep = false;
        float Vx_limit = 1.5f;
        int size = 0;
        int xyStartIdx = 0, xySize = 0, xyLatestIdx = 0;
		void reset() {
			limit = 1.5f;
			rawLimit = 1.5f;
			historySec = 10.0f;
			surfaceMode = false;
			beep = false;
			Vx_limit = 1.5f;
			size = 0;
			xyStartIdx = 0;
			xySize = 0;
			xyLatestIdx = 0;
		}
    } plot;

    struct PauseCfg {
        bool flag = false;
        struct SelectArea {
            struct vec2d { double Min = 0.0, Max = 0.0; };
            vec2d X, Y;
        } selectArea;

        inline void set(double xMin, double xMax, double yMin, double yMax) noexcept {
            selectArea.X = { xMin, xMax };
            selectArea.Y = { yMin, yMax };
        }
    } pause;

    // ---------------------------------------------------------
    // [2] Data Buffers & Containers
    // ---------------------------------------------------------
    struct XYs {
        std::vector<double> x;
        std::vector<double> y;
        void clear() { x.clear(); y.clear(); }
        void push_back(double xVal, double yVal) { x.push_back(xVal); y.push_back(yVal); }
        void resize(size_t newSize) { x.resize(newSize); y.resize(newSize); }
    };

    struct XYRecs {
        XYs ch1xys, ch2xys;
    } xyRecs;

    struct RingBuffer {
        std::vector<double> times;
        std::vector<double> deltaTimes;
        XYs ch[2];
        int nofm = 0;      // 総測定回数
        int latestIdx = 0; // 最新データのインデックス
        int writeIdx = 0;  // 書き込み位置のインデックス 
        int size = 0;      // 有効データ数
		RingBuffer() {
			update(dt, sec);
		}
        void update(const double dt_, const int  sec_) {
            dt = dt_;
            sec = sec_;
            int bufferSize = static_cast<int>(sec / dt + 1);
            times.resize(bufferSize);
            deltaTimes.resize(bufferSize);
            ch[0].resize(bufferSize);
            ch[1].resize(bufferSize);
        }
		double getDt() const { return dt; }
		int getSec() const { return sec; }
		int getMeasurementSize() const { return static_cast<int>(times.size()); }
    private:
        double dt = LiaConfigDefaultConsts::RINGBUFFER_DT;
        int  sec = LiaConfigDefaultConsts::RINGBUFFER_SEC;
    } ringBuffer;

    struct Raw {
        std::vector<double> times;
        std::vector<std::vector<double>> waveforms;
        std::vector<double> freqs;
        std::vector<std::vector<std::complex<double>>> fftCh;
        std::vector<std::vector<double>> fftAbs;
		const int numHarmonics = 10; // 1倍、3倍、5倍波
        std::vector<XYs> harmonics;

        void initialize(const int numChannels, const size_t newSize, const double raw_dt) {
            size_t halfSize = newSize / 2 + 1;
            times.resize(newSize);
            for (size_t i = 0; i < times.size(); i++) {
                times[i] = (double)i * raw_dt;
            }
            freqs.resize(halfSize);
            for (size_t i = 0; i < freqs.size(); i++) {
                freqs[i] = (double)i / newSize / raw_dt;
            }
			for (int i = 0; i < numChannels; ++i) {
				waveforms.push_back(std::vector<double>(newSize));
				fftCh.push_back(std::vector<std::complex<double>>(halfSize));
				fftAbs.push_back(std::vector<double>(halfSize));
				harmonics.push_back(XYs{ std::vector<double>(numHarmonics), std::vector<double>(numHarmonics) });
			}
        }

        void calculateFFT(const bool flagCh2, const float freq, const double raw_dt) {
            static size_t N = times.size();
            static pocketfft::detail::shape_t shape{ N };
            static const pocketfft::detail::stride_t stride_in{ sizeof(double) };
            static const pocketfft::detail::stride_t stride_out{ sizeof(std::complex<double>) };

            auto processChannel = [&](int chIndex) {
                pocketfft::r2c(shape, stride_in, stride_out, { 0 }, pocketfft::FORWARD, waveforms[chIndex].data(), fftCh[chIndex].data(), 1.0);
                for (size_t i = 0; i < fftCh[chIndex].size(); i++) {
                    fftCh[chIndex][i] *= (2.0 / (double)N);
                    fftAbs[chIndex][i] = std::abs(fftCh[chIndex][i]);
                }

                int baseIdx = static_cast<int>(N * raw_dt * freq);
                // 1倍、3倍、5倍波の抽出
                for (int m = 0; m < numHarmonics; ++m) {
                    int harmonicIdx = baseIdx * (1 + m * 2);
                    harmonics[chIndex].x[m] = fftCh[chIndex][harmonicIdx].real() * cos(std::numbers::pi /2) - fftCh[chIndex][harmonicIdx].imag() * sin(std::numbers::pi /2);
                    harmonics[chIndex].y[m] = fftCh[chIndex][harmonicIdx].imag() * cos(std::numbers::pi /2) + fftCh[chIndex][harmonicIdx].real() * sin(std::numbers::pi /2);
                }
            };

            processChannel(0);
            if (flagCh2) processChannel(1);
        }
    } raw;

    struct ACFMData {
        std::vector<double> Vhs = { 0.022, 0.023, 0.025, 0.027, 0.058, 0.067 };
        std::vector<double> Vvs = { 0.102, 0.106, 0.115, 0.121, 0.212, 0.239 };
        double mmk[3] = { 2.4827, 3.0433, 0.1393 };
        size_t size = 6;
        double vxpp = 0.0;
        double vpp_vz = 0.0;
    } acfmData;

    // ---------------------------------------------------------
    // [3] System State & Hardware Status
    // ---------------------------------------------------------
    Timer timer;
    Daq_dwf* pDaq = nullptr;
    std::string device_sn = "SN:XXXXXXXXXX";
    std::string dirName = ".";

    bool isCh2Enabled = false;
    std::atomic<bool> flagAutoOffset = false;
    std::atomic<bool> flagAutoSetupW2History = false;
    std::atomic<bool> flagAutoSetupW2{ false };
    std::atomic<bool> statusMeasurement{ false };
    std::atomic<bool> statusPipe{ false };

    XYs autoSetupHistoryW1, autoSetupHistoryW2;
    std::vector<std::array<float, 6>> cmds;

private:
    Psd psd;
    struct Hpf { HighPassFilter x, y; };
    struct Lpf { LowPassFilter  x, y; };
    Hpf hpfCh[2];
    Lpf lpfCh[2];

public:
    // ---------------------------------------------------------
    // [4] Constructor & Lifecycle Methods
    // ---------------------------------------------------------
    LiaConfig() {
        initializeDirectory();
        allocateBuffers();
        loadSettingsFromFile();
        raw.initialize(scope.getNumChannels(), scope.getBufferSize(), scope.getSamplingDt());
    }

    ~LiaConfig() {
        saveResultsToFile();
        saveSettingsToFile();
        saveCmdRecordsToFile();
    }

    void reset() {
		plot.reset();
		post.reset();
        window.acfmWindow = false;
        isCh2Enabled = false;
        flagAutoOffset = false;
        pause.flag = false;
		
        awg.reset();
		scope.reset();
		if (pDaq) {
            pDaq->awg.start(awg.ch[0].freq, awg.ch[0].amp, awg.ch[0].phase, awg.ch[0].func, awg.ch[1].freq, awg.ch[1].amp, awg.ch[1].phase, awg.ch[1].func);
			pDaq->scope.open(scope.ch[0].range, scope.ch[1].range, scope.getBufferSize(), 1.0 / scope.getSamplingDt());
            pDaq->scope.trigger();
            pDaq->scope.start();
		}
        setHPFrequency(post.hpFreq);
        setLPFrequency(post.lpFreq);
    }

    // ---------------------------------------------------------
    // [5] Hardware & Processing Methods
    // ---------------------------------------------------------
    void awgStart() {
        if (pDaq) {
            pDaq->awg.start(
                awg.ch[0].freq, awg.ch[0].amp, awg.ch[0].phase, awg.ch[0].func,
                awg.ch[1].freq, awg.ch[1].amp, awg.ch[1].phase, awg.ch[1].func
            );
        }
    }

    void setHPFrequency(double freq) {
        post.hpFreq = static_cast<float>(freq);
        for (int i = 0; i < 2; ++i) {
            hpfCh[i].x.setCutoffFrequency(freq, ringBuffer.getDt());
            hpfCh[i].y.setCutoffFrequency(freq, ringBuffer.getDt());
        }
    }

    void setLPFrequency(double freq) {
        post.lpFreq = static_cast<float>(freq);
        for (int i = 0; i < 2; ++i) {
            lpfCh[i].x.setCutoffFrequency(freq, ringBuffer.getDt());
            lpfCh[i].y.setCutoffFrequency(freq, ringBuffer.getDt());
        }
    }

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
        // PSD初期化
        if (psd.getCurrentFreq() != awg.ch[0].freq) {
            psd.initialize(awg.ch[0].freq, scope.getSamplingDt(), raw.waveforms[0].size());
        }

        // PSD計算
        auto [x1, y1] = psd.calculate(raw.waveforms[0].data());
        double x2 = 0.0, y2 = 0.0;
        if (isCh2Enabled) {
            std::tie(x2, y2) = psd.calculate(raw.waveforms[1].data());
        }

        // オートオフセット処理
        if (flagAutoOffset) {
            post.offset[0].x = x1;
            post.offset[0].y = y1;
            if (isCh2Enabled) {
                post.offset[1].x = x2;
                post.offset[1].y = y2;
                cmds.push_back({ (float)timer.elapsedSec(), (float)ButtonType::PostAutoOffset, (float)x1, (float)y1, 0, 0 });
            }
            flagAutoOffset = false;
        }

        // オフセットと位相回転の適用 (キャッシュして高速化)
        auto [final_x1, final_y1] = psd.rotate_phase(
            x1 - post.offset[0].x, y1 - post.offset[0].y, post.offset[0].phase
        );

        if (isCh2Enabled) {
            auto [final_x2, final_y2] = psd.rotate_phase(
                x2 - post.offset[1].x, y2 - post.offset[1].y, post.offset[1].phase
            );
            AddPoint(t, final_x1, final_y1, final_x2, final_y2);
        }
        else {
            AddPoint(t, final_x1, final_y1);
        }
    }

    // ---------------------------------------------------------
    // [6] File I/O Methods
    // ---------------------------------------------------------
    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        if (localtime_s(&local_tm, &now_time) != 0) throw std::runtime_error("localtime_s failed");

        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y%m%d%H%M%S");
        return oss.str();
    }

    bool saveRawData(const std::string& filename = "raw.csv") const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << (isCh2Enabled ? "# t(s), ch1(V), ch2(V)\n" : "# t(s), ch1(V)\n");
        for (size_t i = 0; i < raw.waveforms[0].size(); ++i) {
            file << std::format("{:e},{:e}", scope.getSamplingDt() * i, raw.waveforms[0][i]);
            if (isCh2Enabled) file << std::format(",{:e}", raw.waveforms[1][i]);
            file << "\n";
        }
        return true;
    }

    bool saveFftData(const std::string& filename = "fft.csv") const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << (isCh2Enabled ? "# f(Hz), ch1real(V), ch1imag(V), ch2real(V), ch2imag(V)\n" : "# f(Hz), ch1real(V), ch1imag(V)\n");
        for (size_t i = 0; i < raw.freqs.size(); ++i) {
            file << std::format("{:e},{:e},{:e}", raw.freqs[i], raw.fftCh[0][i].real(), raw.fftCh[0][i].imag());
            if (isCh2Enabled) file << std::format(",{:e},{:e}", raw.fftCh[1][i].real(), raw.fftCh[1][i].imag());
            file << "\n";
        }
        return true;
    }

    bool saveResultsToFile(const std::string& filename = LiaConfigDefaultConsts::RESULTS_FILE, const double sec = 0) const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << (isCh2Enabled ? "# t(s), x1(V), y1(V), x2(V), y2(V)\n" : "# t(s), x(V), y(V)\n");

        int outputSize = ringBuffer.size;
        if (sec > 0) {
            int reqSize = static_cast<int>(sec / ringBuffer.getDt());
            outputSize = std::min(reqSize, ringBuffer.size);
        }

        int idx = (ringBuffer.writeIdx >= outputSize)
            ? (ringBuffer.writeIdx - outputSize)
            : (ringBuffer.getMeasurementSize() - (outputSize - ringBuffer.writeIdx));

        for (int i = 0; i < outputSize; ++i) {
            file << std::format("{:e},{:e},{:e}", ringBuffer.times[idx], ringBuffer.ch[0].x[idx], ringBuffer.ch[0].y[idx]);
            if (isCh2Enabled) file << std::format(",{:e},{:e}", ringBuffer.ch[1].x[idx], ringBuffer.ch[1].y[idx]);
            file << "\n";

            if (++idx >= ringBuffer.getMeasurementSize()) idx = 0;
        }
        return true;
    }

    bool saveCmdRecordsToFile(const std::string& filename = LiaConfigDefaultConsts::CMDS_FILE) const {
        std::ofstream file(std::format("./{}/{}", dirName, filename));
        if (!file) return false;

        file << "# Time(s), ButtonNo, ButtonName, Value\n";
        for (const auto& cmd : cmds) {
            file << std::format("{:e},{:.0f},{:s},{:e}\n", cmd[0], cmd[1], cmdToString(static_cast<ButtonType>(cmd[1])), cmd[2]);
        }
        return true;
    }

    // ---------------------------------------------------------
    // [7] UI Button Actions
    // ---------------------------------------------------------
    void buttonClear() {
        plot.xyStartIdx = ringBuffer.latestIdx;
        plot.xySize = 0;
        xyRecs.ch1xys.clear();
        xyRecs.ch2xys.clear();
        flagAutoSetupW2History = false;
        cmds.push_back({ (float)timer.elapsedSec(), (float)ButtonType::XYClear, 0, 0, 0, 0 });
    }

    void buttonPause() {
        pause.flag = !pause.flag;
        if (pause.flag) {
            window.imPlotFlag = 0; // ImPlotFlags_None
        }
        else {
            buttonClear();
            window.imPlotFlag |= 4; // ImPlotFlags_NoMouseText
        }
        cmds.push_back({ (float)timer.elapsedSec(), (float)ButtonType::TimePause, (float)pause.flag, 0, 0, 0 });
    }

    void buttonAutoOffset() {
        flagAutoOffset = true;
    }

    void buttonAutoOffsetOff() {
        flagAutoOffset = false;
        cmds.push_back({ (float)timer.elapsedSec(), (float)ButtonType::PostOffsetOff, 0, 0, 0, 0 });
    }

    void buttonRec() {
        xyRecs.ch1xys.push_back(ringBuffer.ch[0].x[ringBuffer.latestIdx], ringBuffer.ch[0].y[ringBuffer.latestIdx]);
        xyRecs.ch2xys.push_back(ringBuffer.ch[1].x[ringBuffer.latestIdx], ringBuffer.ch[1].y[ringBuffer.latestIdx]);

        std::ofstream file(std::format("./{}/rec.csv", dirName));
        if (!file) {
            std::cerr << "Error: Could not open file rec.csv\n";
            return;
        }

        std::stringstream ss;
        ss << (isCh2Enabled ? "# ch1x(V),ch1y(V),ch2x(V),ch2y(V)\n" : "# ch1x(V),ch1y(V)\n");
        for (size_t i = 0; i < xyRecs.ch1xys.x.size(); ++i) {
            ss << std::format("{:e},{:e}", xyRecs.ch1xys.x[i], xyRecs.ch1xys.y[i]);
            if (isCh2Enabled) ss << std::format(",{:e},{:e}", xyRecs.ch2xys.x[i], xyRecs.ch2xys.y[i]);
            ss << "\n";
        }
        file << ss.str();
        cmds.push_back({ (float)timer.elapsedSec(), (float)ButtonType::XYRec, 0, 0, 0, 0 });
    }

private:
    // ---------------------------------------------------------
    // [8] Private Helper Methods
    // ---------------------------------------------------------
    void initializeDirectory() {
        dirName = getCurrentTimestamp();
        try { std::filesystem::create_directory(dirName); }
        catch (const std::filesystem::filesystem_error& e) { std::cerr << "Filesystem error: " << e.what() << '\n'; }
    }

    void allocateBuffers() {
        raw.initialize(scope.getNumChannels(), scope.getBufferSize(), scope.getSamplingDt());
        ringBuffer.update(ringBuffer.getDt(), ringBuffer.getMeasurementSize());
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
        ringBuffer.deltaTimes[ringBuffer.writeIdx] = (ringBuffer.nofm > 0) ?
            (ringBuffer.times[ringBuffer.writeIdx] - ringBuffer.times[ringBuffer.latestIdx]) * 1e3 : 0.0;

        ringBuffer.latestIdx = ringBuffer.writeIdx;
        ringBuffer.nofm++;

        if (++ringBuffer.writeIdx >= ringBuffer.getMeasurementSize()) ringBuffer.writeIdx = 0;
        ringBuffer.size = std::min(ringBuffer.nofm, ringBuffer.getMeasurementSize());

        // XYPlot 表示範囲の更新
        plot.xyLatestIdx = ringBuffer.latestIdx;
        int xyMaxSize = static_cast<int>(plot.historySec / ringBuffer.getDt());

        plot.xySize = std::min(plot.xySize + 1, xyMaxSize);

        int xyStartIdx = plot.xyLatestIdx - plot.xySize + 1;
        if (xyStartIdx >= 0) {
            plot.xyStartIdx = xyStartIdx;
        }
        else {
            plot.xySize = ringBuffer.size + ringBuffer.latestIdx + 1 - plot.xyStartIdx;
        }
    }

    void saveSettingsToFile(const std::string& filename = LiaConfigDefaultConsts::SETTINGS_FILE) const {
        IniWrapper ini;
        // Window
        ini.set("Window", "pos.x", window.pos.x);
        ini.set("Window", "pos.y", window.pos.y);
        ini.set("Window", "size.x", window.size.x);
        ini.set("Window", "size.y", window.size.y);
        ini.set("Window", "fontSize", window.fontSize);
        ini.set("Window", "controlWindow", window.controlWindow);
        ini.set("Window", "rawWindow", window.rawWindow);
        ini.set("Window", "xyWindow", window.xyWindow);
        ini.set("Window", "timeWindow", window.timeWindow);
        ini.set("Window", "deltaTimeWindow", window.deltaTimeWindow);
        ini.set("Window", "acfm", window.acfmWindow);
        ini.set("Window", "theme", window.theme);
        ini.set("Window", "imGuiWindowFlag", window.imGuiWindowFlag);
        ini.set("Window", "imGuiCondFlag", window.imGuiCondFlag);
        // AWG
        ini.set("Awg", "ch[0].freq", awg.ch[0].freq);
        ini.set("Awg", "ch[0].amp", awg.ch[0].amp);
        ini.set("Awg", "ch[0].func", awg.ch[0].func);
        ini.set("Awg", "ch[1].amp", awg.ch[1].amp);
        ini.set("Awg", "ch[1].phase", awg.ch[1].phase);
		// Scope
        ini.set("Scope", "flagCh2", isCh2Enabled);
		for (int i = 0; i < scope.getNumChannels(); ++i) {
			ini.set("Scope", "ch[" + std::to_string(i) + "].range", scope.ch[i].range);
		}
        // Post
        ini.set("Post", "offset[0].phase", post.offset[0].phase);
        ini.set("Post", "offset[0].x", post.offset[0].x);
        ini.set("Post", "offset[0].y", post.offset[0].y);
        ini.set("Post", "offset[1].phase", post.offset[1].phase);
        ini.set("Post", "offset[1].x", post.offset[1].x);
        ini.set("Post", "offset[1].y", post.offset[1].y);
        ini.set("Post", "hpFreq", post.hpFreq);
        ini.set("Post", "lpFreq", post.lpFreq);
        // Plot
        ini.set("Plot", "limit", plot.limit);
        ini.set("Plot", "rawLimit", plot.rawLimit);
        ini.set("Plot", "historySec", plot.historySec);
        ini.set("Plot", "surfaceMode", plot.surfaceMode);
        ini.set("Plot", "beep", plot.beep);
        ini.set("Plot", "Vx_limt", plot.Vx_limit);
        // ACFM
        ini.set("ACFM", "mmk[0]", acfmData.mmk[0]);
        ini.set("ACFM", "mmk[1]", acfmData.mmk[1]);
        ini.set("ACFM", "mmk[2]", acfmData.mmk[2]);

        ini.save(filename);
    }

    void loadSettingsFromFile(const std::string& filename = LiaConfigDefaultConsts::SETTINGS_FILE) {
        IniWrapper ini;
        ini.load(filename);

        window.pos.x = ini.get("Window", "pos.x", window.pos.x);
        window.pos.y = ini.get("Window", "pos.y", window.pos.y);
        window.size.x = ini.get("Window", "size.x", window.size.x);
        window.size.y = ini.get("Window", "size.y", window.size.y);
        window.fontSize = ini.get("Window", "fontSize", window.fontSize);
        window.controlWindow = ini.get("Window", "controlWindow", window.controlWindow);
        window.rawWindow = ini.get("Window", "rawWindow", window.rawWindow);
        window.xyWindow = ini.get("Window", "xyWindow", window.xyWindow);
        window.timeWindow = ini.get("Window", "timeWindow", window.timeWindow);
        window.deltaTimeWindow = ini.get("Window", "deltaTimeWindow", window.deltaTimeWindow);
        window.acfmWindow = ini.get("Window", "acfmWindow", window.acfmWindow);
        window.theme = ini.get("Window", "theme", window.theme);
        window.imGuiWindowFlag = ini.get("Window", "imGuiWindowFlag", window.imGuiWindowFlag);
        window.imGuiCondFlag = ini.get("Window", "imGuiCondFlag", window.imGuiCondFlag);

        awg.ch[0].freq = ini.get("Awg", "ch[0].freq", awg.ch[0].freq);
        awg.ch[0].amp = ini.get("Awg", "ch[0].amp", awg.ch[0].amp);
        awg.ch[0].func = ini.get("Awg", "ch[0].func", awg.ch[0].func);
        awg.ch[1].amp = ini.get("Awg", "ch[1].amp", awg.ch[1].amp);
        awg.ch[1].phase = ini.get("Awg", "ch[1].phase", awg.ch[1].phase);

        isCh2Enabled = ini.get("Scope", "flagCh2", isCh2Enabled);
        for (int i = 0; i < scope.getNumChannels(); ++i) {
            scope.ch[i].range = ini.get("Scope", "ch[" + std::to_string(i) + "].range", scope.ch[i].range);
        }
        scope.setMaxRange();

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
        plot.Vx_limit = ini.get("Plot", "Vx_limt", plot.Vx_limit);

        acfmData.mmk[0] = ini.get("ACFM", "mmk[0]", acfmData.mmk[0]);
        acfmData.mmk[1] = ini.get("ACFM", "mmk[1]", acfmData.mmk[1]);
        acfmData.mmk[2] = ini.get("ACFM", "mmk[2]", acfmData.mmk[2]);

        // 値のクランプと初期化
        awg.ch[0].freq = std::clamp(awg.ch[0].freq, scope.getLowLimitFreq(), scope.getHighLimitFreq());
        awg.ch[1].freq = awg.ch[0].freq;
        awg.ch[0].amp = std::clamp(awg.ch[0].amp, 0.1f, 5.0f);
        awg.ch[1].amp = std::clamp(awg.ch[1].amp, 0.0f, 5.0f);
        awg.ch[1].func = awg.ch[0].func;
        plot.limit = std::clamp(plot.limit, 0.0f, 5.0f);

        setHPFrequency(post.hpFreq);
        setLPFrequency(post.lpFreq);
    }
};