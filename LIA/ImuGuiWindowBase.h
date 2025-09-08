#pragma once

#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>

class ImuGuiWindowBase
{
public:
    ImuGuiWindowBase(GLFWwindow* window, const char name[]) { this->window = window; this->name = name; }
    void show(void) { ImGui::Begin(this->name); ImGui::End(); }
    const char* name = nullptr;
private:
    GLFWwindow* window = nullptr;
    
};
