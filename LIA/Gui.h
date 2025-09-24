#pragma once

#pragma	comment(lib, "opengl32.lib")
#pragma	comment(lib, "GUI/lib/glew32s.lib")
#pragma	comment(lib, "GUI/lib/glfw3_mt.lib")

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <format>
#include <numbers> // For std::numbers::pi

#include "Settings.h"
#include "ImGuiWindowBase.h"
#include "ControlWindow.h"
#include "PlotWindow.h"
#include "Wave.hpp"

void glfw_error_callback(int error, const char* description)
{
    std::cerr << std::format("GLFW Error {}: {}", error, description) << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void window_size_callback(GLFWwindow* window, int widtt, int height)
{
    //std::cout << std::format("Window w:{}, h:{}", widtt, height) << std::endl;
}


class Gui
{
private:
    const char* glsl_version = "#version 460";
    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    int monitorWidth = 0;
    int monitorHeight = 0;
    Wave* waves[9];
public:
	bool initialized = true;
    ControlWindow* controlWindow = nullptr;
    RawPlotWindow* rawPlotWindow = nullptr;
    TimeChartWindow* timeChartWindow = nullptr;
    DeltaTimeChartWindow* deltaTimeChartWindow = nullptr;
    XYPlotWindow* xyPlotWindow = nullptr;
    ACFMPlotWindow* acfmPlotWindow = nullptr;
    Settings* pSettings = nullptr;
    Gui(Settings* pSettings)
    {
        this->pSettings = pSettings;
        if (!this->initGLFW()) { this->initialized = false; return; }
        if (!this->initImGui()) { this->initialized = false; return; }
        this->initBeep();
        this->controlWindow = new ControlWindow(this->window, pSettings);
        this->rawPlotWindow = new RawPlotWindow(this->window, pSettings);
        this->timeChartWindow = new TimeChartWindow(this->window, pSettings);
        this->deltaTimeChartWindow = new DeltaTimeChartWindow(this->window, pSettings);
        this->xyPlotWindow = new XYPlotWindow(this->window, pSettings);
        this->acfmPlotWindow = new ACFMPlotWindow(this->window, pSettings);
    }
	~Gui()
    {
        glfwGetWindowSize(window, &(pSettings->windowWidth), &(pSettings->windowHeight));
        glfwGetWindowPos(window, &(pSettings->windowPosX), &(pSettings->windowPosY));
        ImPlot::DestroyContext();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }
    bool initGLFW();
    bool initImGui();
    void initBeep();
    void deleteBeep();
    void beep();
	void clear(void) { glClear(GL_COLOR_BUFFER_BIT); };
    bool windowShouldClose(void) { return glfwWindowShouldClose(this->window); };
	void pollEvents(void)
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame(); 
	};
    void rendering(void)
    {
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(
            this->clear_color.x * this->clear_color.w,
            this->clear_color.y * this->clear_color.w,
            this->clear_color.z * this->clear_color.w,
            this->clear_color.w
        );
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    };
    void swapBuffers(void){ glfwSwapBuffers(this->window); };
    void show()
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x4D000000);
        //this->acfmPlotWindow->show();
        this->xyPlotWindow->show();
        this->rawPlotWindow->show();
        this->timeChartWindow->show();
        this->deltaTimeChartWindow->show();
        this->controlWindow->show();
        ImGui::PopStyleColor();
    };
};

inline bool Gui::initGLFW()
{
    glfwSetErrorCallback(glfw_error_callback);
    /* Initialize the library */
    if (!glfwInit())
    {
        this->initialized = false;
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_POSITION_X, pSettings->windowPosX);
    glfwWindowHint(GLFW_POSITION_Y, pSettings->windowPosY);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    int xpos, ypos;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &monitorWidth, &monitorHeight);
    //std::cout << std::format("Monitor w:{}, h:{}", monitorWidth, monitorHeight) << std::endl;

    pSettings->monitorScale = ImGui_ImplGlfw_GetContentScaleForMonitor(monitor); // Valid on GLFW 3.3+ only

    if (monitorWidth == 2880 && monitorHeight == 1824)
    {
        this->window = glfwCreateWindow(
            monitorWidth, 1920,
            "Lock-in amplifier", glfwGetPrimaryMonitor(), NULL);
    }
    else{
        this->window = glfwCreateWindow(
            (int)(pSettings->windowWidth * pSettings->monitorScale),
            (int)(pSettings->windowHeight * pSettings->monitorScale),
            "Lock-in amplifier", NULL, NULL
        );
    }

    if (!this->window)
    {
        this->initialized = false;
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
		return false;
    }

    if (!glewInit())
    {
        this->initialized = false;
        std::cerr << "Failed to initialize GLEW." << std::endl;
		return false;
    }
    glfwSetKeyCallback(this->window, key_callback);
    glfwSetWindowSizeCallback(this->window, window_size_callback);

    /* Make the window's context current */
    glfwMakeContextCurrent(this->window);
    return true;
}

inline bool Gui::initImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.FontGlobalScale = 1.f; // 全体のフォントスケールを変更
    ImFont* myFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Lucon.ttf", 32.0f);
    ImGui::PushFont(myFont);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(pSettings->monitorScale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = pSettings->monitorScale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(this->glsl_version);

    ImPlot::CreateContext();
    return true;
}

inline void Gui::initBeep()
{
    for (int i = 0; i < 9; i++)
    {
        waves[i] = new Wave(4800, 1);
    }
    waves[0]->makeSin(100);
    waves[1]->makeSin(261.626);
    waves[2]->makeSin(293.665);
    waves[3]->makeSin(329.628);
    waves[4]->makeSin(349.228);
    waves[5]->makeSin(391.995);
    waves[6]->makeSin(440.000);
    waves[7]->makeSin(493.883);
    waves[8]->makeSin(523.251);
}

inline void Gui::deleteBeep()
{
    for (int i = 0; i < 9; i++)
    {
        delete waves[i];
    }
}

inline void Gui::beep()
{
    if (pSettings->flagBeep)
    {
        int idx = pSettings->idx;
        double amplitude = pow(pSettings->x1s[idx] * pSettings->x1s[idx] + pSettings->y1s[idx] * pSettings->y1s[idx], 0.5);
        double phase = atan2(pSettings->y1s[idx], pSettings->x1s[idx]) / std::numbers::pi * 180;
        if (amplitude > 0.1)
        {
            if (phase < -80)
            {
                waves[0]->stop();
                waves[1]->play();
                waves[2]->stop();
                waves[3]->stop();
                waves[4]->stop();
                waves[5]->stop();
                waves[6]->stop();
                waves[7]->stop();
                waves[8]->stop();
            }
            else if (-80 <= phase && phase < -60) {
                waves[0]->stop();
                waves[1]->stop();
                waves[2]->play();
                waves[3]->stop();
                waves[4]->stop();
                waves[5]->stop();
                waves[6]->stop();
                waves[7]->stop();
                waves[8]->stop();
            }
            else if (-60 <= phase && phase < -55) {
                waves[0]->stop();
                waves[1]->stop();
                waves[2]->stop();
                waves[3]->stop();
                waves[4]->play();
                waves[5]->stop();
                waves[6]->stop();
                waves[7]->stop();
                waves[8]->stop();
            }
            else if (-55 <= phase && phase < 0) {
                waves[0]->stop();
                waves[1]->stop();
                waves[2]->stop();
                waves[3]->play();
                waves[4]->stop();
                waves[5]->stop();
                waves[6]->stop();
                waves[7]->stop();
                waves[8]->stop();
            }
            else if (0 <= phase && phase < 30) {
                waves[0]->stop();
                waves[1]->stop();
                waves[2]->stop();
                waves[3]->stop();
                waves[4]->stop();
                waves[5]->stop();
                waves[6]->stop();
                waves[7]->play();
                waves[8]->stop();
            }
            else if (30 <= phase && phase < 60) {
                waves[0]->stop();
                waves[1]->stop();
                waves[2]->stop();
                waves[3]->stop();
                waves[4]->stop();
                waves[5]->play();
                waves[6]->stop();
                waves[7]->stop();
                waves[8]->stop();
            }
            else if (60 <= phase) {
                waves[0]->stop();
                waves[1]->stop();
                waves[2]->stop();
                waves[3]->stop();
                waves[4]->stop();
                waves[5]->stop();
                waves[6]->play();
                waves[7]->stop();
                waves[8]->stop();
            }
            else {
                for (int i = 0; i < 9; i++) waves[i]->stop();
            }
        }
        else {
            for (int i = 0; i < 9; i++) waves[i]->stop();
        }
    }
    else {
        for (int i = 0; i < 9; i++) waves[i]->stop();
    }
}
