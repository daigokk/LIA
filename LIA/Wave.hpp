#pragma once
#pragma comment(lib, "winmm.lib")

#include <cmath>
#include <vector>
#include <numbers> // C++20: std::numbers::pi
#include <windows.h>

class Wave {
private:
    static constexpr int BUFFER_COUNT = 2; // 使用するバッファ数

    HWAVEOUT hWaveOut = nullptr;
    WAVEFORMATEX waveFormat = {};
    std::vector<WAVEHDR> waveHeaders;
    std::vector<std::vector<short>> audioBuffers; // 生ポインタ(short**)の代わりにvectorを使用

    int maxSamples; // バッファに格納できる最大サンプル数

    void init(int sampleRate, int durationSec) {
        isReset = true;

        // 1. WAVフォーマットの設定 (16bit モノラル PCM)
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 1;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nSamplesPerSec = sampleRate;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

        // 2. オーディオデバイスのオープン
        waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, reinterpret_cast<DWORD_PTR>(this), 0);

        // 3. バッファのメモリ確保
        maxSamples = sampleRate * durationSec;
        waveHeaders.resize(BUFFER_COUNT);
        audioBuffers.resize(BUFFER_COUNT, std::vector<short>(maxSamples));

        for (int i = 0; i < BUFFER_COUNT; i++) {
            ZeroMemory(&waveHeaders[i], sizeof(WAVEHDR));
            // vectorの内部配列の先頭ポインタをWinMMに渡す
            waveHeaders[i].lpData = reinterpret_cast<LPSTR>(audioBuffers[i].data());
        }
    }

public:
    bool isReset = true;

    Wave(int sampleRate, int durationSec) {
        init(sampleRate, durationSec);
    }

    Wave(int sampleRate, int durationSec, double frequency) {
        init(sampleRate, durationSec);
        makeSineWave(frequency);
    }

    ~Wave() {
        close();
    }

    void close() {
        if (!hWaveOut) return;

        stop();

        // ヘッダーの準備解除
        for (auto& header : waveHeaders) {
            waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        }

        // デバイスを閉じる (元コードで不足していた処理)
        waveOutClose(hWaveOut);
        hWaveOut = nullptr;
    }

    // 1周期分の正弦波データを作成し、ループ再生の設定を行う
    void makeSineWave(double frequency) {
        if (frequency <= 0.0) return;

        double qStep = (2.0 * std::numbers::pi * frequency) / waveFormat.nSamplesPerSec;
        double q = 0.0;
        int sampleCount = 0;

        // 1周期分の波形を生成
        for (sampleCount = 0; sampleCount < maxSamples; sampleCount++) {
            short amplitude = static_cast<short>(32767.0 * std::sin(q));

            for (int i = 0; i < BUFFER_COUNT; i++) {
                audioBuffers[i][sampleCount] = amplitude;
            }

            q += qStep;

            // 位相が2π（1周期）を超えたら波形生成を終了
            if (q >= 2.0 * std::numbers::pi) {
                sampleCount++; // 現在のサンプルを含めてインクリメント
                break;
            }
        }

        // WinMMにループ再生を指示するヘッダー設定
        for (int i = 0; i < BUFFER_COUNT; i++) {
            waveHeaders[i].dwBufferLength = sampleCount * sizeof(short); // バイト数
            waveHeaders[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
            waveHeaders[i].dwLoops = static_cast<DWORD>((20000.0 * frequency) / waveFormat.nSamplesPerSec);
            waveHeaders[i].dwUser = 0;
        }
    }

    void play() {
        isReset = false;
        for (auto& header : waveHeaders) {
            waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
            waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
        }
    }

    void stop() {
        isReset = true;
        if (hWaveOut) {
            waveOutReset(hWaveOut);
        }
    }
};