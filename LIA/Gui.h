#pragma once

#pragma	comment(lib, "opengl32.lib")
#pragma	comment(lib, "GUI/lib/glfw3.lib")
#pragma	comment(lib, "GUI/lib/glew32s.lib")

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <format>

#include "Settings.h"
#include "ImuGuiWindowBase.h"
#include "ControlWindow.h"
#include "PlotWindow.h"


static void glfw_error_callback(int error, const char* description)
{
    std::cerr << std::format("GLFW Error {}: {}", error, description) << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void window_size_callback(GLFWwindow* window, int widtt, int height)
{
    std::cout << std::format("Window w:{}, h:{}", widtt, height) << std::endl;
}


class Gui
{
public:
	bool initialized = true;
    Gui(Settings* pSettings)
    {
        if (!this->initGLFW()) { this->initialized = false; return; }
        if (!this->initImGui()) { this->initialized = false; return; }
        this->controlWindow = new ControlWindow(this->window, pSettings);
        this->rawPlotWindow = new RawPlotWindow(this->window, pSettings);
        this->measurementPlotWindow = new MeasurementPlotWindow(this->window, pSettings);
        this->xyPlotWindow = new XYPlotWindow(this->window, pSettings);
    }
	~Gui()
    {
        ImPlot::DestroyContext();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }
    bool initGLFW();
    bool initImGui();
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
		this->controlWindow->show();
        this->rawPlotWindow->show();
        this->measurementPlotWindow->show();
		this->xyPlotWindow->show();

	};
    ControlWindow* controlWindow = nullptr;
    RawPlotWindow* rawPlotWindow = nullptr;
    MeasurementPlotWindow* measurementPlotWindow = nullptr;
	XYPlotWindow* xyPlotWindow = nullptr;

private:
    const char* glsl_version = "#version 460";
    GLFWwindow* window = nullptr;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    float monitorScale = 1.0;
    int monitorWidth = 0;
    int monitorHeight = 0;
    int windowWidth = 1500;
    int windowHeight = 1000;
	int windowPosX = 0;
	int windowPosY = 30;
    
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
    glfwWindowHint(GLFW_POSITION_X, this->windowPosX);
    glfwWindowHint(GLFW_POSITION_Y, this->windowPosY);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    int xpos, ypos;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &monitorWidth, &monitorHeight);
    std::cout << std::format("Monitor w:{}, h:{}", monitorWidth, monitorHeight) << std::endl;

    this->monitorScale = ImGui_ImplGlfw_GetContentScaleForMonitor(monitor); // Valid on GLFW 3.3+ only
    this->window = glfwCreateWindow(
        (int)(windowWidth * this->monitorScale),
        (int)(windowHeight * this->monitorScale),
        "Lock-in amplifier", NULL, NULL
    );
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
    style.ScaleAllSizes(this->monitorScale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = this->monitorScale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)



    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(this->glsl_version);

    ImPlot::CreateContext();
    return true;
}
