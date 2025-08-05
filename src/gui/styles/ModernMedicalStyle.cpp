#include "ModernMedicalStyle.h"
#include <QScreen>
#include <QGuiApplication>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QDebug>
#include <cmath>

// Static member initialization
double ModernMedicalStyle::s_scaleFactor = 1.0;
bool ModernMedicalStyle::s_initialized = false;

// Color definitions - Medical Device Optimized with High Contrast
const QColor ModernMedicalStyle::Colors::PRIMARY_BLUE(21, 101, 192);           // #1565C0 - Darker for better contrast
const QColor ModernMedicalStyle::Colors::PRIMARY_BLUE_LIGHT(25, 118, 210);     // #1976D2 - Original primary as light
const QColor ModernMedicalStyle::Colors::PRIMARY_BLUE_DARK(13, 71, 161);       // #0D47A1

const QColor ModernMedicalStyle::Colors::MEDICAL_GREEN(27, 94, 32);            // #1B5E20 - Darker for better contrast
const QColor ModernMedicalStyle::Colors::MEDICAL_ORANGE(230, 81, 0);           // #E65100 - Darker for better contrast
const QColor ModernMedicalStyle::Colors::MEDICAL_RED(183, 28, 28);             // #B71C1C - Darker for better contrast
const QColor ModernMedicalStyle::Colors::MEDICAL_PURPLE(74, 20, 140);          // #4A148C - Darker for better contrast

const QColor ModernMedicalStyle::Colors::BACKGROUND_LIGHT(250, 250, 250);      // #FAFAFA
const QColor ModernMedicalStyle::Colors::BACKGROUND_MEDIUM(245, 245, 245);     // #F5F5F5
const QColor ModernMedicalStyle::Colors::BACKGROUND_DARK(238, 238, 238);       // #EEEEEE

const QColor ModernMedicalStyle::Colors::TEXT_PRIMARY(33, 33, 33);             // #212121
const QColor ModernMedicalStyle::Colors::TEXT_SECONDARY(97, 97, 97);           // #616161 - Darker for better contrast
const QColor ModernMedicalStyle::Colors::TEXT_DISABLED(158, 158, 158);         // #9E9E9E - Darker for better contrast
const QColor ModernMedicalStyle::Colors::TEXT_ON_PRIMARY(255, 255, 255);       // #FFFFFF

const QColor ModernMedicalStyle::Colors::BORDER_LIGHT(224, 224, 224);          // #E0E0E0
const QColor ModernMedicalStyle::Colors::BORDER_MEDIUM(189, 189, 189);         // #BDBDBD
const QColor ModernMedicalStyle::Colors::BORDER_DARK(158, 158, 158);           // #9E9E9E

const QColor ModernMedicalStyle::Colors::SHADOW_LIGHT(0, 0, 0, 25);            // rgba(0,0,0,0.1)
const QColor ModernMedicalStyle::Colors::SHADOW_MEDIUM(0, 0, 0, 51);           // rgba(0,0,0,0.2)
const QColor ModernMedicalStyle::Colors::SHADOW_DARK(0, 0, 0, 76);             // rgba(0,0,0,0.3)

// Typography definitions
const QString ModernMedicalStyle::Typography::PRIMARY_FONT = "Segoe UI, Arial, sans-serif";
const QString ModernMedicalStyle::Typography::MONOSPACE_FONT = "Consolas, Monaco, monospace";

void ModernMedicalStyle::initialize(QApplication* app)
{
    if (s_initialized) return;

    // Set a baseline DPI for scaling calculations
    const double baselineDpi = 96.0;

    // Detect display characteristics and set scale factor
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geometry = screen->geometry();
        qreal dpi = screen->logicalDotsPerInch();

        // Use a much smaller scale factor for optimal space utilization on large displays
        double baseScale = 0.64;  // 25% reduction from 0.85x for better information density

        // Adjust slightly based on resolution but keep it compact
        if (geometry.width() >= 3840) {          // 4K and above
            baseScale = 0.68;  // Slightly larger for 4K but still compact
        } else if (geometry.width() >= 2560) {   // QHD
            baseScale = 0.66;  // Slightly larger for QHD but still compact
        } else if (geometry.width() >= 1920) {   // Full HD
            baseScale = 0.64;  // Compact for Full HD - optimal for 50-inch
        } else {
            baseScale = 0.62;  // Even more compact for lower resolutions
        }

        // Apply DPI adjustment
        if (dpi > 120) {
            baseScale *= (dpi / baselineDpi) * 0.9;
        }

        s_scaleFactor = baseScale;

        qDebug() << "Display Scaling:";
        qDebug() << "  Resolution:" << geometry.width() << "x" << geometry.height();
        qDebug() << "  DPI:" << dpi;
        qDebug() << "  Final Scale Factor:" << s_scaleFactor;
    } else {
        // Fallback if no screen is found
        s_scaleFactor = 0.64;  // Default compact scale for better space utilization
    }

    // Set application style
    app->setStyle(QStyleFactory::create("Fusion"));

    // Set application font
    QFont appFont(Typography::PRIMARY_FONT);
    appFont.setPointSize(Typography::getBody());
    appFont.setWeight(Typography::WEIGHT_NORMAL);
    app->setFont(appFont);

    s_initialized = true;
}

double ModernMedicalStyle::getScaleFactor()
{
    return s_scaleFactor;
}

void ModernMedicalStyle::setScaleFactor(double factor)
{
    s_scaleFactor = qMax(0.5, qMin(4.0, factor));  // Clamp between 0.5x and 4x
}

int ModernMedicalStyle::scaleValue(int baseValue)
{
    return static_cast<int>(std::round(baseValue * s_scaleFactor));
}

QString ModernMedicalStyle::scalePixelValue(int baseValue)
{
    return QString("%1px").arg(scaleValue(baseValue));
}

// Typography methods - Reasonable base sizes
int ModernMedicalStyle::Typography::getDisplayTitle() { return scaleValue(32); }
int ModernMedicalStyle::Typography::getDisplaySubtitle() { return scaleValue(24); }
int ModernMedicalStyle::Typography::getHeadline() { return scaleValue(20); }
int ModernMedicalStyle::Typography::getTitle() { return scaleValue(18); }
int ModernMedicalStyle::Typography::getSubtitle() { return scaleValue(16); }
int ModernMedicalStyle::Typography::getBody() { return scaleValue(14); }
int ModernMedicalStyle::Typography::getCaption() { return scaleValue(12); }
int ModernMedicalStyle::Typography::getButton() { return scaleValue(16); }

// Spacing methods
int ModernMedicalStyle::Spacing::getBaseUnit() { return scaleValue(8); }
int ModernMedicalStyle::Spacing::getXSmall() { return scaleValue(4); }
int ModernMedicalStyle::Spacing::getSmall() { return scaleValue(8); }
int ModernMedicalStyle::Spacing::getMedium() { return scaleValue(16); }
int ModernMedicalStyle::Spacing::getLarge() { return scaleValue(24); }
int ModernMedicalStyle::Spacing::getXLarge() { return scaleValue(32); }
int ModernMedicalStyle::Spacing::getXXLarge() { return scaleValue(48); }

int ModernMedicalStyle::Spacing::getMinTouchTarget() { return scaleValue(44); }
int ModernMedicalStyle::Spacing::getRecommendedTouchTarget() { return scaleValue(60); }
int ModernMedicalStyle::Spacing::getLargeTouchTarget() { return scaleValue(80); }

int ModernMedicalStyle::Spacing::getSmallRadius() { return scaleValue(4); }
int ModernMedicalStyle::Spacing::getMediumRadius() { return scaleValue(8); }
int ModernMedicalStyle::Spacing::getLargeRadius() { return scaleValue(12); }
int ModernMedicalStyle::Spacing::getCircularRadius() { return 9999; } // Large value for circular

// Elevation methods - Using Qt-native border styling instead of unsupported box-shadow
QString ModernMedicalStyle::Elevation::getLevel1()
{
    return QString("border: %1 solid %2;")
           .arg(scalePixelValue(1))
           .arg(Colors::BORDER_LIGHT.name());
}

QString ModernMedicalStyle::Elevation::getLevel2()
{
    return QString("border: %1 solid %2;")
           .arg(scalePixelValue(2))
           .arg(Colors::BORDER_MEDIUM.name());
}

QString ModernMedicalStyle::Elevation::getLevel3()
{
    return QString("border: %1 solid %2;")
           .arg(scalePixelValue(3))
           .arg(Colors::PRIMARY_BLUE.name());
}

QString ModernMedicalStyle::Elevation::getLevel4()
{
    return QString("border: %1 solid %2;")
           .arg(scalePixelValue(4))
           .arg(Colors::PRIMARY_BLUE_DARK.name());
}

QString ModernMedicalStyle::Elevation::getLevel5()
{
    return QString("border: %1 solid %2;")
           .arg(scalePixelValue(5))
           .arg(Colors::PRIMARY_BLUE_DARK.name());
}

// Animation methods
QString ModernMedicalStyle::Animation::getEaseInOut() { return "cubic-bezier(0.4, 0.0, 0.2, 1)"; }
QString ModernMedicalStyle::Animation::getEaseOut() { return "cubic-bezier(0.0, 0.0, 0.2, 1)"; }
QString ModernMedicalStyle::Animation::getEaseIn() { return "cubic-bezier(0.4, 0.0, 1, 1)"; }

void ModernMedicalStyle::applyToWidget(QWidget* widget)
{
    if (!widget) return;
    
    // Apply base styling to widget
    widget->setFont(QFont(Typography::PRIMARY_FONT, Typography::getBody()));
}

QColor ModernMedicalStyle::adjustColorForContrast(const QColor& color, double factor)
{
    int h, s, l, a;
    color.getHsl(&h, &s, &l, &a);

    // Adjust lightness for better contrast
    l = qBound(0, static_cast<int>(l * factor), 255);

    return QColor::fromHsl(h, s, l, a);
}

// Style sheet generators
QString ModernMedicalStyle::getButtonStyle(const QString& type)
{
    QColor bgColor, hoverColor, pressedColor, textColor, borderColor;

    if (type == "primary") {
        bgColor = Colors::PRIMARY_BLUE;
        hoverColor = Colors::PRIMARY_BLUE_LIGHT;
        pressedColor = Colors::PRIMARY_BLUE_DARK;
        textColor = Colors::TEXT_ON_PRIMARY;
        borderColor = Colors::PRIMARY_BLUE;
    } else if (type == "success") {
        bgColor = Colors::MEDICAL_GREEN;
        hoverColor = adjustColorForContrast(Colors::MEDICAL_GREEN, 1.1);
        pressedColor = adjustColorForContrast(Colors::MEDICAL_GREEN, 0.9);
        textColor = Colors::TEXT_ON_PRIMARY;
        borderColor = Colors::MEDICAL_GREEN;
    } else if (type == "warning") {
        bgColor = Colors::MEDICAL_ORANGE;
        hoverColor = adjustColorForContrast(Colors::MEDICAL_ORANGE, 1.1);
        pressedColor = adjustColorForContrast(Colors::MEDICAL_ORANGE, 0.9);
        textColor = Colors::TEXT_ON_PRIMARY;
        borderColor = Colors::MEDICAL_ORANGE;
    } else if (type == "danger") {
        bgColor = Colors::MEDICAL_RED;
        hoverColor = adjustColorForContrast(Colors::MEDICAL_RED, 1.1);
        pressedColor = adjustColorForContrast(Colors::MEDICAL_RED, 0.9);
        textColor = Colors::TEXT_ON_PRIMARY;
        borderColor = Colors::MEDICAL_RED;
    } else { // secondary/default
        bgColor = Colors::BACKGROUND_LIGHT;
        hoverColor = Colors::BACKGROUND_MEDIUM;
        pressedColor = Colors::BACKGROUND_DARK;
        textColor = Colors::TEXT_PRIMARY;
        borderColor = Colors::BORDER_MEDIUM;
    }

    return QString(
        "QPushButton {"
        "    background-color: %1;"
        "    border: %2 solid %3;"
        "    border-radius: %4;"
        "    color: %5;"
        "    font-family: %6;"
        "    font-size: %7pt;"
        "    font-weight: %8;"
        "    padding: %9 %10;"
        "    min-height: %11;"
        "    min-width: %12;"
        "    text-align: center;"
        "}"
        "QPushButton:hover {"
        "    background-color: %14;"
        "    border-color: %15;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %16;"
        "    border-color: %17;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %18;"
        "    color: %19;"
        "    border-color: %20;"
        "}"
    ).arg(bgColor.name())
     .arg(scalePixelValue(2))
     .arg(borderColor.name())
     .arg(scalePixelValue(Spacing::getMediumRadius()))
     .arg(textColor.name())
     .arg(Typography::PRIMARY_FONT)
     .arg(Typography::getButton())
     .arg(Typography::WEIGHT_MEDIUM)
     .arg(scalePixelValue(Spacing::getMedium()))
     .arg(scalePixelValue(Spacing::getLarge()))
     .arg(scalePixelValue(Spacing::getRecommendedTouchTarget()))
     .arg(scalePixelValue(120))
     .arg(hoverColor.name())
     .arg(borderColor.name())
     .arg(pressedColor.name())
     .arg(borderColor.name())
     .arg(Colors::BACKGROUND_MEDIUM.name())
     .arg(Colors::TEXT_DISABLED.name())
     .arg(Colors::BORDER_LIGHT.name());
}

QString ModernMedicalStyle::getLabelStyle(const QString& type)
{
    int fontSize = Typography::getBody();
    int fontWeight = Typography::WEIGHT_NORMAL;
    QColor textColor = Colors::TEXT_PRIMARY;

    if (type == "title") {
        fontSize = Typography::getTitle();
        fontWeight = Typography::WEIGHT_MEDIUM;
    } else if (type == "subtitle") {
        fontSize = Typography::getSubtitle();
        fontWeight = Typography::WEIGHT_MEDIUM;
    } else if (type == "headline") {
        fontSize = Typography::getHeadline();
        fontWeight = Typography::WEIGHT_BOLD;
    } else if (type == "display-title") {
        fontSize = Typography::getDisplayTitle();
        fontWeight = Typography::WEIGHT_BOLD;
    } else if (type == "caption") {
        fontSize = Typography::getCaption();
        textColor = Colors::TEXT_SECONDARY;
    } else if (type == "secondary") {
        textColor = Colors::TEXT_SECONDARY;
    }

    return QString(
        "QLabel {"
        "    color: %1;"
        "    font-family: %2;"
        "    font-size: %3pt;"
        "    font-weight: %4;"
        "    line-height: 1.4;"
        "}"
    ).arg(textColor.name())
     .arg(Typography::PRIMARY_FONT)
     .arg(fontSize)
     .arg(fontWeight);
}

QString ModernMedicalStyle::getGroupBoxStyle()
{
    return QString(
        "QGroupBox {"
        "    font-family: %1;"
        "    font-size: %2pt;"
        "    font-weight: %3;"
        "    color: %4;"
        "    border: %5 solid %6;"
        "    border-radius: %7;"
        "    margin-top: %8;"
        "    padding-top: %9;"
        "    background-color: %10;"
        "    %11"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: %12;"
        "    padding: 0 %13 0 %13;"
        "    background-color: %14;"
        "    border-radius: %15;"
        "}"
    ).arg(Typography::PRIMARY_FONT)
     .arg(Typography::getSubtitle())
     .arg(Typography::WEIGHT_MEDIUM)
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(scalePixelValue(2))
     .arg(Colors::BORDER_LIGHT.name())
     .arg(scalePixelValue(Spacing::getMediumRadius()))
     .arg(scalePixelValue(Spacing::getMedium()))
     .arg(scalePixelValue(Spacing::getLarge()))
     .arg(Colors::BACKGROUND_LIGHT.name())
     .arg(Elevation::getLevel1())
     .arg(scalePixelValue(Spacing::getMedium()))
     .arg(scalePixelValue(Spacing::getSmall()))
     .arg(Colors::BACKGROUND_LIGHT.name())
     .arg(scalePixelValue(Spacing::getSmallRadius()));
}

QString ModernMedicalStyle::getFrameStyle()
{
    return QString(
        "QFrame {"
        "    background-color: %1;"
        "    border: %2 solid %3;"
        "    border-radius: %4;"
        "    %5"
        "}"
    ).arg(Colors::BACKGROUND_LIGHT.name())
     .arg(scalePixelValue(1))
     .arg(Colors::BORDER_LIGHT.name())
     .arg(scalePixelValue(Spacing::getMediumRadius()))
     .arg(Elevation::getLevel1());
}

QString ModernMedicalStyle::getPressureDisplayStyle()
{
    return QString(
        ".pressure-display {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 %1, stop:1 %2);"
        "    border: %3 solid %4;"
        "    border-radius: %5;"
        "    color: %6;"
        "    font-family: %7;"
        "    font-size: %8pt;"
        "    font-weight: %9;"
        "    padding: %10;"
        "    text-align: center;"
        "    %11"
        "}"
    ).arg(adjustColorForContrast(Colors::PRIMARY_BLUE, 0.1).name())
     .arg(adjustColorForContrast(Colors::PRIMARY_BLUE, 0.05).name())
     .arg(scalePixelValue(3))
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(scalePixelValue(Spacing::getLargeRadius()))
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(Typography::MONOSPACE_FONT)
     .arg(Typography::getDisplayTitle())
     .arg(Typography::WEIGHT_BOLD)
     .arg(scalePixelValue(Spacing::getLarge()))
     .arg(Elevation::getLevel3());
}

QString ModernMedicalStyle::getEmergencyButtonStyle()
{
    return QString(
        ".emergency-button {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
        "                fx:0.3, fy:0.3, stop:0 %1, stop:1 %2);"
        "    border: %3 solid %4;"
        "    border-radius: %5;"
        "    color: %6;"
        "    font-family: %7;"
        "    font-size: %8pt;"
        "    font-weight: %9;"
        "    min-height: %10;"
        "    min-width: %10;"
        "    %11"
        "}"
        ".emergency-button:hover {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
        "                fx:0.3, fy:0.3, stop:0 %12, stop:1 %13);"
        "}"
        ".emergency-button:pressed {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
        "                fx:0.3, fy:0.3, stop:0 %14, stop:1 %15);"
        "}"
    ).arg(adjustColorForContrast(Colors::MEDICAL_RED, 1.2).name())
     .arg(Colors::MEDICAL_RED.name())
     .arg(scalePixelValue(4))
     .arg(adjustColorForContrast(Colors::MEDICAL_RED, 0.8).name())
     .arg(scalePixelValue(Spacing::getCircularRadius()))
     .arg(Colors::TEXT_ON_PRIMARY.name())
     .arg(Typography::PRIMARY_FONT)
     .arg(Typography::getTitle())
     .arg(Typography::WEIGHT_BOLD)
     .arg(scalePixelValue(Spacing::getLargeTouchTarget() * 2))
     .arg(Elevation::getLevel4())
     .arg(adjustColorForContrast(Colors::MEDICAL_RED, 1.3).name())
     .arg(adjustColorForContrast(Colors::MEDICAL_RED, 1.1).name())
     .arg(adjustColorForContrast(Colors::MEDICAL_RED, 0.9).name())
     .arg(adjustColorForContrast(Colors::MEDICAL_RED, 0.8).name());
}

QString ModernMedicalStyle::getInputFieldStyle()
{
    return QString(
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "    font-size: %1pt;"
        "    padding: %2px;"
        "    border: %3px solid %4;"
        "    border-radius: %5px;"
        "    background-color: %6;"
        "    color: %7;"
        "    font-family: %8;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
        "    border-color: %9;"
        "}"
        "QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {"
        "    background-color: %10;"
        "    color: %11;"
        "    border-color: %12;"
        "}"
    ).arg(Typography::getBody())
     .arg(scalePixelValue(Spacing::getMedium()))
     .arg(scalePixelValue(2))
     .arg(Colors::BORDER_MEDIUM.name())
     .arg(scalePixelValue(Spacing::getSmallRadius()))
     .arg(Colors::BACKGROUND_LIGHT.name())
     .arg(Colors::TEXT_PRIMARY.name())
     .arg(Typography::PRIMARY_FONT)
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(Colors::BACKGROUND_DARK.name())
     .arg(Colors::TEXT_DISABLED.name())
     .arg(Colors::BORDER_LIGHT.name());
}

QString ModernMedicalStyle::getListWidgetStyle()
{
    return QString(
        "QListWidget {"
        "    font-size: %1pt;"
        "    border: %2px solid %3;"
        "    border-radius: %4px;"
        "    background-color: %5;"
        "    color: %6;"
        "    font-family: %7;"
        "    alternate-background-color: %8;"
        "}"
        "QListWidget::item {"
        "    padding: %9px;"
        "    border-bottom: 1px solid %10;"
        "    min-height: %11px;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: %12;"
        "    color: %13;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: %14;"
        "}"
    ).arg(Typography::getBody())
     .arg(scalePixelValue(2))
     .arg(Colors::BORDER_MEDIUM.name())
     .arg(scalePixelValue(Spacing::getSmallRadius()))
     .arg(Colors::BACKGROUND_LIGHT.name())
     .arg(Colors::TEXT_PRIMARY.name())
     .arg(Typography::PRIMARY_FONT)
     .arg(Colors::BACKGROUND_MEDIUM.name())
     .arg(scalePixelValue(Spacing::getMedium()))
     .arg(Colors::BORDER_LIGHT.name())
     .arg(scalePixelValue(Spacing::getMinTouchTarget()))
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(Colors::TEXT_ON_PRIMARY.name())
     .arg(Colors::PRIMARY_BLUE_LIGHT.name());
}

QString ModernMedicalStyle::getTabWidgetStyle()
{
    return QString(
        "QTabWidget::pane {"
        "    border: %1px solid %2;"
        "    border-radius: %3px;"
        "    background-color: %4;"
        "    top: -1px;"
        "}"
        "QTabBar::tab {"
        "    font-size: %5pt;"
        "    font-family: %6;"
        "    padding: %7px %8px;"
        "    margin-right: %9px;"
        "    background-color: %10;"
        "    color: %11;"
        "    border: %12px solid %13;"
        "    border-bottom: none;"
        "    border-top-left-radius: %14px;"
        "    border-top-right-radius: %15px;"
        "    min-width: %16px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: %17;"
        "    color: %18;"
        "    border-color: %19;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: %20;"
        "}"
    ).arg(scalePixelValue(2))
     .arg(Colors::BORDER_MEDIUM.name())
     .arg(scalePixelValue(Spacing::getSmallRadius()))
     .arg(Colors::BACKGROUND_LIGHT.name())
     .arg(Typography::getButton())
     .arg(Typography::PRIMARY_FONT)
     .arg(scalePixelValue(Spacing::getMedium()))
     .arg(scalePixelValue(Spacing::getLarge()))
     .arg(scalePixelValue(2))
     .arg(Colors::BACKGROUND_MEDIUM.name())
     .arg(Colors::TEXT_PRIMARY.name())
     .arg(scalePixelValue(1))
     .arg(Colors::BORDER_MEDIUM.name())
     .arg(scalePixelValue(Spacing::getSmallRadius()))
     .arg(scalePixelValue(Spacing::getSmallRadius()))
     .arg(scalePixelValue(80))
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(Colors::TEXT_ON_PRIMARY.name())
     .arg(Colors::PRIMARY_BLUE.name())
     .arg(Colors::PRIMARY_BLUE_LIGHT.name());
}
