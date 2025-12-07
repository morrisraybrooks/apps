#ifndef PRESSUREGAUGE_H
#define PRESSUREGAUGE_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>

/**
 * @brief Custom pressure gauge widget for vacuum controller
 * 
 * This widget provides a circular gauge display optimized for touch interfaces
 * and large displays. Features include:
 * - Smooth animated needle movement
 * - Color-coded pressure zones (safe, warning, critical)
 * - Large, readable text displays
 * - Touch-friendly design for 50-inch displays
 * - Customizable ranges and thresholds
 */
class PressureGauge : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double targetValue READ targetValue WRITE setTargetValue)

public:
    explicit PressureGauge(QWidget *parent = nullptr);
    ~PressureGauge();

    // Value properties
    double value() const { return m_currentValue; }
    double targetValue() const { return m_targetValue; }
    
    // Range configuration
    void setRange(double minimum, double maximum);
    double minimum() const { return m_minimum; }
    double maximum() const { return m_maximum; }
    
    // Threshold configuration
    void setWarningThreshold(double threshold);
    void setCriticalThreshold(double threshold);
    double warningThreshold() const { return m_warningThreshold; }
    double criticalThreshold() const { return m_criticalThreshold; }
    
    // Display configuration
    void setTitle(const QString& title);
    void setUnits(const QString& units);
    void setPrecision(int decimals);
    void setShowValue(bool show);
    void setShowThresholds(bool show);
    
    // Animation configuration
    void setAnimationEnabled(bool enabled);
    void setAnimationDuration(int ms);
    
    // Color configuration
    void setSafeColor(const QColor& color);
    void setWarningColor(const QColor& color);
    void setCriticalColor(const QColor& color);
    void setNeedleColor(const QColor& color);
    void setBackgroundColor(const QColor& color);

public Q_SLOTS:
    void setValue(double value);
    void setTargetValue(double value);
    void resetValue();

Q_SIGNALS:
    void valueChanged(double value);
    void thresholdExceeded(double value, const QString& level);
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private Q_SLOTS:
    void updateAnimation();

private:
    void drawBackground(QPainter& painter);
    void drawScale(QPainter& painter);
    void drawThresholds(QPainter& painter);
    void drawNeedle(QPainter& painter);
    void drawValue(QPainter& painter);
    void drawTitle(QPainter& painter);
    
    QColor getValueColor(double value) const;
    double valueToAngle(double value) const;
    QPointF getPointOnCircle(double angle, double radius) const;
    void updateGaugeGeometry();
    
    // Value properties
    double m_currentValue;
    double m_targetValue;
    double m_minimum;
    double m_maximum;
    
    // Thresholds
    double m_warningThreshold;
    double m_criticalThreshold;
    
    // Display properties
    QString m_title;
    QString m_units;
    int m_precision;
    bool m_showValue;
    bool m_showThresholds;
    
    // Animation
    QPropertyAnimation* m_animation;
    bool m_animationEnabled;
    int m_animationDuration;
    
    // Colors
    QColor m_safeColor;
    QColor m_warningColor;
    QColor m_criticalColor;
    QColor m_needleColor;
    QColor m_backgroundColor;
    QColor m_textColor;
    
    // Geometry
    QRectF m_gaugeRect;
    QPointF m_center;
    double m_radius;
    double m_needleLength;
    double m_needleWidth;
    
    // Constants
    static const double START_ANGLE;      // -135 degrees
    static const double SPAN_ANGLE;       // 270 degrees
    static const int DEFAULT_SIZE = 200;
    static const int MIN_SIZE = 100;
    static const int DEFAULT_ANIMATION_DURATION = 500;
    static const int DEFAULT_PRECISION = 1;
};

#endif // PRESSUREGAUGE_H
