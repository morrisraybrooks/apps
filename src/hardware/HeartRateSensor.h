#ifndef HEARTRATESENSOR_H
#define HEARTRATESENSOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <QSerialPort>
#include <memory>

class MCP3008;

/**
 * @brief Heart Rate Sensor Interface for arousal detection
 * 
 * Supports multiple sensor types:
 * - Pulse oximeter (MAX30102) via I2C
 * - Analog pulse sensor via MCP3008 ADC
 * - Polar H10 chest strap via Bluetooth (external)
 * - Serial protocol sensors
 * 
 * Heart rate is a key physiological indicator of arousal:
 * - Resting: 60-80 BPM
 * - Early arousal: 80-100 BPM
 * - Plateau: 100-130 BPM  
 * - Pre-orgasm: 130-160 BPM
 * - Orgasm: 150-180+ BPM with HRV decrease
 * 
 * Also calculates Heart Rate Variability (HRV) which decreases during orgasm.
 */
class HeartRateSensor : public QObject
{
    Q_OBJECT

public:
    // Sensor types
    enum class SensorType {
        ANALOG_PULSE,      // Simple analog pulse sensor on ADC
        MAX30102_I2C,      // MAX30102 pulse oximeter via I2C
        POLAR_BLUETOOTH,   // Polar H10 via Bluetooth
        SERIAL_PROTOCOL,   // Generic serial heart rate monitor
        SIMULATED          // For testing without hardware
    };
    Q_ENUM(SensorType)

    // Heart rate zones for arousal mapping
    enum class HeartRateZone {
        RESTING,           // < 80 BPM
        ELEVATED,          // 80-100 BPM
        MODERATE,          // 100-130 BPM
        HIGH,              // 130-160 BPM
        PEAK               // > 160 BPM
    };
    Q_ENUM(HeartRateZone)

    explicit HeartRateSensor(SensorType type = SensorType::ANALOG_PULSE, 
                             QObject* parent = nullptr);
    ~HeartRateSensor();

    // Initialization
    bool initialize();
    bool initializeWithADC(MCP3008* adc, int channel = 3);  // For analog pulse sensor
    bool initializeSerial(const QString& portName, int baudRate = 9600);
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Heart rate readings
    int getCurrentBPM() const;                  // Current heart rate in BPM
    int getAverageBPM(int windowSeconds = 10) const;  // Average over window
    double getHeartRateVariability() const;     // RMSSD in milliseconds
    HeartRateZone getCurrentZone() const;
    
    // Arousal features
    double getHeartRateNormalized() const;      // 0.0-1.0 based on zone
    double getHRVNormalized() const;            // 0.0-1.0 (inverted - lower HRV = higher arousal)
    double getHeartRateAcceleration() const;    // Rate of BPM change
    bool isOrgasmSignature() const;             // Detect orgasm HR pattern
    
    // Raw data access
    QVector<int> getBPMHistory() const;
    QVector<double> getRRIntervals() const;     // R-R intervals in ms
    
    // Calibration
    void calibrateRestingHR(int durationSeconds = 60);
    void setRestingBPM(int bpm);
    void setMaxBPM(int bpm);                    // Usually 220 - age
    
    // Configuration
    void setFilteringEnabled(bool enabled);
    void setUpdateRate(int hz);                 // Default 10 Hz
    void setSensorType(SensorType type);
    
    // Status
    bool hasPulseSignal() const { return m_hasPulseSignal; }
    int getSignalQuality() const { return m_signalQuality; }  // 0-100%
    QString getLastError() const { return m_lastError; }

Q_SIGNALS:
    void heartRateUpdated(int bpm);
    void heartRateZoneChanged(HeartRateZone zone);
    void pulseDetected(qint64 timestampMs);
    void hrvUpdated(double rmssd);
    void signalLost();
    void signalRecovered();
    void sensorError(const QString& error);
    void calibrationProgress(int percent);
    void calibrationComplete(int restingBPM);

private Q_SLOTS:
    void onUpdateTick();
    void onSerialDataReady();

private:
    // Signal processing
    void processAnalogPulse(int adcValue);
    void processSerialData(const QByteArray& data);
    bool detectPeak(double value);
    void calculateBPM();
    void calculateHRV();
    void updateZone();
    
    // Filtering
    double applyBandpassFilter(double value);
    double applyLowPassFilter(double value, double alpha);
    
    // State
    SensorType m_sensorType;
    bool m_initialized;
    bool m_hasPulseSignal;
    int m_signalQuality;
    QString m_lastError;
    mutable QMutex m_mutex;
    
    // Hardware interfaces
    MCP3008* m_adc;
    int m_adcChannel;
    std::unique_ptr<QSerialPort> m_serialPort;
    
    // Timing
    QTimer* m_updateTimer;
    QElapsedTimer m_pulseTimer;
    qint64 m_lastPeakTime;
    
    // Heart rate data
    int m_currentBPM;
    double m_currentHRV;
    HeartRateZone m_currentZone;
    int m_restingBPM;
    int m_maxBPM;
    
    // History buffers
    QVector<int> m_bpmHistory;
    QVector<double> m_rrIntervals;      // R-R intervals in ms
    QVector<double> m_signalHistory;    // Raw signal for peak detection
    int m_historyIndex;

    // Peak detection state
    double m_filteredSignal;
    double m_threshold;
    bool m_inPeak;
    int m_peakCount;

    // Filtering state
    bool m_filteringEnabled;
    double m_dcOffset;
    double m_prevFilteredValue;

    // Constants
    static const int UPDATE_INTERVAL_MS = 100;      // 10 Hz default
    static const int BPM_HISTORY_SIZE = 60;         // 1 minute of history
    static const int RR_HISTORY_SIZE = 30;          // Last 30 R-R intervals
    static const int SIGNAL_HISTORY_SIZE = 200;     // 2 seconds at 100 Hz
    static const int MIN_VALID_BPM = 40;
    static const int MAX_VALID_BPM = 220;
    static const int DEFAULT_RESTING_BPM = 70;
    static const int DEFAULT_MAX_BPM = 180;

    // Zone thresholds
    static const int ZONE_ELEVATED_BPM = 80;
    static const int ZONE_MODERATE_BPM = 100;
    static const int ZONE_HIGH_BPM = 130;
    static const int ZONE_PEAK_BPM = 160;
};

#endif // HEARTRATESENSOR_H

