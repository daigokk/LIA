#include "Gui.h"
#include <iostream>
#include <format>

// ---------------------------------------------------------
// 静的メンバーの初期化
// ---------------------------------------------------------
GLFWwindow* Gui::window_ = nullptr;
float Gui::monitorScale = 1.0f;
int Gui::monitorWidth = 0;
int Gui::monitorHeight = 0;
bool Gui::isSurfacePro7 = false;
int Gui::windowFlag = 4; // ImGuiCond_FirstUseEver

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
bool Gui::Initialize(const GuiConfig& config) {
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
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &monitorWidth, &monitorHeight);

    // モニターのスケールを取得 (GLFW 3.3+)
    monitorScale = ImGui_ImplGlfw_GetContentScaleForMonitor(monitor);

    // 特定の解像度（Surface Pro 7）に対する特殊処理
    isSurfacePro7 = (monitorWidth == 2880 && monitorHeight == 1824);
    if (isSurfacePro7) {
        window_ = glfwCreateWindow(monitorWidth, 1920, config.title.c_str(), monitor, NULL);
    }
    else {
        int scaledWidth = static_cast<int>(config.width * monitorScale);
        int scaledHeight = static_cast<int>(config.height * monitorScale);
        window_ = glfwCreateWindow(scaledWidth, scaledHeight, config.title.c_str(), NULL, NULL);

        if (window_) {
            glfwSetWindowPos(window_, config.posX, config.posY);
        }
    }

    if (!window_) {
        std::cerr << "[Error] Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(config.enableVsync ? 1 : 0);
    glfwSetKeyCallback(window_, KeyCallback);

    // ImGui & ImPlot 初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.0f;

    // フォントの読み込み
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(config.fontPath.c_str(), config.fontSize);
    if (myFont) {
        ImGui::PushFont(myFont);
    }
    else {
        std::cerr << "[Warning] Failed to load font: " << config.fontPath << "\n";
    }

    // スタイルのスケーリング設定
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(monitorScale);
    style.FontScaleDpi = monitorScale;

    // バックエンド初期化
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void Gui::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Gui::BeginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::EndFrame() {
    if (!window_) return;

    ImGui::Render();

    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
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