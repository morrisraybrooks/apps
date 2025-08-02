#include "StatusIndicator.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QGridLayout>

StatusIndicator::StatusIndicator(QWidget *parent)
    : QWidget(parent)
    , m_status(OK)
    , m_text("OK")
    , m_animated(false)
    , m_animationEnabled(true)
    , m_blinkEnabled(false)
    , m_pulseEnabled(false)
    , m_horizontalLayout(false)
    , m_blinkTimer(new QTimer(this))
    , m_pulseAnimation(nullptr)
    , m_colorAnimation(new QPropertyAnimation(this, "color"))
    , m_blinkState(false)
    , m_pulseOpacity(1.0)
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

void StatusIndicator::setStatus(StatusLevel status, const QString &text)
{
    if (m_status != status || m_text != text) {
        m_status = status;
        m_text = text;
        
        updateColors();
        
        // Enable blinking for critical status
        if (status == CRITICAL || status == ERROR) {
            if (m_blinkEnabled) {
                m_blinkTimer->start();
            }
        } else {
            m_blinkTimer->stop();
            m_blinkState = false;
        }
        
        update();
        emit statusChanged(status);
    }
}

void StatusIndicator::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
}

void StatusIndicator::setBlinkingEnabled(bool enabled)
{
    m_blinkEnabled = enabled;

    if (!enabled) {
        m_blinkTimer->stop();
        m_blinkState = false;
        update();
    } else if (m_status == CRITICAL || m_status == ERROR) {
        m_blinkTimer->start();
    }
}

void StatusIndicator::setPulseEnabled(bool enabled)
{
    m_pulseEnabled = enabled;
    if (m_pulseAnimation) {
        if (enabled) {
            m_pulseAnimation->start();
        } else {
            m_pulseAnimation->stop();
        }
    }
}

void StatusIndicator::setHorizontalLayout(bool horizontal)
{
    m_horizontalLayout = horizontal;
    // Layout changes would require UI restructuring
    update();
}

void StatusIndicator::setStatusColor(StatusLevel status, const QColor& color)
{
    m_statusColors[status] = color;
    if (m_status == status) {
        updateColors();
        update();
    }
}

QColor StatusIndicator::getStatusColor(StatusLevel status) const
{
    if (m_statusColors.contains(status)) {
        return m_statusColors[status];
    }

    // Return default colors if not customized
    switch (status) {
    case OK:
        return QColor(76, 175, 80);    // Green
    case INFO:
        return QColor(33, 150, 243);   // Blue
    case WARNING:
        return QColor(255, 193, 7);    // Amber
    case CRITICAL:
        return QColor(244, 67, 54);    // Red
    case ERROR:
        return QColor(156, 39, 176);   // Purple
    case OFFLINE:
        return QColor(158, 158, 158);  // Gray
    default:
        return QColor(158, 158, 158);  // Gray
    }
}

void StatusIndicator::updateStatus(StatusLevel status, const QString& message)
{
    setStatus(status, message);
}

void StatusIndicator::clearStatus()
{
    setStatus(OK, "OK");
}

void StatusIndicator::startAnimation()
{
    setAnimated(true);
}

void StatusIndicator::stopAnimation()
{
    setAnimated(false);
}

void StatusIndicator::setAnimated(bool animated)
{
    m_animated = animated;
    if (animated) {
        if (m_pulseAnimation && m_pulseEnabled) {
            m_pulseAnimation->start();
        }
        if (m_blinkEnabled && (m_status == CRITICAL || m_status == ERROR)) {
            m_blinkTimer->start();
        }
    } else {
        if (m_pulseAnimation) {
            m_pulseAnimation->stop();
        }
        m_blinkTimer->stop();
        m_blinkState = false;
        update();
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

QSize StatusIndicator::minimumSizeHint() const
{
    return QSize(80, 24);
}

void StatusIndicator::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit clicked();
}

void StatusIndicator::setStatus(StatusLevel status)
{
    setStatus(status, QString());
}

void StatusIndicator::onPulseAnimation()
{
    // Pulse animation slot - could update opacity or other visual effects
    if (m_pulseAnimation && m_pulseEnabled) {
        m_pulseAnimation->start();
    }
}

// MultiStatusIndicator implementation
MultiStatusIndicator::MultiStatusIndicator(QWidget *parent)
    : QWidget(parent)
    , m_gridLayout(new QGridLayout(this))
    , m_columns(DEFAULT_COLUMNS)
{
    setLayout(m_gridLayout);
}

MultiStatusIndicator::~MultiStatusIndicator()
{
    // Qt will handle cleanup of child widgets
}

void MultiStatusIndicator::addStatus(const QString& name, const QString& label, StatusIndicator::StatusLevel initialStatus)
{
    if (m_indicators.contains(name)) {
        return; // Already exists
    }

    StatusIndicator* indicator = new StatusIndicator(this);
    indicator->setStatus(initialStatus, label);
    m_indicators[name] = indicator;

    // Connect click signal
    connect(indicator, &StatusIndicator::clicked, this, &MultiStatusIndicator::onIndicatorClicked);

    updateLayout();
}

void MultiStatusIndicator::removeStatus(const QString& name)
{
    if (m_indicators.contains(name)) {
        StatusIndicator* indicator = m_indicators.take(name);
        m_gridLayout->removeWidget(indicator);
        indicator->deleteLater();
        updateLayout();
    }
}

void MultiStatusIndicator::updateStatus(const QString& name, StatusIndicator::StatusLevel status, const QString& message)
{
    if (m_indicators.contains(name)) {
        m_indicators[name]->setStatus(status, message);
    }
}

void MultiStatusIndicator::setColumns(int columns)
{
    if (columns > 0) {
        m_columns = columns;
        updateLayout();
    }
}

StatusIndicator* MultiStatusIndicator::getIndicator(const QString& name) const
{
    return m_indicators.value(name, nullptr);
}

QStringList MultiStatusIndicator::getStatusNames() const
{
    return m_indicators.keys();
}

void MultiStatusIndicator::onIndicatorClicked()
{
    StatusIndicator* indicator = qobject_cast<StatusIndicator*>(sender());
    if (indicator) {
        QString name = m_indicators.key(indicator);
        if (!name.isEmpty()) {
            emit statusClicked(name, indicator->status());
        }
    }
}

void MultiStatusIndicator::updateLayout()
{
    // Clear current layout
    while (QLayoutItem* item = m_gridLayout->takeAt(0)) {
        delete item;
    }

    // Re-add indicators in grid layout
    int row = 0, col = 0;
    for (auto it = m_indicators.begin(); it != m_indicators.end(); ++it) {
        m_gridLayout->addWidget(it.value(), row, col);
        col++;
        if (col >= m_columns) {
            col = 0;
            row++;
        }
    }
}
