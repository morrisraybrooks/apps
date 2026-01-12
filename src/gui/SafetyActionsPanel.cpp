#include "SafetyActionsPanel.h"
#include "components/TouchButton.h"
#include "styles/ModernMedicalStyle.h"
#include "../game/GameEngine.h"
#include "../game/ProgressTracker.h"
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

SafetyActionsPanel::SafetyActionsPanel(GameEngine* gameEngine,
                                       ProgressTracker* progressTracker,
                                       QWidget* parent)
    : QWidget(parent)
    , m_gameEngine(gameEngine)
    , m_progressTracker(progressTracker)
    , m_mainLayout(nullptr)
    , m_safetyGroup(nullptr)
    , m_yellowButton(nullptr)
    , m_redButton(nullptr)
    , m_emergencyStopButton(nullptr)
    , m_statusLabel(nullptr)
    , m_instructionsLabel(nullptr)
    , m_gameActive(false)
{
    setupUI();
    connectSignals();
}

SafetyActionsPanel::~SafetyActionsPanel()
{
}

void SafetyActionsPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);

    // Safety Actions Group
    m_safetyGroup = new QGroupBox("Safety Actions");
    m_safetyGroup->setStyleSheet(ModernMedicalStyle::getGroupBoxStyle(ModernMedicalStyle::Colors::MEDICAL_RED));
    
    QVBoxLayout* safetyLayout = new QVBoxLayout(m_safetyGroup);
    safetyLayout->setSpacing(15);

    // Instructions
    m_instructionsLabel = new QLabel(
        "<b>⚠️ ALWAYS ACCESSIBLE SAFETY CONTROLS</b><br>"
        "These buttons work at all times, regardless of game state or DOM commands."
    );
    m_instructionsLabel->setWordWrap(true);
    m_instructionsLabel->setStyleSheet("font-size: 12pt; color: #f44336; padding: 10px; background-color: #FFEBEE; border-radius: 5px;");
    safetyLayout->addWidget(m_instructionsLabel);

    // YELLOW Button (Reduce Intensity)
    m_yellowButton = new TouchButton("🟡 YELLOW\nReduce Intensity");
    m_yellowButton->setButtonType(TouchButton::Warning);
    m_yellowButton->setMinimumSize(200, 80);
    m_yellowButton->setStyleSheet(
        "QPushButton { background-color: #FFC107; color: #000; font-size: 16pt; font-weight: bold; }"
        "QPushButton:hover { background-color: #FFD54F; }"
        "QPushButton:pressed { background-color: #FFA000; }"
    );
    
    QLabel* yellowDesc = new QLabel("• Reduces stimulation intensity by 30%\n• Pauses consequences temporarily\n• Game continues");
    yellowDesc->setStyleSheet("font-size: 10pt; color: #666; margin-left: 10px;");
    
    safetyLayout->addWidget(m_yellowButton);
    safetyLayout->addWidget(yellowDesc);
    safetyLayout->addSpacing(10);

    // RED Button (Safeword)
    m_redButton = new TouchButton("🔴 RED (SAFEWORD)\nEnd Game Gracefully");
    m_redButton->setButtonType(TouchButton::Danger);
    m_redButton->setMinimumSize(200, 80);
    m_redButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: #fff; font-size: 16pt; font-weight: bold; }"
        "QPushButton:hover { background-color: #E57373; }"
        "QPushButton:pressed { background-color: #D32F2F; }"
    );
    
    QLabel* redDesc = new QLabel("• Ends game immediately\n• No penalties applied\n• Requires safeword verification");
    redDesc->setStyleSheet("font-size: 10pt; color: #666; margin-left: 10px;");
    
    safetyLayout->addWidget(m_redButton);
    safetyLayout->addWidget(redDesc);
    safetyLayout->addSpacing(10);

    // EMERGENCY STOP Button
    m_emergencyStopButton = new TouchButton("⛔ EMERGENCY STOP\nImmediate Halt");
    m_emergencyStopButton->setButtonType(TouchButton::Emergency);
    m_emergencyStopButton->setMinimumSize(200, 80);
    m_emergencyStopButton->setPulseEffect(true);
    m_emergencyStopButton->setStyleSheet(
        "QPushButton { background-color: #B71C1C; color: #fff; font-size: 16pt; font-weight: bold; border: 3px solid #fff; }"
        "QPushButton:hover { background-color: #C62828; }"
        "QPushButton:pressed { background-color: #8B0000; }"
    );
    
    QLabel* emergencyDesc = new QLabel("• Immediate halt of ALL stimulation\n• Vents vacuum system\n• Stops TENS immediately");
    emergencyDesc->setStyleSheet("font-size: 10pt; color: #666; margin-left: 10px;");
    
    safetyLayout->addWidget(m_emergencyStopButton);
    safetyLayout->addWidget(emergencyDesc);

    // Status label
    m_statusLabel = new QLabel("Status: Ready");
    m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #4CAF50; padding: 10px; margin-top: 10px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    safetyLayout->addWidget(m_statusLabel);

    m_mainLayout->addWidget(m_safetyGroup);
    m_mainLayout->addStretch();

    setLayout(m_mainLayout);
}

void SafetyActionsPanel::connectSignals()
{
    // Connect button clicks
    connect(m_yellowButton, &TouchButton::clicked, this, &SafetyActionsPanel::onYellowClicked);
    connect(m_redButton, &TouchButton::clicked, this, &SafetyActionsPanel::onRedClicked);
    connect(m_emergencyStopButton, &TouchButton::clicked, this, &SafetyActionsPanel::onEmergencyStopClicked);

    // Connect to game engine state changes
    if (m_gameEngine) {
        connect(m_gameEngine, &GameEngine::stateChanged, this, &SafetyActionsPanel::onGameStateChanged);
    }
}

void SafetyActionsPanel::setGameActive(bool active)
{
    m_gameActive = active;
    
    // Update status label
    if (active) {
        m_statusLabel->setText("Status: Game Active - Safety Controls Available");
        m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #FF9800; padding: 10px;");
    } else {
        m_statusLabel->setText("Status: No Active Game");
        m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #4CAF50; padding: 10px;");
    }
    
    // NOTE: We do NOT disable the buttons - they are ALWAYS accessible
    // This is a critical safety feature
}

void SafetyActionsPanel::onYellowClicked()
{
    qDebug() << "SafetyActionsPanel: YELLOW button clicked";

    if (!m_gameEngine) {
        QMessageBox::warning(this, "Error", "Game engine not available");
        return;
    }

    // Confirm action
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Yellow Safety Action",
        "Reduce stimulation intensity by 30% and pause consequences?\n\n"
        "The game will continue, but at reduced intensity.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_gameEngine->triggerSafetyAction(SafetyAction::YELLOW);
        emit safetyActionTriggered(SafetyAction::YELLOW);

        m_statusLabel->setText("Status: YELLOW - Intensity Reduced");
        m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #FFC107; padding: 10px;");

        qDebug() << "SafetyActionsPanel: YELLOW action triggered";
    }
}

void SafetyActionsPanel::onRedClicked()
{
    qDebug() << "SafetyActionsPanel: RED button clicked";

    if (!m_gameEngine) {
        QMessageBox::warning(this, "Error", "Game engine not available");
        return;
    }

    // Verify safeword
    if (!verifySafeword()) {
        qDebug() << "SafetyActionsPanel: Safeword verification failed";
        return;
    }

    // Trigger RED action (SAFEWORD)
    m_gameEngine->triggerSafetyAction(SafetyAction::RED);
    emit safetyActionTriggered(SafetyAction::RED);

    m_statusLabel->setText("Status: SAFEWORD ACTIVATED - Game Ended");
    m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #f44336; padding: 10px;");

    qDebug() << "SafetyActionsPanel: RED (SAFEWORD) action triggered";
}

void SafetyActionsPanel::onEmergencyStopClicked()
{
    qDebug() << "SafetyActionsPanel: EMERGENCY STOP clicked";

    if (!m_gameEngine) {
        QMessageBox::warning(this, "Error", "Game engine not available");
        return;
    }

    // No confirmation for emergency stop - immediate action
    m_gameEngine->emergencyStop();
    emit emergencyStopRequested();

    m_statusLabel->setText("Status: EMERGENCY STOP ACTIVATED");
    m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #B71C1C; padding: 10px;");

    qDebug() << "SafetyActionsPanel: EMERGENCY STOP triggered";
}

void SafetyActionsPanel::onGameStateChanged(GameState state)
{
    switch (state) {
        case GameState::RUNNING:
            setGameActive(true);
            break;
        case GameState::PAUSED:
            m_statusLabel->setText("Status: Game Paused - Safety Controls Available");
            m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #FF9800; padding: 10px;");
            break;
        case GameState::SAFEWORD:
            m_statusLabel->setText("Status: SAFEWORD ACTIVATED");
            m_statusLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #f44336; padding: 10px;");
            break;
        case GameState::IDLE:
        case GameState::VICTORY:
        case GameState::FAILURE:
        case GameState::TIMEOUT:
        case GameState::POST_GAME:
            setGameActive(false);
            break;
        default:
            break;
    }
}

bool SafetyActionsPanel::verifySafeword()
{
    if (!m_progressTracker) {
        qWarning() << "SafetyActionsPanel: No progress tracker available";
        return false;
    }

    // Get configured safeword from settings
    QString configuredSafeword = m_progressTracker->safeWord();

    if (configuredSafeword.isEmpty()) {
        // No safeword configured - allow action with warning
        QMessageBox::StandardButton reply = QMessageBox::warning(
            this,
            "No Safeword Configured",
            "No safeword is configured. Use RED action anyway?\n\n"
            "This will end the game immediately with no penalties.",
            QMessageBox::Yes | QMessageBox::No
        );
        return reply == QMessageBox::Yes;
    }

    // Show verification dialog
    SafewordVerificationDialog dialog(configuredSafeword, this);
    dialog.exec();

    return dialog.isVerified();
}

// ============================================================================
// SafewordVerificationDialog Implementation
// ============================================================================

SafewordVerificationDialog::SafewordVerificationDialog(const QString& configuredSafeword,
                                                       QWidget* parent)
    : QDialog(parent)
    , m_configuredSafeword(configuredSafeword)
    , m_verified(false)
    , m_safewordInput(nullptr)
    , m_verifyButton(nullptr)
    , m_cancelButton(nullptr)
    , m_instructionLabel(nullptr)
    , m_errorLabel(nullptr)
{
    setupUI();
    setModal(true);
    setWindowTitle("Safeword Verification");
    resize(400, 200);
}

void SafewordVerificationDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Instruction label
    m_instructionLabel = new QLabel(
        "<b>🔴 SAFEWORD VERIFICATION REQUIRED</b><br><br>"
        "To activate the RED safety action and end the game,<br>"
        "please enter your configured safeword below."
    );
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("font-size: 12pt; padding: 10px; background-color: #FFEBEE; border-radius: 5px;");
    mainLayout->addWidget(m_instructionLabel);

    // Safeword input
    m_safewordInput = new QLineEdit;
    m_safewordInput->setPlaceholderText("Enter safeword...");
    m_safewordInput->setEchoMode(QLineEdit::Password);
    m_safewordInput->setMinimumHeight(40);
    m_safewordInput->setStyleSheet("font-size: 14pt; padding: 5px;");
    mainLayout->addWidget(m_safewordInput);

    // Error label (hidden by default)
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: #f44336; font-weight: bold; font-size: 11pt;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;

    m_verifyButton = new QPushButton("Verify & Activate RED");
    m_verifyButton->setMinimumSize(150, 50);
    m_verifyButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: #fff; font-size: 12pt; font-weight: bold; }"
        "QPushButton:hover { background-color: #E57373; }"
    );

    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setMinimumSize(100, 50);
    m_cancelButton->setStyleSheet("font-size: 12pt;");

    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_verifyButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_verifyButton, &QPushButton::clicked, this, &SafewordVerificationDialog::onVerifyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SafewordVerificationDialog::onCancelClicked);
    connect(m_safewordInput, &QLineEdit::returnPressed, this, &SafewordVerificationDialog::onVerifyClicked);

    // Focus on input
    m_safewordInput->setFocus();
}

void SafewordVerificationDialog::onVerifyClicked()
{
    QString enteredSafeword = m_safewordInput->text().trimmed();

    if (enteredSafeword.isEmpty()) {
        m_errorLabel->setText("⚠️ Please enter your safeword");
        m_errorLabel->show();
        return;
    }

    // Case-insensitive comparison
    if (enteredSafeword.toLower() == m_configuredSafeword.toLower()) {
        m_verified = true;
        accept();
    } else {
        m_errorLabel->setText("❌ Incorrect safeword. Please try again.");
        m_errorLabel->show();
        m_safewordInput->clear();
        m_safewordInput->setFocus();
    }
}

void SafewordVerificationDialog::onCancelClicked()
{
    m_verified = false;
    reject();
}

