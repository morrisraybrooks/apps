#include "MockOrgasmControlAlgorithm.h"
#include <QMutexLocker>
#include <QDebug>

MockOrgasmControlAlgorithm::MockOrgasmControlAlgorithm(QObject *parent)
    : QObject(parent)
{
}

void MockOrgasmControlAlgorithm::startAdaptiveEdging(int targetCycles)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(targetCycles);
    m_mode = Mode::ADAPTIVE_EDGING;
    m_state = ControlState::BUILDING;
    emit modeChanged(m_mode);
    emit stateChanged(m_state);
}

void MockOrgasmControlAlgorithm::startForcedOrgasm(int targetOrgasms, int maxDurationMs)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(targetOrgasms);
    Q_UNUSED(maxDurationMs);
    m_mode = Mode::FORCED_ORGASM;
    m_state = ControlState::FORCING;
    emit modeChanged(m_mode);
    emit stateChanged(m_state);
}

void MockOrgasmControlAlgorithm::startDenial(int durationMs)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(durationMs);
    m_mode = Mode::DENIAL;
    m_state = ControlState::BUILDING;
    emit modeChanged(m_mode);
    emit stateChanged(m_state);
}

void MockOrgasmControlAlgorithm::startMilking(int durationMs, int failureMode)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(durationMs);
    m_milkingFailureMode = failureMode;
    m_mode = Mode::MILKING;
    m_state = ControlState::MILKING;
    emit modeChanged(m_mode);
    emit stateChanged(m_state);
}

void MockOrgasmControlAlgorithm::stop()
{
    QMutexLocker locker(&m_mutex);
    m_mode = Mode::MANUAL;
    m_state = ControlState::STOPPED;
    emit modeChanged(m_mode);
    emit stateChanged(m_state);
}

void MockOrgasmControlAlgorithm::emergencyStop()
{
    QMutexLocker locker(&m_mutex);
    m_mode = Mode::MANUAL;
    m_state = ControlState::ERROR;
    emit modeChanged(m_mode);
    emit stateChanged(m_state);
}

void MockOrgasmControlAlgorithm::setEdgeThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_edgeThreshold = qBound(0.5, threshold, 0.95);
    emit edgeThresholdChanged(m_edgeThreshold);
}

void MockOrgasmControlAlgorithm::setOrgasmThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_orgasmThreshold = qBound(0.85, threshold, 1.0);
    emit orgasmThresholdChanged(m_orgasmThreshold);
}

void MockOrgasmControlAlgorithm::setRecoveryThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_recoveryThreshold = qBound(0.3, threshold, 0.8);
    emit recoveryThresholdChanged(m_recoveryThreshold);
}

void MockOrgasmControlAlgorithm::setMilkingZoneLower(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_milkingZoneLower = qBound(0.6, threshold, 0.85);
}

void MockOrgasmControlAlgorithm::setMilkingZoneUpper(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_milkingZoneUpper = qBound(0.8, threshold, 0.95);
}

void MockOrgasmControlAlgorithm::setDangerThreshold(double threshold)
{
    QMutexLocker locker(&m_mutex);
    m_dangerThreshold = qBound(0.88, threshold, 0.98);
}

void MockOrgasmControlAlgorithm::setMilkingFailureMode(int mode)
{
    QMutexLocker locker(&m_mutex);
    m_milkingFailureMode = qBound(0, mode, 3);
}

void MockOrgasmControlAlgorithm::setTENSEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_tensEnabled = enabled;
}

void MockOrgasmControlAlgorithm::setAntiEscapeEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_antiEscapeEnabled = enabled;
}

void MockOrgasmControlAlgorithm::simulateArousalChange(double level)
{
    QMutexLocker locker(&m_mutex);
    m_arousalLevel = qBound(0.0, level, 1.0);
    emit arousalLevelChanged(m_arousalLevel);
}

void MockOrgasmControlAlgorithm::simulateStateChange(ControlState state)
{
    QMutexLocker locker(&m_mutex);
    m_state = state;
    emit stateChanged(m_state);
}

