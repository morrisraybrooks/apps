#include "GuiUpdateThread.h"
#include <QDebug>
#include <QApplication>

GuiUpdateThread::GuiUpdateThread(QObject *parent)
    : QThread(parent)
    , m_running(false)
    , m_updateInterval(33) // ~30 FPS
    , m_frameCount(0)
    , m_lastFrameTime(0)
    , m_averageFrameRate(0.0)
{
    m_frameTimer.start();
}

GuiUpdateThread::~GuiUpdateThread()
{
    stop();
    wait(5000); // Wait up to 5 seconds for thread to finish
}

void GuiUpdateThread::start()
{
    if (!m_running) {
        m_running = true;
        QThread::start();
        qDebug() << "GUI Update Thread started";
    }
}

void GuiUpdateThread::stop()
{
    if (m_running) {
        m_running = false;
        qDebug() << "GUI Update Thread stop requested";
    }
}

void GuiUpdateThread::setUpdateInterval(int intervalMs)
{
    if (intervalMs > 0 && intervalMs <= 1000) {
        m_updateInterval = intervalMs;
        qDebug() << "GUI update interval set to" << intervalMs << "ms";
    }
}

double GuiUpdateThread::getFrameRate() const
{
    return m_averageFrameRate;
}

int GuiUpdateThread::getFrameCount() const
{
    return m_frameCount;
}

void GuiUpdateThread::run()
{
    qDebug() << "GUI Update Thread running";
    
    while (m_running) {
        qint64 frameStartTime = m_frameTimer.elapsed();
        
        // Emit update signal for GUI components
        emit updateGUI();
        
        // Calculate frame rate
        calculateFrameRate(frameStartTime);
        
        // Sleep for the remaining time to maintain target frame rate
        qint64 frameEndTime = m_frameTimer.elapsed();
        qint64 frameTime = frameEndTime - frameStartTime;
        
        if (frameTime < m_updateInterval) {
            msleep(m_updateInterval - frameTime);
        }
        
        // Check if we should emit performance metrics
        if (m_frameCount % 30 == 0) { // Every 30 frames (~1 second at 30 FPS)
            emit performanceUpdate(m_averageFrameRate, frameTime);
        }
    }
    
    qDebug() << "GUI Update Thread finished";
}

void GuiUpdateThread::calculateFrameRate(qint64 currentTime)
{
    m_frameCount++;
    
    if (m_lastFrameTime == 0) {
        m_lastFrameTime = currentTime;
        return;
    }
    
    qint64 timeDelta = currentTime - m_lastFrameTime;
    
    if (timeDelta > 0) {
        double instantFrameRate = 1000.0 / timeDelta;
        
        // Use exponential moving average for smooth frame rate calculation
        if (m_averageFrameRate == 0.0) {
            m_averageFrameRate = instantFrameRate;
        } else {
            m_averageFrameRate = 0.9 * m_averageFrameRate + 0.1 * instantFrameRate;
        }
    }
    
    m_lastFrameTime = currentTime;
}
