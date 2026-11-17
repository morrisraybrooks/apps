#ifndef PULSERCARD_H
#define PULSERCARD_H

#include <QWidget>
#include <functional>

class HardwareManager;
class OrgasmControlAlgorithm;
class PatternEngine;
class QLabel;
class QPushButton;
class QTimer;

/**
 * @brief "CLITORAL PULSER" dashboard card: PAUSE/RESUME (works in every mode),
 * a hand-run pulser session with SLOWER/FASTER, and VENT TANK.
 * Big buttons and numbers for a low-vision owner.
 *
 * automationActive() says whether a program or pattern is in charge; while it is,
 * the hand controls are gray and the number shows the rate in use.
 *
 * Design: docs/superpowers/specs/2026-11-17-pulser-card-design.md
 */
class PulserCard : public QWidget
{
    Q_OBJECT

public:
    PulserCard(HardwareManager* hw, std::function<bool()> automationActive,
               QWidget* parent = nullptr);

    /// A program or pattern is actually running (left-over ERROR states after an
    /// emergency stop do not count)
    static bool isAutomationActive(const OrgasmControlAlgorithm* algorithm,
                                   const PatternEngine* patterns);

public slots:
    void refresh();

private:
    void onStartStop();
    void onPause();

    HardwareManager* m_hw;
    std::function<bool()> m_automationActive;
    QPushButton* m_startStop;
    QPushButton* m_slower;
    QPushButton* m_faster;
    QLabel* m_rate;
    QPushButton* m_pause;
    QPushButton* m_ventTank;
    QLabel* m_status;
    QTimer* m_timer;
    int m_pauseStyle = -1;   ///< 0 running style, 1 paused style; avoids restyling every tick
};

#endif // PULSERCARD_H
