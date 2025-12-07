#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QMap>
#include <QQueue>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QVariant>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>

class MemoryManager : public QObject
{
    Q_OBJECT

public:
    struct MemoryStats {
        qint64 totalAllocated;
        qint64 totalFreed;
        qint64 currentUsage;
        qint64 peakUsage;
        int activeAllocations;
        double fragmentationRatio;
        
        MemoryStats() : totalAllocated(0), totalFreed(0), currentUsage(0), 
                       peakUsage(0), activeAllocations(0), fragmentationRatio(0.0) {}
    };

    struct BufferInfo {
        QString name;
        qint64 size;
        qint64 capacity;
        qint64 lastAccessed;
        int accessCount;
        bool autoResize;
        
        BufferInfo() : size(0), capacity(0), lastAccessed(0), 
                      accessCount(0), autoResize(true) {}
    };

    explicit MemoryManager(QObject *parent = nullptr);
    ~MemoryManager();

    QSharedPointer<QByteArray> createBuffer(const QString& name, qint64 size, bool autoResize = true);
    void releaseBuffer(const QString& name);
    QSharedPointer<QByteArray> getBuffer(const QString& name) const;
    
    void cacheValue(const QString& key, const QVariant& value, int ttlSeconds = 3600);
    QVariant getCachedValue(const QString& key);
    void removeCacheItem(const QString& key);
    void clearCache();
    void setCacheMaxSize(qint64 maxSize);
    
    MemoryStats getMemoryStats() const;
    QMap<QString, BufferInfo> getBufferInfo() const;
    qint64 getCurrentUsage() const;
    qint64 getAvailableMemory() const;
    double getFragmentationRatio() const;
    
    void optimizeMemory();
    void defragmentMemory();
    void compactBuffers();
    void cleanupUnusedMemory();
    void triggerGarbageCollection();
    
    void setMemoryLimit(qint64 limitBytes);
    void setAutoCleanupEnabled(bool enabled);
    void setAutoCleanupInterval(int intervalMs);
    void setFragmentationThreshold(double threshold);

public Q_SLOTS:
    void performCleanup();
    void checkMemoryUsage();
    void optimizeBuffers();

Q_SIGNALS:
    void memoryLimitExceeded(qint64 currentUsage, qint64 limit);
    void memoryOptimized(qint64 freedBytes);
    void bufferCreated(const QString& name, qint64 size);
    void bufferReleased(const QString& name);
    void cacheItemExpired(const QString& key);

private Q_SLOTS:
    void onCleanupTimer();
    void onCacheCleanupTimer();

private:
    void initializeMemoryManager();
    void setupCleanupTimers();
    void cleanupExpiredCacheItems();
    void cleanupUnusedBuffers();
    void checkMemoryLimits();
    
    void trackAllocation(qint64 size);
    void trackDeallocation(qint64 size);
    void updatePeakUsage();
    void updateMemoryStats();
    
    struct CacheItem {
        QVariant value;
        qint64 timestamp;
        qint64 expiryTime;
        qint64 size;
        int accessCount;
        
        CacheItem() : timestamp(0), expiryTime(0), size(0), accessCount(0) {}
        CacheItem(const QVariant& v, int ttl) 
            : value(v), timestamp(QDateTime::currentMSecsSinceEpoch()),
              expiryTime(timestamp + ttl * 1000), accessCount(1) {
            size = calculateSize(v);
        }
        
        bool isExpired() const {
            return QDateTime::currentMSecsSinceEpoch() > expiryTime;
        }
        
    private:
        qint64 calculateSize(const QVariant& v) const;
    };
    
    QMap<QString, QSharedPointer<QByteArray>> m_buffers;
    QMap<QString, BufferInfo> m_bufferInfo;
    mutable QMutex m_bufferMutex;
    
    QMap<QString, CacheItem> m_cache;
    mutable QMutex m_cacheMutex;
    qint64 m_cacheMaxSize;
    
    MemoryStats m_stats;
    mutable QMutex m_statsMutex;
    
    qint64 m_memoryLimit;
    bool m_autoCleanupEnabled;
    int m_autoCleanupInterval;
    double m_fragmentationThreshold;
    
    QTimer* m_cleanupTimer;
    QTimer* m_cacheCleanupTimer;
    
    static const qint64 DEFAULT_MEMORY_LIMIT = 1024 * 1024 * 1024; // 1GB
    static const int DEFAULT_CLEANUP_INTERVAL = 60000;             // 1 minute
    static const int DEFAULT_CACHE_CLEANUP_INTERVAL = 300000;      // 5 minutes
    static constexpr double DEFAULT_FRAGMENTATION_THRESHOLD = 0.3; // 30%
    static const qint64 DEFAULT_CACHE_MAX_SIZE = 100 * 1024 * 1024; // 100MB
    static const int DEFAULT_CACHE_TTL = 3600;                     // 1 hour
};

#endif // MEMORYMANAGER_H