#pragma once
#include "Settings.h"
#include <algorithm>   // std::transform
#include <cmath>       // std::round
#include <format>
#include <functional>  // std::function, std::bind, std::placeholders
#include <iostream>
#include <map>         // std::map
#include <sstream>     // std::istringstream
#include <string>
#include <vector>


const std::vector<std::string> helps = {
    "Available commands:",
    "  reset or *rst               : Reset all settings to default values",
    "  *idn?                       : Identify connected DAQ device",
    "  error?                      : Show last error message",
    "  end, exit, quit, close      : Exit the program",
    "  pause                       : Pause data acquisition",
    "  run                         : Resume data acquisition",
    "  data:raw:save [filename]    : Save raw data to file (optional filename)",
    "  data:raw:size?              : Get size of raw data buffer",
    "  data:raw?                   : Output raw data (time, ch1 [, ch2])",
    "  data:txy? [seconds]         : Output time and XY data for specified seconds (default all)",
    "  data:xy?                    : Output latest XY data point",
    std::format("  disp|display:xy:limit <value>    : Set XY display limit (0.01 to {} V)", RAW_RANGE * 1.2f),
    std::format("  disp|display:raw:limit <value>   : Set raw display limit (0.1 to {} V)", RAW_RANGE * 1.2f),
    "  chan2:disp [on|off] or chan2:disp?: Enable/disable or query CH2 display state",
    "  acfm:disp [on|off] or acfm:disp?  : Enable/disable or query ACFM window display state",
    "  w1|w2:freq|frequency [min|max|value] : Set or query waveform frequency",
    "  w1|w2:amp|amplitude [min|max|value]  : Set or query waveform amplitude",
    "  w1|w2:phase or w1|w2:phase? [value]  : Set or query waveform phase in degrees",
    "  calc|calculate:offset:state [on|off] : Enable/disable auto offset (turns off if 'off')",
    "  calc|calculate:offset:auto once      : Perform one-time auto offset",
    "  calc|calculate:hpf:freq|frequency [value]: Set or query high-pass filter frequency (0 to 50 Hz)",
    "  calc1|calc2:offset:phase or calc1|calc2:offset:phase? [value]: Set or query calculation offset phase in degrees",
    "  help? or ?                  : Show this help message",
};

// 文字列分割関数
std::vector<std::string> split(const std::string& str, char delimiter = ':') {
    std::vector<std::string> tokens;
    if (str.empty()) {
        return tokens;
    }
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        if (token.length() == 0) { continue; }
        tokens.push_back(token);
    }
    return tokens;
}


// pipe関数: コマンドラインからの入力を受け取り処理するメイン関数
void pipe(std::stop_token st, Settings* pSettings)
{
    pSettings->statusPipe = true;
    std::string lastErrorCmd = "";
    bool awgUpdateRequired = false;

    // コマンドの処理関数を定義するための型エイリアス
    // 引数: (トークン化されたコマンド, 文字列の引数, 数値の引数) -> 成功/失敗
    using CommandHandler = std::function<bool(const std::vector<std::string>&, const std::string&, float)>;

    // コマンド名と処理関数をマッピングするディスパッチテーブル
    std::map<std::string, CommandHandler> commandMap;

    // コマンドのエイリアスを正規化するマップ (例: "*rst" -> "reset")
    std::map<std::string, std::string> aliasMap = {
        {"*rst", "reset"},
        {"disp", "display"},
        {"calc", "calculate"}
    };

    // トークン化されたコマンドの最初の要素（メインコマンド）を正規化するヘルパー関数
    auto getNormalizedMainCommand = [&](const std::vector<std::string>& tokens, const std::string& originalCmd) -> std::string {
        if (tokens.empty()) return originalCmd;
        std::string mainCmd = tokens[0];
        if (aliasMap.count(mainCmd)) return aliasMap.at(mainCmd);
        return mainCmd;
        };

    // --- 波形設定ハンドラ (w1/w2) ---
    auto waveformHandler = [&](bool isW1, const auto& tokens, const std::string& arg, float val) -> bool {
        if (tokens.size() < 2) return false;

        // 参照でチャンネル設定を取得
        auto& ch = isW1 ? pSettings->awg.ch[0] : pSettings->awg.ch[1];

        static const double lowLimitFreq = 0.5 / (RAW_SIZE * RAW_DT);
        static const double highLimitFreq = std::round(1.0 / (1000 * RAW_DT));
        static constexpr double AWG_AMP_MIN = 0.0;
        static constexpr double AWG_AMP_MAX = 5.0;

        std::string subCmd = tokens[1];
        bool isQuery = subCmd.back() == '?';
        if (isQuery) subCmd.pop_back();

        if (subCmd == "phase") {
            if (isQuery) { std::cout << ch.phase << std::endl; }
            else { ch.phase = val; awgUpdateRequired = true; }
            return true;
        }
        else if (subCmd == "freq" || subCmd == "frequency") {
            if (isQuery) {
                if (arg == "min") { std::cout << lowLimitFreq << std::endl; }
                else if (arg == "max") { std::cout << highLimitFreq << std::endl; }
                else { std::cout << ch.freq << std::endl; }
            }
            else if (val >= lowLimitFreq && val <= highLimitFreq) {
                ch.freq = val; awgUpdateRequired = true;
            }
            else { return false; } // 無効な値
            return true;
        }
        else if (subCmd == "volt" || subCmd == "voltage" || subCmd == "amp" || subCmd == "amplitude") {
            if (isQuery) {
                if (arg == "min") { std::cout << AWG_AMP_MIN << std::endl; }
                else if (arg == "max") { std::cout << AWG_AMP_MAX << std::endl; }
                else { std::cout << ch.amp << std::endl; }
            }
            else if (val >= AWG_AMP_MIN && val <= AWG_AMP_MAX) {
                ch.amp = val; awgUpdateRequired = true;
            }
            else { return false; } // 無効な値
            return true;
        }
        return false;
        };


    // --- コマンドハンドラの定義とマップへの登録 ---

    // リセット
    commandMap["reset"] = [&](const auto&, const auto&, auto) {
        pSettings->reset();
        lastErrorCmd = "";
        awgUpdateRequired = true;
        return true;
        };

    // 識別情報
    commandMap["*idn?"] = [&](const auto&, const auto&, auto) {
        if (pSettings->pDaq == nullptr) {
            std::cout << "No DAQ is connected." << std::endl;
        }
        else {
            std::cout << std::format("{},{},{},{}\n",
                pSettings->pDaq->device.manufacturer, pSettings->pDaq->device.name,
                pSettings->pDaq->device.sn, pSettings->pDaq->device.version);
        }
        return true;
        };

    // エラー問い合わせ
    commandMap["error?"] = [&](const auto&, const auto&, auto) {
        if (lastErrorCmd.empty()) {
            std::cout << "No error." << std::endl;
        }
        else {
            std::cout << std::format("Last error: '{}'\n", lastErrorCmd);
            lastErrorCmd.clear();
        }
        return true;
        };

    // 一時停止・再開
    commandMap["pause"] = [&](const auto&, const auto&, auto) { pSettings->flagPause = true; return true; };
    commandMap["run"] = [&](const auto&, const auto&, auto) { pSettings->flagPause = false; return true; };

    // ヘルプ
    commandMap["help?"] = commandMap["?"] = [&](const auto&, const auto&, auto) {
        for (auto& line : helps) {
            std::cout << line << std::endl;
        }
        return true;
        };
    commandMap["help"] = [&](const auto& tokens, const auto&, auto) {
        if (tokens.size() > 1 && tokens[1] == "size?") {
            std::cout << helps.size() << std::endl;
            return true;
        }
        return false;
        };

    // データ関連
    commandMap["data"] = [&](const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;
        const std::string& subCmd = tokens[1];

        if (subCmd == "raw") {
            if (tokens.size() > 2) {
                if (tokens[2] == "save") {
                    return arg.empty() ? pSettings->saveRawData() : pSettings->saveRawData(arg.c_str());
                }
                else if (tokens[2] == "size?") {
                    std::cout << pSettings->rawData1.size() << std::endl;
                    return true;
                }
            }
        }
        else if (subCmd == "raw?") {
            for (int i = 0; i < pSettings->rawData1.size(); ++i) {
                if (!pSettings->flagCh2) {
                    std::cout << std::format("{:e},{:e}\n", RAW_DT * i, pSettings->rawData1[i]);
                }
                else {
                    std::cout << std::format("{:e},{:e},{:e}\n", RAW_DT * i, pSettings->rawData1[i], pSettings->rawData2[i]);
                }
            }
            return true;
        }
        else if (subCmd == "txy?") {
            int size = pSettings->size;
            if (val > 0) {
                size = val / MEASUREMENT_DT;
                if (size > pSettings->size) size = pSettings->size;
            }
            int idx = pSettings->tail - size;
            if (idx < 0) {
                if (pSettings->nofm <= MEASUREMENT_SIZE) {
                    idx = 0;
                    size = pSettings->tail;
                }
                else {
                    idx += MEASUREMENT_SIZE;
                }
            }
            std::cout << size << std::endl;
            for (int i = 0; i < size; ++i) {
                if (!pSettings->flagCh2) {
                    std::cout << std::format("{:e},{:e},{:e}\n", pSettings->times[idx], pSettings->x1s[idx], pSettings->y1s[idx]);
                }
                else {
                    std::cout << std::format("{:e},{:e},{:e},{:e},{:e}\n", pSettings->times[idx], pSettings->x1s[idx], pSettings->y1s[idx], pSettings->x2s[idx], pSettings->y2s[idx]);
                }
                idx = (idx + 1) % MEASUREMENT_SIZE;
            }
            return true;
        }
        else if (subCmd == "xy?") {
            size_t idx = pSettings->idx;
            std::cout << std::format("{:e},{:e}", pSettings->x1s[idx], pSettings->y1s[idx]);
            if (pSettings->flagCh2) {
                std::cout << std::format(",{:e},{:e}", pSettings->x2s[idx], pSettings->y2s[idx]);
            }
            std::cout << std::endl;
            return true;
        }
        return false;
        };

    // 表示関連
    commandMap["display"] = [&](const auto& tokens, const auto&, float val) {
        if (tokens.size() < 3) return false;
        if (tokens[1] == "xy" && tokens[2] == "limit") {
            if (val >= 0.01 && val <= RAW_RANGE * 1.2) {
                pSettings->plot.limit = val;
                return true;
            }
        }
        else if (tokens[1] == "raw" && tokens[2] == "limit") {
            if (val >= 0.1 && val <= RAW_RANGE * 1.2) {
                pSettings->plot.rawLimit = val;
                return true;
            }
        }
        return false;
        };

    // CH2表示設定 (フルコマンドで登録)
    commandMap[":chan2:disp"] = commandMap["chan2:disp"] = [&](const auto&, const std::string& arg, auto) {
        if (arg == "on") { pSettings->flagCh2 = true; return true; }
        if (arg == "off") { pSettings->flagCh2 = false; return true; }
        return false;
        };
    commandMap[":chan2:disp?"] = commandMap["chan2:disp?"]  = [&](const auto&, const auto&, auto) {
        std::cout << (pSettings->flagCh2 ? "on" : "off") << std::endl;
        return true;
        };

    // ACFM表示設定 (フルコマンドで登録)
    commandMap[":acfm:disp"] = commandMap["acfm:disp"] = [&](const auto&, const std::string& arg, auto) {
        if (arg == "on") { pSettings->plot.acfm = true; return true; }
        if (arg == "off") { pSettings->plot.acfm = false; return true; }
        return false;
        };
    commandMap[":acfm:disp?"] = commandMap["acfm:disp?"] = [&](const auto&, const auto&, auto) {
        std::cout << (pSettings->plot.acfm ? "on" : "off") << std::endl;
        return true;
        };

    // 波形設定 (W1, W2) - std::bind を使用
    commandMap["w1"] = std::bind(waveformHandler, true, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    commandMap["w2"] = std::bind(waveformHandler, false, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 計算関連
    commandMap["calculate"] = [&](const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;

        if (tokens[1] == "offset") {
            if (tokens.size() > 2) {
                if (tokens[2] == "auto") {
                    if (tokens.size() > 3 && tokens[3] == "once") {
                        pSettings->flagAutoOffset = true; return true;
                    }
                }
                else if (tokens[2] == "state") {
                    if (arg == "on") { pSettings->flagAutoOffset = true; return true; }
                    if (arg == "off") {
                        pSettings->post.offset1X = pSettings->post.offset1Y = 0;
                        pSettings->post.offset2X = pSettings->post.offset2Y = 0;
                        pSettings->flagAutoOffset = false; // state off で自動オフセットもオフにする
                        return true;
                    }
                    if (arg == "state?") {
                        std::cout << (pSettings->flagAutoOffset ? "on" : "off") << std::endl;
                        return true;
                    }
                }
            }
        }
        else if (tokens[1] == "hpf" && tokens.size() > 2) {
            std::string subCmd = tokens[2];
            bool isQuery = subCmd.back() == '?';
            if (isQuery) subCmd.pop_back();

            if (subCmd == "freq" || subCmd == "frequency") {
                if (isQuery) { std::cout << pSettings->post.hpFreq << std::endl; }
                else if (val >= 0.0 && val <= 50.0) { pSettings->post.hpFreq = val; }
                else { return false; }
                return true;
            }
        }
        return false;
        };

    // 計算オフセット位相 (フルコマンドで登録)
    commandMap[":calc1:offset:phase"] = commandMap["calc1:offset:phase"] = [&](const auto&, const auto&, float val) { pSettings->post.offset1Phase = val; return true; };
    commandMap[":calc2:offset:phase"] = commandMap["calc2:offset:phase"] = [&](const auto&, const auto&, float val) { pSettings->post.offset2Phase = val; return true; };
    commandMap[":calc1:offset:phase?"] = commandMap["calc1:offset:phase?"] = [&](const auto&, const auto&, auto) { std::cout << pSettings->post.offset1Phase << std::endl; return true; };
    commandMap[":calc2:offset:phase?"] = commandMap["calc2:offset:phase?"] = [&](const auto&, const auto&, auto) { std::cout << pSettings->post.offset2Phase << std::endl; return true; };


    // --- メインループ ---
    while (!st.stop_requested())
    {
        std::string line;
        if (!std::getline(std::cin, line) || line.empty()) {
            continue;
        }

        std::string original_line = line;
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        std::string commandPart, argumentPart;
        float value = 0;
        std::istringstream iss(line);
        iss >> commandPart;

        // 終了コマンドの優先処理
        if (commandPart == "end" || commandPart == "exit" || commandPart == "quit" || commandPart == "close") {
            break;
        }

        // 残りの入力から引数を取得
        iss >> argumentPart;

        try {
            value = std::stof(argumentPart);
        }
        catch (const std::exception&) {
            value = 0; // stofに失敗した場合は数値引数がないと見なす
        }

        // コマンドをコロンで分割
        auto tokens = split(commandPart, ':');

        // コマンドのディスパッチ
        bool commandOk = false;

        // 1. 完全一致するコマンド（例: chan2:disp, calc1:offset:phase）を検索
        if (auto it_full = commandMap.find(commandPart); it_full != commandMap.end()) {
            commandOk = it_full->second(tokens, argumentPart, value);
        }
        // 2. 正規化されたメインコマンド（例: reset, data, display）を検索
        else if (!tokens.empty()) {
            const std::string mainCommand = getNormalizedMainCommand(tokens, commandPart);
            if (auto it = commandMap.find(mainCommand); it != commandMap.end()) {
                commandOk = it->second(tokens, argumentPart, value);
            }
        }


        if (!commandOk) {
            // エラー処理
            lastErrorCmd = original_line;
            if (commandPart.find('?') != std::string::npos) {
                // クエリが失敗した場合は、エラーメッセージを表示してエラー履歴はクリア
                std::cout << std::format("Error: '{}'\n", lastErrorCmd);
                lastErrorCmd.clear();
            }
            // else: 設定コマンドが失敗した場合は、エラー履歴に保存
        }

        // AWG（任意波形発生器）の更新が必要な場合
        if (awgUpdateRequired && pSettings->pDaq != nullptr) {
            pSettings->pDaq->awg.start(
                pSettings->awg.ch[0].freq, pSettings->awg.ch[0].amp, pSettings->awg.ch[0].phase,
                pSettings->awg.ch[1].freq, pSettings->awg.ch[1].amp, pSettings->awg.ch[1].phase
            );
            awgUpdateRequired = false;
        }

        std::cout << std::flush;
    }
    pSettings->statusPipe = false;
}