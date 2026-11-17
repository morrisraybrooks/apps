#ifndef HANDPULSER_H
#define HANDPULSER_H

#include <QObject>
#include <QVector>

class PneumaticController;

/**
 * @brief A pulser session the owner runs by hand from the pulser card (Manual mode,
 * no pattern): legs sealed at 40 mmHg, clitoral suction at hand_clit_mmhg, pulses at
 * a rate stepped with slower()/faster(). Never above the bench-calibrated maximum.
 *
 * Design: docs/superpowers/specs/2026-11-17-pulser-card-design.md
 */
class HandPulser : public QObject
{
    Q_OBJECT

public:
    explicit HandPulser(PneumaticController* air, QObject* parent = nullptr);

    /// 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100 pulses/s
    static const QVector<double>& rateSteps();
    static constexpr double LEG_MMHG = 40.0;

    double rate() const { return m_rate; }
    void slower();
    void faster();

    bool canStart() const;   ///< calibrated and no emergency stop latched
    bool start();
    void stop();
    bool isRunning() const;

signals:
    void changed();

private:
    void setRate(double rate);
    void clampToMax();

    PneumaticController* m_air;
    double m_rate = 10.0;
    bool m_started = false;
};

#endif // HANDPULSER_H
