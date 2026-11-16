#include "PulserCalibration.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

namespace {
bool fail(QString* error, const QString& message)
{
    if (error) *error = message;
    return false;
}
}  // namespace

bool PulserCalibration::loadFromFile(const QString& path, QString* error)
{
    m_valid = false;
    m_points.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(error, QString("Cannot open pulser calibration %1: %2").arg(path, file.errorString()));
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return fail(error, QString("Pulser calibration %1 is not a JSON object: %2")
                               .arg(path, parseError.errorString()));
    }
    return loadFromJson(doc.object(), error);
}

bool PulserCalibration::loadFromJson(const QJsonObject& obj, QString* error)
{
    m_valid = false;
    m_points.clear();

    if (!obj.value("measured").toBool(false)) {
        return fail(error, "Pulser calibration is not marked \"measured\": true - measure it on the bench first");
    }

    const QJsonArray points = obj.value("points").toArray();
    if (points.size() < 2) {
        return fail(error, "Pulser calibration needs at least 2 points");
    }

    QVector<CalibrationPoint> parsed;
    for (const QJsonValue& value : points) {
        const QJsonObject p = value.toObject();
        const CalibrationPoint point{p.value("pwm_percent").toDouble(-1),
                                     p.value("pulses_per_second").toDouble(-1),
                                     p.value("peak_mmhg").toDouble(-1)};
        if (point.pwmPercent < 0 || point.pwmPercent > 100 || point.pulsesPerSecond <= 0
            || point.peakMmHg < 0) {
            return fail(error, "Pulser calibration point has a missing or out-of-range value");
        }
        if (!parsed.isEmpty() && (point.pwmPercent <= parsed.last().pwmPercent
                                  || point.pulsesPerSecond <= parsed.last().pulsesPerSecond)) {
            return fail(error, "Pulser calibration points must increase in both pwm_percent and pulses_per_second");
        }
        parsed.append(point);
    }

    m_points = parsed;
    m_valid = true;
    return true;
}

double PulserCalibration::minRate() const
{
    return m_valid ? m_points.first().pulsesPerSecond : 0.0;
}

double PulserCalibration::maxRate() const
{
    return m_valid ? m_points.last().pulsesPerSecond : 0.0;
}

double PulserCalibration::interpolate(double pps, double CalibrationPoint::*field) const
{
    if (!m_valid) return 0.0;
    if (pps <= m_points.first().pulsesPerSecond) return m_points.first().*field;
    if (pps >= m_points.last().pulsesPerSecond) return m_points.last().*field;

    for (int i = 1; i < m_points.size(); ++i) {
        const CalibrationPoint& hi = m_points[i];
        if (pps <= hi.pulsesPerSecond) {
            const CalibrationPoint& lo = m_points[i - 1];
            const double t = (pps - lo.pulsesPerSecond) / (hi.pulsesPerSecond - lo.pulsesPerSecond);
            return lo.*field + t * (hi.*field - lo.*field);
        }
    }
    return m_points.last().*field;
}

double PulserCalibration::pwmForRate(double pps) const
{
    return interpolate(pps, &CalibrationPoint::pwmPercent);
}

double PulserCalibration::peakAtRate(double pps) const
{
    return interpolate(pps, &CalibrationPoint::peakMmHg);
}

double PulserCalibration::maxClitoralTarget(double pps) const
{
    return std::max(0.0, CAP_MMHG - peakAtRate(pps));
}
