#pragma once

#include "LiaConfig.h"
#include "ControlWindow.h"
#include "PlotWindow.h"
#include "Beep.h"


static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

class GuiSub {
private:
	LiaConfig& liaConfig;
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
	GuiSub(GLFWwindow* window, LiaConfig& liaConfig)
		: liaConfig(liaConfig), controlWindow(window, liaConfig),
		rawPlotWindow(window, liaConfig),
		timeChartWindow(window, liaConfig),
		timeChartZoomWindow(window, liaConfig),
		deltaTimeChartWindow(window, liaConfig),
		xyPlotWindow(window, liaConfig),
		acfmPlotWindow(window, liaConfig),
		acfmVhVvPlotWindow(window, liaConfig)
	{

	}
	void ShowMainMenuBar() {
		liaConfig.windowCfg.imGuiCondFlag &= ~ImGuiCond_Always;
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Windows")) {
				ImGui::MenuItem("Control panel", NULL, &liaConfig.windowCfg.controlWindow);
				ImGui::Separator();
				ImGui::MenuItem("Raw", NULL, &liaConfig.windowCfg.rawWindow);
				ImGui::MenuItem("XY", NULL, &liaConfig.windowCfg.xyWindow);
				ImGui::MenuItem("Time", NULL, &liaConfig.windowCfg.timeWindow);
				ImGui::MenuItem("Delta time", NULL, &liaConfig.windowCfg.deltaTimeWindow);
				ImGui::Separator();
				ImGui::MenuItem("ACFM", NULL, &liaConfig.windowCfg.acfmWindow);
				ImGui::Separator();
				if (ImGui::MenuItem("Default size pos")) {
					liaConfig.windowCfg.imGuiCondFlag |= ImGuiCond_Always;
				}
				static bool lock = (liaConfig.windowCfg.imGuiWindowFlag & (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) != 0;
				if (ImGui::MenuItem("Lock", NULL, &lock)) {
					liaConfig.windowCfg.imGuiWindowFlag = lock ? ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize : 0;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Theme")) {
				const char* themes[] = { "Dark", "Classic", "Light", "Gray", "NeonBlue", "NeonGreen", "NeonRed", "Eva" };
				ImGui::ListBox("", &liaConfig.windowCfg.theme, themes, IM_ARRAYSIZE(themes), 10);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Settings")) {
				ImGui::MenuItem("Ch2", NULL, &liaConfig.flagCh2);
				ImGui::MenuItem("Beep", NULL, &liaConfig.plotCfg.beep);
				ImGui::MenuItem("Surface mode", NULL, &liaConfig.plotCfg.surfaceMode);
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
		const float nextItemWidth = 150 * liaConfig.windowCfg.monitorScale;
		static int theme = 0;
		if (theme != liaConfig.windowCfg.theme)
		{
			theme = liaConfig.windowCfg.theme;
			Gui::SetTheme(static_cast<GuiTheme>(theme));
		}
		beep.update(liaConfig.plotCfg.beep, liaConfig.ringBuffer.ch[0].x[liaConfig.ringBuffer.latestIdx], liaConfig.ringBuffer.ch[0].y[liaConfig.ringBuffer.latestIdx]);
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4& col = style.Colors[ImGuiCol_WindowBg];
		col.w = 0.4f; // RGBÇÕÇªÇÃÇ‹Ç‹ÅAalphaÇÃÇ›íuÇ´ä∑Ç¶
		ShowMainMenuBar();

		if (liaConfig.windowCfg.deltaTimeWindow) { deltaTimeChartWindow.show(); }
		if (liaConfig.windowCfg.controlWindow) { controlWindow.show(); }
		if (liaConfig.windowCfg.xyWindow) { xyPlotWindow.show(); }
		if (liaConfig.windowCfg.rawWindow) { rawPlotWindow.show(); }
		if (liaConfig.windowCfg.timeWindow) { timeChartWindow.show(); }
		if (liaConfig.windowCfg.acfmWindow) { acfmPlotWindow.show(); }
		if (liaConfig.pauseCfg.flag) {
			col.w = 1.0f;
			timeChartZoomWindow.show();
			if (liaConfig.windowCfg.acfmWindow) acfmVhVvPlotWindow.show();
		}
	}
};