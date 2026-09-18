#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome {
namespace toshiba_output {

// Compact Toshiba indoor-fan data used by the thermal-output estimator.
//
// The control capability matrix is family-based elsewhere in the component.
// This table is deliberately exact-model based because air volume changes with
// capacity even when the control functionality is identical within a family.
//
// E4+2 is the live Toshiba IDU fan-speed feedback. Across the currently tested
// units it represents approximately fan RPM / 10 (for example 24 -> ~240 rpm,
// 60 -> ~600 rpm, and 103 -> ~1030 rpm). The higher values seen on the P2KVSGB
// high-wall unit are genuine higher blower speeds caused by its different fan/
// air-path geometry, not a different protocol scale.
//
// fan_min/fan_max therefore store the live E4+2 speed values used for the
// model-specific airflow mapping. A zero value remains the stopped-fan state.
//
// Sources:
//   J2FVG console: published Toshiba airflow data (cooling/heating).
//   P2KVSGB: provisional airflow endpoints. The observed E4+2 fan-speed envelope
//            is direct P2 evidence; the airflow endpoints remain provisional
//            until published P2 data or direct airflow calibration is available.

enum class AirflowMode : uint8_t { COOLING, HEATING };

enum class ManualFanLevel : uint8_t {
  LOW,
  LOW_MEDIUM,
  MEDIUM,
  MEDIUM_HIGH,
  HIGH,
};

struct ToshibaAirflowRange {
  const char *model;
  AirflowMode mode;
  float fan_min;
  float fan_max;
  uint16_t airflow_min_m3h;
  uint16_t airflow_max_m3h;
};

#define AFR(model, mode, fan_min, fan_max, flow_min, flow_max) \
  {model, AirflowMode::mode, fan_min, fan_max, flow_min, flow_max}

static constexpr ToshibaAirflowRange TOSHIBA_AIRFLOW_RANGES[] = {
    // J2FVG console family.
    AFR("RAS-B10J2FVG-E", COOLING, 24.0f, 53.0f, 198, 498),
    AFR("RAS-B10J2FVG-E", HEATING, 24.0f, 56.0f, 198, 528),
    AFR("RAS-B13J2FVG-E", COOLING, 24.0f, 56.0f, 198, 528),
    AFR("RAS-B13J2FVG-E", HEATING, 24.0f, 60.0f, 198, 570),

    // P2KVSGB provisional airflow range. Direct telemetry on the installed
    // RAS-B10P2KVSGB-E establishes an observed live E4+2 envelope of 51..103
    // (~510..1030 rpm). 103 has been reproduced at Fan-Only level 5 and under
    // the maximum-heating-fan condition. 51/52 has been sustained
    // during low-demand heating. Treat E4+2 as authoritative rather than
    // inferring fan speed from the requested fan/function setting. Airflow
    // endpoints remain provisional until published P2 data is available.
    AFR("RAS-B10P2KVSGB-E", COOLING, 51.0f, 103.0f, 312, 660),
    AFR("RAS-B10P2KVSGB-E", HEATING, 51.0f, 103.0f, 328, 660),
};

#undef AFR

inline bool airflow_model_matches(const char *reported, const char *table_model) {
  if (reported == nullptr || table_model == nullptr) return false;
  if (std::strcmp(reported, table_model) == 0) return true;

  // Some E0 payloads report the hardware revision as a trailing "1"
  // (e.g. RAS-B13J2FVG-E1) while published data names RAS-B13J2FVG-E.
  const size_t n = std::strlen(table_model);
  return std::strncmp(reported, table_model, n) == 0 && reported[n] == '1' && reported[n + 1] == '\0';
}

inline const ToshibaAirflowRange *find_airflow_range(const char *model, AirflowMode mode) {
  if (model == nullptr) return nullptr;
  for (const auto &entry : TOSHIBA_AIRFLOW_RANGES) {
    if (entry.mode == mode && airflow_model_matches(model, entry.model)) return &entry;
  }
  return nullptr;
}

inline bool interpolate_feedback_airflow(const char *model, AirflowMode mode,
                                         float fan_feedback, float &airflow_m3h) {
  const auto *range = find_airflow_range(model, mode);
  if (range == nullptr) return false;

  if (fan_feedback <= 0.0f) {
    airflow_m3h = 0.0f;
    return true;
  }

  const float span = range->fan_max - range->fan_min;
  if (span <= 0.0f) return false;

  const float ratio = (fan_feedback - range->fan_min) / span;
  airflow_m3h = static_cast<float>(range->airflow_min_m3h) +
                ratio * static_cast<float>(range->airflow_max_m3h - range->airflow_min_m3h);

  // Deliberately do not clamp to the configured min/max. Values outside the
  // known range remain visible so new fan points can be detected and the matrix
  // extended rather than silently hidden.
  if (airflow_m3h < 0.0f) airflow_m3h = 0.0f;
  return true;
}

inline bool estimate_manual_airflow(const char *model, AirflowMode mode,
                                    ManualFanLevel fan, float &airflow_m3h) {
  const auto *range = find_airflow_range(model, mode);
  if (range == nullptr) return false;

  // The five ESPHome manual fan settings are evenly distributed over the model
  // range. When live fan feedback is available it takes precedence, so this is
  // only the lightweight fallback for units/firmware without usable E4+2 data.
  float ratio = 0.0f;
  switch (fan) {
    case ManualFanLevel::LOW:
      ratio = 0.0f;
      break;
    case ManualFanLevel::LOW_MEDIUM:
      ratio = 0.25f;
      break;
    case ManualFanLevel::MEDIUM:
      ratio = 0.5f;
      break;
    case ManualFanLevel::MEDIUM_HIGH:
      ratio = 0.75f;
      break;
    case ManualFanLevel::HIGH:
      ratio = 1.0f;
      break;
  }

  airflow_m3h = static_cast<float>(range->airflow_min_m3h) +
                ratio * static_cast<float>(range->airflow_max_m3h - range->airflow_min_m3h);
  return true;
}

}  // namespace toshiba_output
}  // namespace esphome
