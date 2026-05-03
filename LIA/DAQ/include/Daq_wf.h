#pragma once

#pragma comment(lib, "DAQ/lib/dwf.lib")
#include <dwf.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <array>
#include <cmath>

// --- 1. DWF API呼び出し用のカスタム例外 ---
class DwfException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// --- 2. エラー処理関数 ---
[[noreturn]] static void _dwfFatalExit() {
    system("PAUSE");
    exit(EXIT_FAILURE);
}

static void _dwfErrHandler(const char* call_str, const char* file, int line) {
    char szError[512] = { 0 };
    FDwfGetLastErrorMsg(szError);

    std::string err_msg = "--- DWF Error ---\n"
        "Call: " + std::string(call_str) + "\n" +
        "File: " + std::string(file) + ", " + std::to_string(line) + "\n" +
        "Message: " + std::string(szError) + "\n" +
        "-------------------\n";

    std::cerr << err_msg;

#ifndef NDEBUG
    _dwfFatalExit(); // デバッグ時は停止
#else
    throw DwfException(err_msg); // リリース時は例外送出
#endif
}

// --- 3. エラーチェックを自動化するマクロ ---
#define DWF_CALL(call) do { \
    if (!(call)) { \
        _dwfErrHandler(#call, __FILE__, __LINE__); \
    } \
} while(0)

/**
 * @class Daq_dwf
 * @brief Digilent WaveForms デバイスをRAIIで管理するラッパークラス
 */
class Daq_dwf {
private:
    HDWF m_hdwf = 0;

    void init(int idxDevice) {
        DWF_CALL(FDwfGetVersion(device.version));
        DWF_CALL(FDwfEnumDeviceName(idxDevice, device.name));
        DWF_CALL(FDwfEnumSN(idxDevice, device.sn));
        DWF_CALL(FDwfDeviceOpen(idxDevice, &m_hdwf));
    }

    HDWF getHdwf() const { return m_hdwf; }

public:
    // --- 4. 内部構造体・クラスの定義 ---

    struct DeviceInfo {
        const char manufacturer[32] = "Digilent";
        char name[256] = { 0 };
        char sn[32] = { 0 };
        char version[32] = { 0 };
    } device;

    /**
     * @class Awg
     * @brief 任意波形発生器 (AWG) の設定管理
     */
    class Awg {
    private:
        Daq_dwf& m_parent;
        HDWF getHdwf() const { return m_parent.getHdwf(); }

    public:
        static constexpr int NUM_CHANNELS = 2;
        static constexpr int CUSTOM_DATA_SIZE = 5000;

        // チャンネルごとの設定をグループ化
        struct Channel {
            FUNC func;
            TRIGSRC trigSrc;
            double frequency;
            double amplitude;
            double phaseDeg;
            std::vector<double> customData;

            Channel(FUNC f, TRIGSRC ts, double freq, double amp, double phase)
                : func(f), trigSrc(ts), frequency(freq), amplitude(amp), phaseDeg(phase), customData(CUSTOM_DATA_SIZE, 0.0) {}
        };

        std::array<Channel, NUM_CHANNELS> channels = {
            Channel{funcSine, trigsrcNone, 1e3, 1.0, 0.0},
            Channel{funcSine, trigsrcAnalogOut1, 1e3, 0.0, 0.0}
        };

        explicit Awg(Daq_dwf& parent) : m_parent(parent) {}

        void start() {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                const auto& ch = channels[i];
                DWF_CALL(FDwfAnalogOutNodeEnableSet(getHdwf(), i, AnalogOutNodeCarrier, true));
                DWF_CALL(FDwfAnalogOutNodeFunctionSet(getHdwf(), i, AnalogOutNodeCarrier, ch.func));

                if (ch.func == funcCustom) {
                    // vectorの生データポインタを渡す
                    DWF_CALL(FDwfAnalogOutNodeDataSet(getHdwf(), i, AnalogOutNodeCarrier, const_cast<double*>(ch.customData.data()), ch.customData.size()));
                }

                DWF_CALL(FDwfAnalogOutNodeFrequencySet(getHdwf(), i, AnalogOutNodeCarrier, ch.frequency));
                DWF_CALL(FDwfAnalogOutNodeAmplitudeSet(getHdwf(), i, AnalogOutNodeCarrier, ch.amplitude));
                DWF_CALL(FDwfAnalogOutNodeOffsetSet(getHdwf(), i, AnalogOutNodeCarrier, 0.0));
                DWF_CALL(FDwfAnalogOutNodePhaseSet(getHdwf(), i, AnalogOutNodeCarrier, ch.phaseDeg));
                DWF_CALL(FDwfAnalogOutTriggerSourceSet(getHdwf(), i, ch.trigSrc));
            }
            DWF_CALL(FDwfAnalogOutConfigure(getHdwf(), -1, true));
        }

        void start(double frequency, double amplitude1, double phaseDeg1) {
            start(frequency, amplitude1, phaseDeg1, frequency, 0.0, 0.0);
        }

        void start(double freq1, double amp1, double phase1, double freq2, double amp2, double phase2) {
            channels[0].frequency = freq1;
            channels[0].amplitude = amp1;
            channels[0].phaseDeg = phase1;

            channels[1].frequency = freq2;
            channels[1].amplitude = amp2;
            channels[1].phaseDeg = phase2;
            start();
        }

        void off() {
            DWF_CALL(FDwfAnalogOutNodeEnableSet(getHdwf(), -1, AnalogOutNodeCarrier, false));
        }
    };

    /**
     * @class Scope
     * @brief オシロスコープ (Scope) の設定管理
     */
    class Scope {
    private:
        Daq_dwf& m_parent;
        HDWF getHdwf() const { return m_parent.getHdwf(); }

    public:
        double voltsRange = 5.0;
        int bufferSize = 5000;
        double samplingRate = 100e6; // キャメルケースに統一
        double timeoutSec = 0.0;     // 変数名を明確化

        TRIGSRC trigSrc = trigsrcAnalogOut1;
        int trigChannel = 0;
        TRIGTYPE trigType = trigtypeEdge;
        double trigVoltLevel = 0.0;
        DwfTriggerSlope trigSlope = DwfTriggerSlopeRise;

        explicit Scope(Daq_dwf& parent) : m_parent(parent) {}

        void open() {
            DWF_CALL(FDwfAnalogInChannelEnableSet(getHdwf(), -1, true));
            DWF_CALL(FDwfAnalogInChannelOffsetSet(getHdwf(), -1, 0.0));
            DWF_CALL(FDwfAnalogInChannelRangeSet(getHdwf(), -1, voltsRange * 2));
            DWF_CALL(FDwfAnalogInBufferSizeSet(getHdwf(), bufferSize));
            DWF_CALL(FDwfAnalogInBufferSizeGet(getHdwf(), &bufferSize)); // 実サイズ読込
            DWF_CALL(FDwfAnalogInFrequencySet(getHdwf(), samplingRate));
            DWF_CALL(FDwfAnalogInChannelFilterSet(getHdwf(), -1, filterAverageFit));
        }

        void open(double range, int size, double rate) {
            voltsRange = range;
            bufferSize = size;
            samplingRate = rate;
            open();
        }

        void trigger() {
            DWF_CALL(FDwfAnalogInTriggerAutoTimeoutSet(getHdwf(), timeoutSec));
            DWF_CALL(FDwfAnalogInTriggerSourceSet(getHdwf(), trigSrc));

            if (trigSrc != trigsrcAnalogOut1) {
                if (trigSrc == trigsrcDetectorAnalogIn) {
                    DWF_CALL(FDwfAnalogInTriggerChannelSet(getHdwf(), trigChannel));
                }
                DWF_CALL(FDwfAnalogInTriggerTypeSet(getHdwf(), trigType));
                DWF_CALL(FDwfAnalogInTriggerLevelSet(getHdwf(), trigVoltLevel));
                DWF_CALL(FDwfAnalogInTriggerConditionSet(getHdwf(), trigSlope));
            }
        }

        void trigger(double timeout, TRIGSRC src, int ch, TRIGTYPE type, double level, DwfTriggerSlope slope) {
            timeoutSec = timeout;
            trigSrc = src;
            trigChannel = ch;
            trigType = type;
            trigVoltLevel = level;
            trigSlope = slope;
            trigger();
        }

        void start() {
            DWF_CALL(FDwfAnalogInConfigure(getHdwf(), true, true));
        }

        // 生配列から std::vector への変更
        void record(std::vector<double>& buffer1) {
            buffer1.resize(bufferSize);
            waitAndReadData(0, buffer1.data());
        }

        void record(std::vector<double>& buffer1, std::vector<double>& buffer2) {
            buffer1.resize(bufferSize);
            buffer2.resize(bufferSize);
            waitAndReadData(0, buffer1.data());
            waitAndReadData(1, buffer2.data());
        }

    private:
        // データ取得の重複処理を共通化
        void waitAndReadData(int channelIdx, double* bufferPtr) {
            DwfState sts;
            do {
                DWF_CALL(FDwfAnalogInStatus(getHdwf(), true, &sts));
            } while (sts != stsDone);
            DWF_CALL(FDwfAnalogInStatusData(getHdwf(), channelIdx, bufferPtr, bufferSize));
        }
    };

    // --- 5. Daq_dwf インスタンス変数 ---

    Awg awg;
    Scope scope;

    Daq_dwf() : awg(*this), scope(*this) {
        int pcDevice = getPcDevice();
        if (pcDevice < 1) throw DwfException("No AD is connected.");
        init(getIdxFirstEnabledDevice());
    }

    explicit Daq_dwf(const std::string& sn) : awg(*this), scope(*this) {
        int idx = getIdxDevice(sn);
        if (idx < 0) throw DwfException("No AD with SN '" + sn + "' is connected.");
        init(idx);
    }

    ~Daq_dwf() {
        if (m_hdwf != 0) {
            FDwfDeviceClose(m_hdwf);
        }
    }

    Daq_dwf(const Daq_dwf&) = delete;
    Daq_dwf& operator=(const Daq_dwf&) = delete;

    void powerSupply(double volts = 5.0) {
        bool flag = (volts != 0.0);
        double abs_volts = std::abs(volts);

        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 0, 1, abs_volts));  // V+ Level
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 1, 1, -abs_volts)); // V- Level
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 0, 0, flag));       // V+ Enable
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 1, 0, flag));       // V- Enable
        DWF_CALL(FDwfAnalogIOEnableSet(m_hdwf, flag));                  // Master Enable
    }

    // --- 6. 静的ヘルパー関数 ---
    static int getPcDevice() {
        int pcDevice;
        DWF_CALL(FDwfEnum(enumfilterAll, &pcDevice));
        return pcDevice;
    }

    static int getIdxDevice(const std::string& sn) {
        int pcDevice = getPcDevice();
        for (int i = 0; i < pcDevice; i++) {
            char szSN[32] = { 0 };
            DWF_CALL(FDwfEnumSN(i, szSN));
            if (sn == szSN) return i;
        }
        return -1;
    }

    static int getIdxFirstEnabledDevice() {
        int pcDevice = getPcDevice();
        for (int i = 0; i < pcDevice; i++) {
            int isOpened;
            DWF_CALL(FDwfEnumDeviceIsOpened(i, &isOpened));
            if (!isOpened) return i;
        }
        return -1;
    }
};