#ifndef MCP3008_H
#define MCP3008_H

#include <QObject>
#include <QMutex>
#include <cstdint>

/**
 * @brief Interface for MCP3008 8-channel 10-bit ADC via SPI
 * 
 * This class provides communication with the MCP3008 ADC chip
 * for reading analog sensor values. It handles SPI communication
 * and converts raw ADC values to meaningful pressure readings.
 */
class MCP3008 : public QObject
{
    Q_OBJECT

public:
    explicit MCP3008(QObject *parent = nullptr);
    ~MCP3008();

    // Initialization
    bool initialize(int spiChannel = 0, int spiSpeed = 1000000);
    void shutdown();
    bool isReady() const { return m_initialized; }

    // ADC reading functions
    uint16_t readRawValue(int channel);           // Returns raw 10-bit value (0-1023)
    double readVoltage(int channel);              // Sensor output volts (channels 0-2: divider undone, 0-4.95 V)
    double readPressure(int channel);             // Returns pressure in mmHg
    
    // Calibration functions
    void calibrateChannel(int channel, double zeroVoltage, double fullScaleVoltage, 
                         double zeroPressure, double fullScalePressure);
    void setChannelCalibration(int channel, double slope, double offset);
    
    // Channel validation
    bool isValidChannel(int channel) const { return channel >= 0 && channel < 8; }

    /// The three vacuum sensors (channels 0-2) put out 0.2-4.7 V from a 5 V supply; the MCP3008
    /// runs at 3.3 V, so each goes through a 10k (top) / 20k (bottom) divider: ADC volts = sensor volts x 2/3.
    static constexpr double PRESSURE_DIVIDER_RATIO = 20.0 / 30.0;
    /// A reading at or above this sits at the 3.3 V top: divider missing, sensor fault, or far over range.
    static constexpr uint16_t RAIL_RAW = 1020;
    static bool isPressureChannel(int channel) { return channel >= 0 && channel <= 2; }
    /// The sensor's own output voltage for a raw reading (undoes the divider on channels 0-2)
    static double sensorVolts(int channel, uint16_t raw);
    /// Vacuum in mmHg (0 = room air) from a raw reading; -1 (error) when the input sits at the 3.3 V top
    static double pressureFromRaw(int channel, uint16_t raw, double slope, double offset);
    
    // Error handling
    QString getLastError() const { return m_lastError; }

Q_SIGNALS:
    void readingError(int channel, const QString& error);

private:
    struct ChannelCalibration {
        double slope;      // Conversion slope (mmHg per volt)
        double offset;     // Zero offset (mmHg)
        bool calibrated;   // Whether this channel is calibrated
        
        ChannelCalibration() : slope(1.0), offset(0.0), calibrated(false) {}
    };

    bool initializeSPI();
    uint16_t spiTransfer(uint8_t channel);

    // SPI communication
    int m_spiFd;         // SPI file descriptor
    int m_spiChannel;
    int m_spiSpeed;
    bool m_initialized;
    mutable QMutex m_spiMutex;
    
    // Calibration data for each channel
    ChannelCalibration m_calibration[8];
    
    // Error tracking
    QString m_lastError;
    
    // Constants
    static const double REFERENCE_VOLTAGE;  // 3.3V reference
    static const int ADC_RESOLUTION;        // 10-bit = 1024 levels
    static const int MAX_CHANNELS;          // 8 channels
    
    // Default calibration for MPX5010DP sensors
    // These sensors output 0.2V at 0 kPa and 4.7V at 10 kPa
    // Converting to mmHg: 10 kPa = 75 mmHg
    static const double DEFAULT_SLOPE_MMHG_PER_VOLT;
    static const double DEFAULT_OFFSET_MMHG;
};

#endif // MCP3008_H
