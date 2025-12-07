#include "UserInterfaceTests.h"
#include "../src/gui/MainWindow.h"
#include "../src/gui/PressureMonitor.h"
#include "../src/gui/PatternSelector.h"
#include "../src/gui/SafetyPanel.h"
#include "../src/gui/ParameterAdjustmentPanel.h"
#include "../src/gui/SystemDiagnosticsPanel.h"
#include "../src/VacuumController.h"
#include <QApplication>
#include <QScreen>
#include <QWidget>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QStyleOption>
#include <QPainter>
#include <QFontMetrics>

void UserInterfaceTests::initTestCase()
{
    qDebug() << "Initializing UI test environment for 50-inch display";
    
    // Create controller with simulation mode
    m_controller = std::make_unique<VacuumController>();
    m_controller->setSimulationMode(true);
    QVERIFY(m_controller->initialize());
    
    // Create main window
    m_mainWindow = std::make_unique<MainWindow>();
    m_mainWindow->setController(m_controller.get());
    
    // Get UI component references
    m_pressureMonitor = m_mainWindow->getPressureMonitor();
    m_patternSelector = m_mainWindow->getPatternSelector();
    m_safetyPanel = m_mainWindow->getSafetyPanel();
    m_parameterPanel = m_mainWindow->getParameterAdjustmentPanel();
    m_diagnosticsPanel = m_mainWindow->getSystemDiagnosticsPanel();
    
    // Measure display metrics
    m_displayMetrics = measureCurrentDisplay();
    
    // Configure for fullscreen testing
    m_fullscreenTesting = true;
    m_touchSimulationEnabled = true;
    m_testDurationMs = 10000;
    
    // Show window in fullscreen for testing
    if (m_fullscreenTesting) {
        m_mainWindow->showFullScreen();
    } else {
        m_mainWindow->resize(TARGET_SCREEN_WIDTH, TARGET_SCREEN_HEIGHT);
        m_mainWindow->show();
    }
    
    QTest::qWait(1000);  // Allow window to stabilize
    
    qDebug() << "UI test environment initialized";
    qDebug() << "Display size:" << m_displayMetrics.screenSize;
    qDebug() << "DPI:" << m_displayMetrics.dpi;
}

void UserInterfaceTests::cleanupTestCase()
{
    qDebug() << "Cleaning up UI test environment";
    
    if (m_mainWindow) {
        m_mainWindow->close();
        m_mainWindow.reset();
    }
    
    if (m_controller) {
        m_controller->shutdown();
        m_controller.reset();
    }
    
    qDebug() << "UI test environment cleaned up";
}

void UserInterfaceTests::init()
{
    // Reset UI state before each test
    if (m_mainWindow) {
        m_mainWindow->activateWindow();
        m_mainWindow->raise();
        QTest::qWait(100);
    }
}

void UserInterfaceTests::cleanup()
{
    // Cleanup after each test
    QTest::qWait(100);
}

void UserInterfaceTests::testDisplayScaling()
{
    qDebug() << "Testing display scaling for 50-inch screen";
    
    // Verify display metrics meet requirements
    QVERIFY(validateDisplayForMedicalUse(m_displayMetrics));
    
    // Test main window scaling
    QSize windowSize = m_mainWindow->size();
    qDebug() << "Main window size:" << windowSize;
    
    if (m_fullscreenTesting) {
        QVERIFY(windowSize.width() >= TARGET_SCREEN_WIDTH);
        QVERIFY(windowSize.height() >= TARGET_SCREEN_HEIGHT);
    }
    
    // Test component scaling
    QVERIFY(m_pressureMonitor->size().width() > 400);
    QVERIFY(m_patternSelector->size().width() > 300);
    QVERIFY(m_safetyPanel->size().width() > 300);
    
    qDebug() << "Display scaling test completed successfully";
}

void UserInterfaceTests::testFontSizeReadability()
{
    qDebug() << "Testing font size readability on large display";
    
    // Test main window fonts
    checkFontReadability(m_mainWindow.get());
    
    // Test component fonts
    checkFontReadability(m_pressureMonitor);
    checkFontReadability(m_patternSelector);
    checkFontReadability(m_safetyPanel);
    
    // Test specific critical text elements
    auto emergencyButton = m_safetyPanel->findChild<QPushButton*>("emergencyStopButton");
    if (emergencyButton) {
        QFont font = emergencyButton->font();
        QVERIFY(font.pointSize() >= MIN_FONT_SIZE_PT);
        qDebug() << "Emergency button font size:" << font.pointSize() << "pt";
    }
    
    qDebug() << "Font readability test completed successfully";
}

void UserInterfaceTests::testColorContrast()
{
    qDebug() << "Testing color contrast for medical device standards";
    
    // Test main window contrast
    validateColorContrast(m_mainWindow.get());
    
    // Test safety panel contrast (critical for medical devices)
    validateColorContrast(m_safetyPanel);
    
    // Test emergency stop button contrast
    auto emergencyButton = m_safetyPanel->findChild<QPushButton*>("emergencyStopButton");
    if (emergencyButton) {
        validateColorContrast(emergencyButton);
    }
    
    qDebug() << "Color contrast test completed successfully";
}

void UserInterfaceTests::testTouchAccuracy()
{
    qDebug() << "Testing touch accuracy on 50-inch display";
    
    if (!m_touchSimulationEnabled) {
        QSKIP("Touch simulation not enabled");
    }
    
    // Test touch accuracy on various UI elements
    QList<TouchTestResult> results;
    
    // Test pressure monitor touch targets
    if (m_pressureMonitor) {
        QRect monitorRect = m_pressureMonitor->geometry();
        QPoint center = monitorRect.center();
        
        TouchTestResult result = performTouchTest(m_pressureMonitor, center);
        results.append(result);
        QVERIFY(result.successful);
    }
    
    // Test pattern selector buttons
    if (m_patternSelector) {
        auto buttons = m_patternSelector->findChildren<QPushButton*>();
        for (auto button : buttons) {
            if (button->isVisible() && button->isEnabled()) {
                QPoint buttonCenter = button->geometry().center();
                TouchTestResult result = performTouchTest(button, buttonCenter);
                results.append(result);
                QVERIFY(result.successful);
                
                // Verify button size meets touch requirements
                QSize buttonSize = button->size();
                QVERIFY(buttonSize.width() >= MIN_TOUCH_TARGET_SIZE);
                QVERIFY(buttonSize.height() >= MIN_TOUCH_TARGET_SIZE);
            }
        }
    }
    
    // Calculate overall touch accuracy
    int successfulTouches = 0;
    qint64 totalResponseTime = 0;
    
    for (const auto& result : results) {
        if (result.successful) {
            successfulTouches++;
            totalResponseTime += result.responseTime;
        }
    }
    
    double accuracy = (double)successfulTouches / results.size();
    double averageResponseTime = (double)totalResponseTime / successfulTouches;
    
    qDebug() << "Touch accuracy:" << (accuracy * 100) << "%";
    qDebug() << "Average response time:" << averageResponseTime << "ms";
    
    QVERIFY(accuracy >= 0.95);  // 95% accuracy requirement
    QVERIFY(averageResponseTime <= MAX_RESPONSE_TIME_MS);
    
    qDebug() << "Touch accuracy test completed successfully";
}

void UserInterfaceTests::testTouchResponsiveness()
{
    qDebug() << "Testing touch responsiveness";
    
    if (!m_touchSimulationEnabled) {
        QSKIP("Touch simulation not enabled");
    }
    
    // Test emergency stop button responsiveness (critical)
    auto emergencyButton = m_safetyPanel->findChild<QPushButton*>("emergencyStopButton");
    if (emergencyButton) {
        QElapsedTimer timer;
        timer.start();
        
        simulateTouch(emergencyButton, emergencyButton->rect().center());
        
        qint64 responseTime = timer.elapsed();
        qDebug() << "Emergency stop response time:" << responseTime << "ms";
        
        QVERIFY(responseTime <= 50);  // Emergency stop must respond within 50ms
    }
    
    // Test pattern selector responsiveness
    if (m_patternSelector) {
        auto buttons = m_patternSelector->findChildren<QPushButton*>();
        for (auto button : buttons.mid(0, 5)) {  // Test first 5 buttons
            if (button->isVisible() && button->isEnabled()) {
                QVERIFY(verifyResponsiveness(button, MAX_RESPONSE_TIME_MS));
            }
        }
    }
    
    qDebug() << "Touch responsiveness test completed successfully";
}

void UserInterfaceTests::testMainWindowLayout()
{
    qDebug() << "Testing main window layout";
    
    // Verify main window is properly sized
    QSize windowSize = m_mainWindow->size();
    QVERIFY(windowSize.width() >= 800);
    QVERIFY(windowSize.height() >= 600);
    
    // Verify all major components are visible
    QVERIFY(m_pressureMonitor->isVisible());
    QVERIFY(m_patternSelector->isVisible());
    QVERIFY(m_safetyPanel->isVisible());
    
    // Verify components don't overlap inappropriately
    QRect pressureRect = m_pressureMonitor->geometry();
    QRect patternRect = m_patternSelector->geometry();
    QRect safetyRect = m_safetyPanel->geometry();
    
    // Components should not completely overlap
    QVERIFY(!pressureRect.contains(patternRect));
    QVERIFY(!pressureRect.contains(safetyRect));
    QVERIFY(!patternRect.contains(safetyRect));
    
    qDebug() << "Main window layout test completed successfully";
}

void UserInterfaceTests::testEmergencyStopVisibility()
{
    qDebug() << "Testing emergency stop visibility (critical safety test)";
    
    // Find emergency stop button
    auto emergencyButton = m_safetyPanel->findChild<QPushButton*>("emergencyStopButton");
    QVERIFY(emergencyButton != nullptr);
    
    // Verify button is visible and accessible
    QVERIFY(emergencyButton->isVisible());
    QVERIFY(emergencyButton->isEnabled());
    
    // Verify button size meets safety requirements
    QSize buttonSize = emergencyButton->size();
    QVERIFY(buttonSize.width() >= EMERGENCY_BUTTON_MIN_SIZE);
    QVERIFY(buttonSize.height() >= EMERGENCY_BUTTON_MIN_SIZE);
    
    // Verify button is prominently positioned
    QRect buttonRect = emergencyButton->geometry();
    QRect windowRect = m_mainWindow->geometry();
    
    // Emergency button should be in upper portion of screen for visibility
    QVERIFY(buttonRect.top() < windowRect.height() / 2);
    
    // Verify button color contrast for emergency visibility
    validateColorContrast(emergencyButton);
    
    qDebug() << "Emergency stop visibility test completed successfully";
}

void UserInterfaceTests::testGUIResponsiveness()
{
    qDebug() << "Testing GUI responsiveness under load";
    
    m_performanceMetrics = measureUIPerformance(m_testDurationMs);
    
    qDebug() << "Performance metrics:";
    qDebug() << "  Average frame rate:" << m_performanceMetrics.averageFrameRate << "FPS";
    qDebug() << "  Average response time:" << m_performanceMetrics.averageResponseTime << "ms";
    qDebug() << "  Max response time:" << m_performanceMetrics.maxResponseTime << "ms";
    qDebug() << "  Memory usage:" << m_performanceMetrics.memoryUsage / 1024 / 1024 << "MB";
    
    QVERIFY(validatePerformanceRequirements(m_performanceMetrics));
    
    qDebug() << "GUI responsiveness test completed successfully";
}

// Helper method implementations
UserInterfaceTests::DisplayMetrics UserInterfaceTests::measureCurrentDisplay()
{
    DisplayMetrics metrics;
    
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        metrics.screenSize = screen->size();
        metrics.dpi = screen->logicalDotsPerInch();
        metrics.pixelRatio = screen->devicePixelRatio();
    }
    
    return metrics;
}

bool UserInterfaceTests::validateDisplayForMedicalUse(const DisplayMetrics& metrics)
{
    // Verify minimum resolution for medical device use
    if (metrics.screenSize.width() < 1024 || metrics.screenSize.height() < 768) {
        qWarning() << "Display resolution too low for medical device use";
        return false;
    }
    
    // Verify DPI is reasonable for large display
    if (metrics.dpi < 72 || metrics.dpi > 300) {
        qWarning() << "Display DPI outside acceptable range";
        return false;
    }
    
    return true;
}

void UserInterfaceTests::simulateTouch(QWidget* widget, const QPoint& position)
{
    if (!widget || !m_touchSimulationEnabled) return;
    
    // Simulate mouse press/release for touch
    QMouseEvent pressEvent(QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &pressEvent);
    
    QTest::qWait(10);  // Brief delay
    
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, position, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &releaseEvent);
}

UserInterfaceTests::TouchTestResult UserInterfaceTests::performTouchTest(QWidget* widget, const QPoint& position)
{
    TouchTestResult result;
    result.targetPosition = position;
    
    QElapsedTimer timer;
    timer.start();
    
    simulateTouch(widget, position);
    
    result.responseTime = timer.elapsed();
    result.actualPosition = position;  // In simulation, actual = target
    
    // Calculate accuracy
    int distance = (result.actualPosition - result.targetPosition).manhattanLength();
    result.successful = (distance <= TOUCH_ACCURACY_TOLERANCE) && (result.responseTime <= MAX_RESPONSE_TIME_MS);
    
    if (!result.successful) {
        result.errorMessage = QString("Touch test failed: distance=%1px, time=%2ms").arg(distance).arg(result.responseTime);
    }
    
    return result;
}

bool UserInterfaceTests::verifyResponsiveness(QWidget* widget, int maxResponseTimeMs)
{
    if (!widget) return false;
    
    QElapsedTimer timer;
    timer.start();
    
    simulateTouch(widget, widget->rect().center());
    
    qint64 responseTime = timer.elapsed();
    return responseTime <= maxResponseTimeMs;
}

void UserInterfaceTests::checkFontReadability(QWidget* widget)
{
    if (!widget) return;
    
    QFont font = widget->font();
    int fontSize = font.pointSize();
    
    if (fontSize < MIN_FONT_SIZE_PT) {
        qWarning() << "Font size too small in widget:" << widget->objectName() << "Size:" << fontSize;
    }
    
    QVERIFY(fontSize >= MIN_FONT_SIZE_PT);
}

void UserInterfaceTests::validateColorContrast(QWidget* widget)
{
    if (!widget) return;
    
    // This is a simplified contrast check
    // In a real implementation, you would analyze actual pixel colors
    QPalette palette = widget->palette();
    QColor background = palette.color(QPalette::Background);
    QColor foreground = palette.color(QPalette::Foreground);
    
    // Calculate luminance ratio (simplified)
    double bgLuminance = (background.red() * 0.299 + background.green() * 0.587 + background.blue() * 0.114) / 255.0;
    double fgLuminance = (foreground.red() * 0.299 + foreground.green() * 0.587 + foreground.blue() * 0.114) / 255.0;
    
    double contrastRatio = (qMax(bgLuminance, fgLuminance) + 0.05) / (qMin(bgLuminance, fgLuminance) + 0.05);
    
    qDebug() << "Contrast ratio for" << widget->objectName() << ":" << contrastRatio;
    
    // For medical devices, we require higher contrast
    QVERIFY(contrastRatio >= MIN_CONTRAST_RATIO);
}

UserInterfaceTests::PerformanceMetrics UserInterfaceTests::measureUIPerformance(int durationMs)
{
    PerformanceMetrics metrics;
    
    QElapsedTimer timer;
    timer.start();
    
    int frameCount = 0;
    qint64 totalResponseTime = 0;
    qint64 maxResponseTime = 0;
    
    while (timer.elapsed() < durationMs) {
        QElapsedTimer frameTimer;
        frameTimer.start();
        
        // Process events to simulate UI activity
        QApplication::processEvents();
        
        qint64 frameTime = frameTimer.elapsed();
        totalResponseTime += frameTime;
        maxResponseTime = qMax(maxResponseTime, frameTime);
        frameCount++;
        
        QTest::qWait(16);  // Target ~60 FPS
    }
    
    qint64 totalTime = timer.elapsed();
    
    metrics.averageFrameRate = (frameCount * 1000.0) / totalTime;
    metrics.averageResponseTime = totalResponseTime / frameCount;
    metrics.maxResponseTime = maxResponseTime;
    
    return metrics;
}

bool UserInterfaceTests::validatePerformanceRequirements(const PerformanceMetrics& metrics)
{
    bool valid = true;
    
    if (metrics.averageFrameRate < MIN_FRAME_RATE) {
        qWarning() << "Frame rate below minimum:" << metrics.averageFrameRate << "< " << MIN_FRAME_RATE;
        valid = false;
    }
    
    if (metrics.averageResponseTime > MAX_RESPONSE_TIME_MS) {
        qWarning() << "Average response time too high:" << metrics.averageResponseTime << ">" << MAX_RESPONSE_TIME_MS;
        valid = false;
    }
    
    if (metrics.maxResponseTime > MAX_RESPONSE_TIME_MS * 2) {
        qWarning() << "Max response time too high:" << metrics.maxResponseTime << ">" << (MAX_RESPONSE_TIME_MS * 2);
        valid = false;
    }
    
    return valid;
}

QTEST_MAIN(UserInterfaceTests)
