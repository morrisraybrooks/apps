#ifndef USERINTERFACETESTS_H
#define USERINTERFACETESTS_H

#include <QObject>
#include <QTest>
#include <QWidget>
#include <QPoint>
#include <QSize>
#include <QElapsedTimer>
#include <memory>

// Forward declarations
class MainWindow;
class VacuumController;
class PressureMonitor;
class PatternSelector;
class SafetyPanel;
class ParameterAdjustmentPanel;
class SystemDiagnosticsPanel;

/**
 * @brief Comprehensive user interface testing for 50-inch touch displays
 * 
 * These tests validate the user interface for medical device operation:
 * - Touch interface responsiveness and accuracy
 * - Display scaling and readability on 50-inch screens
 * - Accessibility and usability for medical professionals
 * - Visual feedback and status indicators
 * - Error handling and user guidance
 * - Performance under continuous operation
 * - Safety-critical UI elements validation
 */
class UserInterfaceTests : public QObject
{
    Q_OBJECT

private slots:
    // Test setup and cleanup
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Display and scaling tests
    void testDisplayScaling();
    void testFontSizeReadability();
    void testColorContrast();
    void testFullscreenOperation();
    void testResolutionAdaptation();
    
    // Touch interface tests
    void testTouchAccuracy();
    void testTouchResponsiveness();
    void testMultiTouchGestures();
    void testTouchButtonSizes();
    void testTouchFeedback();
    
    // Navigation and layout tests
    void testMainWindowLayout();
    void testTabNavigation();
    void testScrollableAreas();
    void testModalDialogs();
    void testContextMenus();
    
    // Component-specific UI tests
    void testPressureMonitorDisplay();
    void testPatternSelectorInterface();
    void testSafetyPanelAlerts();
    void testParameterAdjustmentControls();
    void testSystemDiagnosticsDisplay();
    
    // Usability and accessibility tests
    void testKeyboardNavigation();
    void testScreenReaderCompatibility();
    void testHighContrastMode();
    void testLargeTextMode();
    void testColorBlindnessSupport();
    
    // Performance and responsiveness tests
    void testGUIResponsiveness();
    void testAnimationPerformance();
    void testMemoryUsageStability();
    void testLongTermOperation();
    void testRapidInteraction();
    
    // Safety-critical UI tests
    void testEmergencyStopVisibility();
    void testSafetyAlertDisplay();
    void testCriticalButtonAccess();
    void testErrorMessageClarity();
    void testStatusIndicatorAccuracy();
    
    // User experience tests
    void testWorkflowEfficiency();
    void testIntuitivenessOfControls();
    void testVisualHierarchy();
    void testConsistentDesign();
    void testUserGuidance();

private:
    // Test utilities
    void simulateTouch(QWidget* widget, const QPoint& position);
    void simulateTouchDrag(QWidget* widget, const QPoint& start, const QPoint& end);
    void simulateMultiTouch(QWidget* widget, const QList<QPoint>& positions);
    
    bool verifyTouchAccuracy(QWidget* widget, const QPoint& expectedPosition, const QPoint& actualPosition);
    bool verifyResponsiveness(QWidget* widget, int maxResponseTimeMs = 100);
    bool verifyVisualFeedback(QWidget* widget);
    
    void measureDisplayMetrics();
    void validateColorContrast(QWidget* widget);
    void checkFontReadability(QWidget* widget);
    void verifyButtonSizes(QWidget* widget);
    
    // Display testing
    struct DisplayMetrics {
        QSize screenSize;
        double dpi;
        double pixelRatio;
        int colorDepth;
        double brightness;
        
        DisplayMetrics() : dpi(0.0), pixelRatio(1.0), colorDepth(24), brightness(1.0) {}
    };
    
    DisplayMetrics measureCurrentDisplay();
    bool validateDisplayForMedicalUse(const DisplayMetrics& metrics);
    
    // Touch testing
    struct TouchTestResult {
        QPoint targetPosition;
        QPoint actualPosition;
        qint64 responseTime;
        bool successful;
        QString errorMessage;
        
        TouchTestResult() : responseTime(0), successful(false) {}
    };
    
    TouchTestResult performTouchTest(QWidget* widget, const QPoint& position);
    QList<TouchTestResult> performTouchAccuracyTest(QWidget* widget, int numTests = 20);
    
    // Performance testing
    struct PerformanceMetrics {
        double averageFrameRate;
        qint64 maxResponseTime;
        qint64 averageResponseTime;
        qint64 memoryUsage;
        double cpuUsage;
        
        PerformanceMetrics() : averageFrameRate(0.0), maxResponseTime(0), 
                              averageResponseTime(0), memoryUsage(0), cpuUsage(0.0) {}
    };
    
    PerformanceMetrics measureUIPerformance(int durationMs = 10000);
    bool validatePerformanceRequirements(const PerformanceMetrics& metrics);
    
    // Accessibility testing
    bool testKeyboardAccessibility(QWidget* widget);
    bool testScreenReaderSupport(QWidget* widget);
    bool testColorAccessibility(QWidget* widget);
    
    // Safety testing
    bool validateEmergencyStopAccess();
    bool validateSafetyAlertVisibility();
    bool validateCriticalControlAccess();
    
    // Test objects
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<VacuumController> m_controller;
    
    // UI component references
    PressureMonitor* m_pressureMonitor;
    PatternSelector* m_patternSelector;
    SafetyPanel* m_safetyPanel;
    ParameterAdjustmentPanel* m_parameterPanel;
    SystemDiagnosticsPanel* m_diagnosticsPanel;
    
    // Test state
    DisplayMetrics m_displayMetrics;
    PerformanceMetrics m_performanceMetrics;
    QElapsedTimer m_testTimer;
    
    // Test configuration
    bool m_fullscreenTesting;
    bool m_touchSimulationEnabled;
    int m_testDurationMs;
    
    // Constants for 50-inch display testing
    static const int TARGET_SCREEN_WIDTH = 1920;
    static const int TARGET_SCREEN_HEIGHT = 1080;
    static const int MIN_TOUCH_TARGET_SIZE = 44;  // 44px minimum touch target
    static const int PREFERRED_TOUCH_TARGET_SIZE = 60;  // 60px preferred
    static const double MIN_CONTRAST_RATIO = 4.5;  // WCAG AA standard
    static const double PREFERRED_CONTRAST_RATIO = 7.0;  // WCAG AAA standard
    static const int MAX_RESPONSE_TIME_MS = 100;  // Maximum UI response time
    static const double MIN_FRAME_RATE = 30.0;  // Minimum frame rate
    static const int TOUCH_ACCURACY_TOLERANCE = 10;  // 10px tolerance for touch accuracy
    
    // Medical device UI requirements
    static const int MIN_FONT_SIZE_PT = 12;  // Minimum readable font size
    static const int PREFERRED_FONT_SIZE_PT = 16;  // Preferred font size
    static const int EMERGENCY_BUTTON_MIN_SIZE = 100;  // Emergency button minimum size
    static const double SAFETY_ALERT_MIN_VISIBILITY = 0.9;  // 90% visibility requirement
};

#endif // USERINTERFACETESTS_H
