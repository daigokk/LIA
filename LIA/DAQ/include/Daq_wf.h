#pragma once

#pragma	comment(lib, "DAQ/lib/dwf.lib")
#include <iostream>         // needed for input/output
#include <string>           // needed for error handling
//#include <fstream>          // needed for input/output
#include <dwf.h>


class Daq_dwf
{
public:
    struct Device
    {
    public:
        HDWF hdwf = 0;
        char name[256] = { "" };
        char sn[32] = { "" };
    } device;
    static void errChk(bool ret, const char* func, const char* file, int line)
    {
#ifndef NODEBUG
        static char szError[512] = { 0 };
        if (!ret) {
            FDwfGetLastErrorMsg(szError);
            std::cerr << "\a\n--- DWF error -----------------------------------\n";
            std::cerr << "Function: " << func << std::endl;
            std::cerr << "File: " << file << ", " << line << std::endl;
            std::cerr << szError;
            std::cerr << "-------------------------------------------------\n";
            system("PAUSE");
            exit(EXIT_FAILURE);
        }
#endif
    }
    static int getPcDevice()
    {
        int pcDevice;
        errChk(FDwfEnum(enumfilterAll, &pcDevice), __func__, __FILE__, __LINE__);
        return pcDevice;
    }
    static int getIdxDevice(const std::string sn)
    {
        for (int i = 0; i < getPcDevice(); i++)
        {
            char szSN[32] = { "" };
            errChk(FDwfEnumSN(i, szSN), __func__, __FILE__, __LINE__);
            if (sn == szSN) return i;
        }
        return -1;
	}
    static int getIdxFirstEnabledDevice()
    {
        for (int i = 0; i < getPcDevice(); i++)
        {
            int flag;
            errChk(FDwfEnumDeviceIsOpened(i, &flag), __func__, __FILE__, __LINE__);
            if (!flag) return i;
        }
        return -1;
    }
    void init(int idxDevice)
    {
        errChk(FDwfEnumDeviceName(idxDevice, device.name), __func__, __FILE__, __LINE__);
        errChk(FDwfEnumSN(idxDevice, device.sn), __func__, __FILE__, __LINE__);
        errChk(FDwfDeviceOpen(idxDevice, &device.hdwf), __func__, __FILE__, __LINE__);
        awg.pHdwf = &device.hdwf;
        scope.pHdwf = &device.hdwf;
    }
    Daq_dwf()
    {
		int idxDevice = getIdxFirstEnabledDevice();
        if (idxDevice == -1)
        {
            std::cerr << "No AD is connected." << std::endl;
            exit(EXIT_FAILURE);
        }
		init(idxDevice);
    }
    Daq_dwf(int idxDevice)
    {
        init(idxDevice);
    }
    ~Daq_dwf()
    {
        // close the connection
        FDwfDeviceClose(device.hdwf);
    }
    void powerSupply(const double volts = 5.0)
    {
        bool flag = true;
        if (volts == 0.0) flag = false;
        errChk( // set voltage(from 0 to 5 V)
            FDwfAnalogIOChannelNodeSet(device.hdwf, 0, 1, abs(volts)),
            __func__, __FILE__, __LINE__
        );
        errChk( // set voltage(from 0 to -5 V)
            FDwfAnalogIOChannelNodeSet(device.hdwf, 1, 1, -abs(volts)),
            __func__, __FILE__, __LINE__
        );
        errChk( // enable positive supply
            FDwfAnalogIOChannelNodeSet(device.hdwf, 0, 0, flag),
            __func__, __FILE__, __LINE__
        );
        errChk( // enable negative supply
            FDwfAnalogIOChannelNodeSet(device.hdwf, 1, 0, flag),
            __func__, __FILE__, __LINE__
        );
        errChk( // master enable
            FDwfAnalogIOEnableSet(device.hdwf, flag),
            __func__, __FILE__, __LINE__
        );
    }
    class Awg
    {
    public:
        HDWF* pHdwf = nullptr;
        int channel = -1;
        FUNC func1 = funcSine, func2 = funcSine;
        double frequency1 = 1e3, frequency2 = 1e3;
        double amplitude1 = 1.0, amplitude2 = 0.0;
        double phaseDeg1 = 0.0, phaseDeg2 = 0.0;
        double rgdData1[5000], rgdData2[5000];
        void start()
        {
            errChk( // enable channel
                FDwfAnalogOutNodeEnableSet(*pHdwf, 0, AnalogOutNodeCarrier, true),
                __func__, __FILE__, __LINE__
            );
            errChk( // enable channel
                FDwfAnalogOutNodeEnableSet(*pHdwf, 1, AnalogOutNodeCarrier, true),
                __func__, __FILE__, __LINE__
            );
            errChk( // set function type
                FDwfAnalogOutNodeFunctionSet(*pHdwf, 0, AnalogOutNodeCarrier, func1),
                __func__, __FILE__, __LINE__
            );
            errChk( // set function type
                FDwfAnalogOutNodeFunctionSet(*pHdwf, 1, AnalogOutNodeCarrier, func2),
                __func__, __FILE__, __LINE__
            );
            if (func1 == funcCustom)
            {
                FDwfAnalogOutNodeDataSet(*pHdwf, 0, AnalogOutNodeCarrier, rgdData1, 5000);
            }
            if (func2 == funcCustom)
            {
                FDwfAnalogOutNodeDataSet(*pHdwf, 1, AnalogOutNodeCarrier, rgdData2, 5000);
            }
            errChk( // set frequency
                FDwfAnalogOutNodeFrequencySet(*pHdwf, 0, AnalogOutNodeCarrier, frequency1),
                __func__, __FILE__, __LINE__
            );
            errChk( // set frequency
                FDwfAnalogOutNodeFrequencySet(*pHdwf, 1, AnalogOutNodeCarrier, frequency2),
                __func__, __FILE__, __LINE__
            );
            errChk( // set amplitude or DC voltage
                FDwfAnalogOutNodeAmplitudeSet(*pHdwf, 0, AnalogOutNodeCarrier, amplitude1),
                __func__, __FILE__, __LINE__
            );
            errChk( // set amplitude or DC voltage
                FDwfAnalogOutNodeAmplitudeSet(*pHdwf, 1, AnalogOutNodeCarrier, amplitude2),
                __func__, __FILE__, __LINE__
            );
            errChk( // set offset
                FDwfAnalogOutNodeOffsetSet(*pHdwf, channel, AnalogOutNodeCarrier, 0.0),
                __func__, __FILE__, __LINE__
            );
            errChk( // set pahse
                FDwfAnalogOutNodePhaseSet(*pHdwf, 0, AnalogOutNodeCarrier, phaseDeg1),
                __func__, __FILE__, __LINE__
            );
            errChk( // set pahse
                FDwfAnalogOutNodePhaseSet(*pHdwf, 1, AnalogOutNodeCarrier, phaseDeg2),
                __func__, __FILE__, __LINE__
            );
            errChk( // start signal generation
                FDwfAnalogOutConfigure(*pHdwf, channel, true),
                __func__, __FILE__, __LINE__
            );
            return;
        }
        void start(const double frequency, const double amplitude1, const double phaseDeg1)
        {
            this->frequency1 = frequency;
            this->amplitude1 = amplitude1;
            this->phaseDeg1 = phaseDeg1;
            this->amplitude2 = 0.0;
            this->phaseDeg2 = 0.0;
            this->start();
        }
        void start(
            const double frequency1, const double amplitude1, const double phaseDeg1,
            const double frequency2, const double amplitude2, const double phaseDeg2
        )
        {
            this->frequency1 = frequency1;
            this->amplitude1 = amplitude1;
            this->phaseDeg1 = phaseDeg1;
            this->frequency2 = frequency2;
            this->amplitude2 = amplitude2;
            this->phaseDeg2 = phaseDeg2;
            this->start();
        }
        void off()
        {
            errChk( // enable channel
                FDwfAnalogOutNodeEnableSet(*pHdwf, 0, AnalogOutNodeCarrier, false),
                __func__, __FILE__, __LINE__
            );
            errChk( // enable channel
                FDwfAnalogOutNodeEnableSet(*pHdwf, 1, AnalogOutNodeCarrier, false),
                __func__, __FILE__, __LINE__
            );
        }
    } awg;
    class Scope
    {
    public:
        HDWF* pHdwf;
        int channel = -1;
        double voltsRange = 5.0;
        int bufferSize = 5000;
        double SamplingRate = 100e6;
        double secTimeout = 0.0;
        TRIGSRC trigSrc = trigsrcAnalogOut1;
        int trigChannel = 0;
        TRIGTYPE trigType = trigtypeEdge;
        double trigVoltLevel = 0.0;
        DwfTriggerSlope trigSlope = DwfTriggerSlopeRise;
        void open()
        {
            errChk( // enable all channels
                FDwfAnalogInChannelEnableSet(*pHdwf, channel, true),
                __func__, __FILE__, __LINE__
            );
            errChk( // set offset voltage(in Volts)
                FDwfAnalogInChannelOffsetSet(*pHdwf, channel, 0.0),
                __func__, __FILE__, __LINE__
            );
            errChk( // set range (maximum signal amplitude in Volts)
                FDwfAnalogInChannelRangeSet(*pHdwf, channel, voltsRange * 2),
                __func__, __FILE__, __LINE__
            );
            errChk( // set the buffer size per channel (data point in a recording)
                FDwfAnalogInBufferSizeSet(*pHdwf, bufferSize),
                __func__, __FILE__, __LINE__
            );
            FDwfAnalogInBufferSizeGet(*pHdwf, &bufferSize);

            errChk( // set the acquisition frequency (in Hz)
                FDwfAnalogInFrequencySet(*pHdwf, SamplingRate),
                __func__, __FILE__, __LINE__
            );
            FDwfAnalogInFrequencyGet(*pHdwf, &SamplingRate);
            //std::cout << SamplingRate << std::endl;
            errChk( // disable averaging (for more info check the documentation)
                FDwfAnalogInChannelFilterSet(*pHdwf, channel, filterAverageFit),
                __func__, __FILE__, __LINE__
            );
            return;
        }
        void open(const int channel, const double voltsRange, const int bufferSize, const double SamplingRate)
        {
            this->channel = channel;
            this->voltsRange = voltsRange;
            this->bufferSize = bufferSize;
            this->SamplingRate = SamplingRate;
            this->open();
        }
        void trigger()
        {
            errChk( // enable/disable auto triggering
                FDwfAnalogInTriggerAutoTimeoutSet(*pHdwf, secTimeout),
                __func__, __FILE__, __LINE__
            );
            errChk( // set trigger source
                FDwfAnalogInTriggerSourceSet(*pHdwf, trigSrc),
                __func__, __FILE__, __LINE__
            );

            // set trigger channel
            if (trigSrc != trigsrcAnalogOut1)
            {
                if (trigSrc == trigsrcDetectorAnalogIn)
                {
                    errChk(
                        FDwfAnalogInTriggerChannelSet(*pHdwf, trigChannel),
                        __func__, __FILE__, __LINE__
                    );
                }
                errChk( // set trigger type
                    FDwfAnalogInTriggerTypeSet(*pHdwf, trigType),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set trigger type
                    FDwfAnalogInTriggerLevelSet(*pHdwf, trigVoltLevel),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set trigger edge
                    FDwfAnalogInTriggerConditionSet(*pHdwf, trigSlope),
                    __func__, __FILE__, __LINE__
                );
            }
            return;
        }
        void trigger(
            const double secTimeout, const TRIGSRC trigSrc,
            const int trigChannel, const TRIGTYPE trigType, const double trigVoltLevel, DwfTriggerSlope trigSlope
        )
        {
            this->secTimeout = secTimeout;
            this->trigSrc = trigSrc;
            this->trigChannel = trigChannel;
            this->trigType = trigType;
            this->trigVoltLevel = trigVoltLevel;
            this->trigSlope = trigSlope;
            this->trigger();
        }
        void start()
        {
            errChk( // set up the instrument
                FDwfAnalogInConfigure(*pHdwf, true, true),
                __func__, __FILE__, __LINE__
            );
        }
        void record(double buffer1[])
        {
            // read data to an internal buffer
            DwfState sts;
            do {
                errChk( // read store buffer status
                    FDwfAnalogInStatus(*pHdwf, true, &sts),
                    __func__, __FILE__, __LINE__
                );
            } while (sts != stsDone);
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf, 0, buffer1, bufferSize),
                __func__, __FILE__, __LINE__
            );
            return;
        }
        void record(double buffer1[], double buffer2[])
        {
            // read data to an internal buffer
            DwfState sts;
            do {
                errChk( // read store buffer status
                    FDwfAnalogInStatus(*pHdwf, true, &sts),
                    __func__, __FILE__, __LINE__
                );
            } while (sts != stsDone);
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf, 0, buffer1, bufferSize),
                __func__, __FILE__, __LINE__
            );
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf, 1, buffer2, bufferSize),
                __func__, __FILE__, __LINE__
            );
            return;
        }
    } scope;
};
