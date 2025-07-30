#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QScreen>
#include <iostream>

#include "VacuumController.h"
#include "gui/MainWindow.h"
#include "safety/SafetyManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Vacuum Controller");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Medical Devices Inc");
    
    // Configure for large display (50-inch HDMI)
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        std::cout << "Display resolution: " << screenGeometry.width() 
                  << "x" << screenGeometry.height() << std::endl;
    }
    
    // Set application style for touch interface
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Apply custom stylesheet for medical device UI
    QString styleSheet = R"(
        QMainWindow {
            background-color: #f0f0f0;
            font-size: 16pt;
        }
        QPushButton {
            background-color: #4CAF50;
            border: 2px solid #45a049;
            color: white;
            padding: 15px 32px;
            text-align: center;
            font-size: 18pt;
            margin: 4px 2px;
            border-radius: 8px;
            min-height: 60px;
            min-width: 120px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        .emergency-button {
            background-color: #f44336;
            border: 2px solid #da190b;
        }
        .emergency-button:hover {
            background-color: #da190b;
        }
        QLabel {
            font-size: 14pt;
            color: #333333;
        }
        .status-label {
            font-size: 16pt;
            font-weight: bold;
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 5px;
            background-color: white;
        }
        .pressure-display {
            font-size: 24pt;
            font-weight: bold;
            color: #2196F3;
            background-color: #e3f2fd;
            border: 3px solid #2196F3;
            border-radius: 10px;
            padding: 20px;
            text-align: center;
        }
    )";
    app.setStyleSheet(styleSheet);
    
    try {
        // Initialize the vacuum controller system
        VacuumController controller;
        
        // Create and show main window
        MainWindow window(&controller);
        
        // Make window fullscreen for 50-inch display
        window.showFullScreen();
        
        // Enable touch events
        window.setAttribute(Qt::WA_AcceptTouchEvents, true);
        
        std::cout << "Vacuum Controller GUI started successfully" << std::endl;
        
        return app.exec();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Startup Error", 
                            QString("Failed to initialize vacuum controller: %1").arg(e.what()));
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
