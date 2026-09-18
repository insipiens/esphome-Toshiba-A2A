#pragma once

#include "toshiba_family.h"

namespace esphome {
namespace toshiba_a2a {
namespace p2kvsg {

namespace a3 {

inline constexpr uint8_t STATE_OFF = 0x31;
inline constexpr uint8_t STATE_VERTICAL = 0x41;
inline constexpr uint8_t STATE_HORIZONTAL = 0x42;
inline constexpr uint8_t STATE_BOTH = 0x43;
inline constexpr uint8_t HADA = 0x60;

inline constexpr uint8_t CMD_OFF_TRANSITION = 0x80;
inline constexpr uint8_t CMD_BOTH_SWING = 0x80;
inline constexpr uint8_t CMD_VERTICAL_SWING = 0xAE;
inline constexpr uint8_t CMD_HORIZONTAL_SWING = 0xB6;

inline constexpr bool decode_packed_fix(uint8_t raw,
                                        uint8_t &horizontal_index,
                                        uint8_t &vertical_index) {
  if ((raw & 0xC0) != 0x80) return false;
  horizontal_index = (raw >> 3) & 0x07;
  vertical_index = raw & 0x07;
  return horizontal_index <= 5 && vertical_index <= 5;
}

inline constexpr uint8_t encode_packed_fix(uint8_t horizontal_index,
                                           uint8_t vertical_index) {
  return 0x80 | ((horizontal_index & 0x07) << 3) |
         (vertical_index & 0x07);
}

}  // namespace a3

inline constexpr uint32_t FEATURES =
    FEATURE_COMMON_HVAC |
    FEATURE_ECO |
    FEATURE_HI_POWER |
    FEATURE_COMFORT_SLEEP |
    FEATURE_POWER_SELECT |
    FEATURE_OUTDOOR_SILENT |
    FEATURE_FIREPLACE |
    FEATURE_EIGHT_DEG_HEAT |
    FEATURE_VERTICAL_AIRFLOW |
    FEATURE_FIXED_POSITION |
    FEATURE_SLEEP |
    FEATURE_COMFORT |
    FEATURE_HORIZONTAL_AIRFLOW |
    FEATURE_PURE |
    FEATURE_START_DEFROST;

inline constexpr ToshibaModeProfile MODE_PROFILES[] = {
    {
        ToshibaHvacMode::AUTO,
        {FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_PURE | FEATURE_START_DEFROST},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
    {
        ToshibaHvacMode::COOL,
        {FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_PURE},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
    {
        ToshibaHvacMode::HEAT,
        {FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_PURE |
         FEATURE_EIGHT_DEG_HEAT | FEATURE_START_DEFROST},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
    {
        ToshibaHvacMode::DRY,
        {FEATURE_POWER_SELECT | FEATURE_PURE},
        FAN_OPTION_AUTO,
    },
    {
        ToshibaHvacMode::FAN,
        {FEATURE_POWER_SELECT | FEATURE_PURE},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
};

inline constexpr ToshibaFamilyProfile PROFILE{
    ToshibaIndoorUnitFamily::P2KVSG,
    ToshibaLouvreEncoding::P2_PACKED,
    {FEATURES},
    MODE_PROFILES,
    sizeof(MODE_PROFILES) / sizeof(MODE_PROFILES[0]),
};

}  // namespace p2kvsg
}  // namespace toshiba_a2a
}  // namespace esphome
