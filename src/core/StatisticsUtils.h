#ifndef STATISTICSUTILS_H
#define STATISTICSUTILS_H

#include <QList>
#include <cmath>

/**
 * @brief Utility class for calculating common statistical measures
 * 
 * Thread-safe: All methods are const and operate on copied data.
 * Used by HardwareTester, DataLogger, and other components needing
 * statistical analysis of sensor readings.
 */
class StatisticsUtils
{
public:
    /**
     * @brief Results from statistical calculation
     */
    struct Stats {
        double mean = 0.0;
        double variance = 0.0;
        double stdDev = 0.0;
        double coefficientOfVariation = 0.0;  // as percentage
        int sampleCount = 0;
        bool valid = false;  // false if insufficient data
    };

    /**
     * @brief Calculate comprehensive statistics from a list of readings
     * 
     * @param readings List of data points
     * @return Stats structure with calculated values; valid=false if readings empty
     */
    static Stats calculate(const QList<double>& readings)
    {
        Stats result;
        result.sampleCount = readings.size();
        
        if (readings.isEmpty()) {
            return result;  // valid = false
        }
        
        // Calculate sum and mean
        double sum = 0.0;
        for (double reading : readings) {
            sum += reading;
        }
        result.mean = sum / readings.size();
        
        // Calculate variance
        if (readings.size() > 1) {
            double varianceSum = 0.0;
            for (double reading : readings) {
                varianceSum += (reading - result.mean) * (reading - result.mean);
            }
            result.variance = varianceSum / readings.size();
            result.stdDev = std::sqrt(result.variance);
        }
        
        // Calculate coefficient of variation (as percentage)
        if (result.mean != 0.0) {
            result.coefficientOfVariation = (result.stdDev / result.mean) * 100.0;
        } else {
            result.coefficientOfVariation = 0.0;  // Avoid division by zero
        }
        
        result.valid = true;
        return result;
    }

    /**
     * @brief Calculate mean only (faster for simple cases)
     */
    static double calculateMean(const QList<double>& readings)
    {
        if (readings.isEmpty()) return 0.0;
        
        double sum = 0.0;
        for (double reading : readings) {
            sum += reading;
        }
        return sum / readings.size();
    }

    /**
     * @brief Calculate standard deviation only
     */
    static double calculateStdDev(const QList<double>& readings)
    {
        Stats stats = calculate(readings);
        return stats.stdDev;
    }

    /**
     * @brief Check if readings are stable (CV below threshold)
     * 
     * @param readings List of data points
     * @param maxCV Maximum acceptable coefficient of variation (percentage)
     * @return true if readings are stable (CV <= maxCV)
     */
    static bool isStable(const QList<double>& readings, double maxCV)
    {
        Stats stats = calculate(readings);
        return stats.valid && stats.coefficientOfVariation <= maxCV;
    }
};

#endif // STATISTICSUTILS_H

