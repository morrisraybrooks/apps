#ifndef PNEUMATICSETTINGS_H
#define PNEUMATICSETTINGS_H

#include <QJsonObject>

/**
 * @brief Tunable numbers for tank, chamber regulation, seal watch and pulser.
 *
 * Defaults are the approved design values (docs/superpowers/specs/
 * 2026-11-16-clitoral-pulser-system-design.md). Optionally overridden by the
 * "pneumatics" section of config/settings.json (snake_case keys).
 */
struct PneumaticSettings {
    double tankFillMmHg = 70;        ///< Pump stops when the tank reaches this
    double tankRestartMmHg = 55;     ///< Pump restarts when the tank falls to this
    double topUpMarginMmHg = 5;      ///< Tank must be this much deeper than a chamber's target
    double legMinMmHg = 30;          ///< Leg target range (0 = released)
    double legMaxMmHg = 50;
    double clitMaxMmHg = 60;         ///< Clitoral target ceiling (stays below the tank)
    double feedBelowMmHg = 2;        ///< Open the feed valve below target - this
    double ventAboveMmHg = 3;        ///< Open the vent above target + this
    int feedPulseMs = 30;            ///< Length of one feed-valve opening
    int averageWindowMs = 200;       ///< Clitoral averaging window while the pulser runs
    double leakDropMmHg = 10;        ///< Leak: legs this far below target
    double leakRateMmHgPer100ms = 5; ///< Leak: legs falling faster than this
    int recoveryAttempts = 3;        ///< Reseal attempts before a safe stop
    int recoveryWindowMs = 2000;     ///< Time allowed per reseal attempt
    bool benchMode = false;          ///< Only the PulserBench tool sets this

    static PneumaticSettings fromJson(const QJsonObject& section)
    {
        PneumaticSettings s;
        auto num = [&section](const char* key, double fallback) {
            return section.value(key).toDouble(fallback);
        };
        s.tankFillMmHg = num("tank_fill_mmhg", s.tankFillMmHg);
        s.tankRestartMmHg = num("tank_restart_mmhg", s.tankRestartMmHg);
        s.topUpMarginMmHg = num("top_up_margin_mmhg", s.topUpMarginMmHg);
        s.legMinMmHg = num("leg_min_mmhg", s.legMinMmHg);
        s.legMaxMmHg = num("leg_max_mmhg", s.legMaxMmHg);
        s.clitMaxMmHg = num("clit_max_mmhg", s.clitMaxMmHg);
        s.feedBelowMmHg = num("feed_below_mmhg", s.feedBelowMmHg);
        s.ventAboveMmHg = num("vent_above_mmhg", s.ventAboveMmHg);
        s.feedPulseMs = static_cast<int>(num("feed_pulse_ms", s.feedPulseMs));
        s.averageWindowMs = static_cast<int>(num("average_window_ms", s.averageWindowMs));
        s.leakDropMmHg = num("leak_drop_mmhg", s.leakDropMmHg);
        s.leakRateMmHgPer100ms = num("leak_rate_mmhg_per_100ms", s.leakRateMmHgPer100ms);
        s.recoveryAttempts = static_cast<int>(num("recovery_attempts", s.recoveryAttempts));
        s.recoveryWindowMs = static_cast<int>(num("recovery_window_ms", s.recoveryWindowMs));
        s.benchMode = section.value("bench_mode").toBool(s.benchMode);
        return s;
    }
};

#endif // PNEUMATICSETTINGS_H
