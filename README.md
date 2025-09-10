# Software Lock-in Amplifier using Digilent Analog Discovery
  ![Hard copy](./docs/images/HardCopy.png)
## What is this?
  - This software facilitates the construction of a software Lock-In Amplifier (LIA) within the Windows using the Digilent Analog Discovery 2 or 3 (AD).
  - The LIA is an instrument that has the capacity to measure the amplitude and phase of sine wave voltages.
  - It utilizes a technique known as phase-sensitive detection (PSD) or Synchronous Detection, as shown in the figure below.
  ![PSD](./docs/images/PSD.png)
## Usage
  - The subsequent diagram illustrates the circuit configuration for Eddy Current Testing (ECT).
  ![Circuit](./docs/images/Circuit.svg)
  ![Photo of Circuit](./docs/images/PhotoOfCircuit.jpg)
  - The AD620 and INA128/129 are regarded as effective instrument amplifiers.
  - The provision of power for sensors, such as coils, and the amplifier can be facilitated by the AD.
  - However, it is imperative to exercise caution with regard to the power supply limitations inherent to the AD. For instance, the maximum voltage from AD is ±5V, and the current is constrained by the capabilities of the USB connection or any additional AC adapters connected.
  - Due to input voltage range of the AD is ±25V, which allows for the possibility of supplying higher voltages through external power sources. It is imperative to exercise caution and avoid the application of excessive voltage or current to the AD to avert potential damage.
## Used software
  - [Digilent Waveforms SDK](https://digilent.com/reference/software/waveforms/waveforms-sdk/reference-manual)
  - [GLFW](https://www.glfw.org/)
  - [Dear ImGui](https://github.com/ocornut/imgui) & [ImPlot](https://github.com/epezent/implot)
  - [inifile-cpp](https://github.com/Rookfighter/inifile-cpp)
