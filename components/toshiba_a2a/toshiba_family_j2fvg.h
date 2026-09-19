#pragma once

#include "toshiba_family.h"

namespace esphome {
namespace toshiba_a2a {
namespace j2fvg {

namespace a3 {

inline constexpr uint8_t OFF = 0x31;
inline constexpr uint8_t VERTICAL_SWING = 0x41;
inline constexpr uint8_t FIX_BASE = 0x4F;

inline constexpr bool decode_fixed(uint8_t raw, uint8_t &vertical_index) {
  if (raw < 0x50 || raw > 0x54) return false;
  vertical_index = raw - FIX_BASE;
  return true;
}

inline constexpr uint8_t encode_fixed(uint8_t vertical_index) {
  return FIX_BASE + vertical_index;
}

}  // namespace a3

inline constexpr uint32_t FEATURES =
    FEATURE_COMMON_HVAC |
    FEATURE_ECO |
    FEATURE_HI_POWER |
    FEATURE_POWER_SELECT |
    FEATURE_OUTDOOR_SILENT |
    FEATURE_FIREPLACE |
    FEATURE_EIGHT_DEG_HEAT |
    FEATURE_VERTICAL_AIRFLOW |
    FEATURE_FIXED_POSITION |
    FEATURE_FLOOR |
    FEATURE_AIR_OUTLET_SELECT |
    FEATURE_START_DEFROST |
    FEATURE_STRONG_DEFROST;

inline constexpr ToshibaModeProfile MODE_PROFILES[] = {
    {
        ToshibaHvacMode::AUTO,
        {FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
    {
        ToshibaHvacMode::COOL,
        {FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
    {
        ToshibaHvacMode::HEAT,
        {FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT |
         FEATURE_FIREPLACE | FEATURE_EIGHT_DEG_HEAT | FEATURE_FLOOR |
         FEATURE_START_DEFROST | FEATURE_STRONG_DEFROST},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
    {
        ToshibaHvacMode::DRY,
        {FEATURE_POWER_SELECT},
        FAN_OPTION_AUTO,
    },
    {
        ToshibaHvacMode::FAN,
        {FEATURE_POWER_SELECT},
        FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET,
    },
};

static_assert(mode_profiles_within_family_capabilities(FEATURES, MODE_PROFILES),
              "J2FVG mode features must be a subset of family capabilities");

inline constexpr ToshibaFamilyProfile PROFILE{
    ToshibaIndoorUnitFamily::J2FVG,
    ToshibaLouvreEncoding::J2_VERTICAL,
    {FEATURES},
    MODE_PROFILES,
    sizeof(MODE_PROFILES) / sizeof(MODE_PROFILES[0]),
};

}  // namespace j2fvg
}  // namespace toshiba_a2a
}  // namespace esphome
