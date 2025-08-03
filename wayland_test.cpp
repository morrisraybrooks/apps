#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QFont>
#include <QScreen>
#include <QDebug>

class VacuumControllerDemo : public QMainWindow
{
    Q_OBJECT

public:
    VacuumControllerDemo(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setupUI();
        setupTimer();
        
        // Set window properties for medical device
        setWindowTitle("Vacuum Controller - Wayland Demo");
        setMinimumSize(800, 600);
        
        // Apply medical device styling
        setStyleSheet(R"(
            QMainWindow {
                background-color: #f0f0f0;
            }
            QLabel#titleLabel {
                font-size: 24px;
                font-weight: bold;
                color: #2c3e50;
                padding: 10px;
            }
            QLabel#statusLabel {
                font-size: 18px;
                color: #27ae60;
                padding: 5px;
            }
            QPushButton {
                font-size: 16px;
                padding: 15px 30px;
                border-radius: 8px;
                border: 2px solid #3498db;
                background-color: #3498db;
                color: white;
                min-width: 120px;
                min-height: 50px;
            }
            QPushButton:hover {
                background-color: #2980b9;
                border-color: #2980b9;
            }
            QPushButton:pressed {
                background-color: #21618c;
            }
            QPushButton#emergencyButton {
                background-color: #e74c3c;
                border-color: #e74c3c;
                font-weight: bold;
            }
            QPushButton#emergencyButton:hover {
                background-color: #c0392b;
                border-color: #c0392b;
            }
            QProgressBar {
                border: 2px solid #bdc3c7;
                border-radius: 5px;
                text-align: center;
                font-size: 14px;
                min-height: 25px;
            }
            QProgressBar::chunk {
                background-color: #3498db;
                border-radius: 3px;
            }
        )");
    }

private slots:
    void startVacuum()
    {
        m_statusLabel->setText("Status: VACUUM ACTIVE");
        m_statusLabel->setStyleSheet("color: #e67e22; font-weight: bold;");
        m_pressureBar->setValue(75);
        m_startButton->setEnabled(false);
        m_stopButton->setEnabled(true);
        
        qDebug() << "Vacuum started - Wayland display working!";
    }
    
    void stopVacuum()
    {
        m_statusLabel->setText("Status: READY");
        m_statusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
        m_pressureBar->setValue(0);
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        
        qDebug() << "Vacuum stopped";
    }
    
    void emergencyStop()
    {
        m_statusLabel->setText("Status: EMERGENCY STOP");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
        m_pressureBar->setValue(0);
        m_startButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        
        qDebug() << "EMERGENCY STOP activated!";
        
        // Re-enable after 3 seconds
        QTimer::singleShot(3000, this, [this]() {
            m_statusLabel->setText("Status: READY");
            m_statusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
            m_startButton->setEnabled(true);
        });
    }
    
    void updateDisplay()
    {
        // Simulate pressure fluctuation
        static int counter = 0;
        counter++;
        
        if (m_pressureBar->value() > 0) {
            int variation = (counter % 10) - 5; // ±5 variation
            int newValue = qBound(0, 75 + variation, 100);
            m_pressureBar->setValue(newValue);
        }
        
        // Update platform info
        m_platformLabel->setText(QString("Platform: %1 | Display: %2x%3")
            .arg(QApplication::platformName())
            .arg(screen()->size().width())
            .arg(screen()->size().height()));
    }

private:
    void setupUI()
    {
        auto *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setSpacing(20);
        mainLayout->setContentsMargins(30, 30, 30, 30);
        
        // Title
        auto *titleLabel = new QLabel("Medical Vacuum Controller", this);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);
        
        // Platform info
        m_platformLabel = new QLabel(this);
        m_platformLabel->setAlignment(Qt::AlignCenter);
        m_platformLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");
        mainLayout->addWidget(m_platformLabel);
        
        // Status
        m_statusLabel = new QLabel("Status: READY", this);
        m_statusLabel->setObjectName("statusLabel");
        m_statusLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_statusLabel);
        
        // Pressure bar
        auto *pressureLayout = new QVBoxLayout();
        auto *pressureTitle = new QLabel("Vacuum Pressure", this);
        pressureTitle->setAlignment(Qt::AlignCenter);
        pressureTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #34495e;");
        
        m_pressureBar = new QProgressBar(this);
        m_pressureBar->setRange(0, 100);
        m_pressureBar->setValue(0);
        m_pressureBar->setFormat("%v mmHg");
        
        pressureLayout->addWidget(pressureTitle);
        pressureLayout->addWidget(m_pressureBar);
        mainLayout->addLayout(pressureLayout);
        
        // Control buttons
        auto *buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(20);
        
        m_startButton = new QPushButton("START VACUUM", this);
        m_stopButton = new QPushButton("STOP VACUUM", this);
        auto *emergencyButton = new QPushButton("EMERGENCY STOP", this);
        
        emergencyButton->setObjectName("emergencyButton");
        m_stopButton->setEnabled(false);
        
        connect(m_startButton, &QPushButton::clicked, this, &VacuumControllerDemo::startVacuum);
        connect(m_stopButton, &QPushButton::clicked, this, &VacuumControllerDemo::stopVacuum);
        connect(emergencyButton, &QPushButton::clicked, this, &VacuumControllerDemo::emergencyStop);
        
        buttonLayout->addWidget(m_startButton);
        buttonLayout->addWidget(m_stopButton);
        buttonLayout->addWidget(emergencyButton);
        
        mainLayout->addLayout(buttonLayout);
        mainLayout->addStretch();
        
        // Wayland info
        auto *waylandInfo = new QLabel("🖥️ Running on Wayland Display System", this);
        waylandInfo->setAlignment(Qt::AlignCenter);
        waylandInfo->setStyleSheet("color: #2ecc71; font-size: 14px; font-weight: bold; padding: 10px;");
        mainLayout->addWidget(waylandInfo);
    }
    
    void setupTimer()
    {
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &VacuumControllerDemo::updateDisplay);
        timer->start(500); // Update every 500ms
    }
    
    QLabel *m_statusLabel;
    QLabel *m_platformLabel;
    QProgressBar *m_pressureBar;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
};

int main(int argc, char *argv[])
{
    // Configure Qt Wayland for optimal display
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
    qputenv("QT_SCALE_FACTOR", "1.2");  // Slight scaling for better visibility
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    qputenv("QT_FONT_DPI", "120");
    
    QApplication app(argc, argv);
    
    app.setApplicationName("Vacuum Controller Demo");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Medical Devices Inc");
    
    qDebug() << "=== Vacuum Controller Wayland Demo ===";
    qDebug() << "Qt Platform:" << QApplication::platformName();
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Available screens:" << QApplication::screens().size();
    
    if (!QApplication::screens().isEmpty()) {
        auto *screen = QApplication::primaryScreen();
        qDebug() << "Primary screen:" << screen->size() << "DPI:" << screen->logicalDotsPerInch();
    }
    
    VacuumControllerDemo window;
    window.show();
    
    qDebug() << "Application started successfully on Wayland!";
    
    return app.exec();
}

#include "wayland_test.moc"
