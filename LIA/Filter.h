#pragma once

//================================================================================
// Filter Class
//================================================================================
class HighPassFilter {
private:
    double m_alpha = 1.0;
    double m_prevInput = 0.0;
    double m_prevOutput = 0.0;
    double m_dt = 0.0;
    double m_cutoffFrequency = 0.0;

public:
    void setCutoffFrequency(double newFrequency, double newDt) {
        if (m_cutoffFrequency == newFrequency) {
            return; // No change needed
        }
        m_dt = newDt;
        if (newFrequency <= 0) {
            m_cutoffFrequency = 0.0;
            m_alpha = 1.0;
        }
        else {
            m_cutoffFrequency = newFrequency;
            double rc = 1.0 / (2.0 * std::numbers::pi * m_cutoffFrequency);
            m_alpha = rc / (rc + m_dt);
        }
    }

    double process(double input) {
        if (m_cutoffFrequency == 0.0) {
            m_prevInput = input;
            m_prevOutput = input;
            return input;
        }

        // Apply the high-pass filter formula
        double output = m_alpha * (m_prevOutput + input - m_prevInput);

        // Update state for next iteration
        m_prevInput = input;
        m_prevOutput = output;

        return output;
    }
};

class LowPassFilter {
private:
    double m_alpha = 1.0;        // 1.0 means no filtering (output = input) by default
    double m_prevOutput = 0.0;   // State: Previous output (y[i-1])
    double m_dt = 0.0;
    double m_cutoffFrequency = 0.0;
    // m_prevInput is removed as it's not needed for LPF

public:
    void setCutoffFrequency(double newFrequency, double newDt) {
        if (m_cutoffFrequency == newFrequency) {
            return; // No change needed
        }
        m_dt = newDt;
        m_cutoffFrequency = newFrequency;

        if (newFrequency <= 0) {
            // Disable filtering (pass-through) logic often implies alpha = 1.0 for LPF
            m_alpha = 1.0;
        }
        else {
            double rc = 1.0 / (2.0 * std::numbers::pi * m_cutoffFrequency);
            // LPF alpha formula: dt / (RC + dt)
            m_alpha = m_dt / (rc + m_dt);
        }
    }

    double process(double input) {
        // Handle initial state or pass-through case
        // If alpha is 1.0, it simply returns the input (no filtering)
        if (m_alpha >= 1.0) {
            m_prevOutput = input;
            return input;
        }

        // Apply the low-pass filter formula (Exponential Moving Average)
        // Formula: y[i] = y[i-1] + alpha * (x[i] - y[i-1])
        // Equivalent to: output = alpha * input + (1 - alpha) * prevOutput
        double output = m_prevOutput + m_alpha * (input - m_prevOutput);

        // Update state for next iteration
        m_prevOutput = output;

        return output;
    }

    // Optional: Helper to reset the filter if needed (e.g. on stream start)
    void reset(double initialValue = 0.0) {
        m_prevOutput = initialValue;
    }
};