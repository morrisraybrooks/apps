#ifndef PERFORMANCETESTS_H
#define PERFORMANCETESTS_H

#include <QObject>
#include <QTest>

/**
 * @brief Performance testing class for vacuum controller
 * 
 * Tests system performance characteristics including:
 * - Application startup time
 * - Memory usage and leak detection
 * - CPU usage under load
 * - Response times for critical operations
 * - Data throughput and processing speed
 * - Concurrent operation performance
 * - Long-running stability
 * - Resource cleanup efficiency
 */
class PerformanceTests : public QObject
{
    Q_OBJECT

public:
    explicit PerformanceTests(QObject *parent = nullptr);

private slots:
    // Test framework setup/teardown
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Performance benchmarks
    void testStartupTime();
    void testMemoryUsage();
    void testCPUUsage();
    void testResponseTimes();
    void testThroughput();
    
    // Stress tests
    void testConcurrentOperations();
    void testMemoryLeaks();
    void testLongRunningStability();
    void testResourceCleanup();

private:
    // Helper methods
    void measureOperationTime(const QString& operation);
    void monitorResourceUsage(int durationMs);
    bool verifyPerformanceMetrics();
    
    // Performance thresholds for Raspberry Pi 4
    static constexpr int MAX_STARTUP_TIME_MS = 3000;
    static constexpr int MAX_GPIO_RESPONSE_MS = 10;
    static constexpr int MAX_SENSOR_READ_MS = 5;
    static constexpr int MAX_PATTERN_START_MS = 50;
    static constexpr double MAX_CPU_USAGE_PERCENT = 80.0;
    static constexpr size_t MAX_MEMORY_INCREASE_MB = 10;
    static constexpr double MIN_THROUGHPUT_OPS_SEC = 1000.0;
};

#endif // PERFORMANCETESTS_H
