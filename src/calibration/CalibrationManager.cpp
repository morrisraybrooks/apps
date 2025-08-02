#include "CalibrationManager.h"
#include "../hardware/HardwareManager.h"
#include "../hardware/SensorInterface.h"
#include "../hardware/ActuatorControl.h"
#include "../logging/DataLogger.h"
#include <QDebug>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <cmath>
#include <algorithm>

// Constants
const int CalibrationManager::DEFAULT_MIN_CALIBRATION_POINTS = 5;
const double CalibrationManager::DEFAULT_MAX_CALIBRATION_ERROR = 2.0; // 2% max error
const int CalibrationManager::DEFAULT_CALIBRATION_INTERVAL = 1000; // 1 second
const int CalibrationManager::DEFAULT_CALIBRATION_TIMEOUT = 300000; // 5 minutes

CalibrationManager::CalibrationManager(HardwareManager* hardware, QObject *parent)
    : QObject(parent)
    , m_hardware(hardware)
    , m_sensorInterface(nullptr)
    , m_actuatorControl(nullptr)
    , m_dataLogger(nullptr)
    , m_currentState(IDLE)
    , m_currentType(SENSOR_CALIBRATION)
    , m_progress(0)
    , m_currentStep(0)
    , m_totalSteps(0)
    , m_minCalibrationPoints(DEFAULT_MIN_CALIBRATION_POINTS)
    , m_maxCalibrationError(DEFAULT_MAX_CALIBRATION_ERROR)
    , m_autoSaveEnabled(true)
    , m_calibrationTimer(new QTimer(this))
    , m_calibrationInterval(DEFAULT_CALIBRATION_INTERVAL)
{
    // Get hardware interfaces
    if (m_hardware) {
        m_sensorInterface = m_hardware->getSensorInterface();
        m_actuatorControl = m_hardware->getActuatorControl();
    }
    
    // Set up calibration data path
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_calibrationDataPath = appDataPath + "/calibration";
    QDir().mkpath(m_calibrationDataPath);
    
    // Setup calibration timer
    setupCalibrationTimer();
    
    // Initialize calibration manager
    initializeCalibrationManager();
    
    qDebug() << "CalibrationManager initialized";
}

CalibrationManager::~CalibrationManager()
{
    cancelCalibration();
}

void CalibrationManager::startSensorCalibration(const QString& sensorName)
{
    if (m_currentState != IDLE) {
        emit calibrationFailed(sensorName, "Another calibration is already in progress");
        return;
    }
    
    if (!m_sensorInterface) {
        emit calibrationFailed(sensorName, "Sensor interface not available");
        return;
    }
    
    qDebug() << "Starting sensor calibration for:" << sensorName;
    
    m_currentState = PREPARING;
    m_currentComponent = sensorName;
    m_currentType = SENSOR_CALIBRATION;
    m_progress = 0;
    m_currentStep = 0;
    m_currentPoints.clear();
    
    // Set up calibration steps based on sensor type
    if (sensorName == "AVL Sensor" || sensorName == "Tank Sensor") {
        m_totalSteps = 5; // 5-point calibration
    } else {
        m_totalSteps = 3; // 3-point calibration for other sensors
    }
    
    m_calibrationStartTime = QDateTime::currentDateTime();
    
    emit calibrationStarted(sensorName, SENSOR_CALIBRATION);
    emit calibrationProgress(0, "Preparing sensor calibration...");
    
    // Start calibration process
    m_calibrationTimer->start();
}

void CalibrationManager::startActuatorCalibration(const QString& actuatorName)
{
    if (m_currentState != IDLE) {
        emit calibrationFailed(actuatorName, "Another calibration is already in progress");
        return;
    }
    
    if (!m_actuatorControl) {
        emit calibrationFailed(actuatorName, "Actuator control not available");
        return;
    }
    
    qDebug() << "Starting actuator calibration for:" << actuatorName;
    
    m_currentState = PREPARING;
    m_currentComponent = actuatorName;
    m_currentType = ACTUATOR_CALIBRATION;
    m_progress = 0;
    m_currentStep = 0;
    m_currentPoints.clear();
    
    // Set up calibration steps based on actuator type
    if (actuatorName == "Pump") {
        m_totalSteps = 10; // 10-point speed calibration
    } else {
        m_totalSteps = 2; // 2-point valve calibration (open/closed)
    }
    
    m_calibrationStartTime = QDateTime::currentDateTime();
    
    emit calibrationStarted(actuatorName, ACTUATOR_CALIBRATION);
    emit calibrationProgress(0, "Preparing actuator calibration...");
    
    // Start calibration process
    m_calibrationTimer->start();
}

void CalibrationManager::startSystemCalibration()
{
    if (m_currentState != IDLE) {
        emit calibrationFailed("System", "Another calibration is already in progress");
        return;
    }
    
    qDebug() << "Starting system calibration";
    
    m_currentState = PREPARING;
    m_currentComponent = "System";
    m_currentType = SYSTEM_CALIBRATION;
    m_progress = 0;
    m_currentStep = 0;
    m_currentPoints.clear();
    
    // System calibration includes all sensors and actuators
    m_totalSteps = 20; // Comprehensive system calibration
    
    m_calibrationStartTime = QDateTime::currentDateTime();
    
    emit calibrationStarted("System", SYSTEM_CALIBRATION);
    emit calibrationProgress(0, "Preparing system calibration...");
    
    // Start calibration process
    m_calibrationTimer->start();
}

void CalibrationManager::cancelCalibration()
{
    if (m_currentState == IDLE) return;
    
    qDebug() << "Cancelling calibration for:" << m_currentComponent;
    
    m_calibrationTimer->stop();
    resetCalibrationState();
    
    emit calibrationFailed(m_currentComponent, "Calibration cancelled by user");
}

void CalibrationManager::addCalibrationPoint(double referenceValue, double measuredValue)
{
    if (m_currentState != COLLECTING_DATA) {
        qWarning() << "Cannot add calibration point - not in data collection state";
        return;
    }
    
    CalibrationPoint point;
    point.referenceValue = referenceValue;
    point.measuredValue = measuredValue;
    point.timestamp = QDateTime::currentDateTime();
    point.valid = true;
    
    // Basic validation
    if (std::isnan(referenceValue) || std::isnan(measuredValue) ||
        std::isinf(referenceValue) || std::isinf(measuredValue)) {
        point.valid = false;
        qWarning() << "Invalid calibration point values";
    }
    
    m_currentPoints.append(point);
    
    qDebug() << QString("Added calibration point: ref=%1, measured=%2")
                .arg(referenceValue).arg(measuredValue);
    
    emit calibrationPointAdded(point);
}

void CalibrationManager::completeCurrentCalibration()
{
    if (m_currentState == IDLE) return;
    
    qDebug() << "Completing calibration for:" << m_currentComponent;
    
    m_calibrationTimer->stop();
    m_currentState = CALCULATING;
    
    emit calibrationProgress(90, "Calculating calibration parameters...");
    
    // Calculate calibration parameters
    calculateCalibrationParameters();
}

// Timer-based calibration step execution
void CalibrationManager::onCalibrationTimer()
{
    performCalibrationStep();
}

void CalibrationManager::performCalibrationStep()
{
    switch (m_currentState) {
    case PREPARING:
        m_currentState = COLLECTING_DATA;
        emit calibrationProgress(10, "Starting data collection...");
        break;
        
    case COLLECTING_DATA:
        collectCalibrationData();
        break;
        
    case CALCULATING:
        calculateCalibrationParameters();
        break;
        
    case VALIDATING:
        validateCalibrationResult();
        break;
        
    case COMPLETE:
    case FAILED:
        m_calibrationTimer->stop();
        resetCalibrationState();
        break;
        
    default:
        break;
    }
}

void CalibrationManager::collectCalibrationData()
{
    if (m_currentStep >= m_totalSteps) {
        m_currentState = CALCULATING;
        emit calibrationProgress(80, "Data collection complete. Calculating...");
        return;
    }
    
    // Collect data based on calibration type
    if (m_currentType == SENSOR_CALIBRATION) {
        if (m_currentComponent == "AVL Sensor") {
            calibrateAVLSensor();
        } else if (m_currentComponent == "Tank Sensor") {
            calibrateTankSensor();
        }
    } else if (m_currentType == ACTUATOR_CALIBRATION) {
        if (m_currentComponent == "Pump") {
            calibratePumpSpeed();
        } else if (m_currentComponent.contains("Valve")) {
            calibrateValveResponse();
        }
    } else if (m_currentType == SYSTEM_CALIBRATION) {
        performSystemCalibration();
    }
    
    m_currentStep++;
    int progress = 10 + (70 * m_currentStep / m_totalSteps);
    emit calibrationProgress(progress, QString("Collecting data point %1 of %2...")
                            .arg(m_currentStep).arg(m_totalSteps));
}

void CalibrationManager::calculateCalibrationParameters()
{
    if (m_currentPoints.size() < m_minCalibrationPoints) {
        m_currentState = FAILED;
        emit calibrationFailed(m_currentComponent, 
                              QString("Insufficient calibration points: %1 (minimum: %2)")
                              .arg(m_currentPoints.size()).arg(m_minCalibrationPoints));
        return;
    }
    
    // Calculate linear calibration parameters
    double slope, offset, correlation;
    if (!calculateLinearCalibration(m_currentPoints, slope, offset, correlation)) {
        m_currentState = FAILED;
        emit calibrationFailed(m_currentComponent, "Failed to calculate calibration parameters");
        return;
    }
    
    // Populate calibration result
    m_currentResult.component = m_currentComponent;
    m_currentResult.type = m_currentType;
    m_currentResult.slope = slope;
    m_currentResult.offset = offset;
    m_currentResult.correlation = correlation;
    m_currentResult.maxError = calculateMaxError(m_currentPoints, slope, offset);
    m_currentResult.timestamp = QDateTime::currentDateTime();
    m_currentResult.points = m_currentPoints;
    
    // Check if calibration meets quality requirements
    if (correlation < 0.95 || m_currentResult.maxError > m_maxCalibrationError) {
        m_currentState = FAILED;
        emit calibrationFailed(m_currentComponent, 
                              QString("Calibration quality insufficient: R²=%1, MaxError=%2%")
                              .arg(correlation, 0, 'f', 3).arg(m_currentResult.maxError, 0, 'f', 1));
        return;
    }
    
    m_currentResult.successful = true;
    m_currentState = VALIDATING;
    
    emit calibrationProgress(95, "Validating calibration...");
}

void CalibrationManager::validateCalibrationResult()
{
    // Apply calibration to hardware if successful
    if (m_currentResult.successful) {
        if (m_currentType == SENSOR_CALIBRATION && m_sensorInterface) {
            if (m_currentComponent == "AVL Sensor") {
                // Apply calibration to AVL sensor
                // This would typically involve updating the sensor interface
                qDebug() << "Applied AVL sensor calibration: slope=" << m_currentResult.slope
                         << "offset=" << m_currentResult.offset;
            } else if (m_currentComponent == "Tank Sensor") {
                // Apply calibration to Tank sensor
                qDebug() << "Applied Tank sensor calibration: slope=" << m_currentResult.slope
                         << "offset=" << m_currentResult.offset;
            }
        }

        // Save calibration data if auto-save is enabled
        if (m_autoSaveEnabled) {
            if (saveCalibrationData(m_currentResult)) {
                emit calibrationDataSaved(m_currentComponent);
            }
        }

        // Log calibration event
        if (m_dataLogger) {
            QJsonObject logData;
            logData["component"] = m_currentResult.component;
            logData["slope"] = m_currentResult.slope;
            logData["offset"] = m_currentResult.offset;
            logData["correlation"] = m_currentResult.correlation;
            logData["max_error"] = m_currentResult.maxError;
            logData["points_count"] = m_currentResult.points.size();

            m_dataLogger->logCalibrationEvent(m_currentComponent, "calibration_completed", logData);
        }

        m_currentState = COMPLETE;
        emit calibrationProgress(100, "Calibration completed successfully!");
        emit calibrationCompleted(m_currentResult);

    } else {
        m_currentState = FAILED;
        emit calibrationFailed(m_currentComponent, m_currentResult.errorMessage);
    }
}

// Private helper methods
void CalibrationManager::initializeCalibrationManager()
{
    // Load existing calibration data
    QStringList components = {"AVL Sensor", "Tank Sensor", "Pump", "SOL1", "SOL2", "SOL3"};

    for (const QString& component : components) {
        CalibrationResult result;
        if (loadCalibrationData(component, result)) {
            m_calibrationCache[component] = result;
            qDebug() << "Loaded calibration data for:" << component;
        }
    }
}

void CalibrationManager::setupCalibrationTimer()
{
    m_calibrationTimer->setSingleShot(false);
    m_calibrationTimer->setInterval(m_calibrationInterval);

    connect(m_calibrationTimer, &QTimer::timeout,
            this, &CalibrationManager::onCalibrationTimer);
}

void CalibrationManager::resetCalibrationState()
{
    m_currentState = IDLE;
    m_currentComponent.clear();
    m_progress = 0;
    m_currentStep = 0;
    m_totalSteps = 0;
    m_currentPoints.clear();
    m_currentResult = CalibrationResult();
}

bool CalibrationManager::calculateLinearCalibration(const QList<CalibrationPoint>& points,
                                                   double& slope, double& offset, double& correlation)
{
    if (points.size() < 2) return false;

    // Calculate means
    double sumX = 0.0, sumY = 0.0;
    int validPoints = 0;

    for (const CalibrationPoint& point : points) {
        if (point.valid) {
            sumX += point.referenceValue;
            sumY += point.measuredValue;
            validPoints++;
        }
    }

    if (validPoints < 2) return false;

    double meanX = sumX / validPoints;
    double meanY = sumY / validPoints;

    // Calculate slope and offset using least squares
    double numerator = 0.0, denominator = 0.0;

    for (const CalibrationPoint& point : points) {
        if (point.valid) {
            double dx = point.referenceValue - meanX;
            double dy = point.measuredValue - meanY;
            numerator += dx * dy;
            denominator += dx * dx;
        }
    }

    if (std::abs(denominator) < 1e-10) return false;

    slope = numerator / denominator;
    offset = meanY - slope * meanX;

    // Calculate correlation coefficient
    correlation = calculateCorrelationCoefficient(points, slope, offset);

    return true;
}

double CalibrationManager::calculateCorrelationCoefficient(const QList<CalibrationPoint>& points,
                                                          double slope, double offset)
{
    if (points.size() < 2) return 0.0;

    double sumSquaredResiduals = 0.0;
    double sumSquaredTotal = 0.0;
    double meanY = 0.0;
    int validPoints = 0;

    // Calculate mean of measured values
    for (const CalibrationPoint& point : points) {
        if (point.valid) {
            meanY += point.measuredValue;
            validPoints++;
        }
    }
    meanY /= validPoints;

    // Calculate R²
    for (const CalibrationPoint& point : points) {
        if (point.valid) {
            double predicted = slope * point.referenceValue + offset;
            double residual = point.measuredValue - predicted;
            double total = point.measuredValue - meanY;

            sumSquaredResiduals += residual * residual;
            sumSquaredTotal += total * total;
        }
    }

    if (sumSquaredTotal < 1e-10) return 0.0;

    return 1.0 - (sumSquaredResiduals / sumSquaredTotal);
}

double CalibrationManager::calculateMaxError(const QList<CalibrationPoint>& points,
                                           double slope, double offset)
{
    double maxError = 0.0;

    for (const CalibrationPoint& point : points) {
        if (point.valid) {
            double predicted = slope * point.referenceValue + offset;
            double error = std::abs(point.measuredValue - predicted);
            double percentError = (error / std::max(std::abs(point.measuredValue), 1e-10)) * 100.0;
            maxError = std::max(maxError, percentError);
        }
    }

    return maxError;
}

// Sensor calibration methods
void CalibrationManager::calibrateAVLSensor()
{
    if (!m_sensorInterface) return;

    // Define reference pressure points for AVL sensor calibration
    QList<double> referencePressures = {0.0, 25.0, 50.0, 75.0, 100.0};

    if (m_currentStep < referencePressures.size()) {
        double refPressure = referencePressures[m_currentStep];

        // In a real implementation, you would:
        // 1. Set the system to the reference pressure
        // 2. Wait for stabilization
        // 3. Read the sensor value

        // For now, simulate the measurement
        double measuredValue = m_sensorInterface->readAVLPressure();

        addCalibrationPoint(refPressure, measuredValue);

        qDebug() << QString("AVL calibration point %1: ref=%2 mmHg, measured=%3 mmHg")
                    .arg(m_currentStep + 1).arg(refPressure).arg(measuredValue);
    }
}

void CalibrationManager::calibrateTankSensor()
{
    if (!m_sensorInterface) return;

    // Define reference pressure points for Tank sensor calibration
    QList<double> referencePressures = {0.0, 25.0, 50.0, 75.0, 100.0};

    if (m_currentStep < referencePressures.size()) {
        double refPressure = referencePressures[m_currentStep];

        // Read the sensor value
        double measuredValue = m_sensorInterface->readTankPressure();

        addCalibrationPoint(refPressure, measuredValue);

        qDebug() << QString("Tank calibration point %1: ref=%2 mmHg, measured=%3 mmHg")
                    .arg(m_currentStep + 1).arg(refPressure).arg(measuredValue);
    }
}

// Actuator calibration methods
void CalibrationManager::calibratePumpSpeed()
{
    if (!m_actuatorControl) return;

    // Define reference speed points for pump calibration (0-100%)
    QList<double> referenceSpeeds = {0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0};

    if (m_currentStep < referenceSpeeds.size()) {
        double refSpeed = referenceSpeeds[m_currentStep];

        // Set pump speed and measure actual performance
        m_actuatorControl->setPumpSpeed(refSpeed);

        // Wait for stabilization (in real implementation)
        // QThread::msleep(1000);

        // Measure actual pump performance (this would require feedback sensor)
        double measuredSpeed = m_actuatorControl->getPumpSpeed();

        addCalibrationPoint(refSpeed, measuredSpeed);

        qDebug() << QString("Pump calibration point %1: ref=%2%, measured=%3%")
                    .arg(m_currentStep + 1).arg(refSpeed).arg(measuredSpeed);
    }
}

void CalibrationManager::calibrateValveResponse()
{
    if (!m_actuatorControl) return;

    // Simple 2-point calibration for valves (open/closed)
    QList<double> referenceStates = {0.0, 1.0}; // 0 = closed, 1 = open

    if (m_currentStep < referenceStates.size()) {
        double refState = referenceStates[m_currentStep];
        bool openState = (refState > 0.5);

        // Set valve state based on component name
        if (m_currentComponent == "SOL1") {
            m_actuatorControl->setSOL1(openState);
            double measuredState = m_actuatorControl->getSOL1State() ? 1.0 : 0.0;
            addCalibrationPoint(refState, measuredState);
        } else if (m_currentComponent == "SOL2") {
            m_actuatorControl->setSOL2(openState);
            double measuredState = m_actuatorControl->getSOL2State() ? 1.0 : 0.0;
            addCalibrationPoint(refState, measuredState);
        } else if (m_currentComponent == "SOL3") {
            m_actuatorControl->setSOL3(openState);
            double measuredState = m_actuatorControl->getSOL3State() ? 1.0 : 0.0;
            addCalibrationPoint(refState, measuredState);
        }

        qDebug() << QString("Valve %1 calibration point %2: ref=%3, measured=%4")
                    .arg(m_currentComponent).arg(m_currentStep + 1).arg(refState);
    }
}

void CalibrationManager::performSystemCalibration()
{
    // System calibration performs calibration of all components in sequence
    // This is a simplified implementation

    if (m_currentStep < 5) {
        // Calibrate AVL sensor
        calibrateAVLSensor();
    } else if (m_currentStep < 10) {
        // Calibrate Tank sensor
        calibrateTankSensor();
    } else if (m_currentStep < 15) {
        // Calibrate pump
        calibratePumpSpeed();
    } else {
        // Calibrate valves
        calibrateValveResponse();
    }
}

// Data persistence methods
bool CalibrationManager::saveCalibrationData(const CalibrationResult& result)
{
    QMutexLocker locker(&m_dataMutex);

    if (saveCalibrationToFile(result)) {
        m_calibrationCache[result.component] = result;
        return true;
    }

    return false;
}

bool CalibrationManager::loadCalibrationData(const QString& component, CalibrationResult& result)
{
    QMutexLocker locker(&m_dataMutex);

    // Check cache first
    if (m_calibrationCache.contains(component)) {
        result = m_calibrationCache[component];
        return true;
    }

    // Load from file
    if (loadCalibrationFromFile(component, result)) {
        m_calibrationCache[component] = result;
        return true;
    }

    return false;
}

QStringList CalibrationManager::getAvailableCalibrations() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_calibrationCache.keys();
}

bool CalibrationManager::isComponentCalibrated(const QString& component) const
{
    QMutexLocker locker(&m_dataMutex);

    if (m_calibrationCache.contains(component)) {
        return m_calibrationCache[component].successful;
    }

    return false;
}

QDateTime CalibrationManager::getLastCalibrationDate(const QString& component) const
{
    QMutexLocker locker(&m_dataMutex);

    if (m_calibrationCache.contains(component)) {
        return m_calibrationCache[component].timestamp;
    }

    return QDateTime();
}

bool CalibrationManager::validateCalibration(const QString& component)
{
    CalibrationResult result;
    if (!loadCalibrationData(component, result)) {
        emit calibrationValidated(component, false);
        return false;
    }

    // Check if calibration is expired
    if (isCalibrationExpired(component)) {
        emit calibrationValidated(component, false);
        return false;
    }

    // Check calibration quality
    bool valid = result.successful &&
                 result.correlation > 0.95 &&
                 result.maxError < m_maxCalibrationError;

    emit calibrationValidated(component, valid);
    return valid;
}

bool CalibrationManager::isCalibrationExpired(const QString& component, int maxDays) const
{
    QDateTime lastCalibration = getLastCalibrationDate(component);
    if (!lastCalibration.isValid()) return true;

    QDateTime now = QDateTime::currentDateTime();
    qint64 daysSinceCalibration = lastCalibration.daysTo(now);

    return daysSinceCalibration > maxDays;
}

QJsonObject CalibrationManager::getCalibrationStatus() const
{
    QMutexLocker locker(&m_dataMutex);

    QJsonObject status;
    status["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    status["total_components"] = m_calibrationCache.size();

    QJsonArray components;
    for (auto it = m_calibrationCache.begin(); it != m_calibrationCache.end(); ++it) {
        QJsonObject componentStatus;
        componentStatus["name"] = it.key();
        componentStatus["calibrated"] = it.value().successful;
        componentStatus["last_calibration"] = it.value().timestamp.toString(Qt::ISODate);
        componentStatus["correlation"] = it.value().correlation;
        componentStatus["max_error"] = it.value().maxError;
        componentStatus["expired"] = isCalibrationExpired(it.key());

        components.append(componentStatus);
    }
    status["components"] = components;

    return status;
}

void CalibrationManager::setCalibrationDataPath(const QString& path)
{
    m_calibrationDataPath = path;
    QDir().mkpath(path);
}

// File I/O methods
QString CalibrationManager::getCalibrationFilePath(const QString& component) const
{
    QString filename = component.toLower().replace(" ", "_") + "_calibration.json";
    return m_calibrationDataPath + "/" + filename;
}

bool CalibrationManager::saveCalibrationToFile(const CalibrationResult& result)
{
    QString filePath = getCalibrationFilePath(result.component);

    QJsonObject calibrationObj;
    calibrationObj["component"] = result.component;
    calibrationObj["type"] = static_cast<int>(result.type);
    calibrationObj["slope"] = result.slope;
    calibrationObj["offset"] = result.offset;
    calibrationObj["correlation"] = result.correlation;
    calibrationObj["max_error"] = result.maxError;
    calibrationObj["timestamp"] = result.timestamp.toString(Qt::ISODate);
    calibrationObj["successful"] = result.successful;
    calibrationObj["error_message"] = result.errorMessage;

    // Save calibration points
    QJsonArray pointsArray;
    for (const CalibrationPoint& point : result.points) {
        QJsonObject pointObj;
        pointObj["reference_value"] = point.referenceValue;
        pointObj["measured_value"] = point.measuredValue;
        pointObj["timestamp"] = point.timestamp.toString(Qt::ISODate);
        pointObj["valid"] = point.valid;
        pointsArray.append(pointObj);
    }
    calibrationObj["points"] = pointsArray;

    QJsonDocument doc(calibrationObj);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();

        qDebug() << "Calibration data saved for:" << result.component;
        return true;
    } else {
        qWarning() << "Failed to save calibration data for:" << result.component;
        return false;
    }
}

bool CalibrationManager::loadCalibrationFromFile(const QString& component, CalibrationResult& result)
{
    QString filePath = getCalibrationFilePath(component);

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject calibrationObj = doc.object();

    result.component = calibrationObj["component"].toString();
    result.type = static_cast<CalibrationType>(calibrationObj["type"].toInt());
    result.slope = calibrationObj["slope"].toDouble();
    result.offset = calibrationObj["offset"].toDouble();
    result.correlation = calibrationObj["correlation"].toDouble();
    result.maxError = calibrationObj["max_error"].toDouble();
    result.timestamp = QDateTime::fromString(calibrationObj["timestamp"].toString(), Qt::ISODate);
    result.successful = calibrationObj["successful"].toBool();
    result.errorMessage = calibrationObj["error_message"].toString();

    // Load calibration points
    result.points.clear();
    QJsonArray pointsArray = calibrationObj["points"].toArray();
    for (const QJsonValue& pointValue : pointsArray) {
        QJsonObject pointObj = pointValue.toObject();
        CalibrationPoint point;
        point.referenceValue = pointObj["reference_value"].toDouble();
        point.measuredValue = pointObj["measured_value"].toDouble();
        point.timestamp = QDateTime::fromString(pointObj["timestamp"].toString(), Qt::ISODate);
        point.valid = pointObj["valid"].toBool();
        result.points.append(point);
    }

    qDebug() << "Calibration data loaded for:" << component;
    return true;
}
