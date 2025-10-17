#pragma once

#include "LiaConfig.h"
#include "ControlWindow.h"
#include "PlotWindow.h"
#include "Beep.h"

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
	ImPlotRect timeChartZoomRect;
	ACFMPlotWindow acfmPlotWindow;
	GuiSub (GLFWwindow* window, LiaConfig& liaConfig)
		: liaConfig(liaConfig), controlWindow(window, liaConfig),
		  rawPlotWindow(window, liaConfig),
		  timeChartWindow(window, liaConfig, timeChartZoomRect),
		  timeChartZoomWindow(window, liaConfig, timeChartZoomRect),
		  deltaTimeChartWindow(window, liaConfig),
		  xyPlotWindow(window, liaConfig),
		  acfmPlotWindow(window, liaConfig)
	{
		timeChartZoomRect = ImPlotRect(0, 1, -1, 1);
	}
	void show(void) {
		const float nextItemWidth = 150 * liaConfig.window.monitorScale;
		static int theme = 0;
		if (theme != liaConfig.imgui.theme)
		{
			theme = liaConfig.imgui.theme;
			Gui::setStyle(theme);
		}
		beep.update(liaConfig.plot.beep, liaConfig.post.offset[0].y, liaConfig.post.offset[0].phase);
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4& col = style.Colors[ImGuiCol_WindowBg];
		col.w = 0.4f; // RGBÇÕÇªÇÃÇ‹Ç‹ÅAalphaÇÃÇ›íuÇ´ä∑Ç¶
		xyPlotWindow.show();
		rawPlotWindow.show();
		timeChartWindow.show();
		deltaTimeChartWindow.show();
		controlWindow.show();
		if (liaConfig.plot.acfm) acfmPlotWindow.show();
		if (liaConfig.flagPause)
		{
			col.w = 1.0f;
			timeChartZoomWindow.show();
		}
	}
};