#pragma once
#include "LiaConfig.h"
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
    "  end, exit, quit or close    : Exit the program",
    "  pause or stop                 : Pause data acquisition",
    "  run                         : Resume data acquisition",
    "  data:raw:save [filename]    : Save raw data to file (optional filename)",
    "  data:raw:size?              : Get size of raw data buffer",
    "  data:raw?                   : Output raw data (time, ch1 [, ch2])",
	"  data:fft:save [filename]    : Save FFT data to file (optional filename)",
    "  data:fft:size?              : Get size of FFT data buffer",
	"  data:fft?                   : Output FFT data (frequency, ch1 [, ch2])",
    "  data:txy? [seconds]         : Output time and XY data for specified seconds (default all)",
    "  data:xy?                    : Output latest XY data point",
    "  disp|display:xy:limit <value>    : Set XY display limit",
    "  disp|display:raw:limit <value>   : Set raw display limit",
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

// ============================================================
// 文字列分割ユーティリティ
// ============================================================
// 指定された区切り文字で文字列を分割し、トークンベクトルを返す
// 空のトークンはスキップされる
std::vector<std::string> split(const std::string& str, char delimiter = ':') {
    std::vector<std::string> tokens;

    // 空の文字列は即座に空のベクトルを返す
    if (str.empty()) {
        return tokens;
    }

    // ストリームを使用して文字列を区切り文字で分割
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        // 空のトークンはスキップ（連続する区切り文字の場合）
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}


// ============================================================
// パイプ処理（コマンドライン入力の処理）
// ============================================================
// ユーザーからのコマンド入力を受け取り、DAQ デバイスの制御を実施
void pipe(std::stop_token st, LiaConfig* pCfg)
{
    pCfg->statusPipe = true;
    std::string lastErrorCmd = "";
    bool awgUpdateRequired = false;

    // ============================================================
    // 型定義
    // ============================================================
    // コマンドハンドラの関数型：
    //   入力: (トークン化コマンド, 文字列引数, 数値引数)
    //   出力: bool (成功 = true, 失敗 = false)
    using CommandHandler = std::function<bool(const std::vector<std::string>&, const std::string&, float)>;

    // ============================================================
    // コマンド設定とエイリアスマップ
    // ============================================================
    std::map<std::string, CommandHandler> commandMap;

    // コマンドエイリアスの正規化マップ
    // 例：*rst → reset、disp → display
    std::map<std::string, std::string> aliasMap = {
        {"*rst", "reset"},
        {"disp", "display"},
        {"calc", "calculate"}
    };

    // ============================================================
    // ヘルパー関数
    // ============================================================
    // トークン化されたコマンドの最初の要素をエイリアスから正規化
    auto getNormalizedMainCommand = [&](const std::vector<std::string>& tokens, const std::string& originalCmd) -> std::string {
        if (tokens.empty()) {
            return originalCmd;
        }
        std::string mainCmd = tokens[0];
        if (aliasMap.count(mainCmd)) {
            return aliasMap.at(mainCmd);
        }
        return mainCmd;
    };

    // ============================================================
    // 波形設定ハンドラ（W1/W2）
    // ============================================================
    // W1 または W2 の周波数、振幅、位相を設定または照会
    auto waveformHandler = [&](bool isW1, const auto& tokens, const std::string& arg, float val) -> bool {
        if (tokens.size() < 2) {
            return false;
        }

        // チャンネルの参照を取得
        auto& ch = isW1 ? pCfg->awg.ch[0] : pCfg->awg.ch[1];

        std::string subCmd = tokens[1];
        bool isQuery = (subCmd.back() == '?');
        if (isQuery) {
            subCmd.pop_back();  // 末尾の '?' を削除
        }

        // ===== 位相（phase）の設定・照会 =====
        if (subCmd == "phase") {
            if (isQuery) {
                std::cout << ch.phase << std::endl;
            } else {
                ch.phase = val;
                awgUpdateRequired = true;
            }
            return true;
        }
        // ===== 周波数（freq/frequency）の設定・照会 =====
        else if (subCmd == "freq" || subCmd == "frequency") {
            if (isQuery) {
                // min/max/current のいずれかを照会
                if (arg == "min") {
                    std::cout << LiaConfigConst::LOW_LIMIT_FREQ << std::endl;
                } else if (arg == "max") {
                    std::cout << LiaConfigConst::HIGH_LIMIT_FREQ << std::endl;
                } else {
                    std::cout << ch.freq << std::endl;
                }
            } else if (val >= LiaConfigConst::LOW_LIMIT_FREQ && val <= LiaConfigConst::HIGH_LIMIT_FREQ) {
                ch.freq = val;
                awgUpdateRequired = true;
            } else {
                return false;  // 周波数範囲外
            }
            return true;
        }
        // ===== 振幅（volt/amplitude）の設定・照会 =====
        else if (subCmd == "volt" || subCmd == "voltage" || subCmd == "amp" || subCmd == "amplitude") {
            if (isQuery) {
                // min/max/current のいずれかを照会
                if (arg == "min") {
                    std::cout << pCfg->awg.AWG_AMP_MIN << std::endl;
                } else if (arg == "max") {
                    std::cout << pCfg->awg.AWG_AMP_MAX << std::endl;
                } else {
                    std::cout << ch.amp << std::endl;
                }
            } else if (val >= pCfg->awg.AWG_AMP_MIN && val <= pCfg->awg.AWG_AMP_MAX) {
                ch.amp = val;
                awgUpdateRequired = true;
            } else {
                return false;  // 振幅範囲外
            }
            return true;
        }

		// === Functionの設定・照会（未実装） ===
        else if (subCmd == "func" || subCmd == "function") {
            if (isQuery) {
				if (ch.func == 1) {
					std::cout << "sine" << std::endl;
				}
				else if (ch.func == 2) {
					std::cout << "square" << std::endl;
				}
				else if (ch.func == 3) {
					std::cout << "triangle" << std::endl;
				}
				else {
					std::cout << "unknown" << std::endl;
				}
            }
            else {
                if (arg == "sine" || arg == "sin") {
                    ch.func = 1;  // funcSine
                }
                else if (arg == "square" || arg == "sq") {
                    ch.func = 2;  // funcSquare
                }
                else if (arg == "triangle" || arg == "tri") {
                    ch.func = 3;  // funcTriangle
                }
                else {
                    return false;  // 不明な関数タイプ
                }
                awgUpdateRequired = true;
            }
            return true;
        }
        return false;  // 不明なサブコマンド
    };

    // ============================================================
    // コマンドハンドラ登録
    // ============================================================
    // リセット：全設定をデフォルト値に戻す
    commandMap["reset"] = [&](const auto&, const auto&, auto) {
        pCfg->reset();
        lastErrorCmd = "";
        awgUpdateRequired = true;
        return true;
    };

    // 識別情報の照会：接続されたDAQ デバイスの情報を出力
    commandMap["*idn?"] = [&](const auto&, const auto&, auto) {
        if (pCfg->pDaq == nullptr) {
            std::cout << "No DAQ is connected." << std::endl;
        } else {
            std::cout << std::format("{},{},{},{}\n",
                pCfg->pDaq->device.manufacturer, pCfg->pDaq->device.name,
                pCfg->pDaq->device.sn, pCfg->pDaq->device.version);
        }
        return true;
    };

    // エラー照会：最後に発生したエラーコマンドを出力
    commandMap["error?"] = [&](const auto&, const auto&, auto) {
        if (lastErrorCmd.empty()) {
            std::cout << "No error." << std::endl;
        } else {
            std::cout << std::format("Last error: '{}'\n", lastErrorCmd);
            lastErrorCmd.clear();
        }
        return true;
    };

    // 一時停止・再開：データ取得を一時停止または再開
    commandMap["pause"] = commandMap["stop"] = [&](const auto&, const auto&, auto) {
        pCfg->pause.flag = true;
        return true;
    };
    commandMap["run"] = [&](const auto&, const auto&, auto) {
        pCfg->pause.flag = false;
        return true;
    };

    // ヘルプ：コマンド一覧を表示
    commandMap["help?"] = commandMap["?"] = [&](const auto&, const auto&, auto) {
        for (const auto& line : helps) {
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

    // データ関連：生データとXY データの照会・保存
    commandMap["data"] = [&](const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) {
            return false;
        }
        const std::string& subCmd = tokens[1];

        // === 生データの処理 ===
        if (subCmd == "raw") {
            if (tokens.size() > 2) {
                if (tokens[2] == "save") {
                    // 生データをファイルに保存
                    return arg.empty() ? pCfg->saveRawData() : pCfg->saveRawData(arg.c_str());
                } else if (tokens[2] == "size?") {
                    // 生データバッファのサイズを表示
                    std::cout << pCfg->raw.waveform[0].size() << std::endl;
                    return true;
                }
            }
        }
        // === 全生データの照会 ===
        else if (subCmd == "raw?") {
            for (int i = 0; i < static_cast<int>(pCfg->raw.waveform[0].size()); ++i) {
                if (!pCfg->isCh2Enabled) {
                    std::cout << std::format("{:e},{:e}\n", LiaConfigConst::RAW_DT * i, pCfg->raw.waveform[0][i]);
                } else {
                    std::cout << std::format("{:e},{:e},{:e}\n", LiaConfigConst::RAW_DT * i, pCfg->raw.waveform[0][i], pCfg->raw.waveform[1][i]);
                }
            }
            return true;
        }
        if (subCmd == "fft") {
            if (tokens.size() > 2) {
                if (tokens[2] == "save") {
                    // 生データをファイルに保存
                    pCfg->raw.calculateFFT(pCfg->isCh2Enabled, pCfg->awg.ch[0].freq, LiaConfigConst::RAW_DT);  // FFT を計算してから処理
                    return arg.empty() ? pCfg->saveFftData() : pCfg->saveFftData(arg.c_str());
                }
                else if (tokens[2] == "size?") {
                    // 生データバッファのサイズを表示
                    std::cout << pCfg->raw.freqs.size() << std::endl;
                    return true;
                }
            }
        }
        // === FFTデータの照会 ===
        else if (subCmd == "fft?") {
            pCfg->raw.calculateFFT(pCfg->isCh2Enabled, pCfg->awg.ch[0].freq, LiaConfigConst::RAW_DT);  // FFT を計算してから処理
            for (int i = 0; i < static_cast<int>(pCfg->raw.freqs.size()); ++i) {
                if (!pCfg->isCh2Enabled) {
                    std::cout << std::format("{:e},{:e}\n", pCfg->raw.freqs[i], pCfg->raw.fftAbs[0][i]);
                }
                else {
                    std::cout << std::format("{:e},{:e},{:e}\n", pCfg->raw.freqs[i], pCfg->raw.fftAbs[0][i], pCfg->raw.fftAbs[1][i]);
                }
            }
            return true;
        }
        // === 時間と XY データの照会 ===
        else if (subCmd == "txy?") {
            size_t size = pCfg->ringBuffer.size;
            if (val > 0) {
                // 指定秒数のデータサイズを計算（C4244 対策：double で計算）
                size = static_cast<size_t>(static_cast<double>(val) / static_cast<double>(LiaConfigConst::MEASUREMENT_DT));
                if (size > pCfg->ringBuffer.size) {
                    size = pCfg->ringBuffer.size;
                }
            }

            // リングバッファのインデックスを計算
            size_t idx = pCfg->ringBuffer.writeIdx - size;
            if (idx < 0) {
                if (pCfg->ringBuffer.nofm <= static_cast<size_t>(LiaConfigConst::MEASUREMENT_SIZE)) {
                    idx = 0;
                    size = pCfg->ringBuffer.writeIdx;
                } else {
                    idx += static_cast<size_t>(LiaConfigConst::MEASUREMENT_SIZE);
                }
            }

            // データ数を出力してから各データを出力
            std::cout << size << std::endl;
            for (size_t i = 0; i < size; ++i) {
                if (!pCfg->isCh2Enabled) {
                    std::cout << std::format("{:e},{:e},{:e}\n",
                        pCfg->ringBuffer.times[idx],
                        pCfg->ringBuffer.ch[0].x[idx],
                        pCfg->ringBuffer.ch[0].y[idx]);
                } else {
                    std::cout << std::format("{:e},{:e},{:e},{:e},{:e}\n",
                        pCfg->ringBuffer.times[idx],
                        pCfg->ringBuffer.ch[0].x[idx],
                        pCfg->ringBuffer.ch[0].y[idx],
                        pCfg->ringBuffer.ch[1].x[idx],
                        pCfg->ringBuffer.ch[1].y[idx]);
                }
                idx = (idx + 1) % static_cast<int>(LiaConfigConst::MEASUREMENT_SIZE);
            }
            return true;
        }
        // === 最新 XY データの照会 ===
        else if (subCmd == "xy?") {
            const size_t idx = pCfg->ringBuffer.latestIdx;
            std::cout << std::format("{:e},{:e}",
                pCfg->ringBuffer.ch[0].x[idx],
                pCfg->ringBuffer.ch[0].y[idx]);
            if (pCfg->isCh2Enabled) {
                std::cout << std::format(",{:e},{:e}",
                    pCfg->ringBuffer.ch[1].x[idx],
                    pCfg->ringBuffer.ch[1].y[idx]);
            }
            std::cout << std::endl;
            return true;
        }

        return false;
    };

    // === 表示設定：グラフ表示範囲の設定 ===
    commandMap["display"] = [&](const auto& tokens, const auto&, float val) {
        if (tokens.size() < 3) {
            return false;
        }

        // XY 表示限界
        if (tokens[1] == "xy" && tokens[2] == "limit") {
            if (val >= 0.01 && val <= pCfg->scope.ch[0].range * 1.2) {
                pCfg->plot.limit = val;
                return true;
            }
        }
        // 生データ表示限界
        else if (tokens[1] == "raw" && tokens[2] == "limit") {
            if (val >= 0.1 && val <= pCfg->scope.ch[0].range * 1.2) {
                pCfg->plot.rawLimit = val;
                return true;
            }
        }

        return false;
    };

    // === CH2 表示設定の制御 ===
    commandMap[":chan2:disp"] = commandMap["chan2:disp"] = [&](const auto&, const std::string& arg, auto) {
        if (arg == "on") {
            pCfg->isCh2Enabled = true;
            return true;
        }
        if (arg == "off") {
            pCfg->isCh2Enabled = false;
            return true;
        }
        return false;
    };
    // CH2 表示状態の照会
    commandMap[":chan2:disp?"] = commandMap["chan2:disp?"] = [&](const auto&, const auto&, auto) {
        std::cout << (pCfg->isCh2Enabled ? "on" : "off") << std::endl;
        return true;
    };

    // === ACFM 表示設定の制御 ===
    commandMap[":acfm:disp"] = commandMap["acfm:disp"] = [&](const auto&, const std::string& arg, auto) {
        if (arg == "on") {
            pCfg->window.acfmWindow = true;
            return true;
        }
        if (arg == "off") {
            pCfg->window.acfmWindow = false;
            return true;
        }
        return false;
    };
    // ACFM 表示状態の照会
    commandMap[":acfm:disp?"] = commandMap["acfm:disp?"] = [&](const auto&, const auto&, auto) {
        std::cout << (pCfg->window.acfmWindow ? "on" : "off") << std::endl;
        return true;
    };

    // === 波形設定（W1, W2）：std::bind で waveformHandler に委譲 ===
    commandMap["w1"] = std::bind(waveformHandler, true, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    commandMap["w2"] = std::bind(waveformHandler, false, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // === ポスト処理（計算）設定：オフセット、フィルタ ===
    commandMap["calculate"] = [&](const auto& tokens, const std::string& arg, float val) {
        if (tokens.size() < 2) {
            return false;
        }

        // オフセット制御
        if (tokens[1] == "offset") {
            if (tokens.size() > 2) {
                // 自動オフセット設定
                if (tokens[2] == "auto") {
                    if (tokens.size() > 3 && tokens[3] == "once") {
                        pCfg->flagAutoOffset = true;
                        return true;
                    }
                }
                // オフセット自動計算の有効/無効設定
                else if (tokens[2] == "state") {
                    if (arg == "on") {
                        pCfg->flagAutoOffset = true;
                        return true;
                    }
                    if (arg == "off") {
                        // オフの場合はオフセット値もリセット
                        pCfg->post.offset[0].x = 0;
                        pCfg->post.offset[0].y = 0;
                        pCfg->post.offset[1].x = 0;
                        pCfg->post.offset[1].y = 0;
                        pCfg->flagAutoOffset = false;
                        return true;
                    }
                    if (arg == "state?") {
                        std::cout << (pCfg->flagAutoOffset ? "on" : "off") << std::endl;
                        return true;
                    }
                }
            }
        }
        // === ハイパスフィルタ周波数の設定・照会 ===
        else if (tokens[1] == "hpf" && tokens.size() > 2) {
            std::string subCmd = tokens[2];
            bool isQuery = (subCmd.back() == '?');
            if (isQuery) {
                subCmd.pop_back();  // 末尾の '?' を削除
            }

            if (subCmd == "freq" || subCmd == "frequency") {
                if (isQuery) {
                    std::cout << pCfg->post.hpFreq << std::endl;
                } else if (val >= 0.0 && val <= 50.0) {
                    pCfg->post.hpFreq = val;
                } else {
                    return false;  // 周波数範囲外
                }
                return true;
            }
        }

        return false;
    };

    // === ポスト処理オフセット位相（Ch1・Ch2 個別設定） ===
    commandMap[":calc1:offset:phase"] = commandMap["calc1:offset:phase"] = [&](const auto&, const auto&, float val) {
        pCfg->post.offset[0].phase = val;
        return true;
    };
    commandMap[":calc2:offset:phase"] = commandMap["calc2:offset:phase"] = [&](const auto&, const auto&, float val) {
        pCfg->post.offset[1].phase = val;
        return true;
    };
    // 位相照会
    commandMap[":calc1:offset:phase?"] = commandMap["calc1:offset:phase?"] = [&](const auto&, const auto&, auto) {
        std::cout << pCfg->post.offset[0].phase << std::endl;
        return true;
    };
    commandMap[":calc2:offset:phase?"] = commandMap["calc2:offset:phase?"] = [&](const auto&, const auto&, auto) {
        std::cout << pCfg->post.offset[1].phase << std::endl;
        return true;
    };

    // ============================================================
    // メインループ：コマンド入力処理
    // ============================================================
    while (!st.stop_requested()) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            // 入力が EOF または 読み取り不能になったら終了
            break;
        }
        
        // 元の入力行を保存（エラー出力用）
        std::string original_line = line;

        // コマンドを小文字に統一
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        // 入力行を "コマンド部分" と "引数部分" に分割
        std::string commandPart, argumentPart;
        float value = 0;
        std::istringstream iss(line);
        iss >> commandPart;

        // ===== 終了コマンドの優先処理 =====
        if (commandPart == "end" || commandPart == "exit" || commandPart == "quit" || commandPart == "close") {
            break;
        }

        // 残りのテキストから引数を取得
        iss >> argumentPart;

        // 第二引数を数値に変換（失敗時は 0 を設定）
        try {
            value = std::stof(argumentPart);
        } catch (const std::exception&) {
            value = 0;  // 数値引数がない場合
        }

        // コマンドをコロン（:）で分割
        auto tokens = split(commandPart, ':');

        // ===== コマンドのディスパッチ =====
        bool commandOk = false;

        // 1. 完全一致するコマンド（例：chan2:disp, calc1:offset:phase）を検索
        if (auto it_full = commandMap.find(commandPart); it_full != commandMap.end()) {
            commandOk = it_full->second(tokens, argumentPart, value);
        }
        // 2. 正規化されたメインコマンド（例：reset, data, display）を検索
        else if (!tokens.empty()) {
            const std::string mainCommand = getNormalizedMainCommand(tokens, commandPart);
            if (auto it = commandMap.find(mainCommand); it != commandMap.end()) {
                commandOk = it->second(tokens, argumentPart, value);
            }
        }

        // ===== エラー処理 =====
        if (!commandOk) {
            lastErrorCmd = original_line;
            if (commandPart.find('?') != std::string::npos) {
                // クエリコマンド（? を含む）が失敗した場合はエラー出力してクリア
                std::cout << std::format("Error: '{}'\n", lastErrorCmd);
                lastErrorCmd.clear();
            }
            // 設定コマンド（? なし）が失敗した場合はエラー履歴に保存
        }

        // ===== AWG 更新 =====
        // 波形設定が変更された場合は DAQ デバイスに反映
        if (awgUpdateRequired && pCfg->pDaq != nullptr) {
            pCfg->pDaq->awg.start(
                pCfg->awg.ch[0].freq, pCfg->awg.ch[0].amp, pCfg->awg.ch[0].phase, pCfg->awg.ch[0].func,
                pCfg->awg.ch[1].freq, pCfg->awg.ch[1].amp, pCfg->awg.ch[1].phase, pCfg->awg.ch[1].func
            );
            awgUpdateRequired = false;
        }


        // 出力バッファをフラッシュ
        std::cout << std::flush;
    }

    pCfg->statusPipe = false;
}

void test_pipe() {
    std::cout << "--- Starting Fixed Pipe Test ---" << std::endl;

    // 1. 準備
    LiaConfig cfg;
    std::stringstream testInput;

    // 標準入力を切り替え
    auto* oldCinBuffer = std::cin.rdbuf(testInput.rdbuf());

    // 2. stop_source を作成してスレッド開始
    // jthread を使うと stop_token が自動的に第1引数に渡されます
    std::stop_source sw;
    std::jthread pipeThread(pipe, sw.get_token(), &cfg);

    // pipe が起動するのを少し待つ
    while (!cfg.statusPipe) {
        std::this_thread::yield();
    }

    // 3. コマンド送信用ヘルパー
    auto sendCommand = [&](const std::string& cmd) {
        testInput << cmd << "\n";
        // 文字列ストリームに書き込んだことを認識させるために少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };

    // --- テストケース実行 ---
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

    // 4. 終了処理
    std::cout << "[Test] Sending exit and requesting stop..." << std::endl;

    // まず exit コマンドを送る
    sendCommand("exit");

    // 重要：pipe.h の continue ループを破るために stop_token をセットする
    sw.request_stop();

    // jthread なのでスコープを抜ければ自動 join されますが、
    // 明示的に待つことで「止まる」現象を確認できます
    if (pipeThread.joinable()) {
        pipeThread.join();
        std::cout << "[Test] Thread joined successfully!" << std::endl;
    }

    // 標準入力を元に戻す
    std::cin.rdbuf(oldCinBuffer);
    std::cout << "--- Test Passed ---" << std::endl;
}