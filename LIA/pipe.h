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
#include <thread>
#include <regex>

// ============================================================
// 定数定義
// ============================================================
constexpr int MAX_CHANNELS = 3; // ★ 3チャンネルまで対応

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
    "  data:raw?                    : Output raw data (time, ch1 [, ch2, ch3])",
    "  data:fft:save [filename]     : Save FFT data to file (optional filename)",
    "  data:fft:size?               : Get size of FFT data buffer",
    "  data:fft?                    : Output FFT data (frequency, ch1 [, ch2, ch3])",
    "  data:txy? [seconds]          : Output time and XY data for specified seconds (default all)",
    "  data:xy?                     : Output latest XY data point",
    "  plot:raw:limit <val>         : Set raw window limit",
    "  plot:xy:limit <val>          : Set XY window limit",
    "  acfm|disp [on|off|?]         : Enable/disable or query ACFM window display state",
    "  chan[n]:range [value|?]      : Set or query channel range",
    "  chan2:disp [on|off|?]        : Enable/disable or query channel display state",
    "  w[n]:freq [min|max|value]    : Set or query waveform frequency",
    "  w[n]:amp [min|max|value]     : Set or query waveform amplitude",
    "  w[n]:phase [value|?]         : Set or query waveform phase in degrees",
    "  post:offset:state [on|off|?] : Enable/disable offset state",
    "  post:offset:auto once        : Perform one-time auto offset",
    "  post:hpf:freq [value|?]      : Set or query high-pass filter frequency (0 to 50 Hz)",
    "  post:lpf:freq [value|?]      : Set or query low-pass filter frequency (10 to 100 Hz)",
    "  post[n]:offset:phase [val|?] : Set or query calculation offset phase in degrees",
    "  help? or ?                   : Show this help message",
};

// ============================================================
// ユーティリティ
// ============================================================
namespace utils {
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

    inline void toLower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    }

    inline std::pair<std::string, int> parseIndex(const std::string& s) {
        std::regex re("([a-z]+)([0-9]+)");
        std::smatch match;
        if (std::regex_match(s, match, re)) {
            return { match[1].str(), std::stoi(match[2].str()) };
        }
        return { s, -1 };
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
                break;
            }

            std::string original_line = line;
            utils::toLower(line);

            std::istringstream iss(line);
            std::string commandPart, argumentPart;
            iss >> commandPart;

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

            if (!commandPart.empty() && commandPart.front() == ':') {
                commandPart.erase(0, 1);
            }

            auto tokens = utils::split(commandPart, ':');
            bool success = dispatchCommand(commandPart, tokens, argumentPart, value);

            if (!success) {
                lastErrorCmd = original_line;
                if (commandPart.find('?') != std::string::npos) {
                    std::cout << std::format("Error: '{}'\n", lastErrorCmd);
                    lastErrorCmd.clear();
                }
            }

            if (awgUpdateRequired && pCfg->pDaq != nullptr) {
                // ※ AWGのstart関数が3chに対応する場合は引数を追加してください
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

    // ★ ハンドラの型に channel index (int) を追加
    using CommandHandler = std::function<bool(int, const std::vector<std::string>&, const std::string&, float)>;

    std::map<std::string, CommandHandler> exactMatchHandlers;
    std::map<std::string, CommandHandler> prefixMatchHandlers;

    // ============================================================
    // コマンド登録
    // ============================================================
    void registerCommands() {
        // --- システム・基本操作 ---
        auto resetHandler = [this](int, auto&, auto&, auto) {
            pCfg->reset();
            lastErrorCmd.clear();
            awgUpdateRequired = true;
            return true;
            };
        exactMatchHandlers["reset"] = resetHandler;
        exactMatchHandlers["*rst"] = resetHandler;

        exactMatchHandlers["*idn?"] = [this](int, auto&, auto&, auto) { return handleIdn(); };
        exactMatchHandlers["error?"] = [this](int, auto&, auto&, auto) { return handleError(); };
        exactMatchHandlers["pause"] = [this](int, auto&, auto&, auto) { pCfg->pause.flag = true; return true; };
        exactMatchHandlers["stop"] = [this](int, auto&, auto&, auto) { pCfg->pause.flag = true; return true; };
        exactMatchHandlers["run"] = [this](int, auto&, auto&, auto) { pCfg->pause.flag = false; return true; };

        auto helpHandler = [this](int, auto&, auto&, auto) {
            for (const auto& line : HELPS) std::cout << line << "\n";
            return true;
            };
        exactMatchHandlers["help?"] = helpHandler;
        exactMatchHandlers["?"] = helpHandler;
        exactMatchHandlers["help"] = [this](int, const auto& tokens, auto&, auto) {
            if (tokens.size() > 1 && tokens[1] == "size?") {
                std::cout << HELPS.size() << "\n";
                return true;
            }
            return false;
            };

        exactMatchHandlers["acfm:disp"] = [this](int, auto&, const std::string& a, auto) { return handleToggle(a, pCfg->window.acfmWindow); };
        exactMatchHandlers["acfm:disp?"] = [this](int, auto&, auto&, auto) { std::cout << (pCfg->window.acfmWindow ? "on\n" : "off\n"); return true; };

        // --- プレフィックス(階層型)コマンド ---
        prefixMatchHandlers["data"] = [this](int ch, auto& t, auto& a, auto v) { return handleData(t, a, v); }; // dataはchIndexを使わない
        prefixMatchHandlers["plot"] = [this](int ch, auto& t, auto& a, auto v) { return handlePlot(t, a, v); };
        prefixMatchHandlers["w"] = [this](int ch, auto& t, auto& a, auto v) { return handleWaveform(ch, t, a, v); };
        prefixMatchHandlers["post"] = [this](int ch, auto& t, auto& a, auto v) { return handlePost(ch, t, a, v); };;
        prefixMatchHandlers["chan"] = [this](int ch, auto& t, auto& a, auto v) { return handleChan(ch, t, a, v); };
    }

    // ============================================================
    // ディスパッチ処理
    // ============================================================
    bool dispatchCommand(const std::string& commandPart, const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (auto it = exactMatchHandlers.find(commandPart); it != exactMatchHandlers.end()) {
            return it->second(-1, tokens, arg, val);
        }

        if (!tokens.empty()) {
            // "chan1", "w2", "calc3" などからベースコマンドとインデックスを分離
            auto [baseCmd, index] = utils::parseIndex(tokens[0]);

            // 0ベースインデックスに変換 (例: chan1 -> 0, chan2 -> 1, chan3 -> 2)
            int chIndex = (index > 0) ? (index - 1) : -1;

            if (auto it = prefixMatchHandlers.find(baseCmd); it != prefixMatchHandlers.end()) {
                return it->second(chIndex, tokens, arg, val);
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

    // ★ チャンネル処理 (chan[n]:range, chan[n]:disp)
    bool handleChan(int chIndex, const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2 || chIndex < 0 || chIndex >= MAX_CHANNELS) return false;

        std::string subCmd = tokens[1];
        bool isQuery = (subCmd.back() == '?');
        if (isQuery) subCmd.pop_back();

        if (subCmd == "range") {
            if (isQuery) {
                std::cout << pCfg->scope.ch[chIndex].range << "\n";
            }
            else {
                pCfg->scope.ch[chIndex].range = val;
                pCfg->scope.setMaxRange();

                // ※ scope.open関数が3chに対応する場合は引数を追加してください
                pCfg->pDaq->scope.open(pCfg->scope.ch[0].range, pCfg->scope.ch[1].range, pCfg->scope.getBufferSize(), 1.0 / pCfg->scope.getSamplingDt());
                pCfg->pDaq->scope.trigger();
                pCfg->pDaq->scope.start();
            }
            return true;
        }

        if (subCmd == "disp") {
            if (isQuery) {
                // ※ LiaConfig側を `bool isChEnabled[MAX_CHANNELS];` に変更している前提
                std::cout << (pCfg->scope.ch[chIndex].enable ? "on\n" : "off\n");
            }
            else {
                return handleToggle(arg, pCfg->scope.ch[chIndex].enable);
            }
            return true;
        }

        return false;
    }

    // ★ 階層型コマンドとして集約された Data 処理
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
            for (size_t i = 0; i < w0.size(); ++i) {
                std::cout << std::format("{:e},{:e}", dt * i, w0[i]);
                for (int c = 1; c < MAX_CHANNELS; ++c) {
                    if (pCfg->scope.ch[c].enable) {
                        std::cout << std::format(",{:e}", pCfg->raw.waveforms[c][i]);
                    }
                }
                std::cout << "\n";
            }
            return true;
        }

        if (subCmd == "fft") {
            if (tokens.size() > 2) {
                if (tokens[2] == "save") {
                    pCfg->raw.calculateFFT(pCfg->scope.ch[1].enable, pCfg->awg.ch[0].freq, pCfg->scope.getSamplingDt()); // ※引数要確認
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
            pCfg->raw.calculateFFT(pCfg->scope.ch[1].enable, pCfg->awg.ch[0].freq, pCfg->scope.getSamplingDt()); // ※引数要確認
            const auto& freqs = pCfg->raw.freqs;
            for (size_t i = 0; i < freqs.size(); ++i) {
                std::cout << std::format("{:e},{:e}", freqs[i], pCfg->raw.fftAbs[0][i]);
                for (int c = 1; c < MAX_CHANNELS; ++c) {
                    if (pCfg->scope.ch[c].enable) {
                        std::cout << std::format(",{:e}", pCfg->raw.fftAbs[c][i]);
                    }
                }
                std::cout << "\n";
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
                std::cout << std::format("{:e},{:e},{:e}", rb.times[idx], rb.ch[0].x[idx], rb.ch[0].y[idx]);
                for (int c = 1; c < MAX_CHANNELS; ++c) {
                    if (pCfg->scope.ch[c].enable) {
                        std::cout << std::format(",{:e},{:e}", rb.ch[c].x[idx], rb.ch[c].y[idx]);
                    }
                }
                std::cout << "\n";
                idx = (idx + 1) % rb.getMeasurementSize();
            }
            return true;
        }

        if (subCmd == "xy?") {
            const size_t idx = pCfg->ringBuffer.latestIdx;
            const auto& rb = pCfg->ringBuffer;
            std::cout << std::format("{:e},{:e}", rb.ch[0].x[idx], rb.ch[0].y[idx]);
            for (int c = 1; c < MAX_CHANNELS; ++c) {
                if (pCfg->scope.ch[c].enable) {
                    std::cout << std::format(",{:e},{:e}", rb.ch[c].x[idx], rb.ch[c].y[idx]);
                }
            }
            std::cout << "\n";
            return true;
        }

        return false;
    }

    bool handlePlot(const std::vector<std::string>& tokens, const std::string&, float val) {
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

    // ★ 波形処理 (w[n]:amp, w[n]:freq など)
    bool handleWaveform(int chIndex, const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2 || chIndex < 0 || chIndex >= MAX_CHANNELS) return false;

        auto& ch = pCfg->awg.ch[chIndex];
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
                if (arg == "sine" || arg == "sin")          ch.func = 1;
                else if (arg == "square" || arg == "sq")    ch.func = 2;
                else if (arg == "triangle" || arg == "tri") ch.func = 3;
                else return false;
                awgUpdateRequired = true;
            }
            return true;
        }

        return false;
    }

    // ★ 後処理 (post[n]:offset:phase, post:hpf など)
    bool handlePost(int chIndex, const std::vector<std::string>& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;

        if (tokens[1] == "offset" && tokens.size() > 2) {
            std::string subCmd = tokens[2];
            bool isQuery = (subCmd.back() == '?');
            if (isQuery) subCmd.pop_back();

            if (subCmd == "auto" && tokens.size() > 3 && tokens[3] == "once") {
                pCfg->flagAutoOffset = true;
                return true;
            }
            if (subCmd == "state") {
                if (arg == "on") {
                    pCfg->flagAutoOffset = true;
                    return true;
                }
                if (arg == "off") {
                    for (int i = 0; i < MAX_CHANNELS; ++i) {
                        pCfg->post.offset[i] = { 0, 0, pCfg->post.offset[i].phase };
                    }
                    pCfg->flagAutoOffset = false;
                    return true;
                }
                if (isQuery) {
                    std::cout << (pCfg->flagAutoOffset ? "on\n" : "off\n");
                    return true;
                }
            }

            // calc[n]:offset:phase の処理
            if (subCmd == "phase" && chIndex >= 0 && chIndex < MAX_CHANNELS) {
                if (isQuery) {
                    std::cout << pCfg->post.offset[chIndex].phase << "\n";
                }
                else {
                    pCfg->post.offset[chIndex].phase = val;
                }
                return true;
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
		else if (tokens[1] == "lpf" && tokens.size() > 2) {
			std::string subCmd = tokens[2];
			bool isQuery = (subCmd.back() == '?');
			if (isQuery) subCmd.pop_back();
			if (subCmd == "freq" || subCmd == "frequency") {
				if (isQuery) {
					std::cout << pCfg->post.lpFreq << "\n";
				}
				else if (val >= 10.0f && val <= 100.0f) {
					pCfg->post.lpFreq = val;
				}
				else return false;
				return true;
			}
		}
        return false;
    }
};

// ============================================================
// エントリーポイント
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

    std::cout << "[Test] Setting CH2 & CH3 Display ON..." << std::endl;
    sendCommand("chan2:disp on");
    sendCommand("chan3:disp on"); // 追加テスト
    assert(cfg.isChEnabled[1] == true); // ★ LiaConfig側の配列化前提
    assert(cfg.isChEnabled[2] == true);

    std::cout << "[Test] Auto Offset (once)..." << std::endl;
    sendCommand("calc:offset:auto once");
    assert(cfg.flagAutoOffset == true);

    std::cout << "[Test] Calc3 offset phase..." << std::endl;
    sendCommand("calc3:offset:phase 45.0"); // 追加テスト
    assert(cfg.post.offset[2].phase == 45.0f);

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