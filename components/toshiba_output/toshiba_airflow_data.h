#pragma once

#include <cstdint>
#include <cstring>

namespace esphome {
namespace toshiba_output {

// Toshiba manufacturer indoor-fan data used by the thermal-output estimator.
//
// Sources:
//   J2FVG console: Toshiba Service Manual SVM-20012-1, tables 1 and 2,
//                  "Indoor fan air flow rate" (cooling/heating), pp. 34/36.
//   G3KVSG high wall: Toshiba Service Manual SVM-22104, table 1,
//                     "Indoor fan air flow rate".
//
// Values below are manufacturer air-volume rates in m3/h. The W1..WF names are
// Toshiba's internal fan-speed levels. Keeping the complete tables here avoids
// inventing/interpolating values and allows E4 fan-feedback mapping to be added
// once its relationship to Toshiba's W levels is established.
//
// P2KVSG note: currently available Toshiba product literature gives only the
// overall airflow range (for example B10P2KVSG-E: 310..660 m3/h), not the
// per-fan-level service table. P2 data is therefore deliberately NOT fabricated
// here; add it only from an authoritative per-level Toshiba table.

enum class AirflowMode : uint8_t { COOLING, HEATING };

struct ToshibaAirflowLevel {
  const char *model;
  AirflowMode mode;
  const char *level;
  uint16_t rpm;
  uint16_t airflow_m3h;
};

#define AF(model, mode, level, rpm, flow) {model, AirflowMode::mode, level, rpm, flow}

static constexpr ToshibaAirflowLevel TOSHIBA_AIRFLOW_LEVELS[] = {
    // RAS-B10J2FVG-E — SVM-20012-1 table 1, cooling.
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

    // RAS-B13J2FVG-E — SVM-20012-1 table 1, cooling.
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

    // RAS-B10J2FVG-E — SVM-20012-1 table 2, heating.
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

    // RAS-B13J2FVG-E — SVM-20012-1 table 2, heating.
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

    // G3KVSG B07/B10 share the SVM-22104 table. The complete per-level
    // values are retained here for the directly relevant B10 family.
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

inline const ToshibaAirflowLevel *find_airflow_level(const char *model, AirflowMode mode,
                                                       const char *level) {
  if (model == nullptr || level == nullptr) return nullptr;
  for (const auto &entry : TOSHIBA_AIRFLOW_LEVELS) {
    if (entry.mode == mode && std::strcmp(entry.model, model) == 0 &&
        std::strcmp(entry.level, level) == 0)
      return &entry;
  }
  return nullptr;
}

}  // namespace toshiba_output
}  // namespace esphome
