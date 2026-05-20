#pragma once

#include "LiaConfig.h"
#include "ControlWindow.h"
#include "PlotWindow.h"
#include "Beep.h"

class GuiSub {
private:
	LiaConfig& cfg;
public:
	Beep9 beep;
	ControlWindow controlWindow;
	RawPlotWindow rawPlotWindow;
	TimeChartWindow timeChartWindow;
	TimeChartZoomWindow timeChartZoomWindow;
	DeltaTimeChartWindow deltaTimeChartWindow;
	XYPlotWindow xyPlotWindow;
	ACFMPlotWindow acfmPlotWindow;
	ACFMVhVvPlotWindow acfmVhVvPlotWindow;
	GuiSub(GLFWwindow* window, LiaConfig& cfg)
		: cfg(cfg), controlWindow(window, cfg),
		rawPlotWindow(window, cfg),
		timeChartWindow(window, cfg),
		timeChartZoomWindow(window, cfg),
		deltaTimeChartWindow(window, cfg),
		xyPlotWindow(window, cfg),
		acfmPlotWindow(window, cfg),
		acfmVhVvPlotWindow(window, cfg)
	{

	}
	void ShowMainMenuBar() {
		cfg.window.imGuiCondFlag &= ~ImGuiCond_Always;
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Windows")) {
				ImGui::MenuItem("Control panel", NULL, &cfg.window.controlWindow);
				ImGui::Separator();
				ImGui::MenuItem("Raw", NULL, &cfg.window.rawWindow);
				ImGui::MenuItem("XY", NULL, &cfg.window.xyWindow);
				ImGui::MenuItem("Time", NULL, &cfg.window.timeWindow);
				ImGui::MenuItem("Delta time", NULL, &cfg.window.deltaTimeWindow);
				ImGui::Separator();
				ImGui::MenuItem("ACFM", NULL, &cfg.window.acfmWindow);
				ImGui::Separator();
				if (ImGui::MenuItem("Default size pos")) {
					cfg.window.imGuiCondFlag |= ImGuiCond_Always;
				}
				static bool isLock = (cfg.window.imGuiWindowFlag & (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) != 0;
				if (ImGui::MenuItem("Lock", NULL, &isLock)) {
					cfg.window.imGuiWindowFlag = isLock ? ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize : 0;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Views")) {
				const char* themes[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue", "NeonGreen", "NeonRed", "Eva" };
				ImGui::SetNextItemWidth(200 * cfg.window.monitorScale);
				ImGui::ListBox("Theme", &cfg.window.theme, themes, IM_ARRAYSIZE(themes), IM_ARRAYSIZE(themes));
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Tools")) {
				ImGui::MenuItem("Ch2", NULL, &cfg.scope.ch[1].enable);
				ImGui::MenuItem("Beep", NULL, &cfg.plot.beep);
				ImGui::MenuItem("Surface mode", NULL, &cfg.plot.surfaceMode);
				if (ImGui::MenuItem("Reset")) {
					cfg.reset();
				}
				ImGui::EndMenu();
			}
			//if (ImGui::BeginMenu("Help")) {
			//	ImGui::MenuItem("About", NULL, &liaConfig.windowCfg.aboutWindow);
			//	ImGui::EndMenu();
			//}
			ImGui::EndMainMenuBar();
		}
	}
	void show(void) {
		const float nextItemWidth = 150 * cfg.window.monitorScale;
		static int theme = 0;
		if (theme != cfg.window.theme)
		{
			theme = cfg.window.theme;
			Gui::SetTheme(static_cast<GuiTheme>(theme));
		}
		beep.update(cfg.plot.beep, cfg.ringBuffer.ch[0].x[cfg.ringBuffer.latestIdx], cfg.ringBuffer.ch[0].y[cfg.ringBuffer.latestIdx]);
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4& col = style.Colors[ImGuiCol_WindowBg];
		col.w = 0.4f; // RGBはそのまま、alphaのみ置き換え
		ShowMainMenuBar();

		if (cfg.window.deltaTimeWindow) { deltaTimeChartWindow.show(); }
		if (cfg.window.controlWindow) { controlWindow.show(); }
		if (cfg.window.xyWindow) { xyPlotWindow.show(); }
		if (cfg.window.rawWindow) { rawPlotWindow.show(); }
		if (cfg.window.timeWindow) { timeChartWindow.show(); }
		if (cfg.window.acfmWindow) { acfmPlotWindow.show(); }
		if (cfg.pause.flag) {
			col.w = 1.0f;
			timeChartZoomWindow.show();
			if (cfg.window.acfmWindow) acfmVhVvPlotWindow.show();
		}
	}
};
