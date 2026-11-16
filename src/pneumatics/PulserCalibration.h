#ifndef PULSERCALIBRATION_H
#define PULSERCALIBRATION_H

#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * @brief One measured bench point for the clitoral pulser.
 */
struct CalibrationPoint {
    double pwmPercent;       ///< Motor PWM duty, 0-100 %
    double pulsesPerSecond;  ///< Measured pulse rate at that duty
    double peakMmHg;         ///< Measured worst-case pulse swing, peak-to-peak (PulserBench "swing")
};

/**
 * @brief Bench-measured table mapping pulse rate to motor PWM and pulse size.
 *
 * Loaded from config/pulser_calibration.json. The pulser must not run without a
 * valid, measured table (spec 5.6). Format:
 * { "measured": true, "measured_on": "YYYY-MM-DD", "pump": "...",
 *   "points": [ {"pwm_percent": 40, "pulses_per_second": 30, "peak_mmhg": 12}, ... ] }
 */
class PulserCalibration
{
public:
    static constexpr double CAP_MMHG = 75.0;  ///< Steady + pulse peak must stay below this

    bool loadFromFile(const QString& path, QString* error = nullptr);
    bool loadFromJson(const QJsonObject& obj, QString* error = nullptr);
    bool isValid() const { return m_valid; }

    double minRate() const;
    double maxRate() const;
    double pwmForRate(double pps) const;
    double peakAtRate(double pps) const;
    double maxClitoralTarget(double pps) const;

private:
    double interpolate(double pps, double CalibrationPoint::*field) const;

    QVector<CalibrationPoint> m_points;
    bool m_valid = false;
};

#endif // PULSERCALIBRATION_H
