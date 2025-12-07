#ifndef TOUCHBUTTON_H
#define TOUCHBUTTON_H

#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QTouchEvent>

/**
 * @brief Touch-optimized button for large display interfaces
 * 
 * This button is specifically designed for 50-inch touch displays with:
 * - Large, easily tappable areas
 * - Visual and haptic feedback
 * - Customizable appearance for different button types
 * - Animation effects for better user experience
 * - Support for long press actions
 * - Emergency button styling
 */
class TouchButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(ButtonType buttonType READ buttonType WRITE setButtonType)
    Q_PROPERTY(bool glowEffect READ hasGlowEffect WRITE setGlowEffect)

public:
    enum ButtonType {
        Normal,
        Primary,
        Success,
        Warning,
        Danger,
        Emergency
    };
    Q_ENUM(ButtonType)

    explicit TouchButton(QWidget *parent = nullptr);
    explicit TouchButton(const QString& text, QWidget *parent = nullptr);
    explicit TouchButton(const QString& text, ButtonType type, QWidget *parent = nullptr);
    ~TouchButton();

    // Button type and appearance
    ButtonType buttonType() const { return m_buttonType; }
    void setButtonType(ButtonType type);
    
    // Visual effects
    bool hasGlowEffect() const { return m_glowEffect; }
    void setGlowEffect(bool enabled);
    
    void setPulseEffect(bool enabled);
    bool hasPulseEffect() const { return m_pulseEffect; }
    
    // Touch configuration
    void setLongPressEnabled(bool enabled);
    void setLongPressDelay(int ms);
    bool isLongPressEnabled() const { return m_longPressEnabled; }
    
    // Size configuration for touch interface
    void setTouchSize(const QSize& size);
    QSize touchSize() const { return m_touchSize; }
    
    // Feedback configuration
    void setHapticFeedback(bool enabled);
    void setSoundFeedback(bool enabled);
    bool hasHapticFeedback() const { return m_hapticFeedback; }
    bool hasSoundFeedback() const { return m_soundFeedback; }

    // Animation configuration
    void setAnimationEnabled(bool enabled);
    bool isAnimationEnabled() const { return m_animationEnabled; }

    // Touch feedback configuration
    void setTouchFeedbackEnabled(bool enabled);
    bool isTouchFeedbackEnabled() const { return m_touchFeedbackEnabled; }

    // Size hints
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public Q_SLOTS:
    void startPulse();
    void stopPulse();
    void flashButton(int count = 3);

Q_SIGNALS:
    void longPressed();
    void touchStarted();
    void touchEnded();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool event(QEvent *event) override;

private Q_SLOTS:
    void onLongPressTimer();
    void onPulseTimer();
    void onFlashTimer();

private:
    void setupButton();
    void setupAnimations();
    void setupEffects();
    void applyButtonStyle();
    void updateButtonStyle();
    void updateButtonColors();
    void provideFeedback();
    
    QString getButtonStyleSheet() const;
    QColor getButtonColor() const;
    QColor getHoverColor() const;
    QColor getPressedColor() const;
    
    // Button properties
    ButtonType m_buttonType;
    QSize m_touchSize;
    
    // Visual effects
    bool m_glowEffect;
    bool m_pulseEffect;
    QGraphicsDropShadowEffect* m_shadowEffect;
    QPropertyAnimation* m_pulseAnimation;
    QPropertyAnimation* m_pressAnimation;
    QPropertyAnimation* m_colorAnimation;
    
    // Touch and interaction
    bool m_longPressEnabled;
    int m_longPressDelay;
    QTimer* m_longPressTimer;
    bool m_touchActive;
    
    // Feedback
    bool m_hapticFeedback;
    bool m_soundFeedback;
    bool m_touchFeedbackEnabled;
    bool m_animationEnabled;
    
    // Flash effect
    QTimer* m_flashTimer;
    int m_flashCount;
    int m_currentFlash;
    bool m_flashState;
    
    // Colors for different button types
    struct ButtonColors {
        QColor normal;
        QColor hover;
        QColor pressed;
        QColor text;
        QColor border;
    };
    
    static const ButtonColors NORMAL_COLORS;
    static const ButtonColors PRIMARY_COLORS;
    static const ButtonColors SUCCESS_COLORS;
    static const ButtonColors WARNING_COLORS;
    static const ButtonColors DANGER_COLORS;
    static const ButtonColors EMERGENCY_COLORS;
    
    // Constants
    static const int DEFAULT_TOUCH_WIDTH = 120;
    static const int DEFAULT_TOUCH_HEIGHT = 60;
    static const int LARGE_TOUCH_WIDTH = 200;
    static const int LARGE_TOUCH_HEIGHT = 80;
    static const int EMERGENCY_TOUCH_SIZE = 150;
    static const int DEFAULT_LONG_PRESS_DELAY = 1000;  // 1 second
    static const int PULSE_DURATION = 1000;
    static const int PRESS_ANIMATION_DURATION = 100;
    static const int FLASH_INTERVAL = 200;
};

#endif // TOUCHBUTTON_H
