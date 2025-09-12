#pragma once
#pragma	comment(lib, "DAQ/lib/dwf.lib")
#include "dwf.h"
#include <Windows.h>
#include <vector>
#define DAQ_TYPE "Digilent"
#define DAQ_MAX_INSTRUMENTS 10
#define DAQMX_FILENAME "waveforms.csv"
#define Wait(ts) Sleep((int)(1000*ts))

typedef struct
{
	int		ch;
	int		numCh;
	int		triggerDigCh;
	double	range;
	double	rate;
	double	waitAd;
	int	numSampsPerChan;
} settings_t;

class daq_dwf
{
private:
	HDWF hdwf;
	//char	dev[256];
	void errChk(BOOL ret);
	int adTaskHandle = 0, daTaskHandle = 0;
	double _fgOffset = 0;
	double _fgFreq = 10, _fgAmp = 1, _fgPhase = 0;
	double _fgAmp2 = 0, _fgPhase2 = 0;
public:
	char	serialNo[32] = { "" };
	char	type[32];
	//int terminalConfig = 0;
	settings_t adSettings = { 0,1,-1,10.0,1e6,2,100 };
	settings_t daSettings = { 0,1,-1,10.0,1e6,2,100 };
	daq_dwf();
	daq_dwf(const char* serial);
	~daq_dwf();
	settings_t setSettings(const int ch, const int numCh, const int numSampsPerChan, const double range, const double rate, const double waitAd, const int triggerDigCh);
	void init(int idxDevice);
	double ad(const int ch, const int range);
	double ad_init(const settings_t _settings);
	double ad_init();
	void ad_start();
	void ad_get(const size_t length, double ch1[]);
	void ad_get(const size_t length, double ch1[], double ch2[]);
	void adWaveforms(const settings_t _settings, double* data);
	void adWaveforms(double* data);
	void adWaveforms(const char* filepath);
	void adWaveforms();
	void adWaveforms(const settings_t _settings, double ch1[], double ch2[]);
	void da(const int ch, const double volt);
	void daWaveform(const double amplitude, const double freq, const std::vector<double>& waveform);
	void daOff();
	void fg(const double amplitude, const double freq, const double phase, const double amplitude2, const double phase2);
	void fgOn();
	void fgOff();
	void fgFreq(const double freq);
	double fgFreq();
	void fgVpp(const double vpp);
	double fgVpp();
	void fgVrms(const double vrms);
	double fgVrms();
	void fgPhase(const double phase);
	double fgPhase();
	void powerSupply(const double volts);
	void psd(const double freq, const double rate, const int num, const double wave[], double* x, double* y);
};
