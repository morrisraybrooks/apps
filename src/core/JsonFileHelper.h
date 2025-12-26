#ifndef JSONFILEHELPER_H
#define JSONFILEHELPER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

/**
 * @brief Utility class for JSON file I/O operations
 *
 * Consolidates duplicate JSON file loading and saving patterns found across
 * 15+ files in the codebase. Provides thread-safe, error-handling wrappers
 * for common JSON file operations.
 *
 * Usage:
 *   QJsonObject obj;
 *   if (JsonFileHelper::loadObject("config.json", obj)) { ... }
 *
 *   JsonFileHelper::saveObject("data.json", myObject);
 */
namespace JsonFileHelper {

/**
 * @brief Load a JSON object from file
 * @param filePath Path to the JSON file
 * @param result Output parameter for the loaded object
 * @param errorMessage Optional output for error description
 * @return true on success, false on failure
 */
inline bool loadObject(const QString& filePath, QJsonObject& result, QString* errorMessage = nullptr) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QString("Cannot open file: %1").arg(filePath);
        }
        qWarning() << "JsonFileHelper: Cannot open file:" << filePath;
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QString("JSON parse error: %1").arg(parseError.errorString());
        }
        qWarning() << "JsonFileHelper: JSON parse error in" << filePath << ":" << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        if (errorMessage) {
            *errorMessage = "JSON document is not an object";
        }
        qWarning() << "JsonFileHelper: JSON document is not an object:" << filePath;
        return false;
    }

    result = doc.object();
    return true;
}

/**
 * @brief Load a JSON array from file
 * @param filePath Path to the JSON file
 * @param result Output parameter for the loaded array
 * @param errorMessage Optional output for error description
 * @return true on success, false on failure
 */
inline bool loadArray(const QString& filePath, QJsonArray& result, QString* errorMessage = nullptr) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QString("Cannot open file: %1").arg(filePath);
        }
        qWarning() << "JsonFileHelper: Cannot open file:" << filePath;
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QString("JSON parse error: %1").arg(parseError.errorString());
        }
        qWarning() << "JsonFileHelper: JSON parse error in" << filePath << ":" << parseError.errorString();
        return false;
    }

    if (!doc.isArray()) {
        if (errorMessage) {
            *errorMessage = "JSON document is not an array";
        }
        qWarning() << "JsonFileHelper: JSON document is not an array:" << filePath;
        return false;
    }

    result = doc.array();
    return true;
}

/**
 * @brief Save a JSON object to file
 * @param filePath Path to the JSON file
 * @param object The JSON object to save
 * @param compact Whether to use compact formatting (default: false for readability)
 * @param createDirs Whether to create parent directories if they don't exist
 * @return true on success, false on failure
 */
inline bool saveObject(const QString& filePath, const QJsonObject& object,
                       bool compact = false, bool createDirs = true) {
    if (createDirs) {
        QDir().mkpath(QFileInfo(filePath).absolutePath());
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "JsonFileHelper: Cannot write to file:" << filePath;
        return false;
    }

    QJsonDocument doc(object);
    file.write(compact ? doc.toJson(QJsonDocument::Compact) : doc.toJson());
    file.close();
    return true;
}

/**
 * @brief Save a JSON array to file
 * @param filePath Path to the JSON file
 * @param array The JSON array to save
 * @param compact Whether to use compact formatting
 * @param createDirs Whether to create parent directories if they don't exist
 * @return true on success, false on failure
 */
inline bool saveArray(const QString& filePath, const QJsonArray& array,
                      bool compact = false, bool createDirs = true) {
    if (createDirs) {
        QDir().mkpath(QFileInfo(filePath).absolutePath());
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "JsonFileHelper: Cannot write to file:" << filePath;
        return false;
    }

    QJsonDocument doc(array);
    file.write(compact ? doc.toJson(QJsonDocument::Compact) : doc.toJson());
    file.close();
    return true;
}

} // namespace JsonFileHelper

#endif // JSONFILEHELPER_H

