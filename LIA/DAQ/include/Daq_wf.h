#pragma	comment(lib, "DAQ/lib/dwf.lib")
#include <iostream>         // needed for input/output
#include <string>           // needed for error handling
#include <fstream>          // needed for input/output
#include <dwf.h>


void errChk(bool ret, const char* func, const char* file, int line)
{
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
}

class Daq_wf
{
private:
    struct Device
    {
    public:
        HDWF hdwf = 0;
        char name[256] = { "" };
        char sn[32] = { "" };
    } Device;

public:
    Daq_wf(int idxDevice = 0)
    {
        int pcDevice;
        errChk(FDwfEnum(enumfilterAll, &pcDevice), __func__, __FILE__, __LINE__);
        errChk(FDwfEnumDeviceName(idxDevice, Device.name), __func__, __FILE__, __LINE__);
        errChk(FDwfEnumSN(idxDevice, Device.sn), __func__, __FILE__, __LINE__);
        errChk(FDwfDeviceOpen(idxDevice, &Device.hdwf), __func__, __FILE__, __LINE__);

        std::cout << Device.name << "," << Device.sn << std::endl;
        Fg.pHdwf = &Device.hdwf;
        Scope.pHdwf = &Device.hdwf;
    }
    ~Daq_wf()
    {
        // close the connection
        FDwfDeviceClose(Device.hdwf);
    }
    void powerSupply(const double volts = 5.0)
    {
        bool flag = true;
        if (volts == 0.0) flag = false;
        // set voltage(from 0 to 5 V)
        FDwfAnalogIOChannelNodeSet(Device.hdwf, 0, 1, abs(volts));
        // set voltage(from 0 to - 5 V)
        FDwfAnalogIOChannelNodeSet(Device.hdwf, 1, 1, -abs(volts));
        // enable positive and negative supply
        FDwfAnalogIOChannelNodeSet(Device.hdwf, 0, 0, true);
        FDwfAnalogIOChannelNodeSet(Device.hdwf, 1, 0, true);
        // master enable
        FDwfAnalogIOEnableSet(Device.hdwf, flag);
    }
    class _Fg
    {
    public:
        HDWF* pHdwf;
        int channel = 0;
        double frequency = 1e3;
        double amplitude = 1.0;
        double phaseDeg = 0.0;
        void start()
        {
            errChk( // enable channel
                FDwfAnalogOutNodeEnableSet(*pHdwf, channel, AnalogOutNodeCarrier, true),
                __func__, __FILE__, __LINE__
            );
            errChk( // set function type
                FDwfAnalogOutNodeFunctionSet(*pHdwf, channel, AnalogOutNodeCarrier, funcSine),
                __func__, __FILE__, __LINE__
            );
            errChk( // set frequency
                FDwfAnalogOutNodeFrequencySet(*pHdwf, channel, AnalogOutNodeCarrier, frequency),
                __func__, __FILE__, __LINE__
            );
            errChk( // set amplitude or DC voltage
                FDwfAnalogOutNodeAmplitudeSet(*pHdwf, channel, AnalogOutNodeCarrier, amplitude),
                __func__, __FILE__, __LINE__
            );
            errChk( // set offset
                FDwfAnalogOutNodeOffsetSet(*pHdwf, channel, AnalogOutNodeCarrier, 0.0),
                __func__, __FILE__, __LINE__
            );
            errChk( // set pahse
                FDwfAnalogOutNodePhaseSet(*pHdwf, channel, AnalogOutNodeCarrier, phaseDeg),
                __func__, __FILE__, __LINE__
            );
            errChk( // start signal generation
                FDwfAnalogOutConfigure(*pHdwf, channel, true),
                __func__, __FILE__, __LINE__
            );
            return;
        }
        void start(const double frequency, const double amplitude, const double phaseDeg)
        {
            this->frequency = frequency;
            this->amplitude = amplitude;
            this->phaseDeg = phaseDeg;
            this->start();
        }
    } Fg;
    class _Scope
    {
    public:
        HDWF* pHdwf;
        int channel = 0;
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
                FDwfAnalogInChannelRangeSet(*pHdwf, channel, voltsRange),
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
                FDwfAnalogInChannelFilterSet(*pHdwf, channel, filterDecimate),
                __func__, __FILE__, __LINE__
            );
            //std::cout << bufferSize << ", " << SamplingRate << std::endl;
            return;
        }
        void trigger()
        {
            //errChk( // enable/disable auto triggering
            //    FDwfAnalogInTriggerAutoTimeoutSet(*pHdwf, secTimeout),
            //    __func__, __FILE__, __LINE__
            //);
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
           
            errChk( // set up the instrument
                FDwfAnalogInConfigure(*pHdwf, true, true),
                __func__, __FILE__, __LINE__
            );
            return;
        }
        void record(double buffer[])
        {
            // read data to an internal buffer
            DwfState sts;
            do {
                errChk( // read store buffer status
                    FDwfAnalogInStatus(*pHdwf, false, &sts),
                    __func__, __FILE__, __LINE__
                );
            } while (sts != stsDone);
            errChk( // set up the instrument
                FDwfAnalogInStatusData(*pHdwf, channel, buffer, bufferSize),
                __func__, __FILE__, __LINE__
            );
            errChk( // set up the instrument
                FDwfAnalogInConfigure(*pHdwf, true, true),
                __func__, __FILE__, __LINE__
            );
            return;
        }
    } Scope;
};
