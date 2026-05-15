#pragma once
#include "LiaConfig.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <thread> // std::this_thread, std::jthread

// ============================================================
// 定数定義
// ============================================================
const std::vector<std::string> HELPS = {
    "Available commands:",
    "  reset or *rst                : Reset all settings to default values",
    "  *idn?                        : Identify connected DAQ device",
    "  error?                       : Show last error message",
    "  end, exit, quit or close     : Exit the program",
    "  pause or stop                : Pause data acquisition",
    "  run                          : Resume data acquisition",
    "  data:raw:save [filename]     : Save raw data to file (optional filename)",
    "  data:raw:size?               : Get size of raw data buffer",
    "  data:raw?                    : Output raw data (time, ch1 [, ch2])",
    "  data:fft:save [filename]     : Save FFT data to file (optional filename)",
    "  data:fft:size?               : Get size of FFT data buffer",
    "  data:fft?                    : Output FFT data (frequency, ch1 [, ch2])",
    "  data:txy? [seconds]          : Output time and XY data for specified seconds (default all)",
    "  data:xy?                     : Output latest XY data point",
    "  disp|display:xy:limit <val>  : Set XY display limit",
    "  disp|display:raw:limit <val> : Set raw display limit",
    "  chan1|chan2:range [value|?]  : Set or query channel range",
    "  chan2:disp [on|off|?]        : Enable/disable or query CH2 display state",
    "  acfm:disp [on|off|?]         : Enable/disable or query ACFM window display state",
    "  w1|w2:freq [min|max|value]   : Set or query waveform frequency",
    "  w1|w2:amp [min|max|value]    : Set or query waveform amplitude",
    "  w1|w2:phase [value|?]        : Set or query waveform phase in degrees",
    "  calc:offset:state [on|off|?] : Enable/disable auto offset",
    "  calc:offset:auto once        : Perform one-time auto offset",
    "  calc:hpf:freq [value|?]      : Set or query high-pass filter frequency (0 to 50 Hz)",
    "  calc1|calc2:offset:phase [v|?]: Set or query calculation offset phase in degrees",
    "  help? or ?                   : Show this help message",
};

// ============================================================
// ユーティリティ
// ============================================================
namespace utils {
    // 文字列を区切り文字で分割し、空のトークンを除外する
    inline std::vector<std::string> split(const std::string& str, char delimiter = ':') {
        std::vector<std::string> tokens;
        if (str.empty()) return tokens;

        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    // 文字列を小文字に変換
    inline void toLower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    }
}

// ============================================================
// コマンド処理クラス
// ============================================================
class CommandProcessor {
public:
    explicit CommandProcessor(LiaConfig* config) : pCfg(config) {
        registerCommands();
    }

    void processStream(std::stop_token st) {
        pCfg->statusPipe = true;

        while (!st.stop_requested()) {
            std::string line;
            if (!std::getline(std::cin, line)) {
                break; // 入力ストリームの終了またはエラー
            }

            std::string original_line = line;
            utils::toLower(line);

            std::istringstream iss(line);
            std::string commandPart, argumentPart;
            iss >> commandPart;

            // 終了判定
            if (commandPart == "end" || commandPart == "exit" || commandPart == "quit" || commandPart == "close") {
                break;
            }

            iss >> argumentPart;
            float value = 0.0f;
            try {
                if (!argumentPart.empty()) {
                    value = std::stof(argumentPart);
                }
            }
            catch (...) {
                value = 0.0f;
            }

            // 先頭の ':' を除去してコマンドを正規化 (例: ":chan1:range" -> "chan1:range")
            if (!commandPart.empty() && commandPart.front() == ':') {
                commandPart.erase(0, 1);
            }

            auto tokens = utils::split(commandPart, ':');
            bool success = dispatchCommand(commandPart, tokens, argumentPart, value);

            // エラー処理
            if (!success) {
                lastErrorCmd = original_line;
                // クエリ (?) の場合は即座にエラーを出力
                if (commandPart.find('?') != std::string::npos) {
                    std::cout << std::format("Error: '{}'\n", lastErrorCmd);
                    lastErrorCmd.clear();
                }
            }

            // AWGの更新が必要な場合はデバイスに反映
            if (awgUpdateRequired && pCfg->pDaq != nullptr) {
                const auto& ch0 = pCfg->awg.ch[0];
                const auto& ch1 = pCfg->awg.ch[1];
                pCfg->pDaq->awg.start(
                    ch0.freq, ch0.amp, ch0.phase, ch0.func,
                    ch1.freq, ch1.amp, ch1.phase, ch1.func
                );
                awgUpdateRequired = false;
            }

            std::cout << std::flush;
        }

        pCfg->statusPipe = false;
    }

private:
    LiaConfig* pCfg;
    std::string lastErrorCmd;
    bool awgUpdateRequired = false;

    // ハンドラの型
    using CommandHandler = std::function<bool(const std::vector<std::string>&, const std::string&, float)>;

    // 完全一致ルーティングと、プレフィックス（先頭要素）一致ルーティングを分離
    std::map<std::string, CommandHandler> exactMatchHandlers;
    std::map<std::string, CommandHandler> prefixMatchHandlers;

    // ============================================================
    // コマンド登録 (ルーティングテーブル)
    // ============================================================
    void registerCommands() {
        // --- システム・基本操作 ---
        auto resetHandler = [this](auto&, auto&, auto) {
            pCfg->reset();
            lastErrorCmd.clear();
            awgUpdateRequired = true;
            return true;
            };
        exactMatchHandlers["reset"] = resetHandler;
        exactMatchHandlers["*rst"] = resetHandler;

        exactMatchHandlers["*idn?"] = [this](auto&, auto&, auto) { return handleIdn(); };
        exactMatchHandlers["error?"] = [this](auto&, auto&, auto) { return handleError(); };
        exactMatchHandlers["pause"] = [this](auto&, auto&, auto) { pCfg->pause.flag = true; return true; };
        exactMatchHandlers["stop"] = [this](auto&, auto&, auto) { pCfg->pause.flag = true; return true; };
        exactMatchHandlers["run"] = [this](auto&, auto&, auto) { pCfg->pause.flag = false; return true; };

        auto helpHandler = [this](auto&, auto&, auto) {
            for (const auto& line : HELPS) std::cout << line << "\n";
            return true;
            };
        exactMatchHandlers["help?"] = helpHandler;
        exactMatchHandlers["?"] = helpHandler;
        exactMatchHandlers["help"] = [this](const auto& tokens, auto&, auto) {
            if (tokens.size() > 1 && tokens[1] == "size?") {
                std::cout << HELPS.size() << "\n";
                return true;
            }
            return false;
            };

        // --- プレフィックス(階層型)コマンド ---
        prefixMatchHandlers["data"] = [this](auto& t, auto& a, auto v) { return handleData(t, a, v); };
        prefixMatchHandlers["display"] = [this](auto& t, auto& a, auto v) { return handleDisplay(t, a, v); };
        prefixMatchHandlers["disp"] = [this](auto& t, auto& a, auto v) { return handleDisplay(t, a, v); };
        prefixMatchHandlers["w1"] = [this](auto& t, auto& a, auto v) { return handleWaveform(true, t, a, v); };
        prefixMatchHandlers["w2"] = [this](auto& t, auto& a, auto v) { return handleWaveform(false, t, a, v); };
        prefixMatchHandlers["calculate"] = [this](auto& t, auto& a, auto v) { return handleCalculate(t, a, v); };
        prefixMatchHandlers["calc"] = [this](auto& t, auto& a, auto v) { return handleCalculate(t, a, v); };

        // --- 完全一致による設定・照会コマンド ---
        exactMatchHandlers["chan1:range"] = [this](auto&, auto&, float v) {
            pCfg->scope.ch[0].range = v;
			pCfg->scope.setMaxRange();
            pCfg->pDaq->scope.open(pCfg->scope.ch[0].range, pCfg->scope.ch[1].range, pCfg->scope.getBufferSize(), 1.0 / pCfg->scope.getSamplingDt());
            pCfg->pDaq->scope.trigger();
            pCfg->pDaq->scope.start();
            return true;
        };
        exactMatchHandlers["chan1:range?"] = [this](auto&, auto&, auto) { std::cout << pCfg->scope.ch[0].range << "\n"; return true; };
        exactMatchHandlers["chan2:range"] = [this](auto&, auto&, float v) {
            pCfg->scope.ch[1].range = v;
            pCfg->scope.setMaxRange();
            pCfg->pDaq->scope.open(pCfg->scope.ch[0].range, pCfg->scope.ch[1].range, pCfg->scope.getBufferSize(), 1.0 / pCfg->scope.getSamplingDt());
            pCfg->pDaq->scope.trigger();
            pCfg->pDaq->scope.start();
            return true;
        };
        exactMatchHandlers["chan2:range?"] = [this](auto&, auto&, auto) { std::cout << pCfg->scope.ch[1].range << "\n"; return true; };

        exactMatchHandlers["chan2:disp"] = [this](auto&, const std::string& a, auto) { return handleToggle(a, pCfg->isCh2Enabled); };
        exactMatchHandlers["chan2:disp?"] = [this](auto&, auto&, auto) { std::cout << (pCfg->isCh2Enabled ? "on\n" : "off\n"); return true; };

        exactMatchHandlers["acfm:disp"] = [this](auto&, const std::string& a, auto) { return handleToggle(a, pCfg->window.acfmWindow); };
        exactMatchHandlers["acfm:disp?"] = [this](auto&, auto&, auto) { std::cout << (pCfg->window.acfmWindow ? "on\n" : "off\n"); return true; };

        exactMatchHandlers["calc1:offset:phase"] = [this](auto&, auto&, float v) { pCfg->post.offset[0].phase = v; return true; };
        exactMatchHandlers["calc1:offset:phase?"] = [this](auto&, auto&, auto) { std::cout << pCfg->post.offset[0].phase << "\n"; return true; };
        exactMatchHandlers["calc2:offset:phase"] = [this](auto&, auto&, float v) { pCfg->post.offset[1].phase = v; return true; };
        exactMatchHandlers["calc2:offset:phase?"] = [this](auto&, auto&, auto) { std::cout << pCfg->post.offset[1].phase << "\n"; return true; };
    }

    // ============================================================
    // ディスパッチ処理
    // ============================================================
    bool dispatchCommand(const std::string& commandPart, const std::vector<std::string>& tokens, const std::string& arg, float val) {
        // 1. 完全一致の検索 (例: "chan1:range")
        if (auto it = exactMatchHandlers.find(commandPart); it != exactMatchHandlers.end()) {
            return it->second(tokens, arg, val);
        }

        // 2. プレフィックスによる検索 (例: "data" や "w1")
        if (!tokens.empty()) {
            const std::string& mainCmd = tokens[0];
            if (auto it = prefixMatchHandlers.find(mainCmd); it != prefixMatchHandlers.end()) {
                return it->second(tokens, arg, val);
            }
        }
        return false;
    }

    // ============================================================
    // 個別ハンドラの実装
    // ============================================================
    bool handleIdn() {
        if (pCfg->pDaq == nullptr) {
            std::cout << "No DAQ is connected.\n";
        }
        else {
            const auto& dev = pCfg->pDaq->device;
            std::cout << std::format("{},{},{},{}\n", dev.manufacturer, dev.name, dev.sn, dev.version);
        }
        return true;
    }

    bool handleError() {
        if (lastErrorCmd.empty()) {
            std::cout << "No error.\n";
        }
        else {
            std::cout << std::format("Last error: '{}'\n", lastErrorCmd);
            lastErrorCmd.clear();
        }
        return true;
    }

    bool handleToggle(const std::string& arg, bool& stateFlag) {
        if (arg == "on") { stateFlag = true; return true; }
        if (arg == "off") { stateFlag = false; return true; }
        return false;
    }

    bool handleData(const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;
        const std::string& subCmd = tokens[1];

        if (subCmd == "raw") {
            if (tokens.size() > 2) {
                if (tokens[2] == "save")  return arg.empty() ? pCfg->saveRawData() : pCfg->saveRawData(arg.c_str());
                if (tokens[2] == "size?") { std::cout << pCfg->raw.waveforms[0].size() << "\n"; return true; }
            }
            return false;
        }

        if (subCmd == "raw?") {
            const double dt = pCfg->scope.getSamplingDt();
            const auto& w0 = pCfg->raw.waveforms[0];
            const auto& w1 = pCfg->raw.waveforms[1];
            for (size_t i = 0; i < w0.size(); ++i) {
                if (!pCfg->isCh2Enabled) std::cout << std::format("{:e},{:e}\n", dt * i, w0[i]);
                else                     std::cout << std::format("{:e},{:e},{:e}\n", dt * i, w0[i], w1[i]);
            }
            return true;
        }

        if (subCmd == "fft") {
            if (tokens.size() > 2) {
                if (tokens[2] == "save") {
                    pCfg->raw.calculateFFT(pCfg->isCh2Enabled, pCfg->awg.ch[0].freq, pCfg->scope.getSamplingDt());
                    return arg.empty() ? pCfg->saveFftData() : pCfg->saveFftData(arg.c_str());
                }
                if (tokens[2] == "size?") {
                    std::cout << pCfg->raw.freqs.size() << "\n";
                    return true;
                }
            }
            return false;
        }

        if (subCmd == "fft?") {
            pCfg->raw.calculateFFT(pCfg->isCh2Enabled, pCfg->awg.ch[0].freq, pCfg->scope.getSamplingDt());
            const auto& freqs = pCfg->raw.freqs;
            const auto& abs0 = pCfg->raw.fftAbs[0];
            const auto& abs1 = pCfg->raw.fftAbs[1];
            for (size_t i = 0; i < freqs.size(); ++i) {
                if (!pCfg->isCh2Enabled) std::cout << std::format("{:e},{:e}\n", freqs[i], abs0[i]);
                else                     std::cout << std::format("{:e},{:e},{:e}\n", freqs[i], abs0[i], abs1[i]);
            }
            return true;
        }

        if (subCmd == "txy?") {
            int size = pCfg->ringBuffer.size;
            if (val > 0) {
                size = static_cast<int>(val / pCfg->ringBuffer.getDt());
                size = std::min(size, pCfg->ringBuffer.size);
            }

            int idx = pCfg->ringBuffer.writeIdx >= size ? pCfg->ringBuffer.writeIdx - size : 0;
            if (pCfg->ringBuffer.writeIdx < size) {
                if (pCfg->ringBuffer.nofm <= static_cast<int>(pCfg->ringBuffer.getMeasurementSize())) {
                    idx = 0;
                    size = pCfg->ringBuffer.writeIdx;
                }
                else {
                    idx = pCfg->ringBuffer.writeIdx - size + pCfg->ringBuffer.getMeasurementSize();
                }
            }

            std::cout << size << "\n";
            const auto& rb = pCfg->ringBuffer;
            for (size_t i = 0; i < size; ++i) {
                if (!pCfg->isCh2Enabled) {
                    std::cout << std::format("{:e},{:e},{:e}\n", rb.times[idx], rb.ch[0].x[idx], rb.ch[0].y[idx]);
                }
                else {
                    std::cout << std::format("{:e},{:e},{:e},{:e},{:e}\n",
                        rb.times[idx], rb.ch[0].x[idx], rb.ch[0].y[idx], rb.ch[1].x[idx], rb.ch[1].y[idx]);
                }
                idx = (idx + 1) % rb.getMeasurementSize();
            }
            return true;
        }

        if (subCmd == "xy?") {
            const size_t idx = pCfg->ringBuffer.latestIdx;
            const auto& rb = pCfg->ringBuffer;
            std::cout << std::format("{:e},{:e}", rb.ch[0].x[idx], rb.ch[0].y[idx]);
            if (pCfg->isCh2Enabled) {
                std::cout << std::format(",{:e},{:e}", rb.ch[1].x[idx], rb.ch[1].y[idx]);
            }
            std::cout << "\n";
            return true;
        }

        return false;
    }

    bool handleDisplay(const std::vector<std::string>& tokens, const std::string&, float val) {
        if (tokens.size() < 3 || tokens[2] != "limit") return false;

        if (tokens[1] == "xy" && val >= 0.01f && val <= pCfg->plot.limit) {
            pCfg->plot.limit = val;
            return true;
        }
        if (tokens[1] == "raw" && val >= 0.1f && val <= pCfg->plot.rawLimit) {
            pCfg->plot.rawLimit = val;
            return true;
        }
        return false;
    }

    bool handleWaveform(bool isW1, const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;

        auto& ch = isW1 ? pCfg->awg.ch[0] : pCfg->awg.ch[1];
        std::string subCmd = tokens[1];
        bool isQuery = (subCmd.back() == '?');
        if (isQuery) subCmd.pop_back();

        if (subCmd == "phase") {
            if (isQuery) { std::cout << ch.phase << "\n"; }
            else { ch.phase = val; awgUpdateRequired = true; }
            return true;
        }

        if (subCmd == "freq" || subCmd == "frequency") {
            if (isQuery) {
                if (arg == "min")      std::cout << pCfg->scope.getLowLimitFreq() << "\n";
                else if (arg == "max") std::cout << pCfg->scope.getHighLimitFreq() << "\n";
                else                   std::cout << ch.freq << "\n";
            }
            else if (val >= pCfg->scope.getLowLimitFreq() && val <= pCfg->scope.getHighLimitFreq()) {
                ch.freq = val;
                awgUpdateRequired = true;
            }
            else return false;
            return true;
        }

        if (subCmd == "volt" || subCmd == "voltage" || subCmd == "amp" || subCmd == "amplitude") {
            if (isQuery) {
                if (arg == "min")      std::cout << pCfg->awg.AWG_AMP_MIN << "\n";
                else if (arg == "max") std::cout << pCfg->awg.AWG_AMP_MAX << "\n";
                else                   std::cout << ch.amp << "\n";
            }
            else if (val >= pCfg->awg.AWG_AMP_MIN && val <= pCfg->awg.AWG_AMP_MAX) {
                ch.amp = val;
                awgUpdateRequired = true;
            }
            else return false;
            return true;
        }

        if (subCmd == "func" || subCmd == "function") {
            if (isQuery) {
                if (ch.func == 1)      std::cout << "sine\n";
                else if (ch.func == 2) std::cout << "square\n";
                else if (ch.func == 3) std::cout << "triangle\n";
                else                   std::cout << "unknown\n";
            }
            else {
                if (arg == "sine" || arg == "sin")        ch.func = 1;
                else if (arg == "square" || arg == "sq")  ch.func = 2;
                else if (arg == "triangle" || arg == "tri") ch.func = 3;
                else return false;
                awgUpdateRequired = true;
            }
            return true;
        }

        return false;
    }

    bool handleCalculate(const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;

        if (tokens[1] == "offset" && tokens.size() > 2) {
            if (tokens[2] == "auto" && tokens.size() > 3 && tokens[3] == "once") {
                pCfg->flagAutoOffset = true;
                return true;
            }
            if (tokens[2] == "state") {
                if (arg == "on") {
                    pCfg->flagAutoOffset = true;
                    return true;
                }
                if (arg == "off") {
                    pCfg->post.offset[0] = { 0, 0, pCfg->post.offset[0].phase }; // 構造体の持ち方に合わせて適宜調整してください
                    pCfg->post.offset[1] = { 0, 0, pCfg->post.offset[1].phase };
                    pCfg->flagAutoOffset = false;
                    return true;
                }
                if (arg == "state?") {
                    std::cout << (pCfg->flagAutoOffset ? "on\n" : "off\n");
                    return true;
                }
            }
        }

        if (tokens[1] == "hpf" && tokens.size() > 2) {
            std::string subCmd = tokens[2];
            bool isQuery = (subCmd.back() == '?');
            if (isQuery) subCmd.pop_back();

            if (subCmd == "freq" || subCmd == "frequency") {
                if (isQuery) {
                    std::cout << pCfg->post.hpFreq << "\n";
                }
                else if (val >= 0.0f && val <= 50.0f) {
                    pCfg->post.hpFreq = val;
                }
                else return false;
                return true;
            }
        }
        return false;
    }
};

// ============================================================
// エントリーポイント（元のAPIを維持）
// ============================================================
void pipe(std::stop_token st, LiaConfig* pCfg) {
    CommandProcessor processor(pCfg);
    processor.processStream(st);
}

// ============================================================
// テストコード
// ============================================================
void test_pipe() {
    std::cout << "--- Starting Fixed Pipe Test ---" << std::endl;

    LiaConfig cfg;
    std::stringstream testInput;
    auto* oldCinBuffer = std::cin.rdbuf(testInput.rdbuf());

    std::stop_source sw;
    std::jthread pipeThread(pipe, sw.get_token(), &cfg);

    while (!cfg.statusPipe) {
        std::this_thread::yield();
    }

    auto sendCommand = [&](const std::string& cmd) {
        testInput << cmd << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };

    const float DEFAULT_AMP = cfg.awg.ch[0].amp;

    std::cout << "[Test] Setting HPF..." << std::endl;
    sendCommand("calc:hpf:freq 15.0");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(cfg.post.hpFreq == 15.0f);

    std::cout << "[Test] Setting W1 Amplitude..." << std::endl;
    sendCommand("w1:amp 1.5");
    assert(cfg.awg.ch[0].amp == 1.5f);

    std::cout << "[Test] Setting CH2 Display ON..." << std::endl;
    sendCommand("chan2:disp on");
    assert(cfg.isCh2Enabled == true);

    std::cout << "[Test] Auto Offset (once)..." << std::endl;
    sendCommand("calc:offset:auto once");
    assert(cfg.flagAutoOffset == true);

    std::cout << "[Test] HPF Frequency..." << std::endl;
    sendCommand("calculate:hpf:freq 25.0");
    assert(cfg.post.hpFreq == 25.0f);

    std::cout << "[Test] Reset Command..." << std::endl;
    sendCommand("reset");
    assert(cfg.awg.ch[0].amp == DEFAULT_AMP);

    std::cout << "[Test] Sending exit and requesting stop..." << std::endl;
    sendCommand("exit");
    sw.request_stop();

    if (pipeThread.joinable()) {
        pipeThread.join();
        std::cout << "[Test] Thread joined successfully!" << std::endl;
    }

    std::cin.rdbuf(oldCinBuffer);
    std::cout << "--- Test Passed ---" << std::endl;
}