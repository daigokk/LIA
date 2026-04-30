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
#include <iostream>

class Gui {
public:
    static void Initialize(
        const char title[], 
        const int windowPosX, const int windowPosY,
        const int windowWidth, const int windowHeight
    );
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();
    static GLFWwindow* GetWindow() { return window_; }
	static void setStyle(const int theme);
	static void ImGui_SetGrayTheme();
	static void ImGui_SetNeonBlueTheme();
	static void ImGui_SetNeonGreenTheme();
	static void ImGui_SetNeonRedTheme();
	static void ImGui_SetEvaTheme();
    static float monitorScale;
    static bool SurfacePro7;
private:
    static GLFWwindow* window_;
    static int monitorWidth;
    static int monitorHeight;
    static int windowFlag;
};

// Implementations are provided in Gui.cpp
