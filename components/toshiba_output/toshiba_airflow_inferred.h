#pragma once

#include <cstdint>
#include <cstring>
#include "toshiba_airflow_data.h"

namespace esphome {
namespace toshiba_output {

// Provisional airflow profile for RAS-B10P2KVSGB-E.
//
// Evidence:
// - Official product data for the installed black P2 B10 gives the same top-line
//   indoor airflow envelope as the B10 G3: approximately 310..660 m3/h.
// - Toshiba SVM-22104 gives the B10 G3 fixed MANUAL fan mapping explicitly.
//
// Until a P2 service manual is located, the normal (non-Hi-POWER) manual fan
// profile below deliberately mirrors the documented B10 G3 profile. It is an
// inference, not manufacturer-confirmed P2 per-level service data.
//
// Hi-POWER is deliberately not inferred here. SVM-22104 describes G3 Hi-POWER
// as increasing fan speed by one tap; without P2 service data, the estimator
// returns unavailable while Hi-POWER is active.

struct ToshibaInferredManualAirflow {
  const char *model;
  AirflowMode mode;
  ManualFanLevel fan;
  uint16_t airflow_m3h;
};

#define IAF(model, mode, fan, flow) {model, AirflowMode::mode, ManualFanLevel::fan, flow}

static constexpr ToshibaInferredManualAirflow TOSHIBA_INFERRED_MANUAL_AIRFLOW[] = {
    // B10 P2 black, provisional B10 G3-equivalent profile — cooling.
    // G3 SVM-22104 Fig. 1 / Table 1:
    // L=W7, M=WA, H=WD; L+ and M+ are midpoint interpolations.
    IAF("RAS-B10P2KVSGB-E", COOLING, LOW, 312),
    IAF("RAS-B10P2KVSGB-E", COOLING, LOW_MEDIUM, 378),
    IAF("RAS-B10P2KVSGB-E", COOLING, MEDIUM, 444),
    IAF("RAS-B10P2KVSGB-E", COOLING, MEDIUM_HIGH, 552),
    IAF("RAS-B10P2KVSGB-E", COOLING, HIGH, 660),

    // B10 P2 black, provisional B10 G3-equivalent profile — heating.
    // G3 SVM-22104 Fig. 3 / Table 1:
    // L=W8, M=WA, H=WE; L+ and M+ are midpoint interpolations.
    IAF("RAS-B10P2KVSGB-E", HEATING, LOW, 328),
    IAF("RAS-B10P2KVSGB-E", HEATING, LOW_MEDIUM, 386),
    IAF("RAS-B10P2KVSGB-E", HEATING, MEDIUM, 444),
    IAF("RAS-B10P2KVSGB-E", HEATING, MEDIUM_HIGH, 552),
    IAF("RAS-B10P2KVSGB-E", HEATING, HIGH, 660),
};

#undef IAF

inline const ToshibaInferredManualAirflow *find_inferred_manual_airflow(
    const char *model, AirflowMode mode, ManualFanLevel fan, bool hi_power) {
  if (model == nullptr || hi_power) return nullptr;
  for (const auto &entry : TOSHIBA_INFERRED_MANUAL_AIRFLOW) {
    if (entry.mode == mode && entry.fan == fan && std::strcmp(model, entry.model) == 0)
      return &entry;
  }
  return nullptr;
}

}  // namespace toshiba_output
}  // namespace esphome
