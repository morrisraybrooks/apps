/**
 * PulserBench - measure the clitoral pulser on the bench (never on a body).
 *
 * Holds the cup (sealed on a flat plate) at each requested steady clitoral vacuum,
 * steps the pulser motor through PWM duties, prints the measured pulse swing at each,
 * and ends with the worst swing per duty across all steady levels.
 * Read the pulse rate at each step with a phone spectrum-analyzer app.
 * Procedure and how to write config/pulser_calibration.json:
 * docs/pulser-bench-calibration.md
 */
#include "HardwareManager.h"
#include "PneumaticController.h"

#include <QCommandLineParser>
#include <QMap>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTextStream>
#include <algorithm>
#include <atomic>
#include <csignal>

namespace {
std::atomic<bool> g_interrupted{false};

void onSignal(int) { g_interrupted = true; }

/// Keep the controller's 100 Hz loop running while we wait.
bool waitMs(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        if (g_interrupted) return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    return true;
}

/// Largest clitoral swing over durationMs, measured in short chunks so the
/// controller keeps regulating between them.
double measureSwing(HardwareManager& hw, int durationMs)
{
    double worst = 0.0;
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < durationMs && !g_interrupted) {
        worst = std::max(worst, hw.sampleClitoralSwing(40));
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }
    return worst;
}
}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("PulserBench");
    QTextStream out(stdout);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Bench calibration for the clitoral pulser. Cup sealed on a flat plate, soft plug filling\n"
        "about 70 % of the clitoral chamber. NEVER run this with the cup on a body.");
    parser.addHelpOption();
    parser.addOption({"steady", "Comma-separated steady clitoral vacuums to test, mmHg (default 10,20,35).",
                      "list", "10,20,35"});
    parser.addOption({"legs", "Leg vacuum holding the cup on the plate, mmHg (default 30).", "mmHg", "30"});
    parser.addOption({"duty", "Comma-separated pulser PWM duties, % (default 30..100).", "list",
                      "30,40,50,60,70,80,90,100"});
    parser.addOption({"seconds", "Seconds per duty step (default 10).", "s", "10"});
    parser.process(app);

    QList<double> steadyLevels;
    for (const QString& v : parser.value("steady").split(',', Qt::SkipEmptyParts)) steadyLevels << v.toDouble();
    const double legs = parser.value("legs").toDouble();
    const int seconds = std::max(4, parser.value("seconds").toInt());
    QList<double> duties;
    for (const QString& d : parser.value("duty").split(',', Qt::SkipEmptyParts)) duties << d.toDouble();

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    HardwareManager hw;
    hw.setPneumaticsBenchMode(true);
    if (!hw.initialize() || !hw.pneumatics()) {
        out << "Hardware did not initialize - stopping.\n";
        return 1;
    }
    PneumaticController* air = hw.pneumatics();

    auto abort = [&](const QString& why, int code) {
        air->emergencyStop();
        out << why << "\n";
        out.flush();
        return code;
    };

    // Worst swing per duty over every steady level (the number for peak_mmhg)
    QMap<double, double> worstSwing;
    for (double duty : duties) worstSwing[duty] = 0.0;
    bool capped = false;

    for (double steady : steadyLevels) {
        out << "\nHolding legs at " << legs << " mmHg and the clitoral chamber at " << steady
            << " mmHg...\n";
        out.flush();
        air->setBenchDuty(-1.0);
        air->setLegTarget(legs);
        air->setClitoralTarget(steady);
        if (!waitMs(5000)) return abort("Interrupted - emergency stop.", 130);

        out << "duty%  swing_mmHg   (read pulses/second from the phone app at each step)\n";
        for (double duty : duties) {
            air->setBenchDuty(duty);
            out << QString("duty=%1%  ...").arg(duty, 3, 'f', 0);
            out.flush();
            if (!waitMs((seconds - 2) * 1000)) return abort("\nInterrupted - emergency stop.", 130);
            const double swing = measureSwing(hw, 2000);
            worstSwing[duty] = std::max(worstSwing[duty], swing);
            out << QString("  swing=%1 mmHg\n").arg(swing, 0, 'f', 1);
            out.flush();
            if (steady + swing > 75.0) {
                // Higher duties only get bigger: skip the rest of this level, vent, carry on
                air->setBenchDuty(0.0);
                out << QString("  steady %1 + swing %2 mmHg would pass the 75 mmHg cap - "
                               "skipping the higher duties at this level.\n")
                           .arg(steady).arg(swing, 0, 'f', 1);
                capped = true;
                break;
            }
        }
        air->setBenchDuty(0.0);
        air->setClitoralTarget(0.0);
        if (!waitMs(1500)) return abort("Interrupted - emergency stop.", 130);
    }

    out << "\nWorst swing per duty (use these as peak_mmhg):\n";
    for (auto it = worstSwing.cbegin(); it != worstSwing.cend(); ++it) {
        out << QString("  duty=%1%  peak_mmhg=%2\n").arg(it.key(), 3, 'f', 0).arg(it.value(), 0, 'f', 1);
    }
    if (capped) {
        out << "Some steps were skipped at the cap: the pulser will hold the steady level\n"
               "low enough at those rates; that is expected.\n";
    }

    air->setBenchDuty(-1.0);
    air->stop();
    out << "\nDone. Chambers vented; the tank keeps its vacuum.\n"
           "Write config/pulser_calibration.json from these numbers (docs/pulser-bench-calibration.md).\n";
    return 0;
}
