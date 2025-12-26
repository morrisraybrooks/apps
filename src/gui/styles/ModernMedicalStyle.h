#ifndef MODERNMEDICALSTYLE_H
#define MODERNMEDICALSTYLE_H

#include <QString>
#include <QColor>
#include <QFont>
#include <QApplication>
#include <QScreen>
#include <QWidget>

/**
 * @brief Modern medical device styling system
 * 
 * This class provides a comprehensive styling system optimized for:
 * - High-resolution medical displays (50-inch and larger)
 * - Touch-friendly interfaces with proper sizing
 * - Medical device color schemes with high contrast
 * - Modern flat design with subtle depth
 * - Accessibility and readability standards
 */
class ModernMedicalStyle
{
public:
    // Color Palette - Medical Device Optimized
    struct Colors {
        // Primary colors
        static const QColor PRIMARY_BLUE;           // #1976D2 - Main accent
        static const QColor PRIMARY_BLUE_LIGHT;     // #42A5F5 - Light variant
        static const QColor PRIMARY_BLUE_DARK;      // #0D47A1 - Dark variant
        
        // Medical status colors
        static const QColor MEDICAL_GREEN;          // #2E7D32 - Safe/Normal
        static const QColor MEDICAL_ORANGE;         // #F57C00 - Warning
        static const QColor MEDICAL_RED;            // #C62828 - Critical/Emergency
        static const QColor MEDICAL_PURPLE;         // #6A1B9A - Special/Info
        
        // Neutral colors
        static const QColor BACKGROUND_LIGHT;       // #FAFAFA - Main background
        static const QColor BACKGROUND_MEDIUM;      // #F5F5F5 - Secondary background
        static const QColor BACKGROUND_DARK;        // #EEEEEE - Tertiary background
        
        // Text colors
        static const QColor TEXT_PRIMARY;           // #212121 - Main text
        static const QColor TEXT_SECONDARY;         // #757575 - Secondary text
        static const QColor TEXT_DISABLED;          // #BDBDBD - Disabled text
        static const QColor TEXT_ON_PRIMARY;        // #FFFFFF - Text on colored backgrounds
        
        // Border and divider colors
        static const QColor BORDER_LIGHT;           // #E0E0E0 - Light borders
        static const QColor BORDER_MEDIUM;          // #BDBDBD - Medium borders
        static const QColor BORDER_DARK;            // #9E9E9E - Dark borders
        
        // Shadow colors
        static const QColor SHADOW_LIGHT;           // rgba(0,0,0,0.1) - Light shadows
        static const QColor SHADOW_MEDIUM;          // rgba(0,0,0,0.2) - Medium shadows
        static const QColor SHADOW_DARK;            // rgba(0,0,0,0.3) - Dark shadows
    };
    
    // Typography - High-DPI Optimized
    struct Typography {
        // Font families
        static const QString PRIMARY_FONT;          // "Segoe UI" or system default
        static const QString MONOSPACE_FONT;        // "Consolas" or monospace
        
        // Font sizes (scaled for high-DPI)
        static int getDisplayTitle();               // 32pt - Main titles
        static int getDisplaySubtitle();            // 24pt - Subtitles
        static int getHeadline();                   // 20pt - Section headers
        static int getTitle();                      // 18pt - Component titles
        static int getSubtitle();                   // 16pt - Subtitles
        static int getBody();                       // 14pt - Body text
        static int getCaption();                    // 12pt - Captions
        static int getButton();                     // 16pt - Button text
        
        // Font weights
        static const int WEIGHT_LIGHT = 300;
        static const int WEIGHT_NORMAL = 400;
        static const int WEIGHT_MEDIUM = 500;
        static const int WEIGHT_BOLD = 700;
    };
    
    // Spacing and Sizing - Touch Optimized
    struct Spacing {
        // Base spacing unit (8px scaled for DPI)
        static int getBaseUnit();
        
        // Common spacing values
        static int getXSmall();                     // 4px scaled
        static int getSmall();                      // 8px scaled
        static int getMedium();                     // 16px scaled
        static int getLarge();                      // 24px scaled
        static int getXLarge();                     // 32px scaled
        static int getXXLarge();                    // 48px scaled
        
        // Touch target sizes
        static int getMinTouchTarget();             // 44px minimum
        static int getRecommendedTouchTarget();     // 60px recommended
        static int getLargeTouchTarget();           // 80px for primary actions
        
        // Border radius
        static int getSmallRadius();                // 4px scaled
        static int getMediumRadius();               // 8px scaled
        static int getLargeRadius();                // 12px scaled
        static int getCircularRadius();             // 50% for circular elements
    };
    
    // Elevation and Shadows
    struct Elevation {
        static QString getLevel1();                 // Subtle elevation
        static QString getLevel2();                 // Card elevation
        static QString getLevel3();                 // Modal elevation
        static QString getLevel4();                 // Navigation elevation
        static QString getLevel5();                 // Floating action button
    };
    
    // Animation and Transitions
    struct Animation {
        static const int FAST_DURATION = 150;      // Quick transitions
        static const int NORMAL_DURATION = 250;    // Standard transitions
        static const int SLOW_DURATION = 400;      // Slow transitions
        
        static QString getEaseInOut();              // Standard easing
        static QString getEaseOut();                // Exit easing
        static QString getEaseIn();                 // Enter easing
    };
    
    // Static methods for applying styles
    static void initialize(QApplication* app);
    static void applyToWidget(QWidget* widget);
    static double getScaleFactor();
    static void setScaleFactor(double factor);

    // Helper methods (made public for external use)
    static int scaleValue(int baseValue);
    static QString scalePixelValue(int baseValue);
    static QColor adjustColorForContrast(const QColor& color, double factor = 1.0);
    
    // Style sheet generators
    static QString getButtonStyle(const QString& type = "primary");
    static QString getLabelStyle(const QString& type = "body");
    static QString getGroupBoxStyle();
    static QString getGroupBoxStyle(const QColor& titleColor);  // Overload with custom title color
    static QString getFrameStyle();
    static QString getScrollAreaStyle();
    static QString getProgressBarStyle();
    static QString getComboBoxStyle();
    static QString getSpinBoxStyle();
    static QString getTableStyle();
    static QString getInputFieldStyle();
    static QString getListWidgetStyle();
    static QString getTabWidgetStyle();
    
    // Medical device specific styles
    static QString getPressureDisplayStyle();
    static QString getStatusIndicatorStyle();
    static QString getEmergencyButtonStyle();
    static QString getSafetyPanelStyle();
    static QString getPatternSelectorStyle();
    
private:
    static double s_scaleFactor;
    static bool s_initialized;
};

#endif // MODERNMEDICALSTYLE_H
