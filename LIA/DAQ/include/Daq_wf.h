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
        HDWF hdwf[2] = { 0, 0 };
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
        errChk(FDwfDeviceOpen(idxDevice, &device.hdwf[0]), __func__, __FILE__, __LINE__);
        awg.pHdwf[0] = &device.hdwf[0];
        scope.pHdwf[0] = &device.hdwf[0];
        if (getIdxFirstEnabledDevice() >= 0)
        {
            errChk(FDwfDeviceOpen(getIdxFirstEnabledDevice(), &device.hdwf[1]), __func__, __FILE__, __LINE__);
            awg.pHdwf[1] = &device.hdwf[1];
            scope.pHdwf[1] = &device.hdwf[1];
        }
    }
    Daq_dwf()
    {
        int pcDevice = getPcDevice();
        if (pcDevice < 1)
        {
            std::cerr << "No AD is connected." << std::endl;
            exit(EXIT_FAILURE);
        }
		init(getIdxFirstEnabledDevice());
    }
    Daq_dwf(const std::string sn)
    {
        for (int i = 0; i < getPcDevice(); i++)
        {
            char szSN[32] = { "" };
            errChk(FDwfEnumSN(i, szSN), __func__, __FILE__, __LINE__);
            if (sn == szSN)
            {
                init(i);
				return;
            }
        }
        std::cerr << "No AD is connected." << std::endl;
        exit(EXIT_FAILURE);
    }
    ~Daq_dwf()
    {
        // close the connection
        FDwfDeviceClose(device.hdwf[0]);
        FDwfDeviceClose(device.hdwf[1]);
    }
    void powerSupply(const double volts = 5.0)
    {
        bool flag = true;
        if (volts == 0.0) flag = false;
        errChk( // set voltage(from 0 to 5 V)
            FDwfAnalogIOChannelNodeSet(device.hdwf[0], 0, 1, abs(volts)),
            __func__, __FILE__, __LINE__
        );
        errChk( // set voltage(from 0 to -5 V)
            FDwfAnalogIOChannelNodeSet(device.hdwf[0], 1, 1, -abs(volts)),
            __func__, __FILE__, __LINE__
        );
        errChk( // enable positive supply
            FDwfAnalogIOChannelNodeSet(device.hdwf[0], 0, 0, flag),
            __func__, __FILE__, __LINE__
        );
        errChk( // enable negative supply
            FDwfAnalogIOChannelNodeSet(device.hdwf[0], 1, 0, flag),
            __func__, __FILE__, __LINE__
        );
        errChk( // master enable
            FDwfAnalogIOEnableSet(device.hdwf[0], flag),
            __func__, __FILE__, __LINE__
        );
    }
    class Awg
    {
    public:
        HDWF* pHdwf[2] = { nullptr, nullptr };
        FUNC func[4] = { funcSine, funcSine, funcSine, funcSine };
        const TRIGSRC trigsrc[4] = { trigsrcNone, trigsrcAnalogOut1, trigsrcExternal1, trigsrcExternal1 };
        double frequency[4] = { 1e3, 1e3, 1e3, 1e3 };
        double amplitude[4] = { 1.0, 0.0, 1.0, 0.0 };
        double phaseDeg[4] = { 0.0, 0.0, 0.0, 0.0 };
        double rgdData[4][5000];
        void start()
        {
            int nch = 2;
            if (pHdwf[1] != nullptr) nch = 4;
            for (int i = 0; i < nch; i++)
            {
                errChk( // enable channel
                    FDwfAnalogOutNodeEnableSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, true),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set function type
                    FDwfAnalogOutNodeFunctionSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, func[i]),
                    __func__, __FILE__, __LINE__
                );
                if (func[i] == funcCustom)
                {
                    FDwfAnalogOutNodeDataSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, rgdData[i], 5000);
                }
                errChk( // set frequency
                    FDwfAnalogOutNodeFrequencySet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, frequency[i]),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set amplitude or DC voltage
                    FDwfAnalogOutNodeAmplitudeSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, amplitude[i]),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set offset
                    FDwfAnalogOutNodeOffsetSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, 0.0),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set phase
                    FDwfAnalogOutNodePhaseSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, phaseDeg[i]),
                    __func__, __FILE__, __LINE__
                );
                errChk( // Sync phase
                    FDwfAnalogOutTriggerSourceSet(*pHdwf[i / 2], i % 2, trigsrc[i]),
                    __func__, __FILE__, __LINE__
                );
			}
            for (int i = 0; i < nch; i++)
            {
                errChk( // start signal generation
                    FDwfAnalogOutConfigure(*pHdwf[i / 2], i % 2, true),
                    __func__, __FILE__, __LINE__
                );
            }
        }
        void start(const double frequency, const double amplitude1, const double phaseDeg1)
        {
            this->frequency[0] = frequency;
            this->amplitude[0] = amplitude1;
            this->phaseDeg[0] = phaseDeg1;
            for (int i = 1; i < 4; i++)
            {
                this->frequency[i] = frequency;
                this->amplitude[i] = 0.0;
                this->phaseDeg[i] = 0.0;
            }
            this->start();
        }
        void start(
            const double frequency1, const double amplitude1, const double phaseDeg1,
            const double frequency2, const double amplitude2, const double phaseDeg2
        )
        {
            this->frequency[0] = frequency1;
            this->amplitude[0] = amplitude1;
            this->phaseDeg[0] = phaseDeg1;
            this->frequency[1] = frequency2;
            this->amplitude[1] = amplitude2;
            this->phaseDeg[1] = phaseDeg2;
            this->start();
        }
        void off()
        {
            int nch = 2;
            if (pHdwf[1] != nullptr) nch = 4;
            for (int i = 0; i < nch; i++)
            {
                errChk( // disable channel
                    FDwfAnalogOutNodeEnableSet(*pHdwf[i / 2], i % 2, AnalogOutNodeCarrier, false),
                    __func__, __FILE__, __LINE__
                );
            }
        }
    } awg;
    class Scope
    {
    public:
        HDWF* pHdwf[2] = { nullptr, nullptr };
        double voltsRange = 5.0;
        int bufferSize = 5000;
        double SamplingRate = 100e6;
        double secTimeout = 0.0;
        TRIGSRC trigSrc[2] = { trigsrcAnalogOut1, trigsrcExternal1 };
        int trigChannel = 0;
        TRIGTYPE trigType = trigtypeEdge;
        double trigVoltLevel = 0.0;
        DwfTriggerSlope trigSlope = DwfTriggerSlopeRise;
        void open()
        {
            int nDevice = 1;
            if (pHdwf[1] != nullptr) nDevice = 2;
            for (int i = 0; i < nDevice; i++)
            {
                errChk( // enable all channels
                    FDwfAnalogInChannelEnableSet(*pHdwf[i], -1, true),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set offset voltage(in Volts)
                    FDwfAnalogInChannelOffsetSet(*pHdwf[i], -1 % 2, 0.0),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set range (maximum signal amplitude in Volts)
                    FDwfAnalogInChannelRangeSet(*pHdwf[i], -1, voltsRange * 2),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set the buffer size per channel (data point in a recording)
                    FDwfAnalogInBufferSizeSet(*pHdwf[i], bufferSize),
                    __func__, __FILE__, __LINE__
                );
                FDwfAnalogInBufferSizeGet(*pHdwf[i], &bufferSize);

                errChk( // set the acquisition frequency (in Hz)
                    FDwfAnalogInFrequencySet(*pHdwf[i], SamplingRate),
                    __func__, __FILE__, __LINE__
                );
                errChk( // disable averaging (for more info check the documentation)
                    FDwfAnalogInChannelFilterSet(*pHdwf[i], -1, filterAverageFit),
                    __func__, __FILE__, __LINE__
                );
            }
            return;
        }
        void open(const double voltsRange, const int bufferSize, const double SamplingRate)
        {
            this->voltsRange = voltsRange;
            this->bufferSize = bufferSize;
            this->SamplingRate = SamplingRate;
            this->open();
        }
        void trigger()
        {
            int nDevice = 1;
            if (pHdwf[1] != nullptr) nDevice = 2;
            for (int i = 0; i < nDevice; i++)
            {
                errChk( // enable/disable auto triggering
                    FDwfAnalogInTriggerAutoTimeoutSet(*pHdwf[i], secTimeout),
                    __func__, __FILE__, __LINE__
                );
                errChk( // set trigger source
                    FDwfAnalogInTriggerSourceSet(*pHdwf[i], trigSrc[i]),
                    __func__, __FILE__, __LINE__
                );

                // set trigger channel
                if (trigSrc[i] != trigsrcAnalogOut1)
                {
                    if (trigSrc[i] == trigsrcDetectorAnalogIn)
                    {
                        errChk(
                            FDwfAnalogInTriggerChannelSet(*pHdwf[i], trigChannel),
                            __func__, __FILE__, __LINE__
                        );
                    }
                    errChk( // set trigger type
                        FDwfAnalogInTriggerTypeSet(*pHdwf[i], trigType),
                        __func__, __FILE__, __LINE__
                    );
                    errChk( // set trigger type
                        FDwfAnalogInTriggerLevelSet(*pHdwf[i], trigVoltLevel),
                        __func__, __FILE__, __LINE__
                    );
                    errChk( // set trigger edge
                        FDwfAnalogInTriggerConditionSet(*pHdwf[i], trigSlope),
                        __func__, __FILE__, __LINE__
                    );
                }
            }
            return;
        }
        void trigger(
            const double secTimeout, const TRIGSRC trigSrc,
            const int trigChannel, const TRIGTYPE trigType, const double trigVoltLevel, DwfTriggerSlope trigSlope
        )
        {
            this->secTimeout = secTimeout;
            this->trigSrc[0] = trigSrc;
            this->trigChannel = trigChannel;
            this->trigType = trigType;
            this->trigVoltLevel = trigVoltLevel;
            this->trigSlope = trigSlope;
            this->trigger();
        }
        void start()
        {
            int nDevice = 1;
            if (pHdwf[1] != nullptr) nDevice = 2;
            for (int i = 0; i < nDevice; i++)
            {
                errChk( // set up the instrument
                    FDwfAnalogInConfigure(*pHdwf[i], true, true),
                    __func__, __FILE__, __LINE__
                );
            }
        }
        void record(double buffer1[])
        {
            // read data to an internal buffer
            DwfState sts;
            do {
                errChk( // read store buffer status
                    FDwfAnalogInStatus(*pHdwf[0], true, &sts),
                    __func__, __FILE__, __LINE__
                );
            } while (sts != stsDone);
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf[0], 0, buffer1, bufferSize),
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
                    FDwfAnalogInStatus(*pHdwf[0], true, &sts),
                    __func__, __FILE__, __LINE__
                );
            } while (sts != stsDone);
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf[0], 0, buffer1, bufferSize),
                __func__, __FILE__, __LINE__
            );
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf[0], 1, buffer2, bufferSize),
                __func__, __FILE__, __LINE__
            );
            return;
        }
        void record2(double buffer1[], double buffer2[])
        {
            // read data to an internal buffer
            DwfState sts;
            do {
                errChk( // read store buffer status
                    FDwfAnalogInStatus(*pHdwf[1], true, &sts),
                    __func__, __FILE__, __LINE__
                );
            } while (sts != stsDone);
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf[1], 0, buffer1, bufferSize),
                __func__, __FILE__, __LINE__
            );
            errChk( // read store buffer
                FDwfAnalogInStatusData(*pHdwf[1], 1, buffer2, bufferSize),
                __func__, __FILE__, __LINE__
            );
            return;
        }
    } scope;
};
