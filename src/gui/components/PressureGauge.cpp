#include "PressureGauge.h"
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>

// Constants
const double PressureGauge::START_ANGLE = -135.0;  // -135 degrees
const double PressureGauge::SPAN_ANGLE = 270.0;    // 270 degrees

PressureGauge::PressureGauge(QWidget *parent)
    : QWidget(parent)
    , m_currentValue(0.0)
    , m_targetValue(0.0)
    , m_minimum(0.0)
    , m_maximum(100.0)
    , m_warningThreshold(80.0)
    , m_criticalThreshold(95.0)
    , m_title("Pressure")
    , m_units("mmHg")
    , m_precision(DEFAULT_PRECISION)
    , m_showValue(true)
    , m_showThresholds(true)
    , m_animation(new QPropertyAnimation(this, "value"))
    , m_animationEnabled(true)
    , m_animationDuration(DEFAULT_ANIMATION_DURATION)
    , m_safeColor(QColor(76, 175, 80))      // Green
    , m_warningColor(QColor(255, 152, 0))   // Orange
    , m_criticalColor(QColor(244, 67, 54))  // Red
    , m_needleColor(QColor(33, 33, 33))     // Dark gray
    , m_backgroundColor(QColor(250, 250, 250)) // Light gray
    , m_textColor(QColor(33, 33, 33))       // Dark gray
    , m_radius(0.0)
    , m_needleLength(0.0)
    , m_needleWidth(4.0)
{
    setMinimumSize(MIN_SIZE, MIN_SIZE);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Configure animation
    m_animation->setDuration(m_animationDuration);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    
    updateGaugeGeometry();
}

PressureGauge::~PressureGauge()
{
    if (m_animation) {
        m_animation->stop();
    }
}

void PressureGauge::setValue(double value)
{
    double clampedValue = qBound(m_minimum, value, m_maximum);
    
    if (qAbs(m_currentValue - clampedValue) < 0.01) {
        return; // No significant change
    }
    
    if (m_animationEnabled && m_animation) {
        m_animation->stop();
        m_animation->setStartValue(m_currentValue);
        m_animation->setEndValue(clampedValue);
        m_animation->start();
    } else {
        m_currentValue = clampedValue;
        update();
    }
    
    // Check for threshold violations
    if (clampedValue > m_criticalThreshold) {
        emit thresholdExceeded(clampedValue, "Critical");
    } else if (clampedValue > m_warningThreshold) {
        emit thresholdExceeded(clampedValue, "Warning");
    }
    
    emit valueChanged(clampedValue);
}

void PressureGauge::setTargetValue(double value)
{
    m_targetValue = qBound(m_minimum, value, m_maximum);
    setValue(m_targetValue);
}

void PressureGauge::setRange(double minimum, double maximum)
{
    if (minimum < maximum) {
        m_minimum = minimum;
        m_maximum = maximum;
        
        // Adjust thresholds if they're outside the new range
        m_warningThreshold = qBound(minimum, m_warningThreshold, maximum);
        m_criticalThreshold = qBound(minimum, m_criticalThreshold, maximum);
        
        // Clamp current value to new range
        setValue(m_currentValue);
        
        update();
    }
}

void PressureGauge::setWarningThreshold(double threshold)
{
    m_warningThreshold = qBound(m_minimum, threshold, m_maximum);
    update();
}

void PressureGauge::setCriticalThreshold(double threshold)
{
    m_criticalThreshold = qBound(m_minimum, threshold, m_maximum);
    update();
}

void PressureGauge::setTitle(const QString& title)
{
    m_title = title;
    update();
}

void PressureGauge::setUnits(const QString& units)
{
    m_units = units;
    update();
}

void PressureGauge::setPrecision(int decimals)
{
    m_precision = qBound(0, decimals, 3);
    update();
}

void PressureGauge::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
}

void PressureGauge::setAnimationDuration(int ms)
{
    m_animationDuration = qBound(100, ms, 2000);
    if (m_animation) {
        m_animation->setDuration(m_animationDuration);
    }
}

void PressureGauge::resetValue()
{
    setValue(m_minimum);
}

void PressureGauge::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawBackground(painter);
    drawScale(painter);
    if (m_showThresholds) {
        drawThresholds(painter);
    }
    drawNeedle(painter);
    if (m_showValue) {
        drawValue(painter);
    }
    drawTitle(painter);
}

void PressureGauge::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateGaugeGeometry();
}

void PressureGauge::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

QSize PressureGauge::sizeHint() const
{
    return QSize(DEFAULT_SIZE, DEFAULT_SIZE);
}

QSize PressureGauge::minimumSizeHint() const
{
    return QSize(MIN_SIZE, MIN_SIZE);
}

void PressureGauge::drawBackground(QPainter& painter)
{
    painter.save();
    
    // Draw outer circle
    painter.setBrush(m_backgroundColor);
    painter.setPen(QPen(m_textColor, 2));
    painter.drawEllipse(m_gaugeRect);
    
    // Draw inner circle for depth effect
    QRectF innerRect = m_gaugeRect.adjusted(10, 10, -10, -10);
    painter.setBrush(QColor(255, 255, 255, 100));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(innerRect);
    
    painter.restore();
}

void PressureGauge::drawScale(QPainter& painter)
{
    painter.save();
    painter.translate(m_center);
    
    // Draw scale marks
    int majorTicks = 10;
    int minorTicks = 5;
    
    for (int i = 0; i <= majorTicks; ++i) {
        double angle = START_ANGLE + (SPAN_ANGLE * i / majorTicks);
        double radians = qDegreesToRadians(angle);
        
        // Major tick
        QPointF outer = getPointOnCircle(angle, m_radius - 15);
        QPointF inner = getPointOnCircle(angle, m_radius - 25);
        
        painter.setPen(QPen(m_textColor, 2));
        painter.drawLine(outer, inner);
        
        // Scale labels
        double value = m_minimum + (m_maximum - m_minimum) * i / majorTicks;
        QString label = QString::number(value, 'f', 0);
        
        QPointF textPos = getPointOnCircle(angle, m_radius - 40);
        QRect textRect(textPos.x() - 20, textPos.y() - 10, 40, 20);
        
        painter.setPen(m_textColor);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(textRect, Qt::AlignCenter, label);
        
        // Minor ticks
        if (i < majorTicks) {
            for (int j = 1; j < minorTicks; ++j) {
                double minorAngle = angle + (SPAN_ANGLE / majorTicks) * j / minorTicks;
                QPointF minorOuter = getPointOnCircle(minorAngle, m_radius - 15);
                QPointF minorInner = getPointOnCircle(minorAngle, m_radius - 20);
                
                painter.setPen(QPen(m_textColor, 1));
                painter.drawLine(minorOuter, minorInner);
            }
        }
    }
    
    painter.restore();
}

void PressureGauge::drawThresholds(QPainter& painter)
{
    painter.save();
    painter.translate(m_center);
    
    // Draw warning threshold arc
    double warningAngle = valueToAngle(m_warningThreshold);
    double criticalAngle = valueToAngle(m_criticalThreshold);
    
    QPen warningPen(m_warningColor, 8);
    warningPen.setCapStyle(Qt::RoundCap);
    painter.setPen(warningPen);
    
    QRectF arcRect(-m_radius + 30, -m_radius + 30, 2 * (m_radius - 30), 2 * (m_radius - 30));
    painter.drawArc(arcRect, static_cast<int>(warningAngle * 16), static_cast<int>((criticalAngle - warningAngle) * 16));
    
    // Draw critical threshold arc
    QPen criticalPen(m_criticalColor, 8);
    criticalPen.setCapStyle(Qt::RoundCap);
    painter.setPen(criticalPen);
    
    painter.drawArc(arcRect, static_cast<int>(criticalAngle * 16), static_cast<int>((START_ANGLE + SPAN_ANGLE - criticalAngle) * 16));
    
    painter.restore();
}

void PressureGauge::drawNeedle(QPainter& painter)
{
    painter.save();
    painter.translate(m_center);
    
    double angle = valueToAngle(m_currentValue);
    painter.rotate(angle);
    
    // Draw needle
    QColor needleColor = getValueColor(m_currentValue);
    QPen needlePen(needleColor, m_needleWidth);
    needlePen.setCapStyle(Qt::RoundCap);
    painter.setPen(needlePen);
    
    painter.drawLine(0, 0, 0, -m_needleLength);
    
    // Draw center circle
    painter.setBrush(needleColor);
    painter.setPen(QPen(needleColor.darker(), 2));
    painter.drawEllipse(-8, -8, 16, 16);
    
    painter.restore();
}

void PressureGauge::drawValue(QPainter& painter)
{
    painter.save();
    
    // Draw value text
    QString valueText = QString::number(m_currentValue, 'f', m_precision);
    if (!m_units.isEmpty()) {
        valueText += " " + m_units;
    }
    
    QFont valueFont("Arial", 16, QFont::Bold);
    painter.setFont(valueFont);
    painter.setPen(getValueColor(m_currentValue));
    
    QRect valueRect(m_center.x() - 60, m_center.y() + 20, 120, 30);
    painter.drawText(valueRect, Qt::AlignCenter, valueText);
    
    painter.restore();
}

void PressureGauge::drawTitle(QPainter& painter)
{
    painter.save();
    
    QFont titleFont("Arial", 12, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(m_textColor);
    
    QRect titleRect(m_center.x() - 80, m_center.y() - 60, 160, 20);
    painter.drawText(titleRect, Qt::AlignCenter, m_title);
    
    painter.restore();
}

QColor PressureGauge::getValueColor(double value) const
{
    if (value >= m_criticalThreshold) {
        return m_criticalColor;
    } else if (value >= m_warningThreshold) {
        return m_warningColor;
    } else {
        return m_safeColor;
    }
}

double PressureGauge::valueToAngle(double value) const
{
    double normalizedValue = (value - m_minimum) / (m_maximum - m_minimum);
    return START_ANGLE + (SPAN_ANGLE * normalizedValue);
}

QPointF PressureGauge::getPointOnCircle(double angle, double radius) const
{
    double radians = qDegreesToRadians(angle);
    return QPointF(radius * cos(radians), radius * sin(radians));
}

void PressureGauge::updateGaugeGeometry()
{
    int size = qMin(width(), height());
    m_radius = size / 2.0 - 20;
    m_needleLength = m_radius - 30;
    m_center = QPointF(width() / 2.0, height() / 2.0);
    m_gaugeRect = QRectF(m_center.x() - m_radius, m_center.y() - m_radius, 2 * m_radius, 2 * m_radius);
    
    update();
}
