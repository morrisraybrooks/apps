#include "PulserCard.h"

#include "HandPulser.h"
#include "HardwareManager.h"
#include "OrgasmControlAlgorithm.h"
#include "PatternEngine.h"
#include "PneumaticController.h"
#include "styles/ModernMedicalStyle.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

namespace {
QPushButton* bigButton(const QString& text, const char* name, const QString& style, int w, int h)
{
    auto* b = new QPushButton(text);
    b->setObjectName(name);
    b->setMinimumSize(ModernMedicalStyle::scaleValue(w), ModernMedicalStyle::scaleValue(h));
    b->setStyleSheet(ModernMedicalStyle::getButtonStyle(style));
    return b;
}
}  // namespace

PulserCard::PulserCard(HardwareManager* hw, std::function<bool()> automationActive, QWidget* parent)
    : QWidget(parent)
    , m_hw(hw)
    , m_automationActive(std::move(automationActive))
    , m_timer(new QTimer(this))
{
    m_startStop = bigButton("START PULSES", "startStopPulses", "primary", 300, 150);
    m_ventTank = bigButton("VENT TANK", "ventTankButton", "secondary", 300, 150);
    m_slower = bigButton("SLOWER", "slowerButton", "secondary", 300, 150);
    m_faster = bigButton("FASTER", "fasterButton", "secondary", 300, 150);
    m_pause = bigButton("PAUSE PULSES", "pauseButton", "success", 450, 180);

    m_rate = new QLabel;
    m_rate->setObjectName("rateLabel");
    m_rate->setAlignment(Qt::AlignCenter);
    m_rate->setStyleSheet(QString("QLabel { font-family: %1; font-size: %2pt; font-weight: %3; }")
                              .arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
                              .arg(ModernMedicalStyle::Typography::getDisplayTitle() * 2)
                              .arg(ModernMedicalStyle::Typography::WEIGHT_BOLD));

    m_status = new QLabel;
    m_status->setObjectName("statusLabel");
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet(QString("QLabel { font-family: %1; font-size: %2pt; font-weight: %3; }")
                                .arg(ModernMedicalStyle::Typography::PRIMARY_FONT)
                                .arg(ModernMedicalStyle::Typography::getHeadline())
                                .arg(ModernMedicalStyle::Typography::WEIGHT_BOLD));

    auto* grid = new QGridLayout(this);
    grid->setSpacing(ModernMedicalStyle::Spacing::getLarge());
    grid->addWidget(m_startStop, 0, 0);
    grid->addWidget(m_status, 0, 1);
    grid->addWidget(m_ventTank, 0, 2);
    grid->addWidget(m_slower, 1, 0);
    grid->addWidget(m_rate, 1, 1);
    grid->addWidget(m_faster, 1, 2);
    grid->addWidget(m_pause, 2, 0, 1, 3);

    connect(m_startStop, &QPushButton::clicked, this, &PulserCard::onStartStop);
    connect(m_pause, &QPushButton::clicked, this, &PulserCard::onPause);
    connect(m_slower, &QPushButton::clicked, this, [this]() {
        if (m_hw && m_hw->handPulser()) m_hw->handPulser()->slower();
        refresh();
    });
    connect(m_faster, &QPushButton::clicked, this, [this]() {
        if (m_hw && m_hw->handPulser()) m_hw->handPulser()->faster();
        refresh();
    });
    connect(m_ventTank, &QPushButton::clicked, this, [this]() {
        if (m_hw && m_hw->pneumatics()) m_hw->pneumatics()->ventTank();
        refresh();
    });

    if (m_hw && m_hw->pneumatics()) {
        connect(m_hw->pneumatics(), &PneumaticController::userPauseChanged, this, &PulserCard::refresh);
    }
    if (m_hw && m_hw->handPulser()) {
        connect(m_hw->handPulser(), &HandPulser::changed, this, &PulserCard::refresh);
    }

    m_timer->setInterval(200);
    connect(m_timer, &QTimer::timeout, this, &PulserCard::refresh);
    m_timer->start();
    refresh();
}

bool PulserCard::isAutomationActive(const OrgasmControlAlgorithm* algorithm,
                                    const PatternEngine* patterns)
{
    if (algorithm) {
        const auto state = algorithm->getState();
        if (state != OrgasmControlAlgorithm::ControlState::STOPPED
            && state != OrgasmControlAlgorithm::ControlState::ERROR) {
            return true;
        }
    }
    if (patterns) {
        switch (patterns->getState()) {
        case PatternEngine::STARTING:
        case PatternEngine::RUNNING:
        case PatternEngine::PAUSED:
        case PatternEngine::STOPPING:
            return true;
        default:
            break;
        }
    }
    return false;
}

void PulserCard::onStartStop()
{
    HandPulser* hand = m_hw ? m_hw->handPulser() : nullptr;
    if (!hand) return;
    if (hand->isRunning()) {
        hand->stop();
    } else if (!m_automationActive()) {
        hand->start();
    }
    refresh();
}

void PulserCard::onPause()
{
    PneumaticController* air = m_hw ? m_hw->pneumatics() : nullptr;
    if (air) air->setUserPaused(!air->isUserPaused());
    refresh();
}

void PulserCard::refresh()
{
    PneumaticController* air = m_hw ? m_hw->pneumatics() : nullptr;
    HandPulser* hand = m_hw ? m_hw->handPulser() : nullptr;
    if (!air || !hand) {
        for (QPushButton* b : {m_startStop, m_slower, m_faster, m_pause, m_ventTank}) b->setEnabled(false);
        m_status->setText("Pulser unavailable");
        m_rate->setText(QString());
        return;
    }

    const bool automation = m_automationActive && m_automationActive();
    if (automation) hand->yield();   // a program or pattern took over: hand session ends
    const bool running = hand->isRunning();
    const bool paused = air->isUserPaused();

    m_startStop->setText(running ? "STOP PULSES" : "START PULSES");
    m_startStop->setEnabled(running || (hand->canStart() && !automation));
    m_slower->setEnabled(!automation);
    m_faster->setEnabled(!automation);
    m_pause->setEnabled(paused || (air->isActive() && !air->isLatched()));   // needs a session
    m_ventTank->setEnabled(!air->isActive());

    const double rate = automation ? air->pulseRate() : hand->rate();
    m_rate->setText(QString("%1 /s").arg(QString::number(rate)));

    m_pause->setText(paused ? "RESUME" : "PAUSE PULSES");
    if (m_pauseStyle != static_cast<int>(paused)) {
        m_pauseStyle = paused;
        m_pause->setStyleSheet(ModernMedicalStyle::getButtonStyle(paused ? "warning" : "success"));
    }

    if (paused) {
        m_status->setText("Paused");   // what the owner just pressed comes first
    } else if (!air->hasCalibration()) {
        m_status->setText("Pulser not calibrated");
    } else if (automation) {
        m_status->setText("Program in charge");
    } else {
        m_status->setText(QString());
    }
}
