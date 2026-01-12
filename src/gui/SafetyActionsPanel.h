#ifndef SAFETYACTIONSPANEL_H
#define SAFETYACTIONSPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include "../game/GameTypes.h"

// Forward declarations
class GameEngine;
class ProgressTracker;
class TouchButton;

/**
 * @brief Safety Actions Panel for in-game safeword and safety controls
 * 
 * This panel provides ALWAYS-ACCESSIBLE safety controls during gameplay:
 * - YELLOW button: Reduce intensity, pause consequences
 * - RED button: Safeword - end game gracefully, no penalties
 * - EMERGENCY STOP: Immediate halt of all stimulation
 * 
 * These buttons CANNOT be disabled by game logic and work regardless of:
 * - Game state
 * - DOM commands
 * - Network connection
 * - Consequence engine state
 * 
 * The RED button requires safeword verification for safety.
 */
class SafetyActionsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SafetyActionsPanel(GameEngine* gameEngine,
                                ProgressTracker* progressTracker,
                                QWidget* parent = nullptr);
    ~SafetyActionsPanel();

    // Enable/disable panel (but safety buttons always work)
    void setGameActive(bool active);

Q_SIGNALS:
    void safetyActionTriggered(SafetyAction action);
    void emergencyStopRequested();

private Q_SLOTS:
    void onYellowClicked();
    void onRedClicked();
    void onEmergencyStopClicked();
    void onGameStateChanged(GameState state);

private:
    void setupUI();
    void connectSignals();
    bool verifySafeword();

    // Core components
    GameEngine* m_gameEngine;
    ProgressTracker* m_progressTracker;

    // UI components
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_safetyGroup;
    
    // Safety action buttons
    TouchButton* m_yellowButton;
    TouchButton* m_redButton;
    TouchButton* m_emergencyStopButton;
    
    // Status labels
    QLabel* m_statusLabel;
    QLabel* m_instructionsLabel;
    
    // State
    bool m_gameActive;
};

/**
 * @brief Dialog for safeword verification
 */
class SafewordVerificationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SafewordVerificationDialog(const QString& configuredSafeword,
                                        QWidget* parent = nullptr);
    
    bool isVerified() const { return m_verified; }

private Q_SLOTS:
    void onVerifyClicked();
    void onCancelClicked();

private:
    void setupUI();
    
    QString m_configuredSafeword;
    bool m_verified;
    
    QLineEdit* m_safewordInput;
    QPushButton* m_verifyButton;
    QPushButton* m_cancelButton;
    QLabel* m_instructionLabel;
    QLabel* m_errorLabel;
};

#endif // SAFETYACTIONSPANEL_H

