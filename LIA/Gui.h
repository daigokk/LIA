#pragma once

// リンカ設定（Windows API + OpenGL + GLFW）
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "./GLFW/glfw3_mt.lib")

// ImGui & ImPlot
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>

// GLFW & 標準ライブラリ
#include <GLFW/glfw3.h>
#include <string>

// テーマ定義
enum class GuiTheme {
    Dark,
    Classic,
    Light,
    Gray,
    NeonBlue,
    NeonGreen,
    NeonRed,
    Evangelion
};

// 初期化用の設定構造体
struct GuiConfig {
    std::string title = "My Application";
    int posX = 100;
    int posY = 100;
    int width = 1280;
    int height = 720;
    std::string fontPath = "C:/Windows/Fonts/Lucon.ttf";
    float fontSize = 32.0f;
    bool enableVsync = true;
};

class Gui {
public:
    static bool Initialize(const GuiConfig& config);
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();

    static void SetTheme(GuiTheme theme);
    static GLFWwindow* GetWindow() { return window_; }

    // プロパティ
    static float monitorScale;
    static bool isSurfacePro7;

private:
    static GLFWwindow* window_;
    static int monitorWidth;
    static int monitorHeight;
    static int windowFlag;

    // コールバック関数
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // カスタムテーマ群
    static void SetGrayTheme();
    static void SetNeonBlueTheme();
    static void SetNeonGreenTheme();
    static void SetNeonRedTheme();
    static void SetEvaTheme();
};