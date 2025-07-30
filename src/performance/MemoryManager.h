#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QMap>
#include <QQueue>
#include <QSharedPointer>
#include <QWeakPointer>

/**
 * @brief Advanced memory management and optimization system
 * 
 * This system provides:
 * - Memory pool management for frequent allocations
 * - Automatic garbage collection and cleanup
 * - Memory leak detection and prevention
 * - Buffer management for data streams
 * - Cache optimization for frequently accessed data
 * - Memory usage monitoring and alerts
 * - Automatic memory defragmentation
 * - Smart pointer management
 */
class MemoryManager : public QObject
{
    Q_OBJECT

public:
    template<typename T>
    class MemoryPool {
    public:
        MemoryPool(int initialSize = 100, int maxSize = 1000)
            : m_maxSize(maxSize) {
            for (int i = 0; i < initialSize; ++i) {
                m_pool.enqueue(QSharedPointer<T>::create());
            }
        }
        
        QSharedPointer<T> acquire() {
            QMutexLocker locker(&m_mutex);
            if (m_pool.isEmpty()) {
                if (m_totalAllocated < m_maxSize) {
                    m_totalAllocated++;
                    return QSharedPointer<T>::create();
                }
                return QSharedPointer<T>();
            }
            return m_pool.dequeue();
        }
        
        void release(QSharedPointer<T> obj) {
            if (!obj) return;
            QMutexLocker locker(&m_mutex);
            if (m_pool.size() < m_maxSize / 2) {
                // Reset object state if needed
                obj.reset();
                m_pool.enqueue(obj);
            }
        }
        
        int poolSize() const {
            QMutexLocker locker(&m_mutex);
            return m_pool.size();
        }
        
        int totalAllocated() const {
            QMutexLocker locker(&m_mutex);
            return m_totalAllocated;
        }
        
    private:
        QQueue<QSharedPointer<T>> m_pool;
        mutable QMutex m_mutex;
        int m_maxSize;
        int m_totalAllocated = 0;
    };

    struct MemoryStats {
        qint64 totalAllocated;
        qint64 totalFreed;
        qint64 currentUsage;
        qint64 peakUsage;
        int activeAllocations;
        int poolHits;
        int poolMisses;
        double fragmentationRatio;
        
        MemoryStats() : totalAllocated(0), totalFreed(0), currentUsage(0), 
                       peakUsage(0), activeAllocations(0), poolHits(0), 
                       poolMisses(0), fragmentationRatio(0.0) {}
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

    // Memory pool management
    template<typename T>
    void createPool(const QString& poolName, int initialSize = 100, int maxSize = 1000) {
        QMutexLocker locker(&m_poolMutex);
        m_memoryPools[poolName] = QSharedPointer<MemoryPool<T>>::create(initialSize, maxSize);
    }
    
    template<typename T>
    QSharedPointer<T> acquireFromPool(const QString& poolName) {
        QMutexLocker locker(&m_poolMutex);
        auto pool = m_memoryPools.value(poolName);
        if (pool) {
            auto typedPool = pool.dynamicCast<MemoryPool<T>>();
            if (typedPool) {
                m_stats.poolHits++;
                return typedPool->acquire();
            }
        }
        m_stats.poolMisses++;
        return QSharedPointer<T>::create();
    }
    
    template<typename T>
    void releaseToPool(const QString& poolName, QSharedPointer<T> obj) {
        QMutexLocker locker(&m_poolMutex);
        auto pool = m_memoryPools.value(poolName);
        if (pool) {
            auto typedPool = pool.dynamicCast<MemoryPool<T>>();
            if (typedPool) {
                typedPool->release(obj);
            }
        }
    }
    
    // Buffer management
    QByteArray* createBuffer(const QString& name, qint64 initialSize, bool autoResize = true);
    void releaseBuffer(const QString& name);
    QByteArray* getBuffer(const QString& name);
    void resizeBuffer(const QString& name, qint64 newSize);
    void clearBuffer(const QString& name);
    
    // Cache management
    void setCacheItem(const QString& key, const QVariant& value, int ttlSeconds = 3600);
    QVariant getCacheItem(const QString& key);
    void removeCacheItem(const QString& key);
    void clearCache();
    void setCacheMaxSize(qint64 maxSize);
    
    // Memory monitoring
    MemoryStats getMemoryStats() const;
    QMap<QString, BufferInfo> getBufferInfo() const;
    qint64 getTotalMemoryUsage() const;
    qint64 getAvailableMemory() const;
    double getFragmentationRatio() const;
    
    // Memory optimization
    void optimizeMemory();
    void defragmentMemory();
    void compactBuffers();
    void cleanupUnusedMemory();
    void triggerGarbageCollection();
    
    // Configuration
    void setMemoryLimit(qint64 limitBytes);
    void setAutoCleanupEnabled(bool enabled);
    void setAutoCleanupInterval(int intervalMs);
    void setFragmentationThreshold(double threshold);

public slots:
    void performCleanup();
    void checkMemoryUsage();
    void optimizeBuffers();

signals:
    void memoryLimitExceeded(qint64 currentUsage, qint64 limit);
    void memoryOptimized(qint64 freedBytes);
    void bufferCreated(const QString& name, qint64 size);
    void bufferReleased(const QString& name);
    void cacheItemExpired(const QString& key);

private slots:
    void onCleanupTimer();
    void onCacheCleanupTimer();

private:
    void initializeMemoryManager();
    void setupCleanupTimers();
    void updateMemoryStats();
    void cleanupExpiredCacheItems();
    void cleanupUnusedBuffers();
    void checkMemoryLimits();
    
    // Memory tracking
    void trackAllocation(qint64 size);
    void trackDeallocation(qint64 size);
    void updatePeakUsage();
    
    // Cache management
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
    
    // Memory pools
    QMap<QString, QSharedPointer<void>> m_memoryPools;
    QMutex m_poolMutex;
    
    // Buffer management
    QMap<QString, QByteArray*> m_buffers;
    QMap<QString, BufferInfo> m_bufferInfo;
    QMutex m_bufferMutex;
    
    // Cache management
    QMap<QString, CacheItem> m_cache;
    QMutex m_cacheMutex;
    qint64 m_cacheMaxSize;
    qint64 m_currentCacheSize;
    
    // Memory statistics
    MemoryStats m_stats;
    mutable QMutex m_statsMutex;
    
    // Configuration
    qint64 m_memoryLimit;
    bool m_autoCleanupEnabled;
    int m_autoCleanupInterval;
    double m_fragmentationThreshold;
    
    // Cleanup timers
    QTimer* m_cleanupTimer;
    QTimer* m_cacheCleanupTimer;
    
    // Constants
    static const qint64 DEFAULT_MEMORY_LIMIT = 1024 * 1024 * 1024; // 1GB
    static const int DEFAULT_CLEANUP_INTERVAL = 60000;             // 1 minute
    static const int DEFAULT_CACHE_CLEANUP_INTERVAL = 300000;      // 5 minutes
    static const double DEFAULT_FRAGMENTATION_THRESHOLD = 0.3;     // 30%
    static const qint64 DEFAULT_CACHE_MAX_SIZE = 100 * 1024 * 1024; // 100MB
    static const int DEFAULT_CACHE_TTL = 3600;                     // 1 hour
};

#endif // MEMORYMANAGER_H
