#ifndef FLUIDSENSOR_H
#define FLUIDSENSOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <memory>

/**
 * @brief Fluid Collection Sensor for measuring arousal lubrication and orgasmic fluid
 * 
 * Supports multiple sensor types:
 * - Load cell (HX711 ADC) - Primary recommended sensor
 * - Capacitive level sensor (I2C or analog)
 * - Simulated mode for testing
 * 
 * Measures:
 * - Current reservoir volume (mL)
 * - Cumulative session volume (mL)
 * - Flow rate (mL/min and mL/sec)
 * - Orgasmic burst detection
 * - Lubrication rate correlation with arousal
 * 
 * Hardware: TAL220 load cell → HX711 ADC → GPIO 26 (DT), GPIO 19 (SCK)
 */
class FluidSensor : public QObject
{
    Q_OBJECT

public:
    // Sensor types
    enum class SensorType {
        LOAD_CELL_HX711,      // TAL220 + HX711 ADC (recommended)
        CAPACITIVE_I2C,       // Capacitive level sensor via I2C
        CAPACITIVE_ANALOG,    // Capacitive sensor via MCP3008 ADC
        SIMULATED             // For testing without hardware
    };
    Q_ENUM(SensorType)

    // Fluid event types
    enum class FluidEventType {
        LUBRICATION,          // Slow steady accumulation
        PRE_ORGASMIC,         // Moderate increase before orgasm
        ORGASMIC_BURST,       // Rapid burst during orgasm
        SQUIRT                // Large volume rapid expulsion
    };
    Q_ENUM(FluidEventType)

    // Fluid event data structure
    struct FluidEvent {
        FluidEventType type;
        double volumeMl;
        double peakFlowRate;    // mL/sec at peak
        qint64 timestampMs;
        int orgasmNumber;       // -1 if not associated with orgasm
    };

    explicit FluidSensor(SensorType type = SensorType::LOAD_CELL_HX711,
                         QObject* parent = nullptr);
    ~FluidSensor();

    // Initialization
    bool initialize();
    bool initializeHX711(int gpioData = 26, int gpioClock = 19);
    void shutdown();
    bool isReady() const { return m_initialized; }

    // Volume measurements
    double getCurrentVolumeMl() const;           // Current in reservoir
    double getCumulativeVolumeMl() const;        // Total session (includes emptied)
    double getLubricationVolumeMl() const;       // Slow accumulation only
    double getOrgasmicVolumeMl() const;          // Fast bursts only

    // Flow rate measurements
    double getFlowRateMlPerMin() const;          // Averaged rate
    double getInstantFlowRateMlPerSec() const;   // Instant burst detection

    // Arousal correlation
    double getLubricationScore() const;          // 0.0-1.0 normalized
    double getArousalLubricationRate() const;    // Expected rate at arousal level

    // Event detection
    bool isOrgasmicBurstDetected() const;        // Recent burst?
    FluidEvent getLastEvent() const;
    QVector<FluidEvent> getSessionEvents() const;

    // Calibration
    void tare();                                  // Zero with empty container
    void calibrate(double knownMassGrams);       // Calibrate with known weight
    void setFluidDensity(double gramsPerMl);     // Default 1.0 g/mL
    double getCalibrationFactor() const { return m_calibrationFactor; }

    // Correlation with arousal/orgasm
    void setCurrentArousalLevel(double level);   // 0.0-1.0
    void recordOrgasmEvent(int orgasmNumber);    // Mark orgasm for correlation

    // Configuration
    void setUpdateRate(int hz);                  // Default 10 Hz
    void setBurstThreshold(double mlPerSec);     // Default 2.0 mL/sec
    void setOverflowWarning(double ml);          // Default 120 mL
    void setOverflowCritical(double ml);         // Default 140 mL

    // Status
    bool hasSensorSignal() const { return m_hasSensorSignal; }
    int getSignalQuality() const { return m_signalQuality; }
    double getReservoirCapacity() const { return m_reservoirCapacity; }
    double getReservoirPercent() const;
    QString getLastError() const { return m_lastError; }

    // Session management
    void startSession();
    void endSession();
    void resetSession();

Q_SIGNALS:
    void volumeUpdated(double currentMl, double cumulativeMl);
    void flowRateUpdated(double mlPerMin, double mlPerSec);
    void orgasmicBurstDetected(double volumeMl, double peakFlowRate, int orgasmNumber);
    void lubricationRateChanged(double mlPerMin);
    void overflowWarning(double currentMl, double capacityMl);
    void overflowCritical(double currentMl);
    void sensorError(const QString& error);
    void sensorDisconnected();
    void calibrationComplete(double factor);
    void tareComplete();
    void sessionEnded(double totalMl, int orgasmBursts);

private Q_SLOTS:
    void onUpdateTick();

private:
    // HX711 communication
    int readHX711Raw();
    void pulseHX711Clock();
    void setHX711Gain(int gain);  // 128, 64, or 32

    // Signal processing
    void processReading(double rawValue);
    void detectFluidEvent();
    void updateFlowRate();
    void checkOverflow();
    double applyFilter(double value);

    // Simulation
    void simulateReading();
    void simulateOrgasmBurst(double volumeMl);

    // State
    SensorType m_sensorType;
    bool m_initialized;
    bool m_hasSensorSignal;
    int m_signalQuality;
    QString m_lastError;
    mutable QMutex m_mutex;

    // HX711 GPIO pins
    int m_gpioData;
    int m_gpioClock;
    int m_hx711Gain;

    // Timing
    QTimer* m_updateTimer;
    QElapsedTimer m_sessionTimer;
    QElapsedTimer m_flowTimer;

    // Volume tracking
    double m_currentVolumeMl;        // Current in reservoir
    double m_cumulativeVolumeMl;     // Total session
    double m_lubricationVolumeMl;    // Slow accumulation
    double m_orgasmicVolumeMl;       // Fast bursts
    double m_tareOffset;             // Zero offset
    double m_calibrationFactor;      // Raw to grams
    double m_fluidDensity;           // g/mL (default 1.0)

    // Flow rate tracking
    double m_flowRateMlPerMin;
    double m_instantFlowMlPerSec;
    double m_lastVolumeMl;
    qint64 m_lastFlowUpdateMs;
    QVector<double> m_volumeHistory;
    int m_historyIndex;

    // Event detection
    bool m_burstDetected;
    double m_burstThreshold;         // mL/sec for burst detection
    int m_currentOrgasmNumber;
    double m_currentArousalLevel;
    QVector<FluidEvent> m_sessionEvents;
    FluidEvent m_lastEvent;

    // Overflow protection
    double m_reservoirCapacity;
    double m_overflowWarningMl;
    double m_overflowCriticalMl;
    bool m_overflowWarningIssued;

    // Filtering
    double m_filteredValue;
    double m_filterAlpha;

    // Session state
    bool m_sessionActive;

    // Constants
    static const int UPDATE_INTERVAL_MS = 100;       // 10 Hz default
    static const int VOLUME_HISTORY_SIZE = 100;      // 10 seconds at 10 Hz
    static const int HX711_SAMPLES_TO_AVERAGE = 3;

    // Default values
    static constexpr double DEFAULT_DENSITY = 1.0;           // g/mL (water-like)
    static constexpr double DEFAULT_BURST_THRESHOLD = 2.0;   // mL/sec
    static constexpr double DEFAULT_CAPACITY = 150.0;        // mL
    static constexpr double DEFAULT_OVERFLOW_WARNING = 120.0;
    static constexpr double DEFAULT_OVERFLOW_CRITICAL = 140.0;
    static constexpr double FILTER_ALPHA = 0.3;

    // Flow rate thresholds
    static constexpr double LUBRICATION_MAX_RATE = 0.5;      // mL/sec
    static constexpr double PRE_ORGASMIC_MAX_RATE = 2.0;     // mL/sec
    static constexpr double ORGASMIC_MIN_RATE = 2.0;         // mL/sec
    static constexpr double SQUIRT_MIN_RATE = 10.0;          // mL/sec
    static constexpr double SQUIRT_MIN_VOLUME = 30.0;        // mL

    // GPIO definitions for HX711
    static const int DEFAULT_HX711_DATA_GPIO = 26;
    static const int DEFAULT_HX711_CLOCK_GPIO = 19;
};

#endif // FLUIDSENSOR_H

