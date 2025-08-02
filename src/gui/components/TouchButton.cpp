#include "TouchButton.h"
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
    // Set minimum size for touch targets (44px minimum for accessibility)
    setMinimumSize(60, 44);
    
    // Enable focus for keyboard navigation
    setFocusPolicy(Qt::StrongFocus);
    
    // Setup animations
    m_pressAnimation->setDuration(100);
    // Color animation disabled to fix property warnings
    
    // Apply initial styling
    updateButtonStyle();
    
    // Add drop shadow effect for depth
    auto shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(8);
    shadowEffect->setOffset(2, 2);
    shadowEffect->setColor(QColor(0, 0, 0, 60));
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
    QString baseStyle = 
        "TouchButton {"
        "    border: 2px solid %1;"
        "    border-radius: 8px;"
        "    background-color: %2;"
        "    color: %3;"
        "    font-size: 14pt;"
        "    font-weight: bold;"
        "    padding: 8px 16px;"
        "    text-align: center;"
        "}"
        "TouchButton:hover {"
        "    background-color: %4;"
        "    border-color: %5;"
        "}"
        "TouchButton:pressed {"
        "    background-color: %6;"
        "    border-color: %7;"
        "}"
        "TouchButton:disabled {"
        "    background-color: #f0f0f0;"
        "    color: #999999;"
        "    border-color: #cccccc;"
        "}";
    
    QString borderColor, backgroundColor, textColor;
    QString hoverBg, hoverBorder, pressedBg, pressedBorder;
    
    switch (m_buttonType) {
    case Primary:
        borderColor = "#2196F3";
        backgroundColor = "#2196F3";
        textColor = "white";
        hoverBg = "#1976D2";
        hoverBorder = "#1976D2";
        pressedBg = "#0D47A1";
        pressedBorder = "#0D47A1";
        break;
        
    case Success:
        borderColor = "#4CAF50";
        backgroundColor = "#4CAF50";
        textColor = "white";
        hoverBg = "#388E3C";
        hoverBorder = "#388E3C";
        pressedBg = "#1B5E20";
        pressedBorder = "#1B5E20";
        break;
        
    case Warning:
        borderColor = "#FF9800";
        backgroundColor = "#FF9800";
        textColor = "white";
        hoverBg = "#F57C00";
        hoverBorder = "#F57C00";
        pressedBg = "#E65100";
        pressedBorder = "#E65100";
        break;
        
    case Danger:
        borderColor = "#f44336";
        backgroundColor = "#f44336";
        textColor = "white";
        hoverBg = "#D32F2F";
        hoverBorder = "#D32F2F";
        pressedBg = "#B71C1C";
        pressedBorder = "#B71C1C";
        break;
        
    case Normal:
    default:
        borderColor = "#ddd";
        backgroundColor = "white";
        textColor = "#333";
        hoverBg = "#f5f5f5";
        hoverBorder = "#bbb";
        pressedBg = "#e0e0e0";
        pressedBorder = "#999";
        break;
    }
    
    QString style = baseStyle
                   .arg(borderColor)
                   .arg(backgroundColor)
                   .arg(textColor)
                   .arg(hoverBg)
                   .arg(hoverBorder)
                   .arg(pressedBg)
                   .arg(pressedBorder);
    
    setStyleSheet(style);
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
