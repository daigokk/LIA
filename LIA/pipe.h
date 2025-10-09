#pragma once
#include "Settings.h" // 元のコードで必要だったヘッダー
#include <algorithm>   // std::transform
#include <cmath>       // std::round
#include <format>
#include <functional>  // std::function
#include <iostream>
#include <map>         // std::map
#include <sstream>     // std::istringstream
#include <string>
#include <vector>

// 文字列分割関数
std::vector<std::string> split(const std::string& str, char delimiter=':') {
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

    // --- コマンドハンドラの定義 ---

    //// 終了コマンド
    //auto exitHandler = [&](const auto&, const auto&, auto) {
    //    return false; // falseを返すとメインループが終了する
    //    };
    //commandMap["end"] = commandMap["exit"] = commandMap["quit"] = exitHandler;

    // リセットコマンド
    commandMap["reset"] = commandMap["*rst"] = [&](const auto&, const auto&, auto) {
        // pSettingsの各メンバを初期値にリセット
        pSettings->reset();
        lastErrorCmd = "";
        awgUpdateRequired = true;
        return true;
        };

    // 識別情報コマンド
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

    // データ関連コマンド
    commandMap["data"] = [&](const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;
        const std::string& subCmd = tokens[1];

        if (subCmd == "raw" && tokens.size() > 2) {
            if (tokens[2] == "save") {
                return arg.empty() ? pSettings->saveRawData() : pSettings->saveRawData(arg.c_str());
            }
            else if (tokens[2] == "size?") {
                std::cout << pSettings->rawData1.size() << std::endl;
                return true;
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

    // 表示関連コマンド
    commandMap["display"] = commandMap["disp"] = [&](const auto& tokens, const auto&, float val) {
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

    // CH2表示設定
    commandMap[":chan2:disp"] = commandMap["chan2:disp"] = [&](const auto&, const std::string& arg, auto) {
        if (arg == "on") { pSettings->flagCh2 = true; return true; }
        if (arg == "off") { pSettings->flagCh2 = false; return true; }
        return false;
        };

    // ACFM表示設定
    commandMap[":acfm:disp"] = commandMap["acfm:disp"] = [&](const auto&, const std::string& arg, auto) {
        if (arg == "on") { pSettings->plot.acfm = true; return true; }
        if (arg == "off") { pSettings->plot.acfm = false; return true; }
        return false;
        };

    // 波形設定 (W1, W2)
    auto waveformHandler = [&](bool isW1, const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;

        float& freq = isW1 ? pSettings->awg.ch[0].freq : pSettings->awg.ch[1].freq;
        float& amp = isW1 ? pSettings->awg.ch[0].amp : pSettings->awg.ch[1].amp;
        float& phase = isW1 ? pSettings->awg.ch[0].phase : pSettings->awg.ch[1].phase;

        static const double lowLimitFreq = 0.5 / (RAW_SIZE * RAW_DT);
        static const double highLimitFreq = std::round(1.0 / (1000 * RAW_DT));
        static constexpr double VOLT_MIN = 0.0;
        static constexpr double VOLT_MAX = 5.0;

        std::string subCmd = tokens[1];
        bool isQuery = subCmd.back() == '?';
        if (isQuery) subCmd.pop_back();

        if (subCmd == "phase") {
            if (isQuery) { std::cout << phase << std::endl; }
            else { phase = val; awgUpdateRequired = true; }
            return true;
        }
        if (subCmd == "freq" || subCmd == "frequency") {
            if (isQuery) {
                if (arg == "min") { std::cout << lowLimitFreq << std::endl; }
                else if (arg == "max") { std::cout << highLimitFreq << std::endl; }
                else { std::cout << freq << std::endl; }
            }
            else if (val >= lowLimitFreq && val <= highLimitFreq) {
                freq = val; awgUpdateRequired = true;
            }
            else { return false; }
            return true;
        }
        if (subCmd == "volt" || subCmd == "voltage") {
            if (isQuery) {
                if (arg == "min") { std::cout << VOLT_MIN << std::endl; }
                else if (arg == "max") { std::cout << VOLT_MAX << std::endl; }
                else { std::cout << amp << std::endl; }
            }
            else if (val >= VOLT_MIN && val <= VOLT_MAX) {
                amp = val; awgUpdateRequired = true;
            }
            else { return false; }
            return true;
        }
        return false;
        };
    commandMap["w1"] = std::bind(waveformHandler, true, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    commandMap["w2"] = std::bind(waveformHandler, false, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 計算関連コマンド
    commandMap["calculate"] = commandMap["calc"] = [&](const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) return false;
        if (tokens[1] == "offset" && tokens.size() > 2) {
            if (tokens[2] == "auto" && tokens.size() > 3 && tokens[3] == "once") {
                pSettings->flagAutoOffset = true; return true;
            }
            if (tokens[2] == "state") {
                if (arg == "on") { pSettings->flagAutoOffset = true; return true; }
                if (arg == "off") {
                    pSettings->post.offset1X = pSettings->post.offset1Y = 0;
                    pSettings->post.offset2X = pSettings->post.offset2Y = 0;
                    return true;
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
    commandMap[":calc1:offset:phase"] = commandMap["calc1:offset:phase"] = [&](const auto&, const auto&, float val) { pSettings->post.offset1Phase = val; return true; };
    commandMap[":calc2:offset:phase"] = commandMap["calc2:offset:phase"] = [&](const auto&, const auto&, float val) { pSettings->post.offset2Phase = val; return true; };

    commandMap["help"] = [&](const auto& tokens, const auto&, auto) {
        if (tokens.size() < 2) return false;
        const std::string& subCmd = tokens[1];
		if (subCmd == "size?") { std::cout << 24 << std::endl; return true; }
        };
    commandMap["?"] = commandMap["help?"] = [&](const auto&, const auto&, auto) {
        std::cout << "Available commands:\n";
        std::cout << "  reset or *rst               : Reset all settings to default values\n";
        std::cout << "  *idn?                       : Identify connected DAQ device\n";
        std::cout << "  error?                      : Show last error message\n";
        std::cout << "  end, exit, quit, close      : Exit the program\n";
        std::cout << "  pause                       : Pause data acquisition\n";
        std::cout << "  run                         : Resume data acquisition\n";
        std::cout << "  data:raw:save [filename]    : Save raw data to file (optional filename)\n";
        std::cout << "  data:raw:size?              : Get size of raw data buffer\n";
        std::cout << "  data:raw?                   : Output raw data (time, ch1 [, ch2])\n";
        std::cout << "  data:txy? [seconds]         : Output time and XY data for specified seconds (default all)\n";
        std::cout << "  data:xy?                    : Output latest XY data point\n";
        std::cout << "  disp|display:xy:limit <value>    : Set XY display limit (0.01 to " << RAW_RANGE * 1.2f << " V)\n";
        std::cout << "  disp|display:raw:limit <value>   : Set raw display limit (0.1 to " << RAW_RANGE * 1.2f << " V)\n";
        std::cout << "  chan2:disp on|off           : Enable or disable CH2 display\n";
		std::cout << "  acfm:disp on|off            : Enable or disable ACFM windo display\n";
        std::cout << "  w1|w2:freq|frequency [min|max|value] : Set or query waveform frequency\n";
        std::cout << "  w1|w2:volt|voltage [min|max|value]   : Set or query waveform voltage\n";
        std::cout << "  w1|w2:phase [value]         : Set or query waveform phase in degrees\n";
        std::cout << "  calc|calculate:offset:state on|off   : Enable or disable auto offset\n";
        std::cout << "  calc|calculate:offset:auto once      : Perform one-time auto offset\n";
		std::cout << "  calc|calculate:hpf:freq|frequency [value]    : Set or query high-pass filter frequency (0 to 50 Hz)\n";
        std::cout << "  calc1|calc2:offset:phase [value]     : Set calculation offset phase in degrees\n";
        std::cout << "  help? or ?                  : Show this help message\n";
        return true;
		};

    // --- メインループ ---
    while (!st.stop_requested())
    {
        std::string line;
        if (!std::getline(std::cin, line) || line.empty()) {
            continue;
        }

        std::string original_line = line; // エラーメッセージ表示用に元の入力を保持
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        std::string commandPart, argumentPart;
        float value = 0;
        std::istringstream iss(line);
        iss >> commandPart;
        iss >> argumentPart; // 最初の引数（主に数値）を取得

        if (commandPart == "end" || commandPart == "exit" || commandPart == "quit" || commandPart == "close") break;

        try {
            value = std::stof(argumentPart);
        }
        catch (const std::exception&) {
            // stofに失敗してもコマンド自体は有効な場合があるため、valueは0のまま継続
            value = 0;
        }

        // コマンドをコロンで分割
        auto tokens = split(commandPart, ':');
        if (tokens.empty() && commandPart.length() > 0) { // :で始まらないコマンド用
            tokens.push_back(commandPart);
        }
        else if (tokens.empty() && commandPart.empty()) {
            continue;
        }

        // コマンドのディスパッチ
        bool commandOk = false;
        const std::string& mainCommand = tokens[0].empty() ? commandPart : tokens[0]; // :で始まるコマンドか判定

        if (auto it = commandMap.find(mainCommand); it != commandMap.end()) {
            // マップからハンドラを見つけて実行
            commandOk = it->second(tokens, argumentPart, value);
        }
        else if (auto it_full = commandMap.find(commandPart); it_full != commandMap.end()) {
            // :chan2:disp のようなフルパスのコマンドを検索
            commandOk = it_full->second(tokens, argumentPart, value);
        }

        if (!commandOk) {
            //if (commandMap.count(mainCommand) && mainCommand.length() > 2) break; // 終了コマンド
            // エラー処理
            lastErrorCmd = original_line;
            if (commandPart.find('?') != std::string::npos) {
                // クエリが失敗した場合は、エラーメッセージを表示してエラー履歴はクリア
                std::cout << std::format("Error: '{}'\n", lastErrorCmd);
                lastErrorCmd.clear();
            }
            else {
                // 設定コマンドが失敗した場合は、エラー履歴に保存
                //std::cout << std::format("Error: Invalid command or parameter for '{}'\n", lastErrorCmd);
            }
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