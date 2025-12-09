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
	void show(void) {
		const float nextItemWidth = 150 * liaConfig.window.monitorScale;
		static int theme = 0;
		if (theme != liaConfig.imgui.theme)
		{
			theme = liaConfig.imgui.theme;
			Gui::setStyle(theme);
		}
		beep.update(liaConfig.plot.beep, liaConfig.ringBuffer.ch[0].x[liaConfig.ringBuffer.idx], liaConfig.ringBuffer.ch[0].y[liaConfig.ringBuffer.idx]);
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4& col = style.Colors[ImGuiCol_WindowBg];
		col.w = 0.4f; // RGBÇÕÇªÇÃÇ‹Ç‹ÅAalphaÇÃÇ›íuÇ´ä∑Ç¶
		xyPlotWindow.show();
		rawPlotWindow.show();
		timeChartWindow.show();
		deltaTimeChartWindow.show();
		controlWindow.show();
		if (liaConfig.plot.acfm) {
			acfmPlotWindow.show();
		}
		if (liaConfig.pauseCfg.flag)
		{
			col.w = 1.0f;
			timeChartZoomWindow.show();
			if (liaConfig.plot.acfm) acfmVhVvPlotWindow.show();
		}
	}
};