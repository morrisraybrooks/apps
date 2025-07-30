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
    double readVoltage(int channel);              // Returns voltage (0-3.3V)
    double readPressure(int channel);             // Returns pressure in mmHg
    
    // Calibration functions
    void calibrateChannel(int channel, double zeroVoltage, double fullScaleVoltage, 
                         double zeroPressure, double fullScalePressure);
    void setChannelCalibration(int channel, double slope, double offset);
    
    // Channel validation
    bool isValidChannel(int channel) const { return channel >= 0 && channel < 8; }
    
    // Error handling
    QString getLastError() const { return m_lastError; }

signals:
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
    double convertToVoltage(uint16_t rawValue);
    double convertToPressure(int channel, double voltage);

    // SPI communication
    int m_spiHandle;
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
