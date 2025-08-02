#ifndef STATUSINDICATOR_H
#define STATUSINDICATOR_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPainter>

/**
 * @brief Status indicator widget for system monitoring
 * 
 * This widget provides visual status indication with:
 * - Color-coded status levels (OK, Warning, Critical, Error)
 * - Animated indicators (blinking, pulsing)
 * - Text and icon display
 * - Large, readable design for 50-inch displays
 * - Customizable appearance and behavior
 */
class StatusIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(StatusLevel status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)

public:
    enum StatusLevel {
        OK,
        INFO,
        WARNING,
        CRITICAL,
        ERROR,
        OFFLINE
    };
    Q_ENUM(StatusLevel)

    enum IndicatorStyle {
        Circle,
        Square,
        LED,
        Bar
    };
    Q_ENUM(IndicatorStyle)

    explicit StatusIndicator(QWidget *parent = nullptr);
    explicit StatusIndicator(const QString& text, QWidget *parent = nullptr);
    explicit StatusIndicator(const QString& text, StatusLevel status, QWidget *parent = nullptr);
    ~StatusIndicator();

    // Status properties
    StatusLevel status() const { return m_status; }
    void setStatus(StatusLevel status);
    void setStatus(StatusLevel status, const QString& text);
    
    // Text and display
    void setText(const QString& text);
    QString text() const { return m_text; }
    
    void setDescription(const QString& description);
    QString description() const { return m_description; }
    
    // Visual configuration
    void setIndicatorStyle(IndicatorStyle style);
    IndicatorStyle indicatorStyle() const { return m_indicatorStyle; }
    
    void setIndicatorSize(const QSize& size);
    QSize indicatorSize() const { return m_indicatorSize; }
    
    // Animation
    bool isAnimated() const { return m_animated; }
    void setAnimated(bool animated);
    void setAnimationEnabled(bool enabled);
    
    void setBlinkEnabled(bool enabled);
    bool isBlinkEnabled() const { return m_blinkEnabled; }
    
    void setPulseEnabled(bool enabled);
    bool isPulseEnabled() const { return m_pulseEnabled; }
    
    // Layout configuration
    void setHorizontalLayout(bool horizontal);
    bool isHorizontalLayout() const { return m_horizontalLayout; }
    
    // Color customization
    void setStatusColor(StatusLevel status, const QColor& color);
    QColor getStatusColor(StatusLevel status) const;

public slots:
    void updateStatus(StatusLevel status, const QString& message = QString());
    void clearStatus();
    void startAnimation();
    void stopAnimation();

signals:
    void statusChanged(StatusLevel status);
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private slots:
    void onBlinkTimer();
    void onPulseAnimation();

private:
    void setupUI();
    void setupAnimations();
    void setupIndicator();
    void updateIndicatorAppearance();
    void updateTextAppearance();
    void updateColors();
    void setBlinkingEnabled(bool enabled);
    
    void drawCircleIndicator(QPainter& painter, const QRect& rect);
    void drawSquareIndicator(QPainter& painter, const QRect& rect);
    void drawLEDIndicator(QPainter& painter, const QRect& rect);
    void drawBarIndicator(QPainter& painter, const QRect& rect);
    
    QColor getCurrentStatusColor() const;
    QString getStatusText(StatusLevel status) const;
    
    // Status properties
    StatusLevel m_status;
    QString m_text;
    QString m_description;
    
    // Visual properties
    IndicatorStyle m_indicatorStyle;
    QSize m_indicatorSize;
    bool m_horizontalLayout;
    
    // Animation properties
    bool m_animated;
    bool m_animationEnabled;
    bool m_blinkEnabled;
    bool m_pulseEnabled;
    QTimer* m_blinkTimer;
    QPropertyAnimation* m_pulseAnimation;
    QPropertyAnimation* m_colorAnimation;
    bool m_blinkState;
    double m_pulseOpacity;
    
    // UI components
    QLabel* m_textLabel;
    QLabel* m_descriptionLabel;
    QHBoxLayout* m_horizontalLayout_ui;
    QVBoxLayout* m_verticalLayout;
    
    // Colors for different status levels
    QMap<StatusLevel, QColor> m_statusColors;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_borderColor;
    
    // Constants
    static const QSize DEFAULT_INDICATOR_SIZE;
    static const QSize LARGE_INDICATOR_SIZE;
    static const int BLINK_INTERVAL = 500;  // 500ms
    static const int PULSE_DURATION = 1000; // 1 second
    
    // Default colors
    static const QColor OK_COLOR;
    static const QColor INFO_COLOR;
    static const QColor WARNING_COLOR;
    static const QColor CRITICAL_COLOR;
    static const QColor ERROR_COLOR;
    static const QColor OFFLINE_COLOR;
};

/**
 * @brief Multi-status indicator for displaying multiple system states
 */
class MultiStatusIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit MultiStatusIndicator(QWidget *parent = nullptr);
    ~MultiStatusIndicator();

    // Status management
    void addStatus(const QString& name, const QString& label, StatusIndicator::StatusLevel initialStatus = StatusIndicator::OK);
    void removeStatus(const QString& name);
    void updateStatus(const QString& name, StatusIndicator::StatusLevel status, const QString& message = QString());
    
    // Layout
    void setColumns(int columns);
    int columns() const { return m_columns; }
    
    // Access individual indicators
    StatusIndicator* getIndicator(const QString& name) const;
    QStringList getStatusNames() const;

signals:
    void statusClicked(const QString& name, StatusIndicator::StatusLevel status);

private slots:
    void onIndicatorClicked();

private:
    void updateLayout();
    
    QMap<QString, StatusIndicator*> m_indicators;
    QGridLayout* m_gridLayout;
    int m_columns;
    
    static const int DEFAULT_COLUMNS = 2;
};

#endif // STATUSINDICATOR_H
