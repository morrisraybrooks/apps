#include "TouchButton.h"
#include "../styles/ModernMedicalStyle.h"
#include <QStyleOption>
#include <QPainter>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

TouchButton::TouchButton(QWidget *parent)
    : QPushButton(parent)
    , m_buttonType(Normal)
    , m_touchFeedbackEnabled(true)
    , m_animationEnabled(true)
    , m_glowEffect(false)
    , m_pulseEffect(false)
    , m_shadowEffect(nullptr)
    , m_pulseAnimation(nullptr)
    , m_pressAnimation(new QPropertyAnimation(this, "geometry"))
    , m_colorAnimation(nullptr)
    , m_longPressEnabled(false)
    , m_longPressDelay(500)
    , m_longPressTimer(new QTimer(this))
    , m_touchActive(false)
    , m_hapticFeedback(false)
    , m_soundFeedback(false)
{
    setupButton();
}

TouchButton::TouchButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
    , m_buttonType(Normal)
    , m_touchFeedbackEnabled(true)
    , m_animationEnabled(true)
    , m_glowEffect(false)
    , m_pulseEffect(false)
    , m_shadowEffect(nullptr)
    , m_pulseAnimation(nullptr)
    , m_pressAnimation(new QPropertyAnimation(this, "geometry"))
    , m_colorAnimation(nullptr)
    , m_longPressEnabled(false)
    , m_longPressDelay(500)
    , m_longPressTimer(new QTimer(this))
    , m_touchActive(false)
    , m_hapticFeedback(false)
    , m_soundFeedback(false)
{
    setupButton();
}

TouchButton::~TouchButton()
{
}

void TouchButton::setupButton()
{
    // Set minimum size for touch targets using modern scaling
    int minTouchTarget = ModernMedicalStyle::Spacing::getMinTouchTarget();
    int recommendedTarget = ModernMedicalStyle::Spacing::getRecommendedTouchTarget();
    setMinimumSize(recommendedTarget, minTouchTarget);

    // Enable focus for keyboard navigation
    setFocusPolicy(Qt::StrongFocus);

    // Setup animations with modern timing
    m_pressAnimation->setDuration(ModernMedicalStyle::Animation::FAST_DURATION);

    // Apply initial modern styling
    updateButtonStyle();

    // Add enhanced drop shadow effect for modern depth
    auto shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(ModernMedicalStyle::scaleValue(12));
    shadowEffect->setOffset(ModernMedicalStyle::scaleValue(2), ModernMedicalStyle::scaleValue(4));
    shadowEffect->setColor(ModernMedicalStyle::Colors::SHADOW_MEDIUM);
    setGraphicsEffect(shadowEffect);
}

void TouchButton::setButtonType(ButtonType type)
{
    if (m_buttonType != type) {
        m_buttonType = type;
        updateButtonStyle();
    }
}

void TouchButton::setTouchFeedbackEnabled(bool enabled)
{
    m_touchFeedbackEnabled = enabled;
}

void TouchButton::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
}

void TouchButton::setHapticFeedback(bool enabled)
{
    m_hapticFeedback = enabled;
}

void TouchButton::setSoundFeedback(bool enabled)
{
    m_soundFeedback = enabled;
}

void TouchButton::setTouchSize(const QSize& size)
{
    m_touchSize = size;
    setMinimumSize(size);
}

void TouchButton::setPulseEffect(bool enabled)
{
    m_pulseEffect = enabled;
    // Pulse effect disabled to fix property warnings
    // Could be implemented with QGraphicsOpacityEffect if needed
}

QSize TouchButton::sizeHint() const
{
    return m_touchSize.isValid() ? m_touchSize : QPushButton::sizeHint();
}

QSize TouchButton::minimumSizeHint() const
{
    return QSize(60, 30);
}

bool TouchButton::event(QEvent *event)
{
    return QPushButton::event(event);
}

void TouchButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
}

void TouchButton::setGlowEffect(bool enabled)
{
    m_glowEffect = enabled;
    if (enabled && !m_shadowEffect) {
        m_shadowEffect = new QGraphicsDropShadowEffect(this);
        m_shadowEffect->setBlurRadius(10);
        m_shadowEffect->setColor(QColor(0, 150, 255, 100));
        m_shadowEffect->setOffset(0, 0);
        setGraphicsEffect(m_shadowEffect);
    }
}

void TouchButton::onLongPressTimer()
{
    if (m_longPressEnabled) {
        emit longPressed();
    }
}

void TouchButton::onFlashTimer()
{
    // Flash timer implementation - toggle button state
    setDown(!isDown());
}

void TouchButton::onPulseTimer()
{
    // Pulse timer implementation - could animate opacity or size
    if (m_pulseAnimation && m_pulseEffect) {
        m_pulseAnimation->start();
    }
}

void TouchButton::startPulse()
{
    if (m_pulseAnimation && m_animationEnabled) {
        m_pulseAnimation->start();
    }
}

void TouchButton::stopPulse()
{
    if (m_pulseAnimation) {
        m_pulseAnimation->stop();
    }
}

void TouchButton::flashButton(int count)
{
    // Simple flash implementation - disabled to fix property warnings
    Q_UNUSED(count)
    // Flash effect could be implemented with style changes if needed
}

void TouchButton::applyButtonStyle()
{
    updateButtonStyle();
}

void TouchButton::updateButtonStyle()
{
    QString styleType;
    switch (m_buttonType) {
    case Primary:
        styleType = "primary";
        break;
    case Success:
        styleType = "success";
        break;
    case Warning:
        styleType = "warning";
        break;
    case Danger:
        styleType = "danger";
        break;
    case Normal:
    default:
        styleType = "secondary";
        break;
    }

    // Use the modern medical style system for consistent styling
    QString modernStyle = ModernMedicalStyle::getButtonStyle(styleType);

    // Add TouchButton-specific enhancements
    QString enhancedStyle = modernStyle + QString(
        "TouchButton {"
        "}"
        "TouchButton:focus {"
        "    outline: %3 solid %4;"
        "    outline-offset: %5;"
        "}"
        "TouchButton:focus {"
        "    outline: %1 solid %2;"
        "    outline-offset: %3;"
        "}"
        "TouchButton:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %4, stop:1 %5);"
        "    border-color: %6;"
        "}"
    ).arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::scalePixelValue(2))
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_LIGHT.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE.name())
     .arg(ModernMedicalStyle::Colors::PRIMARY_BLUE_DARK.name());

    setStyleSheet(enhancedStyle);
}

void TouchButton::mousePressEvent(QMouseEvent *event)
{
    if (m_touchFeedbackEnabled && m_animationEnabled) {
        // Animate button press with slight scale down
        QRect currentGeometry = geometry();
        QRect pressedGeometry = currentGeometry.adjusted(2, 2, -2, -2);
        
        m_pressAnimation->setStartValue(currentGeometry);
        m_pressAnimation->setEndValue(pressedGeometry);
        m_pressAnimation->start();
    }
    
    QPushButton::mousePressEvent(event);
}

void TouchButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_touchFeedbackEnabled && m_animationEnabled) {
        // Animate button release back to normal size
        QRect currentGeometry = geometry();
        QRect normalGeometry = currentGeometry.adjusted(-2, -2, 2, 2);
        
        m_pressAnimation->setStartValue(currentGeometry);
        m_pressAnimation->setEndValue(normalGeometry);
        m_pressAnimation->start();
    }
    
    QPushButton::mouseReleaseEvent(event);
}

void TouchButton::enterEvent(QEvent *event)
{
    if (m_touchFeedbackEnabled) {
        // Add subtle hover effect
        setCursor(Qt::PointingHandCursor);
    }
    
    QPushButton::enterEvent(event);
}

void TouchButton::leaveEvent(QEvent *event)
{
    if (m_touchFeedbackEnabled) {
        setCursor(Qt::ArrowCursor);
    }
    
    QPushButton::leaveEvent(event);
}

void TouchButton::focusInEvent(QFocusEvent *event)
{
    // Add focus indication for keyboard navigation
    setStyleSheet(styleSheet() + 
                 "TouchButton:focus { border: 3px solid #2196F3; }");
    
    QPushButton::focusInEvent(event);
}

void TouchButton::focusOutEvent(QFocusEvent *event)
{
    // Remove focus indication
    applyButtonStyle();
    
    QPushButton::focusOutEvent(event);
}
