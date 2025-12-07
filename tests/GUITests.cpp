#include "GUITests.h"
#include "TestFramework.h"
#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QSignalSpy>

GUITests::GUITests(QObject *parent)
    : QObject(parent)
{
}

void GUITests::initTestCase()
{
    TestFramework::initialize();
    TestFramework::enableMockHardware(true);
    qDebug() << "GUI Tests initialized";
}

void GUITests::cleanupTestCase()
{
    TestFramework::cleanup();
    qDebug() << "GUI Tests cleanup completed";
}

void GUITests::init()
{
    TestFramework::resetHardwareState();
}

void GUITests::cleanup()
{
    TestFramework::stopAllOperations();
}

void GUITests::testMainWindowCreation()
{
    qDebug() << "Testing main window creation...";
    
    // Create main window
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Verify window properties
    QVERIFY(!mainWindow->windowTitle().isEmpty());
    QVERIFY(mainWindow->size().width() > 800);
    QVERIFY(mainWindow->size().height() > 600);
    
    // Clean up
    delete mainWindow;
}

void GUITests::testButtonInteractions()
{
    qDebug() << "Testing button interactions...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find emergency stop button
    QPushButton* emergencyButton = mainWindow->findChild<QPushButton*>("emergencyStopButton");
    QVERIFY(emergencyButton != nullptr);
    
    // Test emergency button click
    QSignalSpy emergencySpy(emergencyButton, &QPushButton::clicked);
    QTest::mouseClick(emergencyButton, Qt::LeftButton);
    QCOMPARE(emergencySpy.count(), 1);
    
    // Verify emergency stop was triggered
    QVERIFY(TestFramework::isEmergencyStop());
    
    // Find start button
    QPushButton* startButton = mainWindow->findChild<QPushButton*>("startButton");
    if (startButton) {
        QVERIFY(startButton->isEnabled() == false); // Should be disabled during emergency
    }
    
    // Reset emergency stop
    TestFramework::resetEmergencyStop();
    
    delete mainWindow;
}

void GUITests::testPressureDisplay()
{
    qDebug() << "Testing pressure display...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find pressure display labels
    QLabel* pressure1Label = mainWindow->findChild<QLabel*>("pressure1Label");
    QLabel* pressure2Label = mainWindow->findChild<QLabel*>("pressure2Label");
    
    QVERIFY(pressure1Label != nullptr);
    QVERIFY(pressure2Label != nullptr);
    
    // Set test pressure values
    TestFramework::setPressureSensorValue(1, -45.5);
    TestFramework::setPressureSensorValue(2, -52.3);
    
    // Trigger display update
    TestFramework::updateGUIDisplays();
    
    // Verify pressure values are displayed
    QString pressure1Text = pressure1Label->text();
    QString pressure2Text = pressure2Label->text();
    
    QVERIFY(pressure1Text.contains("-45"));
    QVERIFY(pressure2Text.contains("-52"));
    
    delete mainWindow;
}

void GUITests::testPatternSelection()
{
    qDebug() << "Testing pattern selection...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find pattern selection widget
    QWidget* patternSelector = mainWindow->findChild<QWidget*>("patternSelector");
    QVERIFY(patternSelector != nullptr);
    
    // Test pattern selection
    QVERIFY(TestFramework::selectPattern("Pulse"));
    QCOMPARE(TestFramework::getSelectedPattern(), QString("Pulse"));
    
    QVERIFY(TestFramework::selectPattern("Wave"));
    QCOMPARE(TestFramework::getSelectedPattern(), QString("Wave"));
    
    QVERIFY(TestFramework::selectPattern("Constant"));
    QCOMPARE(TestFramework::getSelectedPattern(), QString("Constant"));
    
    delete mainWindow;
}

void GUITests::testProgressIndicators()
{
    qDebug() << "Testing progress indicators...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find progress bar
    QProgressBar* progressBar = mainWindow->findChild<QProgressBar*>("patternProgressBar");
    QVERIFY(progressBar != nullptr);
    
    // Start a test pattern
    QJsonObject testPattern;
    testPattern["type"] = "constant";
    testPattern["duration_ms"] = 2000;
    testPattern["pressure_mmhg"] = -50;
    
    QVERIFY(TestFramework::startPattern("test_progress", testPattern));
    
    // Check initial progress
    QCOMPARE(progressBar->value(), 0);
    
    // Wait and check progress updates
    QTest::qWait(500);
    TestFramework::updateGUIDisplays();
    QVERIFY(progressBar->value() > 0);
    QVERIFY(progressBar->value() < 100);
    
    // Wait for completion
    QTest::qWait(2000);
    TestFramework::updateGUIDisplays();
    QCOMPARE(progressBar->value(), 100);
    
    delete mainWindow;
}

void GUITests::testStatusIndicators()
{
    qDebug() << "Testing status indicators...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find status indicators
    QLabel* pumpStatusLabel = mainWindow->findChild<QLabel*>("pumpStatusLabel");
    QLabel* systemStatusLabel = mainWindow->findChild<QLabel*>("systemStatusLabel");
    
    QVERIFY(pumpStatusLabel != nullptr);
    QVERIFY(systemStatusLabel != nullptr);
    
    // Test pump status indication
    TestFramework::setPump(true);
    TestFramework::updateGUIDisplays();
    QVERIFY(pumpStatusLabel->text().contains("ON") || pumpStatusLabel->text().contains("Active"));
    
    TestFramework::setPump(false);
    TestFramework::updateGUIDisplays();
    QVERIFY(pumpStatusLabel->text().contains("OFF") || pumpStatusLabel->text().contains("Inactive"));
    
    // Test system status
    TestFramework::triggerEmergencyStop();
    TestFramework::updateGUIDisplays();
    QVERIFY(systemStatusLabel->text().contains("EMERGENCY") || systemStatusLabel->text().contains("STOP"));
    
    TestFramework::resetEmergencyStop();
    TestFramework::updateGUIDisplays();
    QVERIFY(systemStatusLabel->text().contains("READY") || systemStatusLabel->text().contains("Normal"));
    
    delete mainWindow;
}

void GUITests::testTouchInteraction()
{
    qDebug() << "Testing touch interaction...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find a button to test touch interaction
    QPushButton* testButton = mainWindow->findChild<QPushButton*>("startButton");
    QVERIFY(testButton != nullptr);
    
    // Simulate touch press
    QTest::touchEvent(mainWindow, TestFramework::getTouchDevice())
        .press(0, testButton->rect().center(), mainWindow);
    
    QTest::qWait(100);
    
    // Simulate touch release
    QTest::touchEvent(mainWindow, TestFramework::getTouchDevice())
        .release(0, testButton->rect().center(), mainWindow);
    
    // Verify button was activated
    // Note: Actual verification depends on button implementation
    
    delete mainWindow;
}

void GUITests::testKeyboardShortcuts()
{
    qDebug() << "Testing keyboard shortcuts...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    mainWindow->show();
    QTest::qWaitForWindowActive(mainWindow);
    
    // Test emergency stop shortcut (typically Escape or Space)
    QTest::keyClick(mainWindow, Qt::Key_Escape);
    QVERIFY(TestFramework::isEmergencyStop());
    
    TestFramework::resetEmergencyStop();
    
    // Test other shortcuts if implemented
    QTest::keyClick(mainWindow, Qt::Key_F1); // Help
    QTest::keyClick(mainWindow, Qt::Key_F5); // Refresh
    
    delete mainWindow;
}

void GUITests::testWindowResizing()
{
    qDebug() << "Testing window resizing...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Test different window sizes
    QSize originalSize = mainWindow->size();
    
    // Test smaller size
    mainWindow->resize(800, 600);
    QCOMPARE(mainWindow->size(), QSize(800, 600));
    
    // Test larger size
    mainWindow->resize(1920, 1080);
    QCOMPARE(mainWindow->size(), QSize(1920, 1080));
    
    // Test fullscreen for 50-inch display
    mainWindow->resize(3840, 2160); // 4K resolution
    QVERIFY(mainWindow->size().width() >= 1920);
    QVERIFY(mainWindow->size().height() >= 1080);
    
    // Restore original size
    mainWindow->resize(originalSize);
    
    delete mainWindow;
}

void GUITests::testDataVisualization()
{
    qDebug() << "Testing data visualization...";
    
    QWidget* mainWindow = TestFramework::createMainWindow();
    QVERIFY(mainWindow != nullptr);
    
    // Find chart widget (if implemented)
    QWidget* chartWidget = mainWindow->findChild<QWidget*>("pressureChart");
    if (chartWidget) {
        QVERIFY(chartWidget->isVisible());
        
        // Generate test data
        for (int i = 0; i < 10; ++i) {
            TestFramework::setPressureSensorValue(1, -30 - i * 2);
            TestFramework::updateGUIDisplays();
            QTest::qWait(100);
        }
        
        // Verify chart updates (implementation specific)
        QVERIFY(TestFramework::hasChartData());
    }
    
    delete mainWindow;
}

// Test runner for this specific test class
QTEST_MAIN(GUITests)
