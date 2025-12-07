#include "HeartRateSensor.h"
#include "MCP3008.h"
#include <QDebug>
#include <algorithm>
#include <numeric>
#include <cmath>

HeartRateSensor::HeartRateSensor(SensorType type, QObject* parent)
    : QObject(parent)
    , m_sensorType(type)
    , m_initialized(false)
    , m_hasPulseSignal(false)
    , m_signalQuality(0)
    , m_adc(nullptr)
    , m_adcChannel(3)
    , m_updateTimer(new QTimer(this))
    , m_lastPeakTime(0)
    , m_currentBPM(0)
    , m_currentHRV(0.0)
    , m_currentZone(HeartRateZone::RESTING)
    , m_restingBPM(DEFAULT_RESTING_BPM)
    , m_maxBPM(DEFAULT_MAX_BPM)
    , m_historyIndex(0)
    , m_filteredSignal(0.0)
    , m_threshold(512.0)
    , m_inPeak(false)
    , m_peakCount(0)
    , m_filteringEnabled(true)
    , m_dcOffset(512.0)
    , m_prevFilteredValue(0.0)
{
    m_bpmHistory.resize(BPM_HISTORY_SIZE, 0);
    m_rrIntervals.resize(RR_HISTORY_SIZE, 0.0);
    m_signalHistory.resize(SIGNAL_HISTORY_SIZE, 0.0);
    
    m_pulseTimer.start();
    
    connect(m_updateTimer, &QTimer::timeout, this, &HeartRateSensor::onUpdateTick);
    
    qDebug() << "HeartRateSensor created with type:" << static_cast<int>(type);
}

HeartRateSensor::~HeartRateSensor()
{
    shutdown();
}

bool HeartRateSensor::initialize()
{
    if (m_sensorType == SensorType::SIMULATED) {
        m_initialized = true;
        m_hasPulseSignal = true;
        m_signalQuality = 100;
        m_currentBPM = m_restingBPM;
        m_updateTimer->start(UPDATE_INTERVAL_MS);
        qDebug() << "HeartRateSensor initialized in SIMULATED mode";
        return true;
    }
    
    m_lastError = "Sensor type requires specific initialization method";
    return false;
}

bool HeartRateSensor::initializeWithADC(MCP3008* adc, int channel)
{
    if (!adc) {
        m_lastError = "Invalid ADC pointer";
        return false;
    }
    
    m_adc = adc;
    m_adcChannel = channel;
    m_sensorType = SensorType::ANALOG_PULSE;
    m_initialized = true;
    
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    
    qDebug() << "HeartRateSensor initialized with ADC on channel" << channel;
    return true;
}

bool HeartRateSensor::initializeSerial(const QString& portName, int baudRate)
{
    m_serialPort = std::make_unique<QSerialPort>();
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    
    if (!m_serialPort->open(QIODevice::ReadOnly)) {
        m_lastError = QString("Failed to open serial port: %1").arg(m_serialPort->errorString());
        return false;
    }
    
    connect(m_serialPort.get(), &QSerialPort::readyRead, 
            this, &HeartRateSensor::onSerialDataReady);
    
    m_sensorType = SensorType::SERIAL_PROTOCOL;
    m_initialized = true;
    m_updateTimer->start(UPDATE_INTERVAL_MS);
    
    qDebug() << "HeartRateSensor initialized on serial port" << portName;
    return true;
}

void HeartRateSensor::shutdown()
{
    m_updateTimer->stop();
    
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    
    m_initialized = false;
    m_hasPulseSignal = false;
    
    qDebug() << "HeartRateSensor shutdown";
}

// ============================================================================
// Heart Rate Readings
// ============================================================================

int HeartRateSensor::getCurrentBPM() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentBPM;
}

int HeartRateSensor::getAverageBPM(int windowSeconds) const
{
    QMutexLocker locker(&m_mutex);
    
    int samplesToAverage = qMin(windowSeconds, BPM_HISTORY_SIZE);
    int sum = 0;
    int count = 0;
    
    for (int i = 0; i < samplesToAverage; ++i) {
        int idx = (m_historyIndex - i + BPM_HISTORY_SIZE) % BPM_HISTORY_SIZE;
        if (m_bpmHistory[idx] > 0) {
            sum += m_bpmHistory[idx];
            count++;
        }
    }
    
    return count > 0 ? sum / count : m_currentBPM;
}

double HeartRateSensor::getHeartRateVariability() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentHRV;
}

HeartRateSensor::HeartRateZone HeartRateSensor::getCurrentZone() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentZone;
}

// ============================================================================
// Arousal Features
// ============================================================================

double HeartRateSensor::getHeartRateNormalized() const
{
    QMutexLocker locker(&m_mutex);

    if (m_currentBPM <= m_restingBPM) return 0.0;
    if (m_currentBPM >= m_maxBPM) return 1.0;

    // Normalize between resting and max
    return static_cast<double>(m_currentBPM - m_restingBPM) /
           static_cast<double>(m_maxBPM - m_restingBPM);
}

double HeartRateSensor::getHRVNormalized() const
{
    QMutexLocker locker(&m_mutex);

    // HRV typically ranges from 20-100ms RMSSD
    // Lower HRV = higher arousal (inverted)
    // Normalize: 100ms = 0.0 arousal, 20ms = 1.0 arousal
    const double MIN_HRV = 20.0;
    const double MAX_HRV = 100.0;

    double clamped = qBound(MIN_HRV, m_currentHRV, MAX_HRV);
    return 1.0 - (clamped - MIN_HRV) / (MAX_HRV - MIN_HRV);
}

double HeartRateSensor::getHeartRateAcceleration() const
{
    QMutexLocker locker(&m_mutex);

    // Calculate rate of change over last 5 seconds
    if (m_bpmHistory.size() < 5) return 0.0;

    int currentIdx = m_historyIndex;
    int prevIdx = (m_historyIndex - 5 + BPM_HISTORY_SIZE) % BPM_HISTORY_SIZE;

    int currentBPM = m_bpmHistory[currentIdx];
    int prevBPM = m_bpmHistory[prevIdx];

    if (prevBPM <= 0) return 0.0;

    // Return BPM change per second
    return static_cast<double>(currentBPM - prevBPM) / 5.0;
}

bool HeartRateSensor::isOrgasmSignature() const
{
    QMutexLocker locker(&m_mutex);

    // Orgasm signature:
    // 1. Heart rate > 150 BPM
    // 2. HRV < 30ms (very low variability)
    // 3. Sustained for at least 5 seconds

    if (m_currentBPM < 150) return false;
    if (m_currentHRV > 30.0) return false;

    // Check if sustained
    int highHRCount = 0;
    for (int i = 0; i < 5; ++i) {
        int idx = (m_historyIndex - i + BPM_HISTORY_SIZE) % BPM_HISTORY_SIZE;
        if (m_bpmHistory[idx] >= 150) {
            highHRCount++;
        }
    }

    return highHRCount >= 4;  // At least 4 of last 5 readings
}

QVector<int> HeartRateSensor::getBPMHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_bpmHistory;
}

QVector<double> HeartRateSensor::getRRIntervals() const
{
    QMutexLocker locker(&m_mutex);
    return m_rrIntervals;
}

// ============================================================================
// Calibration
// ============================================================================

void HeartRateSensor::calibrateRestingHR(int durationSeconds)
{
    qDebug() << "Starting resting heart rate calibration for" << durationSeconds << "seconds";

    QVector<int> samples;
    QElapsedTimer calibTimer;
    calibTimer.start();

    while (calibTimer.elapsed() < durationSeconds * 1000) {
        if (m_currentBPM > MIN_VALID_BPM && m_currentBPM < MAX_VALID_BPM) {
            samples.append(m_currentBPM);
        }

        int progress = static_cast<int>((calibTimer.elapsed() * 100) / (durationSeconds * 1000));
        emit calibrationProgress(progress);

        QThread::msleep(1000);
    }

    if (!samples.isEmpty()) {
        // Use median for robustness
        std::sort(samples.begin(), samples.end());
        m_restingBPM = samples[samples.size() / 2];
    }

    emit calibrationComplete(m_restingBPM);
    qDebug() << "Resting HR calibration complete:" << m_restingBPM << "BPM";
}

void HeartRateSensor::setRestingBPM(int bpm)
{
    QMutexLocker locker(&m_mutex);
    m_restingBPM = qBound(MIN_VALID_BPM, bpm, 100);
}

void HeartRateSensor::setMaxBPM(int bpm)
{
    QMutexLocker locker(&m_mutex);
    m_maxBPM = qBound(150, bpm, MAX_VALID_BPM);
}

void HeartRateSensor::setFilteringEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_filteringEnabled = enabled;
}

void HeartRateSensor::setUpdateRate(int hz)
{
    m_updateTimer->setInterval(1000 / hz);
}

void HeartRateSensor::setSensorType(SensorType type)
{
    m_sensorType = type;
}

// ============================================================================
// Timer Callbacks
// ============================================================================

void HeartRateSensor::onUpdateTick()
{
    if (!m_initialized) return;

    switch (m_sensorType) {
        case SensorType::ANALOG_PULSE:
            if (m_adc) {
                int adcValue = m_adc->readChannel(m_adcChannel);
                processAnalogPulse(adcValue);
            }
            break;

        case SensorType::SIMULATED: {
            // Simulate realistic heart rate with some variation
            static double phase = 0.0;
            phase += 0.1;

            // Base rate + sinusoidal variation + noise
            double variation = 5.0 * sin(phase * 0.1) + (rand() % 3 - 1);
            m_currentBPM = static_cast<int>(m_restingBPM + variation);
            m_currentHRV = 50.0 + 10.0 * sin(phase * 0.05);

            m_bpmHistory[m_historyIndex] = m_currentBPM;
            m_historyIndex = (m_historyIndex + 1) % BPM_HISTORY_SIZE;

            updateZone();
            emit heartRateUpdated(m_currentBPM);
            break;
        }

        default:
            break;
    }
}

void HeartRateSensor::onSerialDataReady()
{
    if (!m_serialPort) return;

    QByteArray data = m_serialPort->readAll();
    processSerialData(data);
}

// ============================================================================
// Signal Processing
// ============================================================================

void HeartRateSensor::processAnalogPulse(int adcValue)
{
    // Store raw value
    m_signalHistory[m_historyIndex % SIGNAL_HISTORY_SIZE] = static_cast<double>(adcValue);

    // Apply bandpass filter (0.5-3 Hz for heart rate)
    double filtered = adcValue;
    if (m_filteringEnabled) {
        filtered = applyBandpassFilter(static_cast<double>(adcValue));
    }

    m_filteredSignal = filtered;

    // Peak detection
    if (detectPeak(filtered)) {
        qint64 currentTime = m_pulseTimer.elapsed();

        if (m_lastPeakTime > 0) {
            double rrInterval = static_cast<double>(currentTime - m_lastPeakTime);

            // Validate RR interval (corresponds to 40-220 BPM)
            if (rrInterval > 273 && rrInterval < 1500) {  // 273ms = 220 BPM, 1500ms = 40 BPM
                // Store RR interval
                int rrIdx = m_peakCount % RR_HISTORY_SIZE;
                m_rrIntervals[rrIdx] = rrInterval;
                m_peakCount++;

                calculateBPM();
                calculateHRV();

                m_hasPulseSignal = true;
                m_signalQuality = qMin(100, m_signalQuality + 10);

                emit pulseDetected(currentTime);
            }
        }

        m_lastPeakTime = currentTime;
    }

    // Decay signal quality if no pulses
    static qint64 lastQualityUpdate = 0;
    if (m_pulseTimer.elapsed() - lastQualityUpdate > 2000) {
        m_signalQuality = qMax(0, m_signalQuality - 5);
        lastQualityUpdate = m_pulseTimer.elapsed();

        if (m_signalQuality == 0 && m_hasPulseSignal) {
            m_hasPulseSignal = false;
            emit signalLost();
        }
    }

    // Store BPM in history (1 Hz)
    static qint64 lastHistoryUpdate = 0;
    if (m_pulseTimer.elapsed() - lastHistoryUpdate >= 1000) {
        m_bpmHistory[m_historyIndex] = m_currentBPM;
        m_historyIndex = (m_historyIndex + 1) % BPM_HISTORY_SIZE;
        lastHistoryUpdate = m_pulseTimer.elapsed();

        updateZone();
        emit heartRateUpdated(m_currentBPM);
    }
}

void HeartRateSensor::processSerialData(const QByteArray& data)
{
    // Parse common heart rate monitor protocols
    // Format 1: "HR:XXX\n" where XXX is BPM
    // Format 2: Binary with BPM in specific byte position

    QString str = QString::fromLatin1(data);

    if (str.contains("HR:")) {
        QRegExp rx("HR:(\\d+)");
        if (rx.indexIn(str) != -1) {
            int bpm = rx.cap(1).toInt();
            if (bpm >= MIN_VALID_BPM && bpm <= MAX_VALID_BPM) {
                m_currentBPM = bpm;
                m_bpmHistory[m_historyIndex] = bpm;
                m_historyIndex = (m_historyIndex + 1) % BPM_HISTORY_SIZE;

                m_hasPulseSignal = true;
                m_signalQuality = 100;

                updateZone();
                emit heartRateUpdated(m_currentBPM);
            }
        }
    }
}

bool HeartRateSensor::detectPeak(double value)
{
    // Adaptive threshold peak detection
    static double peakValue = 0.0;
    static double valleyValue = 1024.0;
    static bool rising = false;

    // Update DC offset (slow adaptation)
    m_dcOffset = 0.999 * m_dcOffset + 0.001 * value;

    // Adaptive threshold
    double amplitude = peakValue - valleyValue;
    m_threshold = valleyValue + amplitude * 0.6;

    // Track peaks and valleys
    if (value > peakValue) {
        peakValue = value;
    }
    if (value < valleyValue) {
        valleyValue = value;
    }

    // Decay towards current value
    peakValue = 0.99 * peakValue + 0.01 * m_dcOffset;
    valleyValue = 0.99 * valleyValue + 0.01 * m_dcOffset;

    // Detect rising edge crossing threshold
    bool wasBelowThreshold = !m_inPeak;
    m_inPeak = (value > m_threshold);

    return (m_inPeak && wasBelowThreshold);  // Rising edge detection
}

void HeartRateSensor::calculateBPM()
{
    // Calculate BPM from recent RR intervals
    int validIntervals = qMin(m_peakCount, RR_HISTORY_SIZE);
    if (validIntervals < 2) return;

    // Average last 5 RR intervals for smoothing
    int toAverage = qMin(5, validIntervals);
    double sum = 0.0;

    for (int i = 0; i < toAverage; ++i) {
        int idx = (m_peakCount - 1 - i + RR_HISTORY_SIZE) % RR_HISTORY_SIZE;
        sum += m_rrIntervals[idx];
    }

    double avgRR = sum / toAverage;

    // Convert RR interval (ms) to BPM
    if (avgRR > 0) {
        int bpm = static_cast<int>(60000.0 / avgRR);

        // Validate and smooth
        if (bpm >= MIN_VALID_BPM && bpm <= MAX_VALID_BPM) {
            // Exponential smoothing
            m_currentBPM = static_cast<int>(0.7 * m_currentBPM + 0.3 * bpm);
        }
    }
}

void HeartRateSensor::calculateHRV()
{
    // Calculate RMSSD (Root Mean Square of Successive Differences)
    // This is a standard HRV metric

    int validIntervals = qMin(m_peakCount, RR_HISTORY_SIZE);
    if (validIntervals < 3) return;

    double sumSquaredDiff = 0.0;
    int count = 0;

    for (int i = 1; i < validIntervals; ++i) {
        int idx1 = (m_peakCount - i + RR_HISTORY_SIZE) % RR_HISTORY_SIZE;
        int idx2 = (m_peakCount - i - 1 + RR_HISTORY_SIZE) % RR_HISTORY_SIZE;

        double diff = m_rrIntervals[idx1] - m_rrIntervals[idx2];
        sumSquaredDiff += diff * diff;
        count++;
    }

    if (count > 0) {
        double newHRV = sqrt(sumSquaredDiff / count);

        // Smooth HRV
        m_currentHRV = 0.8 * m_currentHRV + 0.2 * newHRV;

        emit hrvUpdated(m_currentHRV);
    }
}

void HeartRateSensor::updateZone()
{
    HeartRateZone newZone;

    if (m_currentBPM < ZONE_ELEVATED_BPM) {
        newZone = HeartRateZone::RESTING;
    } else if (m_currentBPM < ZONE_MODERATE_BPM) {
        newZone = HeartRateZone::ELEVATED;
    } else if (m_currentBPM < ZONE_HIGH_BPM) {
        newZone = HeartRateZone::MODERATE;
    } else if (m_currentBPM < ZONE_PEAK_BPM) {
        newZone = HeartRateZone::HIGH;
    } else {
        newZone = HeartRateZone::PEAK;
    }

    if (newZone != m_currentZone) {
        m_currentZone = newZone;
        emit heartRateZoneChanged(newZone);
    }
}

double HeartRateSensor::applyBandpassFilter(double value)
{
    // Simple IIR bandpass filter for 0.5-3 Hz at 10 Hz sampling
    // High-pass (0.5 Hz) followed by low-pass (3 Hz)

    static double hp_prev_in = 0.0;
    static double hp_prev_out = 0.0;
    static double lp_prev_out = 0.0;

    // High-pass filter (removes DC offset and slow drift)
    // fc = 0.5 Hz, alpha = 0.969 at 10 Hz sampling
    double hp_alpha = 0.969;
    double hp_out = hp_alpha * (hp_prev_out + value - hp_prev_in);
    hp_prev_in = value;
    hp_prev_out = hp_out;

    // Low-pass filter (removes high-frequency noise)
    // fc = 3 Hz, alpha = 0.653 at 10 Hz sampling
    double lp_alpha = 0.653;
    double lp_out = lp_alpha * hp_out + (1.0 - lp_alpha) * lp_prev_out;
    lp_prev_out = lp_out;

    return lp_out;
}

double HeartRateSensor::applyLowPassFilter(double value, double alpha)
{
    m_prevFilteredValue = alpha * value + (1.0 - alpha) * m_prevFilteredValue;
    return m_prevFilteredValue;
}

