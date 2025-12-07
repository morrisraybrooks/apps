#include "FluidSensor.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <numeric>

#ifdef __linux__
#include <wiringPi.h>
#else
// Stub definitions for non-Linux builds
#define INPUT 0
#define OUTPUT 1
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return 0; }
inline void delayMicroseconds(unsigned int) {}
#endif

FluidSensor::FluidSensor(SensorType type, QObject* parent)
    : QObject(parent)
    , m_sensorType(type)
    , m_initialized(false)
    , m_hasSensorSignal(false)
    , m_signalQuality(0)
    , m_gpioData(DEFAULT_HX711_DATA_GPIO)
    , m_gpioClock(DEFAULT_HX711_CLOCK_GPIO)
    , m_hx711Gain(128)
    , m_updateTimer(new QTimer(this))
    , m_currentVolumeMl(0.0)
    , m_cumulativeVolumeMl(0.0)
    , m_lubricationVolumeMl(0.0)
    , m_orgasmicVolumeMl(0.0)
    , m_tareOffset(0.0)
    , m_calibrationFactor(1.0)
    , m_fluidDensity(DEFAULT_DENSITY)
    , m_flowRateMlPerMin(0.0)
    , m_instantFlowMlPerSec(0.0)
    , m_lastVolumeMl(0.0)
    , m_lastFlowUpdateMs(0)
    , m_historyIndex(0)
    , m_burstDetected(false)
    , m_burstThreshold(DEFAULT_BURST_THRESHOLD)
    , m_currentOrgasmNumber(-1)
    , m_currentArousalLevel(0.0)
    , m_reservoirCapacity(DEFAULT_CAPACITY)
    , m_overflowWarningMl(DEFAULT_OVERFLOW_WARNING)
    , m_overflowCriticalMl(DEFAULT_OVERFLOW_CRITICAL)
    , m_overflowWarningIssued(false)
    , m_filteredValue(0.0)
    , m_filterAlpha(FILTER_ALPHA)
    , m_sessionActive(false)
{
    m_volumeHistory.resize(VOLUME_HISTORY_SIZE, 0.0);
    m_lastEvent = {FluidEventType::LUBRICATION, 0.0, 0.0, 0, -1};
    
    connect(m_updateTimer, &QTimer::timeout, this, &FluidSensor::onUpdateTick);
    
    qDebug() << "FluidSensor created with type:" << static_cast<int>(type);
}

FluidSensor::~FluidSensor()
{
    shutdown();
}

bool FluidSensor::initialize()
{
    if (m_sensorType == SensorType::LOAD_CELL_HX711) {
        return initializeHX711(m_gpioData, m_gpioClock);
    } else if (m_sensorType == SensorType::SIMULATED) {
        m_initialized = true;
        m_hasSensorSignal = true;
        m_signalQuality = 100;
        m_updateTimer->start(UPDATE_INTERVAL_MS);
        qDebug() << "FluidSensor initialized in simulation mode";
        return true;
    }
    
    m_lastError = "Unsupported sensor type";
    return false;
}

bool FluidSensor::initializeHX711(int gpioData, int gpioClock)
{
    QMutexLocker locker(&m_mutex);
    
    m_gpioData = gpioData;
    m_gpioClock = gpioClock;
    
#ifdef __linux__
    // Configure GPIO pins
    pinMode(m_gpioData, INPUT);
    pinMode(m_gpioClock, OUTPUT);
    digitalWrite(m_gpioClock, 0);
    
    // Wait for HX711 to be ready
    int timeout = 100;
    while (digitalRead(m_gpioData) == 1 && timeout > 0) {
        delayMicroseconds(100);
        timeout--;
    }
    
    if (timeout == 0) {
        m_lastError = "HX711 not responding (DATA line stuck high)";
        qWarning() << m_lastError;
        return false;
    }
    
    // Set gain (128 for channel A)
    setHX711Gain(128);
    
    // Take a few readings to stabilize
    for (int i = 0; i < 5; i++) {
        readHX711Raw();
    }
#endif
    
    m_initialized = true;
    m_hasSensorSignal = true;
    m_signalQuality = 100;
    
    // Start update timer
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    m_flowTimer.start();
    
    qDebug() << "FluidSensor HX711 initialized on GPIO" << gpioData << "/" << gpioClock;
    return true;
}

void FluidSensor::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_updateTimer->isActive()) {
        m_updateTimer->stop();
    }
    
    m_initialized = false;
    m_hasSensorSignal = false;
    
    qDebug() << "FluidSensor shutdown";
}

// ============================================================================
// Volume Measurements
// ============================================================================

double FluidSensor::getCurrentVolumeMl() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentVolumeMl;
}

double FluidSensor::getCumulativeVolumeMl() const
{
    QMutexLocker locker(&m_mutex);
    return m_cumulativeVolumeMl;
}

double FluidSensor::getLubricationVolumeMl() const
{
    QMutexLocker locker(&m_mutex);
    return m_lubricationVolumeMl;
}

double FluidSensor::getOrgasmicVolumeMl() const
{
    QMutexLocker locker(&m_mutex);
    return m_orgasmicVolumeMl;
}

double FluidSensor::getFlowRateMlPerMin() const
{
    QMutexLocker locker(&m_mutex);
    return m_flowRateMlPerMin;
}

double FluidSensor::getInstantFlowRateMlPerSec() const
{
    QMutexLocker locker(&m_mutex);
    return m_instantFlowMlPerSec;
}

double FluidSensor::getLubricationScore() const
{
    QMutexLocker locker(&m_mutex);
    // Normalize lubrication rate to 0-1 (2 mL/min = 1.0)
    return std::min(1.0, m_flowRateMlPerMin / 2.0);
}

double FluidSensor::getArousalLubricationRate() const
{
    QMutexLocker locker(&m_mutex);
    // Expected lubrication rate based on arousal level
    // Model: rate = 0.1 + 1.5 * arousal^2 (mL/min)
    return 0.1 + 1.5 * m_currentArousalLevel * m_currentArousalLevel;
}

double FluidSensor::getReservoirPercent() const
{
    QMutexLocker locker(&m_mutex);
    return (m_currentVolumeMl / m_reservoirCapacity) * 100.0;
}

bool FluidSensor::isOrgasmicBurstDetected() const
{
    QMutexLocker locker(&m_mutex);
    return m_burstDetected;
}

FluidSensor::FluidEvent FluidSensor::getLastEvent() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastEvent;
}

QVector<FluidSensor::FluidEvent> FluidSensor::getSessionEvents() const
{
    QMutexLocker locker(&m_mutex);
    return m_sessionEvents;
}

// ============================================================================
// Calibration
// ============================================================================

void FluidSensor::tare()
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized) return;

    // Average several readings for tare
    double sum = 0.0;
    int samples = 10;

    for (int i = 0; i < samples; i++) {
        if (m_sensorType == SensorType::LOAD_CELL_HX711) {
            sum += readHX711Raw();
        } else {
            sum += m_filteredValue;
        }
    }

    m_tareOffset = sum / samples;
    m_currentVolumeMl = 0.0;

    qDebug() << "FluidSensor tare complete, offset:" << m_tareOffset;
    emit tareComplete();
}

void FluidSensor::calibrate(double knownMassGrams)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || knownMassGrams <= 0) return;

    // Read current raw value
    double rawValue = 0.0;
    int samples = 10;

    for (int i = 0; i < samples; i++) {
        if (m_sensorType == SensorType::LOAD_CELL_HX711) {
            rawValue += readHX711Raw();
        } else {
            rawValue += m_filteredValue;
        }
    }
    rawValue /= samples;

    // Calculate calibration factor
    double rawDelta = rawValue - m_tareOffset;
    if (std::abs(rawDelta) > 100) {  // Minimum delta to avoid division issues
        m_calibrationFactor = knownMassGrams / rawDelta;
        qDebug() << "FluidSensor calibrated: factor=" << m_calibrationFactor
                 << "for" << knownMassGrams << "g";
        emit calibrationComplete(m_calibrationFactor);
    } else {
        m_lastError = "Calibration failed: insufficient weight change";
        qWarning() << m_lastError;
    }
}

void FluidSensor::setFluidDensity(double gramsPerMl)
{
    QMutexLocker locker(&m_mutex);
    m_fluidDensity = std::max(0.5, std::min(2.0, gramsPerMl));
}

void FluidSensor::setCurrentArousalLevel(double level)
{
    QMutexLocker locker(&m_mutex);
    m_currentArousalLevel = std::max(0.0, std::min(1.0, level));
}

void FluidSensor::recordOrgasmEvent(int orgasmNumber)
{
    QMutexLocker locker(&m_mutex);
    m_currentOrgasmNumber = orgasmNumber;
    // The next detected burst will be associated with this orgasm
}

// ============================================================================
// Configuration
// ============================================================================

void FluidSensor::setUpdateRate(int hz)
{
    QMutexLocker locker(&m_mutex);
    int intervalMs = 1000 / std::max(1, std::min(100, hz));
    m_updateTimer->setInterval(intervalMs);
}

void FluidSensor::setBurstThreshold(double mlPerSec)
{
    QMutexLocker locker(&m_mutex);
    m_burstThreshold = std::max(0.5, mlPerSec);
}

void FluidSensor::setOverflowWarning(double ml)
{
    QMutexLocker locker(&m_mutex);
    m_overflowWarningMl = ml;
}

void FluidSensor::setOverflowCritical(double ml)
{
    QMutexLocker locker(&m_mutex);
    m_overflowCriticalMl = ml;
}

// ============================================================================
// Session Management
// ============================================================================

void FluidSensor::startSession()
{
    QMutexLocker locker(&m_mutex);

    m_sessionActive = true;
    m_cumulativeVolumeMl = 0.0;
    m_lubricationVolumeMl = 0.0;
    m_orgasmicVolumeMl = 0.0;
    m_currentOrgasmNumber = -1;
    m_sessionEvents.clear();
    m_overflowWarningIssued = false;

    m_sessionTimer.start();

    qDebug() << "FluidSensor session started";
}

void FluidSensor::endSession()
{
    QMutexLocker locker(&m_mutex);

    if (!m_sessionActive) return;

    m_sessionActive = false;

    int orgasmBursts = 0;
    for (const auto& event : m_sessionEvents) {
        if (event.type == FluidEventType::ORGASMIC_BURST ||
            event.type == FluidEventType::SQUIRT) {
            orgasmBursts++;
        }
    }

    qDebug() << "FluidSensor session ended: total=" << m_cumulativeVolumeMl
             << "mL, bursts=" << orgasmBursts;

    emit sessionEnded(m_cumulativeVolumeMl, orgasmBursts);
}

void FluidSensor::resetSession()
{
    endSession();

    QMutexLocker locker(&m_mutex);
    m_currentVolumeMl = 0.0;
    m_cumulativeVolumeMl = 0.0;
    m_lubricationVolumeMl = 0.0;
    m_orgasmicVolumeMl = 0.0;
    m_flowRateMlPerMin = 0.0;
    m_instantFlowMlPerSec = 0.0;
    m_volumeHistory.fill(0.0);
    m_historyIndex = 0;
}

// ============================================================================
// Update Loop
// ============================================================================

void FluidSensor::onUpdateTick()
{
    if (!m_initialized) return;

    double rawValue = 0.0;

    if (m_sensorType == SensorType::LOAD_CELL_HX711) {
        rawValue = readHX711Raw();
    } else if (m_sensorType == SensorType::SIMULATED) {
        simulateReading();
        return;  // simulateReading handles everything
    }

    processReading(rawValue);
}

void FluidSensor::processReading(double rawValue)
{
    QMutexLocker locker(&m_mutex);

    // Apply filtering
    double filteredRaw = applyFilter(rawValue);

    // Convert to mass (grams)
    double massGrams = (filteredRaw - m_tareOffset) * m_calibrationFactor;

    // Convert to volume (mL)
    double newVolumeMl = massGrams / m_fluidDensity;

    // Clamp to valid range (can't be negative)
    newVolumeMl = std::max(0.0, newVolumeMl);

    // Store previous for delta calculation
    double prevVolume = m_currentVolumeMl;
    m_currentVolumeMl = newVolumeMl;

    // Update history
    m_volumeHistory[m_historyIndex] = newVolumeMl;
    m_historyIndex = (m_historyIndex + 1) % VOLUME_HISTORY_SIZE;

    // Calculate delta for cumulative tracking
    double deltaVolume = newVolumeMl - prevVolume;
    if (deltaVolume > 0 && m_sessionActive) {
        m_cumulativeVolumeMl += deltaVolume;
    }

    // Update flow rate
    updateFlowRate();

    // Detect fluid events
    detectFluidEvent();

    // Check overflow
    checkOverflow();

    // Emit signals
    emit volumeUpdated(m_currentVolumeMl, m_cumulativeVolumeMl);
    emit flowRateUpdated(m_flowRateMlPerMin, m_instantFlowMlPerSec);
}

void FluidSensor::updateFlowRate()
{
    qint64 now = m_flowTimer.elapsed();
    qint64 deltaTime = now - m_lastFlowUpdateMs;

    if (deltaTime >= 100) {  // Update every 100ms minimum
        double deltaVolume = m_currentVolumeMl - m_lastVolumeMl;

        // Instant flow rate (mL/sec)
        m_instantFlowMlPerSec = (deltaVolume / deltaTime) * 1000.0;

        // Average flow rate (mL/min) - use sliding window
        double sum = 0.0;
        int count = 0;
        for (int i = 0; i < VOLUME_HISTORY_SIZE - 1; i++) {
            int curr = (m_historyIndex - 1 - i + VOLUME_HISTORY_SIZE) % VOLUME_HISTORY_SIZE;
            int prev = (curr - 1 + VOLUME_HISTORY_SIZE) % VOLUME_HISTORY_SIZE;
            double delta = m_volumeHistory[curr] - m_volumeHistory[prev];
            if (delta > 0) {
                sum += delta;
                count++;
            }
        }

        // Convert to mL/min (samples are at 10 Hz = 0.1 sec each)
        if (count > 0) {
            m_flowRateMlPerMin = (sum / count) * 600.0;  // 0.1s samples → mL/min
        } else {
            m_flowRateMlPerMin = 0.0;
        }

        m_lastVolumeMl = m_currentVolumeMl;
        m_lastFlowUpdateMs = now;
    }
}

void FluidSensor::detectFluidEvent()
{
    // Reset burst flag each tick
    m_burstDetected = false;

    // Classify based on instant flow rate
    FluidEventType eventType = FluidEventType::LUBRICATION;

    if (m_instantFlowMlPerSec >= SQUIRT_MIN_RATE) {
        eventType = FluidEventType::SQUIRT;
        m_burstDetected = true;
    } else if (m_instantFlowMlPerSec >= ORGASMIC_MIN_RATE) {
        eventType = FluidEventType::ORGASMIC_BURST;
        m_burstDetected = true;
    } else if (m_instantFlowMlPerSec >= LUBRICATION_MAX_RATE) {
        eventType = FluidEventType::PRE_ORGASMIC;
    }

    // If burst detected, record event
    if (m_burstDetected && m_sessionActive) {
        FluidEvent event;
        event.type = eventType;
        event.volumeMl = m_instantFlowMlPerSec * 0.1;  // Volume in this tick
        event.peakFlowRate = m_instantFlowMlPerSec;
        event.timestampMs = m_sessionTimer.elapsed();
        event.orgasmNumber = m_currentOrgasmNumber;

        m_sessionEvents.append(event);
        m_lastEvent = event;

        // Accumulate to appropriate category
        if (eventType == FluidEventType::ORGASMIC_BURST ||
            eventType == FluidEventType::SQUIRT) {
            m_orgasmicVolumeMl += event.volumeMl;
            emit orgasmicBurstDetected(event.volumeMl, event.peakFlowRate,
                                       m_currentOrgasmNumber);
        }

        qDebug() << "FluidSensor event:" << static_cast<int>(eventType)
                 << "volume=" << event.volumeMl << "mL";
    } else if (m_instantFlowMlPerSec > 0 && m_sessionActive) {
        // Slow accumulation = lubrication
        m_lubricationVolumeMl += m_instantFlowMlPerSec * 0.1;
    }

    // Emit lubrication rate changes
    static double lastLubRate = 0.0;
    if (std::abs(m_flowRateMlPerMin - lastLubRate) > 0.1) {
        emit lubricationRateChanged(m_flowRateMlPerMin);
        lastLubRate = m_flowRateMlPerMin;
    }
}

void FluidSensor::checkOverflow()
{
    if (m_currentVolumeMl >= m_overflowCriticalMl) {
        emit overflowCritical(m_currentVolumeMl);
        qWarning() << "FluidSensor CRITICAL OVERFLOW:" << m_currentVolumeMl << "mL";
    } else if (m_currentVolumeMl >= m_overflowWarningMl && !m_overflowWarningIssued) {
        emit overflowWarning(m_currentVolumeMl, m_reservoirCapacity);
        m_overflowWarningIssued = true;
        qWarning() << "FluidSensor overflow warning:" << m_currentVolumeMl << "mL";
    } else if (m_currentVolumeMl < m_overflowWarningMl * 0.9) {
        m_overflowWarningIssued = false;  // Reset when below 90% of warning level
    }
}

double FluidSensor::applyFilter(double value)
{
    m_filteredValue = m_filterAlpha * value + (1.0 - m_filterAlpha) * m_filteredValue;
    return m_filteredValue;
}

// ============================================================================
// HX711 Communication
// ============================================================================

int FluidSensor::readHX711Raw()
{
#ifdef __linux__
    // Wait for HX711 to be ready (DATA goes low)
    int timeout = 100;
    while (digitalRead(m_gpioData) == 1 && timeout > 0) {
        delayMicroseconds(10);
        timeout--;
    }

    if (timeout == 0) {
        m_hasSensorSignal = false;
        emit sensorDisconnected();
        return 0;
    }

    m_hasSensorSignal = true;

    // Read 24 bits of data
    int value = 0;
    for (int i = 0; i < 24; i++) {
        digitalWrite(m_gpioClock, 1);
        delayMicroseconds(1);
        value = (value << 1) | digitalRead(m_gpioData);
        digitalWrite(m_gpioClock, 0);
        delayMicroseconds(1);
    }

    // Set gain for next reading (1-3 pulses)
    int gainPulses = 1;  // 128 gain
    if (m_hx711Gain == 64) gainPulses = 3;
    else if (m_hx711Gain == 32) gainPulses = 2;

    for (int i = 0; i < gainPulses; i++) {
        digitalWrite(m_gpioClock, 1);
        delayMicroseconds(1);
        digitalWrite(m_gpioClock, 0);
        delayMicroseconds(1);
    }

    // Convert from 24-bit two's complement
    if (value & 0x800000) {
        value |= 0xFF000000;  // Sign extend to 32 bits
    }

    return value;
#else
    return 0;
#endif
}

void FluidSensor::pulseHX711Clock()
{
#ifdef __linux__
    digitalWrite(m_gpioClock, 1);
    delayMicroseconds(1);
    digitalWrite(m_gpioClock, 0);
    delayMicroseconds(1);
#endif
}

void FluidSensor::setHX711Gain(int gain)
{
    if (gain == 128 || gain == 64 || gain == 32) {
        m_hx711Gain = gain;
        // Read once to set the gain
        readHX711Raw();
    }
}

// ============================================================================
// Simulation
// ============================================================================

void FluidSensor::simulateReading()
{
    QMutexLocker locker(&m_mutex);

    if (!m_sessionActive) {
        emit volumeUpdated(m_currentVolumeMl, m_cumulativeVolumeMl);
        return;
    }

    // Simulate lubrication based on arousal level
    // Model: rate = 0.1 + 1.5 * arousal^2 (mL/min)
    double baseRate = 0.1 + 1.5 * m_currentArousalLevel * m_currentArousalLevel;
    double ratePerTick = baseRate / 600.0;  // mL per 100ms tick

    // Add some randomness
    double noise = (static_cast<double>(rand()) / RAND_MAX - 0.5) * ratePerTick * 0.2;
    double delta = std::max(0.0, ratePerTick + noise);

    // Accumulate
    m_currentVolumeMl += delta;
    m_cumulativeVolumeMl += delta;
    m_lubricationVolumeMl += delta;

    // Update history
    m_volumeHistory[m_historyIndex] = m_currentVolumeMl;
    m_historyIndex = (m_historyIndex + 1) % VOLUME_HISTORY_SIZE;

    // Calculate flow rate
    m_instantFlowMlPerSec = delta * 10.0;  // 10 ticks per second
    m_flowRateMlPerMin = baseRate;

    checkOverflow();

    emit volumeUpdated(m_currentVolumeMl, m_cumulativeVolumeMl);
    emit flowRateUpdated(m_flowRateMlPerMin, m_instantFlowMlPerSec);
}

void FluidSensor::simulateOrgasmBurst(double volumeMl)
{
    QMutexLocker locker(&m_mutex);

    if (!m_sessionActive) return;

    // Add burst volume
    m_currentVolumeMl += volumeMl;
    m_cumulativeVolumeMl += volumeMl;
    m_orgasmicVolumeMl += volumeMl;

    // Create event
    FluidEvent event;
    event.type = volumeMl >= SQUIRT_MIN_VOLUME ?
                 FluidEventType::SQUIRT : FluidEventType::ORGASMIC_BURST;
    event.volumeMl = volumeMl;
    event.peakFlowRate = volumeMl / 0.5;  // Assume 0.5 sec burst
    event.timestampMs = m_sessionTimer.elapsed();
    event.orgasmNumber = m_currentOrgasmNumber;

    m_sessionEvents.append(event);
    m_lastEvent = event;
    m_burstDetected = true;

    qDebug() << "FluidSensor simulated burst:" << volumeMl << "mL for orgasm"
             << m_currentOrgasmNumber;

    emit orgasmicBurstDetected(volumeMl, event.peakFlowRate, m_currentOrgasmNumber);
    emit volumeUpdated(m_currentVolumeMl, m_cumulativeVolumeMl);
}