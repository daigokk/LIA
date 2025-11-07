#pragma once

#pragma comment(lib, "DAQ/lib/dwf.lib")
#include <dwf.h>
#include <iostream>
#include <string>
#include <stdexcept> // std::runtime_error のため

// --- 1. DWF API呼び出し用のカスタム例外 ---
class DwfException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// --- 2. エラー処理関数 (マクロから呼ばれる) ---
// この関数は DWF_CALL マクロの内部でのみ使用されます
static void _dwfErrHandler(const char* call_str, const char* file, int line) {
    static char szError[512] = { 0 };
    FDwfGetLastErrorMsg(szError);
    std::string err_msg = "--- DWF Error ---\n"
        "Call: " + std::string(call_str) + "\n" +
        "File: " + std::string(file) + ", " + std::to_string(line) + "\n" +
        "Message: " + std::string(szError) + "\n" +
        "-------------------\n";
    std::cerr << err_msg;

#ifndef NDEBUG
    // デバッグビルドでは、開発者にすぐ知らせるために停止する
    // (元の system("PAUSE") と exit() の動作)
    system("PAUSE");
    exit(EXIT_FAILURE);
#else
    // リリースビルドでは、アプリが処理できるように例外をスローする
    throw DwfException(err_msg);
#endif
}

// --- 3. エラーチェックを自動化するマクロ ---
// これが可読性の鍵です。
// errChk(FDwf...(...), __func__, ...) を DWF_CALL(FDwf...(...)) に置き換えます。
#define DWF_CALL(call) do { \
    if (!(call)) { \
        _dwfErrHandler(#call, __FILE__, __LINE__); \
    } \
} while(0)


/**
 * @class Daq_dwf
 * @brief Digilent WaveForms デバイスをRAIIで管理するラッパークラス
 */
class Daq_dwf
{
private:
    // デバイスハンドルはクラスが秘密裏に管理する
    HDWF m_hdwf = 0;

    /**
     * @brief デバイス情報を初期化し、デバイスを開く
     * @param idxDevice デバイスインデックス
     */
    void init(int idxDevice)
    {
        DWF_CALL(FDwfGetVersion(device.version));
        DWF_CALL(FDwfEnumDeviceName(idxDevice, device.name));
        DWF_CALL(FDwfEnumSN(idxDevice, device.sn));

        // m_hdwf (プライベートメンバー) にハンドルを格納
        DWF_CALL(FDwfDeviceOpen(idxDevice, &m_hdwf));
    }

    // getHdwf() はネストクラスからのみフレンドアクセスを許可
    friend class Awg;
    friend class Scope;
    HDWF getHdwf() const { return m_hdwf; }


public:
    // --- 4. ネストクラスの定義 (Awg, Scope) ---

    /**
     * @class Awg
     * @brief 任意波形発生器 (AWG) の設定を管理する
     */
    class Awg
    {
    private:
        Daq_dwf& m_parent; // 親クラス(Daq_dwf)への参照
        HDWF getHdwf() const { return m_parent.getHdwf(); } // 親のハンドルを取得

    public:
        // 親クラスの参照を受け取るコンストラクタ
        explicit Awg(Daq_dwf& parent) : m_parent(parent) {}

        // --- 設定 (publicで公開し、structのように扱う) ---
        static constexpr int NUM_CHANNELS = 2;
        static constexpr int CUSTOM_DATA_SIZE = 5000;

        FUNC func[NUM_CHANNELS] = { funcSine, funcSine };
        const TRIGSRC trigsrc[NUM_CHANNELS] = { trigsrcNone, trigsrcAnalogOut1 };
        double frequency[NUM_CHANNELS] = { 1e3, 1e3 };
        double amplitude[NUM_CHANNELS] = { 1.0, 0.0 };
        double phaseDeg[NUM_CHANNELS] = { 0.0, 0.0 };
        double rgdData[NUM_CHANNELS][CUSTOM_DATA_SIZE] = { 0 };

        // --- メソッド ---
        void start()
        {
            for (int i = 0; i < NUM_CHANNELS; i++)
            {
                // DWF_CALLマクロで劇的にスッキリ
                DWF_CALL(FDwfAnalogOutNodeEnableSet(getHdwf(), i, AnalogOutNodeCarrier, true));
                DWF_CALL(FDwfAnalogOutNodeFunctionSet(getHdwf(), i, AnalogOutNodeCarrier, func[i]));
                if (func[i] == funcCustom)
                {
                    DWF_CALL(FDwfAnalogOutNodeDataSet(getHdwf(), i, AnalogOutNodeCarrier, rgdData[i], CUSTOM_DATA_SIZE));
                }
                DWF_CALL(FDwfAnalogOutNodeFrequencySet(getHdwf(), i, AnalogOutNodeCarrier, frequency[i]));
                DWF_CALL(FDwfAnalogOutNodeAmplitudeSet(getHdwf(), i, AnalogOutNodeCarrier, amplitude[i]));
                DWF_CALL(FDwfAnalogOutNodeOffsetSet(getHdwf(), i, AnalogOutNodeCarrier, 0.0));
                DWF_CALL(FDwfAnalogOutNodePhaseSet(getHdwf(), i, AnalogOutNodeCarrier, phaseDeg[i]));
                DWF_CALL(FDwfAnalogOutTriggerSourceSet(getHdwf(), i, trigsrc[i]));
            }
            DWF_CALL(FDwfAnalogOutConfigure(getHdwf(), -1, true));
        }

        void start(const double frequency, const double amplitude1, const double phaseDeg1)
        {
            this->frequency[0] = frequency;
            this->amplitude[0] = amplitude1;
            this->phaseDeg[0] = phaseDeg1;
            for (int i = 1; i < NUM_CHANNELS; i++)
            {
                this->frequency[i] = frequency;
                this->amplitude[i] = 0.0;
                this->phaseDeg[i] = 0.0;
            }
            start(); // this-> は不要
        }

        void start(
            const double frequency1, const double amplitude1, const double phaseDeg1,
            const double frequency2, const double amplitude2, const double phaseDeg2
        )
        {
            this->frequency[0] = frequency1;
            this->amplitude[0] = amplitude1;
            this->phaseDeg[0] = phaseDeg1;
            this->frequency[1] = frequency2;
            this->amplitude[1] = amplitude2;
            this->phaseDeg[1] = phaseDeg2;
            start();
        }

        void off()
        {
            DWF_CALL(FDwfAnalogOutNodeEnableSet(getHdwf(), -1, AnalogOutNodeCarrier, false));
        }
    }; // --- End class Awg ---

    /**
     * @class Scope
     * @brief オシロスコープ (Scope) の設定を管理する
     */
    class Scope
    {
    private:
        Daq_dwf& m_parent; // 親クラス(Daq_dwf)への参照
        HDWF getHdwf() const { return m_parent.getHdwf(); } // 親のハンドルを取得

    public:
        explicit Scope(Daq_dwf& parent) : m_parent(parent) {}

        // --- 設定 ---
        double voltsRange = 5.0;
        int bufferSize = 5000;
        double SamplingRate = 100e6;
        double secTimeout = 0.0;
        TRIGSRC trigSrc = trigsrcAnalogOut1;
        int trigChannel = 0;
        TRIGTYPE trigType = trigtypeEdge;
        double trigVoltLevel = 0.0;
        DwfTriggerSlope trigSlope = DwfTriggerSlopeRise;

        // --- メソッド ---
        void open()
        {
            DWF_CALL(FDwfAnalogInChannelEnableSet(getHdwf(), -1, true));
            DWF_CALL(FDwfAnalogInChannelOffsetSet(getHdwf(), -1, 0.0));
            DWF_CALL(FDwfAnalogInChannelRangeSet(getHdwf(), -1, voltsRange * 2));
            DWF_CALL(FDwfAnalogInBufferSizeSet(getHdwf(), bufferSize));
            // 実際に設定されたバッファサイズを読み戻す
            DWF_CALL(FDwfAnalogInBufferSizeGet(getHdwf(), &bufferSize));
            DWF_CALL(FDwfAnalogInFrequencySet(getHdwf(), SamplingRate));
            DWF_CALL(FDwfAnalogInChannelFilterSet(getHdwf(), -1, filterAverageFit));
        }

        void open(const double voltsRange, const int bufferSize, const double SamplingRate)
        {
            this->voltsRange = voltsRange;
            this->bufferSize = bufferSize;
            this->SamplingRate = SamplingRate;
            open();
        }

        void trigger()
        {
            DWF_CALL(FDwfAnalogInTriggerAutoTimeoutSet(getHdwf(), secTimeout));
            DWF_CALL(FDwfAnalogInTriggerSourceSet(getHdwf(), trigSrc));

            if (trigSrc != trigsrcAnalogOut1)
            {
                if (trigSrc == trigsrcDetectorAnalogIn)
                {
                    DWF_CALL(FDwfAnalogInTriggerChannelSet(getHdwf(), trigChannel));
                }
                DWF_CALL(FDwfAnalogInTriggerTypeSet(getHdwf(), trigType));
                DWF_CALL(FDwfAnalogInTriggerLevelSet(getHdwf(), trigVoltLevel));
                DWF_CALL(FDwfAnalogInTriggerConditionSet(getHdwf(), trigSlope));
            }
        }

        void trigger(
            const double secTimeout, const TRIGSRC trigSrc,
            const int trigChannel, const TRIGTYPE trigType, const double trigVoltLevel, DwfTriggerSlope trigSlope
        )
        {
            this->secTimeout = secTimeout;
            this->trigSrc = trigSrc;
            this->trigChannel = trigChannel;
            this->trigType = trigType;
            this->trigVoltLevel = trigVoltLevel;
            this->trigSlope = trigSlope;
            trigger();
        }

        void start()
        {
            DWF_CALL(FDwfAnalogInConfigure(getHdwf(), true, true));
        }

        void record(double buffer1[])
        {
            DwfState sts;
            do {
                DWF_CALL(FDwfAnalogInStatus(getHdwf(), true, &sts));
            } while (sts != stsDone);
            DWF_CALL(FDwfAnalogInStatusData(getHdwf(), 0, buffer1, bufferSize));
        }

        void record(double buffer1[], double buffer2[])
        {
            DwfState sts;
            do {
                DWF_CALL(FDwfAnalogInStatus(getHdwf(), true, &sts));
            } while (sts != stsDone);
            DWF_CALL(FDwfAnalogInStatusData(getHdwf(), 0, buffer1, bufferSize));
            DWF_CALL(FDwfAnalogInStatusData(getHdwf(), 1, buffer2, bufferSize));
        }
        // `record2` は `record(buf1, buf2)` と完全重複のため削除
    }; // --- End class Scope ---


    // --- 5. Daq_dwf のメンバー変数とメソッド ---

    /**
     * @struct Device
     * @brief デバイスの静的な情報を格納する
     */
    struct Device
    {
        const char manufacturer[32] = "Digilent";
        // HDWF hdwf = 0; // ハンドルは Daq_dwf が private で管理
        char name[256] = { "" };
        char sn[32] = { "" };
        char version[32] = { "" };
    } device; // Daq_dwf::device としてアクセス可能

    // サブクラスのインスタンス
    Awg awg;
    Scope scope;

    /**
     * @brief 利用可能な最初のデバイスで初期化する
     */
    Daq_dwf() : awg(*this), scope(*this) // メンバー初期化子でサブクラスに 'this' を渡す
    {
        int pcDevice = getPcDevice();
        if (pcDevice < 1)
        {
            // exit() の代わりに例外をスロー
            throw DwfException("No AD is connected.");
        }
        init(getIdxFirstEnabledDevice());
    }

    /**
     * @brief シリアルナンバーを指定してデバイスを初期化する
     * @param sn 接続するデバイスのシリアルナンバー
     */
    explicit Daq_dwf(const std::string& sn) : awg(*this), scope(*this)
    {
        int idx = getIdxDevice(sn); // 既存のヘルパー関数を利用
        if (idx < 0)
        {
            throw DwfException("No AD with SN '" + sn + "' is connected.");
        }
        init(idx);
    }

    /**
     * @brief デストラクタ (RAII)
     * デバイスハンドルを自動的にクローズする
     */
    ~Daq_dwf()
    {
        if (m_hdwf != 0) {
            FDwfDeviceClose(m_hdwf);
        }
    }

    // コピーは禁止 (リソースハンドルを扱うクラスの定石)
    Daq_dwf(const Daq_dwf&) = delete;
    Daq_dwf& operator=(const Daq_dwf&) = delete;


    /**
     * @brief 電源供給 (V+, V-) を設定する
     * @param volts 供給電圧 (0.0を指定するとOFFになる)
     */
    void powerSupply(const double volts = 5.0)
    {
        bool flag = (volts != 0.0);
        double abs_volts = abs(volts);

        // Channel 0 = V+
        // Channel 1 = V-
        // Node 0 = Enable/Disable
        // Node 1 = Voltage Level

        // V+ 電圧設定
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 0, 1, abs_volts));
        // V- 電圧設定
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 1, 1, -abs_volts));
        // V+ 有効/無効
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 0, 0, flag));
        // V- 有効/無効
        DWF_CALL(FDwfAnalogIOChannelNodeSet(m_hdwf, 1, 0, flag));
        // 電源のマスター有効/無効
        DWF_CALL(FDwfAnalogIOEnableSet(m_hdwf, flag));
    }


    // --- 6. 静的ヘルパー関数 (デバイス列挙用) ---

    /**
     * @brief 接続されているデバイスの総数を取得する
     */
    static int getPcDevice()
    {
        int pcDevice;
        DWF_CALL(FDwfEnum(enumfilterAll, &pcDevice));
        return pcDevice;
    }

    /**
     * @brief シリアルナンバーからデバイスインデックスを取得する
     * @param sn 検索するシリアルナンバー
     * @return 見つかった場合はインデックス [0..N], 見つからない場合は -1
     */
    static int getIdxDevice(const std::string& sn)
    {
        int pcDevice = getPcDevice();
        for (int i = 0; i < pcDevice; i++)
        {
            char szSN[32] = { "" };
            DWF_CALL(FDwfEnumSN(i, szSN));
            if (sn == szSN) return i;
        }
        return -1;
    }
        
    /**
     * @brief 現在使用可能（開かれていない）最初のデバイスインデックスを取得する
     * @return 見つかった場合はインデックス [0..N], すべて使用中の場合は -1
     */
    static int getIdxFirstEnabledDevice()
    {
        int pcDevice = getPcDevice();
        for (int i = 0; i < pcDevice; i++)
        {
            int flag;
            DWF_CALL(FDwfEnumDeviceIsOpened(i, &flag));
            if (!flag) return i;
        }
        return -1;
    }
};