#pragma once

#include <cstdint>
#include <cstring>

namespace esphome {
namespace toshiba_output {

// Toshiba manufacturer indoor-fan data used by the thermal-output estimator.
//
// Sources:
//   J2FVG console: Toshiba Service Manual SVM-20012-1, Fig. 1 / Fig. 3 and
//                  tables 1 and 2, "Indoor fan air flow rate" (cooling/heating),
//                  pp. 34-36.
//   G3KVSG high wall: Toshiba Service Manual SVM-22104, "Indoor fan air flow rate".
//
// TOSHIBA_AIRFLOW_LEVELS preserves Toshiba's complete internal W1..WF tables.
// For B13J2FVG this yields 10 distinct documented cooling air-volume values and
// 11 distinct documented heating air-volume values; duplicate W levels are kept
// because the controller can select them separately.
//
// The manual fixed-speed table below additionally follows Toshiba's MANUAL-mode
// figures. Hi-POWER changes the endpoints used by L/M/H; L+ and M+ are explicitly
// defined by Toshiba as the midpoint of the adjacent manual fan speeds. Those
// midpoint air-volume values are therefore derived from Toshiba's documented
// interpolation rule, rather than being assigned to an arbitrary W level.
//
// P2KVSGB note: official product data for the installed RAS-B10P2KVSGB-E gives
// the same approximately 310..660 m3/h airflow envelope as the B10 G3. Until a
// P2 service manual is located, its normal fixed-speed profile below is inferred
// from the documented B10 G3 profile. This is deliberately marked in comments
// and Hi-POWER is not inferred for P2.

enum class AirflowMode : uint8_t { COOLING, HEATING };

enum class ManualFanLevel : uint8_t {
  LOW,
  LOW_MEDIUM,
  MEDIUM,
  MEDIUM_HIGH,
  HIGH,
};

struct ToshibaAirflowLevel {
  const char *model;
  AirflowMode mode;
  const char *level;
  uint16_t rpm;
  uint16_t airflow_m3h;
};

struct ToshibaManualAirflow {
  const char *model;
  AirflowMode mode;
  ManualFanLevel fan;
  bool hi_power;
  uint16_t airflow_m3h;
};

#define AF(model, mode, level, rpm, flow) {model, AirflowMode::mode, level, rpm, flow}

static constexpr ToshibaAirflowLevel TOSHIBA_AIRFLOW_LEVELS[] = {
    AF("RAS-B10J2FVG-E", COOLING, "WF", 530, 498),
    AF("RAS-B10J2FVG-E", COOLING, "WE", 530, 498),
    AF("RAS-B10J2FVG-E", COOLING, "WD", 530, 498),
    AF("RAS-B10J2FVG-E", COOLING, "WC", 480, 447),
    AF("RAS-B10J2FVG-E", COOLING, "WB", 450, 414),
    AF("RAS-B10J2FVG-E", COOLING, "WA", 400, 366),
    AF("RAS-B10J2FVG-E", COOLING, "W9", 360, 324),
    AF("RAS-B10J2FVG-E", COOLING, "W8", 350, 315),
    AF("RAS-B10J2FVG-E", COOLING, "W7", 300, 258),
    AF("RAS-B10J2FVG-E", COOLING, "W6", 260, 216),
    AF("RAS-B10J2FVG-E", COOLING, "W5", 260, 216),
    AF("RAS-B10J2FVG-E", COOLING, "W4", 240, 198),
    AF("RAS-B10J2FVG-E", COOLING, "W3", 240, 198),
    AF("RAS-B10J2FVG-E", COOLING, "W2", 240, 198),
    AF("RAS-B10J2FVG-E", COOLING, "W1", 240, 198),

    AF("RAS-B13J2FVG-E", COOLING, "WF", 560, 528),
    AF("RAS-B13J2FVG-E", COOLING, "WE", 560, 528),
    AF("RAS-B13J2FVG-E", COOLING, "WD", 550, 519),
    AF("RAS-B13J2FVG-E", COOLING, "WC", 500, 468),
    AF("RAS-B13J2FVG-E", COOLING, "WB", 490, 459),
    AF("RAS-B13J2FVG-E", COOLING, "WA", 440, 408),
    AF("RAS-B13J2FVG-E", COOLING, "W9", 390, 354),
    AF("RAS-B13J2FVG-E", COOLING, "W8", 390, 354),
    AF("RAS-B13J2FVG-E", COOLING, "W7", 340, 300),
    AF("RAS-B13J2FVG-E", COOLING, "W6", 270, 228),
    AF("RAS-B13J2FVG-E", COOLING, "W5", 270, 228),
    AF("RAS-B13J2FVG-E", COOLING, "W4", 250, 210),
    AF("RAS-B13J2FVG-E", COOLING, "W3", 240, 198),
    AF("RAS-B13J2FVG-E", COOLING, "W2", 240, 198),
    AF("RAS-B13J2FVG-E", COOLING, "W1", 240, 198),

    AF("RAS-B10J2FVG-E", HEATING, "WF", 560, 528),
    AF("RAS-B10J2FVG-E", HEATING, "WE", 560, 528),
    AF("RAS-B10J2FVG-E", HEATING, "WD", 480, 443),
    AF("RAS-B10J2FVG-E", HEATING, "WC", 440, 408),
    AF("RAS-B10J2FVG-E", HEATING, "WB", 400, 366),
    AF("RAS-B10J2FVG-E", HEATING, "WA", 380, 342),
    AF("RAS-B10J2FVG-E", HEATING, "W9", 370, 334),
    AF("RAS-B10J2FVG-E", HEATING, "W8", 320, 282),
    AF("RAS-B10J2FVG-E", HEATING, "W7", 260, 216),
    AF("RAS-B10J2FVG-E", HEATING, "W6", 260, 216),
    AF("RAS-B10J2FVG-E", HEATING, "W5", 260, 216),
    AF("RAS-B10J2FVG-E", HEATING, "W4", 260, 216),
    AF("RAS-B10J2FVG-E", HEATING, "W3", 260, 216),
    AF("RAS-B10J2FVG-E", HEATING, "W2", 240, 198),
    AF("RAS-B10J2FVG-E", HEATING, "W1", 240, 198),

    AF("RAS-B13J2FVG-E", HEATING, "WF", 600, 570),
    AF("RAS-B13J2FVG-E", HEATING, "WE", 580, 552),
    AF("RAS-B13J2FVG-E", HEATING, "WD", 520, 486),
    AF("RAS-B13J2FVG-E", HEATING, "WC", 470, 435),
    AF("RAS-B13J2FVG-E", HEATING, "WB", 460, 426),
    AF("RAS-B13J2FVG-E", HEATING, "WA", 410, 376),
    AF("RAS-B13J2FVG-E", HEATING, "W9", 400, 366),
    AF("RAS-B13J2FVG-E", HEATING, "W8", 340, 300),
    AF("RAS-B13J2FVG-E", HEATING, "W7", 270, 228),
    AF("RAS-B13J2FVG-E", HEATING, "W6", 270, 228),
    AF("RAS-B13J2FVG-E", HEATING, "W5", 270, 228),
    AF("RAS-B13J2FVG-E", HEATING, "W4", 270, 228),
    AF("RAS-B13J2FVG-E", HEATING, "W3", 270, 228),
    AF("RAS-B13J2FVG-E", HEATING, "W2", 250, 210),
    AF("RAS-B13J2FVG-E", HEATING, "W1", 240, 198),

    AF("RAS-B10G3KVSG-E", COOLING, "WF", 1030, 720),
    AF("RAS-B10G3KVSG-E", COOLING, "WE", 1030, 720),
    AF("RAS-B10G3KVSG-E", COOLING, "WD", 980, 660),
    AF("RAS-B10G3KVSG-E", COOLING, "WC", 880, 578),
    AF("RAS-B10G3KVSG-E", COOLING, "WB", 800, 540),
    AF("RAS-B10G3KVSG-E", COOLING, "WA", 700, 444),
    AF("RAS-B10G3KVSG-E", COOLING, "W9", 650, 394),
    AF("RAS-B10G3KVSG-E", COOLING, "W8", 630, 374),
    AF("RAS-B10G3KVSG-E", COOLING, "W7", 550, 312),
    AF("RAS-B10G3KVSG-E", COOLING, "W6", 530, 297),
    AF("RAS-B10G3KVSG-E", COOLING, "W5", 520, 290),
    AF("RAS-B10G3KVSG-E", COOLING, "W4", 510, 284),
    AF("RAS-B10G3KVSG-E", COOLING, "W3", 510, 284),
    AF("RAS-B10G3KVSG-E", COOLING, "W2", 500, 280),
    AF("RAS-B10G3KVSG-E", COOLING, "W1", 500, 280),

    AF("RAS-B10G3KVSG-E", HEATING, "WF", 980, 660),
    AF("RAS-B10G3KVSG-E", HEATING, "WE", 980, 660),
    AF("RAS-B10G3KVSG-E", HEATING, "WD", 900, 618),
    AF("RAS-B10G3KVSG-E", HEATING, "WC", 800, 540),
    AF("RAS-B10G3KVSG-E", HEATING, "WB", 740, 460),
    AF("RAS-B10G3KVSG-E", HEATING, "WA", 700, 444),
    AF("RAS-B10G3KVSG-E", HEATING, "W9", 650, 394),
    AF("RAS-B10G3KVSG-E", HEATING, "W8", 570, 328),
    AF("RAS-B10G3KVSG-E", HEATING, "W7", 550, 312),
    AF("RAS-B10G3KVSG-E", HEATING, "W6", 530, 297),
    AF("RAS-B10G3KVSG-E", HEATING, "W5", 520, 290),
    AF("RAS-B10G3KVSG-E", HEATING, "W4", 510, 284),
    AF("RAS-B10G3KVSG-E", HEATING, "W3", 510, 284),
    AF("RAS-B10G3KVSG-E", HEATING, "W2", 510, 284),
    AF("RAS-B10G3KVSG-E", HEATING, "W1", 500, 280),
};

#undef AF

#define MAF(model, mode, fan, hip, flow) {model, AirflowMode::mode, ManualFanLevel::fan, hip, flow}

static constexpr ToshibaManualAirflow TOSHIBA_MANUAL_AIRFLOW[] = {
    // J2FVG fixed manual fan settings. Toshiba Fig. 1 (cooling) and Fig. 3
    // (heating) explicitly alter L/M/H under Hi-POWER; L+ and M+ are midpoint
    // interpolations of adjacent documented speeds.

    // B10 cooling — normal then Hi-POWER.
    MAF("RAS-B10J2FVG-E", COOLING, LOW, false, 258),
    MAF("RAS-B10J2FVG-E", COOLING, LOW_MEDIUM, false, 312),
    MAF("RAS-B10J2FVG-E", COOLING, MEDIUM, false, 366),
    MAF("RAS-B10J2FVG-E", COOLING, MEDIUM_HIGH, false, 432),
    MAF("RAS-B10J2FVG-E", COOLING, HIGH, false, 498),
    MAF("RAS-B10J2FVG-E", COOLING, LOW, true, 315),
    MAF("RAS-B10J2FVG-E", COOLING, LOW_MEDIUM, true, 381),
    MAF("RAS-B10J2FVG-E", COOLING, MEDIUM, true, 447),
    MAF("RAS-B10J2FVG-E", COOLING, MEDIUM_HIGH, true, 473),
    MAF("RAS-B10J2FVG-E", COOLING, HIGH, true, 498),

    // B13 cooling — normal then Hi-POWER.
    MAF("RAS-B13J2FVG-E", COOLING, LOW, false, 300),
    MAF("RAS-B13J2FVG-E", COOLING, LOW_MEDIUM, false, 354),
    MAF("RAS-B13J2FVG-E", COOLING, MEDIUM, false, 408),
    MAF("RAS-B13J2FVG-E", COOLING, MEDIUM_HIGH, false, 464),
    MAF("RAS-B13J2FVG-E", COOLING, HIGH, false, 519),
    MAF("RAS-B13J2FVG-E", COOLING, LOW, true, 354),
    MAF("RAS-B13J2FVG-E", COOLING, LOW_MEDIUM, true, 411),
    MAF("RAS-B13J2FVG-E", COOLING, MEDIUM, true, 468),
    MAF("RAS-B13J2FVG-E", COOLING, MEDIUM_HIGH, true, 498),
    MAF("RAS-B13J2FVG-E", COOLING, HIGH, true, 528),

    // B10 heating — normal then Hi-POWER.
    MAF("RAS-B10J2FVG-E", HEATING, LOW, false, 282),
    MAF("RAS-B10J2FVG-E", HEATING, LOW_MEDIUM, false, 324),
    MAF("RAS-B10J2FVG-E", HEATING, MEDIUM, false, 366),
    MAF("RAS-B10J2FVG-E", HEATING, MEDIUM_HIGH, false, 447),
    MAF("RAS-B10J2FVG-E", HEATING, HIGH, false, 528),
    MAF("RAS-B10J2FVG-E", HEATING, LOW, true, 334),
    MAF("RAS-B10J2FVG-E", HEATING, LOW_MEDIUM, true, 389),
    MAF("RAS-B10J2FVG-E", HEATING, MEDIUM, true, 443),
    MAF("RAS-B10J2FVG-E", HEATING, MEDIUM_HIGH, true, 486),
    MAF("RAS-B10J2FVG-E", HEATING, HIGH, true, 528),

    // B13 heating — normal then Hi-POWER.
    MAF("RAS-B13J2FVG-E", HEATING, LOW, false, 300),
    MAF("RAS-B13J2FVG-E", HEATING, LOW_MEDIUM, false, 363),
    MAF("RAS-B13J2FVG-E", HEATING, MEDIUM, false, 426),
    MAF("RAS-B13J2FVG-E", HEATING, MEDIUM_HIGH, false, 489),
    MAF("RAS-B13J2FVG-E", HEATING, HIGH, false, 552),
    MAF("RAS-B13J2FVG-E", HEATING, LOW, true, 366),
    MAF("RAS-B13J2FVG-E", HEATING, LOW_MEDIUM, true, 426),
    MAF("RAS-B13J2FVG-E", HEATING, MEDIUM, true, 486),
    MAF("RAS-B13J2FVG-E", HEATING, MEDIUM_HIGH, true, 528),
    MAF("RAS-B13J2FVG-E", HEATING, HIGH, true, 570),

    // RAS-B10P2KVSGB-E — provisional normal fixed-speed profile inferred from
    // B10 G3 because Toshiba product data gives the same ~310..660 m3/h envelope.
    // These are not manufacturer-confirmed P2 per-level values. Hi-POWER is
    // intentionally omitted until P2 service data is available.
    MAF("RAS-B10P2KVSGB-E", COOLING, LOW, false, 312),
    MAF("RAS-B10P2KVSGB-E", COOLING, LOW_MEDIUM, false, 378),
    MAF("RAS-B10P2KVSGB-E", COOLING, MEDIUM, false, 444),
    MAF("RAS-B10P2KVSGB-E", COOLING, MEDIUM_HIGH, false, 552),
    MAF("RAS-B10P2KVSGB-E", COOLING, HIGH, false, 660),
    MAF("RAS-B10P2KVSGB-E", HEATING, LOW, false, 328),
    MAF("RAS-B10P2KVSGB-E", HEATING, LOW_MEDIUM, false, 386),
    MAF("RAS-B10P2KVSGB-E", HEATING, MEDIUM, false, 444),
    MAF("RAS-B10P2KVSGB-E", HEATING, MEDIUM_HIGH, false, 552),
    MAF("RAS-B10P2KVSGB-E", HEATING, HIGH, false, 660),
};

#undef MAF

inline bool airflow_model_matches(const char *reported, const char *table_model) {
  if (reported == nullptr || table_model == nullptr) return false;
  if (std::strcmp(reported, table_model) == 0) return true;

  // Some E0 payloads report the hardware revision as a trailing "1"
  // (e.g. RAS-B13J2FVG-E1) while Toshiba service data names RAS-B13J2FVG-E.
  const size_t n = std::strlen(table_model);
  return std::strncmp(reported, table_model, n) == 0 && reported[n] == '1' && reported[n + 1] == '\0';
}

inline const ToshibaAirflowLevel *find_airflow_level(const char *model, AirflowMode mode,
                                                       const char *level) {
  if (model == nullptr || level == nullptr) return nullptr;
  for (const auto &entry : TOSHIBA_AIRFLOW_LEVELS) {
    if (entry.mode == mode && airflow_model_matches(model, entry.model) &&
        std::strcmp(entry.level, level) == 0)
      return &entry;
  }
  return nullptr;
}

inline const ToshibaManualAirflow *find_manual_airflow(const char *model, AirflowMode mode,
                                                        ManualFanLevel fan, bool hi_power) {
  if (model == nullptr) return nullptr;
  for (const auto &entry : TOSHIBA_MANUAL_AIRFLOW) {
    if (entry.mode == mode && entry.fan == fan && entry.hi_power == hi_power &&
        airflow_model_matches(model, entry.model))
      return &entry;
  }
  return nullptr;
}

}  // namespace toshiba_output
}  // namespace esphome
