#include "MCP3008.h"
#include <QDebug>
#include <QMutexLocker>
#include <wiringPiSPI.h>
#include <wiringPi.h>
#include <stdexcept>
#include <cmath>

// Constants
const double MCP3008::REFERENCE_VOLTAGE = 3.3;
const int MCP3008::ADC_RESOLUTION = 1024;
const int MCP3008::MAX_CHANNELS = 8;

// MPX5010DP sensor calibration (0.2V @ 0kPa, 4.7V @ 10kPa)
// 10 kPa = 75 mmHg, so slope = 75 mmHg / (4.7V - 0.2V) = 16.67 mmHg/V
const double MCP3008::DEFAULT_SLOPE_MMHG_PER_VOLT = 16.67;
const double MCP3008::DEFAULT_OFFSET_MMHG = -3.33;  // Offset to zero at 0.2V

MCP3008::MCP3008(QObject *parent)
    : QObject(parent)
    , m_spiHandle(-1)
    , m_spiChannel(0)
    , m_spiSpeed(1000000)
    , m_initialized(false)
{
    // Initialize default calibration for all channels
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        m_calibration[i].slope = DEFAULT_SLOPE_MMHG_PER_VOLT;
        m_calibration[i].offset = DEFAULT_OFFSET_MMHG;
        m_calibration[i].calibrated = true;  // Use default calibration
    }
}

MCP3008::~MCP3008()
{
    shutdown();
}

bool MCP3008::initialize(int spiChannel, int spiSpeed)
{
    try {
        m_spiChannel = spiChannel;
        m_spiSpeed = spiSpeed;
        
        if (!initializeSPI()) {
            m_lastError = "Failed to initialize SPI communication";
            return false;
        }
        
        // Test communication by reading a channel
        uint16_t testValue = readRawValue(0);
        if (testValue == 0xFFFF) {  // Error value
            m_lastError = "Failed to communicate with MCP3008";
            return false;
        }
        
        m_initialized = true;
        qDebug() << "MCP3008 initialized successfully on SPI channel" << spiChannel;
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("MCP3008 initialization error: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void MCP3008::shutdown()
{
    if (m_initialized) {
        // No specific shutdown needed for SPI
        m_initialized = false;
        qDebug() << "MCP3008 shutdown complete";
    }
}

uint16_t MCP3008::readRawValue(int channel)
{
    if (!m_initialized) {
        m_lastError = "MCP3008 not initialized";
        emit readingError(channel, m_lastError);
        return 0xFFFF;
    }
    
    if (!isValidChannel(channel)) {
        m_lastError = QString("Invalid channel: %1").arg(channel);
        emit readingError(channel, m_lastError);
        return 0xFFFF;
    }
    
    QMutexLocker locker(&m_spiMutex);
    
    try {
        return spiTransfer(static_cast<uint8_t>(channel));
        
    } catch (const std::exception& e) {
        m_lastError = QString("SPI transfer error on channel %1: %2").arg(channel).arg(e.what());
        emit readingError(channel, m_lastError);
        return 0xFFFF;
    }
}

double MCP3008::readVoltage(int channel)
{
    uint16_t rawValue = readRawValue(channel);
    if (rawValue == 0xFFFF) {
        return -1.0;  // Error value
    }
    
    return convertToVoltage(rawValue);
}

double MCP3008::readPressure(int channel)
{
    double voltage = readVoltage(channel);
    if (voltage < 0) {
        return -1.0;  // Error value
    }
    
    return convertToPressure(channel, voltage);
}

void MCP3008::calibrateChannel(int channel, double zeroVoltage, double fullScaleVoltage,
                              double zeroPressure, double fullScalePressure)
{
    if (!isValidChannel(channel)) {
        qWarning() << "Invalid channel for calibration:" << channel;
        return;
    }
    
    if (std::abs(fullScaleVoltage - zeroVoltage) < 0.001) {
        qWarning() << "Invalid calibration voltages for channel" << channel;
        return;
    }
    
    double slope = (fullScalePressure - zeroPressure) / (fullScaleVoltage - zeroVoltage);
    double offset = zeroPressure - (slope * zeroVoltage);
    
    setChannelCalibration(channel, slope, offset);
    
    qDebug() << QString("Channel %1 calibrated: slope=%2 mmHg/V, offset=%3 mmHg")
                .arg(channel).arg(slope, 0, 'f', 2).arg(offset, 0, 'f', 2);
}

void MCP3008::setChannelCalibration(int channel, double slope, double offset)
{
    if (isValidChannel(channel)) {
        m_calibration[channel].slope = slope;
        m_calibration[channel].offset = offset;
        m_calibration[channel].calibrated = true;
    }
}

bool MCP3008::initializeSPI()
{
    // Initialize wiringPi SPI
    m_spiHandle = wiringPiSPISetup(m_spiChannel, m_spiSpeed);
    if (m_spiHandle < 0) {
        qCritical() << "Failed to initialize SPI channel" << m_spiChannel;
        return false;
    }
    
    qDebug() << "SPI initialized on channel" << m_spiChannel << "at" << m_spiSpeed << "Hz";
    return true;
}

uint16_t MCP3008::spiTransfer(uint8_t channel)
{
    // MCP3008 SPI protocol:
    // Send: [start bit][single/diff][channel][don't care bits]
    // Receive: [don't care][null bit][data bits 9-0]
    
    uint8_t buffer[3];
    
    // Build command: start bit (1) + single-ended (1) + channel (3 bits)
    buffer[0] = 0x01;  // Start bit
    buffer[1] = (0x80) | (channel << 4);  // Single-ended + channel
    buffer[2] = 0x00;  // Don't care
    
    // Perform SPI transfer
    if (wiringPiSPIDataRW(m_spiChannel, buffer, 3) < 0) {
        throw std::runtime_error("SPI transfer failed");
    }
    
    // Extract 10-bit result
    uint16_t result = ((buffer[1] & 0x03) << 8) | buffer[2];
    
    return result;
}

double MCP3008::convertToVoltage(uint16_t rawValue)
{
    return (static_cast<double>(rawValue) * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
}

double MCP3008::convertToPressure(int channel, double voltage)
{
    if (!isValidChannel(channel) || !m_calibration[channel].calibrated) {
        return -1.0;  // Error value
    }
    
    double pressure = (voltage * m_calibration[channel].slope) + m_calibration[channel].offset;
    
    // Ensure pressure is not negative (vacuum systems)
    return std::max(0.0, pressure);
}
