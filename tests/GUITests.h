#ifndef GUITESTS_H
#define GUITESTS_H

#include <QObject>
#include <QTest>

/**
 * @brief GUI testing class for vacuum controller interface
 * 
 * Tests all user interface functionality including:
 * - Main window creation and layout
 * - Button interactions and responses
 * - Pressure display and updates
 * - Pattern selection interface
 * - Progress indicators and status displays
 * - Touch interaction for 50-inch display
 * - Keyboard shortcuts and accessibility
 * - Window resizing and fullscreen mode
 * - Data visualization and charts
 */
class GUITests : public QObject
{
    Q_OBJECT

public:
    explicit GUITests(QObject *parent = nullptr);

private slots:
    // Test framework setup/teardown
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Window and layout tests
    void testMainWindowCreation();
    void testWindowResizing();
    
    // Interaction tests
    void testButtonInteractions();
    void testTouchInteraction();
    void testKeyboardShortcuts();
    
    // Display tests
    void testPressureDisplay();
    void testProgressIndicators();
    void testStatusIndicators();
    void testDataVisualization();
    
    // Interface tests
    void testPatternSelection();

private:
    // Helper methods
    void simulateUserInteraction();
    bool verifyDisplayUpdate(const QString& elementName, const QString& expectedValue);
    void testResponsiveness();
    
    // Test constants
    static constexpr int DISPLAY_UPDATE_TIMEOUT = 1000; // ms
    static constexpr int TOUCH_PRESS_DURATION = 100;    // ms
    static constexpr int MIN_BUTTON_SIZE = 80;          // pixels for touch
};

#endif // GUITESTS_H
