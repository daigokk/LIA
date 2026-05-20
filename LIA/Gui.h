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
#include <iostream>
#include <format>

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
    std::string fontPaths[2] = { "C:/Windows/Fonts/Lucon.ttf", "C:/Windows/Fonts/Verdana.ttf" };
    ImFont* font_regular = nullptr;
    int fontIdx = 1;
    float fontSize = 32.0f;
    bool enableVsync = true;

    GLFWwindow* pWindow;
    int monitorWidth = 0;
    int monitorHeight = 0;
    int windowFlag = 0;
    float monitorScale;
    bool isSurfacePro7;
};

class Gui {
public:
    static bool Initialize(GuiConfig& config);
    static void Shutdown(GuiConfig& config);
    static void BeginFrame();
    static void EndFrame(GuiConfig& config);
    static void SetTheme(GuiTheme theme);
private:
    // コールバック関数
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // カスタムテーマ群
    static void SetGrayTheme();
    static void SetNeonBlueTheme();
    static void SetNeonGreenTheme();
    static void SetNeonRedTheme();
    static void SetEvaTheme();
};

// ---------------------------------------------------------
// コールバック
// ---------------------------------------------------------
void Gui::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// ---------------------------------------------------------
// 基本機能の実装
// ---------------------------------------------------------
bool Gui::Initialize(GuiConfig& config) {
    if (!glfwInit()) {
        std::cerr << "[Error] Failed to initialize GLFW\n";
        return false;
    }

    // OpenGL コンテキスト設定
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    int xpos, ypos;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &config.monitorWidth, &config.monitorHeight);

    // モニターのスケールを取得 (GLFW 3.3+)
    config.monitorScale = ImGui_ImplGlfw_GetContentScaleForMonitor(monitor);

    // 特定の解像度（Surface Pro 7）に対する特殊処理
    config.isSurfacePro7 = (config.monitorWidth == 2880 && config.monitorHeight == 1824);
    if (config.isSurfacePro7) {
        config.pWindow = glfwCreateWindow(config.monitorWidth, 1920, config.title.c_str(), monitor, NULL);
    }
    else {
        int scaledWidth = static_cast<int>(config.width * config.monitorScale);
        int scaledHeight = static_cast<int>(config.height * config.monitorScale);
        config.pWindow = glfwCreateWindow(scaledWidth, scaledHeight, config.title.c_str(), NULL, NULL);

        if (config.pWindow) {
            glfwSetWindowPos(config.pWindow, config.posX, config.posY);
        }
    }

    if (!config.pWindow) {
        std::cerr << "[Error] Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(config.pWindow);
    glfwSwapInterval(config.enableVsync ? 1 : 0);
    glfwSetKeyCallback(config.pWindow, KeyCallback);

    // ImGui & ImPlot 初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.0f;

    // フォントの読み込み
    config.font_regular = io.Fonts->AddFontFromFileTTF(config.fontPaths[config.fontIdx].c_str());
    if (config.font_regular) {
        ImGui::PushFont(config.font_regular, config.fontSize);
    }
    else {
        std::cerr << "[Warning] Failed to load font: " << config.fontPaths[config.fontIdx] << "\n";
    }

    // スタイルのスケーリング設定
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(config.monitorScale);
    style.FontScaleDpi = config.monitorScale;

    // バックエンド初期化
    ImGui_ImplGlfw_InitForOpenGL(config.pWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void Gui::Shutdown(GuiConfig& config) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    if (config.pWindow) {
        glfwDestroyWindow(config.pWindow);
        config.pWindow = nullptr;
    }
    glfwTerminate();
}

void Gui::BeginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::EndFrame(GuiConfig& config) {
    if (!config.pWindow) return;

    ImGui::Render();

    int width, height;
    glfwGetFramebufferSize(config.pWindow, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(config.pWindow);
}

// ---------------------------------------------------------
// テーマ設定
// ---------------------------------------------------------
void Gui::SetTheme(GuiTheme theme) {
    switch (theme) {
    case GuiTheme::Dark:       ImGui::StyleColorsDark(); break;
    case GuiTheme::Classic:    ImGui::StyleColorsClassic(); break;
    case GuiTheme::Light:      ImGui::StyleColorsLight(); break;
    case GuiTheme::Gray:       SetGrayTheme(); break;
    case GuiTheme::NeonBlue:   SetNeonBlueTheme(); break;
    case GuiTheme::NeonGreen:  SetNeonGreenTheme(); break;
    case GuiTheme::NeonRed:    SetNeonRedTheme(); break;
    case GuiTheme::Evangelion: SetEvaTheme(); break;
    }
}

void Gui::SetGrayTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 0.95f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);

    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
}

void Gui::SetNeonBlueTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // タイトルバーの色設定（青系ネオン）
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.20f, 0.35f, 1.00f);

    // 背景：ダークネイビー
    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.05f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.03f, 0.06f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.08f, 0.15f, 1.00f);

    // フレームとボーダー：ブルーグロー
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.70f, 1.00f, 0.60f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.00f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.70f, 1.00f, 1.00f);

    // ボタン：ネオンブルー
    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.40f, 0.70f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.60f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);

    // スライダーとチェック：エレクトリックブルー
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.90f, 1.00f, 1.00f);

    // タブ：ブルーグラデーション
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);

    // スタイル：シャープで近未来的な印象
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 3.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
}

void Gui::SetNeonGreenTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.40f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.25f, 0.12f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.08f, 0.05f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.03f, 0.10f, 0.06f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.12f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 0.50f, 0.60f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.40f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.00f, 0.60f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.60f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.80f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 0.50f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 1.00f, 0.70f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.50f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.40f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.70f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.90f, 0.45f, 1.00f);

    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 3.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
}

void Gui::SetNeonRedTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_TitleBg] = ImVec4(0.40f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.30f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.03f, 0.03f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(1.00f, 0.20f, 0.20f, 0.60f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.40f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.60f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.50f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);

    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 3.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
}

void Gui::SetEvaTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.0f, 0.4f, 0.6f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.0f, 0.4f, 0.8f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.5f, 0.0f, 0.7f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.3f, 0.0f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.5f, 0.0f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.3f, 0.0f, 0.4f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.0f, 0.7f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.6f, 0.0f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.8f, 0.0f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.6f, 0.0f, 0.8f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8f, 0.0f, 1.0f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.0f, 0.3f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.0f, 0.6f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.6f, 0.0f, 1.00f);

    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 3.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.WindowPadding = ImVec2(8, 8);
}