#ifndef SAFEOPERATIONHELPER_H
#define SAFEOPERATIONHELPER_H

#include <QString>
#include <QDebug>
#include <functional>
#include <optional>

/**
 * @brief Utility for safe execution with consistent error handling
 * 
 * This class consolidates the try-catch-log-emit pattern that was duplicated
 * across many methods in the codebase, particularly in safety-critical components.
 * 
 * Example usage:
 * @code
 * // Before (duplicated pattern):
 * try {
 *     doSomething();
 * } catch (const std::exception& e) {
 *     m_lastError = QString("Operation failed: %1").arg(e.what());
 *     qCritical() << m_lastError;
 *     emit systemError(m_lastError);
 *     return false;
 * }
 * 
 * // After (using helper):
 * auto result = SafeOperationHelper::execute<bool>(
 *     "OperationName", "ComponentName",
 *     [this]() { return doSomething(); },
 *     [this](const QString& err) { emit systemError(err); }
 * );
 * if (!result.has_value()) {
 *     m_lastError = result.error();
 *     return false;
 * }
 * @endcode
 */
class SafeOperationHelper
{
public:
    /**
     * @brief Result type that holds either a value or an error
     */
    template<typename T>
    struct Result {
        std::optional<T> value;
        QString error;

        bool isSuccess() const { return value.has_value(); }
        operator bool() const { return isSuccess(); }
        T get() const { return value.value(); }
        T getOr(T defaultValue) const { return value.value_or(defaultValue); }
    };

    /**
     * @brief Callback type for error notifications
     */
    using ErrorCallback = std::function<void(const QString& error)>;

    /**
     * @brief Execute an operation with standardized error handling
     * @tparam ReturnType The return type of the operation
     * @param operationName Name for logging
     * @param componentName Component name for logging
     * @param operation The operation to execute
     * @param onError Optional callback invoked on error (for emitting signals)
     * @return Result containing the return value or error message
     */
    template<typename ReturnType>
    static Result<ReturnType> execute(
        const QString& operationName,
        const QString& componentName,
        std::function<ReturnType()> operation,
        ErrorCallback onError = nullptr)
    {
        Result<ReturnType> result;
        try {
            result.value = operation();
            return result;
        } catch (const std::exception& e) {
            result.error = QString("%1::%2 failed: %3")
                .arg(componentName, operationName, e.what());
            handleError(result.error, onError);
            return result;
        } catch (...) {
            result.error = QString("%1::%2 failed: Unknown exception")
                .arg(componentName, operationName);
            handleError(result.error, onError);
            return result;
        }
    }

    /**
     * @brief Execute a void operation with standardized error handling
     * @param operationName Name for logging
     * @param componentName Component name for logging
     * @param operation The operation to execute
     * @param onError Optional callback invoked on error
     * @return true if operation succeeded, false on exception
     */
    static bool executeVoid(
        const QString& operationName,
        const QString& componentName,
        std::function<void()> operation,
        ErrorCallback onError = nullptr)
    {
        try {
            operation();
            return true;
        } catch (const std::exception& e) {
            QString error = QString("%1::%2 failed: %3")
                .arg(componentName, operationName, e.what());
            handleError(error, onError);
            return false;
        } catch (...) {
            QString error = QString("%1::%2 failed: Unknown exception")
                .arg(componentName, operationName);
            handleError(error, onError);
            return false;
        }
    }

    /**
     * @brief Execute with retry logic
     * @param maxRetries Maximum number of retry attempts
     * @param retryDelayMs Delay between retries in milliseconds
     * @param other params same as execute()
     */
    template<typename ReturnType>
    static Result<ReturnType> executeWithRetry(
        const QString& operationName,
        const QString& componentName,
        std::function<ReturnType()> operation,
        int maxRetries,
        int retryDelayMs = 100,
        ErrorCallback onError = nullptr);

private:
    static void handleError(const QString& error, ErrorCallback onError) {
        qCritical() << error;
        if (onError) {
            onError(error);
        }
    }
};

#endif // SAFEOPERATIONHELPER_H

