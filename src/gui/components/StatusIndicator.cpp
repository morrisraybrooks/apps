#include "StatusIndicator.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>

StatusIndicator::StatusIndicator(QWidget *parent)
    : QWidget(parent)
    , m_status(OK)
    , m_text("OK")
    , m_animationEnabled(true)
    , m_blinkingEnabled(false)
    , m_blinkTimer(new QTimer(this))
    , m_blinkState(false)
    , m_colorAnimation(new QPropertyAnimation(this, "color"))
{
    setupIndicator();
}

StatusIndicator::~StatusIndicator()
{
}

void StatusIndicator::setupIndicator()
{
    setMinimumSize(100, 30);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    // Setup blink timer
    m_blinkTimer->setInterval(500); // 500ms blink interval
    connect(m_blinkTimer, &QTimer::timeout, this, &StatusIndicator::onBlinkTimer);
    
    // Setup color animation
    m_colorAnimation->setDuration(300);
    
    updateColors();
}

void StatusIndicator::setStatus(Status status, const QString &text)
{
    if (m_status != status || m_text != text) {
        m_status = status;
        m_text = text;
        
        updateColors();
        
        // Enable blinking for critical status
        if (status == CRITICAL || status == ERROR) {
            if (m_blinkingEnabled) {
                m_blinkTimer->start();
            }
        } else {
            m_blinkTimer->stop();
            m_blinkState = false;
        }
        
        update();
        emit statusChanged(status, text);
    }
}

void StatusIndicator::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
}

void StatusIndicator::setBlinkingEnabled(bool enabled)
{
    m_blinkingEnabled = enabled;
    
    if (!enabled) {
        m_blinkTimer->stop();
        m_blinkState = false;
        update();
    } else if (m_status == CRITICAL || m_status == ERROR) {
        m_blinkTimer->start();
    }
}

void StatusIndicator::updateColors()
{
    switch (m_status) {
    case OK:
        m_backgroundColor = QColor(76, 175, 80);    // Green
        m_textColor = QColor(255, 255, 255);        // White
        m_borderColor = QColor(56, 142, 60);        // Dark green
        break;
        
    case WARNING:
        m_backgroundColor = QColor(255, 152, 0);    // Orange
        m_textColor = QColor(255, 255, 255);        // White
        m_borderColor = QColor(245, 124, 0);        // Dark orange
        break;
        
    case ERROR:
        m_backgroundColor = QColor(244, 67, 54);    // Red
        m_textColor = QColor(255, 255, 255);        // White
        m_borderColor = QColor(211, 47, 47);        // Dark red
        break;
        
    case CRITICAL:
        m_backgroundColor = QColor(183, 28, 28);    // Dark red
        m_textColor = QColor(255, 255, 255);        // White
        m_borderColor = QColor(136, 14, 79);        // Purple-red
        break;
        
    case UNKNOWN:
        m_backgroundColor = QColor(158, 158, 158);  // Gray
        m_textColor = QColor(255, 255, 255);        // White
        m_borderColor = QColor(117, 117, 117);      // Dark gray
        break;
        
    case OFFLINE:
        m_backgroundColor = QColor(96, 125, 139);   // Blue-gray
        m_textColor = QColor(255, 255, 255);        // White
        m_borderColor = QColor(69, 90, 100);        // Dark blue-gray
        break;
    }
}

void StatusIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rect = this->rect().adjusted(1, 1, -1, -1);
    
    // Determine colors based on blink state
    QColor bgColor = m_backgroundColor;
    QColor txtColor = m_textColor;
    
    if (m_blinkState && (m_status == CRITICAL || m_status == ERROR)) {
        // Alternate colors during blink
        bgColor = bgColor.lighter(150);
        txtColor = QColor(0, 0, 0); // Black text during blink
    }
    
    // Draw background with rounded corners
    painter.setBrush(QBrush(bgColor));
    painter.setPen(QPen(m_borderColor, 2));
    painter.drawRoundedRect(rect, 6, 6);
    
    // Draw status indicator circle
    int circleSize = qMin(rect.height() - 8, 16);
    QRect circleRect(rect.left() + 8, rect.center().y() - circleSize/2, circleSize, circleSize);
    
    painter.setBrush(QBrush(txtColor));
    painter.setPen(QPen(m_borderColor, 1));
    painter.drawEllipse(circleRect);
    
    // Draw text
    QRect textRect = rect.adjusted(circleRect.width() + 16, 0, -8, 0);
    painter.setPen(txtColor);
    
    QFont font = this->font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
}

QSize StatusIndicator::sizeHint() const
{
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(m_text);
    return QSize(textWidth + 50, 30); // 50px for circle and padding
}

void StatusIndicator::onBlinkTimer()
{
    m_blinkState = !m_blinkState;
    update();
}
