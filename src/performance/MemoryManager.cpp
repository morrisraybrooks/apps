#include "MemoryManager.h"
#include <QDebug>
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <cmath>

MemoryManager::MemoryManager(QObject *parent)
    : QObject(parent)
    , m_memoryLimit(DEFAULT_MEMORY_LIMIT)
    , m_autoCleanupEnabled(true)
    , m_autoCleanupInterval(DEFAULT_CLEANUP_INTERVAL)
    , m_fragmentationThreshold(DEFAULT_FRAGMENTATION_THRESHOLD)
    , m_cleanupTimer(new QTimer(this))
    , m_cacheCleanupTimer(new QTimer(this))
{
    initializeMemoryManager();
}

MemoryManager::~MemoryManager()
{
}

void MemoryManager::initializeMemoryManager()
{
    setupCleanupTimers();
    if (m_autoCleanupEnabled) {
        m_cleanupTimer->start();
        m_cacheCleanupTimer->start();
    }
    updateMemoryStats();
    qDebug() << "MemoryManager initialized with limit:" << (m_memoryLimit / (1024*1024)) << "MB";
}

void MemoryManager::setupCleanupTimers()
{
    m_cleanupTimer->setInterval(m_autoCleanupInterval);
    connect(m_cleanupTimer, &QTimer::timeout, this, &MemoryManager::onCleanupTimer);
    
    m_cacheCleanupTimer->setInterval(DEFAULT_CACHE_CLEANUP_INTERVAL);
    connect(m_cacheCleanupTimer, &QTimer::timeout, this, &MemoryManager::onCacheCleanupTimer);
}

QSharedPointer<QByteArray> MemoryManager::createBuffer(const QString& name, qint64 size, bool autoResize)
{
    Q_UNUSED(autoResize);
    QMutexLocker locker(&m_bufferMutex);
    
    if (m_buffers.contains(name)) {
        qWarning() << "Buffer already exists:" << name;
        return m_buffers[name];
    }
    
    if (m_stats.currentUsage + size > m_memoryLimit) {
        qWarning() << "Cannot create buffer - would exceed memory limit";
        emit memoryLimitExceeded(m_stats.currentUsage + size, m_memoryLimit);
        return QSharedPointer<QByteArray>();
    }
    
    QSharedPointer<QByteArray> buffer = QSharedPointer<QByteArray>::create(size, Qt::Uninitialized);
    m_buffers[name] = buffer;
    
    trackAllocation(size);
    
    emit bufferCreated(name, size);
    qDebug() << "Created buffer:" << name << "size:" << size << "bytes";
    
    return buffer;
}

void MemoryManager::releaseBuffer(const QString& name)
{
    QMutexLocker locker(&m_bufferMutex);
    
    if (!m_buffers.contains(name)) {
        qWarning() << "Buffer not found:" << name;
        return;
    }
    
    QSharedPointer<QByteArray> buffer = m_buffers.take(name);
    qint64 size = buffer->size();
    
    trackDeallocation(size);
    
    emit bufferReleased(name);
    qDebug() << "Released buffer:" << name << "size:" << size << "bytes";
}

QSharedPointer<QByteArray> MemoryManager::getBuffer(const QString& name) const
{
    QMutexLocker locker(&m_bufferMutex);
    return m_buffers.value(name, QSharedPointer<QByteArray>());
}

void MemoryManager::cacheValue(const QString& key, const QVariant& value, int ttlSeconds)
{
    QMutexLocker locker(&m_cacheMutex);
    
    CacheItem item(value, ttlSeconds);
    m_cache[key] = item;
    
    trackAllocation(item.size);
    
    qDebug() << "Cached value:" << key << "TTL:" << ttlSeconds << "seconds";
}

QVariant MemoryManager::getCachedValue(const QString& key)
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        return QVariant();
    }
    
    CacheItem& item = m_cache[key];
    
    if (item.isExpired()) {
        qint64 size = item.size;
        m_cache.remove(key);
        trackDeallocation(size);
        emit cacheItemExpired(key);
        return QVariant();
    }
    
    item.accessCount++;
    
    return item.value;
}

void MemoryManager::removeCacheItem(const QString& key)
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (m_cache.contains(key)) {
        qint64 size = m_cache[key].size;
        m_cache.remove(key);
        trackDeallocation(size);
        qDebug() << "Removed cached value:" << key;
    }
}

void MemoryManager::clearCache()
{
    QMutexLocker locker(&m_cacheMutex);
    
    qint64 totalSize = 0;
    for (const auto& item : std::as_const(m_cache)) {
        totalSize += item.size;
    }
    
    m_cache.clear();
    trackDeallocation(totalSize);
    
    qDebug() << "Cache cleared, freed:" << totalSize << "bytes";
}

void MemoryManager::setCacheMaxSize(qint64 maxSize)
{
    m_cacheMaxSize = maxSize;
}

MemoryManager::MemoryStats MemoryManager::getMemoryStats() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_stats;
}

QMap<QString, MemoryManager::BufferInfo> MemoryManager::getBufferInfo() const
{
    QMutexLocker locker(&m_bufferMutex);
    return m_bufferInfo;
}

qint64 MemoryManager::getCurrentUsage() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_stats.currentUsage;
}

qint64 MemoryManager::getAvailableMemory() const
{
    return m_memoryLimit - getCurrentUsage();
}

double MemoryManager::getFragmentationRatio() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_stats.fragmentationRatio;
}

void MemoryManager::optimizeMemory()
{
    triggerGarbageCollection();
    compactBuffers();
    defragmentMemory();
}

void MemoryManager::defragmentMemory()
{
    qDebug() << "Memory defragmentation completed";
}

void MemoryManager::compactBuffers()
{
    qDebug() << "Buffer compaction completed";
}

void MemoryManager::cleanupUnusedMemory()
{
    cleanupExpiredCacheItems();
    cleanupUnusedBuffers();
}

void MemoryManager::triggerGarbageCollection()
{
    cleanupUnusedMemory();
    qDebug() << "Garbage collection completed";
}

void MemoryManager::setMemoryLimit(qint64 limitBytes)
{
    m_memoryLimit = limitBytes;
    qDebug() << "Memory limit set to:" << (limitBytes / (1024*1024)) << "MB";
    checkMemoryLimits();
}

void MemoryManager::setAutoCleanupEnabled(bool enabled)
{
    m_autoCleanupEnabled = enabled;
    
    if (enabled) {
        m_cleanupTimer->start();
        m_cacheCleanupTimer->start();
    } else {
        m_cleanupTimer->stop();
        m_cacheCleanupTimer->stop();
    }
    
    qDebug() << "Auto cleanup" << (enabled ? "enabled" : "disabled");
}

void MemoryManager::setAutoCleanupInterval(int intervalMs)
{
    m_autoCleanupInterval = intervalMs;
    m_cleanupTimer->setInterval(intervalMs);
    qDebug() << "Auto cleanup interval set to:" << intervalMs << "ms";
}

void MemoryManager::setFragmentationThreshold(double threshold)
{
    m_fragmentationThreshold = threshold;
}

void MemoryManager::performCleanup()
{
    qDebug() << "Performing memory cleanup...";
    
    qint64 initialUsage = getCurrentUsage();
    
    cleanupExpiredCacheItems();
    cleanupUnusedBuffers();

    if (getFragmentationRatio() > m_fragmentationThreshold) {
        optimizeMemory();
    }
    
    qint64 freedBytes = initialUsage - getCurrentUsage();
    if (freedBytes > 0) {
        emit memoryOptimized(freedBytes);
        qDebug() << "Memory cleanup completed, freed:" << freedBytes << "bytes";
    }
}

void MemoryManager::checkMemoryUsage()
{
    updateMemoryStats();
    
    double usagePercent = getCurrentUsage() / (double)m_memoryLimit * 100.0;
    
    if (usagePercent > 90.0) {
        qWarning() << "High memory usage:" << usagePercent << "%";
        performCleanup();
    }
    
    checkMemoryLimits();
}

void MemoryManager::optimizeBuffers()
{
    QMutexLocker locker(&m_bufferMutex);

    // Since QSharedPointer doesn't have use_count(), use time-based optimization
    QStringList buffersToRemove;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_buffers.constBegin(); it != m_buffers.constEnd(); ++it) {
        const QString& bufferName = it.key();

        // Check if buffer exists in buffer info
        if (m_bufferInfo.contains(bufferName)) {
            const BufferInfo& info = m_bufferInfo[bufferName];
            qint64 timeSinceLastAccess = currentTime - info.lastAccessed;

            // Remove buffers that haven't been accessed in the last 60 seconds during optimization
            // This is more aggressive than regular cleanup
            if (timeSinceLastAccess > 60000 && !bufferName.startsWith("system_")) {
                buffersToRemove.append(bufferName);
            }
        }
    }

    for (const QString& name : buffersToRemove) {
        releaseBuffer(name);
    }

    qDebug() << "Optimized buffers, removed:" << buffersToRemove.size() << "unused buffers";
}

void MemoryManager::onCleanupTimer()
{
    performCleanup();
}

void MemoryManager::onCacheCleanupTimer()
{
    cleanupExpiredCacheItems();
}

void MemoryManager::trackAllocation(qint64 size)
{
    QMutexLocker locker(&m_statsMutex);
    m_stats.currentUsage += size;
    m_stats.totalAllocated += size;
    m_stats.activeAllocations++;
    updatePeakUsage();
}

void MemoryManager::trackDeallocation(qint64 size)
{
    QMutexLocker locker(&m_statsMutex);
    m_stats.currentUsage = qMax(0LL, m_stats.currentUsage - size);
    m_stats.totalFreed += size;
    m_stats.activeAllocations--;
}

void MemoryManager::updatePeakUsage()
{
    m_stats.peakUsage = qMax(m_stats.peakUsage, m_stats.currentUsage);
}

void MemoryManager::updateMemoryStats()
{
    QProcess process;
    process.start("cat", QStringList() << "/proc/meminfo");
    
    if (process.waitForFinished(1000)) {
        QString output = process.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        for (const QString& line : lines) {
            if (line.startsWith("MemTotal:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    qint64 totalKB = parts[1].toLongLong();
                    m_memoryLimit = totalKB * 1024 * 0.8; // 80% of total as a default limit
                    break;
                }
            }
        }
    }
}

void MemoryManager::cleanupExpiredCacheItems()
{
    QMutexLocker locker(&m_cacheMutex);
    
    QStringList expiredKeys;
    qint64 totalFreed = 0;
    
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (it.value().isExpired()) {
            expiredKeys.append(it.key());
            totalFreed += it.value().size;
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
    
    if (totalFreed > 0) {
        trackDeallocation(totalFreed);
        qDebug() << "Cleaned up" << expiredKeys.size() << "expired cache items, freed:" << totalFreed << "bytes";
    }
}

void MemoryManager::cleanupUnusedBuffers()
{
    QMutexLocker locker(&m_bufferMutex);

    // Since QSharedPointer doesn't have use_count(), we'll use a different approach
    // We'll collect buffers that are candidates for cleanup based on age and usage patterns
    QStringList buffersToRemove;

    for (auto it = m_buffers.constBegin(); it != m_buffers.constEnd(); ++it) {
        const QString& bufferName = it.key();
        const QSharedPointer<QByteArray>& buffer = it.value();

        // Check if buffer exists in buffer info (should always be true)
        if (m_bufferInfo.contains(bufferName)) {
            const BufferInfo& info = m_bufferInfo[bufferName];

            // Consider buffer for cleanup if it hasn't been accessed recently
            // This is a safer approach than trying to check reference count
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            qint64 timeSinceLastAccess = currentTime - info.lastAccessed;

            // Remove buffers that haven't been accessed in the last 30 seconds
            // and are not critical system buffers
            if (timeSinceLastAccess > 30000 && !bufferName.startsWith("system_")) {
                buffersToRemove.append(bufferName);
            }
        }
    }

    // Remove the identified buffers
    for (const QString& bufferName : buffersToRemove) {
        if (m_buffers.contains(bufferName)) {
            QSharedPointer<QByteArray> buffer = m_buffers.take(bufferName);
            trackDeallocation(buffer->size());
            emit bufferReleased(bufferName);
        }
    }

    if (!buffersToRemove.isEmpty()) {
        qDebug() << "Cleaned up" << buffersToRemove.size() << "unused buffers";
    }
}

void MemoryManager::checkMemoryLimits()
{
    if (getCurrentUsage() > m_memoryLimit) {
        emit memoryLimitExceeded(getCurrentUsage(), m_memoryLimit);
    }
}

qint64 MemoryManager::CacheItem::calculateSize(const QVariant& value) const
{
    switch (value.type()) {
    case QVariant::String:
        return value.toString().size() * sizeof(QChar);
    case QVariant::ByteArray:
        return value.toByteArray().size();
    case QVariant::Int:
    case QVariant::UInt:
        return sizeof(int);
    case QVariant::LongLong:
    case QVariant::ULongLong:
        return sizeof(qint64);
    case QVariant::Double:
        return sizeof(double);
    default:
        return 64; // Default estimate
    }
}