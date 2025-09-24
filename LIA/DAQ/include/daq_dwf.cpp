#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <windows.h>
#include "daq_dwf.hpp"
#include "csv.hpp"

#define PI acos(-1)

void daq_dwf::errChk(BOOL ret)
{
	static char szError[512] = { 0 };
	if (!ret) {
		FDwfGetLastErrorMsg(szError);
		printf("\a\n--- DWF error -----------------------------------\n");
		printf("%s", szError);
		printf("---------------------------------------------\n");
		system("PAUSE");
		exit(EXIT_FAILURE);
	}
}

daq_dwf::daq_dwf()
{
	int pcDevice, idxDevice;
	this->errChk(FDwfEnum(enumfilterAll, &pcDevice));
	if (pcDevice == 0)
	{
		printf("\a\n--- DWF error -------------------------------\n");
		printf("No device.\n");
		printf("---------------------------------------------\n");
		//system("PAUSE");
		exit(EXIT_FAILURE);
	}
	else if (pcDevice==1)
	{
		idxDevice = 0;
	}
	else
	{
		BOOL pfIsUsed;
		printf("ID	Opened	SN\n");
		for (int i = 0; i < pcDevice; i++)
		{	
			this->errChk(FDwfEnumSN(i, this->serialNo));
			this->errChk(FDwfEnumDeviceIsOpened(i, &pfIsUsed));
			printf("%d	%d	%s\n", i, pfIsUsed, this->serialNo);
		}
		printf("Input ID: ");
		scanf_s("%d", &idxDevice);
	}
	this->init(idxDevice);
}

daq_dwf::daq_dwf(const char *serial)
{
	int pcDevice;
	char szSN[32];
	int flag = 0;

	this->errChk(FDwfEnum(enumfilterAll, &pcDevice));
	
	for (int i = 0; i < pcDevice; i++)
	{
		this->errChk(FDwfEnumSN(0, szSN));
		if (strstr(szSN, serial) != NULL)
		{
			this->init(i);
			flag = 1;
			break;
		}
	}
	
	if (flag == 0)
	{
		printf("\a\n--- DWF error -------------------------------\n");
		printf("'%s' was not found.\n", serial);
		printf("---------------------------------------------\n");
		system("PAUSE");
		exit(EXIT_FAILURE);
	}
}

daq_dwf::~daq_dwf()
{
	if(this->hdwf!=NULL)
		this->errChk(FDwfDeviceClose(this->hdwf));
}

settings_t daq_dwf::setSettings(const int ch, const int numCh, const int numSampsPerChan, const double range, const double rate, const double	waitAd, const int triggerDigCh)
{
	settings_t _settings;
	_settings.ch = ch;
	_settings.numCh = numCh;
	_settings.numSampsPerChan = numSampsPerChan;
	_settings.range = range;
	_settings.rate = rate;
	_settings.triggerDigCh = triggerDigCh;
	_settings.waitAd = waitAd;
	return _settings;
}

void daq_dwf::init(int idxDevice)
{
	this->errChk(FDwfDeviceOpen(idxDevice, &this->hdwf));
	this->errChk(FDwfEnumDeviceName(idxDevice, this->type));
	this->errChk(FDwfEnumSN(idxDevice, this->serialNo));
}

double daq_dwf::ad_init(const settings_t _settings)
{
	// enable channels
	FDwfAnalogInChannelEnableSet(this->hdwf, 0, true);
	if (_settings.numCh == 2)
	{
		FDwfAnalogInChannelEnableSet(this->hdwf, 1, true);
	}
	// set pk2pk input range for all channels
	//double voltsMin, voltsMax, nSteps;
	//FDwfAnalogInChannelRangeInfo(this->hdwf, &voltsMin, &voltsMax, &nSteps);//5 to 50Vpp
	FDwfAnalogInChannelRangeSet(this->hdwf, -1, _settings.range * 2);

	// set sample rate
	double hzMin, hzMax, hz;
	FDwfAnalogInFrequencyInfo(this->hdwf,&hzMin, &hzMax);
	if (_settings.rate < hzMin || hzMax < _settings.rate)
	{
		printf("\a\n--- DWF error -------------------------------\n");
		printf("The rate %e is too larger or smaller than minimam or maximum rate %e, %e Hz of this device.\n", _settings.rate, hzMin, hzMax);
		printf("---------------------------------------------\n");
		system("PAUSE");
		exit(EXIT_FAILURE);
	}
	FDwfAnalogInFrequencySet(this->hdwf, _settings.rate);
	FDwfAnalogInFrequencyGet(this->hdwf, &hz);

	// get the maximum buffer size
	int nSizeMax;
	FDwfAnalogInBufferSizeInfo(this->hdwf, NULL, &nSizeMax);
	if (nSizeMax < _settings.numSampsPerChan)
	{
		printf("\a\n--- DWF error -------------------------------\n");
		printf("The number of samples %d is too larger than a buffer %d of this device.\n", _settings.numSampsPerChan, nSizeMax);
		printf("---------------------------------------------\n");
		system("PAUSE");
		exit(EXIT_FAILURE);
	}
	FDwfAnalogInBufferSizeSet(this->hdwf, _settings.numSampsPerChan);

	// configure trigger
	if (0 <= _settings.triggerDigCh)
	{
		FDwfAnalogInTriggerSourceSet(this->hdwf, trigsrcExternal1 + _settings.triggerDigCh);
		//FDwfAnalogInTriggerPositionSet(this->hdwf, 0);
		FDwfAnalogInTriggerAutoTimeoutSet(this->hdwf, _settings.rate * _settings.numSampsPerChan * 3);
		FDwfAnalogInTriggerConditionSet(this->hdwf, trigcondFallingNegative);
	}
	else
	{
		FDwfAnalogInTriggerSourceSet(this->hdwf, trigsrcAnalogOut1);
		//FDwfAnalogInTriggerSourceSet(this->hdwf, trigsrcDetectorAnalogIn);
		//FDwfAnalogInTriggerChannelSet(this->hdwf, 0);
		//FDwfAnalogInTriggerTypeSet(this->hdwf, trigtypeEdge);
		//FDwfAnalogInTriggerLevelSet(this->hdwf, 0);
	}
	
	// wait at least 2 seconds with Analog Discovery for the offset to stabilize, before the first reading after device open or offset/range change
	Wait(_settings.waitAd);
	return hz;
}

double daq_dwf::ad_init()
{
	return this->ad_init(this->adSettings);
}

void daq_dwf::ad_start()
{
	// start
	FDwfAnalogInConfigure(this->hdwf, true, true);
}

void daq_dwf::ad_get(const size_t length, double ch1[])
{
	STS sts;
	// wait trigger
	do {
		FDwfAnalogInStatus(this->hdwf, true, &sts);
	} while (sts != stsDone);

	// get the samples for each channel
	FDwfAnalogInStatusData(this->hdwf, 0, ch1, (int)length);
}

void daq_dwf::ad_get(const size_t length, double ch1[], double ch2[])
{
	STS sts;
	// wait trigger
	do {
		FDwfAnalogInStatus(this->hdwf, true, &sts);
	} while (sts != stsDone);

	// get the samples for each channel
	FDwfAnalogInStatusData(this->hdwf, 0, ch1, (int)length);
	FDwfAnalogInStatusData(this->hdwf, 1, ch2, (int)length);
}

void daq_dwf::adWaveforms(const settings_t _settings, double *data)
{
	STS sts;
	double* rgdSamples;
	rgdSamples = new double[_settings.numSampsPerChan];
	
	this->ad_init(_settings);

	// start
	FDwfAnalogInConfigure(this->hdwf, 0, true);

	printf("Waiting for triggered or auto acquisition\n");
	do {
		FDwfAnalogInStatus(this->hdwf, true, &sts);
	} while (sts != stsDone);

	// get the samples for each channel
	for (int c = _settings.ch; c < _settings.ch + _settings.numCh; c++)
	{
		FDwfAnalogInStatusData(this->hdwf, c, rgdSamples, _settings.numSampsPerChan);
		// do something with it
		for (int i = 0; i < _settings.numSampsPerChan; i++)
		{
			data[i * _settings.numCh + c] = rgdSamples[i];
		}
	}
}

void daq_dwf::adWaveforms(const settings_t _settings, double ch1[], double ch2[])
{
	STS sts;

	this->ad_init(_settings);

	// start
	FDwfAnalogInConfigure(this->hdwf, 0, true);
	do {
		FDwfAnalogInStatus(this->hdwf, true, &sts);
	} while (sts != stsDone);

	// get the samples for each channel
	FDwfAnalogInStatusData(this->hdwf, 0, ch1, _settings.numSampsPerChan);
	FDwfAnalogInStatusData(this->hdwf, 1, ch2, _settings.numSampsPerChan);
}

double daq_dwf::ad(const int ch, const int range)
{
	double     data[2];
	settings_t _settings = this->setSettings(ch, 1, 2, range, 100000, 2, -1);

	this->adWaveforms(_settings, data);
	return (data[0] + data[0]) / 2;
}

void daq_dwf::adWaveforms(double* data)
{
	this->adWaveforms(this->adSettings, data);
}

void daq_dwf::adWaveforms(const char *filepath)
{
	std::vector<double> aV((double)adSettings.numSampsPerChan * adSettings.numCh);
	std::vector<std::vector<double>> aDat(adSettings.numSampsPerChan, std::vector<double>(adSettings.numCh + 1));
	this->adWaveforms(aV.data());

	for (int i = 0; i < adSettings.numSampsPerChan; i++)
	{
		aDat[i][0] = i / adSettings.rate;
		for (int j = 1; j < (adSettings.numCh + 1); j++)
			aDat[i][j] = aV[(int)(i * adSettings.numCh + j - 1)];
	}
	csvWriter::write(filepath, aDat);
	printf("A waveform was captured.\n");
}

void daq_dwf::adWaveforms()
{
	this->adWaveforms(DAQMX_FILENAME);
}

void daq_dwf::da(const int ch, const double volts)
{
	FDwfAnalogOutNodeEnableSet(this->hdwf, ch, AnalogOutNodeCarrier, true);
	// set custom function
	FDwfAnalogOutNodeFunctionSet(this->hdwf, ch, AnalogOutNodeCarrier, funcDC);
	// set DC value as offset
	FDwfAnalogOutNodeOffsetSet(this->hdwf, ch, AnalogOutNodeCarrier, volts);
	// start signal generation
	FDwfAnalogOutConfigure(this->hdwf, ch, true);
}

void daq_dwf::daWaveform(const double amplitude, const double freq, const std::vector<double>& waveform)
{
	double buf = 0;
	for (size_t i = 0; i < waveform.size(); i++)
	{
		if (buf < abs(waveform[i]))
			buf = waveform[i];
	}

	FDwfAnalogOutNodeEnableSet(this->hdwf, -1, AnalogOutNodeCarrier, true);
	// set custom function
	FDwfAnalogOutNodeFunctionSet(this->hdwf, 0, AnalogOutNodeCarrier, funcCustom);
	FDwfAnalogOutNodeFunctionSet(this->hdwf, 1, AnalogOutNodeCarrier, funcSquare);
	// set custom waveform samples
	// normalized to ±1 values
	FDwfAnalogOutNodeDataSet(this->hdwf, 0, AnalogOutNodeCarrier, (double*)waveform.data(), waveform.size());
	// 10kHz waveform frequency
	FDwfAnalogOutNodeFrequencySet(this->hdwf, -1, AnalogOutNodeCarrier, freq);
	// set amplitude
	FDwfAnalogOutNodeAmplitudeSet(this->hdwf, 0, AnalogOutNodeCarrier, amplitude / buf);
	FDwfAnalogOutNodeAmplitudeSet(this->hdwf, 1, AnalogOutNodeCarrier, 1.5);
	// by default the offset is 0V
	FDwfAnalogOutNodeOffsetSet(this->hdwf, 1, AnalogOutNodeCarrier, 1.5);
	// start signal generation
	FDwfAnalogOutConfigure(this->hdwf, -1, true);
}

void daq_dwf::daOff()
{
	FDwfAnalogOutConfigure(this->hdwf, 0, false);
	FDwfAnalogOutConfigure(this->hdwf, 1, false);
}

void daq_dwf::fgOn()
{
	// enable channels
	FDwfAnalogOutNodeEnableSet(this->hdwf, 0, AnalogOutNodeCarrier, true);
	FDwfAnalogOutNodeEnableSet(this->hdwf, 1, AnalogOutNodeCarrier, true);
	// set sine function
	FDwfAnalogOutNodeFunctionSet(this->hdwf, 0, AnalogOutNodeCarrier, funcSine);
	FDwfAnalogOutNodeFunctionSet(this->hdwf, 1, AnalogOutNodeCarrier, funcSine);
	// set frequency
	FDwfAnalogOutNodeFrequencySet(this->hdwf, 0, AnalogOutNodeCarrier, this->_fgFreq);
	FDwfAnalogOutNodeFrequencySet(this->hdwf, 1, AnalogOutNodeCarrier, this->_fgFreq);
	// set amplitude
	FDwfAnalogOutNodeAmplitudeSet(this->hdwf, 0, AnalogOutNodeCarrier, this->_fgAmp);
	FDwfAnalogOutNodeAmplitudeSet(this->hdwf, 1, AnalogOutNodeCarrier, this->_fgAmp2);
	// set offset
	FDwfAnalogOutNodeOffsetSet(this->hdwf, 0, AnalogOutNodeCarrier, this->_fgOffset);
	FDwfAnalogOutNodeOffsetSet(this->hdwf, 1, AnalogOutNodeCarrier, 0);
	// set phase
	FDwfAnalogOutNodePhaseSet(this->hdwf, 0, AnalogOutNodeCarrier, this->_fgPhase);
	FDwfAnalogOutNodePhaseSet(this->hdwf, 1, AnalogOutNodeCarrier, this->_fgPhase2);

	// start signal generation
	FDwfAnalogOutConfigure(this->hdwf, -1, true);
	// it will run until stopped, reset, parameter changed or device closed
}

void daq_dwf::fg(const double amplitude, const double freq, const double phase, const double amplitude2 = 0, const double phase2 = 0)
{
	if (phase < 0)
	{
		this->_fgPhase = phase + 360;
		this->_fgPhase2 = phase2 + 360;
	}
	else
	{
		this->_fgPhase = phase;
		this->_fgPhase2 = phase2;
	}
	this->_fgAmp = amplitude; this->_fgFreq = freq;
	this->_fgAmp2 = amplitude2;
	this->fgOn();
}

void daq_dwf::fgOff()
{
	//this->da(0, 0);
	//this->da(1, 0);
	FDwfAnalogOutConfigure(this->hdwf, 0, false);
	FDwfAnalogOutConfigure(this->hdwf, 1, false);
}

void daq_dwf::fgFreq(const double freq)
{
	this->_fgFreq = freq;
	this->fgOn();
}

double daq_dwf::fgFreq()
{
	return this->_fgFreq;
}

void daq_dwf::fgVpp(const double vpp)
{
	this->_fgAmp = vpp / 2;
	this->fgOn();
}

double daq_dwf::fgVpp()
{
	return this->_fgAmp * 2;
}

void daq_dwf::fgVrms(const double vrms)
{
	this->_fgAmp = vrms*pow(2, 0.5);
	this->fgOn();
}

double daq_dwf::fgVrms()
{
	return this->_fgAmp / pow(2, 0.5);
}

void daq_dwf::fgPhase(const double radians)
{
	this->_fgPhase = radians;
	this->fgOn();
}

double daq_dwf::fgPhase()
{
	return this->_fgPhase;
}

void daq_dwf::powerSupply(const double volts)
{
	if (volts == 0)
	{
		FDwfAnalogIOChannelNodeSet(this->hdwf, 0, 0, false);
		FDwfAnalogIOChannelNodeSet(this->hdwf, 1, 0, false);
		FDwfAnalogIOEnableSet(this->hdwf, false);
		FDwfAnalogIOChannelNodeSet(this->hdwf, 0, 1, 0);
		FDwfAnalogIOChannelNodeSet(this->hdwf, 1, 1, 0);
	}
	else
	{
		// set voltage(from 0 to 5 V)
		FDwfAnalogIOChannelNodeSet(this->hdwf, 0, 1, abs(volts));
		// set voltage(from 0 to - 5 V)
		FDwfAnalogIOChannelNodeSet(this->hdwf, 1, 1, -abs(volts));
		// enable positive and negative supply
		FDwfAnalogIOChannelNodeSet(this->hdwf, 0, 0, true);
		FDwfAnalogIOChannelNodeSet(this->hdwf, 1, 0, true);
		// master enable
		FDwfAnalogIOEnableSet(this->hdwf, true);
	}
}

void daq_dwf::psd(const double freq, const double rate, const int num, const double wave[], double *x, double *y)
{
	static double f = 0, r = 0;
	static std::size_t n = 0;
	static double *aSin = NULL, *aCos = NULL;

	if (f != freq && r != rate && n != num)
	{
		f = freq; r = rate; n = num;

		delete[] aSin;
		delete[] aCos;
		aSin = new double[n];
		aCos = new double[n];
		for (std::size_t i = 0; i < n; i++)
		{
			aSin[i] = sin(2 * PI * f / r * (i + 1));
			aCos[i] = cos(2 * PI * f / r * (i + 1));
		}
	}

	int n_end = (int)((1 / freq*rate) * (n / (int)(1 / freq*rate)));
	*x = 0, *y = 0;
	for (std::size_t i = 0; i < n_end; i++)
	{
		*x += aSin[i] * wave[i];
		*y += aCos[i] * wave[i];
	}
	*x /= (n_end / 2);
	*y /= (n_end / 2);
}
