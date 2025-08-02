#include "PerformanceTests.h"
#include "TestFramework.h"
#include <QTest>
#include <QElapsedTimer>
#include <QThread>
#include <QCoreApplication>

PerformanceTests::PerformanceTests(QObject *parent)
    : QObject(parent)
{
}

void PerformanceTests::initTestCase()
{
    TestFramework::initialize();
    TestFramework::enableMockHardware(true);
    qDebug() << "Performance Tests initialized";
}

void PerformanceTests::cleanupTestCase()
{
    TestFramework::cleanup();
    qDebug() << "Performance Tests cleanup completed";
}

void PerformanceTests::init()
{
    TestFramework::resetHardwareState();
    TestFramework::clearPerformanceCounters();
}

void PerformanceTests::cleanup()
{
    TestFramework::stopAllOperations();
}

void PerformanceTests::testStartupTime()
{
    qDebug() << "Testing application startup time...";
    
    QElapsedTimer timer;
    timer.start();
    
    // Simulate application startup
    TestFramework::simulateApplicationStartup();
    
    qint64 startupTime = timer.elapsed();
    
    // Startup should complete within 3 seconds on Raspberry Pi 4
    QVERIFY2(startupTime < 3000, 
             QString("Startup time %1ms exceeds 3000ms limit").arg(startupTime).toLocal8Bit());
    
    qDebug() << "Startup time:" << startupTime << "ms";
}

void PerformanceTests::testMemoryUsage()
{
    qDebug() << "Testing memory usage...";
    
    // Get initial memory usage
    size_t initialMemory = TestFramework::getCurrentMemoryUsage();
    
    // Perform memory-intensive operations
    for (int i = 0; i < 100; ++i) {
        TestFramework::createLargeDataStructure();
        TestFramework::processLargeDataSet();
        TestFramework::cleanupLargeDataStructure();
    }
    
    // Force garbage collection
    TestFramework::forceGarbageCollection();
    
    size_t finalMemory = TestFramework::getCurrentMemoryUsage();
    size_t memoryIncrease = finalMemory - initialMemory;
    
    // Memory increase should be minimal (< 10MB)
    QVERIFY2(memoryIncrease < 10 * 1024 * 1024,
             QString("Memory increase %1 bytes exceeds 10MB limit").arg(memoryIncrease).toLocal8Bit());
    
    qDebug() << "Memory increase:" << memoryIncrease / 1024 << "KB";
}

void PerformanceTests::testCPUUsage()
{
    qDebug() << "Testing CPU usage...";
    
    // Start CPU monitoring
    TestFramework::startCPUMonitoring();
    
    // Perform CPU-intensive operations
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < 5000) { // Run for 5 seconds
        TestFramework::performCPUIntensiveTask();
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }
    
    double avgCPUUsage = TestFramework::getAverageCPUUsage();
    TestFramework::stopCPUMonitoring();
    
    // CPU usage should not exceed 80% on average
    QVERIFY2(avgCPUUsage < 80.0,
             QString("Average CPU usage %1% exceeds 80% limit").arg(avgCPUUsage).toLocal8Bit());
    
    qDebug() << "Average CPU usage:" << avgCPUUsage << "%";
}

void PerformanceTests::testResponseTimes()
{
    qDebug() << "Testing system response times...";
    
    QElapsedTimer timer;
    
    // Test GPIO response time
    timer.start();
    TestFramework::setSolenoid(1, true);
    qint64 gpioTime = timer.elapsed();
    
    QVERIFY2(gpioTime < 10, "GPIO response time should be < 10ms");
    
    // Test sensor reading time
    timer.restart();
    TestFramework::readPressureSensor(1);
    qint64 sensorTime = timer.elapsed();
    
    QVERIFY2(sensorTime < 5, "Sensor reading time should be < 5ms");
    
    // Test pattern start time
    QJsonObject testPattern;
    testPattern["type"] = "constant";
    testPattern["duration_ms"] = 1000;
    testPattern["pressure_mmhg"] = -50;
    
    timer.restart();
    TestFramework::startPattern("perf_test", testPattern);
    qint64 patternTime = timer.elapsed();
    
    QVERIFY2(patternTime < 50, "Pattern start time should be < 50ms");
    
    TestFramework::stopPattern("perf_test");
    
    qDebug() << "GPIO time:" << gpioTime << "ms";
    qDebug() << "Sensor time:" << sensorTime << "ms";
    qDebug() << "Pattern time:" << patternTime << "ms";
}

void PerformanceTests::testThroughput()
{
    qDebug() << "Testing data throughput...";
    
    QElapsedTimer timer;
    timer.start();
    
    int operationCount = 0;
    const int testDuration = 5000; // 5 seconds
    
    // Perform continuous operations
    while (timer.elapsed() < testDuration) {
        TestFramework::readPressureSensor(1);
        TestFramework::readPressureSensor(2);
        TestFramework::updateSystemState();
        operationCount++;
        
        // Small delay to prevent overwhelming the system
        QThread::usleep(100); // 0.1ms
    }
    
    double actualDuration = timer.elapsed() / 1000.0; // Convert to seconds
    double operationsPerSecond = operationCount / actualDuration;
    
    // Should achieve at least 1000 operations per second
    QVERIFY2(operationsPerSecond >= 1000.0,
             QString("Throughput %1 ops/sec is below 1000 ops/sec minimum")
             .arg(operationsPerSecond).toLocal8Bit());
    
    qDebug() << "Throughput:" << operationsPerSecond << "operations/second";
}

void PerformanceTests::testConcurrentOperations()
{
    qDebug() << "Testing concurrent operations performance...";
    
    QElapsedTimer timer;
    timer.start();
    
    // Start multiple concurrent operations
    TestFramework::startConcurrentSensorReading();
    TestFramework::startConcurrentPatternExecution();
    TestFramework::startConcurrentGUIUpdates();
    
    // Let them run for 3 seconds
    QTest::qWait(3000);
    
    // Stop concurrent operations
    TestFramework::stopConcurrentOperations();
    
    qint64 totalTime = timer.elapsed();
    
    // Verify no deadlocks or excessive delays
    QVERIFY2(totalTime < 3500, "Concurrent operations took too long");
    
    // Check for any errors during concurrent execution
    QVERIFY(!TestFramework::hasConcurrencyErrors());
    
    qDebug() << "Concurrent operations completed in:" << totalTime << "ms";
}

void PerformanceTests::testMemoryLeaks()
{
    qDebug() << "Testing for memory leaks...";
    
    size_t initialMemory = TestFramework::getCurrentMemoryUsage();
    
    // Perform operations that might cause memory leaks
    for (int cycle = 0; cycle < 10; ++cycle) {
        // Create and destroy patterns
        for (int i = 0; i < 20; ++i) {
            QJsonObject pattern;
            pattern["type"] = "pulse";
            pattern["duration_ms"] = 100;
            pattern["pressure_mmhg"] = -50;
            pattern["pulse_width_ms"] = 20;
            pattern["pulse_interval_ms"] = 40;
            
            QString patternName = QString("leak_test_%1").arg(i);
            TestFramework::startPattern(patternName, pattern);
            QTest::qWait(50);
            TestFramework::stopPattern(patternName);
        }
        
        // Create and destroy GUI elements
        for (int i = 0; i < 10; ++i) {
            QWidget* widget = TestFramework::createTestWidget();
            TestFramework::updateWidget(widget);
            delete widget;
        }
        
        // Force cleanup
        TestFramework::forceGarbageCollection();
        
        // Check memory usage periodically
        if (cycle % 3 == 0) {
            size_t currentMemory = TestFramework::getCurrentMemoryUsage();
            size_t memoryIncrease = currentMemory - initialMemory;
            
            qDebug() << "Cycle" << cycle << "- Memory increase:" << memoryIncrease / 1024 << "KB";
        }
    }
    
    // Final memory check
    TestFramework::forceGarbageCollection();
    QTest::qWait(1000); // Allow time for cleanup
    
    size_t finalMemory = TestFramework::getCurrentMemoryUsage();
    size_t totalIncrease = finalMemory - initialMemory;
    
    // Memory increase should be minimal (< 5MB)
    QVERIFY2(totalIncrease < 5 * 1024 * 1024,
             QString("Memory leak detected: %1 bytes increase").arg(totalIncrease).toLocal8Bit());
    
    qDebug() << "Total memory increase:" << totalIncrease / 1024 << "KB";
}

void PerformanceTests::testLongRunningStability()
{
    qDebug() << "Testing long-running stability...";
    
    // Start continuous operations
    TestFramework::startContinuousOperations();
    
    QElapsedTimer timer;
    timer.start();
    
    size_t initialMemory = TestFramework::getCurrentMemoryUsage();
    int errorCount = 0;
    
    // Run for 30 seconds (reduced from hours for testing)
    while (timer.elapsed() < 30000) {
        // Check system health
        if (!TestFramework::isSystemHealthy()) {
            errorCount++;
        }
        
        // Check memory growth
        size_t currentMemory = TestFramework::getCurrentMemoryUsage();
        if (currentMemory > initialMemory + 50 * 1024 * 1024) { // 50MB limit
            QFAIL("Excessive memory growth during long-running test");
        }
        
        QTest::qWait(1000); // Check every second
    }
    
    TestFramework::stopContinuousOperations();
    
    // Verify stability
    QVERIFY2(errorCount < 5, 
             QString("Too many errors during long-running test: %1").arg(errorCount).toLocal8Bit());
    
    qDebug() << "Long-running test completed with" << errorCount << "errors";
}

void PerformanceTests::testResourceCleanup()
{
    qDebug() << "Testing resource cleanup...";
    
    // Create many resources
    QList<QString> patternNames;
    for (int i = 0; i < 50; ++i) {
        QJsonObject pattern;
        pattern["type"] = "constant";
        pattern["duration_ms"] = 10000; // Long duration
        pattern["pressure_mmhg"] = -50;
        
        QString patternName = QString("cleanup_test_%1").arg(i);
        patternNames.append(patternName);
        TestFramework::startPattern(patternName, pattern);
    }
    
    // Verify resources are allocated
    QCOMPARE(TestFramework::getActivePatternCount(), 1); // Only one should be active
    
    // Trigger cleanup
    TestFramework::cleanupAllResources();
    
    // Verify cleanup
    QCOMPARE(TestFramework::getActivePatternCount(), 0);
    QVERIFY(TestFramework::areAllResourcesReleased());
    
    qDebug() << "Resource cleanup test completed";
}

// Test runner for this specific test class
QTEST_MAIN(PerformanceTests)
