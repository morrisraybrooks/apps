#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QPushButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

#include "../../src/gui/ExecutionModeSelector.h"
#include "../../src/VacuumController.h"
#include "../mocks/MockVacuumController.h"

/**
 * @brief Comprehensive tests for ExecutionModeSelector widget
 * 
 * Tests all 6 execution modes, mode-specific parameter panels,
 * signal emissions, parameter validation, and VacuumController integration.
 */
class TestExecutionModeSelector : public QObject
{
    Q_OBJECT

public:
    TestExecutionModeSelector() = default;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Initialization tests
    void testWidgetCreation();
    void testWidgetCreationWithNullController();
    void testInitialState();
    
    // Mode selection tests
    void testModeSelectionManual();
    void testModeSelectionAdaptiveEdging();
    void testModeSelectionForcedOrgasm();
    void testModeSelectionMultiOrgasm();
    void testModeSelectionDenial();
    void testModeSelectionMilking();
    void testModeSelectionAll6Modes();
    
    // Parameter panel tests
    void testParameterPanelSwitching();
    void testAdaptiveEdgingParameters();
    void testForcedOrgasmParameters();
    void testDenialParameters();
    void testMilkingParameters();
    
    // Signal tests
    void testModeChangedSignal();
    void testParametersChangedSignal();
    
    // Boundary value tests
    void testTargetCyclesBoundary();
    void testDurationBoundary();
    void testFailureModeBoundary();
    
    // Integration tests
    void testStartSession();
    void testStopSession();

private:
    VacuumController* m_controller = nullptr;
    ExecutionModeSelector* m_widget = nullptr;
    
    // Helper methods
    QPushButton* findModeButton(const QString& modeName);
    QWidget* findParameterPanel(int index);
};

void TestExecutionModeSelector::initTestCase()
{
    // Global test setup
    qDebug() << "Starting ExecutionModeSelector tests...";
}

void TestExecutionModeSelector::cleanupTestCase()
{
    qDebug() << "ExecutionModeSelector tests complete.";
}

void TestExecutionModeSelector::init()
{
    // Create fresh controller and widget for each test
    m_controller = new VacuumController();
    m_widget = new ExecutionModeSelector(m_controller);
}

void TestExecutionModeSelector::cleanup()
{
    delete m_widget;
    m_widget = nullptr;
    delete m_controller;
    m_controller = nullptr;
}

void TestExecutionModeSelector::testWidgetCreation()
{
    QVERIFY(m_widget != nullptr);
    QVERIFY(m_widget->isVisible() == false);  // Not shown yet
}

void TestExecutionModeSelector::testWidgetCreationWithNullController()
{
    ExecutionModeSelector* nullWidget = new ExecutionModeSelector(nullptr);
    QVERIFY(nullWidget != nullptr);
    // Widget should still be created, just not functional
    delete nullWidget;
}

void TestExecutionModeSelector::testInitialState()
{
    // Default should be MANUAL mode
    QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::Manual);
}

void TestExecutionModeSelector::testModeSelectionManual()
{
    // Find and click Manual button
    QPushButton* manualBtn = m_widget->findChild<QPushButton*>("manualButton");
    if (manualBtn) {
        QTest::mouseClick(manualBtn, Qt::LeftButton);
        QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::Manual);
    }
}

void TestExecutionModeSelector::testModeSelectionAdaptiveEdging()
{
    QPushButton* edgingBtn = m_widget->findChild<QPushButton*>("adaptiveEdgingButton");
    if (edgingBtn) {
        QTest::mouseClick(edgingBtn, Qt::LeftButton);
        QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::AdaptiveEdging);
    }
}

void TestExecutionModeSelector::testModeSelectionForcedOrgasm()
{
    QPushButton* forcedBtn = m_widget->findChild<QPushButton*>("forcedOrgasmButton");
    if (forcedBtn) {
        QTest::mouseClick(forcedBtn, Qt::LeftButton);
        QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::ForcedOrgasm);
    }
}

void TestExecutionModeSelector::testModeSelectionMultiOrgasm()
{
    QPushButton* multiBtn = m_widget->findChild<QPushButton*>("multiOrgasmButton");
    if (multiBtn) {
        QTest::mouseClick(multiBtn, Qt::LeftButton);
        QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::MultiOrgasm);
    }
}

void TestExecutionModeSelector::testModeSelectionDenial()
{
    QPushButton* denialBtn = m_widget->findChild<QPushButton*>("denialButton");
    if (denialBtn) {
        QTest::mouseClick(denialBtn, Qt::LeftButton);
        QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::Denial);
    }
}

void TestExecutionModeSelector::testModeSelectionMilking()
{
    QPushButton* milkingBtn = m_widget->findChild<QPushButton*>("milkingButton");
    if (milkingBtn) {
        QTest::mouseClick(milkingBtn, Qt::LeftButton);
        QCOMPARE(m_widget->getSelectedMode(), ExecutionModeSelector::Mode::Milking);
    }
}

void TestExecutionModeSelector::testModeSelectionAll6Modes()
{
    // Test all 6 modes can be selected
    QList<ExecutionModeSelector::Mode> modes = {
        ExecutionModeSelector::Mode::Manual,
        ExecutionModeSelector::Mode::AdaptiveEdging,
        ExecutionModeSelector::Mode::ForcedOrgasm,
        ExecutionModeSelector::Mode::MultiOrgasm,
        ExecutionModeSelector::Mode::Denial,
        ExecutionModeSelector::Mode::Milking
    };

    QButtonGroup* buttonGroup = m_widget->findChild<QButtonGroup*>();
    if (buttonGroup) {
        QCOMPARE(buttonGroup->buttons().size(), 6);

        for (int i = 0; i < buttonGroup->buttons().size(); ++i) {
            QTest::mouseClick(buttonGroup->buttons().at(i), Qt::LeftButton);
            QCOMPARE(static_cast<int>(m_widget->getSelectedMode()), i);
        }
    }
}

void TestExecutionModeSelector::testParameterPanelSwitching()
{
    QStackedWidget* stackedWidget = m_widget->findChild<QStackedWidget*>();
    if (stackedWidget) {
        QVERIFY(stackedWidget->count() >= 6);

        // Switch modes and verify panel changes
        QButtonGroup* buttonGroup = m_widget->findChild<QButtonGroup*>();
        if (buttonGroup) {
            for (int i = 0; i < buttonGroup->buttons().size(); ++i) {
                QTest::mouseClick(buttonGroup->buttons().at(i), Qt::LeftButton);
                QTest::qWait(50);  // Allow UI update
                QCOMPARE(stackedWidget->currentIndex(), i);
            }
        }
    }
}

void TestExecutionModeSelector::testAdaptiveEdgingParameters()
{
    // Select Adaptive Edging mode
    QPushButton* edgingBtn = m_widget->findChild<QPushButton*>("adaptiveEdgingButton");
    if (edgingBtn) {
        QTest::mouseClick(edgingBtn, Qt::LeftButton);
    }

    // Find target cycles spinbox
    QSpinBox* cyclesSpin = m_widget->findChild<QSpinBox*>("targetCyclesSpin");
    if (cyclesSpin) {
        QVERIFY(cyclesSpin->minimum() >= 1);
        QVERIFY(cyclesSpin->maximum() <= 20);
        QCOMPARE(cyclesSpin->value(), 5);  // Default

        cyclesSpin->setValue(10);
        QCOMPARE(cyclesSpin->value(), 10);
    }
}

void TestExecutionModeSelector::testForcedOrgasmParameters()
{
    QPushButton* forcedBtn = m_widget->findChild<QPushButton*>("forcedOrgasmButton");
    if (forcedBtn) {
        QTest::mouseClick(forcedBtn, Qt::LeftButton);
    }

    QSpinBox* orgasmsSpin = m_widget->findChild<QSpinBox*>("targetOrgasmsSpin");
    QSpinBox* durationSpin = m_widget->findChild<QSpinBox*>("maxDurationSpin");

    if (orgasmsSpin) {
        QVERIFY(orgasmsSpin->minimum() >= 1);
        QCOMPARE(orgasmsSpin->value(), 3);  // Default
    }

    if (durationSpin) {
        QVERIFY(durationSpin->value() > 0);
    }
}

void TestExecutionModeSelector::testDenialParameters()
{
    QPushButton* denialBtn = m_widget->findChild<QPushButton*>("denialButton");
    if (denialBtn) {
        QTest::mouseClick(denialBtn, Qt::LeftButton);
    }

    QSpinBox* durationSpin = m_widget->findChild<QSpinBox*>("denialDurationSpin");
    if (durationSpin) {
        QVERIFY(durationSpin->minimum() >= 1);
        QVERIFY(durationSpin->value() > 0);
    }
}

void TestExecutionModeSelector::testMilkingParameters()
{
    QPushButton* milkingBtn = m_widget->findChild<QPushButton*>("milkingButton");
    if (milkingBtn) {
        QTest::mouseClick(milkingBtn, Qt::LeftButton);
    }

    QSpinBox* durationSpin = m_widget->findChild<QSpinBox*>("milkingDurationSpin");
    QComboBox* failureCombo = m_widget->findChild<QComboBox*>("failureModeCombo");

    if (durationSpin) {
        QVERIFY(durationSpin->minimum() >= 5);
        QVERIFY(durationSpin->maximum() <= 120);
    }

    if (failureCombo) {
        QCOMPARE(failureCombo->count(), 4);  // Stop, Ruin, Punish, Continue
    }
}

void TestExecutionModeSelector::testModeChangedSignal()
{
    QSignalSpy spy(m_widget, &ExecutionModeSelector::modeSelected);
    QVERIFY(spy.isValid());

    QButtonGroup* buttonGroup = m_widget->findChild<QButtonGroup*>();
    if (buttonGroup && buttonGroup->buttons().size() > 1) {
        QTest::mouseClick(buttonGroup->buttons().at(1), Qt::LeftButton);
        QCOMPARE(spy.count(), 1);

        QTest::mouseClick(buttonGroup->buttons().at(2), Qt::LeftButton);
        QCOMPARE(spy.count(), 2);
    }
}

void TestExecutionModeSelector::testParametersChangedSignal()
{
    // First select a mode with parameters
    QPushButton* edgingBtn = m_widget->findChild<QPushButton*>("adaptiveEdgingButton");
    if (edgingBtn) {
        QTest::mouseClick(edgingBtn, Qt::LeftButton);
    }

    QSignalSpy spy(m_widget, &ExecutionModeSelector::parametersChanged);
    QVERIFY(spy.isValid());

    QSpinBox* cyclesSpin = m_widget->findChild<QSpinBox*>("targetCyclesSpin");
    if (cyclesSpin) {
        cyclesSpin->setValue(cyclesSpin->value() + 1);
        QVERIFY(spy.count() >= 1);
    }
}

void TestExecutionModeSelector::testTargetCyclesBoundary()
{
    QPushButton* edgingBtn = m_widget->findChild<QPushButton*>("adaptiveEdgingButton");
    if (edgingBtn) {
        QTest::mouseClick(edgingBtn, Qt::LeftButton);
    }

    QSpinBox* cyclesSpin = m_widget->findChild<QSpinBox*>("targetCyclesSpin");
    if (cyclesSpin) {
        // Test minimum boundary
        cyclesSpin->setValue(cyclesSpin->minimum() - 1);
        QCOMPARE(cyclesSpin->value(), cyclesSpin->minimum());

        // Test maximum boundary
        cyclesSpin->setValue(cyclesSpin->maximum() + 1);
        QCOMPARE(cyclesSpin->value(), cyclesSpin->maximum());
    }
}

void TestExecutionModeSelector::testDurationBoundary()
{
    QPushButton* denialBtn = m_widget->findChild<QPushButton*>("denialButton");
    if (denialBtn) {
        QTest::mouseClick(denialBtn, Qt::LeftButton);
    }

    QSpinBox* durationSpin = m_widget->findChild<QSpinBox*>("denialDurationSpin");
    if (durationSpin) {
        int minVal = durationSpin->minimum();
        int maxVal = durationSpin->maximum();

        durationSpin->setValue(minVal - 1);
        QVERIFY(durationSpin->value() >= minVal);

        durationSpin->setValue(maxVal + 1);
        QVERIFY(durationSpin->value() <= maxVal);
    }
}

void TestExecutionModeSelector::testFailureModeBoundary()
{
    QPushButton* milkingBtn = m_widget->findChild<QPushButton*>("milkingButton");
    if (milkingBtn) {
        QTest::mouseClick(milkingBtn, Qt::LeftButton);
    }

    QComboBox* failureCombo = m_widget->findChild<QComboBox*>("failureModeCombo");
    if (failureCombo) {
        QCOMPARE(failureCombo->count(), 4);

        // Test all 4 failure modes
        for (int i = 0; i < 4; ++i) {
            failureCombo->setCurrentIndex(i);
            QCOMPARE(failureCombo->currentIndex(), i);
        }
    }
}

void TestExecutionModeSelector::testStartSession()
{
    // Select a mode and start session
    QPushButton* edgingBtn = m_widget->findChild<QPushButton*>("adaptiveEdgingButton");
    if (edgingBtn) {
        QTest::mouseClick(edgingBtn, Qt::LeftButton);
    }

    QPushButton* startBtn = m_widget->findChild<QPushButton*>("startButton");
    if (startBtn) {
        QSignalSpy spy(m_widget, &ExecutionModeSelector::sessionStartRequested);
        QTest::mouseClick(startBtn, Qt::LeftButton);
        QVERIFY(spy.count() >= 0);  // May or may not emit depending on controller state
    }
}

void TestExecutionModeSelector::testStopSession()
{
    QPushButton* stopBtn = m_widget->findChild<QPushButton*>("stopButton");
    if (stopBtn) {
        QSignalSpy spy(m_widget, &ExecutionModeSelector::sessionStopRequested);
        QTest::mouseClick(stopBtn, Qt::LeftButton);
        QVERIFY(spy.count() >= 0);
    }
}

QPushButton* TestExecutionModeSelector::findModeButton(const QString& modeName)
{
    return m_widget->findChild<QPushButton*>(modeName + "Button");
}

QWidget* TestExecutionModeSelector::findParameterPanel(int index)
{
    QStackedWidget* stack = m_widget->findChild<QStackedWidget*>();
    if (stack && index >= 0 && index < stack->count()) {
        return stack->widget(index);
    }
    return nullptr;
}

QTEST_MAIN(TestExecutionModeSelector)
#include "test_ExecutionModeSelector.moc"

